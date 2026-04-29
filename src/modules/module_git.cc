#include "modules/module_builders.h"

#include <array>
#include <cstdio>
#include <string>
#include <sys/wait.h>

namespace modules::detail {
namespace {

struct CommandResult {
	int exit_code = -1;
	std::string output;
	std::string error;
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

void GitDescribeCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	const std::string repo_path = ResolveGitRepoPath(args, 0);
	CommandResult inside_worktree;
	CommandResult branch;
	CommandResult head;
	CommandResult status;
	RunCommandCapture(BuildGitCommand(repo_path, "rev-parse --is-inside-work-tree"), &inside_worktree);
	RunCommandCapture(BuildGitCommand(repo_path, "branch --show-current"), &branch);
	RunCommandCapture(BuildGitCommand(repo_path, "rev-parse HEAD"), &head);
	RunCommandCapture(BuildGitCommand(repo_path, "status --porcelain"), &status);
	v8::Local<v8::Object> object = v8::Object::New(isolate);
	SetProperty(isolate, context, object, "repoPath", Engine::Helper::ToV8Str(isolate, repo_path));
	SetProperty(isolate, context, object, "isRepository", v8::Boolean::New(isolate, inside_worktree.exit_code == 0));
	SetProperty(isolate, context, object, "branch", Engine::Helper::ToV8Str(isolate, branch.output));
	SetProperty(isolate, context, object, "head", Engine::Helper::ToV8Str(isolate, head.output));
	SetProperty(isolate, context, object, "statusPorcelain", Engine::Helper::ToV8Str(isolate, status.output));
	args.GetReturnValue().Set(object);
}

void GitCurrentBranchCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	CommandResult branch;
	RunCommandCapture(BuildGitCommand(ResolveGitRepoPath(args, 0), "branch --show-current"), &branch);
	args.GetReturnValue().Set(Engine::Helper::ToV8Str(args.GetIsolate(), branch.output));
}

void GitHeadCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	CommandResult head;
	RunCommandCapture(BuildGitCommand(ResolveGitRepoPath(args, 0), "rev-parse HEAD"), &head);
	args.GetReturnValue().Set(Engine::Helper::ToV8Str(args.GetIsolate(), head.output));
}

void GitStatusPorcelainCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	CommandResult status;
	RunCommandCapture(BuildGitCommand(ResolveGitRepoPath(args, 0), "status --porcelain"), &status);
	args.GetReturnValue().Set(Engine::Helper::ToV8Str(args.GetIsolate(), status.output));
}

}  // namespace

bool BuildGitModule(v8::Isolate* isolate,
			v8::Local<v8::Context> context,
			v8::Local<v8::Object>* module_out,
			std::string* error_out) {
	v8::Local<v8::Object> module = v8::Object::New(isolate);
	bool ok = true;
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "describe", &GitDescribeCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "currentBranch", &GitCurrentBranchCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "head", &GitHeadCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "statusPorcelain", &GitStatusPorcelainCallback);
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