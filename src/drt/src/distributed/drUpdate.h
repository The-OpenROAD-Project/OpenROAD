// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#pragma once
#include "db/infra/frSegStyle.h"
#include "db/obj/frMarker.h"
#include "db/obj/frShape.h"
#include "db/obj/frVia.h"
#include "db/tech/frViaDef.h"
#include "frBaseTypes.h"
#include "odb/geom.h"
namespace drt {

class frNet;

class drUpdate
{
 public:
  enum UpdateType
  {
    ADD_SHAPE,
    ADD_GUIDE,
    REMOVE_FROM_NET,
    REMOVE_FROM_BLOCK,
    REMOVE_FROM_RQ,
    UPDATE_SHAPE,
    ADD_SHAPE_NET_ONLY
  };
  drUpdate(UpdateType type = ADD_SHAPE,
           frNet* net = nullptr,
           int index_in_owner = 0)
      : net_(net), index_in_owner_(index_in_owner), type_(type)
  {
  }

  void setNet(frNet* net) { net_ = net; }
  void setIndexInOwner(int value) { index_in_owner_ = value; }
  void setUpdateType(UpdateType value) { type_ = value; }
  void setPathSeg(const frPathSeg& seg);
  void setPatchWire(const frPatchWire& pwire);
  void setVia(const frVia& via);
  void setMarker(const frMarker& marker);
  frPathSeg getPathSeg() const;
  frPatchWire getPatchWire() const;
  frVia getVia() const;
  UpdateType getType() const { return type_; }
  int getIndexInOwner() const { return index_in_owner_; }
  frNet* getNet() const { return net_; }
  frBlockObjectEnum getObjTypeId() const { return obj_type_; }
  frMarker getMarker() const { return marker_; }

 private:
  frNet* net_{nullptr};
  int index_in_owner_{0};
  UpdateType type_;
  odb::Point begin_;
  odb::Point end_;
  frSegStyle style_;
  odb::Rect offsetBox_;
  frLayerNum layer_{0};
  bool bottomConnected_{false};
  bool topConnected_{false};
  bool tapered_{false};
  const frViaDef* viaDef_{nullptr};
  frBlockObjectEnum obj_type_{frcBlock};
  frMarker marker_;

  template <class Archive>
  void serialize(Archive& ar, unsigned int version);

  friend class boost::serialization::access;
};

}  // namespace drt
