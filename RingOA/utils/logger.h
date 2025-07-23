#ifndef UTILS_LOGGER_H_
#define UTILS_LOGGER_H_

#include <filesystem>
#include <mutex>
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

// Define log levels
#define LOG_LEVEL_NONE  0
#define LOG_LEVEL_FATAL 1
#define LOG_LEVEL_ERROR 2
#define LOG_LEVEL_WARN  3
#define LOG_LEVEL_INFO  4
#define LOG_LEVEL_DEBUG 5
#define LOG_LEVEL_TRACE 6

#ifndef LOG_LEVEL
#define LOG_LEVEL LOG_LEVEL_INFO    // Default log level
#endif

namespace ringoa {

constexpr int     kMsgMaxLength = 70;
const std::string kDash         = "---------------------------------------------------------------------";

/**
 * @brief A struct to store log information.
 */
struct LogFormat {
    std::string log_level;
    std::string time_stamp;
    std::string func_name;
    std::string message;

    /**
     * @brief Default constructor for LogFormat class.
     */
    LogFormat()
        : log_level(""), time_stamp(""), func_name(""), message("") {
    }

    /**
     * @brief Formats log information into a single string.
     * @param del The delimiter used to separate log information in the output string.
     * @return A string containing formatted log information joined by the delimiter.
     */
    std::string Format(const std::string del = ",");
};

/**
 * @brief A simple logger for recording experiment logs.
 */
class Logger {
public:
    /**
     * @brief Default constructor for the Logger class.
     */
    Logger() = delete;

    /**
     * @brief Output FATAL log message to the console and save it to the log list.
     * @param location The location where the log message is generated.
     * @param message The log message to be printed.
     */
    static void FatalLog(const std::string &location, const std::string &message);

    /**
     * @brief Output ERROR log message to the console and save it to the log list.
     * @param location The location where the log message is generated.
     * @param message The log message to be printed.
     */
    static void ErrorLog(const std::string &location, const std::string &message);

    /**
     * @brief Output WARN log message to the console and save it to the log list.
     * @param location The location where the log message is generated.
     * @param message The log message to be printed.
     */
    static void WarnLog(const std::string &location, const std::string &message);

    /**
     * @brief Output INFO log message to the console and save it to the log list.
     * @param location The location where the log message is generated.
     * @param message The log message to be printed.
     */
    static void InfoLog(const std::string &location, const std::string &message);

    /**
     * @brief Output DEBUG log message to the console and save it to the log list.
     * @param location The location where the log message is generated.
     * @param message The log message to be printed.
     */
    static void DebugLog(const std::string &location, const std::string &message);

    /**
     * @brief Output TRACE log message to the console and save it to the log list.
     * @param location The location where the log message is generated.
     * @param message The log message to be printed.
     */
    static void TraceLog(const std::string &location, const std::string &message);

    /**
     * @brief Returns a string with the specified message centered and surrounded by a separator.
     * @param message The message to be centered and surrounded by the separator.
     * @param separator The character used to surround the message.
     * @param width The total width of the output string.
     * @return A string with the message centered and surrounded by the separator.
     */
    static std::string StrWithSep(const std::string &message, char separator = '-', int width = kMsgMaxLength);

    /**
     * @brief Set the flag to print the log message.
     * @param print_log A flag to indicate whether to print the log message.
     */
    static void SetPrintLog(bool print_log);

    /**
     * @brief Get the flag to print the log message.
     * @return A flag to indicate whether to print the log message.
     */
    static bool GetPrintLog();

    /**
     * @brief Get log list.
     * @param log_list A vector to store log entries as strings.
     */
    const static std::vector<std::string> &GetLogList();

    /**
     * @brief Clear log list.
     */
    static void ClearLogList();

    static void ExportLogList(const std::string &file_path, const bool append = false);

private:
    static LogFormat                log_format_; /**< A struct to store log information. */
    static std::mutex               log_mutex_;  /**< A mutex to protect the log list. */
    static std::vector<std::string> log_list_;   /**< A list to store log entries as strings. */
    static bool                     print_log_;  /**< A flag to indicate whether to print the log message. */

    /**
     * @brief Print a debug message to the console.
     * @param location The location where the log message is generated.
     * @param message The log message to be printed.
     */
    static void SetLogFormat(const std::string &log_level, const std::string &func_name, const std::string &message);
};

}    // namespace ringoa

#endif    // UTILS_LOGGER_H_
