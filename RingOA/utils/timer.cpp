#include "timer.h"

#include "logger.h"

namespace ringoa {

int32_t TimerManager::CreateNewTimer(const std::string &name) {
    int32_t timer_id  = timer_count_++;
    timers_[timer_id] = Timer{name, {}, {}, {}, {}};
    return timer_id;
}

void TimerManager::SelectTimer(int32_t timer_id) {
    if (timers_.find(timer_id) == timers_.end()) {
        Logger::ErrorLog(LOC, "Invalid timer ID.");
        return;
    }
    current_timer_id_ = timer_id;
}

void TimerManager::Start() {
    if (current_timer_id_ == -1) {
        Logger::ErrorLog(LOC, "No timer selected.");
        return;
    }
    auto &timer = timers_[current_timer_id_];
    timer.start_times.push_back(std::chrono::high_resolution_clock::now());
}

void TimerManager::Stop(const std::string &msg) {
    if (current_timer_id_ == -1) {
        Logger::ErrorLog(LOC, "No timer selected.");
        return;
    }

    auto &timer = timers_[current_timer_id_];

    if (timer.start_times.empty()) {
        Logger::ErrorLog(LOC, "Timer has not been started.");
        return;
    }

    auto stop_time = std::chrono::high_resolution_clock::now();
    timer.end_times.push_back(stop_time);

    // Record elapsed time in nanoseconds (base unit)
    double elapsed = GetElapsedTime(timer.start_times.back(), stop_time);
    timer.elapsed_times.push_back(elapsed);
    timer.messages.push_back(msg);
}

void TimerManager::Mark(const std::string &msg) {
    if (current_timer_id_ == -1) {
        Logger::ErrorLog(LOC, "No timer selected.");
        return;
    }

    auto &timer = timers_[current_timer_id_];

    if (timer.start_times.empty()) {
        Logger::ErrorLog(LOC, "Timer has not been started.");
        return;
    }

    auto   now     = std::chrono::high_resolution_clock::now();
    double elapsed = GetElapsedTime(timer.start_times.back(), now);
    timer.elapsed_times.push_back(elapsed);
    timer.messages.push_back(msg);
}

void TimerManager::PrintCurrentResults(const std::string &msg, const TimeUnit unit, const bool show_details) const {
    if (current_timer_id_ == -1) {
        Logger::ErrorLog(LOC, "No timer selected.");
        return;
    }

    const auto it = timers_.find(current_timer_id_);
    if (it == timers_.end()) {
        Logger::ErrorLog(LOC, "Invalid timer selected.");
        return;
    }

    const auto &timer    = it->second;
    std::string unit_str = GetUnitString(unit);

    std::ostringstream header_message;
    header_message << "[TimerID=" << current_timer_id_ << "]"
                   << " TimerName=\"" << timer.name << "\""
                   << " Unit=" << unit_str
                   << " Count=" << timer.elapsed_times.size();
    Logger::InfoLog("", header_message.str());

    if (timer.elapsed_times.empty()) {
        Logger::InfoLog("", "[Summary] Name=\"" + timer.name + "\" Unit=" + unit_str +
                                " Message=\"" + msg + "\" No entries.");
        return;
    }

    double total = 0.0;
    double max_v = std::numeric_limits<double>::lowest();
    double min_v = std::numeric_limits<double>::max();

    std::vector<double> converted;
    converted.reserve(timer.elapsed_times.size());

    for (size_t i = 0; i < timer.elapsed_times.size(); ++i) {
        const double elapsed = ConvertElapsedTime(timer.elapsed_times[i], NANOSECONDS, unit);
        converted.push_back(elapsed);
        total += elapsed;
        max_v = std::max(max_v, elapsed);
        min_v = std::min(min_v, elapsed);

        if (show_details) {
            std::ostringstream line;
            line << "TimerName=\"" << timer.name << "\""
                 << " Message=\"" << timer.messages[i] << "\""
                 << " Elapsed=" << elapsed;
            Logger::InfoLog("", line.str());
        }
    }

    const double avg = total / static_cast<double>(converted.size());

    // Sample variance is not strictly required; here we use population variance for simplicity.
    double var = 0.0;
    if (converted.size() > 1) {
        for (const double v : converted) {
            const double d = v - avg;
            var += d * d;
        }
        var /= static_cast<double>(converted.size());
    }
    const double norm_var = (avg != 0.0) ? (var / (avg * avg)) : 0.0;

    std::ostringstream summary;
    summary << std::fixed << std::setprecision(3);

    std::ostringstream summary_header;
    summary_header << "[Summary] Name=\"" << timer.name << "\""
                   << " Unit=" << unit_str
                   << " Message=\"" << msg << "\" ";

    summary.str("");
    summary.clear();
    summary << "Total=" << total;
    Logger::InfoLog("", summary_header.str() + summary.str());

    summary.str("");
    summary.clear();
    summary << "Avg=" << avg;
    Logger::InfoLog("", summary_header.str() + summary.str());

    if (show_details) {
        summary.str("");
        summary.clear();
        summary << "Max=" << max_v << " Min=" << min_v << " Var=" << norm_var;
        Logger::InfoLog("", summary_header.str() + summary.str());
    }
}

void TimerManager::PrintAllResults(const std::string &msg,
                                   const TimeUnit     unit,
                                   const bool         show_details) {
    const int32_t prev = current_timer_id_;    // restore selection afterward
    for (int32_t i = 0; i < timer_count_; ++i) {
        if (timers_.find(i) == timers_.end())
            continue;
        SelectTimer(i);
        PrintCurrentResults(msg, unit, show_details);
    }
    current_timer_id_ = prev;
}

// Base unit: nanoseconds
double TimerManager::GetElapsedTime(const TimePoint &start, const TimePoint &end) const {
    using namespace std::chrono;
    return duration_cast<nanoseconds>(end - start).count();
}

// Convert between (ns, us, ms, s). 'from' is typically NANOSECONDS for this manager.
double TimerManager::ConvertElapsedTime(double time, TimeUnit from, TimeUnit to) const {
    static const double k      = 1000.0;                        // 1e3 scale between adjacent units
    static const double conv[] = {1.0, k, k * k, k * k * k};    // ns, us, ms, s
    return time * (conv[from] / conv[to]);
}

std::string TimerManager::GetUnitString(TimeUnit unit) const {
    switch (unit) {
        case NANOSECONDS:
            return "ns";
        case MICROSECONDS:
            return "\xC2\xB5s";    // micro sign
        case MILLISECONDS:
            return "ms";
        case SECONDS:
            return "s";
        default:
            return "ms";
    }
}

}    // namespace ringoa
