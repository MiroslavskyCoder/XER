
#include <filesystem>
#include <fstream>
#include <iostream>
#include <range/v3/range/conversion.hpp>
#include <range/v3/view/transform.hpp>
#include <sstream>
#include <string>
#include <cstdlib>
#include <vector>

#include "app_command.h"
#include "crash/crash_handler.h"
#include "ecosystem/ecosystem_manifest_loader.h"
#include "engine_params.h"
#include "flowscript/flow_script.h"
#include "provider.h"
#include "runtime_live.h"
#include "watch/watch_live_updatex_script.h"
#include "xer/xer_encode.h"
#include "xer/xer_script.h"

namespace {

void SetEnvValue(const char* key, const std::string& value) {
	setenv(key, value.c_str(), 1);
}

void SetEnvFlag(const char* key, bool enabled) {
	if (!enabled) {
		return;
	}
	setenv(key, "1", 1);
}

// Parse a simple KEY=VALUE .env file and set each variable in the environment.
// Lines starting with '#' and empty lines are ignored.
void LoadEnvFile(const std::string& path) {
	std::ifstream f(path);
	if (!f.is_open()) {
		std::cerr << "[warn] --env_file not found: " << path << "\n";
		return;
	}
	std::string line;
	while (std::getline(f, line)) {
		if (line.empty() || line[0] == '#') continue;
		const auto eq = line.find('=');
		if (eq == std::string::npos) continue;
		const std::string key   = line.substr(0, eq);
		const std::string value = line.substr(eq + 1);
		// Do not overwrite existing env (0 = no-overwrite).
		setenv(key.c_str(), value.c_str(), 0);
	}
}

void ExportParsedToEnv(const AppCommand::Parsed& p) {
	// Script
	SetEnvValue("ENGINE_SCRIPT_PATH", p.script_path);

	// Security
	SetEnvFlag("ENGINE_SANDBOX", p.sandbox);

	// Emit
	SetEnvFlag("ENGINE_NOEMIT",          p.noemit);
	SetEnvFlag("ENGINE_EMIT_SOURCE_MAP", p.emit_source_map);
	if (!p.xer_key.empty()) SetEnvValue("ENGINE_XER_KEY", p.xer_key);
	if (!p.xer_key_file.empty()) SetEnvValue("ENGINE_XER_KEY_FILE", p.xer_key_file);
	if (!p.xer_key_env.empty()) SetEnvValue("ENGINE_XER_KEY_ENV", p.xer_key_env);

	// Compiler / logging
	SetEnvFlag("ENGINE_DETAILS_COMPILER", p.details_compiler);
	SetEnvFlag("ENGINE_VERBOSE",          p.verbose);
	if (!p.log_level.empty()) SetEnvValue("ENGINE_LOG_LEVEL", p.log_level);
	if (!p.log_file.empty())  SetEnvValue("ENGINE_LOG_FILE",  p.log_file);

	// Debug
	SetEnvFlag("ENGINE_DEBUG",              p.debug);
	SetEnvFlag("ENGINE_INSPECT",            p.inspect);
	SetEnvFlag("ENGINE_BREAK_ON_FIRST_LINE", p.break_on_first_line);
	SetEnvFlag("ENGINE_DUMP_AST",           p.dump_ast);
	SetEnvFlag("ENGINE_DUMP_BYTECODE",      p.dump_bytecode);
	if (p.inspect) SetEnvValue("ENGINE_INSPECT_PORT", std::to_string(p.inspect_port));

	// Cache
	SetEnvFlag("ENGINE_REQUIRE_NO_CACHE", p.nocacherequire);
	SetEnvFlag("ENGINE_CACHE_CLEAN",      p.cache_clean);
	SetEnvFlag("ENGINE_CACHE_READONLY",   p.cache_readonly);
	if (!p.cache_dir.empty()) SetEnvValue("ENGINE_CACHE_DIR", p.cache_dir);

	// Diff
	SetEnvFlag("ENGINE_PRINT_DIFF", p.print_diff);
	SetEnvFlag("ENGINE_DIFF_ONLY",  p.diff_only);

	// Threading / resources
	if (p.max_cpu_threads > 0)      SetEnvValue("ENGINE_MAX_CPU_THREADS",      std::to_string(p.max_cpu_threads));
	if (p.max_memory_used > 0)      SetEnvValue("ENGINE_MAX_MEMORY_USED",      std::to_string(p.max_memory_used));
	if (p.timeout_seconds > 0)      SetEnvValue("ENGINE_TIMEOUT_SECONDS",      std::to_string(p.timeout_seconds));
	if (p.async_io_workers > 0)     SetEnvValue("ENGINE_ASYNC_IO_WORKERS",     std::to_string(p.async_io_workers));
	if (p.async_io_queue_depth > 0) SetEnvValue("ENGINE_ASYNC_IO_QUEUE_DEPTH", std::to_string(p.async_io_queue_depth));

	// CPU fine-grained
	if (!p.cpu_affinity.empty())      SetEnvValue("ENGINE_CPU_AFFINITY",         p.cpu_affinity);
	if (p.thread_priority != 0)       SetEnvValue("ENGINE_THREAD_PRIORITY",      std::to_string(p.thread_priority));
	if (p.v8_platform_workers > 0)    SetEnvValue("ENGINE_V8_PLATFORM_WORKERS",  std::to_string(p.v8_platform_workers));

	// Memory fine-grained
	if (p.memory_hard_limit_mib > 0)    SetEnvValue("ENGINE_MEMORY_HARD_LIMIT_MIB",         std::to_string(p.memory_hard_limit_mib));
	if (p.memory_warn_mib > 0)          SetEnvValue("ENGINE_MEMORY_WARN_MIB",               std::to_string(p.memory_warn_mib));
	if (p.memory_check_interval_ms > 0) SetEnvValue("ENGINE_RESOURCE_CHECK_INTERVAL_MS",    std::to_string(p.memory_check_interval_ms));

	// Output formatting
	SetEnvFlag("ENGINE_STACK_FORMATING_NO_COLOR", p.stack_formating_no_color);
	SetEnvFlag("ENGINE_COMPACT_ERRORS",           p.compact_errors);
	SetEnvFlag("ENGINE_NO_SOURCE_EXCERPT",        p.no_source_excerpt);
	SetEnvFlag("ENGINE_TIMESTAMPS",               p.timestamps);
	if (p.stack_formating_no_color) setenv("NO_COLOR", "1", 1);

	// Module system
	SetEnvFlag("ENGINE_STRICT_REQUIRE",       p.strict_require);
	SetEnvFlag("ENGINE_ALLOW_REMOTE_REQUIRE", p.allow_remote_require);
	if (!p.module_root.empty()) SetEnvValue("ENGINE_MODULE_ROOT", p.module_root);

	// TypeScript
	if (!p.ts_compiler.empty()) SetEnvValue("ENGINE_TS_COMPILER", p.ts_compiler);
	SetEnvFlag("ENGINE_TS_STRICT",   p.ts_strict);
	SetEnvFlag("ENGINE_TS_NO_CHECK", p.ts_no_check);
	if (!p.ts_target.empty()) SetEnvValue("ENGINE_TS_TARGET", p.ts_target);

	// Environment
	SetEnvFlag("ENGINE_NO_INHERIT_ENV", p.no_inherit_env);
	if (!p.env_file.empty()) SetEnvValue("ENGINE_ENV_FILE", p.env_file);

	// Doctor
	SetEnvFlag("ENGINE_DOCTOR_VERBOSE", p.doctor_verbose);
}

bool ParseCommandFromTokens(const std::vector<std::string>& tokens,
                            AppCommand::Parsed* out,
                            std::string* error_message) {
	if (out == nullptr) {
		if (error_message) {
			*error_message = "Internal error: ParseCommandFromTokens target is null";
		}
		return false;
	}

	std::vector<std::string> storage = tokens;
	if (storage.empty()) {
		storage.emplace_back("EngineBuilder");
	}

	std::vector<char*> argv_like;
	argv_like = storage
		| ranges::views::transform([](std::string& token) {
			return token.data();
		})
		| ranges::to<std::vector<char*>>();

	*out = AppCommand::Parse(static_cast<int>(argv_like.size()), argv_like.data());
	if (!out->valid && error_message) {
		*error_message = out->error_message;
	}
	return out->valid;
}

}  // namespace

