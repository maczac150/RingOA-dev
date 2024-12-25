#include "timer.hpp"

#include "logger.hpp"

namespace utils {

ExecutionTimer::ExecutionTimer()
    : unit_(MILLISECONDS) {
}

void ExecutionTimer::Start() {
    this->start_ = std::chrono::high_resolution_clock::now();
}

double ExecutionTimer::Print(const std::string &location, const std::string &message) {
    this->end_                             = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = this->end_ - this->start_;

    // Convert the measurement result based on the chosen time unit
    double      time_value(0.0);
    std::string unit_name;
    switch (this->unit_) {
        case NANOSECONDS:
            time_value = std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
            unit_name  = "ns";
            break;
        case MICROSECONDS:
            time_value = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
            unit_name  = "us";
            break;
        case MILLISECONDS:
            time_value = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
            unit_name  = "ms";
            break;
        case SECONDS:
            time_value = duration.count();
            unit_name  = "s";
            break;
    }

    Logger::InfoLog(location, message + "," + std::to_string(time_value) + "," + unit_name);

    return time_value;
}

void ExecutionTimer::SetTimeUnit(const TimeUnit unit) {
    this->unit_ = unit;
}

TimeUnit ExecutionTimer::GetTimeUnit() const {
    return this->unit_;
}

std::string ExecutionTimer::GetTimeUnitStr() const {
    switch (this->unit_) {
        case NANOSECONDS:
            return "ns";
        case MICROSECONDS:
            return "us";
        case MILLISECONDS:
            return "ms";
        case SECONDS:
            return "s";
    }
    return "";
}

bool ExecutionTimer::IsExceedLimitTime(const double res, const uint32_t limit_time_ms, const TimeUnit unit) {
    switch (unit) {
        case NANOSECONDS:
            return res > limit_time_ms * 1000000;
        case MICROSECONDS:
            return res > limit_time_ms * 1000;
        case MILLISECONDS:
            return res > limit_time_ms;
        case SECONDS:
            return res > limit_time_ms / 1000;
    }
    utils::Logger::FatalLog(LOC, "Invalid time unit");
    return false;
}

}    // namespace utils
