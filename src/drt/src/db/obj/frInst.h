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

#ifndef _FR_INST_H_
#define _FR_INST_H_

#include <memory>

#include "db/obj/frBlockage.h"
#include "db/obj/frInstBlockage.h"
#include "db/obj/frRef.h"
#include "frBaseTypes.h"

namespace fr {
class frBlock;
class frInstTerm;

class frInst : public frRef
{
 public:
  // constructors
  frInst(const frString& name, frBlock* refBlock)
      : name_(name), refBlock_(refBlock), pinAccessIdx_(0)
  {
  }
  // getters
  const frString& getName() const { return name_; }
  frBlock* getRefBlock() const { return refBlock_; }
  const std::vector<std::unique_ptr<frInstTerm>>& getInstTerms() const
  {
    return instTerms_;
  }
  const std::vector<std::unique_ptr<frInstBlockage>>& getInstBlockages() const
  {
    return instBlockages_;
  }
  int getPinAccessIdx() const { return pinAccessIdx_; }
  // setters
  void addInstTerm(std::unique_ptr<frInstTerm> in)
  {
    instTerms_.push_back(std::move(in));
  }
  void addInstBlockage(std::unique_ptr<frInstBlockage> in)
  {
    instBlockages_.push_back(std::move(in));
  }
  void setPinAccessIdx(int in) { pinAccessIdx_ = in; }
  // others
  frBlockObjectEnum typeId() const override { return frcInst; }

  /* from frRef
   * getOrient
   * setOrient
   * getOrigin
   * setOrigin
   * getTransform
   * setTransform
   */

  frOrient getOrient() const override { return xform_.orient(); }
  void setOrient(const frOrient& tmpOrient) override { xform_.set(tmpOrient); }
  void getOrigin(frPoint& tmpOrigin) const override
  {
    tmpOrigin.set(xform_.xOffset(), xform_.yOffset());
  }
  void setOrigin(const frPoint& tmpPoint) override { xform_.set(tmpPoint); }
  void getTransform(frTransform& xformIn) const override
  {
    xformIn.set(xform_.xOffset(), xform_.yOffset(), xform_.orient());
  }
  void setTransform(const frTransform& xformIn) override
  {
    xform_.set(xformIn.xOffset(), xformIn.yOffset(), xformIn.orient());
  }

  /* from frPinFig
   * hasPin
   * getPin
   * addToPin
   * removeFromPin
   */

  bool hasPin() const override { return false; }
  frPin* getPin() const override { return nullptr; }
  void addToPin(frPin* in) override { ; }
  void removeFromPin() override { ; }

  /* from frConnFig
   * hasNet
   * getNet
   * addToNet
   * removeFromNet
   */

  bool hasNet() const override { return false; }
  frNet* getNet() const override { return nullptr; }
  void addToNet(frNet* in) override { ; }
  void removeFromNet() override { ; }

  /* from frFig
   * getBBox
   * move
   * overlaps
   */

  void getBBox(frBox& boxIn) const override;

  void move(const frTransform& xform) override { ; }
  bool overlaps(const frBox& box) const override { return false; }
  // others
  void getUpdatedXform(frTransform& in, bool noOrient = false) const;
  void getBoundaryBBox(frBox& in) const;

 private:
  frString name_;
  fr::frBlock* refBlock_;
  std::vector<std::unique_ptr<frInstTerm>> instTerms_;
  std::vector<std::unique_ptr<frInstBlockage>> instBlockages_;
  frTransform xform_;
  int pinAccessIdx_;

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version)
  {
    // instTerms_ are intentionally NOT serialized.  This cuts
    // the serializer from recursing across the whole design.  Any
    // instTerm must attach itself to a instance on deserialization.

    (ar) & boost::serialization::base_object<frRef>(*this);
    (ar) & name_;
    (ar) & refBlock_;
    (ar) & instBlockages_;
    (ar) & xform_;
    (ar) & pinAccessIdx_;
  }

  frInst() = default;  // for serialization

  friend class boost::serialization::access;
};

}  // namespace fr

#endif
