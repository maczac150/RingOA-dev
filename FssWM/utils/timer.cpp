#include "timer.hpp"

#include <chrono>

namespace utils {

int32_t TimerManager::CreateNewTimer(const std::string &name) {
    int32_t timer_id  = timer_count_++;
    timers_[timer_id] = Timer{name, {}, {}};
    return timer_id;
}

void TimerManager::SelectTimer(int32_t timer_id) {
    if (timers_.find(timer_id) == timers_.end()) {
        utils::Logger::FatalLog(LOC, "Invalid timer ID.");
        exit(EXIT_FAILURE);
    }
    current_timer_id_ = timer_id;
}

void TimerManager::SetUnit(TimeUnit unit) {
    unit_ = unit;
}

void TimerManager::Start() {
    if (current_timer_id_ == -1) {
        utils::Logger::FatalLog(LOC, "No timer selected.");
        exit(EXIT_FAILURE);
    }
    auto &timer = timers_[current_timer_id_];
    timer.start_times.push_back(std::chrono::high_resolution_clock::now());
}

void TimerManager::Stop() {
    if (current_timer_id_ == -1) {
        utils::Logger::FatalLog(LOC, "No timer selected.");
        exit(EXIT_FAILURE);
    }
    auto &timer     = timers_[current_timer_id_];
    auto  stop_time = std::chrono::high_resolution_clock::now();
    timer.end_times.push_back(stop_time);

    // Record elapsed time
    double elapsed = GetElapsedTime(timer.start_times.back(), stop_time);
    timer.elapsed_times.push_back(elapsed);
}

void TimerManager::Mark() {
    if (current_timer_id_ == -1) {
        utils::Logger::FatalLog(LOC, "No timer selected.");
        exit(EXIT_FAILURE);
    }

    auto &timer = timers_[current_timer_id_];

    // Record elapsed time
    if (timer.start_times.empty()) {
        utils::Logger::FatalLog(LOC, "Timer has not been started.");
        exit(EXIT_FAILURE);
    }

    auto   now     = std::chrono::high_resolution_clock::now();
    double elapsed = GetElapsedTime(timer.start_times.back(), now);
    timer.elapsed_times.push_back(elapsed);
}

void TimerManager::PrintCurrentResults(TimeUnit unit) const {
    if (current_timer_id_ == -1) {
        utils::Logger::FatalLog(LOC, "No timer selected.");
        exit(EXIT_FAILURE);
    }

    const auto &timer    = timers_.at(current_timer_id_);
    std::string unit_str = GetUnitString(unit);    // Converts unit enum to string

    // --- Print Header ---
    std::ostringstream header_message;
    header_message << "[TimerID=" << current_timer_id_ << "]"
                   << " TimerName=\"" << timer.name << "\""
                   << " Unit=" << unit_str
                   << " Count=" << timer.elapsed_times.size();
    utils::Logger::InfoLog("", header_message.str());

    // --- Process Timings ---
    double total_time = 0.0;
    double max_time   = 0.0;
    double min_time   = std::numeric_limits<double>::max();

    for (size_t i = 0; i < timer.elapsed_times.size(); ++i) {
        // Convert elapsed time to the specified unit
        // double elapsed = GetElapsedTime(timer.start_times[i], timer.end_times[i]);
        double elapsed = timer.elapsed_times[i];
        total_time += elapsed;
        max_time = std::max(max_time, elapsed);
        min_time = std::min(min_time, elapsed);

        // --- Log Elapsed Time with Name ---
        std::ostringstream log_message;
        log_message << "TimerName=\"" << timer.name << "\""
                    << " Elapsed=" << elapsed;
        utils::Logger::InfoLog("", log_message.str());
    }

    // --- Calculate Summary ---
    double avg_time = timer.elapsed_times.empty() ? 0.0 : total_time / timer.elapsed_times.size();

    // --- Print Summary ---
    std::ostringstream summary_message;
    summary_message << "[Summary] Name=\"" << timer.name << "\""
                    << " (Total, Avg, Max, Min)=("
                    << total_time << ", " << avg_time << ", " << max_time << ", " << min_time << ")";
    utils::Logger::InfoLog("", summary_message.str());
}

double TimerManager::GetElapsedTime(const TimePoint &start, const TimePoint &end) const {
    using namespace std::chrono;
    switch (unit_) {
        case NANOSECONDS:
            return duration_cast<nanoseconds>(end - start).count();
        case MICROSECONDS:
            return duration_cast<microseconds>(end - start).count();
        case MILLISECONDS:
            return duration_cast<milliseconds>(end - start).count();
        case SECONDS:
            return duration_cast<seconds>(end - start).count();
        default:
            return duration_cast<milliseconds>(end - start).count();    // Default to milliseconds
    }
}

std::string TimerManager::GetUnitString(TimeUnit unit) const {
    switch (unit) {
        case NANOSECONDS:
            return " ns";
        case MICROSECONDS:
            return " µs";
        case MILLISECONDS:
            return " ms";
        case SECONDS:
            return " s";
        default:
            return " ms";    // Default to milliseconds
    }
}

}    // namespace utils
