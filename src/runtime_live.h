#pragma once

#include <functional>
#include <string>
#include <vector>

namespace FileSystem {
std::string CreateDirectory(const std::string& path);
}

class RuntimeLive {
public:
	static constexpr const char* kProviderGPUToolkit = "gpu_toolkit";

	struct InterfaceCompilerOptions {
		std::string cache_dir;
		std::string provider;
		std::string language;
		std::string compile_flags;
		std::string link_flags;
	};

	struct RuntimeCallbacks {
		std::function<void(const std::string&)> raw_out;
		std::function<void(const std::string&)> raw_err;
	};

	class InterfaceCompiler {
	public:
		struct SourceFile {
			std::string path;
			std::string content;
			bool is_header = false;
		};

		InterfaceCompiler(std::string cache_dir,
						  std::string provider,
						  std::string language,
						  std::string compile_flags,
						  std::string link_flags);

		void AddEntryRaw(std::string source);

		void AddEntryFile(const SourceFile& source_file);

		void ClearEntryFiles();

		bool Runtime(const RuntimeCallbacks& callbacks) const;

	private:
		std::string cache_dir_;
		std::string provider_;
		std::string language_;
		std::string compile_flags_;
		std::string link_flags_;
		std::string source_;
		std::vector<SourceFile> source_files_;
	};

	RuntimeLive();

	bool is_running() const;

	void set_running(bool running);

	static InterfaceCompiler CreateInterfaceCompiler(const InterfaceCompilerOptions& options);

private:
	bool running_;
};
