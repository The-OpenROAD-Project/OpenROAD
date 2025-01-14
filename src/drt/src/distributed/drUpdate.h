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

#pragma once
#include "db/obj/frMarker.h"
#include "db/obj/frShape.h"
#include "db/obj/frVia.h"
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
  Point begin_;
  Point end_;
  frSegStyle style_;
  Rect offsetBox_;
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
