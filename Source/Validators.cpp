#include "Validators.h"

#include "Parsers.h"
#include "Utils.h"

#include <cstddef>
#include <format>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <string_view>

// Validators for CLI11
// Signature:
// std::string validate(const std::string& arg)
// Usage:
// option->check(validate);

namespace validate {
std::string outputPath(const std::string& arg) {
    auto file = parse::stringToFile(arg);
    if (!file.getParentDirectory().exists()) {
        return "Output parent directory does not exist";
    }
    return std::string();
}

std::string binaryOrXml(const std::string& arg) {
    if (arg == "binary" || arg == "xml") {
        return std::string();
    } else {
        return "Output format must be 'binary' or 'xml'";
    }
}

std::string pluginParameter(const std::string& str) {
    try {
        parse::pluginParameterArgument(str);
    } catch (const std::runtime_error& e) {
        return std::string(e.what());
    }

    return std::string();
}

std::string bitDepth(const std::string& str) {
    try {
        int value = std::stoi(str);
        if (value != 8 && value != 16 && value != 24 && value != 32) {
            return std::string("Bit depth must be 8, 16, 24, or 32");
        }
    } catch (const std::exception& e) {
        return std::string("Bit depth must be a valid integer");
    }
    return std::string();
}

std::string outputFormat(const std::string& str) {
    if (formatMap.contains(str)) {
        return "";
    } else {
        return std::format(
            "Unknown output format. Must be {}", string_utils::presentMapKeysAsOptions(formatMap)
        );
    }
}

std::string generator(const std::string& str) {
    auto generatorJson = getJson(str);
    std::vector<std::string> errors;

    auto check_contains = [&](const nlohmann::json& json, const std::string_view element,
                              const std::string_view descriptionOfJsonNode) {
        if (!json.contains(element)) {
            errors.push_back(std::format("'{}' missing in {}", element, descriptionOfJsonNode));
            return false;
        }
        return true;
    };

    auto check_true = [&](bool expression, const std::string_view errorMsg) {
        if (!expression) {
            errors.push_back(std::string{ errorMsg });
            return false;
        }
        return true;
    };

    check_contains(generatorJson, "sample rate", "root of json");
    check_contains(generatorJson, "num channels", "root of json");
    check_contains(generatorJson, "duration", "root of json");

    if (check_contains(generatorJson, "channels", "root of json")) {
        if (check_true(generatorJson["channels"].is_array(), "'channels' must be an array")) {
            std::size_t i{ 0 };
            for (const auto& elem : generatorJson["channels"]) {
                if (check_contains(elem, "generator", std::format("ch{}", i))) {
                    if (elem["generator"] == "sine") {
                        check_contains(elem, "frequency", std::format("ch{}", i));
                    }
                }
                check_contains(elem, "amplitude", std::format("ch{}", i));
                ++i;
            }
        }
    }

    if (!errors.empty()) {
        return string_utils::join(errors, ", ");
    }
    return "";
}

} // namespace validate
