#ifndef UTILS_TIMER_H_
#define UTILS_TIMER_H_

#include <chrono>
#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

#include "utils/logger.hpp"

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
     * @param unit
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
     * @brief Set the time unit for the selected timer.
     * @param unit
     */
    void SetUnit(TimeUnit unit);

    /**
     * @brief Start the selected timer.
     */
    void Start();

    /**
     * @brief Stop the selected timer.
     */
    void Stop();

    /**
     * @brief Mark the current time.
     */
    void Mark();

    /**
     * @brief Print the results of the current timer.
     */
    void PrintCurrentResults(const TimeUnit unit = MILLISECONDS) const;

private:
    /**
     * @brief Timer structure to store timer data.
     */
    struct Timer {
        std::string            name;
        std::vector<TimePoint> start_times;
        std::vector<TimePoint> end_times;
        std::vector<double>    elapsed_times;
    };

    std::map<int, Timer> timers_;                          /**< Map of timers with IDs */
    int32_t              current_timer_id_ = -1;           /**< ID of the selected timer */
    TimeUnit             unit_             = MILLISECONDS; /**< Time unit */
    int32_t              timer_count_      = 0;            /**< Timer count */

    /**
     * @brief Get the elapsed time between two time points.
     * @param start The start time point.
     * @param end The end time point.
     * @param unit The time unit to use.
     * @return double The elapsed time in the selected time unit.
     */
    double GetElapsedTime(const TimePoint &start, const TimePoint &end) const;

    /**
     * @brief Get the string representation of a time unit.
     * @param unit The time unit.
     * @return std::string The string representation of the time unit.
     */
    std::string GetUnitString(TimeUnit unit) const;
};

}    // namespace utils

#endif    // UTILS_TIMER_H_
