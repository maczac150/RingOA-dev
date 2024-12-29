#ifndef UTILS_LOGGER_H_
#define UTILS_LOGGER_H_

#include <chrono>
#include <filesystem>
#include <mutex>
#include <string>
#include <vector>

#include "utils/file_io.hpp"
#include "utils/utils.hpp"

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

/**
 * @brief Enumeration of log levels.
 */
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
    std::string Format(const std::string del = ",") {
        return this->log_level + del + this->time_stamp + del + this->func_name + del + this->message;
    }
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
     * @brief Print a debug message to the console.
     * @param location The location where the log message is generated.
     * @param message The log message to be printed.
     * @param debug A flag to indicate whether to print the log message.
     */
    static void DebugLog(const std::string &location, const std::string &message, const bool debug) {
#ifdef LOG_LEVEL_DEBUG
        PrintDebugMessage(location, message, debug);
#endif
#if defined(LOG_LEVEL_DEBUG) && defined(LOGGING_ENABLED)
        SetLogFormat(kLogLevelDebug, location, message);
#endif
    }

    /**
     * @brief Print an info message to the console.
     * @param location The location where the log message is generated.
     * @param message The log message to be printed.
     */
    static void InfoLog(const std::string &location, const std::string &message) {
        PrintInfoMessage(location, message);
#ifdef LOGGING_ENABLED
        SetLogFormat(kLogLevelInfo, location, message);
#endif
    }

    /**
     * @brief Print a warning message to the console.
     * @param location The location where the log message is generated.
     * @param message The log message to be printed.
     */
    static void WarnLog(const std::string &location, const std::string &message) {
#ifdef LOG_LEVEL_DEBUG
        PrintWarnMessage(location, message);
#endif
#ifdef LOGGING_ENABLED
        SetLogFormat(kLogLevelWarn, location, message);
#endif
    }

    /**
     * @brief Print a fatal message to the console.
     * @param location The location where the log message is generated.
     * @param message The log message to be printed.
     */
    static void FatalLog(const std::string &location, const std::string &message) {
        PrintFatalMessage(location, message);
#ifdef LOGGING_ENABLED
        SetLogFormat(kLogLevelFatal, location, message);
#endif
    }

    /**
     * @brief Save the logs to a file.
     * @param file_path The file path where the logs will be saved.
     * @param is_date_time A flag to indicate whether to include the date and time in the file name.
     */
    static void SaveLogsToFile(const std::string &file_path, const bool is_date_time = true) {
#ifdef LOGGING_ENABLED
        FileIo io(false, ".log");
        if (is_date_time) {
            io.WriteStringVectorToFile(file_path + "_" + utils::GetCurrentDateTimeAsString(), log_list_, true);
        } else {
            io.WriteStringVectorToFile(file_path, log_list_, true);
        }
        log_list_.clear();
#endif
    }

    /**
     * @brief Returns a string with the specified message centered and surrounded by a separator.
     * @param message The message to be centered and surrounded by the separator.
     * @param separator The character used to surround the message.
     * @param width The total width of the output string.
     * @return A string with the message centered and surrounded by the separator.
     */
    static std::string StrWithSep(const std::string &message, char separator = '-', int width = kMsgMaxLength) {
        int messageWidth  = message.length();
        int totalSepWidth = width - messageWidth - 2;    // Subtract 2 for the spaces around the message

        if (totalSepWidth <= 0) {
            // If the width is less than or equal to the length of the message + 2, return the message without separators
            return message;
        }

        int leftSepWidth  = totalSepWidth / 2;
        int rightSepWidth = totalSepWidth - leftSepWidth;    // Adjust for any rounding issues

        std::string leftSeparator(leftSepWidth, separator);
        std::string rightSeparator(rightSepWidth, separator);

        return leftSeparator + " " + message + " " + rightSeparator;
    }

private:
    static LogFormat                log_format_; /**< A struct to store log information. */
    static std::mutex               log_mutex_;  /**< A mutex to protect the log list. */
    static std::vector<std::string> log_list_;   /**< A list to store log entries as strings. */

    /**
     * @brief Print a debug message to the console.
     * @param location The location where the log message is generated.
     * @param message The log message to be printed.
     * @param debug A flag to indicate whether to print the log message.
     */
    static void SetLogFormat(const std::string &log_level, const std::string &func_name, const std::string &message) {
        std::lock_guard<std::mutex> lock(log_mutex_);
        log_format_.log_level = log_level;
        std::time_t now       = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        char        buffer[80];
        std::strftime(buffer, sizeof(buffer), "%Y/%m/%d %H:%M:%S", std::localtime(&now));
        log_format_.time_stamp = buffer;
        log_format_.func_name  = func_name;
        log_format_.message    = message;
        log_list_.push_back(log_format_.Format());
    }
};

inline LogFormat                Logger::log_format_;
inline std::mutex               Logger::log_mutex_;
inline std::vector<std::string> Logger::log_list_;

}    // namespace utils

#endif    // UTILS_LOGGER_H_
