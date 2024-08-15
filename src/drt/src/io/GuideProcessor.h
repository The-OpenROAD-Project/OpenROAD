/////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024, Precision Innovations Inc.
// All rights reserved.
//
// BSD 3-Clause License
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
///////////////////////////////////////////////////////////////////////////////
#pragma once
#include <boost/icl/interval_set.hpp>

#include "db/tech/frTechObject.h"
#include "frDesign.h"
#include "odb/db.h"
#include "utl/Logger.h"

namespace drt::io {
class GuideProcessor
{
 public:
  GuideProcessor(frDesign* designIn,
                 odb::dbDatabase* dbIn,
                 utl::Logger* loggerIn)
      : design_(designIn), logger_(loggerIn), db_(dbIn){};
  bool readGuides();
  void buildGCellPatterns();
  void processGuides();

 private:
  // getters
  frTechObject* getTech() { return design_->getTech(); }
  frDesign* getDesign() { return design_; }
  // processGuides helper functions
  void buildGCellPatterns_getWidth(frCoord& GCELLGRIDX, frCoord& GCELLGRIDY);
  void buildGCellPatterns_getOffset(frCoord GCELLGRIDX,
                                    frCoord GCELLGRIDY,
                                    frCoord& GCELLOFFSETX,
                                    frCoord& GCELLOFFSETY);
  void buildGCellPatterns_helper(frCoord& GCELLGRIDX,
                                 frCoord& GCELLGRIDY,
                                 frCoord& GCELLOFFSETX,
                                 frCoord& GCELLOFFSETY);

  void genGuides(frNet* net, std::vector<frRect>& rects);
  void genGuides_addCoverGuide(frNet* net, std::vector<frRect>& rects);
  template <typename T>
  void genGuides_addCoverGuide_helper(frBlockObject* term,
                                      T* trueTerm,
                                      frInst* inst,
                                      dbTransform& shiftXform,
                                      std::vector<frRect>& rects);
  /**
   * @brief Creates/Extends guides to cover a pin shape at best_pin_loc_coords
   * through its closest guide.
   *
   * This is a helper function to patchGuides() function. it extends/bloats the
   * guide at closest guide index in order to connect to best_pin_loc_coords.
   * @param net the current net whose guides we are processing
   * @param guides list of gr guides of the net
   * @param best_pin_loc_idx The index of the gcell holding the major part of
   * the best/chosen pin shape. Its z coordinate is the layerNum of the chosen
   * pin shape
   * @param closest_guide_idx The index of the guide that is closest to the
   * best_pin_loc_coords in the guides list
   */
  void patchGuides_extendGuidesToCoverPin(frNet* net,
                                          std::vector<frRect>& guides,
                                          const Point3D& best_pin_loc_idx,
                                          const Point3D& best_pin_loc_coords,
                                          const int closest_guide_idx);
  /**
   * @brief Creates/Extends guides to cover pin.
   *
   * This function checks extends the exisiting guides in the guides list to
   * cover/overlap a pin shape in the pin. It can also create new guides and
   * append them to the guides list.
   * @param net the current net whose guides we are processing
   * @param pin a pin in the net which we are attempting to connect to the
   * guides
   * @param guides list of gr guides of the net
   */
  void patchGuides(frNet* net, frBlockObject* pin, std::vector<frRect>& guides);
  void genGuides_pinEnclosure(frNet* net, std::vector<frRect>& guides);
  void checkPinForGuideEnclosure(frBlockObject* pin,
                                 frNet* net,
                                 std::vector<frRect>& guides);
  void genGuides_merge(
      std::vector<frRect>& rects,
      std::vector<std::map<frCoord, boost::icl::interval_set<frCoord>>>& intvs);
  void genGuides_split(
      std::vector<frRect>& rects,
      std::vector<std::map<frCoord, boost::icl::interval_set<frCoord>>>& intvs,
      std::map<std::pair<Point, frLayerNum>,
               std::set<frBlockObject*, frBlockObjectComp>>& gCell2PinMap,
      std::map<frBlockObject*,
               std::set<std::pair<Point, frLayerNum>>,
               frBlockObjectComp>& pin2GCellMap,
      bool isRetry);
  void genGuides_gCell2PinMap(
      frNet* net,
      std::map<std::pair<Point, frLayerNum>,
               std::set<frBlockObject*, frBlockObjectComp>>& gCell2PinMap);
  template <typename T>
  void genGuides_gCell2TermMap(
      std::map<std::pair<Point, frLayerNum>,
               std::set<frBlockObject*, frBlockObjectComp>>& gCell2PinMap,
      T* term,
      frBlockObject* origTerm,
      const dbTransform& xform);
  bool genGuides_gCell2APInstTermMap(
      std::map<std::pair<Point, frLayerNum>,
               std::set<frBlockObject*, frBlockObjectComp>>& gCell2PinMap,
      frInstTerm* instTerm);
  bool genGuides_gCell2APTermMap(
      std::map<std::pair<Point, frLayerNum>,
               std::set<frBlockObject*, frBlockObjectComp>>& gCell2PinMap,
      frBTerm* term);
  void genGuides_initPin2GCellMap(
      frNet* net,
      std::map<frBlockObject*,
               std::set<std::pair<Point, frLayerNum>>,
               frBlockObjectComp>& pin2GCellMap);
  void genGuides_buildNodeMap(
      std::map<std::pair<Point, frLayerNum>, std::set<int>>& nodeMap,
      int& gCnt,
      int& nCnt,
      std::vector<frRect>& rects,
      std::map<frBlockObject*,
               std::set<std::pair<Point, frLayerNum>>,
               frBlockObjectComp>& pin2GCellMap);
  bool genGuides_astar(
      frNet* net,
      std::vector<bool>& adjVisited,
      std::vector<int>& adjPrevIdx,
      std::map<std::pair<Point, frLayerNum>, std::set<int>>& nodeMap,
      int& gCnt,
      int& nCnt,
      bool forceFeedThrough,
      bool retry);
  void genGuides_final(frNet* net,
                       std::vector<frRect>& rects,
                       std::vector<bool>& adjVisited,
                       std::vector<int>& adjPrevIdx,
                       int gCnt,
                       int nCnt,
                       std::map<frBlockObject*,
                                std::set<std::pair<Point, frLayerNum>>,
                                frBlockObjectComp>& pin2GCellMap);
  // write guide
  void saveGuidesUpdates();

  frDesign* design_;
  Logger* logger_;
  odb::dbDatabase* db_;
  std::map<frNet*, std::vector<frRect>, frBlockObjectComp> tmpGuides_;
  std::vector<std::pair<frBlockObject*, Point>> tmpGRPins_;
};
}  // namespace drt::io