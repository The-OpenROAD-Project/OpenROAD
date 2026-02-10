// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#include "db/obj/frMarker.h"

#include <tuple>

#include "db/obj/frBlockObject.h"
#include "db/obj/frFig.h"
#include "db/tech/frConstraint.h"
#include "distributed/frArchive.h"
#include "frBaseTypes.h"
#include "frDesign.h"
#include "odb/geom.h"

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
      std::tuple<frLayerNum, odb::Rect, bool> tup;
      (ar) & tup;
      victims_.emplace_back(obj, tup);
    }
    (ar) & sz;
    while (sz--) {
      frBlockObject* obj;
      serializeBlockObject(ar, obj);
      std::tuple<frLayerNum, odb::Rect, bool> tup;
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
