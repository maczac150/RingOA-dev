#include "network_test.h"

#include "cryptoTools/Common/TestCollection.h"

#include "FssWM/utils/logger.h"
#include "FssWM/utils/network.h"
#include "FssWM/utils/timer.h"
#include "FssWM/utils/utils.h"

namespace test_fsswm {

using fsswm::utils::Logger;
using fsswm::utils::ToString;
using fsswm::utils::TwoPartyNetworkManager;

void Network_Manager_Test(const oc::CLP &cmd) {
    Logger::DebugLog(LOC, "Network_Manager_Test...");

    // Create NetworkManager
    TwoPartyNetworkManager net_mgr("NetworkManager_Test");

    // Data for communication
    std::string             str_server = "Hello from Server!";
    std::string             str_client = "Hello from Client!";
    std::string             str_server_received, str_client_received;
    uint32_t                val_server = 123;
    uint32_t                val_client = 456;
    uint32_t                val_server_received, val_client_received;
    std::vector<uint32_t>   vec_server = {1, 2, 3, 4, 5};
    std::vector<uint32_t>   vec_client = {5, 4, 3, 2, 1};
    std::vector<uint32_t>   vec_server_received, vec_client_received;
    std::array<uint32_t, 5> arr_server = {1, 2, 3, 4, 5};
    std::array<uint32_t, 5> arr_client = {5, 4, 3, 2, 1};
    std::array<uint32_t, 5> arr_server_received, arr_client_received;

    // Server task
    auto server_task = [&](oc::Channel &chl) {
        chl.recv(str_server_received);
        Logger::DebugLog(LOC, "1. Server received string: " + str_server_received);

        chl.send(str_server);
        Logger::DebugLog(LOC, "1. Server sent string: " + str_server);

        chl.recv(val_server_received);
        Logger::DebugLog(LOC, "2. Server received value: " + std::to_string(val_server_received));

        chl.send(val_server);
        Logger::DebugLog(LOC, "2. Server sent value: " + std::to_string(val_server));

        chl.recv(vec_server_received);
        Logger::DebugLog(LOC, "3. Server received vector: " + ToString(vec_server_received));

        chl.send(vec_server);
        Logger::DebugLog(LOC, "3. Server sent vector: " + ToString(vec_server));

        chl.recv(arr_server_received);
        Logger::DebugLog(LOC, "4. Server received array: " + ToString(arr_server_received));

        chl.send(arr_server);
        Logger::DebugLog(LOC, "4. Server sent array: " + ToString(arr_server));
    };

    // Client task
    auto client_task = [&](oc::Channel &chl) {
        chl.send(str_client);
        Logger::DebugLog(LOC, "1. Client sent string: " + str_client);

        chl.recv(str_client_received);
        Logger::DebugLog(LOC, "1. Client received string: " + str_client_received);

        chl.send(val_client);
        Logger::DebugLog(LOC, "2. Client sent value: " + std::to_string(val_client));

        chl.recv(val_client_received);
        Logger::DebugLog(LOC, "2. Client received value: " + std::to_string(val_client_received));

        chl.send(vec_client);
        Logger::DebugLog(LOC, "3. Client sent vector: " + ToString(vec_client));

        chl.recv(vec_client_received);
        Logger::DebugLog(LOC, "3. Client received vector: " + ToString(vec_client_received));

        chl.send(arr_client);
        Logger::DebugLog(LOC, "4. Client sent array: " + ToString(arr_client));

        chl.recv(arr_client_received);
        Logger::DebugLog(LOC, "4. Client received array: " + ToString(arr_client_received));
    };

    // Run server and client tasks
    int party_id = cmd.isSet("party") ? cmd.get<int>("party") : -1;

    // Configure based on party ID
    net_mgr.AutoConfigure(party_id, server_task, client_task);

    // Wait for completion
    net_mgr.WaitForCompletion();

    // Assertions
    if (party_id == 0) {    // Server
        if (str_server_received != str_client)
            throw oc::UnitTestFail("Server received wrong message");
        if (val_server_received != val_client)
            throw oc::UnitTestFail("Server received wrong message");
        if (vec_server_received != vec_client)
            throw oc::UnitTestFail("Server received wrong message");
        if (arr_server_received != arr_client)
            throw oc::UnitTestFail("Server received wrong message");
    } else if (party_id == 1) {    // Client
        if (str_client_received != str_server)
            throw oc::UnitTestFail("Client received wrong message");
        if (val_client_received != val_server)
            throw oc::UnitTestFail("Client received wrong message");
        if (vec_client_received != vec_server)
            throw oc::UnitTestFail("Client received wrong message");
        if (arr_client_received != arr_server)
            throw oc::UnitTestFail("Client received wrong message");
    } else {
        if (str_server_received != str_client)
            throw oc::UnitTestFail("Server received wrong message");
        if (str_client_received != str_server)
            throw oc::UnitTestFail("Client received wrong message");
        if (val_server_received != val_client)
            throw oc::UnitTestFail("Server received wrong message");
        if (val_client_received != val_server)
            throw oc::UnitTestFail("Client received wrong message");
        if (vec_server_received != vec_client)
            throw oc::UnitTestFail("Server received wrong message");
        if (vec_client_received != vec_server)
            throw oc::UnitTestFail("Client received wrong message");
        if (arr_server_received != arr_client)
            throw oc::UnitTestFail("Server received wrong message");
        if (arr_client_received != arr_server)
            throw oc::UnitTestFail("Client received wrong message");
    }

    Logger::DebugLog(LOC, "Network_Manager_Test - Passed");
}

}    // namespace test_fsswm
