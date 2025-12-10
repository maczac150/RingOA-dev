#include "network.h"

#include "logger.h"

namespace ringoa {

// ===== TwoPartyNetworkManager =====

TwoPartyNetworkManager::TwoPartyNetworkManager(const std::string &channel_name,
                                               const std::string &ip_address,
                                               uint16_t           port)
    : channel_name_(channel_name), ip_address_(ip_address), port_(port), ios_() {
}

void TwoPartyNetworkManager::StartServer(std::function<void(osuCrypto::Channel &)> server_task) {
    server_thread_ = std::thread([&, server_task]() {
        osuCrypto::Session server(ios_, ip_address_, port_, osuCrypto::SessionMode::Server);
        auto               chl = server.addChannel(channel_name_, channel_name_);

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
        Logger::DebugLog(LOC, "=============================");
        Logger::DebugLog(LOC, "[Server] Information");
        Logger::DebugLog(LOC, "=============================");
        Logger::DebugLog(LOC, "IP Address  : " + ip_address_);
        Logger::DebugLog(LOC, "Port        : " + std::to_string(port_));
        Logger::DebugLog(LOC, "Channel Name: " + channel_name_);
        Logger::DebugLog(LOC, "=============================");
#endif

        chl.waitForConnection();
        server_task(chl);
    });
}

void TwoPartyNetworkManager::StartClient(std::function<void(osuCrypto::Channel &)> client_task) {
    client_thread_ = std::thread([&, client_task]() {
        osuCrypto::Session client(ios_, ip_address_, port_, osuCrypto::SessionMode::Client);
        auto               chl = client.addChannel(channel_name_, channel_name_);

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
        Logger::DebugLog(LOC, "=============================");
        Logger::DebugLog(LOC, "[Client] Information");
        Logger::DebugLog(LOC, "=============================");
        Logger::DebugLog(LOC, "IP Address  : " + ip_address_);
        Logger::DebugLog(LOC, "Port        : " + std::to_string(port_));
        Logger::DebugLog(LOC, "Channel Name: " + channel_name_);
        Logger::DebugLog(LOC, "=============================");
#endif

        chl.waitForConnection();
        client_task(chl);
    });
}

void TwoPartyNetworkManager::AutoConfigure(
    int                                       party_id,
    std::function<void(osuCrypto::Channel &)> server_task,
    std::function<void(osuCrypto::Channel &)> client_task) {
    if (party_id == 0) {
        StartServer(server_task);
    } else if (party_id == 1) {
        StartClient(client_task);
    } else {
        StartServer(server_task);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));    // Ensure server starts first
        StartClient(client_task);
    }
}

void TwoPartyNetworkManager::WaitForCompletion() {
    if (server_thread_.joinable())
        server_thread_.join();
    if (client_thread_.joinable())
        client_thread_.join();
    ios_.stop();
}

// ===== ThreePartyNetworkManager =====

ThreePartyNetworkManager::ThreePartyNetworkManager(const std::string &ip_address,
                                                   uint16_t           port)
    : ip_address_(ip_address), port_(port), ios_() {
}

