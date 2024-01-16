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

namespace fr {

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
              Logger* logger);

  void init();

  // Get's the index corresponding to the inst's unique instance
  int getIndex(frInst* inst);
  // Get's the pin access index corresponding to the inst
  int getPAIndex(frInst* inst) const;

  // Gets the instances in the equivalence set of the given inst
  std::set<frInst*, frBlockObjectComp>* getClass(frInst* inst) const;

  const std::vector<frInst*>& getUnique() const;
  frInst* getUnique(int idx) const;
  bool hasUnique(frInst* inst) const;

  void report() const;
  void setDesign(frDesign* design) { design_ = design; }

 private:
  using LayerRange = std::tuple<frLayerNum, frLayerNum>;
  using MasterLayerRange = std::map<frMaster*, LayerRange, frBlockObjectComp>;

  frDesign* getDesign() const { return design_; }
  frTechObject* getTech() const { return design_->getTech(); }
  bool isNDRInst(frInst& inst);
  bool hasTrackPattern(frTrackPattern* tp, const Rect& box) const;

  void getPrefTrackPatterns(std::vector<frTrackPattern*>& prefTrackPatterns);
  void applyPatternsFile(const char* file_path);

  void initUniqueInstance();
  void initPinAccess();

  void initMaster2PinLayerRange(MasterLayerRange& master2PinLayerRange);

  void computeUnique(const MasterLayerRange& master2PinLayerRange,
                     const std::vector<frTrackPattern*>& prefTrackPatterns);
  void checkFigsOnGrid(const frMPin* pin);

  frDesign* design_;
  const frCollection<odb::dbInst*>& target_insts_;
  Logger* logger_;

  // All the unique instances
  std::vector<frInst*> unique_;
  // Mapp all instances to their representative unique instance
  std::map<frInst*, frInst*, frBlockObjectComp> inst2unique_;
  // Maps all instances to the set of instances with the same unique inst
  std::map<frInst*, std::set<frInst*, frBlockObjectComp>*> inst2Class_;
  // Maps a unique instance to its pin access index
  std::map<frInst*, int, frBlockObjectComp> unique2paidx_;
  // Maps a unique instance to its index in unique_
  std::map<frInst*, int, frBlockObjectComp> unique2Idx_;
  // master orient track-offset to instances
  std::map<frMaster*,
           std::map<dbOrientType,
                    std::map<std::vector<frCoord>,
                             std::set<frInst*, frBlockObjectComp>>>,
           frBlockObjectComp>
      masterOT2Insts_;
};

}  // namespace fr
