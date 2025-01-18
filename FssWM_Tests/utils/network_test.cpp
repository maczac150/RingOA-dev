#include "network_test.h"

#include "cryptoTools/Common/TestCollection.h"

#include "FssWM/utils/logger.h"
#include "FssWM/utils/network.h"
#include "FssWM/utils/timer.h"
#include "FssWM/utils/utils.h"

namespace test_fsswm {

using fsswm::utils::Logger;
using fsswm::utils::ThreePartyNetworkManager;
using fsswm::utils::ToString;
using fsswm::utils::TwoPartyNetworkManager;

void Network_TwoPartyManager_Test(const oc::CLP &cmd) {
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

void Network_ThreePartyManager_Test(const oc::CLP &cmd) {
    Logger::DebugLog(LOC, "Network_ThreePartyManager_Test...");

    // Create NetworkManager
    ThreePartyNetworkManager net_mgr;

    // Data for communication
    std::string           str_p0 = "Hello from Party 0!";
    std::string           str_p1 = "Hello from Party 1!";
    std::string           str_p2 = "Hello from Party 2!";
    std::string           str_p0_from_p1, str_p0_from_p2, str_p1_from_p0, str_p1_from_p2, str_p2_from_p0, str_p2_from_p1;
    uint32_t              val_p0 = 100;
    uint32_t              val_p1 = 200;
    uint32_t              val_p2 = 300;
    uint32_t              val_p0_from_p1, val_p0_from_p2, val_p1_from_p0, val_p1_from_p2, val_p2_from_p0, val_p2_from_p1;
    std::vector<uint32_t> vec_p0 = {10, 20, 30};
    std::vector<uint32_t> vec_p1 = {40, 50, 60};
    std::vector<uint32_t> vec_p2 = {70, 80, 90};
    std::vector<uint32_t> vec_p0_from_p1, vec_p0_from_p2, vec_p1_from_p0, vec_p1_from_p2, vec_p2_from_p0, vec_p2_from_p1;

    // Party 0 task
    auto task_p0 = [&](oc::Channel &chl_next, oc::Channel &chl_prev) {
        chl_next.send(str_p0);
        chl_prev.send(str_p0);
        chl_next.recv(str_p0_from_p1);
        chl_prev.recv(str_p0_from_p2);
        Logger::DebugLog(LOC, "[Party 0] received string from Party 1: " + str_p0_from_p1);
        Logger::DebugLog(LOC, "[Party 0] received string from Party 2: " + str_p0_from_p2);

        chl_next.send(val_p0);
        chl_prev.send(val_p0);
        chl_next.recv(val_p0_from_p1);
        chl_prev.recv(val_p0_from_p2);
        Logger::DebugLog(LOC, "[Party 0] received value from Party 1: " + std::to_string(val_p0_from_p1));
        Logger::DebugLog(LOC, "[Party 0] received value from Party 2: " + std::to_string(val_p0_from_p2));

        chl_next.send(vec_p0);
        chl_prev.send(vec_p0);
        chl_next.recv(vec_p0_from_p1);
        chl_prev.recv(vec_p0_from_p2);
        Logger::DebugLog(LOC, "[Party 0] received vector from Party 1: " + ToString(vec_p0_from_p1));
        Logger::DebugLog(LOC, "[Party 0] received vector from Party 2: " + ToString(vec_p0_from_p2));
    };

    // Party 1 task
    auto task_p1 = [&](oc::Channel &chl_next, oc::Channel &chl_prev) {
        chl_prev.recv(str_p1_from_p0);
        chl_next.send(str_p1);
        chl_prev.send(str_p1);
        chl_next.recv(str_p1_from_p2);
        Logger::DebugLog(LOC, "[Party 1] received string from Party 0: " + str_p1_from_p0);
        Logger::DebugLog(LOC, "[Party 1] received string from Party 2: " + str_p1_from_p2);

        chl_prev.recv(val_p1_from_p0);
        chl_next.send(val_p1);
        chl_prev.send(val_p1);
        chl_next.recv(val_p1_from_p2);
        Logger::DebugLog(LOC, "[Party 1] received value from Party 0: " + std::to_string(val_p1_from_p0));
        Logger::DebugLog(LOC, "[Party 1] received value from Party 2: " + std::to_string(val_p1_from_p2));

        chl_prev.recv(vec_p1_from_p0);
        chl_next.send(vec_p1);
        chl_prev.send(vec_p1);
        chl_next.recv(vec_p1_from_p2);
        Logger::DebugLog(LOC, "[Party 1] received vector from Party 0: " + ToString(vec_p1_from_p0));
        Logger::DebugLog(LOC, "[Party 1] received vector from Party 2: " + ToString(vec_p1));
    };

    // Party 2 task
    auto task_p2 = [&](oc::Channel &chl_next, oc::Channel &chl_prev) {
        chl_prev.recv(str_p2_from_p1);
        chl_next.recv(str_p2_from_p0);
        chl_prev.send(str_p2);
        chl_next.send(str_p2);
        Logger::DebugLog(LOC, "[Party 2] received string from Party 0: " + str_p2_from_p0);
        Logger::DebugLog(LOC, "[Party 2] received string from Party 1: " + str_p2_from_p1);

        chl_prev.recv(val_p2_from_p1);
        chl_next.recv(val_p2_from_p0);
        chl_prev.send(val_p2);
        chl_next.send(val_p2);
        Logger::DebugLog(LOC, "[Party 2] received value from Party 0: " + std::to_string(val_p2_from_p0));
        Logger::DebugLog(LOC, "[Party 2] received value from Party 1: " + std::to_string(val_p2_from_p1));

        chl_prev.recv(vec_p2_from_p1);
        chl_next.recv(vec_p2_from_p0);
        chl_prev.send(vec_p2);
        chl_next.send(vec_p2);
        Logger::DebugLog(LOC, "[Party 2] received vector from Party 0: " + ToString(vec_p2_from_p0));
        Logger::DebugLog(LOC, "[Party 2] received vector from Party 1: " + ToString(vec_p2_from_p1));
    };

    // Run tasks for all parties
    int party_id = cmd.isSet("party") ? cmd.get<int>("party") : -1;

    // Configure based on party ID
    net_mgr.AutoConfigure(party_id, task_p0, task_p1, task_p2);

    // Wait for completion
    net_mgr.WaitForCompletion();

    // Assertions
    if (party_id == 0) {
        if (str_p0_from_p1 != str_p1)
            throw oc::UnitTestFail("Party 0 received wrong message from Party 1");
        if (str_p0_from_p2 != str_p2)
            throw oc::UnitTestFail("Party 0 received wrong message from Party 2");
        if (val_p0_from_p1 != val_p1)
            throw oc::UnitTestFail("Party 0 received wrong value from Party 1");
        if (val_p0_from_p2 != val_p2)
            throw oc::UnitTestFail("Party 0 received wrong value from Party 2");
        if (vec_p0_from_p1 != vec_p1)
            throw oc::UnitTestFail("Party 0 received wrong vector from Party 1");
        if (vec_p0_from_p2 != vec_p2)
            throw oc::UnitTestFail("Party 0 received wrong vector from Party 2");
    } else if (party_id == 1) {
        if (str_p1_from_p0 != str_p0)
            throw oc::UnitTestFail("Party 1 received wrong message from Party 0");
        if (str_p1_from_p2 != str_p2)
            throw oc::UnitTestFail("Party 1 received wrong message from Party 2");
        if (val_p1_from_p0 != val_p0)
            throw oc::UnitTestFail("Party 1 received wrong value from Party 0");
        if (val_p1_from_p2 != val_p2)
            throw oc::UnitTestFail("Party 1 received wrong value from Party 2");
        if (vec_p1_from_p0 != vec_p0)
            throw oc::UnitTestFail("Party 1 received wrong vector from Party 0");
        if (vec_p1_from_p2 != vec_p2)
            throw oc::UnitTestFail("Party 1 received wrong vector from Party 2");
    } else if (party_id == 2) {
        if (str_p2_from_p0 != str_p0)
            throw oc::UnitTestFail("Party 2 received wrong message from Party 0");
        if (str_p2_from_p1 != str_p1)
            throw oc::UnitTestFail("Party 2 received wrong message from Party 1");
        if (val_p2_from_p0 != val_p0)
            throw oc::UnitTestFail("Party 2 received wrong value from Party 0");
        if (val_p2_from_p1 != val_p1)
            throw oc::UnitTestFail("Party 2 received wrong value from Party 1");
        if (vec_p2_from_p0 != vec_p0)
            throw oc::UnitTestFail("Party 2 received wrong vector from Party 0");
        if (vec_p2_from_p1 != vec_p1)
            throw oc::UnitTestFail("Party 2 received wrong vector from Party 1");
    } else {
        if (str_p0_from_p1 != str_p1)
            throw oc::UnitTestFail("Party 0 received wrong message from Party 1");
        if (str_p0_from_p2 != str_p2)
            throw oc::UnitTestFail("Party 0 received wrong message from Party 2");
        if (str_p1_from_p0 != str_p0)
            throw oc::UnitTestFail("Party 1 received wrong message from Party 0");
        if (str_p1_from_p2 != str_p2)
            throw oc::UnitTestFail("Party 1 received wrong message from Party 2");
        if (str_p2_from_p0 != str_p0)
            throw oc::UnitTestFail("Party 2 received wrong message from Party 0");
        if (str_p2_from_p1 != str_p1)
            throw oc::UnitTestFail("Party 2 received wrong message from Party 1");
        if (val_p0_from_p1 != val_p1)
            throw oc::UnitTestFail("Party 0 received wrong value from Party 1");
        if (val_p0_from_p2 != val_p2)
            throw oc::UnitTestFail("Party 0 received wrong value from Party 2");
        if (val_p1_from_p0 != val_p0)
            throw oc::UnitTestFail("Party 1 received wrong value from Party 0");
        if (val_p1_from_p2 != val_p2)
            throw oc::UnitTestFail("Party 1 received wrong value from Party 2");
        if (val_p2_from_p0 != val_p0)
            throw oc::UnitTestFail("Party 2 received wrong value from Party 0");
        if (val_p2_from_p1 != val_p1)
            throw oc::UnitTestFail("Party 2 received wrong value from Party 1");
        if (vec_p0_from_p1 != vec_p1)
            throw oc::UnitTestFail("Party 0 received wrong vector from Party 1");
        if (vec_p0_from_p2 != vec_p2)
            throw oc::UnitTestFail("Party 0 received wrong vector from Party 2");
        if (vec_p1_from_p0 != vec_p0)
            throw oc::UnitTestFail("Party 1 received wrong vector from Party 0");
        if (vec_p1_from_p2 != vec_p2)
            throw oc::UnitTestFail("Party 1 received wrong vector from Party 2");
        if (vec_p2_from_p0 != vec_p0)
            throw oc::UnitTestFail("Party 2 received wrong vector from Party 0");
        if (vec_p2_from_p1 != vec_p1)
            throw oc::UnitTestFail("Party 2 received wrong vector from Party 1");
    }

    Logger::DebugLog(LOC, "Network_ThreePartyManager_Test - Passed");
}

}    // namespace test_fsswm
