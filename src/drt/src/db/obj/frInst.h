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

#pragma once

#include <memory>

#include "db/obj/frBlockage.h"
#include "db/obj/frInstBlockage.h"
#include "db/obj/frRef.h"
#include "frBaseTypes.h"
#include "odb/db.h"

namespace drt {
class frBlock;
class frMaster;
class frInstTerm;

class frInst : public frRef
{
 public:
  // constructors
  frInst(const frString& name, frMaster* master, odb::dbInst* db_inst)
      : name_(name),
        master_(master),
        db_inst_(db_inst),
        pinAccessIdx_(0),
        toBeDeleted_(false)
  {
  }
  // getters
  const frString& getName() const { return name_; }
  frMaster* getMaster() const { return master_; }
  const std::vector<std::unique_ptr<frInstTerm>>& getInstTerms() const
  {
    return instTerms_;
  }
  const std::vector<std::unique_ptr<frInstBlockage>>& getInstBlockages() const
  {
    return instBlockages_;
  }
  int getPinAccessIdx() const { return pinAccessIdx_; }
  bool isToBeDeleted() const { return toBeDeleted_; }
  // setters
  void addInstTerm(std::unique_ptr<frInstTerm> in)
  {
    instTerms_.push_back(std::move(in));
  }
  void addInstBlockage(std::unique_ptr<frInstBlockage> in)
  {
    in->setIndexeInOwner(instBlockages_.size());
    instBlockages_.push_back(std::move(in));
  }
  void setPinAccessIdx(int in) { pinAccessIdx_ = in; }
  void setToBeDeleted(bool in) { toBeDeleted_ = in; }
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

  dbOrientType getOrient() const override { return xform_.getOrient(); }
  void setOrient(const dbOrientType& tmpOrient) override
  {
    xform_.setOrient(tmpOrient);
  }
  Point getOrigin() const override { return xform_.getOffset(); }
  void setOrigin(const Point& tmpPoint) override { xform_.setOffset(tmpPoint); }
  dbTransform getTransform() const override { return xform_; }
  void setTransform(const dbTransform& xformIn) override { xform_ = xformIn; }
  odb::dbInst* getDBInst() const { return db_inst_; }
  dbTransform getDBTransform() const { return db_inst_->getTransform(); }

  /* from frPinFig
   * hasPin
   * getPin
   * addToPin
   * removeFromPin
   */

  bool hasPin() const override { return false; }
  frPin* getPin() const override { return nullptr; }
  void addToPin(frPin* in) override { ; }
  void addToPin(frBPin* in) override { ; }
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
   * intersects
   */

  Rect getBBox() const override;

  void move(const dbTransform& xform) override { ; }
  bool intersects(const Rect& box) const override { return false; }
  // others
  dbTransform getNoRotationTransform() const;
  Rect getBoundaryBBox() const;

  frInstTerm* getInstTerm(int index);

 private:
  frString name_;
  frMaster* master_;
  std::vector<std::unique_ptr<frInstTerm>> instTerms_;
  std::vector<std::unique_ptr<frInstBlockage>> instBlockages_;
  odb::dbInst* db_inst_;
  dbTransform xform_;
  int pinAccessIdx_;
  bool toBeDeleted_;
};

}  // namespace drt
