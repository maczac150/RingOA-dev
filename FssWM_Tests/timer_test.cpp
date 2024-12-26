#include "cryptoTools/Common/Defines.h"
#include "cryptoTools/Common/TestCollection.h"

#include "FssWM/utils/logger.hpp"
#include "FssWM/utils/timer.hpp"

#include <thread>

namespace test_fsswm {

void Timer_Test() {
    using namespace utils;

    utils::Logger::InfoLog(LOC, "Timer Test");

    // Create a TimerManager instance with millisecond precision
    TimerManager timer_mgr;

    // Create a new timer and assign a name
    int id1 = timer_mgr.CreateNewTimer("A (x10)");
    // Select the created timer
    timer_mgr.SelectTimer(id1);
    // Repeat process A 10 times
    for (int i = 0; i < 10; i++) {
        // Start the timer
        timer_mgr.Start();
        std::this_thread::sleep_for(std::chrono::milliseconds(100 + i * 20));
        // Stop the timer and record the elapsed time
        timer_mgr.Stop();
    }
    // Print the results for the selected timer
    timer_mgr.PrintCurrentResults();

    // Create a new timer and assign a name
    int id2 = timer_mgr.CreateNewTimer("B (x3)");
    // Select the created timer
    timer_mgr.SelectTimer(id2);

    // Start the timer
    timer_mgr.Start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));    // Wait for 100ms
    timer_mgr.Mark();                                               // Mark 1

    std::this_thread::sleep_for(std::chrono::milliseconds(200));    // Wait for 200ms
    timer_mgr.Mark();                                               // Mark 2

    std::this_thread::sleep_for(std::chrono::milliseconds(150));    // Wait for 150ms
    // Stop the timer and record the elapsed time
    timer_mgr.Stop();

    // Print the results for the selected timer
    timer_mgr.PrintCurrentResults();
}

}    // namespace test_fsswm
