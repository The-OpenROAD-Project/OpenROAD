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

#include <queue>

#include "io/io.h"

using namespace std;
using namespace fr;
/* note: M1 guide special treatment. search "no M1 cross-gcell routing allowed"
 */

void io::Parser::genGuides_merge(
    vector<frRect>& rects,
    vector<map<frCoord, boost::icl::interval_set<frCoord>>>& intvs)
{
  for (auto& rect : rects) {
    frBox box;
    rect.getBBox(box);
    frPoint idx;
    frPoint pt(box.lowerLeft());
    design->getTopBlock()->getGCellIdx(pt, idx);
    frCoord x1 = idx.x();
    frCoord y1 = idx.y();
    pt.set(box.right() - 1, box.top() - 1);
    design->getTopBlock()->getGCellIdx(pt, idx);
    frCoord x2 = idx.x();
    frCoord y2 = idx.y();
    auto layerNum = rect.getLayerNum();
    if (tech->getLayer(layerNum)->getDir() == frcHorzPrefRoutingDir) {
      for (auto i = y1; i <= y2; i++) {
        intvs[layerNum][i].insert(
            boost::icl::interval<frCoord>::closed(x1, x2));
      }
    } else {
      for (auto i = x1; i <= x2; i++) {
        intvs[layerNum][i].insert(
            boost::icl::interval<frCoord>::closed(y1, y2));
      }
    }
  }
  // trackIdx, beginIdx, endIdx, layerNum
  vector<tuple<frCoord, frCoord, frCoord, frLayerNum>> touchGuides;
  // append touching edges
  for (int lNum = 0; lNum < (int) intvs.size(); lNum++) {
    auto& m = intvs[lNum];
    int prevTrackIdx = -2;
    for (auto& [trackIdx, intvS] : m) {
      if (trackIdx == prevTrackIdx + 1) {
        auto& prevIntvS = m[prevTrackIdx];
        auto newIntv = intvS & prevIntvS;
        for (auto it = newIntv.begin(); it != newIntv.end(); it++) {
          auto beginIdx = it->lower();
          auto endIdx = it->upper();
          bool haveLU = false;
          // lower layer intersection
          if (lNum - 2 >= 0) {
            auto nbrLayerNum = lNum - 2;
            for (auto it2 = intvs[nbrLayerNum].lower_bound(beginIdx);
                 it2 != intvs[nbrLayerNum].end() && it2->first <= endIdx;
                 it2++) {
              if (boost::icl::contains(it2->second, trackIdx)
                  && boost::icl::contains(it2->second, prevTrackIdx)) {
                haveLU = true;
                break;
              }
            }
          }
          if (lNum + 2 < (int) intvs.size() && !haveLU) {
            auto nbrLayerNum = lNum + 2;
            for (auto it2 = intvs[nbrLayerNum].lower_bound(beginIdx);
                 it2 != intvs[nbrLayerNum].end() && it2->first <= endIdx;
                 it2++) {
              if (boost::icl::contains(it2->second, trackIdx)
                  && boost::icl::contains(it2->second, prevTrackIdx)) {
                haveLU = true;
                break;
              }
            }
          }
          if (!haveLU) {
            // add touching guide;
            // cout <<"found touching guide" <<endl;
            if (lNum + 2 < (int) intvs.size()) {
              touchGuides.push_back(
                  make_tuple(beginIdx, prevTrackIdx, trackIdx, lNum + 2));
            } else if (lNum - 2 >= 0) {
              touchGuides.push_back(
                  make_tuple(beginIdx, prevTrackIdx, trackIdx, lNum - 2));
            } else {
              logger->error(
                  DRT, 228, "genGuides_merge cannot find touching layer");
            }
          }
        }
      }
      prevTrackIdx = trackIdx;
    }
  }

  for (auto& [trackIdx, beginIdx, endIdx, lNum] : touchGuides) {
    intvs[lNum][trackIdx].insert(
        boost::icl::interval<frCoord>::closed(beginIdx, endIdx));
  }
}

