#include "utils_test.h"

#include <cryptoTools/Common/TestCollection.h>

#include "FssWM/utils/file_io.h"
#include "FssWM/utils/logger.h"
#include "FssWM/utils/utils.h"

namespace {

const std::string kCurrentPath    = fsswm::GetCurrentDirectory();
const std::string kTestFileIoPath = kCurrentPath + "/data/test/utils/";

}    // namespace

namespace test_fsswm {

using fsswm::Logger;
using fsswm::ToString;
using fsswm::ToStringFlatMat;

void Utils_Test() {
    Logger::InfoLog(LOC, "Utils_Test...");

    // Scalar tests
    Logger::DebugLog(LOC, "0 = " + ToString(0));
    Logger::DebugLog(LOC, "12345 = " + ToString(12345));
    Logger::DebugLog(LOC, "-100 = " + ToString(-100));

    // Floating-point tests
    Logger::DebugLog(LOC, "3.14159 = " + ToString(3.14159));
    Logger::DebugLog(LOC, "3.14 = " + ToString(3.14159, 2));
    Logger::DebugLog(LOC, "1.000 = " + ToString(1.0, 3));

    // String tests
    Logger::DebugLog(LOC, "hello = " + ToString(std::string_view("hello")));

    // span-based decimal tests
    {
        std::array<int, 5> arr = {1, 2, 3, 4, 5};
        // default delimiter=" ", max_size=64
        Logger::DebugLog(LOC, "1 2 3 4 5 = " + ToString(std::span<const int>(arr), " ", /*max_size*/ 64));
        // custom delimiter=",", max_size=3
        Logger::DebugLog(LOC, "1,2,3,... = " + ToString(std::span<const int>(arr), ",", 3));
    }

    // span + FormatType tests
    {
        std::vector<uint8_t> v  = {1, 2, 15, 255};
        auto                 sp = std::span<const uint8_t>(v);
        Logger::DebugLog(LOC, "1 2 15 255 = " + ToString(sp));
        Logger::DebugLog(LOC, "1 2 F FF = " + ToString(sp, fsswm::FormatType::kHex));
        Logger::DebugLog(LOC, "00000001 00000010 00001111 11111111 = " + ToString(sp, fsswm::FormatType::kBin));
    }

    // contiguous_range tests
    {
        std::vector<int> vec = {10, 20, 30};
        Logger::DebugLog(LOC, "10 20 30 = " + ToString(vec));
        Logger::DebugLog(LOC, "A,14,1E = " + ToString(vec, fsswm::FormatType::kHex, ","));
    }

    // vector<bool> tests
    {
        std::vector<bool> bv = {true, false, true, false};
        Logger::DebugLog(LOC, "1010 = " + ToString(bv));
    }

    // ToStringFlatMat tests
    {
        std::vector<int> flat = {1, 2, 3, 4, 5, 6};
        Logger::DebugLog(LOC, "[1 2 3],[4 5 6] = " + ToStringFlatMat(flat, 2, 3));
        Logger::DebugLog(LOC, "[1 2],[3 4],[5 6] = " +
                                  ToStringFlatMat(flat, 3, 2));
    }
    Logger::DebugLog(LOC, "Utils_Test - Passed");
}

}    // namespace test_fsswm
