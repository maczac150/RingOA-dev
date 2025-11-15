#ifndef BENCH_BENCH_COMMON_H_
#define BENCH_BENCH_COMMON_H_

#include <cryptoTools/Common/CLP.h>

#include "RingOA/utils/logger.h"
#include "RingOA/utils/utils.h"

namespace bench_ringoa {

inline std::vector<uint64_t> SelectBitsizes(const osuCrypto::CLP &cmd) {
    std::string size = cmd.getOr<std::string>("size", "default");
    ringoa::Logger::InfoLog(LOC, "Selected bitsizes: " + size);

    if (size == "small") {
        return {14, 16, 18};
    } else if (size == "medium") {
        return {20, 22, 24};
    } else if (size == "large") {
        return {26, 28, 30};
    } else if (size == "even") {
        return {10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30};
    } else {
        return ringoa::CreateSequence(10, 31);
    }
}

inline std::vector<uint64_t> SelectQueryBitsize(const osuCrypto::CLP &cmd) {
    std::string size = cmd.getOr<std::string>("qsize", "default");

    if (size == "small") {
        return {16};
    } else if (size == "medium") {
        return {64};
    } else if (size == "large") {
        return {128};
    } else {
        return {16};
    }
}

constexpr uint64_t kRepeatDefault = 10;

inline const std::string kCurrentPath = ringoa::GetCurrentDirectory();
// key, share, misc
inline const std::string kBenchPirPath    = kCurrentPath + "/data/bench/pir/";
inline const std::string kBenchRingOAPath = kCurrentPath + "/data/bench/ringoa/";
inline const std::string kBenchSotPath    = kCurrentPath + "/data/bench/sot/";
inline const std::string kBenchOsPath     = kCurrentPath + "/data/bench/os/";
inline const std::string kBenchWmPath     = kCurrentPath + "/data/bench/wm/";
inline const std::string kBenchOfmiPath   = kCurrentPath + "/data/bench/ofmi/";
inline const std::string kBenchSotfmiPath = kCurrentPath + "/data/bench/sotfmi/";
// logs
inline const std::string kLogDpfPath    = kCurrentPath + "/data/logs/dpf/";
inline const std::string kLogPirPath    = kCurrentPath + "/data/logs/pir/";
inline const std::string kLogRingOaPath = kCurrentPath + "/data/logs/ringoa/";
inline const std::string kLogSotPath    = kCurrentPath + "/data/logs/sot/";
inline const std::string kLogOsPath     = kCurrentPath + "/data/logs/os/";
inline const std::string kLogWmPath     = kCurrentPath + "/data/logs/wm/";
inline const std::string kLogOfmiPath   = kCurrentPath + "/data/logs/ofmi/";
inline const std::string kLogSotfmiPath = kCurrentPath + "/data/logs/sotfmi/";
// real data
inline const std::string kChromosomePath = kCurrentPath + "/data/bench/grch38/";
inline const std::string kVafDataPath    = kCurrentPath + "/data/bench/icgc/";

}    // namespace bench_ringoa

#endif    // BENCH_BENCH_COMMON_H_
