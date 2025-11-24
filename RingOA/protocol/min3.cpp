#include "min3.h"

#include <cstring>

#include "RingOA/fss/prg.h"
#include "RingOA/sharing/additive_2p.h"
#include "RingOA/utils/logger.h"
#include "RingOA/utils/network.h"
#include "RingOA/utils/timer.h"
#include "RingOA/utils/to_string.h"
#include "RingOA/utils/utils.h"

namespace ringoa {
namespace proto {

void Min3Parameters::PrintParameters() const {
    Logger::DebugLog(LOC, "[Min3 Parameters]" + GetParametersInfo());
}

Min3Key::Min3Key(const uint64_t id, const Min3Parameters &params)
    : ic_key_1(id, params.GetParameters()),
      ic_key_2(id, params.GetParameters()),
      params_(params),
      serialized_size_(CalculateSerializedSize()) {
}

size_t Min3Key::CalculateSerializedSize() const {
    return ic_key_1.GetSerializedSize() + ic_key_2.GetSerializedSize();
}

void Min3Key::Serialize(std::vector<uint8_t> &buffer) const {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Serializing Min3Key");
#endif

    // Serialize the DPF keys
    std::vector<uint8_t> key_buffer;
    ic_key_1.Serialize(key_buffer);
    buffer.insert(buffer.end(), key_buffer.begin(), key_buffer.end());
    key_buffer.clear();
    ic_key_2.Serialize(key_buffer);
    buffer.insert(buffer.end(), key_buffer.begin(), key_buffer.end());

    // Check size
    if (buffer.size() != serialized_size_) {
        Logger::ErrorLog(LOC, "Serialized size mismatch: " + ToString(buffer.size()) + " != " + ToString(serialized_size_));
        return;
    }
}

void Min3Key::Deserialize(const std::vector<uint8_t> &buffer) {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Deserializing Min3Key");
#endif
    size_t offset = 0;

    // Deserialize the DPF keys
    size_t key_size = ic_key_1.GetSerializedSize();
    ic_key_1.Deserialize(std::vector<uint8_t>(buffer.begin() + offset, buffer.begin() + offset + key_size));
    offset += key_size;
    key_size = ic_key_2.GetSerializedSize();
    ic_key_2.Deserialize(std::vector<uint8_t>(buffer.begin() + offset, buffer.begin() + offset + key_size));
}

void Min3Key::PrintKey(const bool detailed) const {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    if (detailed) {
        Logger::DebugLog(LOC, Logger::StrWithSep("Min3 Key"));
        ic_key_1.PrintKey(true);
        ic_key_2.PrintKey(true);
        Logger::DebugLog(LOC, kDash);
    } else {
        Logger::DebugLog(LOC, "Min3 Key");
        ic_key_1.PrintKey(false);
        ic_key_2.PrintKey(false);
    }
#endif
}

void Min3KeyGenerator::OfflineSetUp(const uint64_t num_eval, const std::string &file_path) const {
    ss_.OfflineSetUp(num_eval * 2, file_path + "bt");
}

Min3KeyGenerator::Min3KeyGenerator(
    const Min3Parameters       &params,
    sharing::AdditiveSharing2P &ss_in,
    sharing::AdditiveSharing2P &ss_out)
    : params_(params), gen_(params.GetParameters(), ss_in, ss_out), ss_(ss_in) {
}

std::pair<Min3Key, Min3Key> Min3KeyGenerator::GenerateKeys() const {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Generating Min3 keys");
#endif

    std::array<Min3Key, 2>      keys     = {Min3Key(0, params_), Min3Key(1, params_)};
    std::pair<Min3Key, Min3Key> key_pair = std::make_pair(std::move(keys[0]), std::move(keys[1]));

    std::pair<IntegerComparisonKey, IntegerComparisonKey> ic_key_pair_1 = gen_.GenerateKeys();
    std::pair<IntegerComparisonKey, IntegerComparisonKey> ic_key_pair_2 = gen_.GenerateKeys();

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    key_pair.first.PrintKey();
    key_pair.second.PrintKey();
#endif

    // Return the generated keys as a pair.
    return key_pair;
}

Min3Evaluator::Min3Evaluator(
    const Min3Parameters       &params,
    sharing::AdditiveSharing2P &ss_in,
    sharing::AdditiveSharing2P &ss_out)
    : params_(params), eval_(params.GetParameters(), ss_in, ss_out), ss_(ss_in) {
}

void Min3Evaluator::OnlineSetUp(const uint64_t party_id, const std::string &file_path) {
    ss_.OnlineSetUp(party_id, file_path + "bt");
}

uint64_t Min3Evaluator::EvaluateSharedInput(osuCrypto::Channel &chl, const Min3Key &key, const std::array<uint64_t, 3> &inputs) const {
    uint64_t party_id = key.ic_key_1.ddcf_key.dcf_key.party_id;
    uint64_t x        = inputs[0];
    uint64_t y        = inputs[1];
    uint64_t z        = inputs[2];
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Evaluating Min3 protocol with shared inputs");
    Logger::DebugLog(LOC, "Party ID: " + ToString(party_id));
    std::string party_str = (party_id == 0) ? "[P0]" : "[P1]";
    Logger::DebugLog(LOC, party_str + " inputs: " + ToString(inputs[0]) + ", " + ToString(inputs[1]) + ", " + ToString(inputs[2]));
#endif

    if (party_id == 0) {
        uint64_t less_xy_c = eval_.EvaluateSharedInput(chl, key.ic_key_1, x, y);
        uint64_t small_xy  = 0;
        ss_.EvaluateSelect(party_id, chl, x, y, less_xy_c, small_xy);
        uint64_t less_xyz_c = eval_.EvaluateSharedInput(chl, key.ic_key_2, small_xy, z);
        uint64_t min_xyz    = 0;
        ss_.EvaluateSelect(party_id, chl, small_xy, z, less_xyz_c, min_xyz);
        return min_xyz;

    } else {
        uint64_t less_xy_c = eval_.EvaluateSharedInput(chl, key.ic_key_1, x, y);
        uint64_t small_xy  = 0;
        ss_.EvaluateSelect(party_id, chl, x, y, less_xy_c, small_xy);
        uint64_t less_xyz_c = eval_.EvaluateSharedInput(chl, key.ic_key_2, small_xy, z);
        uint64_t min_xyz    = 0;
        ss_.EvaluateSelect(party_id, chl, small_xy, z, less_xyz_c, min_xyz);
        return min_xyz;
    }
}

}    // namespace proto
}    // namespace ringoa
