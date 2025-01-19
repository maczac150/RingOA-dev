#include "utils.h"

#include <chrono>
#include <filesystem>

namespace fsswm {

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
    for (uint32_t i = start; i < end; ++i) {
        sequence.push_back(i);
    }
    return sequence;
}

std::string ToString(const std::vector<bool> &bool_vector) {
    std::string res_str = "";
    for (int i = 0; i < static_cast<int>(bool_vector.size()); ++i) {
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

uint32_t GetLowerNBits(const uint32_t value, const uint32_t n) {
    return value & ((1U << n) - 1U);
}

}    // namespace fsswm
