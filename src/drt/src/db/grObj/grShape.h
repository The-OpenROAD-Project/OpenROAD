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

#ifndef _GR_SHAPE_H_
#define _GR_SHAPE_H_

#include "db/grObj/grFig.h"
#include "db/infra/frSegStyle.h"

namespace fr {
class frNet;
class grPin;
class frPathSeg;
class grShape : public grPinFig
{
 public:
  // constructors
  grShape() : grPinFig() {}
  // setters
  virtual void setLayerNum(frLayerNum tmpLayerNum) = 0;
  // getters
  virtual frLayerNum getLayerNum() const = 0;
  // others

  /* from grPinFig
   * hasPin
   * getPin
   * addToPin
   * removeFromPin
   */

  /* from grConnFig
   * hasNet
   * getNet
   * getChild
   * getParent
   * getGrChild
   * getGrParent
   * addToNet
   * removeFromNet
   * setChild
   * setParent
   * setChild
   * setParent
   */

  /* from grFig
   * getBBox
   * move
   * overlaps
   */

  virtual void setIter(frListIter<std::unique_ptr<grShape>>& in) = 0;
  virtual frListIter<std::unique_ptr<grShape>> getIter() const = 0;

 protected:
};

class grPathSeg : public grShape
{
 public:
  // constructors
  grPathSeg()
      : grShape(),
        begin(),
        end(),
        layer(0),
        child(nullptr),
        parent(nullptr),
        owner(nullptr)
  {
  }
  grPathSeg(const grPathSeg& in)
      : begin(in.begin),
        end(in.end),
        layer(in.layer),
        child(in.child),
        parent(in.parent),
        owner(in.owner)
  {
  }
  grPathSeg(const frPathSeg& in);
  // getters
  void getPoints(frPoint& beginIn, frPoint& endIn) const
  {
    beginIn.set(begin);
    endIn.set(end);
  }

  // setters
  void setPoints(const frPoint& beginIn, const frPoint& endIn)
  {
    begin.set(beginIn);
    end.set(endIn);
  }
  // others
  frBlockObjectEnum typeId() const override { return grcPathSeg; }

  /* from grShape
   * setLayerNum
   * getLayerNum
   */
  void setLayerNum(frLayerNum numIn) override { layer = numIn; }
  frLayerNum getLayerNum() const override { return layer; }

  /* from grPinFig
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
   */
  // needs to be updated
  void getBBox(frBox& boxIn) const override
  {
    boxIn.set(begin.x(), begin.y(), end.x(), end.y());
  }

  void setIter(frListIter<std::unique_ptr<grShape>>& in) override { iter = in; }
  frListIter<std::unique_ptr<grShape>> getIter() const override { return iter; }

 protected:
  frPoint begin;
  frPoint end;
  frLayerNum layer;
  frBlockObject* child;
  frBlockObject* parent;
  frBlockObject* owner;
  frListIter<std::unique_ptr<grShape>> iter;
};
}  // namespace fr

#endif
