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
#include <boost/serialization/variant.hpp>

#include "db/obj/frMarker.h"
#include "db/obj/frShape.h"
#include "db/obj/frVia.h"
namespace fr {
class frNet;
class drUpdate
{
 public:
  enum UpdateType
  {
    ADD_SHAPE,
    ADD_GUIDE,
    REMOVE_FROM_NET,
    REMOVE_FROM_BLOCK
  };
  drUpdate(UpdateType type = ADD_SHAPE)
      : net_(nullptr), order_in_owner_(0), type_(type)
  {
  }
  void setNet(frNet* net) { net_ = net; }
  void setPathSeg(const frPathSeg& seg) { obj_ = seg; }
  void setPatchWire(const frPatchWire& pwire) { obj_ = pwire; }
  void setVia(const frVia& via) { obj_ = via; }
  void setMarker(const frMarker& marker) { obj_ = marker; }
  void setOrderInOwner(int value) { order_in_owner_ = value; }
  void setUpdateType(UpdateType value) { type_ = value; }
  boost::variant<frPathSeg, frPatchWire, frVia, frMarker>& getObj()
  {
    return obj_;
  }
  UpdateType getType() const { return type_; }
  int getOrderInOwner() const { return order_in_owner_; }
  frNet* getNet() const { return net_; }

 private:
  frNet* net_;
  int order_in_owner_;
  UpdateType type_;
  boost::variant<frPathSeg, frPatchWire, frVia, frMarker> obj_;

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version);

  friend class boost::serialization::access;
};
}  // namespace fr