void io::Parser::genGuides_split(
    vector<frRect>& rects,
    vector<map<frCoord, boost::icl::interval_set<frCoord>>>& intvs,
    map<pair<frPoint, frLayerNum>, set<frBlockObject*, frBlockObjectComp>>&
        gCell2PinMap,
    map<frBlockObject*, set<pair<frPoint, frLayerNum>>, frBlockObjectComp>&
        pin2GCellMap,
    bool retry)
{
  rects.clear();
  // layerNum->trackIdx->beginIdx->set of obj
  vector<map<frCoord, map<frCoord, set<frBlockObject*, frBlockObjectComp>>>>
      pin_helper(design->getTech()->getLayers().size());
  for (auto& [pr, objS] : gCell2PinMap) {
    auto& point = pr.first;
    auto& lNum = pr.second;
    if (design->getTech()->getLayer(lNum)->getDir() == frcHorzPrefRoutingDir) {
      pin_helper[lNum][point.y()][point.x()] = objS;
    } else {
      pin_helper[lNum][point.x()][point.y()] = objS;
    }
  }

  for (int layerNum = 0; layerNum < (int) intvs.size(); layerNum++) {
    auto dir = design->getTech()->getLayer(layerNum)->getDir();
    for (auto& [trackIdx, curr_intvs] : intvs[layerNum]) {
      // split by lower/upper seg
      for (auto it = curr_intvs.begin(); it != curr_intvs.end(); it++) {
        set<frCoord> lineIdx;
        auto beginIdx = it->lower();
        auto endIdx = it->upper();
        // hardcode layerNum <= VIA_ACCESS_LAYERNUM not used for GR
        if (!retry && layerNum <= VIA_ACCESS_LAYERNUM) {
          // split by pin
          if (pin_helper[layerNum].find(trackIdx)
              != pin_helper[layerNum].end()) {
            auto& pin_helper_map = pin_helper[layerNum][trackIdx];
            for (auto it2 = pin_helper_map.lower_bound(beginIdx);
                 it2 != pin_helper_map.end() && it2->first <= endIdx;
                 it2++) {
              // add pin2GCellmap
              for (auto obj : it2->second) {
                if (dir == frcHorzPrefRoutingDir) {
                  pin2GCellMap[obj].insert(
                      make_pair(frPoint(it2->first, trackIdx), layerNum));
                } else {
                  pin2GCellMap[obj].insert(
                      make_pair(frPoint(trackIdx, it2->first), layerNum));
                }
              }
              // cout <<"pin split" <<endl;
            }
          }
          for (int x = beginIdx; x <= endIdx; x++) {
            frRect tmpRect;
            if (dir == frcHorzPrefRoutingDir) {
              tmpRect.setBBox(frBox(x, trackIdx, x, trackIdx));
            } else {
              tmpRect.setBBox(frBox(trackIdx, x, trackIdx, x));
            }
            tmpRect.setLayerNum(layerNum);
            rects.push_back(tmpRect);
          }
        } else {
          // lower layer intersection
          if (layerNum - 2 >= 0) {
            auto nbrLayerNum = layerNum - 2;
            for (auto it2 = intvs[nbrLayerNum].lower_bound(beginIdx);
                 it2 != intvs[nbrLayerNum].end() && it2->first <= endIdx;
                 it2++) {
              if (boost::icl::contains(it2->second, trackIdx)) {
                lineIdx.insert(
                    it2->first);  // it2->first is intersection frCoord
                // cout <<"found split point" <<endl;
              }
            }
          }
          if (layerNum + 2 < (int) intvs.size()) {
            auto nbrLayerNum = layerNum + 2;
            for (auto it2 = intvs[nbrLayerNum].lower_bound(beginIdx);
                 it2 != intvs[nbrLayerNum].end() && it2->first <= endIdx;
                 it2++) {
              if (boost::icl::contains(it2->second, trackIdx)) {
                lineIdx.insert(it2->first);
                // cout <<"found split point" <<endl;
              }
            }
          }
          // split by pin
          if (pin_helper[layerNum].find(trackIdx)
              != pin_helper[layerNum].end()) {
            auto& pin_helper_map = pin_helper[layerNum][trackIdx];
            for (auto it2 = pin_helper_map.lower_bound(beginIdx);
                 it2 != pin_helper_map.end() && it2->first <= endIdx;
                 it2++) {
              lineIdx.insert(it2->first);
              // add pin2GCellMap
              for (auto obj : it2->second) {
                if (dir == frcHorzPrefRoutingDir) {
                  pin2GCellMap[obj].insert(
                      make_pair(frPoint(it2->first, trackIdx), layerNum));
                } else {
                  pin2GCellMap[obj].insert(
                      make_pair(frPoint(trackIdx, it2->first), layerNum));
                }
              }
              // cout <<"pin split" <<endl;
            }
          }
          // add rect
          if (lineIdx.empty()) {
            logger->error(DRT,
                          229,
                          "genGuides_split lineIdx is empty on {}",
                          design->getTech()->getLayer(layerNum)->getName());
          } else if (lineIdx.size() == 1) {
            auto x = *(lineIdx.begin());
            frRect tmpRect;
            if (dir == frcHorzPrefRoutingDir) {
              tmpRect.setBBox(frBox(x, trackIdx, x, trackIdx));
            } else {
              tmpRect.setBBox(frBox(trackIdx, x, trackIdx, x));
            }
            tmpRect.setLayerNum(layerNum);
            rects.push_back(tmpRect);
          } else {
            auto prevIt = lineIdx.begin();
            for (auto currIt = (++(lineIdx.begin())); currIt != lineIdx.end();
                 currIt++) {
              frRect tmpRect;
              if (dir == frcHorzPrefRoutingDir) {
                tmpRect.setBBox(frBox(*prevIt, trackIdx, *currIt, trackIdx));
              } else {
                tmpRect.setBBox(frBox(trackIdx, *prevIt, trackIdx, *currIt));
              }
              tmpRect.setLayerNum(layerNum);
              prevIt = currIt;
              rects.push_back(tmpRect);
            }
          }
        }
      }
    }
  }
  rects.shrink_to_fit();
}

