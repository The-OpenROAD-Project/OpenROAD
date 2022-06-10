/*
 * Copyright (c) 2021, The Regents of the University of California
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

#ifndef _FR_MTERM_H_
#define _FR_MTERM_H_

#include <memory>

#include "db/obj/frTerm.h"
#include "db/obj/frMPin.h"

namespace fr {
class frMaster;

class frMTerm : public frTerm
{
  public:
  // constructors
  frMTerm(const frString& name)
      : frTerm(name), master_(nullptr), pins_()
  {
  }
  frMTerm(const frMTerm& in)
      : frTerm(in), master_(in.master_)
  {
    for (auto& uPin : in.getPins()) {
      auto pin = uPin.get();
      auto tmp = std::make_unique<frMPin>(*pin);
      addPin(std::move(tmp));
    }
  }
  frMTerm(const frMTerm& in, const dbTransform& xform)
      : frTerm(in), master_(in.master_)
  {
    for (auto& uPin : in.getPins()) {
      auto pin = uPin.get();
      auto tmp = std::make_unique<frMPin>(*pin, xform);
      addPin(std::move(tmp));
    }
  }
  // getters
  bool hasNet() const override { return false; }
  frNet* getNet() const override { return nullptr; }
  frMaster* getMaster() const { return master_; }
  const std::vector<std::unique_ptr<frMPin>>& getPins() const { return pins_; }
  // setters
  void setMaster(frMaster* in) { master_ = in; }
  void addPin(std::unique_ptr<frMPin> in)
  {
    in->setTerm(this);
    for (auto& uFig : in->getFigs()) {
      auto pinFig = uFig.get();
      if (pinFig->typeId() == frcRect) {
        if (bbox_.dx() == 0 && bbox_.dy() == 0)
          bbox_ = static_cast<frRect*>(pinFig)->getBBox();
        else
          bbox_.merge(static_cast<frRect*>(pinFig)->getBBox());
      }
    }
    pins_.push_back(std::move(in));
  }
  // others
  frBlockObjectEnum typeId() const override { return frcMTerm; }
  frAccessPoint* getAccessPoint(frCoord x,
                                frCoord y,
                                frLayerNum lNum,
                                int pinAccessIdx) override
  {
    if (pinAccessIdx == -1) {
      return nullptr;
    }
    for (auto& pin : pins_) {
      if (!pin->hasPinAccess()) {
        continue;
      }
      for (auto& ap : pin->getPinAccess(pinAccessIdx)->getAccessPoints()) {
        if (x == ap->getPoint().x() && y == ap->getPoint().y()
            && lNum == ap->getLayerNum()) {
          return ap.get();
        }
      }
    }
    return nullptr;
  }
  // fills outShapes with copies of the pinFigs
  void getShapes(std::vector<frRect>& outShapes) override
  {
    for (auto& pin : pins_) {
      for (auto& pinShape : pin->getFigs()) {
        if (pinShape->typeId() == frcRect) {
          outShapes.push_back(*static_cast<frRect*>(pinShape.get()));
        }
      }
    }
  }

 protected:
  frMaster* master_;
  std::vector<std::unique_ptr<frMPin>> pins_;  // set later

};

}  // namespace fr

#endif
