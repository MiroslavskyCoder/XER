#include "modules/module_builders.h"

#include <array>
#include <cstdio>
#include <string>
#include <sys/wait.h>
#include <vector>

namespace modules::detail {
namespace {

struct CommandResult {
	int exit_code = -1;
	std::string output;
	std::string error;
};

struct GitStatusEntry {
	char index_status = ' ';
	char work_tree_status = ' ';
	std::string path;
	std::string old_path;
	std::string raw;
};

bool RunCommandCapture(const std::string& command, CommandResult* result) {
	if (result == nullptr) {
		return false;
	}
	*result = CommandResult();
	FILE* pipe = popen(command.c_str(), "r");
	if (pipe == nullptr) {
		result->error = "popen failed";
		return false;
	}
	std::array<char, 4096> buffer {};
	while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr) {
		result->output += buffer.data();
	}
	const int status = pclose(pipe);
	if (status == -1) {
		result->error = "pclose failed";
		return false;
	}
	if (WIFEXITED(status)) {
		result->exit_code = WEXITSTATUS(status);
	} else {
		result->exit_code = status;
	}
	result->output = TrimWhitespace(result->output);
	return true;
}

std::string ResolveGitRepoPath(const v8::FunctionCallbackInfo<v8::Value>& args, int index) {
	return ResolveProjectRelativePath(OptionalStringArg(args, index, ResolveProjectRoot().string())).string();
}

std::string BuildGitCommand(const std::string& repo_path, const std::string& command) {
	return "git -C " + QuoteForShell(repo_path) + " " + command + " 2>&1";
}

std::string DescribeCommandFailure(const CommandResult& result, const std::string& fallback) {
	if (!result.output.empty()) {
		return result.output;
	}
	if (!result.error.empty()) {
		return result.error;
	}
	return fallback;
}

bool RunGitCommand(const std::string& repo_path,
			   const std::string& command,
			   CommandResult* result_out) {
	return RunCommandCapture(BuildGitCommand(repo_path, command), result_out);
}

bool IsGitRepositoryPath(const std::string& repo_path, CommandResult* probe_out) {
	CommandResult probe;
	RunGitCommand(repo_path, "rev-parse --is-inside-work-tree", &probe);
	if (probe_out != nullptr) {
		*probe_out = probe;
	}
	return probe.exit_code == 0;
}

bool RunGitCommandOrThrow(v8::Isolate* isolate,
				 const std::string& repo_path,
				 const std::string& command,
				 const char* fallback_error,
				 CommandResult* result_out) {
	CommandResult result;
	if (!RunGitCommand(repo_path, command, &result)) {
		Engine::Helper::ThrowError(isolate, DescribeCommandFailure(result, fallback_error));
		return false;
	}
	if (result.exit_code != 0) {
		Engine::Helper::ThrowError(isolate, DescribeCommandFailure(result, fallback_error));
		return false;
	}
	if (result_out != nullptr) {
		*result_out = std::move(result);
	}
	return true;
}

std::vector<GitStatusEntry> ParseGitStatusPorcelain(const std::string& status_porcelain) {
	std::vector<GitStatusEntry> entries;
	size_t start = 0;
	while (start < status_porcelain.size()) {
		const size_t end = status_porcelain.find('\n', start);
		const std::string line = end == std::string::npos
			? status_porcelain.substr(start)
			: status_porcelain.substr(start, end - start);
		start = end == std::string::npos ? status_porcelain.size() : end + 1u;
		if (line.empty()) {
			continue;
		}
		GitStatusEntry entry;
		entry.raw = line;
		entry.index_status = line.size() > 0 ? line[0] : ' ';
		entry.work_tree_status = line.size() > 1 ? line[1] : ' ';
		const std::string payload = line.size() > 3 ? TrimWhitespace(line.substr(3)) : std::string();
		const size_t rename_separator = payload.find(" -> ");
		if (rename_separator != std::string::npos) {
			entry.old_path = TrimWhitespace(payload.substr(0, rename_separator));
			entry.path = TrimWhitespace(payload.substr(rename_separator + 4u));
		} else {
			entry.path = payload;
		}
		entries.push_back(std::move(entry));
	}
	return entries;
}

v8::Local<v8::Object> MakeGitStatusEntryObject(v8::Isolate* isolate,
				       v8::Local<v8::Context> context,
				       const GitStatusEntry& entry) {
	v8::Local<v8::Object> object = v8::Object::New(isolate);
	SetProperty(isolate, context, object, "indexStatus", Engine::Helper::ToV8Str(isolate, std::string(1, entry.index_status)));
	SetProperty(isolate, context, object, "workTreeStatus", Engine::Helper::ToV8Str(isolate, std::string(1, entry.work_tree_status)));
	SetProperty(isolate, context, object, "path", Engine::Helper::ToV8Str(isolate, entry.path));
	SetProperty(isolate, context, object, "oldPath", Engine::Helper::ToV8Str(isolate, entry.old_path));
	SetProperty(isolate, context, object, "raw", Engine::Helper::ToV8Str(isolate, entry.raw));
	return object;
}

v8::Local<v8::Array> MakeGitStatusEntriesArray(v8::Isolate* isolate,
				       v8::Local<v8::Context> context,
				       const std::vector<GitStatusEntry>& entries) {
	v8::Local<v8::Array> array = v8::Array::New(isolate, static_cast<int>(entries.size()));
	for (size_t index = 0; index < entries.size(); ++index) {
		array->Set(context, static_cast<uint32_t>(index), MakeGitStatusEntryObject(isolate, context, entries[index])).FromMaybe(false);
	}
	return array;
}

void PopulateGitDescribeObject(v8::Isolate* isolate,
				       v8::Local<v8::Context> context,
				       v8::Local<v8::Object> object,
				       const std::string& repo_path,
				       bool is_repository,
				       const std::string& branch,
				       const std::string& head,
				       const std::string& status_porcelain,
				       const std::vector<GitStatusEntry>& status_entries,
				       const std::string& error_text) {
	SetProperty(isolate, context, object, "repoPath", Engine::Helper::ToV8Str(isolate, repo_path));
	SetProperty(isolate, context, object, "isRepository", v8::Boolean::New(isolate, is_repository));
	SetProperty(isolate, context, object, "branch", Engine::Helper::ToV8Str(isolate, branch));
	SetProperty(isolate, context, object, "head", Engine::Helper::ToV8Str(isolate, head));
	SetProperty(isolate, context, object, "headShort", Engine::Helper::ToV8Str(isolate, head.size() >= 7u ? head.substr(0, 7u) : head));
	SetProperty(isolate, context, object, "statusPorcelain", Engine::Helper::ToV8Str(isolate, status_porcelain));
	SetProperty(isolate, context, object, "statusEntryCount", v8::Number::New(isolate, static_cast<double>(status_entries.size())));
	SetProperty(isolate, context, object, "statusEntries", MakeGitStatusEntriesArray(isolate, context, status_entries));
	SetProperty(isolate, context, object, "dirty", v8::Boolean::New(isolate, !status_entries.empty()));
	SetProperty(isolate, context, object, "error", Engine::Helper::ToV8Str(isolate, error_text));
}

void GitDescribeCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	const std::string repo_path = ResolveGitRepoPath(args, 0);
	v8::Local<v8::Object> object = v8::Object::New(isolate);
	CommandResult inside_worktree;
	if (!IsGitRepositoryPath(repo_path, &inside_worktree)) {
		PopulateGitDescribeObject(
			isolate,
			context,
			object,
			repo_path,
			false,
			std::string(),
			std::string(),
			std::string(),
			{},
			DescribeCommandFailure(inside_worktree, "path is not a git repository"));
		args.GetReturnValue().Set(object);
		return;
	}

