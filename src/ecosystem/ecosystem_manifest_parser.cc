#include "ecosystem/ecosystem_manifest_parser.h" 
#include "ecosystem/ecosystem_manifest_args.h"

#include <json/json.h>
#include <memory>

#include <range/v3/algorithm/all_of.hpp>
#include <range/v3/range/conversion.hpp>
#include <range/v3/view/transform.hpp>

bool EcoSystemManifestParser::Parse(const std::string& json_text,
                                    EcoSystemManifest* out,
                                    std::string* error_message) {
    if (out == nullptr) {
        if (error_message) *error_message = "Manifest output target is null";
        return false;
    }

    *out = EcoSystemManifest{};

    Json::CharReaderBuilder builder;
    builder["collectComments"] = false;

    Json::Value root;
    std::string parse_errors;
    std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
    if (!reader || !reader->parse(
            json_text.data(),
            json_text.data() + json_text.size(),
            &root,
            &parse_errors)) {
        if (error_message) {
            *error_message = "Failed to parse ecosystem JSON: " + parse_errors;
        }
        return false;
    }

    if (!root.isObject()) {
        if (error_message) {
            *error_message = "ecosystem manifest root must be a JSON object";
        }
        return false;
    }

    const Json::Value script_value = root["script"];
    if (!script_value.isString()) {
        if (error_message) {
            *error_message = "ecosystem manifest must contain string field: script";
        }
        return false;
    }
    out->script_path = script_value.asString();

    const Json::Value args_value = root["args"];
    if (args_value.isArray()) {
        if (!ranges::all_of(args_value, [](const Json::Value& item) { return item.isString(); })) {
            if (error_message) {
                *error_message = "ecosystem args array must contain only strings";
            }
            return false;
        }

        out->args = args_value
            | ranges::views::transform([](const Json::Value& item) {
                  return item.asString();
              })
            | ranges::to<std::vector<std::string>>();
        return true;
    }

    if (args_value.isString()) {
        out->args = EcoSystemManifestArgs::Split(args_value.asString());
    } else if (!args_value.isNull()) {
        if (error_message) {
            *error_message = "ecosystem field 'args' must be string or string[]";
        }
        return false;
    }

    return true;
}
