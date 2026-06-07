// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <algorithm>
#include <list>
#include <memory>
#include <utility>
#include <vector>

#include "db/obj/frBlockObject.h"
#include "db/obj/frFig.h"
#include "db/obj/frGuide.h"
#include "db/obj/frShape.h"
#include "db/obj/frVia.h"
#include "frBaseTypes.h"
#include "global.h"
#include "odb/dbTypes.h"

namespace drt {
class frInstTerm;
class frTerm;
class frBTerm;
class frNonDefaultRule;

class frNet : public frBlockObject
{
 public:
  // constructors
  frNet(const frString& in, RouterConfiguration* router_cfg)
      : name_(in), router_cfg_(router_cfg)
  {
  }
  // getters
  const frString& getName() const { return name_; }
  const std::vector<frInstTerm*>& getInstTerms() const { return instTerms_; }
  const std::vector<frBTerm*>& getBTerms() const { return bterms_; }
  const std::list<std::unique_ptr<frShape>>& getShapes() const
  {
    return shapes_;
  }
  const std::list<std::unique_ptr<frVia>>& getVias() const { return vias_; }
  const std::list<std::unique_ptr<frShape>>& getPatchWires() const
  {
    return pwires_;
  }
  const std::vector<std::unique_ptr<frGuide>>& getGuides() const
  {
    return guides_;
  }
  bool isModified() const { return modified_; }
  bool isFake() const { return isFakeNet_; }
  frNonDefaultRule* getNondefaultRule() const { return ndr_; }
  bool hasInitialRouting() const { return hasInitialRouting_; }
  bool isFixed() const { return isFixed_; }
  bool hasGuides() const { return !guides_.empty(); }
  bool hasBTermsAboveTopLayer() const;
  // setters
  void addInstTerm(frInstTerm* in) { instTerms_.push_back(in); }
  void removeInstTerm(frInstTerm* in)
  {
    for (auto itr = instTerms_.begin(); itr != instTerms_.end(); itr++) {
      if (*itr == in) {
        instTerms_.erase(itr);
        return;
      }
    }
  }
  void addBTerm(frBTerm* in) { bterms_.push_back(in); }
  void setName(const frString& stringIn) { name_ = stringIn; }
  void addShape(std::unique_ptr<frShape> in)
  {
    in->addToNet(this);
    in->setIndexInOwner(all_pinfigs_.size());
    auto rptr = in.get();
    shapes_.push_back(std::move(in));
    rptr->setIter(--shapes_.end());
    all_pinfigs_.push_back(rptr);
  }
  void addVia(std::unique_ptr<frVia> in)
  {
    in->addToNet(this);
    in->setIndexInOwner(all_pinfigs_.size());
    auto rptr = in.get();
    vias_.push_back(std::move(in));
    rptr->setIter(--vias_.end());
    all_pinfigs_.push_back(rptr);
  }
  void addPatchWire(std::unique_ptr<frShape> in)
  {
    in->addToNet(this);
    in->setIndexInOwner(all_pinfigs_.size());
    auto rptr = in.get();
    pwires_.push_back(std::move(in));
    rptr->setIter(--pwires_.end());
    all_pinfigs_.push_back(rptr);
  }
  void addGuide(std::unique_ptr<frGuide> in)
  {
    auto rptr = in.get();
    rptr->addToNet(this);
    in->setIndexInOwner(guides_.size());
    guides_.push_back(std::move(in));
  }
  void clearGuides() { guides_.clear(); }
  void clearOrigGuides() { orig_guides_.clear(); }
  void clearConns()
  {
    instTerms_.clear();
    bterms_.clear();
  }
  void removeShape(frShape* in) { shapes_.erase(in->getIter()); }
  void removeVia(frVia* in) { vias_.erase(in->getIter()); }
  void removePatchWire(frShape* in) { pwires_.erase(in->getIter()); }
  void setModified(bool in) { modified_ = in; }
  void setIsFake(bool in) { isFakeNet_ = in; }
  void setHasInitialRouting(bool in) { hasInitialRouting_ = in; }
  void setFixed(bool in) { isFixed_ = in; }
  // others
  void clearRoutes()
  {
    shapes_.clear();
    vias_.clear();
    pwires_.clear();
  }
  odb::dbSigType getType() const { return type_; }
  void setType(const odb::dbSigType& in) { type_ = in; }
  frBlockObjectEnum typeId() const override { return frcNet; }
  void updateNondefaultRule(frNonDefaultRule* n)
  {
    ndr_ = n;
    updateAbsPriority();
  }
  bool hasNDR() const { return getNondefaultRule() != nullptr; }
  void setAbsPriorityLvl(int l) { absPriorityLvl_ = l; }
  int getAbsPriorityLvl() const { return absPriorityLvl_; }
  bool isClock() const { return isClock_; }
  void updateIsClock(bool ic)
  {
    isClock_ = ic;
    updateAbsPriority();
  }
  void updateAbsPriority()
  {
    int max = absPriorityLvl_;
    if (hasNDR()) {
      max = std::max(max, router_cfg_->NDR_NETS_ABS_PRIORITY);
    }
    if (isClock()) {
      max = std::max(max, router_cfg_->CLOCK_NETS_ABS_PRIORITY);
    }
    absPriorityLvl_ = max;
  }
  bool isSpecial() const { return is_special_; }
  void setIsSpecial(bool s) { is_special_ = s; }
  bool isConnectedByAbutment() const { return is_connected_by_abutment_; }
  void setIsConnectedByAbutment(bool s) { is_connected_by_abutment_ = s; }
  frPinFig* getPinFig(const int& id) { return all_pinfigs_[id]; }
  void setOrigGuides(const std::vector<frRect>& guides)
  {
    orig_guides_ = guides;
  }
  const std::vector<frRect>& getOrigGuides() const { return orig_guides_; }
  void setHasJumpers(bool has_jumpers) { has_jumpers_ = has_jumpers; }
  bool hasJumpers() { return has_jumpers_; }
  void setToBeDeleted(bool to_be_deleted) { to_be_deleted_ = to_be_deleted; }
  bool toBeDeleted() { return to_be_deleted_; }

 protected:
  frString name_;
  RouterConfiguration* router_cfg_;
  std::vector<frInstTerm*> instTerms_;
  std::vector<frBTerm*> bterms_;
  std::list<std::unique_ptr<frShape>> shapes_;
  std::list<std::unique_ptr<frVia>> vias_;
  std::list<std::unique_ptr<frShape>> pwires_;
  std::vector<std::unique_ptr<frGuide>> guides_;
  std::vector<frRect> orig_guides_;
  odb::dbSigType type_{odb::dbSigType::SIGNAL};
  bool modified_{false};
  bool isFakeNet_{false};  // indicate floating PG nets
  frNonDefaultRule* ndr_{nullptr};
  int absPriorityLvl_{0};  // absolute priority level: will be checked in net
                           // ordering before other criteria
  bool isClock_{false};
  bool is_special_{false};
  bool is_connected_by_abutment_{false};
  bool hasInitialRouting_{false};
  bool isFixed_{false};

  // Flag to mark when a frNet has a jumper, which is a special route guide used
  // to prevent antenna violations
  bool has_jumpers_{false};
  std::vector<frPinFig*> all_pinfigs_;
  bool to_be_deleted_{false};
};
}  // namespace drt
