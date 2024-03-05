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

#include "db/drObj/drAccessPattern.h"

#include "distributed/frArchive.h"
namespace drt {

bool drAccessPattern::hasValidAccess(const frDirEnum& dir)
{
  switch (dir) {
    case (frDirEnum::E):
      return validAccess_[0];
      break;
    case (frDirEnum::S):
      return validAccess_[1];
      break;
    case (frDirEnum::W):
      return validAccess_[2];
      break;
    case (frDirEnum::N):
      return validAccess_[3];
      break;
    case (frDirEnum::U):
      return validAccess_[4];
      break;
    case (frDirEnum::D):
      return validAccess_[5];
      break;
    case (frDirEnum::UNKNOWN):
      return false;
  }
  return false;
}

bool drAccessPattern::nextAccessViaDef(const frDirEnum& dir)
{
  bool sol = true;
  if (dir == frDirEnum::U) {
    if ((*vU_).size() == 1) {
      sol = false;
    } else {
      ++vUIdx_;
      if (vUIdx_ >= (int) (*vU_).size()) {
        vUIdx_ -= (int) (*vU_).size();
      }
    }
  } else {
    if ((*vD_).size() == 1) {
      sol = false;
    } else {
      ++vDIdx_;
      if (vDIdx_ >= (int) (*vD_).size()) {
        vDIdx_ -= (int) (*vD_).size();
      }
    }
  }
  return sol;
}

bool drAccessPattern::prevAccessViaDef(const frDirEnum& dir)
{
  bool sol = true;
  if (dir == frDirEnum::U) {
    if ((*vU_).size() == 1) {
      sol = false;
    } else {
      --vUIdx_;
      if (vUIdx_ < 0) {
        vUIdx_ += (int) (*vU_).size();
      }
    }
  } else {
    if ((*vD_).size() == 1) {
      sol = false;
    } else {
      --vDIdx_;
      if (vDIdx_ < 0) {
        vDIdx_ += (int) (*vD_).size();
      }
    }
  }
  return sol;
}

template <class Archive>
void drAccessPattern::serialize(Archive& ar, const unsigned int version)
{
  (ar) & boost::serialization::base_object<drBlockObject>(*this);
  (ar) & beginPoint_;
  (ar) & beginLayerNum_;
  (ar) & beginArea_;
  (ar) & mazeIdx_;
  (ar) & pin_;
  (ar) & validAccess_;
  (ar) & vUIdx_;
  (ar) & vDIdx_;
  (ar) & onTrackX_;
  (ar) & onTrackY_;
  (ar) & pinCost_;
  // vU_ and vD_ are initialized at init and never used in end.(No Need to
  // serialize)
}

// Explicit instantiations
template void drAccessPattern::serialize<frIArchive>(
    frIArchive& ar,
    const unsigned int file_version);

template void drAccessPattern::serialize<frOArchive>(
    frOArchive& ar,
    const unsigned int file_version);

}  // namespace drt
