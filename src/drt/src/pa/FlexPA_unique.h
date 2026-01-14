// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#pragma once

#include <map>
#include <memory>
#include <set>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include "db/obj/frBlockObject.h"
#include "db/obj/frInst.h"
#include "db/obj/frMPin.h"
#include "db/obj/frTrackPattern.h"
#include "db/tech/frTechObject.h"
#include "frBaseTypes.h"
#include "frDesign.h"
#include "global.h"
#include "odb/db.h"
#include "odb/dbTypes.h"

namespace drt {

struct UniqueClassKey
{
  frMaster* master{nullptr};
  odb::dbOrientType orient{odb::dbOrientType::R0};
  std::vector<frCoord> offsets;
  frInst* ndr_inst{nullptr};
  std::set<frTerm*> stubborn_terms;

  UniqueClassKey(frMaster* master_in,
                 const odb::dbOrientType& orient_in,
                 std::vector<frCoord> offsets_in,
                 frInst* ndr_inst_in = nullptr,
                 std::set<frTerm*> stubborn_terms_in = {})
      : master(master_in),
        orient(orient_in),
        offsets(std::move(offsets_in)),
        ndr_inst(ndr_inst_in),
        stubborn_terms(std::move(stubborn_terms_in))
  {
  }

  bool operator<(const UniqueClassKey& other) const
  {
    return std::tie(master, orient, offsets, ndr_inst, stubborn_terms)
           < std::tie(other.master,
                      other.orient,
                      other.offsets,
                      other.ndr_inst,
                      other.stubborn_terms);
  }

  bool operator==(const UniqueClassKey& other) const
  {
    return std::tie(master, orient, offsets, ndr_inst, stubborn_terms)
           == std::tie(other.master,
                       other.orient,
                       other.offsets,
                       other.ndr_inst,
                       other.stubborn_terms);
  }
};

class UniqueClass
{
 public:
  using InstSet = frOrderedIdSet<frInst*>;

  UniqueClass(const UniqueClassKey& key);

  const UniqueClassKey& key() const { return key_; }
  frMaster* getMaster() const { return key_.master; }
  odb::dbOrientType getOrient() const { return key_.orient; }
  const std::vector<frCoord>& getOffsets() const { return key_.offsets; }
  const std::set<frTerm*>& getStubbornTerms() const
  {
    return key_.stubborn_terms;
  }
  const InstSet& getInsts() const { return insts_; }
  int getPinAccessIdx() const { return pin_access_idx_; }
  void setPinAccessIdx(int idx) { pin_access_idx_ = idx; }
  void addInst(frInst* inst);
  void removeInst(frInst* inst);
  bool hasInst(frInst* inst) const;
  frInst* getFirstInst() const;
  bool isSkipTerm(frMTerm* term) const;
  void setSkipTerm(frMTerm* term, bool skip);
  bool isInitialized() const { return pin_access_idx_ != -1; }

 private:
  UniqueClassKey key_;
  InstSet insts_;
  std::map<frMTerm*, bool> skip_term_;
  int pin_access_idx_{-1};
};

// Instances are grouped into equivalence classes based on master,
// orientation, and track-offset.  From each equivalence class a
// representative instance is chosen for analysis - the unique
// instance.
//
// Because multiple unique instances can have the same master,
// any data stored on the master is indexed by an id associated
// with the unique instance.  That index is called the pin access
// index (paidx).  That index is stored on all equivalent instances.

class UniqueInsts
{
 public:
  // if target_insts is non-empty then analysis is limited to
  // those instances.
  UniqueInsts(frDesign* design,
              const frCollection<odb::dbInst*>& target_insts,
              utl::Logger* logger,
              RouterConfiguration* router_cfg_);

  /**
   * @brief Initializes Unique Instances and Pin Acess data.
   */
  void init();

  // Gets the instances in the equivalence set of the given inst
  UniqueClass* getUniqueClass(frInst* inst) const;

