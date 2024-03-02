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

#include "distributed/drUpdate.h"

#include "db/obj/frNet.h"
#include "distributed/frArchive.h"
#include "serialization.h"

namespace drt {

void drUpdate::setPathSeg(const frPathSeg& seg)
{
  obj_type_ = frcPathSeg;
  begin_ = seg.getBeginPoint();
  end_ = seg.getEndPoint();
  layer_ = seg.getLayerNum();
  style_ = seg.getStyle();
  tapered_ = seg.isTapered();
}

void drUpdate::setPatchWire(const frPatchWire& pwire)
{
  obj_type_ = frcPatchWire;
  offsetBox_ = pwire.getOffsetBox();
  begin_ = pwire.getOrigin();
  layer_ = pwire.getLayerNum();
}

void drUpdate::setVia(const frVia& via)
{
  obj_type_ = frcVia;
  bottomConnected_ = via.isBottomConnected();
  topConnected_ = via.isTopConnected();
  tapered_ = via.isTapered();
  begin_ = via.getOrigin();
  viaDef_ = via.getViaDef();
}

void drUpdate::setMarker(const frMarker& marker)
{
  obj_type_ = frcMarker;
  marker_ = marker;
}

frPathSeg drUpdate::getPathSeg() const
{
  frPathSeg seg;
  seg.setPoints(begin_, end_);
  seg.setLayerNum(layer_);
  seg.setStyle(style_);
  seg.setTapered(tapered_);
  return seg;
}
frPatchWire drUpdate::getPatchWire() const
{
  frPatchWire pwire;
  pwire.setOffsetBox(offsetBox_);
  pwire.setOrigin(begin_);
  pwire.setLayerNum(layer_);
  return pwire;
}

frVia drUpdate::getVia() const
{
  frVia via;
  via.setOrigin(begin_);
  via.setTopConnected(topConnected_);
  via.setBottomConnected(bottomConnected_);
  via.setTapered(tapered_);
  via.setViaDef(viaDef_);
  return via;
}

template <class Archive>
void drUpdate::serialize(Archive& ar, const unsigned int version)
{
  (ar) & type_;
  (ar) & index_in_owner_;
  (ar) & begin_;
  (ar) & end_;
  (ar) & style_;
  (ar) & offsetBox_;
  (ar) & layer_;
  (ar) & obj_type_;
  bool tmp = false;
  if (is_loading(ar)) {
    frBlockObject* obj;
    serializeBlockObject(ar, obj);
    net_ = (frNet*) obj;
    (ar) & tmp;
    tapered_ = tmp;
    (ar) & tmp;
    bottomConnected_ = tmp;
    (ar) & tmp;
    topConnected_ = tmp;
  } else {
    frBlockObject* obj = (frBlockObject*) net_;
    serializeBlockObject(ar, obj);
    tmp = tapered_;
    (ar) & tmp;
    tmp = bottomConnected_;
    (ar) & tmp;
    tmp = topConnected_;
    (ar) & tmp;
  }
  if (obj_type_ == frcVia) {
    serializeViaDef(ar, viaDef_);
  }
  if (obj_type_ == frcMarker) {
    (ar) & marker_;
  }
}

template void drUpdate::serialize<frIArchive>(frIArchive& ar,
                                              const unsigned int file_version);

template void drUpdate::serialize<frOArchive>(frOArchive& ar,
                                              const unsigned int file_version);

}  // namespace drt
