/* Authors: Lutong Wang and Bangqi Xu */
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

#include <algorithm>

#include "dr/FlexDR.h"
#include "frRTree.h"

namespace drt {

namespace bgi = boost::geometry::index;

bool FlexDRWorker::isRoutePatchWire(const frPatchWire* pwire) const
{
  const auto& gridBBox = getRouteBox();
  Point origin = pwire->getOrigin();
  return isInitDR() ? gridBBox.overlaps(origin) : gridBBox.intersects(origin);
}

bool FlexDRWorker::isRouteVia(const frVia* via) const
{
  const auto& gridBBox = getRouteBox();
  Point origin = via->getOrigin();
  return isInitDR() ? gridBBox.overlaps(origin) : gridBBox.intersects(origin);
}

void FlexDRWorker::initNetObjs_pathSeg(
    frPathSeg* pathSeg,
    std::set<frNet*, frBlockObjectComp>& nets,
    std::map<frNet*,
             std::vector<std::unique_ptr<drConnFig>>,
             frBlockObjectComp>& netRouteObjs,
    std::map<frNet*,
             std::vector<std::unique_ptr<drConnFig>>,
             frBlockObjectComp>& netExtObjs)
{
  const auto [begin, end] = pathSeg->getPoints();
  auto net = pathSeg->getNet();
  if (begin.x() != end.x() && begin.y() != end.y()) {
    const double dbu = getTech()->getDBUPerUU();
    logger_->error(DRT,
                   1010,
                   "Unsupported non-orthogonal wire begin=({}, {}) end=({}, "
                   "{}), layer {} on net {}",
                   begin.x() / dbu,
                   begin.y() / dbu,
                   end.x() / dbu,
                   end.y() / dbu,
                   getTech()->getLayer(pathSeg->getLayerNum())->getName(),
                   net->getName());
  }

  nets.insert(net);

  const auto along = begin.x() == end.x() ? odb::vertical : odb::horizontal;
  const auto ortho = along.turn_90();

  const auto gridBBox = getRouteBox();
  if (begin.get(ortho) < gridBBox.low(ortho)
      || gridBBox.high(ortho) < begin.get(ortho)) {
    // does not cross the routeBBox
    netExtObjs[net].push_back(std::make_unique<drPathSeg>(*pathSeg));
    return;
  }

  const int begin_coord = begin.get(along);
  const int end_coord = end.get(along);
  const int box_min = gridBBox.low(along);
  const int box_max = gridBBox.high(along);

  // split seg below box_min
  if (begin_coord < box_min) {
    auto uPathSeg = std::make_unique<drPathSeg>(*pathSeg);
    Point new_end(end);
    new_end.set(along, std::min(end_coord, box_min));
    uPathSeg->setPoints(begin, new_end);
    if (end_coord < box_min) {
      netExtObjs[net].push_back(std::move(uPathSeg));  // pure ext
    } else {
      // change boundary style to ext if pathSeg does not end exactly at
      // boundary
      frSegStyle style = pathSeg->getStyle();
      if (end_coord != box_min) {
        style.setEndStyle(frEndStyle(frcExtendEndStyle), style.getEndExt());
      }
      uPathSeg->setStyle(style);

      netRouteObjs[net].push_back(std::move(uPathSeg));
    }
  }

  // split seg in box_min to box_max
  if (begin_coord < box_max && end_coord > box_min) {
    auto uPathSeg = std::make_unique<drPathSeg>(*pathSeg);
    Point new_begin(begin);
    new_begin.set(along, std::max(begin_coord, box_min));
    Point new_end(end);
    new_end.set(along, std::min(end_coord, box_max));
    uPathSeg->setPoints(new_begin, new_end);
    // change boundary style to ext if it does not end within at boundary
    frSegStyle style = pathSeg->getStyle();
    if (begin_coord < box_min) {
      style.setBeginStyle(frEndStyle(frcExtendEndStyle), style.getBeginExt());
    }
    if (end_coord > box_max) {
      style.setEndStyle(frEndStyle(frcExtendEndStyle), style.getEndExt());
    }
    uPathSeg->setStyle(style);

    netRouteObjs[net].push_back(std::move(uPathSeg));
  }

  // split seg above box_max
  if (end_coord > box_max) {
    auto uPathSeg = std::make_unique<drPathSeg>(*pathSeg);
    Point new_begin(begin);
    new_begin.set(along, std::max(begin_coord, box_max));
    uPathSeg->setPoints(new_begin, end);
    if (begin_coord > box_max) {
      netExtObjs[net].push_back(std::move(uPathSeg));  // pure ext
    } else {
      // change boundary style to ext if pathSeg does not end exactly at
      // boundary
      frSegStyle style = pathSeg->getStyle();
      if (begin_coord != box_max) {
        style.setBeginStyle(frEndStyle(frcExtendEndStyle), style.getBeginExt());
      }
      uPathSeg->setStyle(style);

      netRouteObjs[net].push_back(std::move(uPathSeg));
    }
  }
}

void FlexDRWorker::initNetObjs_via(
    const frVia* via,
    std::set<frNet*, frBlockObjectComp>& nets,
    std::map<frNet*,
             std::vector<std::unique_ptr<drConnFig>>,
             frBlockObjectComp>& netRouteObjs,
    std::map<frNet*,
             std::vector<std::unique_ptr<drConnFig>>,
             frBlockObjectComp>& netExtObjs)
{
  auto net = via->getNet();
  nets.insert(net);
  if (isRouteVia(via)) {
    auto uVia = std::make_unique<drVia>(*via);
    std::unique_ptr<drConnFig> uDRObj(std::move(uVia));
    netRouteObjs[net].push_back(std::move(uDRObj));
  } else {
    netExtObjs[net].push_back(std::make_unique<drVia>(*via));
  }
}

void FlexDRWorker::initNetObjs_patchWire(
    frPatchWire* pwire,
    std::set<frNet*, frBlockObjectComp>& nets,
    std::map<frNet*,
             std::vector<std::unique_ptr<drConnFig>>,
             frBlockObjectComp>& netRouteObjs,
    std::map<frNet*,
             std::vector<std::unique_ptr<drConnFig>>,
             frBlockObjectComp>& netExtObjs)
{
  auto net = pwire->getNet();
  nets.insert(net);
  if (isRoutePatchWire(pwire)) {
    auto uPWire = std::make_unique<drPatchWire>(*pwire);
    std::unique_ptr<drConnFig> uDRObj(std::move(uPWire));
    netRouteObjs[net].push_back(std::move(uDRObj));
  } else {
    netExtObjs[net].push_back(std::make_unique<drPatchWire>(*pwire));
  }
}

// inits nets based on the routing shapes in the extRouteBox or based on the
// guides, if initDR(). inits netRouteObjs with routing shapes touching routeBox
// (shapes that touch only right/top borders of routeBox are not considered, IF
// initDR()). inits netExtObjs with routing shapes not touching routeBox, using
// the same criterion above
void FlexDRWorker::initNetObjs(
    const frDesign* design,
    std::set<frNet*, frBlockObjectComp>& nets,
    std::map<frNet*,
             std::vector<std::unique_ptr<drConnFig>>,
             frBlockObjectComp>& netRouteObjs,
    std::map<frNet*,
             std::vector<std::unique_ptr<drConnFig>>,
             frBlockObjectComp>& netExtObjs,
    std::map<frNet*, std::vector<frRect>, frBlockObjectComp>& netOrigGuides,
    std::map<frNet*, std::vector<frRect>, frBlockObjectComp>& netGuides)
{
  std::vector<frBlockObject*> result;
  design->getRegionQuery()->queryDRObj(getExtBox(), result);
  for (auto rptr : result) {
    if (rptr->typeId() == frcPathSeg) {
      auto cptr = static_cast<frPathSeg*>(rptr);
      if (cptr->hasNet()) {
        initNetObjs_pathSeg(cptr, nets, netRouteObjs, netExtObjs);
      } else {
        std::cout << "Error: initNetObjs hasNet() empty" << std::endl;
      }
    } else if (rptr->typeId() == frcVia) {
      auto cptr = static_cast<frVia*>(rptr);
      if (cptr->hasNet()) {
        initNetObjs_via(cptr, nets, netRouteObjs, netExtObjs);
      } else {
        std::cout << "Error: initNetObjs hasNet() empty" << std::endl;
      }
    } else if (rptr->typeId() == frcPatchWire) {
      auto cptr = static_cast<frPatchWire*>(rptr);
      if (cptr->hasNet()) {
        initNetObjs_patchWire(cptr, nets, netRouteObjs, netExtObjs);
      } else {
        std::cout << "Error: initNetObjs hasNet() empty" << std::endl;
      }
    } else {
      std::cout << rptr->typeId() << "\n";
      std::cout << "Error: initCopyDRObjs unsupported type" << std::endl;
    }
  }
  if (isInitDR()) {
    std::vector<frGuide*> guides;
    design->getRegionQuery()->queryGuide(getRouteBox(), guides);
    for (auto& guide : guides) {
      if (auto net = guide->getNet()) {
        if (getRipupMode() == RipUpMode::INCR && net->hasInitialRouting()) {
          continue;
        }
        if (nets.find(net) == nets.end()) {
          nets.insert(net);
          netRouteObjs[net].clear();
          netExtObjs[net].clear();
        }
      }
    }
  }

  if (isFollowGuide()) {
    frRegionQuery::Objects<frNet> origGuides;
    for (auto lNum = getTech()->getBottomLayerNum();
         lNum <= getTech()->getTopLayerNum();
         lNum++) {
      origGuides.clear();
      design->getRegionQuery()->queryOrigGuide(getRouteBox(), lNum, origGuides);
      for (auto& [box, net] : origGuides) {
        if (nets.find(net) == nets.end()) {
          continue;
        }
        frRect rect;
        rect.setBBox(box);
        rect.setLayerNum(lNum);
        netOrigGuides[net].push_back(rect);
      }
    }
    std::vector<frGuide*> guides;
    design->getRegionQuery()->queryGuide(getRouteBox(), guides);
    for (auto& guide : guides) {
      if (nets.find(guide->getNet()) == nets.end()) {
        continue;
      }
      auto [bp, ep] = guide->getPoints();
      Point bpIdx = design_->getTopBlock()->getGCellIdx(bp);
      Point epIdx = design_->getTopBlock()->getGCellIdx(ep);
      Rect bbox = design_->getTopBlock()->getGCellBox(bpIdx);
      Rect ebox = design_->getTopBlock()->getGCellBox(epIdx);
      frLayerNum bNum = guide->getBeginLayerNum();
      frLayerNum eNum = guide->getEndLayerNum();
      frRect rect;
      rect.setBBox({bbox.xMin(), bbox.yMin(), ebox.xMax(), ebox.yMax()});
      for (auto lNum = std::min(bNum, eNum); lNum <= std::max(bNum, eNum);
           lNum += 2) {
        rect.setLayerNum(lNum);
        netGuides[guide->getNet()].push_back(rect);
      }
    }
  }
}

static bool segOnBorder(const Rect& routeBox,
                        const Point& begin,
                        const Point& end)
{
  if (begin.x() == end.x()) {
    return begin.x() == routeBox.xMin() || begin.x() == routeBox.xMax();
  }
  return begin.y() == routeBox.yMin() || begin.y() == routeBox.yMax();
}

void FlexDRWorker::initNets_segmentTerms(
    const Point& bp,
    const frLayerNum lNum,
    const frNet* net,
    std::set<frBlockObject*, frBlockObjectComp>& terms)
{
  auto regionQuery = design_->getRegionQuery();
  frRegionQuery::Objects<frBlockObject> result;
  regionQuery->query({bp, bp}, lNum, result);
  for (auto& [bx, rqObj] : result) {
    switch (rqObj->typeId()) {
      case frcInstTerm: {
        auto instTerm = static_cast<frInstTerm*>(rqObj);
        if (instTerm->getNet() == net) {
          terms.insert(rqObj);
        }
        break;
      }
      case frcBTerm: {
        auto term = static_cast<frBTerm*>(rqObj);
        if (term->getNet() == net) {
          terms.insert(rqObj);
        }
        break;
      }
      default:
        break;
    }
  }
}

namespace {
void dfs(const int start,
         const std::map<int, std::vector<int>>& adj_list,
         std::vector<bool>& visited,
         std::vector<int>& component)
{
  std::stack<int> stack;
  stack.push(start);
  while (!stack.empty()) {
    const int node = stack.top();
    stack.pop();
    if (!visited[node]) {
      visited[node] = true;
      component.push_back(node);
      auto it = adj_list.find(node);
      if (it == adj_list.end()) {
        continue;
      }
      for (int neighbor : (*it).second) {
        if (!visited[neighbor]) {
          stack.push(neighbor);
        }
      }
    }
  }
}

frSquaredDistance getSqrdDist(const Rect& rect1, const Rect& rect2)
{
  frSquaredDistance dist = gtl::square_euclidean_distance(
      gtl::rectangle_data<frCoord>(
          rect1.xMin(), rect1.yMin(), rect1.xMax(), rect1.yMax()),
      gtl::rectangle_data<frCoord>(
          rect2.xMin(), rect2.yMin(), rect2.xMax(), rect2.yMax()));
  return dist;
}
}  // namespace
int FlexDRWorker::initNets_initDR_helper_getObjComponent(
    drConnFig* obj,
    const std::vector<std::vector<int>>& connectedComponents,
    const std::vector<frRect>& netGuides)
{
  switch (obj->typeId()) {
    case drcPathSeg: {
      auto pathSeg = static_cast<drPathSeg*>(obj);
      const auto lNum = pathSeg->getLayerNum();
      const auto rect = pathSeg->getBBox();
      frSquaredDistance minDist = std::numeric_limits<frSquaredDistance>::max();
      int minIndex = -1;
      for (int j = 0; j < connectedComponents.size(); j++) {
        // component index is j
        for (const int idx : connectedComponents.at(j)) {
          if (netGuides.at(idx).getLayerNum() == lNum
              && netGuides.at(idx).intersects(rect)) {
            return j;
          }
          frSquaredDistance dist
              = getSqrdDist(netGuides.at(idx).getBBox(), rect);
          if (dist < minDist) {
            minDist = dist;
            minIndex = j;
          }
        }
      }
      return minIndex;
    }
    case drcVia: {
      auto via = static_cast<drVia*>(obj);
      const auto rect = via->getBBox();
      frSquaredDistance minDist = std::numeric_limits<frSquaredDistance>::max();
      int minIndex = -1;
      for (int j = 0; j < connectedComponents.size(); j++) {
        // component index is j
        for (const int idx : connectedComponents.at(j)) {
          frSquaredDistance dist
              = getSqrdDist(netGuides.at(idx).getBBox(), rect);
          if (dist < minDist) {
            if (dist == 0) {
              return j;
            }
            minDist = dist;
            minIndex = j;
          }
        }
      }
      return minIndex;
    }
    case drcPatchWire: {
      auto wire = static_cast<drPatchWire*>(obj);
      const auto lNum = wire->getLayerNum();
      const auto rect = wire->getBBox();
      frSquaredDistance minDist = std::numeric_limits<frSquaredDistance>::max();
      int minIndex = -1;
      for (int j = 0; j < connectedComponents.size(); j++) {
        // component index is j
        for (const int idx : connectedComponents.at(j)) {
          if (netGuides.at(idx).getLayerNum() == lNum
              && netGuides.at(idx).intersects(rect)) {
            return j;
          }
          frSquaredDistance dist
              = getSqrdDist(netGuides.at(idx).getBBox(), rect);
          if (dist < minDist) {
            minDist = dist;
            minIndex = j;
          }
        }
      }
      return minIndex;
    }
    default:
      return -1;
  }
}

void FlexDRWorker::initNets_initDR_helper(
    frNet* net,
    std::vector<std::unique_ptr<drConnFig>>& netRouteObjs,
    std::vector<std::unique_ptr<drConnFig>>& netExtObjs,
    const std::vector<frBlockObject*>& netTerms,
    const std::vector<frRect>& netOrigGuides,
    const std::vector<frRect>& netGuides)
{
  struct Node
  {
    enum Type
    {
      TERM,
      GUIDE
    };
    Type type;
    int idx;
    Rect rect;
    Node(Type typeIn, int idxIn, Rect rectIn)
        : type(typeIn), idx(idxIn), rect(rectIn)
    {
    }
  };
  std::vector<Node> nodes;
  nodes.reserve(netGuides.size() + netTerms.size());
  for (int i = 0; i < netGuides.size(); i++) {
    nodes.emplace_back(Node::GUIDE, i, netGuides.at(i).getBBox());
  }
  for (int i = 0; i < netTerms.size(); i++) {
    Rect rect;
    if (netTerms.at(i)->typeId() == frcInstTerm) {
      auto iterm = static_cast<frInstTerm*>(netTerms.at(i));
      rect = iterm->getBBox(true);
    } else {
      auto bterm = static_cast<frBTerm*>(netTerms.at(i));
      rect = bterm->getBBox();
    }
    nodes.emplace_back(Node::TERM, i, rect);
  }
  std::map<int, std::vector<int>> nodeMap;
  for (int i = 0; i < nodes.size(); i++) {
    for (int j = i + 1; j < nodes.size(); j++) {
      if (nodes[i].rect.intersects(nodes[j].rect)) {
        nodeMap[i].emplace_back(j);
        nodeMap[j].emplace_back(i);
      }
    }
  }
  std::vector<bool> visited(nodes.size(), false);
  std::vector<std::vector<int>> connectedComponents;
  for (int i = 0; i < nodes.size(); i++) {
    if (visited[i]) {
      continue;
    }
    std::vector<int> component;
    dfs(i, nodeMap, visited, component);
    connectedComponents.emplace_back(std::move(component));
  }
  auto compIt = connectedComponents.begin();
  while (compIt != connectedComponents.end()) {
    bool foundGuide = false;
    for (auto nodeIdx : (*compIt)) {
      if (nodes[nodeIdx].type == Node::GUIDE) {
        foundGuide = true;
        break;
      }
    }
    if (!foundGuide) {
      compIt = connectedComponents.erase(compIt);
    } else {
      compIt++;
    }
  }
  if (connectedComponents.size() <= 1) {
    std::vector<std::pair<Point, frLayerNum>> bounds;
    auto it = boundaryPin_.find(net);
    if (it != boundaryPin_.end()) {
      std::transform(it->second.begin(),
                     it->second.end(),
                     inserter(bounds, bounds.end()),
                     [](const std::pair<Point, frLayerNum>& pr) { return pr; });
    }
    initNet(design_,
            net,
            netRouteObjs,
            netExtObjs,
            netOrigGuides,
            netTerms,
            bounds);
    return;
  }
  std::vector<std::vector<std::unique_ptr<drConnFig>>> routeObjs(
      connectedComponents.size());
  std::vector<std::vector<std::unique_ptr<drConnFig>>> extObjs(
      connectedComponents.size());
  std::vector<std::vector<frBlockObject*>> terms(connectedComponents.size());
  std::vector<std::vector<std::pair<Point, frLayerNum>>> bounds(
      connectedComponents.size());
  for (int i = 0; i < netTerms.size(); i++) {
    Rect rect;
    if (netTerms.at(i)->typeId() == frcInstTerm) {
      auto iterm = static_cast<frInstTerm*>(netTerms.at(i));
      rect = iterm->getBBox(true);
    } else {
      auto bterm = static_cast<frBTerm*>(netTerms.at(i));
      rect = bterm->getBBox();
    }
    frSquaredDistance minDist = std::numeric_limits<frSquaredDistance>::max();
    int bestIndex = -1;
    for (int j = 0; j < connectedComponents.size(); j++) {
      // component index is j
      for (auto idx : connectedComponents[j]) {
        frSquaredDistance dist = getSqrdDist(nodes[idx].rect, rect);
        if (dist < minDist) {
          minDist = dist;
          bestIndex = j;
        }
        if (nodes[idx].type == Node::TERM && nodes[idx].idx == i) {
          bestIndex = j;
          break;
        }
      }
    }
    terms[bestIndex].emplace_back(netTerms.at(i));
  }
  const auto it = boundaryPin_.find(net);
  if (it != boundaryPin_.end()) {
    for (auto [point, lNum] : (*it).second) {
      frSquaredDistance minDist = std::numeric_limits<frSquaredDistance>::max();
      int bestIndex = -1;
      for (int j = 0; j < connectedComponents.size(); j++) {
        for (auto idx : connectedComponents[j]) {
          if (nodes[idx].type == Node::TERM) {
            continue;
          }
          frSquaredDistance dist = getSqrdDist(nodes[idx].rect, {point, point});
          dist += std::abs(netGuides.at(nodes[idx].idx).getLayerNum() - lNum);
          if (dist < minDist) {
            minDist = dist;
            bestIndex = j;
          }
        }
      }
      bounds[bestIndex].emplace_back(point, lNum);
    }
  }
  // Remove pins from graph for routeObjs/extObjs net resolution
  for (auto& component : connectedComponents) {
    auto itr = component.begin();
    while (itr != component.end()) {
      if (nodes[*itr].type == Node::TERM) {
        itr = component.erase(itr);
      } else {
        *itr = nodes[*itr].idx;
        itr++;
      }
    }
  }
  for (auto& obj : netRouteObjs) {
    auto compIdx = initNets_initDR_helper_getObjComponent(
        obj.get(), connectedComponents, netGuides);
    routeObjs[compIdx].emplace_back(std::move(obj));
  }
  for (auto& obj : netExtObjs) {
    auto compIdx = initNets_initDR_helper_getObjComponent(
        obj.get(), connectedComponents, netGuides);
    extObjs[compIdx].emplace_back(std::move(obj));
  }
  for (int i = 0; i < connectedComponents.size(); i++) {
    initNet(design_,
            net,
            routeObjs[i],
            extObjs[i],
            netOrigGuides,
            terms[i],
            bounds[i]);
  }
}

// inits nets based on the pins
void FlexDRWorker::initNets_initDR(
    const frDesign* design,
    std::set<frNet*, frBlockObjectComp>& nets,
    std::map<frNet*,
             std::vector<std::unique_ptr<drConnFig>>,
             frBlockObjectComp>& netRouteObjs,
    std::map<frNet*,
             std::vector<std::unique_ptr<drConnFig>>,
             frBlockObjectComp>& netExtObjs,
    std::map<frNet*, std::vector<frRect>, frBlockObjectComp>& netOrigGuides,
    std::map<frNet*, std::vector<frRect>, frBlockObjectComp>& netGuides)
{
  std::map<frNet*,
           std::set<frBlockObject*, frBlockObjectComp>,
           frBlockObjectComp>
      netTerms;
  std::vector<frBlockObject*> result;
  design->getRegionQuery()->queryGRPin(getRouteBox(), result);
  for (auto obj : result) {
    frNet* net;
    switch (obj->typeId()) {
      case frcInstTerm: {
        net = static_cast<frInstTerm*>(obj)->getNet();
        break;
      }
      case frcBTerm: {
        net = static_cast<frBTerm*>(obj)->getNet();
        break;
      }
      default:
        logger_->error(utl::DRT, 0, "initNetTerms unsupported obj.");
    }
    if (getRipupMode() == RipUpMode::INCR && net->hasInitialRouting()) {
      continue;
    }
    nets.insert(net);
    netTerms[net].insert(obj);
  }
  std::vector<std::unique_ptr<drConnFig>> vRouteObjs;
  std::vector<std::unique_ptr<drConnFig>> vExtObjs;
  for (auto net : nets) {
    if (getRipupMode() == RipUpMode::INCR && net->hasInitialRouting()) {
      continue;
    }
    vRouteObjs.clear();
    vExtObjs.clear();
    vExtObjs = std::move(netExtObjs[net]);
    for (int i = 0; i < (int) netRouteObjs[net].size(); i++) {
      const auto& obj = netRouteObjs[net][i];
      if (obj->typeId() == drcPathSeg) {
        const auto ps = static_cast<drPathSeg*>(obj.get());
        const frSegStyle style = ps->getStyle();
        const auto [bp, ep] = ps->getPoints();
        const auto& box = getRouteBox();
        const bool onBorder = segOnBorder(box, bp, ep);
        if (box.intersects(bp) && box.intersects(ep)
            && !(onBorder
                 && (style.getBeginStyle() != frcTruncateEndStyle
                     || style.getEndStyle() != frcTruncateEndStyle))) {
          if (onBorder) {
            initNets_segmentTerms(bp, ps->getLayerNum(), net, netTerms[net]);
            initNets_segmentTerms(ep, ps->getLayerNum(), net, netTerms[net]);
          }
          vRouteObjs.push_back(std::move(netRouteObjs[net][i]));
        } else {
          vExtObjs.push_back(std::move(netRouteObjs[net][i]));
        }
      } else if (obj->typeId() == drcVia) {
        vRouteObjs.push_back(std::move(netRouteObjs[net][i]));
      } else if (obj->typeId() == drcPatchWire) {
        vRouteObjs.push_back(std::move(netRouteObjs[net][i]));
      }
    }
    std::vector<frBlockObject*> tmpTerms;
    tmpTerms.assign(netTerms[net].begin(), netTerms[net].end());
    initNets_initDR_helper(net,
                           vRouteObjs,
                           vExtObjs,
                           tmpTerms,
                           netOrigGuides[net],
                           netGuides[net]);
  }
}

// copied to FlexDR::checkConnectivity_pin2epMap_helper
void FlexDRWorker::initNets_searchRepair_pin2epMap_helper(
    const frDesign* design,
    const frNet* net,
    const Point& bp,
    const frLayerNum lNum,
    std::map<frBlockObject*,
             std::set<std::pair<Point, frLayerNum>>,
             frBlockObjectComp>& pin2epMap)
{
  frRegionQuery::Objects<frBlockObject> result;
  design->getRegionQuery()->query({bp, bp}, lNum, result);
  for (auto& [bx, rqObj] : result) {
    switch (rqObj->typeId()) {
      case frcInstTerm: {
        auto instTerm = static_cast<frInstTerm*>(rqObj);
        if (instTerm->getNet() == net) {
          pin2epMap[rqObj].insert(std::make_pair(bp, lNum));
        }
        break;
      }
      case frcBTerm: {
        auto term = static_cast<frBTerm*>(rqObj);
        if (term->getNet() == net) {
          pin2epMap[rqObj].insert(std::make_pair(bp, lNum));
        }
        break;
      }
      default:
        break;
    }
  }
}
// maps the currently used access points to their pins
void FlexDRWorker::initNets_searchRepair_pin2epMap(
    const frDesign* design,
    const frNet* net,
    const std::vector<std::unique_ptr<drConnFig>>& netRouteObjs,
    std::map<frBlockObject*,
             std::set<std::pair<Point, frLayerNum>>,
             frBlockObjectComp>& pin2epMap)
{
  // should not count extObjs in union find
  for (auto& uPtr : netRouteObjs) {
    auto connFig = uPtr.get();
    if (connFig->typeId() == drcPathSeg) {
      const auto obj = static_cast<drPathSeg*>(connFig);
      const auto [bp, ep] = obj->getPoints();
      const auto lNum = obj->getLayerNum();
      const frSegStyle style = obj->getStyle();
      if (getRouteBox().intersects(bp)
          && style.getBeginStyle() == frEndStyle(frcTruncateEndStyle)) {
        initNets_searchRepair_pin2epMap_helper(
            design, net, bp, lNum, pin2epMap);
      }
      if (getRouteBox().intersects(ep)
          && style.getEndStyle() == frEndStyle(frcTruncateEndStyle)) {
        initNets_searchRepair_pin2epMap_helper(
            design, net, ep, lNum, pin2epMap);
      }
    } else if (connFig->typeId() == drcVia) {
      const auto obj = static_cast<drVia*>(connFig);
      const Point bp = obj->getOrigin();
      const auto l1Num = obj->getViaDef()->getLayer1Num();
      const auto l2Num = obj->getViaDef()->getLayer2Num();
      if (getRouteBox().intersects(bp)) {
        if (obj->isBottomConnected()) {
          initNets_searchRepair_pin2epMap_helper(
              design, net, bp, l1Num, pin2epMap);
        }
        if (obj->isTopConnected()) {
          initNets_searchRepair_pin2epMap_helper(
              design, net, bp, l2Num, pin2epMap);
        }
      }
    } else if (connFig->typeId() == drcPatchWire) {
    } else {
      std::cout << "Error: initNets_searchRepair_pin2epMap unsupported type"
                << std::endl;
    }
  }
}

// maps begin/end points of shapes to their shapes
void FlexDRWorker::initNets_searchRepair_nodeMap_routeObjEnd(
    const std::vector<std::unique_ptr<drConnFig>>& netRouteObjs,
    std::map<std::pair<Point, frLayerNum>, std::set<int>>& nodeMap)
{
  for (int i = 0; i < (int) netRouteObjs.size(); i++) {
    auto connFig = netRouteObjs[i].get();
    if (connFig->typeId() == drcPathSeg) {
      auto obj = static_cast<drPathSeg*>(connFig);
      const auto [bp, ep] = obj->getPoints();
      const auto lNum = obj->getLayerNum();
      nodeMap[{bp, lNum}].insert(i);
      nodeMap[{ep, lNum}].insert(i);
    } else if (connFig->typeId() == drcVia) {
      auto obj = static_cast<drVia*>(connFig);
      auto bp = obj->getOrigin();
      const auto l1Num = obj->getViaDef()->getLayer1Num();
      const auto l2Num = obj->getViaDef()->getLayer2Num();
      nodeMap[{bp, l1Num}].insert(i);
      nodeMap[{bp, l2Num}].insert(i);
    } else if (connFig->typeId() == drcPatchWire) {
      auto obj = static_cast<drPatchWire*>(connFig);
      const Point bp = obj->getOrigin();
      const auto lNum = obj->getLayerNum();
      nodeMap[{bp, lNum}].insert(i);
    } else {
      std::cout << "Error: initNets_searchRepair_nodeMap unsupported type"
                << std::endl;
    }
  }
}

void FlexDRWorker::initNets_searchRepair_nodeMap_routeObjSplit_helper(
    const Point& crossPt,
    const frCoord trackCoord,
    const frCoord splitCoord,
    const frLayerNum lNum,
    std::vector<std::map<frCoord, std::map<frCoord, std::pair<frCoord, int>>>>&
        mergeHelper,
    std::map<std::pair<Point, frLayerNum>, std::set<int>>& nodeMap)
{
  auto it1 = mergeHelper[lNum].find(trackCoord);
  if (it1 == mergeHelper[lNum].end()) {
    return;
  }

  auto& mp = it1->second;  // map<ep, pair<bp, objIdx> >
  auto it2 = mp.lower_bound(splitCoord);
  if (it2 == mp.end()) {
    return;
  }

  const auto& endP = it2->first;
  const auto& [beginP, objIdx] = it2->second;
  if (endP > splitCoord && beginP < splitCoord) {
    nodeMap[{crossPt, lNum}].insert(objIdx);
  }
}

// creates mapping of middle points of shapes, that are intersecting other
// shapes, to their shapes
void FlexDRWorker::initNets_searchRepair_nodeMap_routeObjSplit(
    const std::vector<std::unique_ptr<drConnFig>>& netRouteObjs,
    std::map<std::pair<Point, frLayerNum>, std::set<int>>& nodeMap)
{
  const int num_layers = getTech()->getLayers().size();
  std::vector<std::map<frCoord, std::map<frCoord, std::pair<frCoord, int>>>>
      horzMergeHelper(num_layers);
  std::vector<std::map<frCoord, std::map<frCoord, std::pair<frCoord, int>>>>
      vertMergeHelper(num_layers);
  for (int i = 0; i < (int) netRouteObjs.size(); i++) {
    auto connFig = netRouteObjs[i].get();
    if (connFig->typeId() == drcPathSeg) {
      auto obj = static_cast<drPathSeg*>(connFig);
      const auto [bp, ep] = obj->getPoints();
      const auto lNum = obj->getLayerNum();
      if (bp.x() == ep.x()) {  // vert seg
        vertMergeHelper[lNum][bp.x()][ep.y()] = {bp.y(), i};
      } else {  // horz seg
        horzMergeHelper[lNum][bp.y()][ep.x()] = {bp.x(), i};
      }
    }
  }
  for (const auto& connFig : netRouteObjs) {
    // ep on pathseg
    if (connFig->typeId() == drcPathSeg) {
      auto obj = static_cast<const drPathSeg*>(connFig.get());
      const auto [bp, ep] = obj->getPoints();
      const auto lNum = obj->getLayerNum();
      if (bp.x() == ep.x()) {  // vert seg, find horz crossing seg
        // find whether there is horz track at bp
        initNets_searchRepair_nodeMap_routeObjSplit_helper(
            bp, bp.y(), bp.x(), lNum, horzMergeHelper, nodeMap);
        // find whether there is horz track at ep
        initNets_searchRepair_nodeMap_routeObjSplit_helper(
            ep, ep.y(), ep.x(), lNum, horzMergeHelper, nodeMap);
      } else {  // horz seg
        // find whether there is vert track at bp
        initNets_searchRepair_nodeMap_routeObjSplit_helper(
            bp, bp.x(), bp.y(), lNum, vertMergeHelper, nodeMap);
        // find whether there is vert track at ep
        initNets_searchRepair_nodeMap_routeObjSplit_helper(
            ep, ep.x(), ep.y(), lNum, vertMergeHelper, nodeMap);
      }
    } else if (connFig->typeId() == drcVia) {
      auto obj = static_cast<const drVia*>(connFig.get());
      const Point bp = obj->getOrigin();
      const auto lNum1 = obj->getViaDef()->getLayer1Num();
      // find whether there is horz track at bp on layer1
      initNets_searchRepair_nodeMap_routeObjSplit_helper(
          bp, bp.y(), bp.x(), lNum1, horzMergeHelper, nodeMap);
      // find whether there is vert track at bp on layer1
      initNets_searchRepair_nodeMap_routeObjSplit_helper(
          bp, bp.x(), bp.y(), lNum1, vertMergeHelper, nodeMap);

      const auto lNum2 = obj->getViaDef()->getLayer2Num();
      // find whether there is horz track at bp on layer2
      initNets_searchRepair_nodeMap_routeObjSplit_helper(
          bp, bp.y(), bp.x(), lNum2, horzMergeHelper, nodeMap);
      // find whether there is vert track at bp on layer2
      initNets_searchRepair_nodeMap_routeObjSplit_helper(
          bp, bp.x(), bp.y(), lNum2, vertMergeHelper, nodeMap);
    }
  }
}
// creates entries in the node_map with pin access points and their pins
void FlexDRWorker::initNets_searchRepair_nodeMap_pin(
    const std::vector<std::unique_ptr<drConnFig>>& netRouteObjs,
    std::vector<frBlockObject*>& netPins,
    const std::map<frBlockObject*,
                   std::set<std::pair<Point, frLayerNum>>,
                   frBlockObjectComp>& pin2epMap,
    std::map<std::pair<Point, frLayerNum>, std::set<int>>& nodeMap)
{
  int currCnt = (int) netRouteObjs.size();
  netPins.reserve(pin2epMap.size());
  for (auto& [obj, locS] : pin2epMap) {
    netPins.push_back(obj);
    for (auto& pr : locS) {
      nodeMap[pr].insert(currCnt);
    }
    ++currCnt;
  }
}

// maps points (in the wire tips, via centers, wire intersections) to their
// shapes (routeObjs)
void FlexDRWorker::initNets_searchRepair_nodeMap(
    const std::vector<std::unique_ptr<drConnFig>>& netRouteObjs,
    std::vector<frBlockObject*>& netPins,
    const std::map<frBlockObject*,
                   std::set<std::pair<Point, frLayerNum>>,
                   frBlockObjectComp>& pin2epMap,
    std::map<std::pair<Point, frLayerNum>, std::set<int>>& nodeMap)
{
  initNets_searchRepair_nodeMap_routeObjEnd(netRouteObjs, nodeMap);
  initNets_searchRepair_nodeMap_routeObjSplit(netRouteObjs, nodeMap);
  initNets_searchRepair_nodeMap_pin(netRouteObjs, netPins, pin2epMap, nodeMap);
}

// maps routeObjs to sub-nets
void FlexDRWorker::initNets_searchRepair_connComp(
    frNet* net,
    std::map<std::pair<Point, frLayerNum>, std::set<int>>& nodeMap,
    std::vector<int>& compIdx)
{
  const int nCnt = (int) compIdx.size();  // total node cnt
  std::vector<std::vector<int>> adjVec(nCnt, std::vector<int>());
  std::vector<bool> adjVisited(nCnt, false);
  for (const auto& [pr, idxS] : nodeMap) {
    for (auto it1 = idxS.begin(); it1 != idxS.end(); it1++) {
      auto it2 = it1;
      it2++;
      const int idx1 = *it1;
      for (; it2 != idxS.end(); it2++) {
        const int idx2 = *it2;
        adjVec[idx1].push_back(idx2);
        adjVec[idx2].push_back(idx1);
      }
    }
  }

  struct wf
  {
    int nodeIdx;
    int cost;
    bool operator<(const wf& b) const
    {
      if (cost == b.cost) {
        return nodeIdx > b.nodeIdx;
      }
      return cost > b.cost;
    }
  };

  int currNetIdx = 0;
  auto it = find(adjVisited.begin(), adjVisited.end(), false);
  while (it != adjVisited.end()) {
    std::priority_queue<wf> pq;
    int srcIdx = distance(adjVisited.begin(), it);
    pq.push({srcIdx, 0});
    while (!pq.empty()) {
      const auto wfront = pq.top();
      const auto currIdx = wfront.nodeIdx;
      pq.pop();
      if (adjVisited[currIdx]) {
        continue;
      }
      adjVisited[currIdx] = true;
      compIdx[currIdx] = currNetIdx;
      for (auto nbrIdx : adjVec[currIdx]) {
        if (!adjVisited[nbrIdx]) {
          pq.push({nbrIdx, wfront.cost + 1});
        }
      }
    }
    it = find(adjVisited.begin(), adjVisited.end(), false);
    ++currNetIdx;
  }
}

void FlexDRWorker::initNets_searchRepair(
    const frDesign* design,
    const std::set<frNet*, frBlockObjectComp>& nets,
    std::map<frNet*,
             std::vector<std::unique_ptr<drConnFig>>,
             frBlockObjectComp>& netRouteObjs,
    std::map<frNet*,
             std::vector<std::unique_ptr<drConnFig>>,
             frBlockObjectComp>& netExtObjs,
    std::map<frNet*, std::vector<frRect>, frBlockObjectComp>& netOrigGuides)
{
  for (auto net : nets) {
    if (isInitDR() && getRipupMode() == RipUpMode::INCR
        && !net->hasInitialRouting()) {
      continue;
    }
    // build big graph;
    // node number : routeObj, pins
    std::map<frBlockObject*,
             std::set<std::pair<Point, frLayerNum>>,
             frBlockObjectComp>
        pin2epMap;
    initNets_searchRepair_pin2epMap(design, net, netRouteObjs[net], pin2epMap);

    std::vector<frBlockObject*> netPins;
    std::map<std::pair<Point, frLayerNum>, std::set<int>> nodeMap;
    initNets_searchRepair_nodeMap(
        netRouteObjs[net], netPins, pin2epMap, nodeMap);

    std::vector<int> compIdx(
        (int) netPins.size() + (int) netRouteObjs[net].size(), 0);

    initNets_searchRepair_connComp(net, nodeMap, compIdx);

    std::vector<std::vector<std::unique_ptr<drConnFig>>> vExtObjs;

    const auto it = max_element(compIdx.begin(), compIdx.end());
    const int numSubNets = (it == compIdx.end()) ? 1 : ((*it) + 1);
    // put all pure ext objs to the first subnet
    vExtObjs.resize(numSubNets);
    vExtObjs[0] = std::move(netExtObjs[net]);

    std::vector<std::vector<std::unique_ptr<drConnFig>>> vRouteObjs;
    std::vector<std::vector<frBlockObject*>> vPins;

    vRouteObjs.resize(numSubNets);
    vPins.resize(numSubNets);
    // does a logical assignment of routing shapes to sub-nets
    for (int i = 0; i < (int) compIdx.size(); i++) {
      const int subNetIdx = compIdx[i];
      if (i < (int) netRouteObjs[net].size()) {
        const auto& obj = netRouteObjs[net][i];
        if (obj->typeId() == drcPathSeg) {
          auto ps = static_cast<drPathSeg*>(obj.get());
          auto [bp, ep] = ps->getPoints();
          auto& box = getRouteBox();
          if (box.intersects(bp) && box.intersects(ep)) {
            vRouteObjs[subNetIdx].push_back(std::move(netRouteObjs[net][i]));
          } else {
            vExtObjs[subNetIdx].push_back(std::move(netRouteObjs[net][i]));
          }
        } else if (obj->typeId() == drcVia) {
          vRouteObjs[subNetIdx].push_back(std::move(netRouteObjs[net][i]));
        } else if (obj->typeId() == drcPatchWire) {
          vRouteObjs[subNetIdx].push_back(std::move(netRouteObjs[net][i]));
        }
      } else {
        vPins[subNetIdx].push_back(netPins[i - (int) netRouteObjs[net].size()]);
      }
    }

    for (int i = 0; i < numSubNets; i++) {
      initNet(design,
              net,
              vRouteObjs[i],
              vExtObjs[i],
              netOrigGuides[net],
              vPins[i]);
    }
  }
}

bool FlexDRWorker::findAPTracks(const frLayerNum startLayerNum,
                                const frLayerNum endLayerNum,
                                const Rectangle& pinRect,
                                std::set<frCoord>& xLocs,
                                std::set<frCoord>& yLocs)
{
  const int inc = (startLayerNum < endLayerNum ? 2 : -2);

  for (frLayerNum lNum = startLayerNum; lNum != endLayerNum + inc;
       lNum += inc) {
    const dbTechLayerDir currPrefRouteDir = getTech()->getLayer(lNum)->getDir();
    if (currPrefRouteDir == dbTechLayerDir::HORIZONTAL) {
      getTrackLocs(true, lNum, yl(pinRect), yh(pinRect), yLocs);
    } else {
      getTrackLocs(false, lNum, xl(pinRect), xh(pinRect), xLocs);
    }
    // track found or is normal layer (so can jog)
    if (!xLocs.empty() || !yLocs.empty() || !isRestrictedRouting(lNum)) {
      return true;
    }
  }
  return false;
}

bool FlexDRWorker::isRestrictedRouting(const frLayerNum lNum)
{
  return getTech()->getLayer(lNum)->isUnidirectional()
         || lNum < BOTTOM_ROUTING_LAYER || lNum > TOP_ROUTING_LAYER;
}

void FlexDRWorker::initNet_termGenAp(const frDesign* design, drPin* dPin)
{
  // FIXME  using namespace boost::polygon::operators;
  using bPoint = boost::polygon::point_data<int>;

  const auto routeBox = getRouteBox();
  Rectangle routeRect(
      routeBox.xMin(), routeBox.yMin(), routeBox.xMax(), routeBox.yMax());
  bPoint routeRectCenter;
  center(routeRectCenter, routeRect);
  auto dPinTerm = dPin->getFrTerm();
  if (dPinTerm->typeId() == frcInstTerm) {
    auto instTerm = static_cast<frInstTerm*>(dPinTerm);
    auto inst = instTerm->getInst();
    const dbTransform xform = inst->getUpdatedXform();

    for (auto& uPin : instTerm->getTerm()->getPins()) {
      auto pin = uPin.get();
      for (auto& uPinFig : pin->getFigs()) {
        auto pinFig = uPinFig.get();
        // horizontal tracks == yLocs
        std::set<frCoord> xLocs, yLocs;
        if (pinFig->typeId() == frcRect) {
          auto rpinRect = static_cast<frRect*>(pinFig);
          const frLayerNum currLayerNum = rpinRect->getLayerNum();
          if (getTech()->getLayer(currLayerNum)->getType()
              != dbTechLayerType::ROUTING) {
            continue;
          }
          frRect instPinRect(*rpinRect);
          instPinRect.move(xform);
          const Rect instPinRectBBox = instPinRect.getBBox();
          Rectangle pinRect(instPinRectBBox.xMin(),
                            instPinRectBBox.yMin(),
                            instPinRectBBox.xMax(),
                            instPinRectBBox.yMax());
          if (!boost::polygon::intersect(pinRect, routeRect)) {
            continue;
          }
          // pinRect now equals intersection of pinRect and routeRect
          const auto currPrefRouteDir
              = getTech()->getLayer(currLayerNum)->getDir();
          // get intersecting tracks if any
          if (currPrefRouteDir == dbTechLayerDir::HORIZONTAL) {
            getTrackLocs(true, currLayerNum, yl(pinRect), yh(pinRect), yLocs);
            if (currLayerNum + 2 <= getTech()->getTopLayerNum()) {
              getTrackLocs(
                  false, currLayerNum + 2, xl(pinRect), xh(pinRect), xLocs);
            } else if (currLayerNum - 2 >= getTech()->getBottomLayerNum()) {
              getTrackLocs(
                  false, currLayerNum - 2, xl(pinRect), xh(pinRect), xLocs);
            }
          } else {
            getTrackLocs(false, currLayerNum, xl(pinRect), xh(pinRect), xLocs);
            if (currLayerNum + 2 <= getTech()->getTopLayerNum()) {
              getTrackLocs(
                  true, currLayerNum + 2, yl(pinRect), yh(pinRect), yLocs);
            } else if (currLayerNum - 2 >= getTech()->getBottomLayerNum()) {
              getTrackLocs(
                  true, currLayerNum - 2, yl(pinRect), yh(pinRect), yLocs);
            }
          }
          // gen new temp on-track access point if any
          if (!xLocs.empty() && !yLocs.empty()) {
            // TODO: update access pattern information as needed
            auto uap = std::make_unique<drAccessPattern>();
            const auto xLoc = *(xLocs.begin());
            const auto yLoc = *(yLocs.begin());
            const Point pt(xLoc, yLoc);
            uap->setBeginLayerNum(currLayerNum);
            uap->setPoint(pt);
            // prevent unchecked planar access
            std::vector<bool> validAccess(6, true);
            validAccess[5] = false;
            if (currLayerNum == VIA_ACCESS_LAYERNUM) {
              validAccess[0] = false;
              validAccess[1] = false;
              validAccess[2] = false;
              validAccess[3] = false;
            }
            uap->setValidAccess(validAccess);
            uap->setOnTrack(false, true);
            uap->setOnTrack(false, false);
            uap->setPin(dPin);
            // any non-zero value works
            uap->setPinCost(7);
            // to resolve temp AP end seg minArea patch problem
            // frCoord reqArea = 0;
            auto minAreaConstraint
                = getTech()->getLayer(currLayerNum)->getAreaConstraint();
            if (minAreaConstraint) {
              auto reqArea = minAreaConstraint->getMinArea();
              uap->setBeginArea(reqArea);
            }

            if (!isInitDR()
                || (xLoc != xh(routeRect) && yLoc != yh(routeRect))) {
              dPin->addAccessPattern(std::move(uap));
              break;
            }
          }
        } else {
          std::cout << "Error: initNet_termGenAp unsupported pinFig\n";
        }
      }

      // no on-track temp ap found
      for (auto& uPinFig : pin->getFigs()) {
        auto pinFig = uPinFig.get();
        // horizontal tracks == yLocs
        std::set<frCoord> xLocs, yLocs;
        if (pinFig->typeId() == frcRect) {
          auto rpinRect = static_cast<frRect*>(pinFig);
          frLayerNum currLayerNum = rpinRect->getLayerNum();
          frLayer* layer = getTech()->getLayer(currLayerNum);
          if (layer->getType() != dbTechLayerType::ROUTING) {
            continue;
          }
          frRect instPinRect(*rpinRect);
          instPinRect.move(xform);
          Rect instPinRectBBox = instPinRect.getBBox();
          Rectangle pinRect(instPinRectBBox.xMin(),
                            instPinRectBBox.yMin(),
                            instPinRectBBox.xMax(),
                            instPinRectBBox.yMax());
          if (!boost::polygon::intersect(pinRect, routeRect)) {
            continue;
          }
          frCoord xLoc, yLoc;
          // pinRect now equals intersection of pinRect and routeRect
          auto currPrefRouteDir = layer->getDir();
          bool restrictedRouting = isRestrictedRouting(currLayerNum);
          if (currLayerNum + 2 <= getTech()->getTopLayerNum()) {
            restrictedRouting
                = restrictedRouting || isRestrictedRouting(currLayerNum + 2);
          }
          if (currLayerNum - 2 >= getTech()->getBottomLayerNum()
              && currLayerNum - 2 >= VIA_ACCESS_LAYERNUM) {
            restrictedRouting
                = restrictedRouting || isRestrictedRouting(currLayerNum - 2);
          }
          // get intersecting tracks if any
          if (restrictedRouting) {
            bool found = findAPTracks(
                currLayerNum + 2,
                std::min(TOP_ROUTING_LAYER, getTech()->getTopLayerNum()),
                pinRect,
                xLocs,
                yLocs);
            if (!found) {
              found = findAPTracks(currLayerNum - 2,
                                   std::max(BOTTOM_ROUTING_LAYER,
                                            getTech()->getBottomLayerNum()),
                                   pinRect,
                                   xLocs,
                                   yLocs);
            }
            if (!found) {
              continue;
            }
          } else if (currPrefRouteDir == dbTechLayerDir::HORIZONTAL) {
            getTrackLocs(true, currLayerNum, yl(pinRect), yh(pinRect), yLocs);
            if (currLayerNum + 2 <= getTech()->getTopLayerNum()) {
              getTrackLocs(
                  false, currLayerNum + 2, xl(pinRect), xh(pinRect), xLocs);
            } else if (currLayerNum - 2 >= getTech()->getBottomLayerNum()) {
              getTrackLocs(
                  false, currLayerNum - 2, xl(pinRect), xh(pinRect), xLocs);
            }
          } else {
            getTrackLocs(false, currLayerNum, xl(pinRect), xh(pinRect), xLocs);
            if (currLayerNum + 2 <= getTech()->getTopLayerNum()) {
              getTrackLocs(
                  true, currLayerNum + 2, yl(pinRect), yh(pinRect), yLocs);
            } else if (currLayerNum - 2 >= getTech()->getBottomLayerNum()) {
              getTrackLocs(
                  true, currLayerNum - 2, yl(pinRect), yh(pinRect), yLocs);
            }
          }
          // xLoc
          if (!xLocs.empty()) {
            xLoc = *(xLocs.begin());
          } else {
            xLoc = (xl(pinRect) + xh(pinRect)) / 2;
            xLoc = snapCoordToManufacturingGrid(xLoc, routeBox.ll().x());
          }
          // xLoc
          if (!yLocs.empty()) {
            yLoc = *(yLocs.begin());
          } else {
            yLoc = (yl(pinRect) + yh(pinRect)) / 2;
            yLoc = snapCoordToManufacturingGrid(yLoc, routeBox.ll().y());
          }
          if (restrictedRouting) {
            specialAccessAPs.emplace_back(xLoc, yLoc, currLayerNum);
          }
          // TODO: update as drAccessPattern updated
          auto uap = std::make_unique<drAccessPattern>();
          Point pt(xLoc, yLoc);
          uap->setBeginLayerNum(currLayerNum);
          uap->setPoint(pt);
          // prevent unchecked planar access
          std::vector<bool> validAccess(6, true);
          validAccess[5] = false;
          if (currLayerNum == VIA_ACCESS_LAYERNUM) {
            validAccess[0] = false;
            validAccess[1] = false;
            validAccess[2] = false;
            validAccess[3] = false;
          }
          uap->setValidAccess(validAccess);
          uap->setOnTrack(false, true);
          uap->setOnTrack(false, false);
          uap->setPin(dPin);
          // any non-zero value works
          uap->setPinCost(7);
          // to resolve temp AP end seg minArea patch problem
          // frCoord reqArea = 0;
          auto minAreaConstraint
              = getTech()->getLayer(currLayerNum)->getAreaConstraint();
          if (minAreaConstraint) {
            auto reqArea = minAreaConstraint->getMinArea();
            uap->setBeginArea(reqArea);
          }

          dPin->addAccessPattern(std::move(uap));
          if (!restrictedRouting) {
            break;
          }
        } else {
          std::cout << "Error: initNet_termGenAp unsupported pinFig\n";
        }
      }
    }
  } else if (dPinTerm->typeId() == frcBTerm) {
    auto term = static_cast<frBTerm*>(dPinTerm);
    for (auto& uPin : term->getPins()) {
      auto pin = uPin.get();
      bool hasTempAp = false;
      for (auto& uPinFig : pin->getFigs()) {
        auto pinFig = uPinFig.get();
        // horizontal tracks == yLocs
        std::set<frCoord> xLocs, yLocs;
        if (pinFig->typeId() == frcRect) {
          auto rpinRect = static_cast<frRect*>(pinFig);
          frLayerNum currLayerNum = rpinRect->getLayerNum();
          if (getTech()->getLayer(currLayerNum)->getType()
              != dbTechLayerType::ROUTING) {
            continue;
          }
          //          halfWidth =
          //          getTech()->getLayer(currLayerNum)->getMinWidth() / 2;
          frRect instPinRect(*rpinRect);
          // instPinRect.move(xform);
          Rect instPinRectBBox = instPinRect.getBBox();
          Rectangle pinRect(instPinRectBBox.xMin(),
                            instPinRectBBox.yMin(),
                            instPinRectBBox.xMax(),
                            instPinRectBBox.yMax());
          if (!boost::polygon::intersect(pinRect, routeRect)) {
            continue;
          }
          // pinRect now equals intersection of pinRect and routeRect
          auto xSpan = instPinRectBBox.dx();
          auto ySpan = instPinRectBBox.dy();
          bool isPinRectHorz = (xSpan > ySpan);

          int layerWidth = getTech()
                               ->getLayer(currLayerNum)
                               ->getWidth();  // for ISPD off track pins
          if (isPinRectHorz) {
            frCoord manuGrid = getTech()->getManufacturingGrid();
            auto centerY = (instPinRectBBox.yMax() + instPinRectBBox.yMin()) / 2
                           / manuGrid * manuGrid;
            if (centerY >= yl(routeRect) && centerY < yh(routeRect)) {
              yLocs.insert(centerY);
              if (currLayerNum + 2 <= getTech()->getTopLayerNum()) {
                getTrackLocs(
                    false, currLayerNum + 2, xl(pinRect), xh(pinRect), xLocs);
              } else if (currLayerNum - 2 >= getTech()->getBottomLayerNum()) {
                getTrackLocs(
                    false, currLayerNum - 2, xl(pinRect), xh(pinRect), xLocs);
              }
            } else {
              getTrackLocs(true, currLayerNum, yl(pinRect), yh(pinRect), yLocs);
              frCoord lowerBoundX = xl(pinRect);
              frCoord upperBoundX = xh(pinRect);
              if ((upperBoundX - lowerBoundX) >= 2 * layerWidth) {
                lowerBoundX += layerWidth;
                upperBoundX -= layerWidth;
              }
              if (currLayerNum + 2 <= getTech()->getTopLayerNum()) {
                getTrackLocs(
                    false, currLayerNum + 2, lowerBoundX, upperBoundX, xLocs);
              } else if (currLayerNum - 2 >= getTech()->getBottomLayerNum()) {
                getTrackLocs(
                    false, currLayerNum - 2, lowerBoundX, upperBoundX, xLocs);
              }
            }
          } else {
            frCoord manuGrid = getTech()->getManufacturingGrid();
            auto centerX = (instPinRectBBox.xMin() + instPinRectBBox.xMax()) / 2
                           / manuGrid * manuGrid;
            if (centerX >= xl(routeRect) && centerX < xh(routeRect)) {
              xLocs.insert(centerX);
              if (currLayerNum + 2 <= getTech()->getTopLayerNum()) {
                getTrackLocs(
                    true, currLayerNum + 2, yl(pinRect), yh(pinRect), yLocs);
              } else if (currLayerNum - 2 >= getTech()->getBottomLayerNum()) {
                getTrackLocs(
                    true, currLayerNum - 2, yl(pinRect), yh(pinRect), yLocs);
              }
            } else {
              getTrackLocs(
                  false, currLayerNum, xl(pinRect), xh(pinRect), xLocs);
              frCoord lowerBoundY = yl(pinRect);
              frCoord upperBoundY = yh(pinRect);
              if ((upperBoundY - lowerBoundY) >= 2 * layerWidth) {
                lowerBoundY += layerWidth;
                upperBoundY -= layerWidth;
                if (currLayerNum + 2 <= getTech()->getTopLayerNum()) {
                  getTrackLocs(
                      true, currLayerNum + 2, lowerBoundY, upperBoundY, yLocs);
                } else if (currLayerNum - 2 >= getTech()->getBottomLayerNum()) {
                  getTrackLocs(
                      true, currLayerNum - 2, lowerBoundY, upperBoundY, yLocs);
                }
              }
            }
          }

          // gen new temp on-track access point if any
          if (!xLocs.empty() && !yLocs.empty()) {
            // TODO: update access pattern information as needed
            for (auto xLoc : xLocs) {
              for (auto yLoc : yLocs) {
                auto uap = std::make_unique<drAccessPattern>();
                // auto xLoc =*(xLocs.begin());
                // auto yLoc =*(yLocs.begin());
                Point pt(xLoc, yLoc);
                uap->setBeginLayerNum(currLayerNum);
                uap->setPoint(pt);
                // to resolve temp AP end seg minArea patch problem
                // frCoord reqArea = 0;
                auto minAreaConstraint
                    = getTech()->getLayer(currLayerNum)->getAreaConstraint();
                if (minAreaConstraint) {
                  auto reqArea = minAreaConstraint->getMinArea();
                  uap->setBeginArea(reqArea);
                }
                // io pin pref direction setting
                // Point pinCenter;
                // center(pinCenter, pinRect);
                std::vector<bool> validAccess(6, false);
                if (isPinRectHorz) {
                  validAccess[0] = true;
                  validAccess[2] = true;
                } else {
                  validAccess[3] = true;
                  validAccess[1] = true;
                }
                uap->setValidAccess(validAccess);
                uap->setPin(dPin);
                if (!isInitDR() || xLoc != xh(routeRect)
                    || yLoc != yh(routeRect)) {
                  if (xLoc >= xl(routeRect) && xLoc < xh(routeRect)
                      && yLoc >= yl(routeRect) && yLoc < yh(routeRect)) {
                    hasTempAp = true;
                    dPin->addAccessPattern(std::move(uap));
                  }
                }
              }
            }

            if (hasTempAp) {
              break;
            }
          }
        } else {
          std::cout << "Error: initNet_termGenAp unsupported pinFig\n";
        }
      }

      // no on-track temp ap found
      if (!hasTempAp) {
        for (auto& uPinFig : pin->getFigs()) {
          auto pinFig = uPinFig.get();
          // horizontal tracks == yLocs
          std::set<frCoord> xLocs, yLocs;
          if (pinFig->typeId() == frcRect) {
            auto rpinRect = static_cast<frRect*>(pinFig);
            frLayerNum currLayerNum = rpinRect->getLayerNum();
            if (getTech()->getLayer(currLayerNum)->getType()
                != dbTechLayerType::ROUTING) {
              continue;
            }
            frRect instPinRect(*rpinRect);
            // instPinRect.move(xform);
            Rect instPinRectBBox = instPinRect.getBBox();
            Rectangle pinRect(instPinRectBBox.xMin(),
                              instPinRectBBox.yMin(),
                              instPinRectBBox.xMax(),
                              instPinRectBBox.yMax());
            if (!boost::polygon::intersect(pinRect, routeRect)) {
              continue;
            }

            frCoord xLoc, yLoc;
            auto instPinCenterX
                = (instPinRectBBox.xMin() + instPinRectBBox.xMax()) / 2;
            auto instPinCenterY
                = (instPinRectBBox.yMin() + instPinRectBBox.yMax()) / 2;
            auto pinCenterX = (xl(pinRect) + xh(pinRect)) / 2;
            auto pinCenterY = (yl(pinRect) + yh(pinRect)) / 2;
            if (instPinCenterX >= xl(routeRect)
                && instPinCenterX < xh(routeRect)) {
              xLoc = instPinCenterX;
              xLoc = snapCoordToManufacturingGrid(xLoc, routeBox.ll().x());
            } else {
              xLoc = pinCenterX;
              xLoc = snapCoordToManufacturingGrid(xLoc, routeBox.ll().x());
            }
            if (instPinCenterY >= yl(routeRect)
                && instPinCenterY < yh(routeRect)) {
              yLoc = instPinCenterY;
              yLoc = snapCoordToManufacturingGrid(yLoc, routeBox.ll().y());
            } else {
              yLoc = pinCenterY;
              yLoc = snapCoordToManufacturingGrid(yLoc, routeBox.ll().y());
            }

            if (!isInitDR() || xLoc != xh(routeRect) || yLoc != yh(routeRect)) {
              // TODO: update as drAccessPattern updated
              auto uap = std::make_unique<drAccessPattern>();
              Point pt(xLoc, yLoc);
              uap->setBeginLayerNum(currLayerNum);
              uap->setPoint(pt);
              uap->setPin(dPin);
              // to resolve temp AP end seg minArea patch problem
              // frCoord reqArea = 0;
              auto minAreaConstraint
                  = getTech()->getLayer(currLayerNum)->getAreaConstraint();
              if (minAreaConstraint) {
                auto reqArea = minAreaConstraint->getMinArea();
                uap->setBeginArea(reqArea);
              }

              dPin->addAccessPattern(std::move(uap));
              break;
            }
          } else {
            std::cout << "Error: initNet_termGenAp unsupported pinFig\n";
          }
        }
      }
    }
  } else {
    std::cout << "Error: initNet_termGenAp unexpected type\n";
  }
}

// when isHorzTracks == true, it means track loc == y loc
void FlexDRWorker::getTrackLocs(const bool isHorzTracks,
                                const frLayerNum currLayerNum,
                                const frCoord low,
                                const frCoord high,
                                std::set<frCoord>& trackLocs)
{
  // Consecutive metal layers always have opposite routing directions (H & V)
  for (auto& tp : design_->getTopBlock()->getTrackPatterns(currLayerNum)) {
    if (tp->isHorizontal() != isHorzTracks) {
      int trackNum = (low - tp->getStartCoord()) / (int) tp->getTrackSpacing();
      trackNum = std::max(trackNum, 0);
      if (trackNum * (int) tp->getTrackSpacing() + tp->getStartCoord() < low) {
        ++trackNum;
      }
      for (; trackNum < (int) tp->getNumTracks()
             && trackNum * (int) tp->getTrackSpacing() + tp->getStartCoord()
                    <= high;
           ++trackNum) {
        const frCoord trackLoc
            = trackNum * tp->getTrackSpacing() + tp->getStartCoord();
        trackLocs.insert(trackLoc);
      }
    }
  }
}

void FlexDRWorker::initNet_term(const frDesign* design,
                                drNet* dNet,
                                const std::vector<frBlockObject*>& terms)
{
  for (auto term : terms) {
    // ap
    // TODO is instXform used properly here?
    dbTransform instXform;  // (0,0), R0
    dbTransform shiftXform;
    switch (term->typeId()) {
      case frcInstTerm: {
        auto instTerm = static_cast<frInstTerm*>(term);
        frInst* inst = instTerm->getInst();
        shiftXform = inst->getTransform();
        shiftXform.setOrient(dbOrientType(dbOrientType::R0));
        instXform = inst->getUpdatedXform();
        auto trueTerm = instTerm->getTerm();
        const std::string name = inst->getName() + "/" + trueTerm->getName();
        initNet_term_helper(
            design, trueTerm, term, inst, dNet, name, shiftXform);
        break;
      }
      case frcBTerm: {
        auto trueTerm = static_cast<frBTerm*>(term);
        const std::string name = "PIN/" + trueTerm->getName();
        initNet_term_helper(
            design, trueTerm, term, nullptr, dNet, name, shiftXform);
        break;
      }
      default:
        logger_->error(
            utl::DRT, 1009, "initNet_term invoked with non-term object.");
    }
  }
}

template <typename T>
void FlexDRWorker::initNet_term_helper(const frDesign* design,
                                       T* trueTerm,
                                       frBlockObject* term,
                                       frInst* inst,
                                       drNet* dNet,
                                       const std::string& name,
                                       const dbTransform& shiftXform)
{
  auto dPin = std::make_unique<drPin>();
  dPin->setFrTerm(term);

  int pinIdx = 0;
  const int pinAccessIdx = (inst) ? inst->getPinAccessIdx() : 0;
  for (auto& pin : trueTerm->getPins()) {
    frAccessPoint* prefAp = nullptr;
    if (inst) {
      prefAp = (static_cast<frInstTerm*>(term)->getAccessPoints())[pinIdx];
    }
    if (!pin->hasPinAccess()) {
      continue;
    }
    for (auto& ap : pin->getPinAccess(pinAccessIdx)->getAccessPoints()) {
      Point bp = ap->getPoint();
      const auto bNum = ap->getLayerNum();
      shiftXform.apply(bp);

      auto dAp = std::make_unique<drAccessPattern>();
      dAp->setPoint(bp);
      dAp->setBeginLayerNum(bNum);
      if (ap.get() == prefAp) {
        dAp->setPinCost(0);
      } else {
        dAp->setPinCost(1);
      }
      // set min area
      if (ENABLE_BOUNDARY_MAR_FIX) {
        auto minAreaConstraint = getTech()->getLayer(bNum)->getAreaConstraint();
        if (minAreaConstraint) {
          auto reqArea = minAreaConstraint->getMinArea();
          dAp->setBeginArea(reqArea);
        }
      }
      dAp->setValidAccess(ap->getAccess());
      if (ap->hasAccess(frDirEnum::U)) {
        if (!(ap->getViaDefs().empty())) {
          dAp->setAccessViaDef(frDirEnum::U, &(ap->getViaDefs()));
        }
      }
      if (getRouteBox().intersects(bp)) {
        dPin->addAccessPattern(std::move(dAp));
      }
    }
    pinIdx++;
  }

  if (dPin->getAccessPatterns().empty()) {
    initNet_termGenAp(design, dPin.get());
    if (dPin->getAccessPatterns().empty()) {
      std::cout << "\nError: pin " << name << " still does not have temp ap"
                << std::endl;
      if (graphics_) {
        graphics_->debugWholeDesign();
      }
      exit(1);
    }
  }
  dPin->setId(pinCnt_++);
  dNet->addPin(std::move(dPin));
}

void FlexDRWorker::initNet_boundary(
    drNet* dNet,
    const std::vector<std::unique_ptr<drConnFig>>& extObjs,
    std::vector<std::pair<Point, frLayerNum>> bounds)
{
  const auto gridBBox = getRouteBox();
  // location to area
  std::map<std::pair<Point, frLayerNum>, frCoord> extBounds;
  frCoord currArea = 0;
  bool initFromExtObjs = isInitDR() ? (getRipupMode() == RipUpMode::INCR
                                       && dNet->getFrNet()->hasInitialRouting())
                                    : true;
  if (initFromExtObjs) {
    for (auto& obj : extObjs) {
      if (obj->typeId() == drcPathSeg) {
        auto ps = static_cast<drPathSeg*>(obj.get());
        const auto [begin, end] = ps->getPoints();
        const frLayerNum lNum = ps->getLayerNum();
        // vert pathseg
        if (begin.x() == end.x() && begin.x() >= gridBBox.xMin()
            && end.x() <= gridBBox.xMax()) {
          if (begin.y() == gridBBox.yMax() && end.y() > gridBBox.yMax()) {
            extBounds[std::make_pair(begin, lNum)] = currArea;
          }
          if (end.y() == gridBBox.yMin() && begin.y() < gridBBox.yMin()) {
            extBounds[std::make_pair(end, lNum)] = currArea;
          }
          // horz pathseg
        } else if (begin.y() == end.y() && begin.y() >= gridBBox.yMin()
                   && end.y() <= gridBBox.yMax()) {
          if (begin.x() == gridBBox.xMax() && end.x() > gridBBox.xMax()) {
            extBounds[std::make_pair(begin, lNum)] = currArea;
          }
          if (end.x() == gridBBox.xMin() && begin.x() < gridBBox.xMin()) {
            extBounds[std::make_pair(end, lNum)] = currArea;
          }
        }
      }
    }
    // initDR
  } else {
    transform(bounds.begin(),
              bounds.end(),
              inserter(extBounds, extBounds.end()),
              [](const std::pair<Point, frLayerNum>& pr) {
                return std::make_pair(pr, 0);
              });
  }
  for (auto& [pr, area] : extBounds) {
    auto& [pt, lNum] = pr;
    auto dPin = std::make_unique<drPin>();
    auto dAp = std::make_unique<drAccessPattern>();
    dAp->setPoint(pt);
    dAp->setBeginLayerNum(lNum);
    dPin->addAccessPattern(std::move(dAp));
    // ap
    dPin->setId(pinCnt_);
    pinCnt_++;
    dNet->addPin(std::move(dPin));
  }
}

void FlexDRWorker::initNet_addNet(std::unique_ptr<drNet> in)
{
  owner2nets_[in->getFrNet()].push_back(in.get());
  nets_.push_back(std::move(in));
}

void FlexDRWorker::initNet(const frDesign* design,
                           frNet* net,
                           std::vector<std::unique_ptr<drConnFig>>& routeObjs,
                           std::vector<std::unique_ptr<drConnFig>>& extObjs,
                           const std::vector<frRect>& origGuides,
                           const std::vector<frBlockObject*>& terms,
                           std::vector<std::pair<Point, frLayerNum>> bounds)
{
  auto dNet = std::make_unique<drNet>(net);
  // true pin
  initNet_term(design, dNet.get(), terms);
  // boundary pin, could overlap with any of true pins
  initNet_boundary(dNet.get(), extObjs, std::move(bounds));
  // no ext routes in initDR to avoid weird TA shapes
  for (auto& obj : extObjs) {
    dNet->addRoute(std::move(obj), true);
  }
  if (getRipupMode() != RipUpMode::ALL) {
    for (auto& obj : routeObjs) {
      dNet->addRoute(std::move(obj), false);
    }
  }
  dNet->setOrigGuides(origGuides);
  dNet->setId(nets_.size());
  initNet_addNet(std::move(dNet));
}

void FlexDRWorker::initNets_numPinsIn()
{
  std::vector<rq_box_value_t<drPin*>> allPins;
  for (auto& net : nets_) {
    for (auto& pin : net->getPins()) {
      bool hasPrefAP = false;
      drAccessPattern* firstAP = nullptr;
      for (auto& ap : pin->getAccessPatterns()) {
        if (firstAP == nullptr) {
          firstAP = ap.get();
        }
        if (ap->getPinCost() == 0) {
          const Point pt = ap->getPoint();
          allPins.emplace_back(Rect(pt, pt), pin.get());
          hasPrefAP = true;
          break;
        }
      }
      if (!hasPrefAP) {
        const Point pt = firstAP->getPoint();
        allPins.emplace_back(Rect(pt, pt), pin.get());
      }
    }
  }
  RTree<drPin*> pinRegionQuery(allPins);
  for (auto& net : nets_) {
    frCoord x1 = getExtBox().xMax();
    frCoord x2 = getExtBox().xMin();
    frCoord y1 = getExtBox().yMax();
    frCoord y2 = getExtBox().yMin();
    for (auto& pin : net->getPins()) {
      bool hasPrefAP = false;
      drAccessPattern* firstAP = nullptr;
      Point pt;
      for (auto& ap : pin->getAccessPatterns()) {
        if (firstAP == nullptr) {
          firstAP = ap.get();
        }
        if (ap->getPinCost() == 0) {
          pt = ap->getPoint();
          hasPrefAP = true;
          break;
        }
      }
      if (!hasPrefAP) {
        pt = firstAP->getPoint();
      }

      x1 = std::min(x1, pt.getX());
      x2 = std::max(x2, pt.getX());
      y1 = std::min(y1, pt.getY());
      y2 = std::max(y2, pt.getY());
    }
    if (x1 <= x2 && y1 <= y2) {
      const Rect box = Rect(x1, y1, x2, y2);
      allPins.clear();
      pinRegionQuery.query(bgi::intersects(box), back_inserter(allPins));
      net->setNumPinsIn(allPins.size());
      net->setPinBox(box);
    } else {
      net->setNumPinsIn(99999);
      net->setPinBox(getExtBox());
    }
  }
}

void FlexDRWorker::initNets_boundaryArea()
{
  const auto& workerRegionQuery = getWorkerRegionQuery();
  std::vector<rq_box_value_t<drConnFig*>> results;
  std::vector<rq_box_value_t<drConnFig*>> results2;

  for (auto& uNet : nets_) {
    auto net = uNet.get();
    for (auto& pin : net->getPins()) {
      if (pin->hasFrTerm()) {
        continue;
      }
      for (auto& ap : pin->getAccessPatterns()) {
        // initialization
        results.clear();
        frCoord currArea = 0;

        const Point bp = ap->getPoint();
        const frLayerNum lNum = ap->getBeginLayerNum();
        const Rect queryBox = Rect(bp, bp);
        workerRegionQuery.query(queryBox, lNum, results);
        for (auto& [ignored, connFig] : results) {
          if (connFig->getNet() != net) {
            continue;
          }
          if (connFig->typeId() == drcPathSeg) {
            auto obj = static_cast<drPathSeg*>(connFig);
            const auto [psBp, psEp] = obj->getPoints();
            // psEp is outside
            if (bp == psBp && (!getRouteBox().intersects(psEp))) {
              // calc area
              const frSegStyle segStyle = obj->getStyle();
              currArea
                  += Point::manhattanDistance(psEp, psBp) * segStyle.getWidth();
              results2.clear();
              workerRegionQuery.query({psEp, psEp}, lNum, results2);
              for (auto& [viaBox2, connFig2] : results) {
                if (connFig2->getNet() != net) {
                  continue;
                }
                if (connFig2->typeId() == drcVia) {
                  auto obj2 = static_cast<drVia*>(connFig2);
                  if (obj2->getOrigin() == psEp) {
                    currArea += viaBox2.area() / 2;
                    break;
                  }
                } else if (connFig2->typeId() == drcPatchWire) {
                  auto obj2 = static_cast<drPatchWire*>(connFig2);
                  if (obj2->getOrigin() == psEp) {
                    currArea += viaBox2.minDXDY()
                                * viaBox2.maxDXDY();  // patch wire no need / 2
                    break;
                  }
                }
              }
            }
            // psBp is outside
            if ((!getRouteBox().intersects(psBp)) && bp == psEp) {
              // calc area
              const frSegStyle segStyle = obj->getStyle();
              currArea
                  += Point::manhattanDistance(psEp, psBp) * segStyle.getWidth();
              results2.clear();
              workerRegionQuery.query({psEp, psEp}, lNum, results2);
              for (auto& [viaBox2, connFig2] : results) {
                if (connFig2->getNet() != net) {
                  continue;
                }
                if (connFig2->typeId() == drcVia) {
                  auto obj2 = static_cast<drVia*>(connFig2);
                  if (obj2->getOrigin() == psBp) {
                    currArea += viaBox2.area() / 2;
                    break;
                  }
                } else if (connFig2->typeId() == drcPatchWire) {
                  auto obj2 = static_cast<drPatchWire*>(connFig2);
                  if (obj2->getOrigin() == psBp) {
                    currArea += viaBox2.area();
                    break;
                  }
                }
              }
            }
          }
        }
        ap->setBeginArea(currArea);
      }
    }
  }
}

void FlexDRWorker::initRipUpNetsFromMarkers()
{
  std::set<drNet*> ripUpNets;
  for (auto& marker : markers_) {
    const auto bloatDist
        = getTech()->getLayer(marker.getLayerNum())->getWidth() * 2;
    getRipUpNetsFromMarker(&marker, ripUpNets, bloatDist);
  }
  for (const auto& net : ripUpNets) {
    for (const auto& fig : net->getRouteConnFigs()) {
      getWorkerRegionQuery().remove(fig.get());
    }
    net->clearRouteConnFigs();
    net->setRipup();
  }
}

void FlexDRWorker::initNets(const frDesign* design)
{
  std::set<frNet*, frBlockObjectComp> nets;
  std::map<frNet*, std::vector<std::unique_ptr<drConnFig>>, frBlockObjectComp>
      netRouteObjs;
  std::map<frNet*, std::vector<std::unique_ptr<drConnFig>>, frBlockObjectComp>
      netExtObjs;
  std::map<frNet*, std::vector<frRect>, frBlockObjectComp> netOrigGuides;
  std::map<frNet*, std::vector<frRect>, frBlockObjectComp> netGuides;
  // get lock
  initNetObjs(design, nets, netRouteObjs, netExtObjs, netOrigGuides, netGuides);
  // release lock
  if (isInitDR()) {
    initNets_initDR(
        design, nets, netRouteObjs, netExtObjs, netOrigGuides, netGuides);
  }
  if (!isInitDR() || getRipupMode() == RipUpMode::INCR) {
    // find instTerm/terms using netRouteObjs;
    initNets_searchRepair(
        design, nets, netRouteObjs, netExtObjs, netOrigGuides);
  }
  getWorkerRegionQuery().init();
  if (getRipupMode() == RipUpMode::NEARDRC) {
    initRipUpNetsFromMarkers();
  }

  initNets_numPinsIn();
  // here because region query is needed
  if (ENABLE_BOUNDARY_MAR_FIX) {
    initNets_boundaryArea();
  }
}

void FlexDRWorker::initTrackCoords_route(
    drNet* net,
    std::map<frCoord, std::map<frLayerNum, frTrackPattern*>>& xMap,
    std::map<frCoord, std::map<frLayerNum, frTrackPattern*>>& yMap)
{
  // add for routes
  std::vector<drConnFig*> allObjs;
  for (auto& uConnFig : net->getExtConnFigs()) {
    allObjs.push_back(uConnFig.get());
  }
  for (auto& uConnFig : net->getRouteConnFigs()) {
    allObjs.push_back(uConnFig.get());
  }
  for (auto& uConnFig : allObjs) {
    if (uConnFig->typeId() == drcPathSeg) {
      auto obj = static_cast<drPathSeg*>(uConnFig);
      const auto [bp, ep] = obj->getPoints();
      const auto lNum = obj->getLayerNum();
      // vertical
      if (bp.x() == ep.x()) {
        // non pref dir
        if (getTech()->getLayer(lNum)->getDir() == dbTechLayerDir::HORIZONTAL) {
          if (lNum + 2 <= getTech()->getTopLayerNum()) {
            xMap[bp.x()][lNum + 2]
                = nullptr;  // default add track to upper layer
          } else if (lNum - 2 >= getTech()->getBottomLayerNum()) {
            xMap[bp.x()][lNum - 2] = nullptr;
          } else {
            std::cout << "Error: initTrackCoords cannot add non-pref track"
                      << std::endl;
          }
          // add bp, ep
          yMap[bp.y()][lNum] = nullptr;
          yMap[ep.y()][lNum] = nullptr;
          // pref dir
        } else {
          xMap[bp.x()][lNum] = nullptr;
          // add bp, ep
          if (lNum + 2 <= getTech()->getTopLayerNum()) {
            yMap[bp.y()][lNum + 2]
                = nullptr;  // default add track to upper layer
            yMap[ep.y()][lNum + 2]
                = nullptr;  // default add track to upper layer
          } else if (lNum - 2 >= getTech()->getBottomLayerNum()) {
            yMap[bp.y()][lNum - 2] = nullptr;
            yMap[ep.y()][lNum - 2] = nullptr;
          } else {
            std::cout << "Error: initTrackCoords cannot add non-pref track"
                      << std::endl;
          }
        }
        // horizontal
      } else {
        // non pref dir
        if (getTech()->getLayer(lNum)->getDir() == dbTechLayerDir::VERTICAL) {
          if (lNum + 2 <= getTech()->getTopLayerNum()) {
            yMap[bp.y()][lNum + 2] = nullptr;
          } else if (lNum - 2 >= getTech()->getBottomLayerNum()) {
            yMap[bp.y()][lNum - 2] = nullptr;
          } else {
            std::cout << "Error: initTrackCoords cannot add non-pref track"
                      << std::endl;
          }
          // add bp, ep
          xMap[bp.x()][lNum] = nullptr;
          xMap[ep.x()][lNum] = nullptr;
        } else {
          yMap[bp.y()][lNum] = nullptr;
          // add bp, ep
          if (lNum + 2 <= getTech()->getTopLayerNum()) {
            xMap[bp.x()][lNum + 2]
                = nullptr;  // default add track to upper layer
            xMap[ep.x()][lNum + 2]
                = nullptr;  // default add track to upper layer
          } else if (lNum - 2 >= getTech()->getBottomLayerNum()) {
            xMap[bp.x()][lNum - 2] = nullptr;
            xMap[ep.x()][lNum - 2] = nullptr;
          } else {
            std::cout << "Error: initTrackCoords cannot add non-pref track"
                      << std::endl;
          }
        }
      }
    } else if (uConnFig->typeId() == drcVia) {
      auto obj = static_cast<drVia*>(uConnFig);
      const Point pt = obj->getOrigin();
      // add pref dir track to layer1
      auto layer1Num = obj->getViaDef()->getLayer1Num();
      if (getTech()->getLayer(layer1Num)->getDir()
          == dbTechLayerDir::HORIZONTAL) {
        yMap[pt.y()][layer1Num] = nullptr;
      } else {
        xMap[pt.x()][layer1Num] = nullptr;
      }
      // add pref dir track to layer2
      auto layer2Num = obj->getViaDef()->getLayer2Num();
      if (getTech()->getLayer(layer2Num)->getDir()
          == dbTechLayerDir::HORIZONTAL) {
        yMap[pt.y()][layer2Num] = nullptr;
      } else {
        xMap[pt.x()][layer2Num] = nullptr;
      }
    } else if (uConnFig->typeId() == drcPatchWire) {
    } else {
      std::cout << "Error: initTrackCoords unsupported type" << std::endl;
    }
  }
}

void FlexDRWorker::initTrackCoords_pin(
    drNet* net,
    std::map<frCoord, std::map<frLayerNum, frTrackPattern*>>& xMap,
    std::map<frCoord, std::map<frLayerNum, frTrackPattern*>>& yMap)
{
  // add for aps
  for (auto& pin : net->getPins()) {
    for (auto& ap : pin->getAccessPatterns()) {
      const Point pt = ap->getPoint();
      const auto lNum = ap->getBeginLayerNum();
      frLayerNum lNum2 = 0;
      if (lNum + 2 <= getTech()->getTopLayerNum()) {
        lNum2 = lNum + 2;
      } else if (lNum - 2 >= getTech()->getBottomLayerNum()) {
        lNum2 = lNum - 2;
      } else {
        std::cout << "Error: initTrackCoords cannot add non-pref track"
                  << std::endl;
      }
      if (getTech()->getLayer(lNum)->getDir() == dbTechLayerDir::HORIZONTAL) {
        yMap[pt.y()][lNum] = nullptr;
      } else {
        xMap[pt.x()][lNum] = nullptr;
      }
      if (getTech()->getLayer(lNum2)->getDir() == dbTechLayerDir::HORIZONTAL) {
        yMap[pt.y()][lNum2] = nullptr;
      } else {
        xMap[pt.x()][lNum2] = nullptr;
      }
    }
  }
}

void FlexDRWorker::initTrackCoords(
    std::map<frCoord, std::map<frLayerNum, frTrackPattern*>>& xMap,
    std::map<frCoord, std::map<frLayerNum, frTrackPattern*>>& yMap)
{
  // add boundary points
  // lNum = -10 to indicate routeBox and extBox frCoord
  const auto rbox = getRouteBox();
  const auto ebox = getExtBox();
  yMap[rbox.yMin()][-10] = nullptr;
  yMap[rbox.yMax()][-10] = nullptr;
  yMap[ebox.yMin()][-10] = nullptr;
  yMap[ebox.yMax()][-10] = nullptr;
  xMap[rbox.xMin()][-10] = nullptr;
  xMap[rbox.xMax()][-10] = nullptr;
  xMap[ebox.xMin()][-10] = nullptr;
  xMap[ebox.xMax()][-10] = nullptr;
  // add all track coords
  for (auto& net : nets_) {
    initTrackCoords_route(net.get(), xMap, yMap);
    initTrackCoords_pin(net.get(), xMap, yMap);
  }
}

void FlexDRWorker::initGridGraph(const frDesign* design)
{
  // get all track coords based on existing objs and aps
  std::map<frCoord, std::map<frLayerNum, frTrackPattern*>> xMap;
  std::map<frCoord, std::map<frLayerNum, frTrackPattern*>> yMap;
  initTrackCoords(xMap, yMap);
  gridGraph_.setCost(workerDRCCost_, workerMarkerCost_, workerFixedShapeCost_);
  gridGraph_.init(design,
                  getRouteBox(),
                  getExtBox(),
                  xMap,
                  yMap,
                  isInitDR(),
                  isFollowGuide());
}

void FlexDRWorker::initMazeIdx_connFig(drConnFig* connFig)
{
  if (connFig->typeId() == drcPathSeg) {
    auto obj = static_cast<drPathSeg*>(connFig);
    auto [bp, ep] = obj->getPoints();
    bp = {std::max(bp.x(), getExtBox().xMin()),
          std::max(bp.y(), getExtBox().yMin())};
    ep = {std::min(ep.x(), getExtBox().xMax()),
          std::min(ep.y(), getExtBox().yMax())};
    const auto lNum = obj->getLayerNum();
    if (gridGraph_.hasMazeIdx(bp, lNum) && gridGraph_.hasMazeIdx(ep, lNum)) {
      FlexMazeIdx bi, ei;
      gridGraph_.getMazeIdx(bi, bp, lNum);
      gridGraph_.getMazeIdx(ei, ep, lNum);
      obj->setMazeIdx(bi, ei);
    } else {
      std::cout << "Error: initMazeIdx_connFig pathseg no idx (" << bp.x()
                << ", " << bp.y() << ") (" << ep.x() << ", " << ep.y() << ") "
                << getTech()->getLayer(lNum)->getName() << std::endl;
    }
  } else if (connFig->typeId() == drcVia) {
    auto obj = static_cast<drVia*>(connFig);
    const Point bp = obj->getOrigin();
    const auto layer1Num = obj->getViaDef()->getLayer1Num();
    const auto layer2Num = obj->getViaDef()->getLayer2Num();
    if (gridGraph_.hasMazeIdx(bp, layer1Num)
        && gridGraph_.hasMazeIdx(bp, layer2Num)) {
      FlexMazeIdx bi, ei;
      gridGraph_.getMazeIdx(bi, bp, layer1Num);
      gridGraph_.getMazeIdx(ei, bp, layer2Num);
      obj->setMazeIdx(bi, ei);
      // std::cout <<"has idx via" <<std::endl;
    } else {
      std::cout << "Error: initMazeIdx_connFig via no idx (" << bp.x() << ", "
                << bp.y() << ") "
                << getTech()->getLayer(layer1Num + 1)->getName() << std::endl;
    }
  } else if (connFig->typeId() == drcPatchWire) {
  } else {
    std::cout << "Error: initMazeIdx_connFig unsupported type" << std::endl;
  }
}

void FlexDRWorker::initMazeIdx_ap(drAccessPattern* ap)
{
  const Point bp = ap->getPoint();
  const auto lNum = ap->getBeginLayerNum();
  if (gridGraph_.hasMazeIdx(bp, lNum)) {
    FlexMazeIdx bi;
    gridGraph_.getMazeIdx(bi, bp, lNum);
    ap->setMazeIdx(bi);
    // set curr layer on track status
    if (getTech()->getLayer(lNum)->getDir() == dbTechLayerDir::HORIZONTAL) {
      if (gridGraph_.hasGridCost(bi.x(), bi.y(), bi.z(), frDirEnum::W)
          || gridGraph_.hasGridCost(bi.x(), bi.y(), bi.z(), frDirEnum::E)) {
        ap->setOnTrack(false, true);
      }
    } else {
      if (gridGraph_.hasGridCost(bi.x(), bi.y(), bi.z(), frDirEnum::S)
          || gridGraph_.hasGridCost(bi.x(), bi.y(), bi.z(), frDirEnum::N)) {
        ap->setOnTrack(false, false);
      }
    }
  } else {
    std::cout << "Error: initMazeIdx_ap no idx (" << bp.x() << ", " << bp.y()
              << ") " << getTech()->getLayer(lNum)->getName() << std::endl;
  }

  if (gridGraph_.hasMazeIdx(bp, lNum + 2)) {
    FlexMazeIdx bi;
    gridGraph_.getMazeIdx(bi, bp, lNum + 2);
    // set curr layer on track status
    if (getTech()->getLayer(lNum + 2)->getDir() == dbTechLayerDir::HORIZONTAL) {
      if (gridGraph_.hasGridCost(bi.x(), bi.y(), bi.z(), frDirEnum::W)
          || gridGraph_.hasGridCost(bi.x(), bi.y(), bi.z(), frDirEnum::E)) {
        ap->setOnTrack(false, true);
      }
    } else {
      if (gridGraph_.hasGridCost(bi.x(), bi.y(), bi.z(), frDirEnum::S)
          || gridGraph_.hasGridCost(bi.x(), bi.y(), bi.z(), frDirEnum::N)) {
        ap->setOnTrack(false, false);
      }
    }
  }
}

void FlexDRWorker::initMazeIdx()
{
  for (auto& net : nets_) {
    for (auto& connFig : net->getExtConnFigs()) {
      initMazeIdx_connFig(connFig.get());
    }
    for (auto& connFig : net->getRouteConnFigs()) {
      initMazeIdx_connFig(connFig.get());
    }
    for (auto& pin : net->getPins()) {
      for (auto& ap : pin->getAccessPatterns()) {
        initMazeIdx_ap(ap.get());
      }
    }
  }
}

void FlexDRWorker::initMazeCost_ap_planarGrid_helper(const FlexMazeIdx& mi,
                                                     const frDirEnum& dir,
                                                     const frCoord bloatLen,
                                                     const bool isAddPathCost)
{
  frCoord currLen = 0;
  frMIdx x = mi.x();
  frMIdx y = mi.y();
  frMIdx z = mi.z();
  while (true) {
    if (currLen > bloatLen || !gridGraph_.hasEdge(x, y, z, dir)) {
      break;
    }
    if (isAddPathCost) {
      gridGraph_.setGridCost(x, y, z, dir);
      gridGraph_.setGridCost(x, y, z, frDirEnum::D);
      gridGraph_.setGridCost(x, y, z, frDirEnum::U);
    } else {
      gridGraph_.resetGridCost(x, y, z, dir);
      gridGraph_.resetGridCost(x, y, z, frDirEnum::D);
      gridGraph_.resetGridCost(x, y, z, frDirEnum::U);
    }
    switch (dir) {
      case frDirEnum::W:
        --x;
        currLen += gridGraph_.getEdgeLength(x, y, z, frDirEnum::W);
        break;
      case frDirEnum::E:
        ++x;
        currLen += gridGraph_.getEdgeLength(x, y, z, frDirEnum::E);
        break;
      case frDirEnum::S:
        --y;
        currLen += gridGraph_.getEdgeLength(x, y, z, frDirEnum::S);
        break;
      case frDirEnum::N:
        ++y;
        currLen += gridGraph_.getEdgeLength(x, y, z, frDirEnum::N);
        break;
      default:;
    }
  }
}

void FlexDRWorker::initMazeCost_ap_helper(drNet* net, const bool isAddPathCost)
{
  const int planarGridBloatNumWidth = 10;
  for (auto& pin : net->getPins()) {
    bool isStdCellPin = true;
    auto term = pin->getFrTerm();
    if (term) {
      switch (term->typeId()) {
        case frcInstTerm: {  // macro cell or stdcell
          const dbMasterType masterType = static_cast<frInstTerm*>(term)
                                              ->getInst()
                                              ->getMaster()
                                              ->getMasterType();
          if (masterType.isBlock() || masterType.isPad()
              || masterType == dbMasterType::RING) {
            isStdCellPin = false;
          }
          break;
        }
        case frcBTerm: {  // IO
          isStdCellPin = false;
          break;
        }
        default:
          break;
      }
    } else {
      continue;
    }

    bool hasUpperOnTrackAP = false;
    if (isStdCellPin) {
      for (auto& ap : pin->getAccessPatterns()) {
        const frLayerNum lNum = ap->getBeginLayerNum();
        if (ap->hasValidAccess(frDirEnum::U)) {
          if (getTech()->getLayer(lNum + 2)->getDir()
                  == dbTechLayerDir::HORIZONTAL
              && ap->isOnTrack(true)) {
            hasUpperOnTrackAP = true;
            break;
          }
          if (getTech()->getLayer(lNum + 2)->getDir()
                  == dbTechLayerDir::VERTICAL
              && ap->isOnTrack(false)) {
            hasUpperOnTrackAP = true;
            break;
          }
        }
      }
    }

    for (auto& ap : pin->getAccessPatterns()) {
      const FlexMazeIdx mi = ap->getMazeIdx();
      const frLayerNum lNum = ap->getBeginLayerNum();
      const frCoord defaultWidth = getTech()->getLayer(lNum)->getWidth();
      if (ap->hasValidAccess(frDirEnum::U)) {
        if (lNum + 2 <= getTech()->getTopLayerNum()) {
          const auto upperDefaultWidth
              = getTech()->getLayer(lNum + 2)->getWidth();
          if (getTech()->getLayer(lNum + 2)->getDir()
                  == dbTechLayerDir::HORIZONTAL
              && !ap->isOnTrack(true)) {
            if (!hasUpperOnTrackAP) {
              const auto upperMi = FlexMazeIdx(mi.x(), mi.y(), mi.z() + 1);
              initMazeCost_ap_planarGrid_helper(
                  upperMi,
                  frDirEnum::W,
                  planarGridBloatNumWidth * upperDefaultWidth,
                  isAddPathCost);
              initMazeCost_ap_planarGrid_helper(
                  upperMi,
                  frDirEnum::E,
                  planarGridBloatNumWidth * upperDefaultWidth,
                  isAddPathCost);
            }
          }
          if (getTech()->getLayer(lNum + 2)->getDir()
                  == dbTechLayerDir::VERTICAL
              && !ap->isOnTrack(false)) {
            if (!hasUpperOnTrackAP) {
              const auto upperMi = FlexMazeIdx(mi.x(), mi.y(), mi.z() + 1);
              initMazeCost_ap_planarGrid_helper(
                  upperMi,
                  frDirEnum::N,
                  planarGridBloatNumWidth * upperDefaultWidth,
                  isAddPathCost);
              initMazeCost_ap_planarGrid_helper(
                  upperMi,
                  frDirEnum::S,
                  planarGridBloatNumWidth * upperDefaultWidth,
                  isAddPathCost);
            }
          }
        }

        if (isAddPathCost) {
          gridGraph_.resetOverrideShapeCostVia(mi.x(), mi.y(), mi.z());
        } else {
          gridGraph_.setOverrideShapeCostVia(mi.x(), mi.y(), mi.z());
        }
      }

      if (!isStdCellPin) {
        for (const auto dir : frDirEnumPlanar) {
          initMazeCost_ap_planarGrid_helper(
              mi, dir, planarGridBloatNumWidth * defaultWidth, isAddPathCost);
        }
      }
    }
  }
}

void FlexDRWorker::initMazeCost_ap()
{
  for (auto& net : nets_) {
    for (auto& pin : net->getPins()) {
      for (auto& ap : pin->getAccessPatterns()) {
        const FlexMazeIdx mi = ap->getMazeIdx();
        for (const auto dir : frDirEnumAll) {
          if (!ap->hasValidAccess(dir)) {
            gridGraph_.setBlocked(mi.x(), mi.y(), mi.z(), dir);
          } else {
            gridGraph_.resetBlocked(mi.x(), mi.y(), mi.z(), dir);
          }
        }

        if (ap->hasAccessViaDef(frDirEnum::U)) {
          gridGraph_.setSVia(mi.x(), mi.y(), mi.z());
          apSVia_[mi] = ap.get();
        }
      }
    }
  }
}

void FlexDRWorker::initMazeCost_marker_route_queue_addHistoryCost(
    const frMarker& marker)
{
  std::set<drNet*> vioNets;  // for self-violation, only add cost for one side
                             // (experiment with self cut spacing)

  const Rect mBox = marker.getBBox();
  const auto lNum = marker.getLayerNum();

  std::vector<rq_box_value_t<drConnFig*>> results;
  getWorkerRegionQuery().query(mBox, lNum, results);

  for (auto& [objBox, connFig] : results) {
    if (connFig->typeId() == drcPathSeg) {
      auto obj = static_cast<drPathSeg*>(connFig);
      // skip if unfixable obj
      const auto [bp, ep] = obj->getPoints();
      if (!(getRouteBox().intersects(bp) && getRouteBox().intersects(ep))) {
        continue;
      }
      // add history cost
      // get points to mark up, markup up to "width" grid points to the left and
      // right of pathseg
      const frSegStyle segStyle = obj->getStyle();
      const frCoord width = segStyle.getWidth();
      Rect bloatBox;
      mBox.bloat(width, bloatBox);
      FlexMazeIdx mIdx1, mIdx2;
      gridGraph_.getIdxBox(mIdx1, mIdx2, bloatBox);

      const auto [objMIdx1, objMIdx2] = obj->getMazeIdx();
      const bool isH = (objMIdx1.y() == objMIdx2.y());
      // temporarily bloat marker at most 4X width to find neighboring grid
      // point block at least two width grid points to ensure no matter where
      // the startpoint is
      for (int i = 0; i < 5; i++) {
        if (i == 4) {
          std::cout
              << "Warning: marker bloat 4x width but could not find two grids "
                 "to add marker cost"
              << std::endl;
          std::cout << "  marker -- src: ";
          for (auto src : marker.getSrcs()) {
            if (src) {
              switch (src->typeId()) {
                case frcNet:
                  std::cout << (static_cast<frNet*>(src))->getName() << " ";
                  break;
                case frcInstTerm: {
                  frInstTerm* instTerm = (static_cast<frInstTerm*>(src));
                  std::cout << instTerm->getInst()->getName() << "/"
                            << instTerm->getTerm()->getName() << " ";
                  break;
                }
                case frcBTerm: {
                  frBTerm* bterm = (static_cast<frBTerm*>(src));
                  std::cout << "PIN/" << bterm->getName() << " ";
                  break;
                }
                case frcInstBlockage: {
                  frInst* inst = (static_cast<frInstBlockage*>(src))->getInst();
                  std::cout << inst->getName() << "/OBS"
                            << " ";
                  break;
                }
                case frcInst: {
                  frInst* inst = (static_cast<frInst*>(src));
                  std::cout << inst->getName() << "/OBS"
                            << " ";
                  break;
                }
                case frcBlockage: {
                  std::cout << "PIN/OBS"
                            << " ";
                  break;
                }
                default:;
              }
            }
            std::cout << "\n";
            // get violation bbox
            const Rect bbox = marker.getBBox();
            const double dbu = getTech()->getDBUPerUU();
            std::cout << "    bbox = ( " << bbox.xMin() / dbu << ", "
                      << bbox.yMin() / dbu << " ) - ( " << bbox.xMax() / dbu
                      << ", " << bbox.yMax() / dbu << " ) on Layer ";
            if (getTech()->getLayer(marker.getLayerNum())->getType()
                    == dbTechLayerType::CUT
                && marker.getLayerNum() - 1 >= getTech()->getBottomLayerNum()) {
              std::cout
                  << getTech()->getLayer(marker.getLayerNum() - 1)->getName()
                  << "\n";
            } else {
              std::cout << getTech()->getLayer(marker.getLayerNum())->getName()
                        << "\n";
            }
          }
        }
        if (isH
            && std::max(objMIdx1.x(), mIdx1.x())
                   >= std::min(objMIdx2.x(), mIdx2.x())) {
          bloatBox.bloat(width, bloatBox);
          // std::cout <<"i=" <<i <<" " <<width <<", " <<bloatBox <<std::endl;
        } else if ((!isH)
                   && std::max(objMIdx1.y(), mIdx1.y())
                          >= std::min(objMIdx2.y(), mIdx2.y())) {
          bloatBox.bloat(width, bloatBox);
          // std::cout <<bloatBox <<std::endl;
        } else {
          break;
        }
        gridGraph_.getIdxBox(mIdx1, mIdx2, bloatBox);
      }
      if (isH) {
        for (int i = std::max(objMIdx1.x(), mIdx1.x());
             i <= std::min(objMIdx2.x(), mIdx2.x());
             i++) {
          gridGraph_.addMarkerCostPlanar(i, objMIdx1.y(), objMIdx1.z());
          planarHistoryMarkers_.insert(
              FlexMazeIdx(i, objMIdx1.y(), objMIdx1.z()));
        }
      } else {
        for (int i = std::max(objMIdx1.y(), mIdx1.y());
             i <= std::min(objMIdx2.y(), mIdx2.y());
             i++) {
          gridGraph_.addMarkerCostPlanar(objMIdx1.x(), i, objMIdx1.z());
          planarHistoryMarkers_.insert(
              FlexMazeIdx(objMIdx1.x(), i, objMIdx1.z()));
        }
      }
    } else if (connFig->typeId() == drcVia) {
      auto obj = static_cast<drVia*>(connFig);
      auto bp = obj->getOrigin();
      // skip if unfixable obj
      if (!getRouteBox().intersects(bp)) {
        continue;
      }
      if (vioNets.find(obj->getNet()) == vioNets.end()) {
        // add history cost
        const auto [objMIdx1, objMIdx2] = obj->getMazeIdx();
        if (viaHistoryMarkers_.find(objMIdx1) == viaHistoryMarkers_.end()) {
          gridGraph_.addMarkerCostVia(objMIdx1.x(), objMIdx1.y(), objMIdx1.z());
          viaHistoryMarkers_.insert(objMIdx1);

          vioNets.insert(obj->getNet());
        }
      }
    } else if (connFig->typeId() == drcPatchWire) {
      auto obj = static_cast<drPatchWire*>(connFig);
      auto bp = obj->getOrigin();
      // skip if unfixable obj
      if (!getRouteBox().intersects(bp)) {
        continue;
      }
      // add history cost
      // gridGraph_.getMazeIdx(objMIdx1, bp, lNum);
      Rect patchBBox = obj->getBBox();
      Point patchLL = patchBBox.ll();
      Point patchUR = patchBBox.ur();
      FlexMazeIdx startIdx, endIdx;
      gridGraph_.getMazeIdx(startIdx, patchLL, lNum);
      gridGraph_.getMazeIdx(endIdx, patchUR, lNum);
      for (auto xIdx = startIdx.x(); xIdx <= endIdx.x(); xIdx++) {
        for (auto yIdx = startIdx.y(); yIdx <= endIdx.y(); yIdx++) {
          gridGraph_.addMarkerCostPlanar(xIdx, yIdx, startIdx.z());
          gridGraph_.addMarkerCost(
              xIdx,
              yIdx,
              startIdx.z(),
              frDirEnum::U);  // always block upper via in case stack via
          gridGraph_.addMarkerCost(
              xIdx,
              yIdx,
              startIdx.z(),
              frDirEnum::D);  // always block upper via in case stack via
          FlexMazeIdx objMIdx(xIdx, yIdx, startIdx.z());
          planarHistoryMarkers_.insert(objMIdx);
        }
      }
    }
  }
}

// TODO: modify behavior for bloat width != 0
void FlexDRWorker::initMazeCost_marker_route_queue(const frMarker& marker)
{
  initMazeCost_marker_route_queue_addHistoryCost(marker);
}

void FlexDRWorker::route_queue_resetRipup()
{
  for (auto& net : nets_) {
    net->resetNumReroutes();
  }
}

void FlexDRWorker::route_queue_markerCostDecay()
{
  for (auto it = planarHistoryMarkers_.begin();
       it != planarHistoryMarkers_.end();) {
    auto currIt = it;
    const auto& mi = *currIt;
    ++it;
    if (gridGraph_.decayMarkerCostPlanar(
            mi.x(), mi.y(), mi.z(), workerMarkerDecay_)) {
      planarHistoryMarkers_.erase(currIt);
    }
  }
  for (auto it = viaHistoryMarkers_.begin(); it != viaHistoryMarkers_.end();) {
    auto currIt = it;
    const auto& mi = *currIt;
    ++it;
    if (gridGraph_.decayMarkerCostVia(
            mi.x(), mi.y(), mi.z(), workerMarkerDecay_)) {
      viaHistoryMarkers_.erase(currIt);
    }
  }
}

void FlexDRWorker::route_queue_addMarkerCost(
    const std::vector<std::unique_ptr<frMarker>>& markers)
{
  for (auto& uMarker : markers) {
    auto& marker = *(uMarker.get());
    initMazeCost_marker_route_queue(marker);
  }
}

// init from member markers
void FlexDRWorker::route_queue_addMarkerCost()
{
  for (auto& marker : markers_) {
    initMazeCost_marker_route_queue(marker);
  }
}

void FlexDRWorker::route_queue_init_queue(
    std::queue<RouteQueueEntry>& rerouteQueue)
{
  std::set<frBlockObject*> uniqueVictims;
  std::set<frBlockObject*> uniqueAggressors;
  std::vector<RouteQueueEntry> checks;
  std::vector<RouteQueueEntry> routes;

  if (getRipupMode() == RipUpMode::DRC) {
    for (auto& marker : markers_) {
      route_queue_update_from_marker(
          &marker, uniqueVictims, uniqueAggressors, checks, routes);
    }
    mazeIterInit_sortRerouteQueue(0, checks);
    mazeIterInit_sortRerouteQueue(0, routes);
  } else if (getRipupMode() == RipUpMode::ALL) {
    // ripup all nets and clear objs here
    // nets are ripped up during initNets()
    std::vector<drNet*> ripupNets;
    ripupNets.reserve(nets_.size());
    for (auto& net : nets_) {
      ripupNets.push_back(net.get());
    }
    // sort nets
    mazeIterInit_sortRerouteNets(0, ripupNets);
    for (auto& net : ripupNets) {
      routes.push_back({net, 0, true});
      // reserve via because all nets are ripped up
      initMazeCost_via_helper(net, true);
      // no need to clear the net because route objs are not pushed to the net
      // (See FlexDRWorker::initNet)
    }
  } else if (getRipupMode() == RipUpMode::NEARDRC) {
    std::vector<drNet*> ripupNets;
    for (auto& net : nets_) {
      if (net->isRipup()) {
        ripupNets.push_back(net.get());
      }
    }
    int currId = ripupNets.size();
    std::set<drNet*> addedNets;
    for (auto& marker : markers_) {
      for (auto net : ripupNets) {
        if (marker.getSrcs().find(net->getFrNet()) != marker.getSrcs().end()) {
          if (addedNets.find(net) != addedNets.end()) {
            continue;
          }
          addedNets.insert(net);
          net->setPriority(currId--);
        }
      }
    }
    // sort nets
    mazeIterInit_sortRerouteNets(0, ripupNets);
    for (auto& net : ripupNets) {
      routes.push_back({net, 0, true});
      initMazeCost_via_helper(net, true);
    }
  } else if (getRipupMode() == RipUpMode::INCR) {
    std::vector<drNet*> ripupNets;
    ripupNets.reserve(nets_.size());
    for (auto& net : nets_) {
      if (!net->getFrNet()->hasInitialRouting()) {
        ripupNets.push_back(net.get());
      }
    }
    // sort nets
    mazeIterInit_sortRerouteNets(0, ripupNets);
    for (auto& net : ripupNets) {
      routes.push_back({net, 0, true});
      // reserve via because all nets are ripped up
      initMazeCost_via_helper(net, true);
      // no need to clear the net because route objs are not pushed to the net
      // (See FlexDRWorker::initNet)
    }
  } else {
    std::cout << "Error: unsupported ripup mode\n";
  }
  route_queue_update_queue(checks, routes, rerouteQueue);
}

void FlexDRWorker::route_queue_update_queue(
    const std::vector<RouteQueueEntry>& checks,
    const std::vector<RouteQueueEntry>& routes,
    std::queue<RouteQueueEntry>& rerouteQueue)
{
  for (auto& route : routes) {
    rerouteQueue.push(route);
  }
  for (auto& check : checks) {
    rerouteQueue.push(check);
  }
}

//*****************************************************************************************//
// EXPONENTIAL QUEUE SIZE IF NOT MAKE AGGRESSORS AND VICTIMS UNIQUE FOR A SET OF
// MARKERS!! // NET A --> PUSH ROUTE A --> PUSH CHECK A * 3 --> //
//        |                                      | //
//        ---------------------------------------- //
//*****************************************************************************************//
void FlexDRWorker::route_queue_update_from_marker(
    frMarker* marker,
    std::set<frBlockObject*>& uniqueVictims,
    std::set<frBlockObject*>& uniqueAggressors,
    std::vector<RouteQueueEntry>& checks,
    std::vector<RouteQueueEntry>& routes)
{
  // if shapes don't overlap routeBox, ignore violation
  if (!getRouteBox().intersects(marker->getBBox())) {
    bool overlaps = false;
    for (auto& s : marker->getAggressors()) {
      if (std::get<1>(s.second).intersects(getRouteBox())) {
        overlaps = true;
        break;
      }
    }
    if (!overlaps) {
      for (auto& s : marker->getVictims()) {
        if (std::get<1>(s.second).intersects(getRouteBox())) {
          overlaps = true;
          break;
        }
      }
      if (!overlaps) {
        return;
      }
    }
  }
  std::vector<frBlockObject*> uniqueVictimOwners;     // to maintain order
  std::vector<frBlockObject*> uniqueAggressorOwners;  // to maintain order

  auto& markerAggressors = marker->getAggressors();
  std::set<frNet*> movableAggressorNets;
  std::set<frBlockObject*> movableAggressorOwners;

  for (auto& aggressorPair : markerAggressors) {
    auto& aggressor = aggressorPair.first;
    if (aggressor && aggressor->typeId() == frcNet) {
      auto fNet = static_cast<frNet*>(aggressor);
      if (!fNet->getType().isSupply()) {
        movableAggressorNets.insert(fNet);
        if (getDRNets(fNet)) {
          for (auto dNet : *(getDRNets(fNet))) {
            if (!canRipup(dNet)) {
              continue;
            }
            movableAggressorOwners.insert(aggressor);
          }
        }
      }
    }
  }

  // push movable aggressors for reroute and other srcs for drc checking
  bool hasRerouteNet = false;
  if (!movableAggressorNets.empty()) {
    for (auto& fNet : movableAggressorNets) {
      if (getDRNets(fNet)) {
        // int subNetIdx = -1;
        for (auto dNet : *(getDRNets(fNet))) {
          // subNetIdx++;
          if (!canRipup(dNet)) {
            continue;
          }
          // rerouteQueue.push_back(std::make_pair(dNet, std::make_pair(true,
          // dNet->getNumReroutes())));
          if (uniqueAggressors.find(fNet) == uniqueAggressors.end()) {
            uniqueAggressors.insert(fNet);
            uniqueAggressorOwners.push_back(fNet);
          }
          hasRerouteNet = true;
        }
      }
    }
  }

  if (hasRerouteNet) {
    std::set<frBlockObject*> checkDRCOwners;
    for (auto& src : marker->getSrcs()) {
      if (movableAggressorOwners.find(src) == movableAggressorOwners.end()) {
        if (src) {
          checkDRCOwners.insert(src);
        }
      }
    }
    // push checkDRCOwners to queue for DRC
    for (auto& owner : checkDRCOwners) {
      // rerouteQueue.push_back(std::make_pair(owner, std::make_pair(false,
      // -1)));
      if (uniqueVictims.find(owner) == uniqueVictims.end()) {
        uniqueVictims.insert(owner);
        uniqueVictimOwners.push_back(owner);
      }
    }
  } else {
    std::set<frBlockObject*> owners, otherOwners, routeOwners;
    auto& srcs = marker->getSrcs();
    for (auto& src : srcs) {
      if (src) {
        owners.insert(src);
      }
    }
    std::set_difference(owners.begin(),
                        owners.end(),
                        movableAggressorOwners.begin(),
                        movableAggressorOwners.end(),
                        std::inserter(otherOwners, otherOwners.end()));
    for (auto& owner : otherOwners) {
      if (owner && owner->typeId() == frcNet) {
        auto fNet = static_cast<frNet*>(owner);
        if (!fNet->getType().isSupply()) {
          if (getDRNets(fNet)) {
            // int subNetIdx = -1;
            for (auto dNet : *(getDRNets(fNet))) {
              if (!canRipup(dNet)) {
                continue;
              }
              if (uniqueAggressors.find(fNet) == uniqueAggressors.end()) {
                uniqueAggressors.insert(fNet);
                uniqueAggressorOwners.push_back(fNet);
              }
              routeOwners.insert(owner);
              hasRerouteNet = true;
            }
          }
        }
      }
    }
    if (hasRerouteNet) {
      std::set<frBlockObject*> checkDRCOwners;
      for (auto& src : marker->getSrcs()) {
        if (routeOwners.find(src) == routeOwners.end()) {
          if (src) {
            checkDRCOwners.insert(src);
          }
        }
      }
      // push checkDRCOwners to queue for DRC
      for (auto& owner : checkDRCOwners) {
        if (uniqueVictims.find(owner) == uniqueVictims.end()) {
          uniqueVictims.insert(owner);
          uniqueVictimOwners.push_back(owner);
        }
      }
    }
  }
  std::vector<drNet*> avoidRipupCandidates;
  bool allowAvoidRipup = false;

  // add to victims and aggressors as appropriate
  for (auto& aggressorOwner : uniqueAggressorOwners) {
    if (aggressorOwner && aggressorOwner->typeId() == frcNet) {
      auto fNet = static_cast<frNet*>(aggressorOwner);
      if (!fNet->getType().isSupply()) {
        if (getDRNets(fNet)) {
          for (auto dNet : *(getDRNets(fNet))) {
            if (!canRipup(dNet)) {
              continue;
            }
            if (uniqueAggressorOwners.size() + uniqueVictimOwners.size() > 1) {
              if (dNet->canAvoidRipup()) {
                avoidRipupCandidates.push_back(dNet);
                continue;
              }
              allowAvoidRipup = true;
              dNet->setNRipupAvoids(0);
            }
            routes.push_back({dNet, dNet->getNumReroutes(), true});
          }
        }
      }
    }
  }
  for (drNet* dNet : avoidRipupCandidates) {
    if (allowAvoidRipup) {
      dNet->incNRipupAvoids();
      checks.push_back({dNet, -1, false});
    } else {
      dNet->setNRipupAvoids(0);
      routes.push_back({dNet, dNet->getNumReroutes(), true});
    }
  }
  for (auto& victimOwner : uniqueVictimOwners) {
    checks.push_back({victimOwner, -1, false});
  }
}

void FlexDRWorker::getRipUpNetsFromMarker(frMarker* marker,
                                          std::set<drNet*>& nets,
                                          const frCoord bloatDist)
{
  // if shapes don't overlap routeBox, ignore violation
  if (!getRouteBox().intersects(marker->getBBox())) {
    bool overlaps = false;
    for (auto& s : marker->getAggressors()) {
      if (std::get<1>(s.second).intersects(getRouteBox())) {
        overlaps = true;
        break;
      }
    }
    if (!overlaps) {
      for (auto& s : marker->getVictims()) {
        if (std::get<1>(s.second).intersects(getRouteBox())) {
          overlaps = true;
          break;
        }
      }
      if (!overlaps) {
        return;
      }
    }
  }
  auto bbox = marker->getBBox();
  bbox.bloat(bloatDist, bbox);
  std::vector<drConnFig*> figs;
  getWorkerRegionQuery().query(bbox, marker->getLayerNum(), figs);
  for (auto fig : figs) {
    auto net = fig->getNet();
    nets.insert(net);
  }
}

bool FlexDRWorker::canRipup(drNet* n)
{
  return !(n->getNumReroutes() >= getMazeEndIter()) && !n->getFrNet()->isFixed()
         && n->getFrNet()->hasGuides()
         && (getRipupMode() != RipUpMode::INCR
             || (getRipupMode() == RipUpMode::INCR
                 && !n->getFrNet()->hasInitialRouting()));
}

void FlexDRWorker::route_queue_update_queue(
    const std::vector<std::unique_ptr<frMarker>>& markers,
    std::queue<RouteQueueEntry>& rerouteQueue)
{
  std::set<frBlockObject*> uniqueVictims;
  std::set<frBlockObject*> uniqueAggressors;
  std::vector<RouteQueueEntry> checks;
  std::vector<RouteQueueEntry> routes;

  for (auto& uMarker : markers) {
    auto marker = uMarker.get();
    route_queue_update_from_marker(
        marker, uniqueVictims, uniqueAggressors, checks, routes);
  }

  route_queue_update_queue(checks, routes, rerouteQueue);
}

void FlexDRWorker::initMazeCost_guide_helper(drNet* net, const bool isAdd)
{
  for (auto& rect : net->getOrigGuides()) {
    const Rect box = rect.getBBox();
    const frLayerNum lNum = rect.getLayerNum();
    const frMIdx z = gridGraph_.getMazeZIdx(lNum);
    FlexMazeIdx mIdx1, mIdx2;
    gridGraph_.getIdxBox(mIdx1, mIdx2, box);
    if (isAdd) {
      gridGraph_.setGuide(mIdx1.x(), mIdx1.y(), mIdx2.x(), mIdx2.y(), z);
    } else {
      gridGraph_.resetGuide(mIdx1.x(), mIdx1.y(), mIdx2.x(), mIdx2.y(), z);
    }
  }
}

void FlexDRWorker::initMazeCost(const frDesign* design)
{
  // init Maze cost by snet shapes and blockages
  initMazeCost_fixedObj(design);
  initMazeCost_ap();
  initMazeCost_connFig();
  // init Maze Cost by planar access terms (prevent early wrongway / turn)
  initMazeCost_planarTerm(design);
}

// init maze cost for snet objs and blockages
void FlexDRWorker::initMazeCost_fixedObj(const frDesign* design)
{
  frRegionQuery::Objects<frBlockObject> result;
  frMIdx zIdx = 0;
  std::map<frNet*, std::set<frBlockObject*>> frNet2Terms;
  for (auto layerNum = getTech()->getBottomLayerNum();
       layerNum <= getTech()->getTopLayerNum();
       ++layerNum) {
    bool isRoutingLayer = true;
    result.clear();
    if (getTech()->getLayer(layerNum)->getType() == dbTechLayerType::ROUTING) {
      isRoutingLayer = true;
      zIdx = gridGraph_.getMazeZIdx(layerNum);
    } else if (getTech()->getLayer(layerNum)->getType()
               == dbTechLayerType::CUT) {
      isRoutingLayer = false;
      if (getTech()->getBottomLayerNum() <= layerNum - 1
          && getTech()->getLayer(layerNum - 1)->getType()
                 == dbTechLayerType::ROUTING) {
        zIdx = gridGraph_.getMazeZIdx(layerNum - 1);
      } else {
        continue;
      }
    } else {
      continue;
    }
    design->getRegionQuery()->query(getExtBox(), layerNum, result);
    // process blockage first, then unblock based on pin shape
    for (auto& [box, obj] : result) {
      if ((obj->typeId() == frcBlockage)
          || (obj->typeId() == frcInstBlockage)) {
        if (isRoutingLayer) {
          // assume only routing layer
          modMinSpacingCostPlanar(box, zIdx, ModCostType::addFixedShape, true);
          modMinSpacingCostVia(
              box, zIdx, ModCostType::addFixedShape, true, false, true);
          modMinSpacingCostVia(
              box, zIdx, ModCostType::addFixedShape, false, false, true);
          modEolSpacingRulesCost(box, zIdx, ModCostType::addFixedShape);
          // block
          modBlockedPlanar(box, zIdx, true);
          modBlockedVia(box, zIdx, true);
        } else {
          modCutSpacingCost(box, zIdx, ModCostType::addFixedShape, true);
          modInterLayerCutSpacingCost(
              box, zIdx, ModCostType::addFixedShape, true);
          modInterLayerCutSpacingCost(
              box, zIdx, ModCostType::addFixedShape, false);
        }
      }
    }
    for (auto& [box, obj] : result) {
      switch (obj->typeId()) {
        case frcBTerm: {  // term no bloat
          frNet2Terms[static_cast<frBTerm*>(obj)->getNet()].insert(obj);
          break;
        }
        case frcInstTerm: {
          frNet2Terms[static_cast<frInstTerm*>(obj)->getNet()].insert(obj);
          if (isRoutingLayer) {
            // unblock planar edge for obs over pin, ap will unblock via edge
            // for legal pin access
            modBlockedPlanar(box, zIdx, false);
            if (zIdx <= (VIA_ACCESS_LAYERNUM / 2 - 1)) {
              modMinSpacingCostPlanar(
                  box, zIdx, ModCostType::addFixedShape, true);
              modEolSpacingRulesCost(box, zIdx, ModCostType::addFixedShape);
            }
          } else {
            modCutSpacingCost(box, zIdx, ModCostType::addFixedShape, true);
            modInterLayerCutSpacingCost(
                box, zIdx, ModCostType::addFixedShape, true);
            modInterLayerCutSpacingCost(
                box, zIdx, ModCostType::addFixedShape, false);
          }
          break;
        }
        case frcPathSeg: {  // snet
          auto ps = static_cast<frPathSeg*>(obj);
          // assume only routing layer
          modMinSpacingCostPlanar(box, zIdx, ModCostType::addFixedShape);
          modMinSpacingCostVia(
              box, zIdx, ModCostType::addFixedShape, true, true);
          modMinSpacingCostVia(
              box, zIdx, ModCostType::addFixedShape, false, true);
          modEolSpacingRulesCost(box, zIdx, ModCostType::addFixedShape);
          // block for PDN (fixed obj)
          if (ps->getNet()->getType().isSupply()) {
            modBlockedPlanar(box, zIdx, true);
            modBlockedVia(box, zIdx, true);
          }
          break;
        }
        case frcVia: {  // snet
          if (isRoutingLayer) {
            // assume only routing layer
            modMinSpacingCostPlanar(box, zIdx, ModCostType::addFixedShape);
            modMinSpacingCostVia(
                box, zIdx, ModCostType::addFixedShape, true, false);
            modMinSpacingCostVia(
                box, zIdx, ModCostType::addFixedShape, false, false);
            modEolSpacingRulesCost(box, zIdx, ModCostType::addFixedShape);
          } else {
            auto via = static_cast<frVia*>(obj);
            modAdjCutSpacingCost_fixedObj(design, box, via);

            modCutSpacingCost(box, zIdx, ModCostType::addFixedShape);
            modInterLayerCutSpacingCost(
                box, zIdx, ModCostType::addFixedShape, true);
            modInterLayerCutSpacingCost(
                box, zIdx, ModCostType::addFixedShape, false);
          }
          break;
        }
        default:
          break;
      }
    }
  }

  // assign terms to each subnet
  for (auto& [net, objs] : frNet2Terms) {
    for (auto dNet : owner2nets_[net]) {
      dNet->setFrNetTerms(objs);
    }
    initMazeCost_terms(objs, true);
  }
}

void FlexDRWorker::modBlockedEdgesForMacroPin(frInstTerm* instTerm,
                                              const dbTransform& xform,
                                              const bool isAddCost)
{
  const frDirEnum dirs[4]{
      frDirEnum::E, frDirEnum::W, frDirEnum::N, frDirEnum::S};
  const int pinAccessIdx = instTerm->getInst()->getPinAccessIdx();
  for (auto& pin : instTerm->getTerm()->getPins()) {
    if (!pin->hasPinAccess()) {
      continue;
    }
    for (auto& ap : pin->getPinAccess(pinAccessIdx)->getAccessPoints()) {
      Point bp = ap->getPoint();
      xform.apply(bp);
      const frMIdx xIdx = gridGraph_.getMazeXIdx(bp.x());
      const frMIdx yIdx = gridGraph_.getMazeYIdx(bp.y());
      const frMIdx zIdx = gridGraph_.getMazeZIdx(ap->getLayerNum());
      for (frDirEnum dir : dirs) {
        if (ap->hasAccess(dir)) {
          if (isAddCost) {
            gridGraph_.setBlocked(xIdx, yIdx, zIdx, dir);
          } else {
            gridGraph_.resetBlocked(xIdx, yIdx, zIdx, dir);
          }
        }
      }
    }
  }
}

void FlexDRWorker::initMazeCost_terms(const std::set<frBlockObject*>& objs,
                                      const bool isAddPathCost,
                                      const bool isSkipVia)
{
  for (auto& obj : objs) {
    if (obj->typeId() == frcBTerm) {
      auto term = static_cast<frBTerm*>(obj);
      for (auto& uPin : term->getPins()) {
        auto pin = uPin.get();
        for (auto& uPinFig : pin->getFigs()) {
          auto pinFig = uPinFig.get();
          if (pinFig->typeId() == frcRect) {
            auto rpinRect = static_cast<frRect*>(pinFig);
            const frLayerNum layerNum = rpinRect->getLayerNum();
            if (getTech()->getLayer(layerNum)->getType()
                != dbTechLayerType::ROUTING) {
              continue;
            }
            frMIdx zIdx;
            const frRect instPinRect(*rpinRect);
            const Rect box = instPinRect.getBBox();

            bool isRoutingLayer = true;
            if (getTech()->getLayer(layerNum)->getType()
                == dbTechLayerType::ROUTING) {
              isRoutingLayer = true;
              zIdx = gridGraph_.getMazeZIdx(layerNum);
            } else if (getTech()->getLayer(layerNum)->getType()
                       == dbTechLayerType::CUT) {
              isRoutingLayer = false;
              if (getTech()->getBottomLayerNum() <= layerNum - 1
                  && getTech()->getLayer(layerNum - 1)->getType()
                         == dbTechLayerType::ROUTING) {
                zIdx = gridGraph_.getMazeZIdx(layerNum - 1);
              } else {
                continue;
              }
            } else {
              continue;
            }

            const ModCostType type = isAddPathCost ? ModCostType::addFixedShape
                                                   : ModCostType::subFixedShape;

            if (isRoutingLayer) {
              modMinSpacingCostPlanar(box, zIdx, type);
              modMinSpacingCostVia(box, zIdx, type, true, false);
              modMinSpacingCostVia(box, zIdx, type, false, false);
              modEolSpacingRulesCost(box, zIdx, type);
            } else {
              modCutSpacingCost(box, zIdx, type);
              modInterLayerCutSpacingCost(box, zIdx, type, true);
              modInterLayerCutSpacingCost(box, zIdx, type, false);
            }
          } else {
            std::cout << "Error: initMazeCost_terms unsupported pinFig\n";
          }
        }
      }

    } else if (obj->typeId() == frcInstTerm) {
      auto instTerm = static_cast<frInstTerm*>(obj);
      auto inst = instTerm->getInst();
      const dbTransform xform = inst->getUpdatedXform();
      const dbTransform shiftXform(inst->getTransform().getOffset());
      const dbMasterType masterType = inst->getMaster()->getMasterType();
      bool accessHorz = false;
      bool accessVert = false;
      if (masterType.isBlock() && !isAddPathCost) {
        for (const auto& pin : instTerm->getTerm()->getPins()) {
          if (!pin->hasPinAccess()) {
            continue;
          }
          for (auto& ap :
               pin->getPinAccess(inst->getPinAccessIdx())->getAccessPoints()) {
            if (ap->hasAccess(frDirEnum::E) || ap->hasAccess(frDirEnum::W)) {
              accessHorz = true;
            }
            if (ap->hasAccess(frDirEnum::N) || ap->hasAccess(frDirEnum::S)) {
              accessVert = true;
            }
          }
        }
      } else {
        accessHorz = accessVert = true;
      }
      for (auto& uPin : instTerm->getTerm()->getPins()) {
        auto pin = uPin.get();
        for (auto& uPinFig : pin->getFigs()) {
          auto pinFig = uPinFig.get();
          if (pinFig->typeId() == frcRect) {
            auto rpinRect = static_cast<frRect*>(pinFig);
            const frLayerNum layerNum = rpinRect->getLayerNum();
            if (getTech()->getLayer(layerNum)->getType()
                != dbTechLayerType::ROUTING) {
              continue;
            }
            frMIdx zIdx;
            frRect instPinRect(*rpinRect);
            instPinRect.move(xform);
            const Rect box = instPinRect.getBBox();

            // add cost
            bool isRoutingLayer = true;
            if (getTech()->getLayer(layerNum)->getType()
                == dbTechLayerType::ROUTING) {
              isRoutingLayer = true;
              zIdx = gridGraph_.getMazeZIdx(layerNum);
            } else if (getTech()->getLayer(layerNum)->getType()
                       == dbTechLayerType::CUT) {
              isRoutingLayer = false;
              if (getTech()->getBottomLayerNum() <= layerNum - 1
                  && getTech()->getLayer(layerNum - 1)->getType()
                         == dbTechLayerType::ROUTING) {
                zIdx = gridGraph_.getMazeZIdx(layerNum - 1);
              } else {
                continue;
              }
            } else {
              continue;
            }

            ModCostType type = isAddPathCost ? ModCostType::addFixedShape
                                             : ModCostType::subFixedShape;

            if (isRoutingLayer) {
              if (!isSkipVia) {
                modMinSpacingCostVia(box, zIdx, type, true, false);
                modMinSpacingCostVia(box, zIdx, type, false, false);
              }
              if (masterType.isBlock()) {
                modCornerToCornerSpacing(
                    box, zIdx, type);  // temp solution for ISPD19 benchmarks
                modBlockedEdgesForMacroPin(instTerm, shiftXform, isAddPathCost);
                if (isAddPathCost) {
                  type = ModCostType::setFixedShape;
                } else {
                  type = ModCostType::resetFixedShape;
                }
              }
              modEolSpacingRulesCost(
                  box, zIdx, type, false, nullptr, accessHorz, accessVert);
              modMinSpacingCostPlanar(box,
                                      zIdx,
                                      type,
                                      false,
                                      nullptr,
                                      masterType.isBlock(),
                                      accessHorz,
                                      accessVert);
            } else {
              modCutSpacingCost(box, zIdx, type);
              modInterLayerCutSpacingCost(box, zIdx, type, true);
              modInterLayerCutSpacingCost(box, zIdx, type, false);
            }
            // temporary solution, only add cost around macro pins
            if ((masterType.isBlock() || masterType.isPad()
                 || masterType == dbMasterType::RING)
                && !isSkipVia) {
              modMinimumcutCostVia(box, zIdx, type, true);
              modMinimumcutCostVia(box, zIdx, type, false);
            }
          } else {
            std::cout << "Error: initMazeCost_terms unsupported pinFig\n";
          }
        }
      }
    } else {
      std::cout << "Error: unexpected obj type in initMazeCost_terms\n";
    }
  }
}

void FlexDRWorker::initMazeCost_planarTerm(const frDesign* design)
{
  frRegionQuery::Objects<frBlockObject> result;
  for (auto layerNum = getTech()->getBottomLayerNum();
       layerNum <= getTech()->getTopLayerNum();
       ++layerNum) {
    result.clear();
    const frLayer* layer = getTech()->getLayer(layerNum);
    if (layer->getType() != dbTechLayerType::ROUTING) {
      continue;
    }
    const frMIdx zIdx = gridGraph_.getMazeZIdx(layerNum);
    design->getRegionQuery()->query(getExtBox(), layerNum, result);
    for (auto& [box, obj] : result) {
      // term no bloat
      switch (obj->typeId()) {
        case frcBTerm: {
          FlexMazeIdx mIdx1, mIdx2;
          gridGraph_.getIdxBox(mIdx1, mIdx2, box);
          const bool isLayerHorz = layer->isHorizontal();
          for (int i = mIdx1.x(); i <= mIdx2.x(); i++) {
            for (int j = mIdx1.y(); j <= mIdx2.y(); j++) {
              FlexMazeIdx mIdx(i, j, zIdx);
              gridGraph_.setBlocked(i, j, zIdx, frDirEnum::U);
              gridGraph_.setBlocked(i, j, zIdx, frDirEnum::D);
              if (isLayerHorz) {
                gridGraph_.setBlocked(i, j, zIdx, frDirEnum::N);
                gridGraph_.setBlocked(i, j, zIdx, frDirEnum::S);
              } else {
                gridGraph_.setBlocked(i, j, zIdx, frDirEnum::W);
                gridGraph_.setBlocked(i, j, zIdx, frDirEnum::E);
              }
            }
          }
          break;
        }
        default:
          break;
      }
    }
  }
}

void FlexDRWorker::initMazeCost_connFig()
{
  for (auto& net : nets_) {
    for (auto& connFig : net->getExtConnFigs()) {
      addPathCost(connFig.get());
    }
    for (auto& connFig : net->getRouteConnFigs()) {
      addPathCost(connFig.get());
    }
    gcWorker_->updateDRNet(net.get());
    gcWorker_->updateGCWorker();
    modEolCosts_poly(gcWorker_->getNet(net->getFrNet()),
                     ModCostType::addRouteShape);
  }
}

void FlexDRWorker::initMazeCost_via_helper(drNet* net, bool isAddPathCost)
{
  std::unique_ptr<drVia> via = nullptr;
  for (auto& pin : net->getPins()) {
    if (pin->getFrTerm() == nullptr) {
      continue;
    }
    // MACRO pin does not prefer via access
    // bool macroPinViaBlock = false;
    auto dPinTerm = pin->getFrTerm();
    if (dPinTerm->typeId() == frcInstTerm) {
      frInstTerm* instTerm = static_cast<frInstTerm*>(dPinTerm);
      const dbMasterType masterType
          = instTerm->getInst()->getMaster()->getMasterType();
      if (masterType.isBlock() || masterType.isPad()
          || masterType == dbMasterType::RING) {
        continue;
      }
    }

    drAccessPattern* minCostAP = nullptr;
    for (auto& ap : pin->getAccessPatterns()) {
      if (ap->hasAccessViaDef(frDirEnum::U)) {
        if (minCostAP == nullptr) {
          minCostAP = ap.get();
        }
        if (ap->getPinCost() < minCostAP->getPinCost()) {
          minCostAP = ap.get();
          if (ap->getPinCost() == 0) {
            break;
          }
        }
      }
    }

    if (!minCostAP) {
      continue;
    }

    const Point bp = minCostAP->getPoint();
    frViaDef* viaDef = minCostAP->getAccessViaDef();
    via = std::make_unique<drVia>(viaDef);
    via->setOrigin(bp);
    via->addToNet(net);
    initMazeIdx_connFig(via.get());
    if (isAddPathCost) {
      addPathCost(via.get(), true);
    } else {
      subPathCost(via.get(), true);
    }
  }
}

// TODO: replace l1Box / l2Box calculation with via get bounding box function
void FlexDRWorker::initMazeCost_minCut_helper(drNet* net, bool isAddPathCost)
{
  const ModCostType modType
      = isAddPathCost ? ModCostType::addRouteShape : ModCostType::subRouteShape;
  for (auto& connFig : net->getExtConnFigs()) {
    if (connFig->typeId() == drcVia) {
      auto via = static_cast<drVia*>(connFig.get());
      const dbTransform xform = via->getTransform();

      const auto l1Num = via->getViaDef()->getLayer1Num();
      const auto l1Fig = (via->getViaDef()->getLayer1Figs()[0].get());
      Rect l1Box = l1Fig->getBBox();
      xform.apply(l1Box);
      modMinimumcutCostVia(l1Box, gridGraph_.getMazeZIdx(l1Num), modType, true);
      modMinimumcutCostVia(
          l1Box, gridGraph_.getMazeZIdx(l1Num), modType, false);

      const auto l2Num = via->getViaDef()->getLayer2Num();
      const auto l2Fig = (via->getViaDef()->getLayer2Figs()[0].get());
      Rect l2Box = l2Fig->getBBox();
      xform.apply(l2Box);
      modMinimumcutCostVia(l2Box, gridGraph_.getMazeZIdx(l2Num), modType, true);
      modMinimumcutCostVia(
          l2Box, gridGraph_.getMazeZIdx(l2Num), modType, false);
    }
  }
}

void FlexDRWorker::initMazeCost_boundary_helper(drNet* net, bool isAddPathCost)
{
  // do not check same-net rules between ext and route objs to avoid pessimism
  for (auto& connFig : net->getExtConnFigs()) {
    if (isAddPathCost) {
      addPathCost(connFig.get());
    } else {
      subPathCost(connFig.get());
    }
  }
}

void FlexDRWorker::initMarkers(const frDesign* design)
{
  std::vector<frMarker*> result;
  // get all markers within drc box
  design->getRegionQuery()->queryMarker(getDrcBox(), result);
  for (auto mptr : result) {
    // check recheck  if true  then markers.clear(), set drWorker bit to check
    // drc at start
    if (mptr->getConstraint()->typeId()
        != frConstraintTypeEnum::frcRecheckConstraint) {
      markers_.push_back(*mptr);
    } else {
      needRecheck_ = true;
    }
  }
  setInitNumMarkers(getNumMarkers());
  if (getDRIter() <= 1) {
    if (getNumMarkers() == 0) {
      setInitNumMarkers(1);
    }
  }
}

void FlexDRWorker::init(const frDesign* design)
{
  initNets(design);
  if (nets_.empty()
      && (getRipupMode() == RipUpMode::ALL
          || getRipupMode() == RipUpMode::INCR)) {
    skipRouting_ = true;
    return;
  }
  initGridGraph(design);
  initMazeIdx();
  std::unique_ptr<FlexGCWorker> gcWorker
      = std::make_unique<FlexGCWorker>(design->getTech(), logger_, this);
  gcWorker->setExtBox(getExtBox());
  gcWorker->setDrcBox(getDrcBox());
  gcWorker->init(design);
  gcWorker->setEnableSurgicalFix(true);
  setGCWorker(std::move(gcWorker));
  initMazeCost(design);
}

frCoord FlexDRWorker::snapCoordToManufacturingGrid(const frCoord coord,
                                                   const int lowerLeftCoord)
{
  const frCoord manuGrid = getTech()->getManufacturingGrid();
  frCoord onGridCoord = coord;
  if (coord % manuGrid != 0) {
    onGridCoord = manuGrid * std::floor(static_cast<float>(coord) / manuGrid);
    if (onGridCoord < lowerLeftCoord) {
      onGridCoord = manuGrid * std::ceil(static_cast<float>(coord) / manuGrid);
    }
  }

  return onGridCoord;
}

}  // namespace drt
