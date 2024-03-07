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
#include "db/obj/frMarker.h"

#include "db/tech/frConstraint.h"
#include "distributed/frArchive.h"
#include "frDesign.h"

namespace drt {

template <class Archive>
void frMarker::serialize(Archive& ar, const unsigned int version)
{
  frDesign* design = ar.getDesign();
  (ar) & boost::serialization::base_object<frFig>(*this);
  (ar) & bbox_;
  (ar) & layerNum_;
  // iter is handled by the owner
  (ar) & vioHasDir_;
  (ar) & vioIsH_;
  (ar) & index_in_owner_;

  if (is_loading(ar)) {
    int conId = -1;
    (ar) & conId;
    if (conId >= 0) {
      constraint_ = design->getTech()->getConstraint(conId);
    } else {
      constraint_ = nullptr;
    }
    int sz = 0;
    (ar) & sz;
    while (sz--) {
      frBlockObject* obj;
      serializeBlockObject(ar, obj);
      srcs_.insert(obj);
    }
    (ar) & sz;
    while (sz--) {
      frBlockObject* obj;
      serializeBlockObject(ar, obj);
      std::tuple<frLayerNum, Rect, bool> tup;
      (ar) & tup;
      victims_.emplace_back(obj, tup);
    }
    (ar) & sz;
    while (sz--) {
      frBlockObject* obj;
      serializeBlockObject(ar, obj);
      std::tuple<frLayerNum, Rect, bool> tup;
      (ar) & tup;
      aggressors_.emplace_back(obj, tup);
    }

  } else {
    int conId;
    if (constraint_ != nullptr) {
      conId = constraint_->getId();
    } else {
      conId = -1;
    }
    (ar) & conId;
    int sz = srcs_.size();
    (ar) & sz;
    for (auto obj : srcs_) {
      serializeBlockObject(ar, obj);
    }
    sz = victims_.size();
    (ar) & sz;
    for (auto [obj, tup] : victims_) {
      serializeBlockObject(ar, obj);
      (ar) & tup;
    }
    sz = aggressors_.size();
    (ar) & sz;
    for (auto [obj, tup] : aggressors_) {
      serializeBlockObject(ar, obj);
      (ar) & tup;
    }
  }
}

// Explicit instantiations
template void frMarker::serialize<frIArchive>(frIArchive& ar,
                                              const unsigned int file_version);

template void frMarker::serialize<frOArchive>(frOArchive& ar,
                                              const unsigned int file_version);

}  // namespace drt
