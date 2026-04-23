#include "ecosystem/ecosystem_manifest_loader.h"

#include "ecosystem/ecosystem_manifest_parser.h"
#include "tool_to.h"

#include <filesystem>

#include <range/v3/algorithm/copy.hpp>

EcoSystemManifestLoader::EcoSystemManifestLoader(std::string manifest_path)
    : manifest_path_(std::move(manifest_path)) {}

bool EcoSystemManifestLoader::Load(EcoSystemManifest* out,
                                   std::string* error_message) const {
    if (manifest_path_.empty()) {
        if (error_message) *error_message = "ecosystem path is empty";
        return false;
    }

    const std::filesystem::path path(manifest_path_);
    const std::string text = ToolTo::ReadTextFile(path);
    if (text.empty()) {
        if (error_message) {
            *error_message = "Unable to read ecosystem manifest: " + path.string();
        }
        return false;
    }

    return EcoSystemManifestParser::Parse(text, out, error_message);
}

std::vector<std::string> EcoSystemManifestLoader::BuildArgv(const EcoSystemManifest& manifest) {
    std::vector<std::string> argv;
    argv.reserve(3 + manifest.args.size());
    argv.emplace_back("engine");
    argv.emplace_back("run");
    argv.emplace_back(manifest.script_path);
    ranges::copy(manifest.args, std::back_inserter(argv));
    return argv;
}
