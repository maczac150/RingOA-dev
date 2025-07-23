#include "timer_test.h"

#include <cryptoTools/Common/TestCollection.h>

#include "RingOA/utils/logger.h"
#include "RingOA/utils/timer.h"
#include "RingOA/utils/to_string.h"
#include "RingOA/utils/utils.h"

#include <thread>

namespace test_ringoa {

using ringoa::Logger;
using ringoa::TimerManager;
using ringoa::ToString;

void Timer_Test() {
    Logger::DebugLog(LOC, "Timer_Test ...");

    // Create a timer manager
    TimerManager timer_mgr;

    // Create a new timer and assign a name
    int id1 = timer_mgr.CreateNewTimer("Process A");
    timer_mgr.SelectTimer(id1);

    // Repeat process A 10 times
    for (int i = 0; i < 10; ++i) {
        timer_mgr.Start();
        // =====================================
        // Process A
        std::this_thread::sleep_for(std::chrono::milliseconds(10 + i * 20));
        Logger::TraceLog(LOC, "Process A - " + ToString(i));
        // =====================================
        timer_mgr.Stop("i=" + ToString(i));
    }
    // Print the results for the selected timer
    timer_mgr.PrintCurrentResults();

    int id2 = timer_mgr.CreateNewTimer("Process B");
    timer_mgr.SelectTimer(id2);

    timer_mgr.Start();
    // =====================================
    // Process B - 1
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    // =====================================
    timer_mgr.Mark("Process B - 1");

    // =====================================
    // Process B - 2
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    // =====================================
    timer_mgr.Mark("Process B - 2");

    // =====================================
    // Process B - 3
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    // =====================================
    timer_mgr.Mark("Process B - 3");

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    timer_mgr.Stop("Process B finished");

    // Print the results for the selected timer with details
    timer_mgr.PrintCurrentResults("", ringoa::TimeUnit::MICROSECONDS, true);

    Logger::DebugLog(LOC, "Timer_Test - Passed");
}

}    // namespace test_ringoa
