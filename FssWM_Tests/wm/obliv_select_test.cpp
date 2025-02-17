#include "obliv_select_test.h"

#include "FssWM/sharing/additive_2p.h"
#include "FssWM/sharing/additive_3p.h"
#include "FssWM/utils/logger.h"
#include "FssWM/wm/key_io.h"
#include "FssWM/wm/obliv_select.h"

namespace {

const std::string kCurrentPath = fsswm::GetCurrentDirectory();
const std::string kTestOSPath  = kCurrentPath + "/data/test/wm/";

}    // namespace

namespace test_fsswm {

using fsswm::Logger;
using fsswm::sharing::AdditiveSharing2P;
using fsswm::sharing::ReplicatedSharing3P;
using fsswm::wm::KeyIo;
using fsswm::wm::OblivSelectKey;
using fsswm::wm::OblivSelectKeyGenerator;
using fsswm::wm::OblivSelectParameters;

void OblivSelect_Offline_Test() {
    Logger::DebugLog(LOC, "OblivSelect_Offline_Test");
    std::vector<OblivSelectParameters> params_list = {
        OblivSelectParameters(5),
        // OblivSelectParameters(10),
        // OblivSelectParameters(15),
        // OblivSelectParameters(20),
    };

    for (const auto &params : params_list) {
        params.PrintParameters();
        uint32_t                d = params.GetParameters().GetInputBitsize();
        ReplicatedSharing3P     rss(d);
        AdditiveSharing2P       ass(d);
        OblivSelectKeyGenerator gen(params, rss, ass);
        KeyIo                   key_io;

        // Generate keys
        std::array<OblivSelectKey, 3> keys = gen.GenerateKeys();

        // Save keys
        std::string key_path = kTestOSPath + "oskey_d" + std::to_string(d);
        key_io.SaveKey(key_path + "_0", keys[0]);
        key_io.SaveKey(key_path + "_1", keys[1]);
        key_io.SaveKey(key_path + "_2", keys[2]);
    }
}

void OblivSelect_Online_Test() {
}

}    // namespace test_fsswm
