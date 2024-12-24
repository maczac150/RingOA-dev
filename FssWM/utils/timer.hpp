#ifndef UTILS_TIMER_H_
#define UTILS_TIMER_H_

#include <chrono>
#include <string>
#include <vector>

namespace utils {

/**
 * @enum TimeUnit
 * @brief Enumeration of time units.
 *
 * This enumeration defines various time units for representing durations.
 * It includes:
 * - NANOSECONDS: Representing durations in nanoseconds. <br>
 * - MILLISECONDS: Representing durations in milliseconds.
 * - SECONDS: Representing durations in seconds.
 *
 * @note
 * - Use these enum values to specify the unit of time for various time-related operations.
 * - Each enum value corresponds to a specific time unit.
 */
enum TimeUnit
{
    NANOSECONDS,  /**< Nanoseconds */
    MICROSECONDS, /**< Microseconds */
    MILLISECONDS, /**< Milliseconds */
    SECONDS       /**< Seconds */
};

/**
 * @class ExecutionTimer
 * @brief A utility class for measuring execution time.
 *
 * The `ExecutionTimer` class provides a simple interface to measure the execution time
 * of a code segment. It can record the start time and calculate the elapsed time when needed.
 *
 * @note
 * - Use the `Start` method to record the start time.
 * - Use the `End` method to calculate the elapsed time in the specified time unit (default is milliseconds).
 * - The class is based on the high-resolution clock for accurate timing.
 */
class ExecutionTimer {
public:
    /**
     * @brief Default constructor for the ExecutionTimer class.
     */
    ExecutionTimer();

    /**
     * @brief Record the start time for measuring execution duration.
     *
     * Use this method to record the current time as the start time for measuring execution duration.
     */
    void Start();

    /**
     * @brief Calculate the elapsed time since the start time.
     *
     * Use this method to calculate the elapsed time since the start time in the specified time unit.
     *
     * @param time_unit The time unit for representing the elapsed time.
     * @return The elapsed time since the start time in the specified time unit.
     */
    double Print(const std::string &location, const std::string &message = "");

    /**
     * @brief Set the time unit for representing the elapsed time.
     *
     * Use this method to set the time unit for representing the elapsed time.
     *
     * @param time_unit The time unit for representing the elapsed time.
     */
    void SetTimeUnit(const TimeUnit time_unit);

    /**
     * @brief Get the time unit for representing the elapsed time.
     *
     * Use this method to get the time unit for representing the elapsed time.
     *
     * @return The time unit for representing the elapsed time.
     */
    TimeUnit GetTimeUnit() const;

    /**
     * @brief Get the time unit string.
     *
     * Use this method to get the time unit string.
     *
     * @return The time unit string.
     */
    std::string GetTimeUnitStr() const;

    /**
     * @brief Check if the execution time exceeds the specified limit.
     *
     * @param res The measured execution time.
     * @param limit_time_ms The time limit in milliseconds.
     * @return `true`  If the execution time exceeds the limit.
     * @return `false` Otherwise.
     */
    static bool IsExceedLimitTime(const double res, const uint32_t limit_time_ms, const TimeUnit unit);

private:
    std::chrono::time_point<std::chrono::high_resolution_clock> start_; /**< The start time point. */
    std::chrono::time_point<std::chrono::high_resolution_clock> end_;   /**< The end time point. */
    TimeUnit                                                    unit_;  /**< The time unit for representing the elapsed time. */
};

}    // namespace utils

#endif    // UTILS_TIMER_H_
