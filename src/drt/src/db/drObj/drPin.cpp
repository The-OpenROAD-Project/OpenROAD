// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#include "db/drObj/drPin.h"

#include <algorithm>
#include <limits>
#include <utility>

#include "db/drObj/drBlockObject.h"
#include "distributed/frArchive.h"
#include "dr/FlexMazeTypes.h"
#include "frBaseTypes.h"

namespace drt {

std::pair<FlexMazeIdx, FlexMazeIdx> drPin::getAPBbox()
{
  FlexMazeIdx l(std::numeric_limits<frMIdx>::max(),
                std::numeric_limits<frMIdx>::max(),
                std::numeric_limits<frMIdx>::max());
  FlexMazeIdx h(std::numeric_limits<frMIdx>::min(),
                std::numeric_limits<frMIdx>::min(),
                std::numeric_limits<frMIdx>::min());
  for (auto& ap : getAccessPatterns()) {
    FlexMazeIdx mi = ap->getMazeIdx();
    l.set(std::min(l.x(), mi.x()),
          std::min(l.y(), mi.y()),
          std::min(l.z(), mi.z()));
    h.set(std::max(h.x(), mi.x()),
          std::max(h.y(), mi.y()),
          std::max(h.z(), mi.z()));
  }
  return {l, h};
}

template <class Archive>
void drPin::serialize(Archive& ar, const unsigned int version)
{
  (ar) & boost::serialization::base_object<drBlockObject>(*this);
  (ar) & accessPatterns_;
  (ar) & net_;
  serializeBlockObject(ar, term_);
}

// Explicit instantiations
template void drPin::serialize<frIArchive>(frIArchive& ar,
                                           const unsigned int file_version);

template void drPin::serialize<frOArchive>(frOArchive& ar,
                                           const unsigned int file_version);

}  // namespace drt
