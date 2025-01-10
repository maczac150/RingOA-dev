#ifndef UTILS_NETWORK_H_
#define UTILS_NETWORK_H_

#include <functional>
#include <thread>

#include "cryptoTools/Network/Channel.h"
#include "cryptoTools/Network/IOService.h"

namespace fsswm {
namespace utils {

/**
 * @brief Two-Party Network Manager
 *
 * Utility class to manage communication between server and client.
 * Simplifies communication setup, data exchange, and thread management for two-party protocols.
 */
class TwoPartyNetworkManager {
public:
    // Default IP address and port
    static constexpr const char *DEFAULT_IP   = "127.0.0.1";
    static constexpr uint16_t    DEFAULT_PORT = 54321;

    /**
     * @brief Constructor: Sets default or user-specified values
     * @param channel_name Channel name for communication
     * @param ip_address IP address for communication (default: 127.0.0.1)
     * @param port Port number for communication (default: 54321)
     */
    TwoPartyNetworkManager(const std::string &channel_name,
                           const std::string &ip_address = DEFAULT_IP,
                           uint16_t           port       = DEFAULT_PORT);

    /**
     * @brief Start the server
     * @param server_task Task to execute on the server
     */
    void StartServer(std::function<void(oc::Channel &)> server_task);

    /**
     * @brief Start the client
     * @param client_task Task to execute on the client
     */
    void StartClient(std::function<void(oc::Channel &)> client_task);

    /**
     * @brief Automatically configure server and client based on party ID.
     * @param party_id Party ID (0 for server, 1 for client). If unspecified, run both.
     * @param server_task Task to execute on the server
     * @param client_task Task to execute on the client
     */
    void AutoConfigure(int                                party_id,
                       std::function<void(oc::Channel &)> server_task,
                       std::function<void(oc::Channel &)> client_task);

    /**
     * @brief Wait for both server and client threads to complete
     */
    void WaitForCompletion();

    /**
     * @brief Get statistics about the communication.
     * @return A string containing data sent and data received.
     */
    std::string GetStatistics(int party_id) const;

private:
    std::string   channel_name_;
    std::string   ip_address_;
    uint16_t      port_;
    oc::IOService ios_;
    std::thread   server_thread_;
    std::thread   client_thread_;

    // Statistics
    size_t server_sent_;
    size_t client_sent_;
};

}    // namespace utils
}    // namespace fsswm

#endif    // UTILS_NETWORK_H_
