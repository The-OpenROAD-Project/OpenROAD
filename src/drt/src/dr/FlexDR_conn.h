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
namespace utl {
class Logger;
}

namespace drt {

class TritonRoute;

class FlexDRConnectivityChecker
{
 public:
  FlexDRConnectivityChecker(drt::TritonRoute* router,
                            Logger* logger,
                            RouterConfiguration* router_cfg,
                            FlexDRGraphics* graphics,
                            bool save_updates = false);
  void check(int iter = -1);

 private:
  using NetRouteObjs = std::vector<frConnFig*>;
  // layer -> track -> indices of NetRouteObjs
  using PathSegsByLayerAndTrack
      = std::vector<std::map<frCoord, std::vector<int>>>;
  // The track id matches the map iteration order above
  using PathSegsByLayerAndTrackId = std::vector<std::vector<std::vector<int>>>;
  struct Span
  {
    frCoord lo;
    frCoord hi;
    bool operator<(const Span& rhs) const
    {
      return std::tie(lo, hi) < std::tie(rhs.lo, rhs.hi);
    }
  };
  using SpansByLayerAndTrackId = std::vector<std::vector<std::vector<Span>>>;

  /**
   * Fills the netRouteObjs with the net's shapes(segments and vias)
   *
   * The function filters the list of net's shapes from patches and adds only
   * the segments and vias to the list netRouteObjs.
   */
  void initRouteObjs(const frNet* net, NetRouteObjs& netRouteObjs);
  void buildPin2epMap(const frNet* net,
                      const NetRouteObjs& netRouteObjs,
                      std::map<frBlockObject*,
                               std::set<std::pair<Point, frLayerNum>>,
                               frBlockObjectComp>& pin2epMap);
  void pin2epMap_helper(const frNet* net,
                        const Point& pt,
                        frLayerNum lNum,
                        std::map<frBlockObject*,
                                 std::set<std::pair<Point, frLayerNum>>,
                                 frBlockObjectComp>& pin2epMap);
  void buildNodeMap(
      const frNet* net,
      const NetRouteObjs& netRouteObjs,
      std::vector<frBlockObject*>& netPins,
      const std::map<frBlockObject*,
                     std::set<std::pair<Point, frLayerNum>>,
                     frBlockObjectComp>& pin2epMap,
      std::map<std::pair<Point, frLayerNum>, std::set<int>>& nodeMap);
  void nodeMap_routeObjEnd(
      const frNet* net,
      const std::vector<frConnFig*>& netRouteObjs,
      std::map<std::pair<Point, frLayerNum>, std::set<int>>& nodeMap);
  void nodeMap_routeObjSplit(
      const frNet* net,
      const std::vector<frConnFig*>& netRouteObjs,
      std::map<std::pair<Point, frLayerNum>, std::set<int>>& nodeMap);
  void nodeMap_routeObjSplit_helper(
      const Point& crossPt,
      frCoord trackCoord,
      frCoord splitCoord,
      frLayerNum lNum,
      const std::vector<
          std::map<frCoord, std::map<frCoord, std::pair<frCoord, int>>>>&
          mergeHelper,
      std::map<std::pair<Point, frLayerNum>, std::set<int>>& nodeMap);
  void nodeMap_pin(
      const std::vector<frConnFig*>& netRouteObjs,
      std::vector<frBlockObject*>& netPins,
      const std::map<frBlockObject*,
                     std::set<std::pair<Point, frLayerNum>>,
                     frBlockObjectComp>& pin2epMap,
      std::map<std::pair<Point, frLayerNum>, std::set<int>>& nodeMap);
  /**
   * Maps the net's segments to track indices.
   *
   * The function iterates over all the net's segments from the netRouteObjs
   * list and fills the horzPathSegs and vertPathSegs list. The lists are
   * indexed by the layerNum. Each entry in the list is a track-indexed map that
   * represents. The map key is a track coordinate on the layer and a list of
   * segment indices that lie on that track. P.S. The segment indices refer to
   * their location in the netRouteObjs list.
   */
  void organizePathSegsByLayerAndTrack(const frNet* net,
                                       const NetRouteObjs& netRouteObjs,
                                       PathSegsByLayerAndTrack& horzPathSegs,
                                       PathSegsByLayerAndTrack& vertPathSegs);
  void findSegmentOverlaps(NetRouteObjs& netRouteObjs,
                           const PathSegsByLayerAndTrack& horzPathSegs,
                           const PathSegsByLayerAndTrack& vertPathSegs,
                           PathSegsByLayerAndTrackId& horzVictims,
                           PathSegsByLayerAndTrackId& vertVictims,
                           SpansByLayerAndTrackId& horzNewSegSpans,
                           SpansByLayerAndTrackId& vertNewSegSpans);
  void handleSegmentOverlaps(frNet* net,
                             NetRouteObjs& netRouteObjs,
                             const PathSegsByLayerAndTrack& horzPathSegs,
                             const PathSegsByLayerAndTrack& vertPathSegs,
                             const PathSegsByLayerAndTrackId& horzVictims,
                             const PathSegsByLayerAndTrackId& vertVictims,
                             const SpansByLayerAndTrackId& horzNewSegSpans,
                             const SpansByLayerAndTrackId& vertNewSegSpans);
  void addMarker(frNet* net, frLayerNum lNum, const Rect& bbox);
  void handleOverlaps_perform(NetRouteObjs& netRouteObjs,
                              const std::vector<int>& indices,
                              std::vector<int>& victims,
                              std::vector<Span>& newSegSpans,
                              bool isHorz);
  void splitPathSegs(NetRouteObjs& netRouteObjs,
                     std::vector<std::pair<Span, int>>& segSpans);
  void splitPathSegs_commit(std::vector<int>& splitPoints,
                            frPathSeg* highestPs,
                            int first,
                            int& i,
                            std::vector<std::pair<Span, int>>& segSpans,
                            NetRouteObjs& netRouteObjs);
  void merge_perform_helper(NetRouteObjs& netRouteObjs,
                            const std::vector<std::pair<Span, int>>& segSpans,
                            std::vector<int>& victims,
                            std::vector<Span>& newSegSpans);
  void merge_commit(frNet* net,
                    std::vector<frConnFig*>& netRouteObjs,
                    const std::vector<int>& victims,
                    frCoord trackCoord,
                    const std::vector<Span>& newSegSpans,
                    bool isHorz);
  bool astar(
      const frNet* net,
      std::vector<char>& adjVisited,
      std::vector<int>& adjPrevIdx,
      const std::map<std::pair<Point, frLayerNum>, std::set<int>>& nodeMap,
      const NetRouteObjs& netRouteObjs,
      int nNetRouteObjs,
      int nNetObjs);
  void finish(frNet* net,
              NetRouteObjs& netRouteObjs,
              const std::vector<frBlockObject*>& netPins,
              const std::vector<char>& adjVisited,
              int gCnt,
              int nCnt,
              std::map<std::pair<Point, frLayerNum>, std::set<int>>& nodeMap);

  frRegionQuery* getRegionQuery() const;
  frTechObject* getTech() const;
  frDesign* getDesign() const;
  drt::TritonRoute* router_;
  Logger* logger_;
  RouterConfiguration* router_cfg_;
  FlexDRGraphics* graphics_;
  bool save_updates_;
};

}  // namespace drt
