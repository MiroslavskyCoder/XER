#include "modules/git_module.h"

#include <array>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <sstream>
#include <string>
#include <vector>

namespace {

std::string ValueToString(v8::Isolate* isolate, v8::Local<v8::Value> value) {
    v8::String::Utf8Value utf8(isolate, value);
    if (*utf8 == nullptr) {
        return std::string();
    }
    return *utf8;
}

std::string EscapeSingleQuotes(const std::string& value) {
    std::string escaped;
    escaped.reserve(value.size() + 8);
    for (char c : value) {
        if (c == '\'') {
            escaped += "'\\''";
        } else {
            escaped.push_back(c);
        }
    }
    return escaped;
}

std::string BuildShellCommand(const std::string& command, const std::string& cwd) {
    if (cwd.empty()) {
        return command;
    }
    return "cd '" + EscapeSingleQuotes(cwd) + "' && " + command;
}

bool ExecCapture(const std::string& command, const std::string& cwd, std::string* output, int* status_code) {
    *output = std::string();
    *status_code = -1;

    const std::string shell_command = BuildShellCommand(command, cwd) + " 2>&1";
    FILE* pipe = popen(shell_command.c_str(), "r");
    if (pipe == nullptr) {
        return false;
    }

    std::array<char, 4096> buffer{};
    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr) {
        output->append(buffer.data());
    }

    const int code = pclose(pipe);
    *status_code = code;
    return true;
}

v8::Local<v8::Object> BuildResultObject(v8::Isolate* isolate,
                                        v8::Local<v8::Context> context,
                                        int status_code,
                                        const std::string& output) {
    v8::Local<v8::Object> result = v8::Object::New(isolate);
    (void)result
        ->Set(context,
              v8::String::NewFromUtf8Literal(isolate, "code"),
              v8::Integer::New(isolate, status_code))
        .FromMaybe(false);
    (void)result
        ->Set(context,
              v8::String::NewFromUtf8Literal(isolate, "ok"),
              v8::Boolean::New(isolate, status_code == 0))
        .FromMaybe(false);
    (void)result
        ->Set(context,
              v8::String::NewFromUtf8Literal(isolate, "output"),
              v8::String::NewFromUtf8(isolate, output.c_str()).ToLocalChecked())
        .FromMaybe(false);
    return result;
}

std::string ReadOptionalCwd(v8::Isolate* isolate,
                            v8::Local<v8::Context> context,
                            const v8::FunctionCallbackInfo<v8::Value>& args,
                            int index) {
    if (args.Length() <= index) {
        return std::string();
    }
    if (!args[index]->IsString()) {
        return std::string();
    }
    return ValueToString(isolate, args[index]);
}

void ExecCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();

    if (args.Length() < 1 || !args[0]->IsString()) {
        isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8Literal(isolate, "Git.exec expects command string")));
        return;
    }

    const std::string command = ValueToString(isolate, args[0]);
    const std::string cwd = ReadOptionalCwd(isolate, context, args, 1);

    std::string output;
    int code = -1;
    if (!ExecCapture("git " + command, cwd, &output, &code)) {
        isolate->ThrowException(v8::Exception::Error(
            v8::String::NewFromUtf8Literal(isolate, "Failed to execute git command")));
        return;
    }

    args.GetReturnValue().Set(BuildResultObject(isolate, context, code, output));
}

void IsRepoCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();

    const std::string cwd = ReadOptionalCwd(isolate, context, args, 0);

    std::string output;
    int code = -1;
    if (!ExecCapture("git rev-parse --is-inside-work-tree", cwd, &output, &code)) {
        args.GetReturnValue().Set(v8::Boolean::New(isolate, false));
        return;
    }

    args.GetReturnValue().Set(v8::Boolean::New(isolate, code == 0));
}

void CurrentBranchCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();

    const std::string cwd = ReadOptionalCwd(isolate, context, args, 0);

    std::string output;
    int code = -1;
    if (!ExecCapture("git rev-parse --abbrev-ref HEAD", cwd, &output, &code) || code != 0) {
        args.GetReturnValue().Set(v8::String::NewFromUtf8Literal(isolate, ""));
        return;
    }

    while (!output.empty() && (output.back() == '\n' || output.back() == '\r')) {
        output.pop_back();
    }
    args.GetReturnValue().Set(v8::String::NewFromUtf8(isolate, output.c_str()).ToLocalChecked());
}

void CurrentCommitCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();

    const std::string cwd = ReadOptionalCwd(isolate, context, args, 0);

    std::string output;
    int code = -1;
    if (!ExecCapture("git rev-parse HEAD", cwd, &output, &code) || code != 0) {
        args.GetReturnValue().Set(v8::String::NewFromUtf8Literal(isolate, ""));
        return;
    }

    while (!output.empty() && (output.back() == '\n' || output.back() == '\r')) {
        output.pop_back();
    }
    args.GetReturnValue().Set(v8::String::NewFromUtf8(isolate, output.c_str()).ToLocalChecked());
}

void StatusShortCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();

    const std::string cwd = ReadOptionalCwd(isolate, context, args, 0);

    std::string output;
    int code = -1;
    if (!ExecCapture("git status --short", cwd, &output, &code)) {
        args.GetReturnValue().Set(v8::String::NewFromUtf8Literal(isolate, ""));
        return;
    }

    args.GetReturnValue().Set(v8::String::NewFromUtf8(isolate, output.c_str()).ToLocalChecked());
}

void ChangedFilesCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();

    const std::string cwd = ReadOptionalCwd(isolate, context, args, 0);

    std::string output;
    int code = -1;
    if (!ExecCapture("git status --porcelain", cwd, &output, &code) || code != 0) {
        args.GetReturnValue().Set(v8::Array::New(isolate));
        return;
    }

    std::istringstream stream(output);
    std::string line;
    std::vector<std::string> files;
    while (std::getline(stream, line)) {
        if (line.size() < 4) {
            continue;
        }
        files.push_back(line.substr(3));
    }

    v8::Local<v8::Array> array = v8::Array::New(isolate, static_cast<int>(files.size()));
    for (uint32_t i = 0; i < files.size(); ++i) {
        (void)array
            ->Set(context,
                  i,
                  v8::String::NewFromUtf8(isolate, files[i].c_str()).ToLocalChecked())
            .FromMaybe(false);
    }

    args.GetReturnValue().Set(array);
}

void LastCommitMessageCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();

    const std::string cwd = ReadOptionalCwd(isolate, context, args, 0);

    std::string output;
    int code = -1;
    if (!ExecCapture("git log -1 --pretty=%B", cwd, &output, &code) || code != 0) {
        args.GetReturnValue().Set(v8::String::NewFromUtf8Literal(isolate, ""));
        return;
    }

    while (!output.empty() && (output.back() == '\n' || output.back() == '\r')) {
        output.pop_back();
    }
    args.GetReturnValue().Set(v8::String::NewFromUtf8(isolate, output.c_str()).ToLocalChecked());
}

void CloneCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();

    if (args.Length() < 2 || !args[0]->IsString() || !args[1]->IsString()) {
        isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8Literal(isolate, "Git.clone expects url and path")));
        return;
    }

    const std::string url = ValueToString(isolate, args[0]);
    const std::string dir = ValueToString(isolate, args[1]);

    std::string output;
    int code = -1;
    if (!ExecCapture("git clone '" + EscapeSingleQuotes(url) + "' '" + EscapeSingleQuotes(dir) + "'",
                     std::string(),
                     &output,
                     &code)) {
        isolate->ThrowException(v8::Exception::Error(
            v8::String::NewFromUtf8Literal(isolate, "Failed to run git clone")));
        return;
    }

    args.GetReturnValue().Set(BuildResultObject(isolate, context, code, output));
}

void PullCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();

    const std::string cwd = ReadOptionalCwd(isolate, context, args, 0);
    std::string output;
    int code = -1;
    if (!ExecCapture("git pull", cwd, &output, &code)) {
        isolate->ThrowException(v8::Exception::Error(
            v8::String::NewFromUtf8Literal(isolate, "Failed to run git pull")));
        return;
    }

    args.GetReturnValue().Set(BuildResultObject(isolate, context, code, output));
}

void CheckoutCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();

    if (args.Length() < 1 || !args[0]->IsString()) {
        isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8Literal(isolate, "Git.checkout expects ref")));
        return;
    }

    const std::string ref = ValueToString(isolate, args[0]);
    const std::string cwd = ReadOptionalCwd(isolate, context, args, 1);

    std::string output;
    int code = -1;
    if (!ExecCapture("git checkout '" + EscapeSingleQuotes(ref) + "'", cwd, &output, &code)) {
        isolate->ThrowException(v8::Exception::Error(
            v8::String::NewFromUtf8Literal(isolate, "Failed to run git checkout")));
        return;
    }

    args.GetReturnValue().Set(BuildResultObject(isolate, context, code, output));
}

}  // namespace

namespace modules {

bool RegisterGitModule(v8::Isolate* isolate, v8::Local<v8::Context> context) {
    v8::HandleScope handle_scope(isolate);

    v8::Local<v8::Object> git = v8::Object::New(isolate);
    bool ok = git
                  ->Set(context,
                        v8::String::NewFromUtf8Literal(isolate, "exec"),
                        v8::Function::New(context, ExecCallback).ToLocalChecked())
                  .FromMaybe(false);
    ok = ok && git
                   ->Set(context,
                         v8::String::NewFromUtf8Literal(isolate, "isRepo"),
                         v8::Function::New(context, IsRepoCallback).ToLocalChecked())
                   .FromMaybe(false);
    ok = ok && git
                   ->Set(context,
                         v8::String::NewFromUtf8Literal(isolate, "currentBranch"),
                         v8::Function::New(context, CurrentBranchCallback).ToLocalChecked())
                   .FromMaybe(false);
    ok = ok && git
                   ->Set(context,
                         v8::String::NewFromUtf8Literal(isolate, "currentCommit"),
                         v8::Function::New(context, CurrentCommitCallback).ToLocalChecked())
                   .FromMaybe(false);
    ok = ok && git
                   ->Set(context,
                         v8::String::NewFromUtf8Literal(isolate, "statusShort"),
                         v8::Function::New(context, StatusShortCallback).ToLocalChecked())
                   .FromMaybe(false);
    ok = ok && git
                   ->Set(context,
                         v8::String::NewFromUtf8Literal(isolate, "changedFiles"),
                         v8::Function::New(context, ChangedFilesCallback).ToLocalChecked())
                   .FromMaybe(false);
    ok = ok && git
                   ->Set(context,
                         v8::String::NewFromUtf8Literal(isolate, "lastCommitMessage"),
                         v8::Function::New(context, LastCommitMessageCallback).ToLocalChecked())
                   .FromMaybe(false);
    ok = ok && git
                   ->Set(context,
                         v8::String::NewFromUtf8Literal(isolate, "clone"),
                         v8::Function::New(context, CloneCallback).ToLocalChecked())
                   .FromMaybe(false);
    ok = ok && git
                   ->Set(context,
                         v8::String::NewFromUtf8Literal(isolate, "pull"),
                         v8::Function::New(context, PullCallback).ToLocalChecked())
                   .FromMaybe(false);
    ok = ok && git
                   ->Set(context,
                         v8::String::NewFromUtf8Literal(isolate, "checkout"),
                         v8::Function::New(context, CheckoutCallback).ToLocalChecked())
                   .FromMaybe(false);

    if (!ok) {
        return false;
    }

    return context->Global()
        ->Set(context, v8::String::NewFromUtf8Literal(isolate, "Git"), git)
        .FromMaybe(false);
}

}  // namespace modules
