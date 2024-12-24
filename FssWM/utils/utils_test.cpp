#include "utils.hpp"

#include <thread>

#include "file_io.hpp"
#include "logger.hpp"
#include "timer.hpp"

namespace {

const std::string kCurrentPath    = utils::GetCurrentDirectory();
const std::string kUtilsPath      = kCurrentPath + "/data/test/io/";
const std::string kTestValPath    = kUtilsPath + "val";
const std::string kTestVecPath    = kUtilsPath + "vec";
const std::string kTestStrPath    = kUtilsPath + "str";
const std::string kTestStrVecPath = kUtilsPath + "str_vec";
const std::string kTestLogPath    = kUtilsPath + "log";

}    // namespace

namespace utils {
namespace test {

bool Test_WriteReadValueToFile(const bool debug);
bool Test_WriteReadVectorToFile(const bool debug);
bool Test_WriteReadStringToFile(const bool debug);
bool Test_WriteReadStringVectorToFile(const bool debug);
bool Test_LogAppend(const bool debug);

void Test_FileIo(const uint32_t mode, bool debug) {
    std::vector<std::string> modes         = {"File I/O unit tests", "Write and read value to file", "Write and read vector to file", "Write and read string to file", "Write and read string vector to file", "Log append"};
    uint32_t                 selected_mode = mode;
    if (selected_mode < 1 || selected_mode > modes.size()) {
        utils::OptionHelpMessage(LOCATION, modes);
        exit(EXIT_FAILURE);
    }

    // Create server and client objects based on the party ID.
    utils::PrintText(utils::Logger::StrWithSep(modes[selected_mode - 1]));
    if (selected_mode == 1) {
        debug = false;
        utils::PrintTestResult("Test_WriteReadValueToFile", Test_WriteReadValueToFile(debug));
        utils::PrintTestResult("Test_WriteReadVectorToFile", Test_WriteReadVectorToFile(debug));
        utils::PrintTestResult("Test_WriteReadStringToFile", Test_WriteReadStringToFile(debug));
        utils::PrintTestResult("Test_WriteReadStringVectorToFile", Test_WriteReadStringVectorToFile(debug));
        utils::PrintTestResult("Test_LogAppend", Test_LogAppend(debug));
    } else if (selected_mode == 2) {
        utils::PrintTestResult("Test_WriteReadValueToFile", Test_WriteReadValueToFile(debug));
    } else if (selected_mode == 3) {
        utils::PrintTestResult("Test_WriteReadVectorToFile", Test_WriteReadVectorToFile(debug));
    } else if (selected_mode == 4) {
        utils::PrintTestResult("Test_WriteReadStringToFile", Test_WriteReadStringToFile(debug));
    } else if (selected_mode == 5) {
        utils::PrintTestResult("Test_WriteReadStringVectorToFile", Test_WriteReadStringVectorToFile(debug));
    } else if (selected_mode == 6) {
        utils::PrintTestResult("Test_LogAppend", Test_LogAppend(debug));
    }
    utils::PrintText(utils::kDash);
}

bool Test_WriteReadValueToFile(const bool debug) {
    FileIo io(debug);

    uint32_t x = 12345;
    io.WriteValueToFile(kTestValPath, x);

    uint32_t r_x;
    io.ReadValueFromFile(kTestValPath, r_x);

    return x == r_x;
}

bool Test_WriteReadVectorToFile(const bool debug) {
    FileIo io(debug);

    std::vector<uint32_t> vec = utils::CreateSequence(0, 10);
    io.WriteVectorToFile(kTestVecPath, vec);

    std::vector<uint32_t> r_vec;
    io.ReadVectorFromFile(kTestVecPath, r_vec);

    return vec == r_vec;
}

bool Test_WriteReadStringToFile(const bool debug) {
    FileIo io(debug);

    std::string str = "This is test.";
    io.WriteStringToFile(kTestStrPath, str);

    std::string r_str;
    io.ReadStringFromFile(kTestStrPath, r_str);

    return str == r_str;
}

bool Test_WriteReadStringVectorToFile(const bool debug) {
    FileIo io(debug);

    std::vector<std::string> str_vec = {"This is test.", "Hello world", "What's up?"};
    io.WriteStringVectorToFile(kTestStrVecPath, str_vec);

    return true;
}

bool Test_LogAppend(const bool debug) {
    FileIo io(debug);

    uint32_t                 x       = 12345;
    std::vector<uint32_t>    vec     = utils::CreateSequence(0, 10);
    std::string              str     = "This is test.";
    std::vector<std::string> str_vec = {"This is test.", "Hello world", "#########"};

    bool is_append = true;
    io.WriteValueToFile(kTestLogPath, x);
    io.WriteVectorToFile(kTestLogPath, vec, is_append);
    io.WriteStringToFile(kTestLogPath, str, is_append);
    io.WriteStringVectorToFile(kTestLogPath, str_vec, is_append);

    return true;
}

}    // namespace test
}    // namespace utils
