// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "db/obj/frVia.h"

#include "db/drObj/drVia.h"
#include "db/obj/frRef.h"
#include "distributed/frArchive.h"
#include "serialization.h"

namespace drt {

frVia::frVia(const drVia& in)
{
  origin_ = in.getOrigin();
  viaDef_ = in.getViaDef();
  setTapered(in.isTapered());
  setBottomConnected(in.isBottomConnected());
  setTopConnected(in.isTopConnected());
  setIsLonely(in.isLonely());
}

template <class Archive>
void frVia::serialize(Archive& ar, const unsigned int version)
{
  (ar) & boost::serialization::base_object<frRef>(*this);
  (ar) & origin_;
  bool tmp = false;
  if (is_loading(ar)) {
    (ar) & tmp;
    tapered_ = tmp;
    (ar) & tmp;
    bottomConnected_ = tmp;
    (ar) & tmp;
    topConnected_ = tmp;
  } else {
    tmp = tapered_;
    (ar) & tmp;
    tmp = bottomConnected_;
    (ar) & tmp;
    tmp = topConnected_;
    (ar) & tmp;
  }
  (ar) & index_in_owner_;
  serializeViaDef(ar, viaDef_);
  serializeBlockObject(ar, owner_);
}

template void frVia::serialize<frIArchive>(frIArchive& ar,
                                           const unsigned int file_version);

template void frVia::serialize<frOArchive>(frOArchive& ar,
                                           const unsigned int file_version);

}  // namespace drt