void io::Parser::genGuides_gCell2TermMap(
    map<pair<frPoint, frLayerNum>, set<frBlockObject*, frBlockObjectComp>>&
        gCell2PinMap,
    frTerm* term,
    frBlockObject* origTerm)
{
  for (auto& uPin : term->getPins()) {
    for (auto& uFig : uPin->getFigs()) {
      auto fig = uFig.get();
      if (fig->typeId() == frcRect) {
        auto shape = static_cast<frRect*>(fig);
        auto lNum = shape->getLayerNum();
        auto layer = design->getTech()->getLayer(lNum);
        frBox box;
        shape->getBBox(box);
        frPoint idx;
        frPoint pt(box.left() - 1, box.bottom() - 1);
        design->getTopBlock()->getGCellIdx(pt, idx);
        frCoord x1 = idx.x();
        frCoord y1 = idx.y();
        pt.set(box.upperRight());
        design->getTopBlock()->getGCellIdx(pt, idx);
        frCoord x2 = idx.x();
        frCoord y2 = idx.y();
        // ispd18_test4 and ispd18_test5 have zero overlap guide
        // excludes double-zero overlap area on the upper-right corner due to
        // initDR requirements
        bool condition2 = false;  // upper right corner has zero-length
                                  // overlapped with gcell
        frBox gcellBox;
        frPoint tmpIdx;
        design->getTopBlock()->getGCellIdx(box.lowerLeft(), tmpIdx);
        design->getTopBlock()->getGCellBox(tmpIdx, gcellBox);
        if (box.lowerLeft() == gcellBox.lowerLeft()) {
          condition2 = true;
        }

        bool condition3 = false;  // GR implies wrongway connection but
                                  // technology does not allow
        if ((layer->getDir() == frcVertPrefRoutingDir
             && layer->isUnidirectional() && box.left() == gcellBox.left())
            || (layer->getDir() == frcHorzPrefRoutingDir
                && layer->isUnidirectional()
                && box.bottom() == gcellBox.bottom())) {
          condition3 = true;
        }
        for (int x = x1; x <= x2; x++) {
          for (int y = y1; y <= y2; y++) {
            if (condition2 && x == tmpIdx.x() - 1 && y == tmpIdx.y() - 1) {
              if (VERBOSE > 0) {
                frString name = (origTerm->typeId() == frcInstTerm)
                                    ? ((frInstTerm*) origTerm)->getName()
                                    : term->getName();
                logger->warn(DRT,
                             230,
                             "genGuides_gCell2TermMap avoid condition2, may "
                             "result in guide open: {}",
                             name);
              }
            } else if (condition3
                       && ((x == tmpIdx.x() - 1
                            && layer->getDir() == frcVertPrefRoutingDir)
                           || (y == tmpIdx.y() - 1
                               && layer->getDir() == frcHorzPrefRoutingDir))) {
              if (VERBOSE > 0) {
                frString name = (origTerm->typeId() == frcInstTerm)
                                    ? ((frInstTerm*) origTerm)->getName()
                                    : term->getName();
                logger->warn(DRT,
                             231,
                             "genGuides_gCell2TermMap avoid condition3, may "
                             "result in guide open: {}",
                             name);
              }
            } else {
              gCell2PinMap[make_pair(frPoint(x, y), lNum)].insert(origTerm);
            }
          }
        }
      } else {
        logger->error(DRT, 232, "genGuides_gCell2TermMap unsupoprted pinfig");
      }
    }
  }
}

void io::Parser::genGuides_gCell2PinMap(
    frNet* net,
    map<pair<frPoint, frLayerNum>, set<frBlockObject*, frBlockObjectComp>>&
        gCell2PinMap)
{
  for (auto& instTerm : net->getInstTerms()) {
    frTransform xform;
    instTerm->getInst()->getUpdatedXform(xform);
    auto origTerm = instTerm->getTerm();
    auto uTerm = make_unique<frTerm>(*origTerm, xform);
    auto term = uTerm.get();
    if (DBPROCESSNODE == "GF14_13M_3Mx_2Cx_4Kx_2Hx_2Gx_LB") {
      if (!genGuides_gCell2APInstTermMap(gCell2PinMap, instTerm)) {
        genGuides_gCell2TermMap(gCell2PinMap, term, instTerm);
      }
    } else {
      genGuides_gCell2TermMap(gCell2PinMap, term, instTerm);
    }
  }
  for (auto& term : net->getTerms()) {
    if (DBPROCESSNODE == "GF14_13M_3Mx_2Cx_4Kx_2Hx_2Gx_LB") {
      if (!genGuides_gCell2APTermMap(gCell2PinMap, term)) {
        genGuides_gCell2TermMap(gCell2PinMap, term, term);
      }
    } else {
      genGuides_gCell2TermMap(gCell2PinMap, term, term);
    }
  }
}

