// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#pragma once

#include <map>
#include <unordered_map>
#include <utility>
#include <vector>

#include "frDesign.h"

namespace drt {

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
  using InstSet = frOrderedIdSet<frInst*>;
  // if target_insts is non-empty then analysis is limited to
  // those instances.
  UniqueInsts(frDesign* design,
              const frCollection<odb::dbInst*>& target_insts,
              Logger* logger,
              RouterConfiguration* router_cfg_);

  /**
   * @brief Initializes Unique Instances and Pin Acess data.
   */
  void init();

  // Get's the index corresponding to the inst's unique instance
  int getIndex(frInst* inst);

  // Gets the instances in the equivalence set of the given inst
  InstSet* getClass(frInst* inst) const;

  const std::vector<frInst*>& getUnique() const;
  frInst* getUnique(int idx) const;
  frInst* getUnique(frInst* inst) const;
  bool hasUnique(frInst* inst) const;

  /**
   * @brief Computes the unique class of an inst.
   *
   * @param inst inst to have its unique class computed
   *
   * @returns the unique class.
   */
  UniqueInsts::InstSet& computeUniqueClass(frInst* inst);

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
  frInst* deleteInst(frInst* inst);

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
  bool isNDRInst(frInst& inst);
  bool hasTrackPattern(frTrackPattern* tp, const Rect& box) const;

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

  void initUniqueInstPinAccess(frInst* unique_inst);

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
   * @throws DRT 322 if the pin figure is unsuported (not Rect of Polygon)
   *
   * @param pin Pin to be checked.
   */
  void checkFigsOnGrid(const frMPin* pin);

  frDesign* design_;
  const frCollection<odb::dbInst*>& target_insts_;
  Logger* logger_;
  RouterConfiguration* router_cfg_;

  // All the unique instances
  std::vector<frInst*> unique_;
  // Maps all instances to their representative unique instance
  frOrderedIdMap<frInst*, frInst*> inst_to_unique_;
  // Maps all instances to the set of instances with the same unique inst
  std::unordered_map<frInst*, InstSet*> inst_to_class_;
  // Maps a unique instance to its index in unique_
  frOrderedIdMap<frInst*, int> unique_to_idx_;
  // master orient track-offset to instances
  frOrderedIdMap<
      frMaster*,
      std::map<dbOrientType, std::map<std::vector<frCoord>, InstSet>>>
      master_orient_trackoffset_to_insts_;
  std::vector<frTrackPattern*> pref_track_patterns_;
  MasterLayerRange master_to_pin_layer_range_;
};

}  // namespace drt
