#include "network.h"

#include "logger.h"

namespace fsswm {
namespace utils {

TwoPartyNetworkManager::TwoPartyNetworkManager(const std::string &channel_name,
                                               const std::string &ip_address,
                                               uint16_t           port)
    : channel_name_(channel_name), ip_address_(ip_address), port_(port), ios_(),
      server_sent_(0), client_sent_(0) {
}

void TwoPartyNetworkManager::StartServer(std::function<void(oc::Channel &)> server_task) {
    server_thread_ = std::thread([&, server_task]() {
        oc::Session server(ios_, ip_address_, port_, oc::SessionMode::Server);
        auto        chl = server.addChannel(channel_name_, channel_name_);

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
        server_sent_ += chl.getTotalDataSent();

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
        Logger::DebugLog(LOC, "=============================");
        Logger::DebugLog(LOC, "[Server] Statistics");
        Logger::DebugLog(LOC, "=============================");
        Logger::DebugLog(LOC, "Total Data Sent   : " + std::to_string(server_sent_) + " bytes");
        Logger::DebugLog(LOC, "=============================");
#endif
    });
}

void TwoPartyNetworkManager::StartClient(std::function<void(oc::Channel &)> client_task) {
    client_thread_ = std::thread([&, client_task]() {
        oc::Session client(ios_, ip_address_, port_, oc::SessionMode::Client);
        auto        chl = client.addChannel(channel_name_, channel_name_);

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
        client_sent_ += chl.getTotalDataSent();

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
        Logger::DebugLog(LOC, "=============================");
        Logger::DebugLog(LOC, "[Client] Statistics");
        Logger::DebugLog(LOC, "=============================");
        Logger::DebugLog(LOC, "Total Data Sent   : " + std::to_string(client_sent_) + " bytes");
        Logger::DebugLog(LOC, "=============================");
#endif
    });
}

void TwoPartyNetworkManager::AutoConfigure(int                                party_id,
                                           std::function<void(oc::Channel &)> server_task,
                                           std::function<void(oc::Channel &)> client_task) {
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

std::string TwoPartyNetworkManager::GetStatistics(int party_id) const {
    std::ostringstream stats;
    if (party_id == 0) {
        stats << "Server sent=" << server_sent_ << " bytes";
    } else if (party_id == 1) {
        stats << "Client sent=" << client_sent_ << " bytes";
    } else {
        stats << "Server sent=" << server_sent_ << " bytes, Client sent=" << client_sent_ << " bytes";
    }
    return stats.str();
}

}    // namespace utils
}    // namespace fsswm
