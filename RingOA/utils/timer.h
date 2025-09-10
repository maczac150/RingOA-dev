#ifndef UTILS_TIMER_H_
#define UTILS_TIMER_H_

#include <chrono>
#include <map>
#include <string>
#include <vector>

namespace ringoa {

/**
 * @brief Enumeration of time units.
 */
enum TimeUnit
{
    NANOSECONDS,  /**< Nanoseconds */
    MICROSECONDS, /**< Microseconds */
    MILLISECONDS, /**< Milliseconds */
    SECONDS       /**< Seconds */
};

/**
 * @brief Alias for high-resolution clock time points.
 */
typedef std::chrono::high_resolution_clock::time_point TimePoint;

/**
 * TimerManager provides simple measurement utilities.
 *
 * Usage flow:
 *   1. CreateNewTimer("name") -> returns timer ID
 *   2. SelectTimer(id)
 *   3. Start() ... Stop("msg")   // measure interval
 *   4. Mark("msg")               // checkpoint without stopping
 *   5. PrintCurrentResults() or PrintAllResults()
 *
 * Features:
 *   - Multiple timers can be created and managed by ID.
 *   - Results can be printed in ns/us/ms/s.
 *   - Each Start/Stop pair records one elapsed time.
 *
 * Notes:
 *   - Not thread-safe.
 *   - Start/Stop calls should be balanced.
 *   - Results accumulate until TimerManager is destroyed.
 *
 * Example:
 *   ringoa::TimerManager tm;
 *   int id = tm.CreateNewTimer("exp1");
 *   tm.SelectTimer(id);
 *   tm.Start();
 *   // ... computation ...
 *   tm.Mark("after step1");
 *   // ... computation ...
 *   tm.Stop("finished");
 *   tm.PrintCurrentResults("Experiment 1", ringoa::MILLISECONDS, true);
 */
class TimerManager {
public:
    TimerManager() {
    }

    int32_t CreateNewTimer(const std::string &name = "");
    void    SelectTimer(int32_t timer_id);
    void    Start();
    void    Stop(const std::string &msg = "");
    /**
     * Mark(): checkpoints the time since the last Start() without stopping the timer.
     * Note: If you call Mark() multiple times after one Start(), each mark grows cumulatively.
     * Summaries sum all recorded entries (including marks).
     */
    void Mark(const std::string &msg = "");
    void PrintCurrentResults(const std::string &msg          = "",
                             TimeUnit           unit         = MILLISECONDS,
                             bool               show_details = false) const;
    void PrintAllResults(const std::string &msg          = "",
                         TimeUnit           unit         = MILLISECONDS,
                         bool               show_details = false);

private:
    struct Timer {
        std::string              name;
        std::vector<TimePoint>   start_times;
        std::vector<TimePoint>   end_times;
        std::vector<double>      elapsed_times;
        std::vector<std::string> messages;
    };

    std::map<int, Timer> timers_;
    int32_t              current_timer_id_ = -1;
    int32_t              timer_count_      = 0;

    double      GetElapsedTime(const TimePoint &start, const TimePoint &end) const;
    double      ConvertElapsedTime(double time, TimeUnit from, TimeUnit to) const;
    std::string GetUnitString(TimeUnit unit) const;
};

}    // namespace ringoa

#endif    // UTILS_TIMER_H_
