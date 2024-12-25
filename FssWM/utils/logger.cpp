#include "logger.hpp"

#include <chrono>

#include "file_io.hpp"
#include "utils.hpp"

namespace utils {

LogFormat                Logger::log_format_;
std::vector<std::string> Logger::log_list_;
std::mutex               Logger::log_mutex_;

LogFormat::LogFormat()
    : log_level(""),
      time_stamp(""),
      func_name(""),
      message("") {
}

std::string LogFormat::Format(const std::string del) {
    return this->log_level + del + this->time_stamp + del + this->func_name + del + this->message;
}

void Logger::DebugLog(const std::string &location, const std::string &message, const bool debug) {
#ifdef LOG_LEVEL_DEBUG
    PrintDebugMessage(location, message, debug);
#endif
#if defined(LOG_LEVEL_DEBUG) && defined(LOGGING_ENABLED)
    SetLogFormat(kLogLevelDebug, location, message);
#endif
}

void Logger::InfoLog(const std::string &location, const std::string &message) {
    PrintInfoMessage(location, message);
#ifdef LOGGING_ENABLED
    SetLogFormat(kLogLevelInfo, location, message);
#endif
}

void Logger::WarnLog(const std::string &location, const std::string &message) {
    PrintWarnMessage(location, message);
#ifdef LOGGING_ENABLED
    SetLogFormat(kLogLevelWarn, location, message);
#endif
}

void Logger::FatalLog(const std::string &location, const std::string &message) {
    PrintFatalMessage(location, message);
#ifdef LOGGING_ENABLED
    SetLogFormat(kLogLevelFatal, location, message);
#endif
}

void Logger::SaveLogsToFile(const std::string &file_path, const bool is_date_time) {
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

std::string Logger::StrWithSep(const std::string &message, char separator, int width) {
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

void Logger::SetLogFormat(const std::string &log_level, const std::string &func_name, const std::string &message) {
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

}    // namespace utils