  const std::vector<std::unique_ptr<UniqueClass>>& getUniqueClasses() const;
  bool hasUnique(frInst* inst) const;

  /**
   * @brief Computes the unique class key of an inst
   *
   * This function is used to compute the unique class key of an inst. The key
   * is composed by the master, the orientation and the offsets.
   *
   * @param inst inst to have its unique class key computed
   *
   * @returns the unique class key.
   */
  UniqueClassKey computeUniqueClassKey(frInst* inst) const;

  /**
   * @brief Computes the unique class of an inst.
   *
   * @param inst inst to have its unique class computed
   *
   * @returns the unique class.
   */
  UniqueClass* computeUniqueClass(frInst* inst);

  /**
   * @brief Adds the instance to the unique instances structures,
   * inserting new data if it is actually a new unique instance.
   *
   * @param inst instance to be added.
   *
   * @returns True if the instance is the first of its unique class.
   */
  bool addInst(frInst* inst);

  /**
   * @brief deletes an inst from the unique insts structures
   *
   * @param inst instance to be deleted
   *
   * @returns the unique inst that represents the unique class. If the class was
   * deleted returns nullptr
   */
  void deleteInst(frInst* inst);

  void deleteUniqueClass(UniqueClass* unique_class);

  void initUniqueInstPinAccess(UniqueClass* unique_class);
  void report() const;
  void setDesign(frDesign* design) { design_ = design; }

 private:
  using LayerRange = std::pair<frLayerNum, frLayerNum>;
  using MasterLayerRange = frOrderedIdMap<frMaster*, LayerRange>;

  frDesign* getDesign() const { return design_; }
  frTechObject* getTech() const { return design_->getTech(); }

  /**
   * @brief Checks if any net related to the instance has a NonDefaultRule.
   *
   * @param inst A cell instance.
   *
   * @return If instance contains a NonDefaultRule net connected to any
   * terminal.
   */
  bool isNDRInst(frInst* inst) const;
  bool hasTrackPattern(frTrackPattern* tp, const odb::Rect& box) const;

  /**
   * @brief Creates a vector of preferred track patterns.
   *
   * Not every track pattern is a preferred one,
   * this function acts as filter of design_->getTopBlock()->getTrackPatterns()
   * to only take the preferred ones, which are the ones in the routing
   * direction of the layer.
   *
   * @return A vector of track patterns objects.
   */
  void computePrefTrackPatterns();
  void applyPatternsFile(const char* file_path);

  /**
   * @brief Initializes pin access idx of all instances
   */
  void initPinAccess();

  /**
   * @brief Creates a map from Master instance to LayerRanges.
   *
   * LayerRange represents the lower and upper layer of a Master instance.
   */
  void initMasterToPinLayerRange();

  /**
   * @brief Computes all unique instances data structures.
   *
   * Fills: master_OT_to_insts_, inst_to_unique_, inst_to_class_ and
   * unique_to_idx_.
   */
  void computeUnique();

  /**
   * @brief Raises an error if pin shape is illegal.
   *
   * @throws DRT 320/321 if the term has offgrid pin shape
   * @throws DRT 322 if the pin figure is unsuported (not odb::Rect of Polygon)
   *
   * @param pin Pin to be checked.
   */
  void checkFigsOnGrid(const frMPin* pin);

  frDesign* design_;
  const frCollection<odb::dbInst*>& target_insts_;
  utl::Logger* logger_;
  RouterConfiguration* router_cfg_;

  // All unique classes
  std::vector<std::unique_ptr<UniqueClass>> unique_classes_;
  // Map from UniqueClassKey to UniqueClass
  std::map<UniqueClassKey, UniqueClass*> unique_class_by_key_;
  // Map from inst to UniqueClass
  frOrderedIdMap<frInst*, UniqueClass*> inst_to_unique_class_;
  std::vector<frTrackPattern*> pref_track_patterns_;
  MasterLayerRange master_to_pin_layer_range_;
};

}  // namespace drt