bool io::Parser::genGuides_gCell2APInstTermMap(
    map<pair<frPoint, frLayerNum>, set<frBlockObject*, frBlockObjectComp>>&
        gCell2PinMap,
    frInstTerm* instTerm)
{
  bool isSuccess = false;

  if (!instTerm) {
    return isSuccess;
  }

  // ap
  frTransform shiftXform;
  frTransform xform;
  instTerm->getInst()->getUpdatedXform(xform);
  frTerm* trueTerm = instTerm->getTerm();
  string name;
  frInst* inst = instTerm->getInst();
  inst->getTransform(shiftXform);
  shiftXform.set(frOrient(frcR0));

  int pinIdx = 0;
  int pinAccessIdx = (inst) ? inst->getPinAccessIdx() : -1;
  int succesPinCnt = 0;
  for (auto& pin : trueTerm->getPins()) {
    frAccessPoint* prefAp = nullptr;
    if (inst) {
      prefAp = (instTerm->getAccessPoints())[pinIdx];
    }
    if (!pin->hasPinAccess()) {
      continue;
    }
    if (pinAccessIdx == -1) {
      continue;
    }

    if (!prefAp) {
      for (auto& ap : pin->getPinAccess(pinAccessIdx)->getAccessPoints()) {
        prefAp = ap.get();
        break;
      }
    }

    if (prefAp) {
      frPoint bp;
      prefAp->getPoint(bp);
      auto bNum = prefAp->getLayerNum();
      bp.transform(shiftXform);

      frPoint idx;
      design->getTopBlock()->getGCellIdx(bp, idx);
      gCell2PinMap[make_pair(idx, bNum)].insert(
          static_cast<frBlockObject*>(instTerm));
      succesPinCnt++;
      if (succesPinCnt == int(trueTerm->getPins().size())) {
        isSuccess = true;
      }
    }
    pinIdx++;
  }
  return isSuccess;
}

bool io::Parser::genGuides_gCell2APTermMap(
    map<pair<frPoint, frLayerNum>, set<frBlockObject*, frBlockObjectComp>>&
        gCell2PinMap,
    frTerm* term)
{
  bool isSuccess = false;

  if (!term) {
    return isSuccess;
  }

  // ap
  frTerm* trueTerm = term;

  int pinIdx = 0;
  int pinAccessIdx = 0;
  int succesPinCnt = 0;
  for (auto& pin : trueTerm->getPins()) {
    frAccessPoint* prefAp = nullptr;
    if (!pin->hasPinAccess()) {
      continue;
    }
    if (pinAccessIdx == -1) {
      continue;
    }

    if (!prefAp) {
      for (auto& ap : pin->getPinAccess(pinAccessIdx)->getAccessPoints()) {
        prefAp = ap.get();
        break;
      }
    }

    if (prefAp) {
      frPoint bp;
      prefAp->getPoint(bp);
      auto bNum = prefAp->getLayerNum();

      frPoint idx;
      design->getTopBlock()->getGCellIdx(bp, idx);
      gCell2PinMap[make_pair(idx, bNum)].insert(
          static_cast<frBlockObject*>(term));
      succesPinCnt++;
      if (succesPinCnt == int(trueTerm->getPins().size())) {
        isSuccess = true;
      }
    }
    pinIdx++;
  }
  return isSuccess;
}

void io::Parser::genGuides_initPin2GCellMap(
    frNet* net,
    std::map<frBlockObject*,
             std::set<std::pair<frPoint, frLayerNum>>,
             frBlockObjectComp>& pin2GCellMap)
{
  for (auto& instTerm : net->getInstTerms()) {
    pin2GCellMap[instTerm];
  }
  for (auto& term : net->getTerms()) {
    pin2GCellMap[term];
  }
}

