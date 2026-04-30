#include "compiler_source.h"

#include "runtime_live.h"

#include <cctype>
#include <cstdlib>
#include <sstream>
#include <utility>

CompilerSource::CompilerSource(std::string source,
							   std::string cache_dir,
							   std::string provider,
							   std::string language,
							   std::string compile_flags,
							   std::string link_flags,
							   std::vector<std::string> source_files)
	: source_(std::move(source)),
	  cache_dir_(std::move(cache_dir)),
	  provider_(std::move(provider)),
	  language_(std::move(language)),
	  compile_flags_(std::move(compile_flags)),
	  link_flags_(std::move(link_flags)),
	  source_files_(std::move(source_files)) {}

const std::string& CompilerSource::source() const {
	return source_;
}

std::string CompilerSource::source_with_stdio() const {
	if (source_.find("#include <stdio.h>") != std::string::npos) {
		return source_;
	}

	return std::string("#include <stdio.h>\n") + source_;
}

bool CompilerSource::use_cpp() const {
	return language_ == "cpp" || language_ == "cxx" || language_ == "c++";
}

const std::filesystem::path& CompilerSource::cache_dir() const {
	return cache_dir_;
}

std::filesystem::path CompilerSource::source_path() const {
	return cache_dir_ / (use_cpp() ? "entry.cc" : "entry.c");
}

std::filesystem::path CompilerSource::binary_path() const {
	return cache_dir_ / "entry.out";
}

std::filesystem::path CompilerSource::compile_out_path() const {
	return cache_dir_ / "compile.out";
}

std::filesystem::path CompilerSource::compile_err_path() const {
	return cache_dir_ / "compile.err";
}

std::filesystem::path CompilerSource::run_out_path() const {
	return cache_dir_ / "run.out";
}

std::filesystem::path CompilerSource::run_err_path() const {
	return cache_dir_ / "run.err";
}

std::vector<std::string> CompilerSource::BuildCompilerArgs() const {
	std::vector<std::string> args;
	args.push_back(use_cpp() ? "-std=c++17" : "-std=c11");
	args.push_back("-O2");
	args.push_back(BuildProviderDefine());

	for (const std::string& flag : SplitFlags(compile_flags_)) {
		args.push_back(flag);
	}

	if (source_files_.empty()) {
		args.push_back(source_path().string());
	} else {
		for (const std::string& file_name : source_files_) {
			args.push_back((cache_dir_ / file_name).string());
		}
		args.push_back("-I");
		args.push_back(cache_dir_.string());
	}
	args.push_back("-o");
	args.push_back(binary_path().string());

	for (const std::string& flag : SplitFlags(link_flags_)) {
		args.push_back(flag);
	}

	return args;
}

std::string CompilerSource::BuildCompilerCommandPreview(const std::string& compiler_binary) const {
	std::ostringstream out;
	out << QuoteForShell(compiler_binary);
	for (const std::string& arg : BuildCompilerArgs()) {
		out << " " << QuoteForShell(arg);
	}
	return out.str();
}

std::vector<std::string> CompilerSource::SplitFlags(const std::string& flags) const {
	std::vector<std::string> out;
	std::string current;
	bool in_single_quotes = false;
	bool in_double_quotes = false;
	bool escaped = false;

	for (char ch : flags) {
		if (escaped) {
			current.push_back(ch);
			escaped = false;
			continue;
		}

		if (ch == '\\') {
			escaped = true;
			continue;
		}

		if (!in_double_quotes && ch == '\'') {
			in_single_quotes = !in_single_quotes;
			continue;
		}

		if (!in_single_quotes && ch == '"') {
			in_double_quotes = !in_double_quotes;
			continue;
		}

		if (!in_single_quotes && !in_double_quotes && std::isspace(static_cast<unsigned char>(ch)) != 0) {
			if (!current.empty()) {
				out.push_back(current);
				current.clear();
			}
			continue;
		}

		current.push_back(ch);
	}

	if (!current.empty()) {
		out.push_back(current);
	}
	return out;
}

std::string CompilerSource::QuoteForShell(const std::string& token) const {
	if (token.empty()) {
		return "''";
	}

	bool needs_quote = false;
	for (char ch : token) {
		if (std::isspace(static_cast<unsigned char>(ch)) != 0 || ch == '\'' || ch == '"') {
			needs_quote = true;
			break;
		}
	}

	if (!needs_quote) {
		return token;
	}

	std::string out = "'";
	for (char ch : token) {
		if (ch == '\'') {
			out += "'\\''";
		} else {
			out.push_back(ch);
		}
	}
	out.push_back('\'');
	return out;
}

std::string CompilerSource::BuildProviderDefine() const {
	if (provider_ == RuntimeLive::kProviderGPUToolkit) {
		const char* env_vendor = std::getenv("ENGINE_BUILDER_GPU_VENDOR");
		const std::string vendor = (env_vendor != nullptr && env_vendor[0] != '\0')
									   ? std::string(env_vendor)
									   : std::string("GenericGPU");
		return std::string("-DENGINE_BUILDER_GPU_VENDOR=\"") + vendor + "\"";
	}

	return "-DENGINE_BUILDER_GPU_VENDOR=\"NoGPUProvider\"";
}