#include "file_io_test.h"

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

void File_Io_Test() {
    Logger::InfoLog(LOC, "File_Io_Test...");

    fsswm::FileIo io;
    // Write data to files
    uint32_t                 val     = 123;
    std::string              str     = "test";
    std::vector<uint32_t>    vec     = {1, 2, 3};
    std::vector<std::string> str_vec = {"a", "b", "c"};
    std::array<uint32_t, 2>  arr     = {1, 2};

    uint32_t                val_bin = 0x12345678;
    std::vector<uint32_t>   vec_bin = {0x12345678, 0x87654321};
    std::array<uint32_t, 2> arr_bin = {0x12345678, 0x87654321};

    Logger::DebugLog(LOC, "val: " + ToString(val));
    Logger::DebugLog(LOC, "str: " + str);
    Logger::DebugLog(LOC, "vec: " + ToString(vec));
    Logger::DebugLog(LOC, "str_vec: " + ToString(str_vec));
    Logger::DebugLog(LOC, "arr: " + ToString(arr));
    Logger::DebugLog(LOC, "val_bin: " + ToString(val_bin));
    Logger::DebugLog(LOC, "vec_bin: " + ToString(vec_bin));
    Logger::DebugLog(LOC, "arr_bin: " + ToString(arr_bin));

    io.WriteToFile(kTestFileIoPath + "val", val);
    io.WriteToFile(kTestFileIoPath + "str", str);
    io.WriteToFile(kTestFileIoPath + "vec", vec);
    io.WriteToFile(kTestFileIoPath + "str_vec", str_vec);
    io.WriteToFile(kTestFileIoPath + "arr", arr);
    io.WriteToFileBinary(kTestFileIoPath + "val_bin", val_bin);
    io.WriteToFileBinary(kTestFileIoPath + "vec_bin", vec_bin);
    io.WriteToFileBinary(kTestFileIoPath + "arr_bin", arr_bin);

    // Read data from files
    uint32_t                 val_read;
    std::string              str_read;
    std::vector<uint32_t>    vec_read;
    std::vector<std::string> str_vec_read;
    std::array<uint32_t, 2>  arr_read;
    uint32_t                 val_bin_read;
    std::vector<uint32_t>    vec_bin_read;
    std::array<uint32_t, 2>  arr_bin_read;

    io.ReadFromFile(kTestFileIoPath + "val", val_read);
    io.ReadFromFile(kTestFileIoPath + "str", str_read);
    io.ReadFromFile(kTestFileIoPath + "vec", vec_read);
    io.ReadFromFile(kTestFileIoPath + "str_vec", str_vec_read);
    io.ReadFromFile(kTestFileIoPath + "arr", arr_read);
    io.ReadFromFileBinary(kTestFileIoPath + "val_bin", val_bin_read);
    io.ReadFromFileBinary(kTestFileIoPath + "vec_bin", vec_bin_read);
    io.ReadFromFileBinary(kTestFileIoPath + "arr_bin", arr_bin_read);

    Logger::DebugLog(LOC, "val_read: " + ToString(val_read));
    Logger::DebugLog(LOC, "str_read: " + str_read);
    Logger::DebugLog(LOC, "vec_read: " + ToString(vec_read));
    Logger::DebugLog(LOC, "str_vec_read: " + ToString(str_vec_read));
    Logger::DebugLog(LOC, "arr_read: " + ToString(arr_read));
    Logger::DebugLog(LOC, "val_bin_read: " + ToString(val_bin_read));
    Logger::DebugLog(LOC, "vec_bin_read: " + ToString(vec_bin_read));
    Logger::DebugLog(LOC, "arr_bin_read: " + ToString(arr_bin_read));

    // Check if the data was read correctly
    if (val != val_read)
        throw osuCrypto::UnitTestFail("Failed to read val correctly.");
    if (str != str_read)
        throw osuCrypto::UnitTestFail("Failed to read str correctly.");
    if (vec != vec_read)
        throw osuCrypto::UnitTestFail("Failed to read vec correctly.");
    if (str_vec != str_vec_read)
        throw osuCrypto::UnitTestFail("Failed to read str_vec correctly.");
    if (arr != arr_read)
        throw osuCrypto::UnitTestFail("Failed to read arr correctly.");
    if (val_bin != val_bin_read)
        throw osuCrypto::UnitTestFail("Failed to read val_bin correctly.");
    if (vec_bin != vec_bin_read)
        throw osuCrypto::UnitTestFail("Failed to read vec_bin correctly.");
    if (arr_bin != arr_bin_read)
        throw osuCrypto::UnitTestFail("Failed to read arr_bin correctly.");

    //
    // --- Additional tests (keep original code above) ---
    //
    // 1. Additional arithmetic types
    int    val_int    = -42;
    double val_double = 3.1415926535897;
    float  val_float  = 2.71828f;

    // 2. Additional containers with arithmetic types
    std::vector<float> vec_float = {1.5f, 2.5f, 3.5f};
    std::array<int, 3> arr_int   = {10, 20, 30};
    std::vector<int>   vec_int   = {100, 200, 300};

    // 3. Binary tests for these types
    int                val_int_bin   = -123456;
    std::vector<float> vec_float_bin = {11.11f, 22.22f, 33.33f};
    std::array<int, 3> arr_int_bin   = {1001, 1002, 1003};

    // Log original data
    Logger::DebugLog(LOC, "val_int: " + ToString(val_int));
    Logger::DebugLog(LOC, "val_double: " + ToString(val_double));
    Logger::DebugLog(LOC, "val_float: " + ToString(val_float));
    Logger::DebugLog(LOC, "vec_float: " + ToString(vec_float));
    Logger::DebugLog(LOC, "arr_int: " + ToString(arr_int));
    Logger::DebugLog(LOC, "vec_int: " + ToString(vec_int));
    Logger::DebugLog(LOC, "val_int_bin: " + ToString(val_int_bin));
    Logger::DebugLog(LOC, "vec_float_bin: " + ToString(vec_float_bin));
    Logger::DebugLog(LOC, "arr_int_bin: " + ToString(arr_int_bin));

    // Write additional types (text)
    io.WriteToFile(kTestFileIoPath + "val_int", val_int);
    io.WriteToFile(kTestFileIoPath + "val_double", val_double);
    io.WriteToFile(kTestFileIoPath + "val_float", val_float);
    io.WriteToFile(kTestFileIoPath + "vec_float", vec_float);
    io.WriteToFile(kTestFileIoPath + "arr_int", arr_int);
    io.WriteToFile(kTestFileIoPath + "vec_int", vec_int);

    // Write additional types (binary)
    io.WriteToFileBinary(kTestFileIoPath + "val_int_bin", val_int_bin);
    io.WriteToFileBinary(kTestFileIoPath + "vec_float_bin", vec_float_bin);
    io.WriteToFileBinary(kTestFileIoPath + "arr_int_bin", arr_int_bin);

    // Prepare variables to read them back
    int                val_int_read    = 0;
    double             val_double_read = 0.0;
    float              val_float_read  = 0.0f;
    std::vector<float> vec_float_read;
    std::array<int, 3> arr_int_read = {};
    std::vector<int>   vec_int_read;

    int                val_int_bin_read = 0;
    std::vector<float> vec_float_bin_read;
    std::array<int, 3> arr_int_bin_read = {};

    // Read additional types (text)
    io.ReadFromFile(kTestFileIoPath + "val_int", val_int_read);
    io.ReadFromFile(kTestFileIoPath + "val_double", val_double_read);
    io.ReadFromFile(kTestFileIoPath + "val_float", val_float_read);
    io.ReadFromFile(kTestFileIoPath + "vec_float", vec_float_read);
    io.ReadFromFile(kTestFileIoPath + "arr_int", arr_int_read);
    io.ReadFromFile(kTestFileIoPath + "vec_int", vec_int_read);

    // Read additional types (binary)
    io.ReadFromFileBinary(kTestFileIoPath + "val_int_bin", val_int_bin_read);
    io.ReadFromFileBinary(kTestFileIoPath + "vec_float_bin", vec_float_bin_read);
    io.ReadFromFileBinary(kTestFileIoPath + "arr_int_bin", arr_int_bin_read);

    // Log read data
    Logger::DebugLog(LOC, "val_int_read: " + ToString(val_int_read));
    Logger::DebugLog(LOC, "val_double_read: " + ToString(val_double_read));
    Logger::DebugLog(LOC, "val_float_read: " + ToString(val_float_read));
    Logger::DebugLog(LOC, "vec_float_read: " + ToString(vec_float_read));
    Logger::DebugLog(LOC, "arr_int_read: " + ToString(arr_int_read));
    Logger::DebugLog(LOC, "vec_int_read: " + ToString(vec_int_read));
    Logger::DebugLog(LOC, "val_int_bin_read: " + ToString(val_int_bin_read));
    Logger::DebugLog(LOC, "vec_float_bin_read: " + ToString(vec_float_bin_read));
    Logger::DebugLog(LOC, "arr_int_bin_read: " + ToString(arr_int_bin_read));

    // Check if the data was read correctly
    if (val_int != val_int_read)
        throw osuCrypto::UnitTestFail("Failed to read val_int correctly.");
    if (std::fabs(val_double - val_double_read) > 1e-5)
        throw osuCrypto::UnitTestFail("Failed to read val_double correctly.");
    if (std::fabs(val_float - val_float_read) > 1e-6f)
        throw osuCrypto::UnitTestFail("Failed to read val_float correctly.");
    if (vec_float != vec_float_read)
        throw osuCrypto::UnitTestFail("Failed to read vec_float correctly.");
    if (arr_int != arr_int_read)
        throw osuCrypto::UnitTestFail("Failed to read arr_int correctly.");
    if (vec_int != vec_int_read)
        throw osuCrypto::UnitTestFail("Failed to read vec_int correctly.");

    if (val_int_bin != val_int_bin_read)
        throw osuCrypto::UnitTestFail("Failed to read val_int_bin correctly.");
    if (vec_float_bin.size() != vec_float_bin_read.size()) {
        throw osuCrypto::UnitTestFail("Failed to read vec_float_bin size correctly.");
    } else {
        for (size_t i = 0; i < vec_float_bin.size(); ++i) {
            if (std::fabs(vec_float_bin[i] - vec_float_bin_read[i]) > 1e-6f)
                throw osuCrypto::UnitTestFail("Failed to read vec_float_bin correctly (value mismatch).");
        }
    }
    if (arr_int_bin != arr_int_bin_read)
        throw osuCrypto::UnitTestFail("Failed to read arr_int_bin correctly.");

    Logger::DebugLog(LOC, "File_Io_Test - Passed");
}

}    // namespace test_fsswm
