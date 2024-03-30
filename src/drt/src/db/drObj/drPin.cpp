/* Authors: Osama */
/*
 * Copyright (c) 2022, The Regents of the University of California
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the University nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "db/drObj/drPin.h"

#include "distributed/frArchive.h"

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
