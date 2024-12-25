#ifndef UTILS_LOGGER_H_
#define UTILS_LOGGER_H_

#include <filesystem>
#include <mutex>
#include <string>
#include <vector>

// LOC macro: Returns the current file name, line number, and function name as a string
// Without function name: [LOG-INFO][filename.cpp:123]
#define LOC (std::string(std::filesystem::path(__FILE__).filename()) + ":" + std::to_string(__LINE__)).c_str()

// Absolute path: [LOG-INFO][/home/username/.../filename.cpp:123][function_name]
// #define LOC (std::string(__FILE__) + ":" + std::to_string(__LINE__) + "][" + std::string(__func__)).c_str()
// File name only: [LOG-INFO][filename.cpp:123][function_name]
// #define LOC (std::string(std::filesystem::path(__FILE__).filename()) + ":" + std::to_string(__LINE__) + "][" + std::string(__func__)).c_str()

// PRETTY_LOC macro: Returns the current file name, line number, and pretty function name as a string
#define PRETTY_LOC (std::string(std::filesystem::path(__FILE__).filename()) + ":" + std::to_string(__LINE__) + "][" + std::string(__PRETTY_FUNCTION__)).c_str()

namespace utils {

enum class LogLevel
{
    DEBUG,
    INFO,
    WARNING,
    FATAL
};

constexpr int     kMsgMaxLength  = 70;
const std::string kLogLevelDebug = "[DEBUG]";
const std::string kLogLevelInfo  = "[INFO]";
const std::string kLogLevelWarn  = "[WARNING]";
const std::string kLogLevelFatal = "[FATAL]";
const std::string kDash          = "---------------------------------------------------------------------";

struct LogFormat {
    std::string log_level;
    std::string time_stamp;
    std::string func_name;
    std::string message;

    /**
     * @brief Default constructor for LogFormat class.
     *
     * Initializes the LogFormat object with empty strings for its member variables:
     * log_level, time_stamp, func_name, message, and detail.
     */
    LogFormat();

    /**
     * @brief Formats log information into a single string.
     *
     * Formats log information (log_level, time_stamp, func_name, message, and detail)
     * into a single string using the specified delimiter 'del'.
     *
     * @param del The delimiter used to separate log information in the output string.
     * @return A string containing formatted log information joined by the delimiter.
     */
    std::string Format(const std::string del = ",");
};

/**
 * @brief A simple logger for recording experiment logs.
 *
 * The `Logger` class provides a basic logging mechanism for recording experiment logs.
 */
class Logger {
public:
    /**
     * @brief Default constructor for the Logger class.
     *
     * The default constructor for the Logger class is deleted to prevent instantiation.
     */
    Logger() = delete;

    /**
     * USEAGE:
     * utils::Logger::DebugLog(LOC, "This is DEBUG log.", debug);
     */
    static void DebugLog(const std::string &location, const std::string &message, const bool debug);

    /**
     * USEAGE:
     * utils::Logger::InfoLog(LOC, "This is INFO log.");
     */
    static void InfoLog(const std::string &location, const std::string &message);

    /**
     * USEAGE:
     * utils::Logger::WarnLog(LOC, "This is WARNING log.");
     */
    static void WarnLog(const std::string &location, const std::string &message);

    /**
     * USEAGE:
     * utils::Logger::FatalLog(LOC, "This is FATAL log.");
     */
    static void FatalLog(const std::string &location, const std::string &message);

    /**
     * @brief Save the logs to a file.
     *
     * This method saves the log entries stored in the log list to a specified file.
     * It also prints each log entry to the console. After saving, the log list is cleared.
     *
     * @param file_path The file path where the logs will be saved.
     * @param is_date_time A flag to indicate whether to include the date and time in the file name.
     */
    static void SaveLogsToFile(const std::string &file_path, const bool is_date_time = true);

    /**
     * @brief Returns a string with the specified message centered and surrounded by a separator.
     *
     * Ensures that the total length of the output string is exactly 'width' characters.
     * If the width is less than the length of the message, returns the message without separators.
     *
     * @param message The message to be centered and surrounded by the separator.
     * @param separator The character used to surround the message.
     * @param width The total width of the output string.
     * @return A string with the message centered and surrounded by the separator.
     */
    static std::string StrWithSep(const std::string &message, char separator = '-', int width = kMsgMaxLength);

private:
    static LogFormat                log_format_;
    static std::mutex               log_mutex_;
    static std::vector<std::string> log_list_; /**< A list to store log entries as strings. */
    static void                     SetLogFormat(const std::string &log_level, const std::string &func_name, const std::string &message);
};

}    // namespace utils

#endif    // UTILS_LOGGER_H_
