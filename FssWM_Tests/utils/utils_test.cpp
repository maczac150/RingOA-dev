#include "utils_test.h"

#include <cryptoTools/Common/TestCollection.h>

#include "FssWM/utils/logger.h"
#include "FssWM/utils/to_string.h"
#include "FssWM/utils/utils.h"

namespace {

const std::string kCurrentPath    = fsswm::GetCurrentDirectory();
const std::string kTestFileIoPath = kCurrentPath + "/data/test/utils/";

}    // namespace

namespace test_fsswm {

using fsswm::Format, fsswm::FormatMatrix;
using fsswm::Logger;
using fsswm::ToString, fsswm::ToStringMatrix;

void Utils_Test() {
    Logger::InfoLog(LOC, "Utils_Test...");

    // Scalar tests
    Logger::DebugLog(LOC, "0 = " + ToString(0));
    Logger::DebugLog(LOC, "12345 = " + ToString(12345));
    Logger::DebugLog(LOC, "-100 = " + ToString(-100));

    // Floating-point tests
    Logger::DebugLog(LOC, "3.14159 = " + ToString(3.14159));

    // String tests
    Logger::DebugLog(LOC, "hello = " + ToString(std::string_view("hello")));

    // vector<bool> tests
    {
        std::vector<bool> bv = {true, false, true, false};
        Logger::DebugLog(LOC, "1010 = " + ToString(bv));
    }

    // span-based decimal tests
    {
        std::array<int, 5> arr = {1, 2, 3, 4, 5};
        // default delimiter=" ", max_size=100
        Logger::DebugLog(LOC, "1 2 3 4 5 = " + ToString(std::span<const int>(arr), " ", /*max_size*/ 100));
        // custom delimiter=",", max_size=3
        Logger::DebugLog(LOC, "1,2,3,... = " + ToString(std::span<const int>(arr), ",", 3));
    }

    // contiguous_range tests
    {
        std::vector<int> vec = {10, 20, 30};
        Logger::DebugLog(LOC, "10 20 30 = " + ToString(vec));
    }

    // ToStringMatrix tests
    {
        std::vector<int> flat = {1, 2, 3, 4, 5, 6};
        Logger::DebugLog(LOC, "[1 2 3],[4 5 6] = " + ToStringMatrix(flat, 2, 3));
        Logger::DebugLog(LOC, "[1 2],[3 4],[5 6] = " +
                                  ToStringMatrix(flat, 3, 2));
    }

    // block tests
    {
        fsswm::block blk(0x1234567890abcdef, 0xfedcba0987654321);
        Logger::DebugLog(LOC, "Block Hex: " + Format(blk));
        Logger::DebugLog(LOC, "Block Bin: " + Format(blk, fsswm::FormatType::kBin));
    }

    // span-based block tests
    {
        std::vector<fsswm::block> blocks = {
            fsswm::block(0x1234567890abcdef, 0xfedcba0987654321),
            fsswm::block(0x1122334455667788, 0x8877665544332211)};
        Logger::DebugLog(LOC, "Blocks Hex: " + Format(std::span<const fsswm::block>(blocks), fsswm::FormatType::kHex));
        Logger::DebugLog(LOC, "Blocks Bin: " + Format(std::span<const fsswm::block>(blocks), fsswm::FormatType::kBin));
    }

    // contiguous_range block tests
    {
        std::vector<fsswm::block> blocks = {
            fsswm::block(0x1234567890abcdef, 0xfedcba0987654321),
            fsswm::block(0x1122334455667788, 0x8877665544332211)};
        Logger::DebugLog(LOC, "Blocks Hex: " +
                                  FormatMatrix(blocks, 2, 1, fsswm::FormatType::kHex));
        Logger::DebugLog(LOC, "Blocks Bin: " +
                                  FormatMatrix(blocks, 2, 1, fsswm::FormatType::kBin));
    }

    // FormatMatrix block tests
    {
        std::vector<fsswm::block> blocks = {
            fsswm::MakeBlock(0, 0), fsswm::MakeBlock(1, 1),
            fsswm::MakeBlock(2, 2), fsswm::MakeBlock(3, 3),
            fsswm::MakeBlock(4, 4), fsswm::MakeBlock(5, 5)};
        Logger::DebugLog(LOC, "Blocks Hex: " +
                                  FormatMatrix(std::span<const fsswm::block>(blocks), 3, 2, fsswm::FormatType::kHex));
        Logger::DebugLog(LOC, "Blocks Bin: " +
                                  FormatMatrix(std::span<const fsswm::block>(blocks), 2, 3, fsswm::FormatType::kBin));
    }

    Logger::DebugLog(LOC, "Utils_Test - Passed");
}

}    // namespace test_fsswm