int main(int argc, char** argv) {
	CrashHandler::Install();

	AppCommand::Parsed parsed = AppCommand::Parse(argc, argv);
	if (!parsed.valid) {
		std::cerr << parsed.error_message << "\n";
		std::cerr << AppCommand::BuildHelpText(argv[0]) << "\n";
		return 2;
	}

	const bool cli_watch = parsed.watch;
	const int cli_watch_interval_ms = parsed.watch_interval_ms;
	if (!parsed.ecosystem_manifest_path.empty()) {
		EcoSystemManifestLoader loader(parsed.ecosystem_manifest_path);
		EcoSystemManifest manifest;
		std::string ecosystem_error;
		if (!loader.Load(&manifest, &ecosystem_error)) {
			std::cerr << "Failed to load ecosystem manifest: " << ecosystem_error << "\n";
			return 2;
		}

		const std::vector<std::string> manifest_tokens = EcoSystemManifestLoader::BuildArgv(manifest);
		AppCommand::Parsed manifest_parsed;
		if (!ParseCommandFromTokens(manifest_tokens, &manifest_parsed, &ecosystem_error)) {
			std::cerr << "Invalid manifest CLI args: " << ecosystem_error << "\n";
			return 2;
		}

		parsed = manifest_parsed;
		if (cli_watch) {
			parsed.watch = true;
			parsed.watch_interval_ms = cli_watch_interval_ms;
		}
	}

	if (parsed.type == AppCommand::Type::kHelp) {
		std::cout << AppCommand::BuildHelpText(argv[0]) << "\n";
		return 0;
	}

	if (parsed.type == AppCommand::Type::kVersion) {
		std::cout << "EngineBuilder version 1.1.0\n";
		return 0;
	}

	if (parsed.type == AppCommand::Type::kDoctor) {
		const bool has_default_script = std::filesystem::exists("example/project.js");
		const bool has_build_dir      = std::filesystem::exists("build");
		std::cout << "Doctor summary:\n";
		std::cout << "  example/project.js : " << (has_default_script ? "ok" : "missing") << "\n";
		std::cout << "  build directory    : " << (has_build_dir      ? "ok" : "missing") << "\n";
		if (parsed.doctor_verbose) {
			// Extra dependency checks.
			const bool has_node = std::filesystem::exists("/usr/bin/node") ||
			                      std::filesystem::exists("/usr/local/bin/node");
			const bool has_tsc  = std::filesystem::exists("/usr/bin/tsc") ||
			                      std::filesystem::exists("/usr/local/bin/tsc");
			std::cout << "  node               : " << (has_node ? "ok" : "missing") << "\n";
			std::cout << "  tsc                : " << (has_tsc  ? "ok" : "missing") << "\n";
		}
		return (has_default_script && has_build_dir) ? 0 : 1;
	}

	// Load .env file first so command-line flags can override it.
	if (!parsed.env_file.empty()) {
		LoadEnvFile(parsed.env_file);
	}

	// Export all flags to environment before building EngineParams.
	ExportParsedToEnv(parsed);

	const std::string script_path = parsed.script_path;
	if (!std::filesystem::exists(script_path)) {
		std::cerr << "Script not found: " << script_path << "\n";
		return 1;
	}

	if (parsed.type == AppCommand::Type::kInspect) {
		Xer::XerEncode encoder;
		std::string inspect_report;
		std::string inspect_error;
		if (!encoder.InspectFile(script_path, &inspect_report, &inspect_error)) {
			std::cerr << "Failed to inspect XER artifact: " << inspect_error << "\n";
			return 1;
		}
		std::cout << inspect_report;
		if (inspect_report.empty() || inspect_report.back() != '\n') {
			std::cout << "\n";
		}
		return 0;
	}

	if (parsed.type == AppCommand::Type::kCompile) {
		std::string extension = std::filesystem::path(script_path).extension().string();
		for (char& ch : extension) {
			ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
		}
		if (extension != ".xer") {
			std::cerr << "Compile command only supports .xer sources: " << script_path << "\n";
			return 1;
		}

		Xer::XerEncode encoder;
		std::string protection_error;
		const Xer::XerProtectionOptions protection = Xer::ResolveProtectionOptionsFromEnvironment(&protection_error);
		if (!protection_error.empty()) {
			std::cerr << "Failed to resolve XER encryption key: " << protection_error << "\n";
			return 1;
		}
		std::string compile_error;
		const Xer::XerEncodedBlock block = encoder.Compile(script_path, protection, &compile_error);
		if (!compile_error.empty()) {
			std::cerr << "Failed to compile .xer script: " << compile_error << "\n";
			return 1;
		}

		const std::filesystem::path output_dir = parsed.output_dir.empty()
			? std::filesystem::path()
			: std::filesystem::path(parsed.output_dir);
		if (!encoder.WriteArtifacts(script_path, block, output_dir, &compile_error)) {
			std::cerr << "Failed to write compile artifacts: " << compile_error << "\n";
			return 1;
		}

		const std::filesystem::path destination_dir = output_dir.empty()
			? std::filesystem::path(script_path).parent_path()
			: output_dir;
		const std::string stem = std::filesystem::path(script_path).stem().string();
		std::cout << "Compiled XER artifacts:\n";
		std::cout << "  " << (destination_dir / (stem + ".bin")).string() << "\n";
		std::cout << "  " << (destination_dir / (stem + ".bak")).string() << "\n";
		return 0;
	}

	CrashHandler::SetScriptPath(script_path);
	CrashHandler::SetDumpDir(".");
	const bool use_xer_runtime = XerScript::Supports(script_path);

	if (parsed.watch) {
		WatchLiveUpdatexScriptConfig watch_cfg;
		watch_cfg.script_path = script_path;
		watch_cfg.poll_interval_ms = parsed.watch_interval_ms;
		WatchLiveUpdatexScript watcher(std::move(watch_cfg));
		const int watch_code = watcher.Run();
		if (!use_xer_runtime) {
			FlowScript::Shutdown();
		}
		return watch_code;
	}

	if (use_xer_runtime) {
		XerScript script(script_path);
		if (!script.Run()) {
			std::cerr << "Failed to run .xer script: " << script_path << "\n";
			return 1;
		}
		return 0;
	}

	FlowScript script(script_path);
	if (!script.Run()) {
		std::cerr << "Failed to run script: " << script_path << "\n";
		FlowScript::Shutdown();
		return 1;
	}

	FlowScript::Shutdown();
	return 0;
}
