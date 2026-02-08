// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <map>
#include <set>
#include <tuple>
#include <utility>
#include <vector>

#include "db/obj/frBlockObject.h"
#include "db/obj/frFig.h"
#include "db/obj/frShape.h"
#include "db/tech/frTechObject.h"
#include "dr/AbstractDRGraphics.h"
#include "frBaseTypes.h"
#include "frDesign.h"
#include "frRegionQuery.h"
#include "odb/geom.h"

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
                            utl::Logger* logger,
                            RouterConfiguration* router_cfg,
                            AbstractDRGraphics* graphics,
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
  void buildPin2epMap(
      const frNet* net,
      const NetRouteObjs& netRouteObjs,
      frOrderedIdMap<frBlockObject*,
                     std::set<std::pair<odb::Point, frLayerNum>>>& pin2epMap);
  void pin2epMap_helper(
      const frNet* net,
      const odb::Point& pt,
      frLayerNum lNum,
      frOrderedIdMap<frBlockObject*,
                     std::set<std::pair<odb::Point, frLayerNum>>>& pin2epMap);
  void buildNodeMap(
      const frNet* net,
      const NetRouteObjs& netRouteObjs,
      std::vector<frBlockObject*>& netPins,
      const frOrderedIdMap<frBlockObject*,
                           std::set<std::pair<odb::Point, frLayerNum>>>&
          pin2epMap,
      std::map<std::pair<odb::Point, frLayerNum>, std::set<int>>& nodeMap);
  void nodeMap_routeObjEnd(
      const frNet* net,
      const std::vector<frConnFig*>& netRouteObjs,
      std::map<std::pair<odb::Point, frLayerNum>, std::set<int>>& nodeMap);
  void nodeMap_routeObjSplit(
      const frNet* net,
      const std::vector<frConnFig*>& netRouteObjs,
      std::map<std::pair<odb::Point, frLayerNum>, std::set<int>>& nodeMap);
  void nodeMap_routeObjSplit_helper(
      const odb::Point& crossPt,
      frCoord trackCoord,
      frCoord splitCoord,
      frLayerNum lNum,
      const std::vector<
          std::map<frCoord, std::map<frCoord, std::pair<frCoord, int>>>>&
          mergeHelper,
      std::map<std::pair<odb::Point, frLayerNum>, std::set<int>>& nodeMap);
  void nodeMap_pin(
      const std::vector<frConnFig*>& netRouteObjs,
      std::vector<frBlockObject*>& netPins,
      const frOrderedIdMap<frBlockObject*,
                           std::set<std::pair<odb::Point, frLayerNum>>>&
          pin2epMap,
      std::map<std::pair<odb::Point, frLayerNum>, std::set<int>>& nodeMap);
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
  void addMarker(frNet* net, frLayerNum lNum, const odb::Rect& bbox);
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
      const std::map<std::pair<odb::Point, frLayerNum>, std::set<int>>& nodeMap,
      const NetRouteObjs& netRouteObjs,
      int nNetRouteObjs,
      int nNetObjs);
  void finish(
      frNet* net,
      NetRouteObjs& netRouteObjs,
      const std::vector<frBlockObject*>& netPins,
      const std::vector<char>& adjVisited,
      int gCnt,
      int nCnt,
      std::map<std::pair<odb::Point, frLayerNum>, std::set<int>>& nodeMap);

  frRegionQuery* getRegionQuery() const;
  frTechObject* getTech() const;
  frDesign* getDesign() const;
  drt::TritonRoute* router_;
  utl::Logger* logger_;
  RouterConfiguration* router_cfg_;
  AbstractDRGraphics* graphics_;
  bool save_updates_;
};

}  // namespace drt
