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

void TimerManager::Start() {
    if (current_timer_id_ == -1) {
        utils::Logger::FatalLog(LOC, "No timer selected.");
        exit(EXIT_FAILURE);
    }
    auto &timer = timers_[current_timer_id_];
    timer.start_times.push_back(std::chrono::high_resolution_clock::now());
}

void TimerManager::Stop(const std::string &msg) {
    if (current_timer_id_ == -1) {
        utils::Logger::FatalLog(LOC, "No timer selected.");
        exit(EXIT_FAILURE);
    }
    auto &timer     = timers_[current_timer_id_];
    auto  stop_time = std::chrono::high_resolution_clock::now();
    timer.end_times.push_back(stop_time);

    // Record elapsed time in nanoseconds (base unit)
    double elapsed = GetElapsedTime(timer.start_times.back(), stop_time);
    timer.elapsed_times.push_back(elapsed);
    timer.messages.push_back(msg);
}

void TimerManager::Mark(const std::string &msg) {
    if (current_timer_id_ == -1) {
        utils::Logger::FatalLog(LOC, "No timer selected.");
        exit(EXIT_FAILURE);
    }

    auto &timer = timers_[current_timer_id_];

    if (timer.start_times.empty()) {
        utils::Logger::FatalLog(LOC, "Timer has not been started.");
        exit(EXIT_FAILURE);
    }

    auto   now     = std::chrono::high_resolution_clock::now();
    double elapsed = GetElapsedTime(timer.start_times.back(), now);
    timer.elapsed_times.push_back(elapsed);
    timer.messages.push_back(msg);
}

void TimerManager::PrintCurrentResults(const std::string &msg, const TimeUnit unit, const bool show_details) const {
    if (current_timer_id_ == -1) {
        utils::Logger::FatalLog(LOC, "No timer selected.");
        exit(EXIT_FAILURE);
    }

    const auto &timer    = timers_.at(current_timer_id_);
    std::string unit_str = GetUnitString(unit);

    std::ostringstream header_message;
    header_message << "[TimerID=" << current_timer_id_ << "]"
                   << " TimerName=\"" << timer.name << "\""
                   << " Unit=" << unit_str
                   << " Count=" << timer.elapsed_times.size();
    utils::Logger::InfoLog("", header_message.str());

    double total_time = 0.0;
    double max_time   = 0.0;
    double min_time   = std::numeric_limits<double>::max();

    std::vector<double> elapsed_converted;
    for (size_t i = 0; i < timer.elapsed_times.size(); ++i) {
        double elapsed = ConvertElapsedTime(timer.elapsed_times[i], NANOSECONDS, unit);
        elapsed_converted.push_back(elapsed);
        total_time += elapsed;
        max_time = std::max(max_time, elapsed);
        min_time = std::min(min_time, elapsed);

        std::ostringstream log_message;
        log_message << "TimerName=\"" << timer.name << "\""
                    << " Message=\"" << timer.messages[i] << "\""
                    << " Elapsed=" << elapsed;
        utils::Logger::InfoLog("", log_message.str());
    }

    double avg_time = timer.elapsed_times.empty() ? 0.0 : total_time / timer.elapsed_times.size();

    // Calculate normalized variance
    double variance = 0.0;
    for (const auto &elapsed : elapsed_converted) {
        variance += (elapsed - avg_time) * (elapsed - avg_time);
    }
    variance /= elapsed_converted.size();
    double normalized_variance = variance / (avg_time * avg_time);

    std::ostringstream summary_header;
    summary_header << "[Summary] Name=\"" << timer.name << "\""
                   << " Unit=" << unit_str
                   << " Message=\"" << msg << "\"";

    std::ostringstream summary_total;
    summary_total << std::fixed << std::setprecision(3);
    summary_total << "Total=" << total_time;
    utils::Logger::InfoLog("", summary_header.str() + " " + summary_total.str());

    std::ostringstream summary_avg;
    summary_avg << std::fixed << std::setprecision(3);
    summary_avg << "Avg=" << avg_time;
    utils::Logger::InfoLog("", summary_header.str() + " " + summary_avg.str());

    if (show_details) {
        std::ostringstream summary_details;
        summary_details << std::fixed << std::setprecision(3);
        summary_details << "Max=" << max_time << " Min=" << min_time << " Var=" << normalized_variance;
        utils::Logger::InfoLog("", summary_header.str() + " " + summary_details.str());
    }
}

// Base time unit is nanoseconds
double TimerManager::GetElapsedTime(const TimePoint &start, const TimePoint &end) const {
    using namespace std::chrono;
    return duration_cast<nanoseconds>(end - start).count();    // 常にナノ秒で返す
}

// Converts elapsed time between units
double TimerManager::ConvertElapsedTime(double time, TimeUnit from, TimeUnit to) const {
    static const double conversion[] = {1.0, 1000.0, 1000000.0, 1000000000.0};
    return time * (conversion[from] / conversion[to]);
}

std::string TimerManager::GetUnitString(TimeUnit unit) const {
    switch (unit) {
        case NANOSECONDS:
            return "ns";
        case MICROSECONDS:
            return "\xC2\xB5s";    // Microseconds symbol
        case MILLISECONDS:
            return "ms";
        case SECONDS:
            return "s";
        default:
            return "ms";
    }
}

}    // namespace utils
