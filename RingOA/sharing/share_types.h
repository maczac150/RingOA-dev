#ifndef SHARING_SHARE_TYPES_H_
#define SHARING_SHARE_TYPES_H_

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "rep_share.h"

namespace ringoa {
namespace sharing {

constexpr size_t kTwoParties   = 2;
constexpr size_t kThreeParties = 3;

using RepShare32        = RepShare<uint32_t>;
using RepShareVec32     = RepShareVec<uint32_t>;
using RepShareMat32     = RepShareMat<uint32_t>;
using RepShareView32    = RepShareView<uint32_t>;
using RepShare64        = RepShare<uint64_t>;
using RepShareVec64     = RepShareVec<uint64_t>;
using RepShareMat64     = RepShareMat<uint64_t>;
using RepShareView64    = RepShareView<uint64_t>;
using RepShareBlock     = RepShare<block>;
using RepShareVecBlock  = RepShareVec<block>;
using RepShareMatBlock  = RepShareMat<block>;
using RepShareViewBlock = RepShareView<block>;

}    // namespace sharing
}    // namespace ringoa

#endif    // SHARING_SHARE_TYPES_H_