void io::Parser::genGuides_addCoverGuide(frNet* net, vector<frRect>& rects)
{
  vector<frBlockObject*> terms;
  for (auto& instTerm : net->getInstTerms()) {
    terms.push_back(instTerm);
  }
  for (auto& term : net->getTerms()) {
    terms.push_back(term);
  }

  for (auto term : terms) {
    // ap
    frTransform instXform;  // (0,0), frcR0
    frTransform shiftXform;
    frTerm* trueTerm = nullptr;
    string name;
    frInst* inst = nullptr;
    if (term->typeId() == frcInstTerm) {
      inst = static_cast<frInstTerm*>(term)->getInst();
      inst->getTransform(shiftXform);
      shiftXform.set(frOrient(frcR0));
      inst->getUpdatedXform(instXform);
      trueTerm = static_cast<frInstTerm*>(term)->getTerm();
      name = inst->getName() + string("/") + trueTerm->getName();
    } else if (term->typeId() == frcTerm) {
      trueTerm = static_cast<frTerm*>(term);
      name = string("PIN/") + trueTerm->getName();
    }
    int pinIdx = 0;
    int pinAccessIdx = (inst) ? inst->getPinAccessIdx() : -1;
    for (auto& pin : trueTerm->getPins()) {
      frAccessPoint* prefAp = nullptr;
      if (inst) {
        prefAp = (static_cast<frInstTerm*>(term)->getAccessPoints())[pinIdx];
      }
      if (!pin->hasPinAccess()) {
        continue;
      }
      if (pinAccessIdx == -1) {
        continue;
      }

      if (!prefAp) {
        for (auto& ap : pin->getPinAccess(pinAccessIdx)->getAccessPoints()) {
          prefAp = ap.get();
          break;
        }
      }

      if (prefAp) {
        frPoint bp;
        prefAp->getPoint(bp);
        auto bNum = prefAp->getLayerNum();
        bp.transform(shiftXform);

        frPoint idx;
        frBox llBox, urBox;
        design->getTopBlock()->getGCellIdx(bp, idx);
        design->getTopBlock()->getGCellBox(frPoint(idx.x() - 1, idx.y() - 1),
                                           llBox);
        design->getTopBlock()->getGCellBox(frPoint(idx.x() + 1, idx.y() + 1),
                                           urBox);
        frBox coverBox(
            llBox.left(), llBox.bottom(), urBox.right(), urBox.top());
        frLayerNum beginLayerNum, endLayerNum;
        beginLayerNum = bNum;
        endLayerNum = min(bNum + 4, design->getTech()->getTopLayerNum());

        for (auto lNum = beginLayerNum; lNum <= endLayerNum; lNum += 2) {
          for (int xIdx = -1; xIdx <= 1; xIdx++) {
            for (int yIdx = -1; yIdx <= 1; yIdx++) {
              frRect coverGuideRect;
              coverGuideRect.setBBox(coverBox);
              coverGuideRect.setLayerNum(lNum);
              rects.push_back(coverGuideRect);
            }
          }
        }
      }
      pinIdx++;
    }
  }
}

void io::Parser::genGuides(frNet* net, vector<frRect>& rects)
{
  // cout <<"net " <<net->getName() <<endl <<flush;
  net->clearGuides();
  vector<map<frCoord, boost::icl::interval_set<frCoord>>> intvs(
      tech->getLayers().size());
  if (DBPROCESSNODE == "GF14_13M_3Mx_2Cx_4Kx_2Hx_2Gx_LB") {
    genGuides_addCoverGuide(net, rects);
  }
  genGuides_merge(rects, intvs);  // merge and add touching guide

  // gcell to pin map
  map<pair<frPoint, frLayerNum>, set<frBlockObject*, frBlockObjectComp>>
      gCell2PinMap;
  map<frBlockObject*, set<pair<frPoint, frLayerNum>>, frBlockObjectComp>
      pin2GCellMap;
  genGuides_gCell2PinMap(net, gCell2PinMap);
  genGuides_initPin2GCellMap(net, pin2GCellMap);

  bool retry = false;
  while (1) {
    genGuides_split(rects,
                    intvs,
                    gCell2PinMap,
                    pin2GCellMap,
                    retry);  // split on LU intersecting guides and pins

    // filter pin2GCellMap with aps

    if (pin2GCellMap.empty()) {
      logger->warn(DRT, 214, "genGuides empty pin2GCellMap");
      debugPrint(
          logger, DRT, "io", 1, "gcell2pin.size() = {}", gCell2PinMap.size());
    }
    for (auto& [obj, locS] : pin2GCellMap) {
      if (locS.empty()) {
        if (obj->typeId() == frcInstTerm) {
          auto ptr = static_cast<frInstTerm*>(obj);
          logger->warn(DRT,
                       215,
                       "pin {}/{} not covered by guide",
                       ptr->getInst()->getName(),
                       ptr->getTerm()->getName());
        } else if (obj->typeId() == frcTerm) {
          auto ptr = static_cast<frTerm*>(obj);
          logger->warn(
              DRT, 216, "pin PIN/{} not covered by guide", ptr->getName());
        } else {
          logger->warn(DRT, 217, "genGuides unknown type");
        }
      }
    }

    // steiner (i.e., gcell end and pin gcell idx) to guide idx (pin idx)
    map<pair<frPoint, frLayerNum>, set<int>> nodeMap;
    int gCnt = 0;
    int nCnt = 0;
    genGuides_buildNodeMap(nodeMap, gCnt, nCnt, rects, pin2GCellMap);
    // cout <<"build node map done" <<endl <<flush;

    vector<bool> adjVisited;
    vector<int> adjPrevIdx;
    if (genGuides_astar(
            net, adjVisited, adjPrevIdx, nodeMap, gCnt, nCnt, false, retry)) {
      // cout <<"astar done" <<endl <<flush;
      genGuides_final(
          net, rects, adjVisited, adjPrevIdx, gCnt, nCnt, pin2GCellMap);
      break;
    } else {
      if (retry) {
        if (!ALLOW_PIN_AS_FEEDTHROUGH) {
          if (genGuides_astar(net,
                              adjVisited,
                              adjPrevIdx,
                              nodeMap,
                              gCnt,
                              nCnt,
                              true,
                              retry)) {
            genGuides_final(
                net, rects, adjVisited, adjPrevIdx, gCnt, nCnt, pin2GCellMap);
            break;
          } else {
            logger->error(DRT, 218, "Guide is not connected to design");
          }
        } else {
          logger->error(DRT, 219, "Guide is not connected to design");
        }
      } else {
        retry = true;
      }
    }
  }
}