void ThreePartyNetworkManager::Start(const uint32_t party_id, std::function<void(osuCrypto::Channel &, osuCrypto::Channel &)> task) {
    // Start the thread for the specified party
    std::thread *party_thread = nullptr;
    switch (party_id) {
        case 0:
            party_thread = &party0_thread_;
            break;
        case 1:
            party_thread = &party1_thread_;
            break;
        case 2:
            party_thread = &party2_thread_;
            break;
        default:
            Logger::FatalLog(LOC, "Invalid party ID: " + std::to_string(party_id));
            exit(EXIT_FAILURE);
    }

    *party_thread = std::thread([&, party_id, task]() {
        // Set up the network configuration
        uint32_t               id_next           = (party_id + 1) % 3;
        uint32_t               id_prev           = (party_id + 2) % 3;
        std::string            session_name_next = "P" + std::to_string(std::min(party_id, id_next)) + "_P" + std::to_string(std::max(party_id, id_next));
        std::string            session_name_prev = "P" + std::to_string(std::min(party_id, id_prev)) + "_P" + std::to_string(std::max(party_id, id_prev));
        osuCrypto::SessionMode mode_next         = (party_id < id_next) ? osuCrypto::SessionMode::Server : osuCrypto::SessionMode::Client;
        osuCrypto::SessionMode mode_prev         = (party_id < id_prev) ? osuCrypto::SessionMode::Server : osuCrypto::SessionMode::Client;

        // Use distinct ports per unordered pair to avoid collisions when all parties run on the same host.
        auto pair_port = [&](uint32_t a, uint32_t b) -> uint16_t {
            uint32_t lo = std::min(a, b), hi = std::max(a, b);
            if (lo == 0 && hi == 1) return port_;
            if (lo == 0 && hi == 2) return port_ + 1;
            // lo == 1 && hi == 2
            return port_ + 2;
        };
        uint16_t port_next = pair_port(party_id, id_next);
        uint16_t port_prev = pair_port(party_id, id_prev);

        // Create sessions for next and previous parties
        osuCrypto::Session session_next(ios_, ip_address_, port_next, mode_next, session_name_next);
        osuCrypto::Session session_prev(ios_, ip_address_, port_prev, mode_prev, session_name_prev);
        auto               chl_next = session_next.addChannel();
        auto               chl_prev = session_prev.addChannel();

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
        Logger::DebugLog(LOC, "=============================");
        Logger::DebugLog(LOC, "[Party " + std::to_string(party_id) + "] Information");
        Logger::DebugLog(LOC, "=============================");
        Logger::DebugLog(LOC, "IP Address  : " + ip_address_);
        Logger::DebugLog(LOC, "Next Port   : " + std::to_string(port_next));
        Logger::DebugLog(LOC, "Prev Port   : " + std::to_string(port_prev));
        std::string mode_next_str = ((mode_next == osuCrypto::SessionMode::Server) ? "Server" : "Client");
        std::string mode_prev_str = ((mode_prev == osuCrypto::SessionMode::Server) ? "Server" : "Client");
        Logger::DebugLog(LOC, "Next Mode   : " + mode_next_str);
        Logger::DebugLog(LOC, "Prev Mode   : " + mode_prev_str);
        Logger::DebugLog(LOC, "Next Channel: " + session_name_next);
        Logger::DebugLog(LOC, "Prev Channel: " + session_name_prev);
        Logger::DebugLog(LOC, "=============================");
#endif

        // Wait for connections to be established
        chl_next.waitForConnection();

        chl_prev.waitForConnection();

        // Exchange IDs with next and previous parties
        chl_next.send(party_id);
        chl_prev.send(party_id);

        uint32_t id_next_recv, id_prev_recv;
        chl_next.recv(id_next_recv);
        chl_prev.recv(id_prev_recv);

        // Validate the received IDs
        if (id_next_recv != id_next) {
            Logger::FatalLog(LOC, "Party " + std::to_string(party_id) + ": Next party ID mismatch");
            exit(EXIT_FAILURE);
        }
        if (id_prev_recv != id_prev) {
            Logger::FatalLog(LOC, "Party " + std::to_string(party_id) + ": Previous party ID mismatch");
            exit(EXIT_FAILURE);
        }

        // Execute the task with the two channels
        task(chl_next, chl_prev);
    });
}

void ThreePartyNetworkManager::AutoConfigure(
    int                                                             party_id,
    std::function<void(osuCrypto::Channel &, osuCrypto::Channel &)> party0_task,
    std::function<void(osuCrypto::Channel &, osuCrypto::Channel &)> party1_task,
    std::function<void(osuCrypto::Channel &, osuCrypto::Channel &)> party2_task) {
    if (party_id == 0) {
        Start(0, party0_task);
    } else if (party_id == 1) {
        Start(1, party1_task);
    } else if (party_id == 2) {
        Start(2, party2_task);
    } else {
        Start(0, party0_task);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));    // Ensure party 0 starts first
        Start(1, party1_task);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));    // Ensure party 1 starts second
        Start(2, party2_task);
    }
}

void ThreePartyNetworkManager::WaitForCompletion() {
    if (party0_thread_.joinable())
        party0_thread_.join();
    if (party1_thread_.joinable())
        party1_thread_.join();
    if (party2_thread_.joinable())
        party2_thread_.join();
    ios_.stop();
}

}    // namespace ringoa
