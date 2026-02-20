// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <memory>
#include <utility>
#include <vector>

#include "db/obj/frBPin.h"
#include "db/obj/frBlockage.h"
#include "db/obj/frInstBlockage.h"
#include "db/obj/frNet.h"
#include "db/obj/frPin.h"
#include "db/obj/frRef.h"
#include "frBaseTypes.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"

namespace drt {
class frBlock;
class frMaster;
class frInstTerm;

class frInst : public frRef
{
 public:
  // constructors
  frInst(const frString& name, frMaster* master, odb::dbInst* db_inst)
      : name_(name), master_(master), db_inst_(db_inst)
  {
  }
  // used for archive serialization
  frInst() : master_(nullptr), db_inst_(nullptr) {}

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
  void deletePinAccessIdx() { pinAccessIdx_ = -1; }
  bool hasPinAccessIdx() { return pinAccessIdx_ != -1; }
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

  odb::dbOrientType getOrient() const override { return xform_.getOrient(); }
  void setOrient(const odb::dbOrientType& tmpOrient) override
  {
    xform_.setOrient(tmpOrient);
  }
  odb::Point getOrigin() const override { return xform_.getOffset(); }
  void setOrigin(const odb::Point& tmpPoint) override
  {
    xform_.setOffset(tmpPoint);
  }
  odb::dbTransform getTransform() const override { return xform_; }
  void setTransform(const odb::dbTransform& xformIn) override
  {
    xform_ = xformIn;
  }
  odb::dbInst* getDBInst() const { return db_inst_; }
  odb::dbTransform getDBTransform() const { return db_inst_->getTransform(); }

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

  odb::Rect getBBox() const override;

  void move(const odb::dbTransform& xform) override { ; }
  bool intersects(const odb::Rect& box) const override { return false; }
  // others
  odb::dbTransform getNoRotationTransform() const;
  odb::Rect getBoundaryBBox() const;

  frInstTerm* getInstTerm(int index);
  bool hasPinAccessUpdate() const { return has_pin_access_update_; }
  void setHasPinAccessUpdate(bool in) { has_pin_access_update_ = in; }
  odb::dbTransform getLatestPATransform() const { return latest_pa_xform_; }
  void setLatestPATransform() { latest_pa_xform_ = xform_; }

 private:
  frString name_;
  frMaster* master_;
  std::vector<std::unique_ptr<frInstTerm>> instTerms_;
  std::vector<std::unique_ptr<frInstBlockage>> instBlockages_;
  odb::dbInst* db_inst_;
  odb::dbTransform xform_;
  odb::dbTransform latest_pa_xform_;
  int pinAccessIdx_{-1};
  bool toBeDeleted_{false};
  bool has_pin_access_update_{true};
};

}  // namespace drt