void io::Parser::genGuides_final(
    frNet* net,
    vector<frRect>& rects,
    vector<bool>& adjVisited,
    vector<int>& adjPrevIdx,
    int gCnt,
    int nCnt,
    map<frBlockObject*, set<pair<frPoint, frLayerNum>>, frBlockObjectComp>&
        pin2GCellMap)
{
  vector<frBlockObject*> pin2ptr;
  for (auto& [obj, idxS] : pin2GCellMap) {
    pin2ptr.push_back(obj);
  }
  // find pin in which guide
  vector<vector<pair<frPoint, frLayerNum>>> pinIdx2GCellUpdated(nCnt - gCnt);
  vector<vector<int>> guideIdx2Pins(gCnt);
  for (int i = 0; i < (int) adjPrevIdx.size(); i++) {
    if (!adjVisited[i]) {
      continue;
    }
    if (i < gCnt && adjPrevIdx[i] >= gCnt) {
      auto pinIdx = adjPrevIdx[i] - gCnt;
      auto guideIdx = i;
      frBox box;
      auto& rect = rects[guideIdx];
      rect.getBBox(box);
      auto lNum = rect.getLayerNum();
      auto obj = pin2ptr[pinIdx];
      // cout <<" pin1 id " <<adjPrevIdx[i] <<" prev " <<i <<endl;
      if (pin2GCellMap[obj].find(make_pair(box.lowerLeft(), lNum))
          != pin2GCellMap[obj].end()) {
        pinIdx2GCellUpdated[pinIdx].push_back(make_pair(box.lowerLeft(), lNum));
      } else if (pin2GCellMap[obj].find(make_pair(box.upperRight(), lNum))
                 != pin2GCellMap[obj].end()) {
        pinIdx2GCellUpdated[pinIdx].push_back(
            make_pair(box.upperRight(), lNum));
      } else {
        logger->warn(DRT, 220, "genGuides_final error 1");
      }
      guideIdx2Pins[guideIdx].push_back(pinIdx);
    } else if (i >= gCnt && adjPrevIdx[i] >= 0 && adjPrevIdx[i] < gCnt) {
      auto pinIdx = i - gCnt;
      auto guideIdx = adjPrevIdx[i];
      frBox box;
      auto& rect = rects[guideIdx];
      rect.getBBox(box);
      auto lNum = rect.getLayerNum();
      auto obj = pin2ptr[pinIdx];
      // cout <<" pin2 id " <<i <<" prev " <<adjPrevIdx[i] <<endl;
      if (pin2GCellMap[obj].find(make_pair(box.lowerLeft(), lNum))
          != pin2GCellMap[obj].end()) {
        pinIdx2GCellUpdated[pinIdx].push_back(make_pair(box.lowerLeft(), lNum));
      } else if (pin2GCellMap[obj].find(make_pair(box.upperRight(), lNum))
                 != pin2GCellMap[obj].end()) {
        pinIdx2GCellUpdated[pinIdx].push_back(
            make_pair(box.upperRight(), lNum));
      } else {
        logger->warn(DRT, 221, "genGuides_final error 2");
      }
      guideIdx2Pins[guideIdx].push_back(pinIdx);
    }
  }
  for (auto& guides : pinIdx2GCellUpdated) {
    if (guides.empty()) {
      logger->warn(DRT, 222, "genGuides_final pin not in any guide");
    }
  }

  map<pair<frPoint, frLayerNum>, set<int>> updatedNodeMap;
  // pinIdx2GCellUpdated tells pin residency in gcell
  for (int i = 0; i < nCnt - gCnt; i++) {
    auto obj = pin2ptr[i];
    for (auto& [pt, lNum] : pinIdx2GCellUpdated[i]) {
      frPoint absPt;
      design->getTopBlock()->getGCellCenter(pt, absPt);
      tmpGRPins.push_back(make_pair(obj, absPt));
      updatedNodeMap[make_pair(pt, lNum)].insert(i + gCnt);
    }
  }
  for (int i = 0; i < gCnt; i++) {
    if (!adjVisited[i]) {
      continue;
    }
    auto& rect = rects[i];
    frBox box;
    rect.getBBox(box);
    updatedNodeMap[make_pair(frPoint(box.left(), box.bottom()),
                             rect.getLayerNum())]
        .insert(i);
    updatedNodeMap[make_pair(frPoint(box.right(), box.top()),
                             rect.getLayerNum())]
        .insert(i);
    // cout <<"add guide " <<i <<" to " <<frPoint(box.left(),  box.bottom()) <<"
    // " <<rect.getLayerNum() <<endl; cout <<"add guide " <<i <<" to "
    // <<frPoint(box.right(), box.top())    <<" " <<rect.getLayerNum() <<endl;
  }
  for (auto& [pr, idxS] : updatedNodeMap) {
    auto& [pt, lNum] = pr;
    if ((int) idxS.size() == 1) {
      auto idx = *(idxS.begin());
      if (idx < gCnt) {
        // no upper/lower guide
        if (updatedNodeMap.find(make_pair(pt, lNum + 2)) == updatedNodeMap.end()
            && updatedNodeMap.find(make_pair(pt, lNum - 2))
                   == updatedNodeMap.end()) {
          auto& rect = rects[idx];
          frBox box;
          rect.getBBox(box);
          if (box.lowerLeft() == pt) {
            rect.setBBox(frBox(box.right(), box.top(), box.right(), box.top()));
          } else {
            rect.setBBox(
                frBox(box.left(), box.bottom(), box.left(), box.bottom()));
          }
        }
      } else {
        logger->error(DRT,
                      223,
                      "pin dangling id {} ({},{}) {}",
                      idx,
                      pt.x(),
                      pt.y(),
                      lNum);
      }
    }
  }
  // guideIdx2Pins enables fiding from guide to pin
  // adjVisited tells guide to write back
  for (int i = 0; i < gCnt; i++) {
    if (!adjVisited[i]) {
      continue;
    }
    auto& rect = rects[i];
    frBox box;
    rect.getBBox(box);
    auto guide = make_unique<frGuide>();
    frPoint begin, end;
    design->getTopBlock()->getGCellCenter(box.lowerLeft(), begin);
    design->getTopBlock()->getGCellCenter(box.upperRight(), end);
    guide->setPoints(begin, end);
    guide->setBeginLayerNum(rect.getLayerNum());
    guide->setEndLayerNum(rect.getLayerNum());
    guide->addToNet(net);
    net->addGuide(std::move(guide));
    // cout <<"add guide " <<begin <<" " <<end <<" " <<rect.getLayerNum()
    // <<endl;
  }
}

