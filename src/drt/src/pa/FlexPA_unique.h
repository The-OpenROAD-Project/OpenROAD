/*
 * Copyright (c) 2023, The Regents of the University of California
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
  using InstSet = std::set<frInst*, frBlockObjectComp>;
  // if target_insts is non-empty then analysis is limited to
  // those instances.
  UniqueInsts(frDesign* design,
              const frCollection<odb::dbInst*>& target_insts,
              Logger* logger);

  /**
   * @brief Initializes Unique Instances and Pin Acess data.
   */
  void init();

  // Get's the index corresponding to the inst's unique instance
  int getIndex(frInst* inst);
  // Get's the pin access index corresponding to the inst
  int getPAIndex(frInst* inst) const;

  // Gets the instances in the equivalence set of the given inst
  InstSet* getClass(frInst* inst) const;

  const std::vector<frInst*>& getUnique() const;
  frInst* getUnique(int idx) const;
  bool hasUnique(frInst* inst) const;

  void report() const;
  void setDesign(frDesign* design) { design_ = design; }

 private:
  using LayerRange = std::pair<frLayerNum, frLayerNum>;
  using MasterLayerRange = std::map<frMaster*, LayerRange, frBlockObjectComp>;

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
  std::vector<frTrackPattern*> getPrefTrackPatterns();
  void applyPatternsFile(const char* file_path);

  /**
   * @brief Computes all unique instances data structures
   *
   * Proxies computeUnique, only initializing the input data strcutures before.
   *
   * @todo This function can probably be eliminated
   */
  void initUniqueInstance();

  /**
   * @brief Initializes pin access structures
   * Fills unique_to_pa_idx_adds pin access unique points to pins
   */
  void initPinAccess();

  /**
   * @brief Creates a map from Master instance to LayerRanges.
   *
   * LayerRange represents the lower and upper layer of a Master instance.
   *
   * @return A map from Master instance to LayerRange.
   */
  MasterLayerRange initMasterToPinLayerRange();

  /**
   * @brief Computes all unique instances data structures.
   *
   * @param master_to_pin_layer_range Map from a master instance to layerRange.
   * @param pref_track_patterns Vector of preffered track patterns.
   * Fills: master_OT_to_insts_, inst_to_unique_, inst_to_class_ and
   * unique_to_idx_.
   */
  void computeUnique(const MasterLayerRange& master_to_pin_layer_range,
                     const std::vector<frTrackPattern*>& pref_track_patterns);

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

  // All the unique instances
  std::vector<frInst*> unique_;
  // Mapp all instances to their representative unique instance
  std::map<frInst*, frInst*, frBlockObjectComp> inst_to_unique_;
  // Maps all instances to the set of instances with the same unique inst
  std::unordered_map<frInst*, InstSet*> inst_to_class_;
  // Maps a unique instance to its pin access index
  std::map<frInst*, int, frBlockObjectComp> unique_to_pa_idx_;
  // Maps a unique instance to its index in unique_
  std::map<frInst*, int, frBlockObjectComp> unique_to_idx_;
  // master orient track-offset to instances
  std::map<frMaster*,
           std::map<dbOrientType, std::map<std::vector<frCoord>, InstSet>>,
           frBlockObjectComp>
      master_orient_trackoffset_to_insts_;
};

}  // namespace drt
