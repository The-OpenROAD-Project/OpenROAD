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

#include "dr/FlexDR_graphics.h"
#include "frDesign.h"

namespace odb {
class dbDatabase;
}
namespace ord {
class Logger;
}

namespace fr {

class FlexDRConnectivityChecker
{
 public:
  FlexDRConnectivityChecker(frDesign* design,
                            Logger* logger,
                            odb::dbDatabase* db,
                            FlexDRGraphics* graphics);
  void check(int iter = -1);

 private:
  using NetRoutObjs = vector<frConnFig*>;
  // layer -> track -> indices of NetRoutObjs
  using PathSegsByLayerAndTrack = vector<map<frCoord, vector<int>>>;
  // The track id matches the map iteration order above
  using PathSegsByLayerAndTrackId = vector<vector<vector<int>>>;
  struct Span
  {
    frCoord lo;
    frCoord hi;
    bool operator<(const Span& rhs) const
    {
      return std::tie(lo, hi) < std::tie(rhs.lo, rhs.hi);
    }
  };
  using SpansByLayerAndTrackId = vector<vector<vector<Span>>>;

  void initRouteObjs(const frNet* net, NetRoutObjs& netRouteObjs);
  void buildPin2epMap(const frNet* net,
                      const NetRoutObjs& netRouteObjs,
                      std::map<frBlockObject*,
                               std::set<std::pair<frPoint, frLayerNum>>,
                               frBlockObjectComp>& pin2epMap);
  void pin2epMap_helper(const frNet* net,
                        const frPoint& pt,
                        const frLayerNum lNum,
                        std::map<frBlockObject*,
                                 std::set<std::pair<frPoint, frLayerNum>>,
                                 frBlockObjectComp>& pin2epMap,
                        const bool isWire);
  void buildNodeMap(
      const frNet* net,
      const NetRoutObjs& netRouteObjs,
      std::vector<frBlockObject*>& netPins,
      const std::map<frBlockObject*,
                     std::set<std::pair<frPoint, frLayerNum>>,
                     frBlockObjectComp>& pin2epMap,
      std::map<std::pair<frPoint, frLayerNum>, std::set<int>>& nodeMap);
  void nodeMap_routeObjEnd(
      const frNet* net,
      const std::vector<frConnFig*>& netRouteObjs,
      std::map<std::pair<frPoint, frLayerNum>, std::set<int>>& nodeMap);
  void nodeMap_routeObjSplit(
      const frNet* net,
      const std::vector<frConnFig*>& netRouteObjs,
      std::map<std::pair<frPoint, frLayerNum>, std::set<int>>& nodeMap);
  void nodeMap_routeObjSplit_helper(
      const frPoint& crossPt,
      const frCoord trackCoord,
      const frCoord splitCoord,
      const frLayerNum lNum,
      const std::vector<
          std::map<frCoord, std::map<frCoord, std::pair<frCoord, int>>>>&
          mergeHelper,
      std::map<std::pair<frPoint, frLayerNum>, std::set<int>>& nodeMap);
  void nodeMap_pin(
      const std::vector<frConnFig*>& netRouteObjs,
      std::vector<frBlockObject*>& netPins,
      const std::map<frBlockObject*,
                     std::set<std::pair<frPoint, frLayerNum>>,
                     frBlockObjectComp>& pin2epMap,
      std::map<std::pair<frPoint, frLayerNum>, std::set<int>>& nodeMap);
  void organizePathSegsByLayerAndTrack(const frNet* net,
                                       const NetRoutObjs& netRouteObjs,
                                       PathSegsByLayerAndTrack& horzPathSegs,
                                       PathSegsByLayerAndTrack& vertPathSegs);
  void findSegmentOverlaps(const NetRoutObjs& netRouteObjs,
                           const PathSegsByLayerAndTrack& horzPathSegs,
                           const PathSegsByLayerAndTrack& vertPathSegs,
                           PathSegsByLayerAndTrackId& horzVictims,
                           PathSegsByLayerAndTrackId& vertVictims,
                           SpansByLayerAndTrackId& horzNewSegSpans,
                           SpansByLayerAndTrackId& vertNewSegSpans);
  void mergeSegmentOverlaps(frNet* net,
                            NetRoutObjs& netRouteObjs,
                            const PathSegsByLayerAndTrack& horzPathSegs,
                            const PathSegsByLayerAndTrack& vertPathSegs,
                            const PathSegsByLayerAndTrackId& horzVictims,
                            const PathSegsByLayerAndTrackId& vertVictims,
                            const SpansByLayerAndTrackId& horzNewSegSpans,
                            const SpansByLayerAndTrackId& vertNewSegSpans);
  void addMarker(frNet* net, frLayerNum lNum, const frBox& bbox);
  void merge_perform(const NetRoutObjs& netRouteObjs,
                     const std::vector<int>& indices,
                     std::vector<int>& victims,
                     std::vector<Span>& newSegSpans,
                     const bool isHorz);
  void merge_perform_helper(const std::vector<std::pair<Span, int>>& segSpans,
                            std::vector<int>& victims,
                            std::vector<Span>& newSegSpans);
  void merge_commit(frNet* net,
                    std::vector<frConnFig*>& netRouteObjs,
                    const std::vector<int>& victims,
                    const frCoord trackCoord,
                    const std::vector<Span>& newSegSpans,
                    const bool isHorz);
  bool astar(
      const frNet* net,
      std::vector<bool>& adjVisited,
      std::vector<int>& adjPrevIdx,
      const std::map<std::pair<frPoint, frLayerNum>, std::set<int>>& nodeMap,
      const NetRoutObjs& netRouteObjs,
      const int& gCnt,
      const int& nCnt);
  void finish(frNet* net,
              NetRoutObjs& netRouteObjs,
              const std::vector<frBlockObject*>& netPins,
              const std::vector<bool>& adjVisited,
              const int gCnt,
              const int nCnt,
              std::map<std::pair<frPoint, frLayerNum>, std::set<int>>& nodeMap);

  frRegionQuery* getRegionQuery() const { return design_->getRegionQuery(); }
  frTechObject* getTech() const { return design_->getTech(); }

  frDesign* design_;
  Logger* logger_;
  odb::dbDatabase* db_;
  FlexDRGraphics* graphics_;
};

}  // namespace fr
