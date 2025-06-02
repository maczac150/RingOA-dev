#include "rep_share.h"

namespace fsswm {
namespace sharing {

// Explicit instantiations for common types
template struct RepShare<uint64_t>;
template struct RepShareVec<uint64_t>;
template struct RepShareMat<uint64_t>;
template struct RepShareView<uint64_t>;
template struct RepShare<block>;
template struct RepShareVec<block>;
template struct RepShareMat<block>;
template struct RepShareView<block>;

}    // namespace sharing
}    // namespace fsswm
