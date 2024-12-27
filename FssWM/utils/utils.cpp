#include "utils.hpp"

#include <chrono>
#include <filesystem>

namespace utils {

std::string GetCurrentDateTimeAsString() {
    auto              now         = std::chrono::system_clock::now();
    std::time_t       currentTime = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&currentTime), "%Y%m%d_%H%M%S");
    return ss.str();
}

std::string GetCurrentDirectory() {
    return std::filesystem::current_path().string();
}

std::vector<uint32_t> CreateSequence(const uint32_t start, const uint32_t end) {
    std::vector<uint32_t> sequence;
    sequence.reserve(end - start);
    for (uint32_t i = start; i < end; i++) {
        sequence.push_back(i);
    }
    return sequence;
}

std::vector<std::uint32_t> CreateVectorWithSameValue(const uint32_t value, const uint32_t size) {
    std::vector<std::uint32_t> result(size, value);
    return result;
}

std::string ToString(const std::vector<bool> &bool_vector) {
    std::string res_str = "";
    for (int i = 0; i < static_cast<int>(bool_vector.size()); i++) {
        res_str += std::to_string(bool_vector[i]);
    }
    return res_str;
}

std::string ToString(const double val, const size_t digits) {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(digits) << val;
    std::string result  = ss.str();
    size_t      dot_pos = result.find('.');
    if (dot_pos != std::string::npos) {
        size_t digits_after_dot = result.length() - dot_pos - 1;
        if (digits_after_dot < digits) {
            result.append(digits - digits_after_dot, '0');
        }
    } else if (digits > 0) {
        result.append(1, '.');
        result.append(digits, '0');
    }
    return result;
}

std::string ConvertToHex(unsigned char *data, size_t length) {
    std::stringstream ss;
    for (size_t i = 0; i < length; ++i) {
        ss << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(data[i]) << " ";
    }
    return ss.str();
}

std::string GetValidity(const bool is_valid) {
    return is_valid ? "[VALID]" : "[INVALID]";
}

// #######################
// ######## Print ########
// #######################

void PrintText(const std::string &text) {
    std::cout << text << std::endl;
}

void PrintColoredText(const std::string &text, const int color_code) {
    std::cout << "\x1b[" << color_code << "m";    // Set console color
    std::cout << text;
    std::cout << "\x1b[0m";    // Reset console color
}

void PrintBoldText(const std::string &text) {
    std::cout << "\033[1m" << text << "\033[0m";
}

void PrintValidity(const std::string &info_msg, const std::string &msg_body, const bool is_valid, const bool debug) {
    PrintColoredText("[INFO]", color_map["bright_green"]);
    std::cout << "::";
    PrintBoldText("[" + info_msg + "] ");
    std::cout << msg_body + " -> ";
    if (is_valid) {
        PrintColoredText(GetValidity(is_valid) + "\n", color_map["green"]);
    } else {
        PrintColoredText(GetValidity(is_valid) + "\n", color_map["red"]);
    }
}

void PrintValidity(const std::string &info_msg, const uint32_t x, const uint32_t y, const bool debug) {
    PrintColoredText("[INFO]", color_map["bright_green"]);
    std::cout << "::";
    PrintBoldText("[" + info_msg + "] ");
    std::cout << "Equality check: (" + std::to_string(x) + ", " + std::to_string(y) + ") -> ";
    const bool is_valid = (x == y);
    if (is_valid) {
        PrintColoredText(GetValidity(is_valid) + "\n", color_map["green"]);
    } else {
        PrintColoredText(GetValidity(is_valid) + "\n", color_map["red"]);
    }
}

void PrintTestResult(const std::string &test_name, bool result) {
    const int   name_width  = 50;
    std::string result_text = result ? "[PASS]" : "[FAIL]";
    int         color_code  = result ? color_map["bright_green"] : color_map["bright_red"];

    std::cout << "     " << std::left << std::setw(name_width) << test_name << "- ";
    std::cout << "\x1b[1m";    // Bold text
    PrintColoredText(result_text, color_code);
    std::cout << "\x1b[0m";    // Reset bold text
    std::cout << std::endl;
}

void PrintDebugMessage(const std::string &info_msg, const std::string &msg_body, const bool debug) {
#ifdef LOG_LEVEL_DEBUG
    if (debug) {
        PrintColoredText("[DEBUG]", color_map["bright_blue"]);
        PrintBoldText("[" + info_msg + "] ");
        std::cout << msg_body << std::endl;
    }
#endif
}

void PrintInfoMessage(const std::string &info_msg, const std::string &msg_body) {
    PrintColoredText("[INFO]", color_map["bright_green"]);
    PrintBoldText("[" + info_msg + "] ");
    std::cout << msg_body << std::endl;
}

void PrintWarnMessage(const std::string &info_msg, const std::string &msg_body) {
    PrintColoredText("[WARNING]", color_map["yellow"]);
    PrintBoldText("[" + info_msg + "] ");
    std::cout << msg_body << std::endl;
}

void PrintFatalMessage(const std::string &info_msg, const std::string &msg_body) {
    PrintColoredText("[FATAL]", color_map["red"]);
    PrintBoldText("[" + info_msg + "] ");
    std::cout << msg_body << std::endl;
}

void AddNewLine(bool debug) {
#ifdef LOG_LEVEL_DEBUG
    if (debug) {
        std::cout << std::endl;
    }
#endif
}

void OptionHelpMessage(const std::string &location, const std::vector<std::string> &options) {
    PrintFatalMessage(location, "Invalid options (-m, --mode). Please select the correct option.");
    PrintInfoMessage(location, "######################################");
    PrintInfoMessage(location, "# Available options:");
    for (size_t i = 0; i < options.size(); ++i) {
        PrintInfoMessage(location, "# " + std::to_string(i + 1) + ". " + options[i]);
    }
    PrintInfoMessage(location, "######################################");
    exit(EXIT_FAILURE);
}

uint32_t ExcludeBitsAbove(const uint32_t value, const uint32_t bit_position) {
    uint32_t mask = (1U << (bit_position - 1U)) - 1U;
    return value & mask;
}

bool GetBitAtPosition(const uint32_t value, const uint32_t bit_position) {
    uint32_t mask = 1U << (bit_position - 1U);
    return (value & mask) != 0;
}

uint32_t GetLowerNBits(const uint32_t value, const uint32_t n) {
    return value & ((1U << n) - 1U);
}

}    // namespace utils