	CommandResult branch;
	CommandResult head;
	CommandResult status;
	std::string error_text;
	if (!RunGitCommand(repo_path, "branch --show-current", &branch) || branch.exit_code != 0) {
		error_text = DescribeCommandFailure(branch, "failed to resolve git branch");
	}
	if (!RunGitCommand(repo_path, "rev-parse HEAD", &head) || head.exit_code != 0) {
		if (error_text.empty()) {
			error_text = DescribeCommandFailure(head, "failed to resolve git HEAD");
		}
	}
	if (!RunGitCommand(repo_path, "status --porcelain", &status) || status.exit_code != 0) {
		if (error_text.empty()) {
			error_text = DescribeCommandFailure(status, "failed to read git status");
		}
	}
	const std::vector<GitStatusEntry> status_entries = status.exit_code == 0
		? ParseGitStatusPorcelain(status.output)
		: std::vector<GitStatusEntry>();
	PopulateGitDescribeObject(
		isolate,
		context,
		object,
		repo_path,
		true,
		branch.exit_code == 0 ? branch.output : std::string(),
		head.exit_code == 0 ? head.output : std::string(),
		status.exit_code == 0 ? status.output : std::string(),
		status_entries,
		error_text);
	args.GetReturnValue().Set(object);
}

void GitIsRepositoryCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	CommandResult probe;
	args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), IsGitRepositoryPath(ResolveGitRepoPath(args, 0), &probe)));
}

void GitCurrentBranchCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	CommandResult branch;
	if (!RunGitCommandOrThrow(isolate, ResolveGitRepoPath(args, 0), "branch --show-current", "failed to resolve git branch", &branch)) {
		return;
	}
	args.GetReturnValue().Set(Engine::Helper::ToV8Str(isolate, branch.output));
}

void GitHeadCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	CommandResult head;
	if (!RunGitCommandOrThrow(isolate, ResolveGitRepoPath(args, 0), "rev-parse HEAD", "failed to resolve git HEAD", &head)) {
		return;
	}
	args.GetReturnValue().Set(Engine::Helper::ToV8Str(isolate, head.output));
}

void GitStatusPorcelainCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	CommandResult status;
	if (!RunGitCommandOrThrow(isolate, ResolveGitRepoPath(args, 0), "status --porcelain", "failed to read git status", &status)) {
		return;
	}
	args.GetReturnValue().Set(Engine::Helper::ToV8Str(isolate, status.output));
}

void GitStatusCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	const std::string repo_path = ResolveGitRepoPath(args, 0);
	CommandResult status;
	if (!RunGitCommandOrThrow(isolate, repo_path, "status --porcelain", "failed to read git status", &status)) {
		return;
	}
	const std::vector<GitStatusEntry> entries = ParseGitStatusPorcelain(status.output);
	v8::Local<v8::Object> object = v8::Object::New(isolate);
	SetProperty(isolate, context, object, "repoPath", Engine::Helper::ToV8Str(isolate, repo_path));
	SetProperty(isolate, context, object, "statusPorcelain", Engine::Helper::ToV8Str(isolate, status.output));
	SetProperty(isolate, context, object, "entryCount", v8::Number::New(isolate, static_cast<double>(entries.size())));
	SetProperty(isolate, context, object, "entries", MakeGitStatusEntriesArray(isolate, context, entries));
	SetProperty(isolate, context, object, "dirty", v8::Boolean::New(isolate, !entries.empty()));
	args.GetReturnValue().Set(object);
}

}  // namespace

bool BuildGitModule(v8::Isolate* isolate,
			v8::Local<v8::Context> context,
			v8::Local<v8::Object>* module_out,
			std::string* error_out) {
	v8::Local<v8::Object> module = v8::Object::New(isolate);
	bool ok = true;
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "describe", &GitDescribeCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "isRepository", &GitIsRepositoryCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "currentBranch", &GitCurrentBranchCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "head", &GitHeadCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "statusPorcelain", &GitStatusPorcelainCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "status", &GitStatusCallback);
	if (!ok) {
		if (error_out != nullptr) {
			*error_out = "failed to build Git module";
		}
		return false;
	}
	*module_out = module;
	return true;
}

}  // namespace modules::detail