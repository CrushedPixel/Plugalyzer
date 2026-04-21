#include "Validators.h"

#include "Parsers.h"
#include "Utils.h"

// Validators for CLI11
// Signature:
// std::string validate(const std::string& arg)
// Usage:
// option->check(validate);

namespace validate
{
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

} // namespace validate

