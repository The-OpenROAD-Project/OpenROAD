// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <memory>
#include <utility>
#include <vector>

#include "db/obj/frBlockage.h"
#include "db/obj/frFig.h"
#include "db/obj/frInstBlockage.h"
#include "db/obj/frPin.h"
#include "frBaseTypes.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"

namespace drt {
class frBlock;
class frMaster;
class frInstTerm;

class frInst : public frPinFig
{
 public:
  frInst(frMaster* master, odb::dbInst* db_inst)
      : master_(master), db_inst_(db_inst)
  {
    if (db_inst_ == nullptr) {
      throw std::runtime_error("frInst requires non-null dbInst");
    }
  }
  // used for archive serialization
  frInst() : master_(nullptr), db_inst_(nullptr) {}

  frString getName() const { return db_inst_->getName(); }
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

  frBlockObjectEnum typeId() const override { return frcInst; }

  odb::dbOrientType getOrient() const { return xform_.getOrient(); }
  odb::Point getOrigin() const { return xform_.getOffset(); }
  odb::dbTransform getTransform() const { return xform_; }
  void setTransform(const odb::dbTransform& xformIn) { xform_ = xformIn; }
  odb::dbInst* getDBInst() const { return db_inst_; }
  odb::dbTransform getDBTransform() const { return db_inst_->getTransform(); }

  bool hasPin() const override { return false; }
  frPin* getPin() const override { return nullptr; }
  void addToPin(frPin* in) override { ; }
  void addToPin(frBPin* in) override { ; }
  void removeFromPin() override { ; }

  bool hasNet() const override { return false; }
  frNet* getNet() const override { return nullptr; }
  void addToNet(frNet* in) override { ; }
  void removeFromNet() override { ; }

  odb::Rect getBBox() const override;

  void move(const odb::dbTransform& xform) override { ; }
  bool intersects(const odb::Rect& box) const override { return false; }

  odb::dbTransform getNoRotationTransform() const;
  odb::Rect getBoundaryBBox() const;

  frInstTerm* getInstTerm(int index);
  bool hasPinAccessUpdate() const { return has_pin_access_update_; }
  void setHasPinAccessUpdate(bool in) { has_pin_access_update_ = in; }
  odb::dbTransform getLatestPATransform() const { return latest_pa_xform_; }
  void setLatestPATransform() { latest_pa_xform_ = xform_; }

 private:
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