void io::Parser::genGuides_buildNodeMap(
    map<pair<frPoint, frLayerNum>, set<int>>& nodeMap,
    int& gCnt,
    int& nCnt,
    vector<frRect>& rects,
    map<frBlockObject*, set<pair<frPoint, frLayerNum>>, frBlockObjectComp>&
        pin2GCellMap)
{
  for (int i = 0; i < (int) rects.size(); i++) {
    auto& rect = rects[i];
    frBox box;
    rect.getBBox(box);
    nodeMap[make_pair(box.lowerLeft(), rect.getLayerNum())].insert(i);
    nodeMap[make_pair(box.upperRight(), rect.getLayerNum())].insert(i);
  }
  gCnt = rects.size();  // total guide cnt
  int nodeIdx = rects.size();
  for (auto& [obj, locS] : pin2GCellMap) {
    for (auto& loc : locS) {
      nodeMap[loc].insert(nodeIdx);
    }
    nodeIdx++;
  }
  nCnt = nodeIdx;  // total node cnt
}

bool io::Parser::genGuides_astar(
    frNet* net,
    vector<bool>& adjVisited,
    vector<int>& adjPrevIdx,
    map<pair<frPoint, frLayerNum>, set<int>>& nodeMap,
    int& gCnt,
    int& nCnt,
    bool forceFeedThrough,
    bool retry)
{
  // a star search

  // node index, node visited
  vector<vector<int>> adjVec(nCnt, vector<int>());
  vector<bool> onPathIdx(nCnt, false);
  adjVisited.clear();
  adjPrevIdx.clear();
  adjVisited.resize(nCnt, false);
  adjPrevIdx.resize(nCnt, -1);
  for (auto& [pr, idxS] : nodeMap) {
    auto& [pt, lNum] = pr;
    for (auto it1 = idxS.begin(); it1 != idxS.end(); it1++) {
      auto it2 = it1;
      it2++;
      auto idx1 = *it1;
      for (; it2 != idxS.end(); it2++) {
        auto idx2 = *it2;
        // two pins, no edge
        if (idx1 >= gCnt && idx2 >= gCnt) {
          continue;
          // two gcells, has edge
        } else if (idx1 < gCnt && idx2 < gCnt) {
          // no M1 cross-gcell routing allowed
          // BX200307: in general VIA_ACCESS_LAYER should not be used (instead
          // of 0)
          if (lNum != VIA_ACCESS_LAYERNUM) {
            adjVec[idx1].push_back(idx2);
            adjVec[idx2].push_back(idx1);
          }
          // cout <<"add edge1 " <<idx1 <<" " <<idx2 <<endl;
          // one pin, one gcell
        } else {
          auto gIdx = min(idx1, idx2);
          auto pIdx = max(idx1, idx2);
          // only out edge
          if (ALLOW_PIN_AS_FEEDTHROUGH || forceFeedThrough) {
            adjVec[pIdx].push_back(gIdx);
            adjVec[gIdx].push_back(pIdx);
          } else {
            if (pIdx == gCnt) {
              adjVec[pIdx].push_back(gIdx);
              // cout <<"add edge2 " <<pIdx <<" " <<gIdx <<endl;
              // only in edge
            } else {
              adjVec[gIdx].push_back(pIdx);
              // cout <<"add edge3 " <<gIdx <<" " <<pIdx <<endl;
            }
          }
        }
      }
      // add intersecting guide2guide edge excludes pin
      if (idx1 < gCnt
          && nodeMap.find(make_pair(pt, lNum + 2)) != nodeMap.end()) {
        for (auto nbrIdx : nodeMap[make_pair(pt, lNum + 2)]) {
          if (nbrIdx < gCnt) {
            adjVec[idx1].push_back(nbrIdx);
            adjVec[nbrIdx].push_back(idx1);
            // cout <<"add edge4 " <<idx1 <<" " <<nbrIdx <<endl;
            // cout <<"add edge5 " <<nbrIdx <<" " <<idx1 <<endl;
          }
        }
      }
    }
  }

  struct wf
  {
    int nodeIdx;
    int prevIdx;
    int cost;
    bool operator<(const wf& b) const
    {
      if (cost == b.cost) {
        return nodeIdx > b.nodeIdx;
      } else {
        return cost > b.cost;
      }
    }
  };
  for (int findNode = gCnt; findNode < nCnt - 1; findNode++) {
    // cout <<"finished " <<findNode <<" nodes" <<endl;
    priority_queue<wf> pq;
    if (findNode == gCnt) {
      // push only first pin into pq
      pq.push({gCnt, -1, 0});
    } else {
      // push every visited node into pq
      for (int i = 0; i < nCnt; i++) {
        if (onPathIdx[i]) {
          // penalize feedthrough in normal mode
          if (ALLOW_PIN_AS_FEEDTHROUGH && i >= gCnt) {
            pq.push({i, adjPrevIdx[i], 2});
            // penalize feedthrough in fallback mode
          } else if (forceFeedThrough && i >= gCnt) {
            pq.push({i, adjPrevIdx[i], 10});
          } else {
            pq.push({i, adjPrevIdx[i], 0});
          }
        }
      }
    }
    int lastNodeIdx = -1;
    while (!pq.empty()) {
      auto wfront = pq.top();
      pq.pop();
      if (!onPathIdx[wfront.nodeIdx] && adjVisited[wfront.nodeIdx]) {
        continue;
      }
      if (wfront.nodeIdx > gCnt && wfront.nodeIdx < nCnt
          && adjVisited[wfront.nodeIdx] == false) {
        adjVisited[wfront.nodeIdx] = true;
        adjPrevIdx[wfront.nodeIdx] = wfront.prevIdx;
        lastNodeIdx = wfront.nodeIdx;
        break;
      }
      adjVisited[wfront.nodeIdx] = true;
      adjPrevIdx[wfront.nodeIdx] = wfront.prevIdx;
      // visit other nodes
      for (auto nbrIdx : adjVec[wfront.nodeIdx]) {
        if (!adjVisited[nbrIdx]) {
          pq.push({nbrIdx, wfront.nodeIdx, wfront.cost + 1});
        }
      }
    }
    // trace back path
    while ((lastNodeIdx != -1) && (!onPathIdx[lastNodeIdx])) {
      onPathIdx[lastNodeIdx] = true;
      lastNodeIdx = adjPrevIdx[lastNodeIdx];
    }
    adjVisited = onPathIdx;
  }
  // skip one-pin net
  if (nCnt == gCnt + 1) {
    return true;
  }
  int pinVisited = count(adjVisited.begin() + gCnt, adjVisited.end(), true);
  // true error when allowing feedthrough
  if (pinVisited != nCnt - gCnt
      && (ALLOW_PIN_AS_FEEDTHROUGH || forceFeedThrough) && retry) {
    logger->warn(DRT,
                 224,
                 "{} {} pin not visited #guides = {}",
                 net->getName(),
                 nCnt - gCnt - pinVisited,
                 gCnt);
  }
  // fallback to feedthrough in next iter
  if (pinVisited != nCnt - gCnt && !ALLOW_PIN_AS_FEEDTHROUGH
      && !forceFeedThrough && retry) {
    logger->warn(DRT,
                 225,
                 "{} {} pin not visited, fall back to feedrough mode",
                 net->getName(),
                 nCnt - gCnt - pinVisited);
  }
  if (pinVisited == nCnt - gCnt) {
    return true;
  } else {
    return false;
  }
}
