/* Authors: Lutong Wang and Bangqi Xu */
/*
 * Copyright (c) 2019, The Regents of the University of California
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

#ifndef _GR_VIA_H_
#define _GR_VIA_H_

#include <memory>

#include "db/grObj/grRef.h"
#include "db/tech/frViaDef.h"

namespace fr {

class grVia : public grRef
{
 public:
  // constructor
  grVia()
      : grRef(),
        origin(),
        viaDef(nullptr),
        child(nullptr),
        parent(nullptr),
        owner(nullptr)
  {
  }
  grVia(const grVia& in)
      : grRef(in),
        origin(in.origin),
        viaDef(in.viaDef),
        child(in.child),
        parent(in.parent),
        owner(in.owner)
  {
  }

  // getters
  frViaDef* getViaDef() const { return viaDef; }

  // setters
  void setViaDef(frViaDef* in) { viaDef = in; }

  // others
  frBlockObjectEnum typeId() const override { return grcVia; }

  /* from grRef
   * getOrient
   * setOrient
   * getOrigin
   * setOrigin
   * getTransform
   * setTransform
   */

  frOrient getOrient() const override { return frOrient(); }
  void setOrient(const frOrient& in) override { ; }
  void getOrigin(frPoint& in) const override { in.set(origin); }
  void setOrigin(const frPoint& in) override { origin.set(in); }
  void getTransform(frTransform& in) const override { in.set(origin); }
  void setTransform(const frTransform& in) override { ; }

  /* from gfrPinFig
   * hasPin
   * getPin
   * addToPin
   * removeFromPin
   */
  bool hasPin() const override
  {
    return (owner) && (owner->typeId() == grcPin);
  }
  grPin* getPin() const override { return reinterpret_cast<grPin*>(owner); }
  void addToPin(grPin* in) override
  {
    owner = reinterpret_cast<frBlockObject*>(in);
  }
  void removeFromPin() override { owner = nullptr; }

  /* from grConnFig
   * hasNet
   * getNet
   * hasGrNet
   * getGrNet
   * getChild
   * getParent
   * getGrChild
   * getGrParent
   * addToNet
   * removeFromNet
   * setChild
   * setParent
   */
  // if obj hasNet, then it is global GR net
  // if obj hasGrNet, then it is GR worker subnet
  bool hasNet() const override
  {
    return (owner) && (owner->typeId() == frcNet);
  }
  bool hasGrNet() const override
  {
    return (owner) && (owner->typeId() == grcNet);
  }
  frNet* getNet() const override { return reinterpret_cast<frNet*>(owner); }
  grNet* getGrNet() const override { return reinterpret_cast<grNet*>(owner); }
  frNode* getChild() const override { return reinterpret_cast<frNode*>(child); }
  frNode* getParent() const override
  {
    return reinterpret_cast<frNode*>(parent);
  }
  grNode* getGrChild() const override
  {
    return reinterpret_cast<grNode*>(child);
  }
  grNode* getGrParent() const override
  {
    return reinterpret_cast<grNode*>(parent);
  }
  void addToNet(frBlockObject* in) override { owner = in; }
  void removeFromNet() override { owner = nullptr; }
  void setChild(frBlockObject* in) override { child = in; }
  void setParent(frBlockObject* in) override { parent = in; }

  /* from grFig
   * getBBox
   * move
   * overlaps
   */

  void getBBox(frBox& in) const override
  {
    in.set(origin.x(), origin.y(), origin.x(), origin.y());
  }

  void setIter(frListIter<std::unique_ptr<grVia>>& in) { iter = in; }
  frListIter<std::unique_ptr<grVia>> getIter() const { return iter; }

 protected:
  frPoint origin;
  frViaDef* viaDef;
  frBlockObject* child;
  frBlockObject* parent;
  frBlockObject* owner;
  frListIter<std::unique_ptr<grVia>> iter;
};
}  // namespace fr

#endif
