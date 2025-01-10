#ifndef UTILS_TIMER_H_
#define UTILS_TIMER_H_

#include <chrono>
#include <map>
#include <string>
#include <vector>

namespace fsswm {
namespace utils {

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
 * @brief Timer Manager to handle multiple timers.
 */
class TimerManager {
public:
    /**
     * @brief Construct a new Timer Manager object.
     */
    TimerManager() {};

    /**
     * @brief Create a new timer.
     * @param name
     * @return int
     */
    int32_t CreateNewTimer(const std::string &name = "");

    /**
     * @brief Select a timer by ID.
     * @param timer_id
     */
    void SelectTimer(int32_t timer_id);

    /**
     * @brief Start the selected timer.
     */
    void Start();

    /**
     * @brief Stop the selected timer.
     * @param msg The message to record.
     */
    void Stop(const std::string &msg = "");

    /**
     * @brief Mark the current time.
     * @param msg The message to record.
     */
    void Mark(const std::string &msg = "");

    /**
     * @brief Print the results of the current timer.
     * @param msg The message to print.
     * @param unit The time unit to print.
     */
    void PrintCurrentResults(const std::string &msg = "", const TimeUnit unit = MILLISECONDS, const bool show_details = false) const;

    /**
     * @brief Print all results of the timers.
     * @param unit The time unit to print.
     */
    void PrintAllResults(const std::string &msg = "", const TimeUnit unit = MILLISECONDS, const bool show_details = false);

private:
    /**
     * @brief Timer structure to store timer data.
     */
    struct Timer {
        std::string              name;
        std::vector<TimePoint>   start_times;
        std::vector<TimePoint>   end_times;
        std::vector<double>      elapsed_times;
        std::vector<std::string> messages;
    };

    std::map<int, Timer> timers_;                /**< Map of timers with IDs */
    int32_t              current_timer_id_ = -1; /**< ID of the selected timer */
    int32_t              timer_count_      = 0;  /**< Timer count */

    /**
     * @brief Get the elapsed time between two time points.
     * @param start The start time point.
     * @param end The end time point.
     * @return double The elapsed time between the two time points.
     */
    double GetElapsedTime(const TimePoint &start, const TimePoint &end) const;

    /**
     * @brief Convert elapsed time between units.
     * @param time The elapsed time.
     * @param from The source time unit.
     * @param to The target time unit.
     * @return double The converted elapsed time.
     */
    double ConvertElapsedTime(double time, TimeUnit from, TimeUnit to) const;

    /**
     * @brief Get the string representation of a time unit.
     * @param unit The time unit.
     * @return std::string The string representation of the time unit.
     */
    std::string GetUnitString(TimeUnit unit) const;
};

}    // namespace utils
}    // namespace fsswm

#endif    // UTILS_TIMER_H_
