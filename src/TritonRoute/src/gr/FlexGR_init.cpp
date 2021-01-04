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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE REGENTS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <iostream>
#include "FlexGR.h"
#include "FlexGRCMap.h"
#include <deque>

using namespace std;
using namespace fr;

void FlexGR::init() {
  initGCell();
  initLayerPitch();
  initCMap();
}

void FlexGR::initLayerPitch() {
  int numRoutingLayer = 0;
  for (unsigned lNum = 0; lNum < design->getTech()->getLayers().size(); lNum++) {
    if (design->getTech()->getLayer(lNum)->getType() != frLayerTypeEnum::ROUTING) {
      continue;
    }
    numRoutingLayer++;
  }

  // init pitches
  trackPitches.resize(numRoutingLayer, -1);
  line2ViaPitches.resize(numRoutingLayer, -1);
  layerPitches.resize(numRoutingLayer, -1);
  zHeights.resize(numRoutingLayer, 0);


  // compute pitches
  for (int lNum = 0; lNum < (int)design->getTech()->getLayers().size(); lNum++) {
    if (design->getTech()->getLayer(lNum)->getType() != frLayerTypeEnum::ROUTING) {
      continue;
    }
    // zIdx  always equal to lNum / 2 - 1
    int zIdx = lNum / 2 - 1;
    auto layer = design->getTech()->getLayer(lNum);
    bool isLayerHorz = (layer->getDir() == frPrefRoutingDirEnum::frcHorzPrefRoutingDir);
    // get track pitch
    for (auto &tp: design->getTopBlock()->getTrackPatterns(lNum)) {
      if ((isLayerHorz && !tp->isHorizontal()) ||
          (!isLayerHorz && tp->isHorizontal())) {
        if (trackPitches[zIdx] == -1 || (int) tp->getTrackSpacing() < trackPitches[zIdx]) {
          trackPitches[zIdx] = tp->getTrackSpacing();
        }
      }
    }

    // calculate line-2-via pitch
    frViaDef* downVia = nullptr;
    frViaDef* upVia = nullptr;
    if (getDesign()->getTech()->getBottomLayerNum() <= lNum - 1) {
      downVia = getDesign()->getTech()->getLayer(lNum - 1)->getDefaultViaDef();
    }
    if (getDesign()->getTech()->getTopLayerNum() >= lNum + 1) {
      upVia = getDesign()->getTech()->getLayer(lNum + 1)->getDefaultViaDef();
    }

    frCoord defaultWidth = layer->getWidth();
    frCoord line2ViaPitchDown = -1, line2ViaPitchUp = -1;
    frCoord minLine2ViaPitch = -1;

    if (downVia) {
      frBox viaBox;
      frVia via(downVia);
      via.getLayer2BBox(viaBox);
      frCoord enclosureWidth = viaBox.width();
      frCoord prl = isLayerHorz ? (viaBox.right() - viaBox.left()) : (viaBox.top() - viaBox.bottom());
      // calculate minNonOvlpDist
      frCoord minNonOvlpDist = defaultWidth / 2;
      if (isLayerHorz) {
        minNonOvlpDist += (viaBox.top() - viaBox.bottom()) / 2;
      } else {
        minNonOvlpDist += (viaBox.right() - viaBox.left()) / 2;
      }
      // calculate minSpc required val
      frCoord minReqDist = INT_MIN;

      auto con = layer->getMinSpacing();
      if (con) {
        if (con->typeId() == frConstraintTypeEnum::frcSpacingConstraint) {
          minReqDist = static_cast<frSpacingConstraint*>(con)->getMinSpacing();
        } else if (con->typeId() == frConstraintTypeEnum::frcSpacingTablePrlConstraint) {
          minReqDist = static_cast<frSpacingTablePrlConstraint*>(con)->find(max(enclosureWidth, defaultWidth), prl);
        } else if (con->typeId() == frConstraintTypeEnum::frcSpacingTableTwConstraint) {
          minReqDist = static_cast<frSpacingTableTwConstraint*>(con)->find(enclosureWidth, defaultWidth, prl);
        }
      }
      if (minReqDist != INT_MIN) {
        minReqDist += minNonOvlpDist;
        // cout << layer->getName() << " line-2-via (down) pitch = " << minReqDist / (double)(design->getTopBlock()->getDBUPerUU()) << endl;
        line2ViaPitchDown = minReqDist;
        if (line2ViaPitches[zIdx] == -1 || minReqDist > line2ViaPitches[zIdx]) {
          line2ViaPitches[zIdx] = minReqDist;
        }
        if (minLine2ViaPitch == -1 || minReqDist < minLine2ViaPitch) {
          minLine2ViaPitch = minReqDist;
        }
      }
    }

    if (upVia) {
      frBox viaBox;
      frVia via(upVia);
      via.getLayer1BBox(viaBox);
      frCoord enclosureWidth = viaBox.width();
      frCoord prl = isLayerHorz ? (viaBox.right() - viaBox.left()) : (viaBox.top() - viaBox.bottom());
      // calculate minNonOvlpDist
      frCoord minNonOvlpDist = defaultWidth / 2;
      if (isLayerHorz) {
        minNonOvlpDist += (viaBox.top() - viaBox.bottom()) / 2;
      } else {
        minNonOvlpDist += (viaBox.right() - viaBox.left()) / 2;
      }
      // calculate minSpc required val
      frCoord minReqDist = INT_MIN;

      auto con = layer->getMinSpacing();
      if (con) {
        if (con->typeId() == frConstraintTypeEnum::frcSpacingConstraint) {
          minReqDist = static_cast<frSpacingConstraint*>(con)->getMinSpacing();
        } else if (con->typeId() == frConstraintTypeEnum::frcSpacingTablePrlConstraint) {
          minReqDist = static_cast<frSpacingTablePrlConstraint*>(con)->find(max(enclosureWidth, defaultWidth), prl);
        } else if (con->typeId() == frConstraintTypeEnum::frcSpacingTableTwConstraint) {
          minReqDist = static_cast<frSpacingTableTwConstraint*>(con)->find(enclosureWidth, defaultWidth, prl);
        }
      }
      if (minReqDist != INT_MIN) {
        minReqDist += minNonOvlpDist;
        // cout << layer->getName() << " line-2-via (up) pitch = " << minReqDist / (double)(design->getTopBlock()->getDBUPerUU()) << endl;
        line2ViaPitchUp = minReqDist;
        if (line2ViaPitches[zIdx] == -1 || minReqDist > line2ViaPitches[zIdx]) {
          line2ViaPitches[zIdx] = minReqDist;
        }
        if (minLine2ViaPitch == -1 || minReqDist < minLine2ViaPitch) {
          minLine2ViaPitch = minReqDist;
        }
      }
    }

    
    if (minLine2ViaPitch > trackPitches[zIdx]) {
      layerPitches[zIdx] = max(line2ViaPitchDown, line2ViaPitchUp);
    } else {
      layerPitches[zIdx] = trackPitches[zIdx];
    }

    // output
    cout << layer->getName();
    if (isLayerHorz) {
      cout << " H ";
    } else {
      cout << " V ";
    }
    cout << "Track-Pitch = " << fixed << setprecision(5) << trackPitches[zIdx] / (double)(design->getTopBlock()->getDBUPerUU()) 
         << "  line-2-Via Pitch = " << fixed << setprecision(5) << minLine2ViaPitch / (double)(design->getTopBlock()->getDBUPerUU()) << endl;
    if (trackPitches[zIdx] < minLine2ViaPitch) {
      cout << "Warning: Track pitch is too small compared with line-2-via pitch\n";
    }
    // cout << "  layer pitch = " << fixed << setprecision(5) << layerPitches[zIdx] / (double)(design->getTopBlock()->getDBUPerUU()) << endl;
    // layerPitches[zIdx] = max(trackPitches[zIdx], line2ViaPitches[zIdx]);

    if (zIdx == 0) {
      zHeights[zIdx] = layerPitches[zIdx];
    } else {
      zHeights[zIdx] = zHeights[zIdx - 1] + layerPitches[zIdx];
    }

  }
}

void FlexGR::initGCell() {
  auto &gcellPatterns = design->getTopBlock()->getGCellPatterns();
  if (gcellPatterns.empty()) {
    auto layer = design->getTech()->getLayer(2);
    auto pitch = layer->getPitch();
    cout << endl << "Generating GCell with size = 15 tracks, using layer " << layer->getName()
         << " pitch  = " << pitch / (double)(design->getTopBlock()->getDBUPerUU()) << "\n";
    frBox dieBox;
    design->getTopBlock()->getBoundaryBBox(dieBox);

    frGCellPattern xgp, ygp;
    // set xgp
    xgp.setHorizontal(false);
    frCoord startCoordX = dieBox.left() - design->getTech()->getManufacturingGrid();
    // frCoord startCoordX = dieBox.left();
    xgp.setStartCoord(startCoordX);
    frCoord GCELLGRIDX = pitch * 15;
    xgp.setSpacing(GCELLGRIDX);
    xgp.setCount((dieBox.right() - startCoordX) / GCELLGRIDX);
    // set ygp
    ygp.setHorizontal(true);
    frCoord startCoordY = dieBox.bottom() - design->getTech()->getManufacturingGrid();
    // frCoord startCoordY = dieBox.bottom();
    ygp.setStartCoord(startCoordY);
    frCoord GCELLGRIDY = pitch * 15;
    ygp.setSpacing(GCELLGRIDY);
    ygp.setCount((dieBox.top() - startCoordY) / GCELLGRIDY);

    design->getTopBlock()->setGCellPatterns({xgp, ygp});
  }
}

void FlexGR::initCMap() {
  cout << endl << "initializing congestion map...\n";
  auto cmap = make_unique<FlexGRCMap>(design);
  cmap->setLayerTrackPitches(trackPitches);
  cmap->setLayerLine2ViaPitches(line2ViaPitches);
  cmap->setLayerPitches(layerPitches);
  cmap->init();
  auto cmap2D = make_unique<FlexGRCMap>(design);
  cmap2D->initFrom3D(cmap.get());
  // cmap->print2D(true);
  // cmap->print();
  setCMap(cmap);
  setCMap2D(cmap2D);
}

// FlexGRWorker related

void FlexGRWorker::initBoundary() {
  bool enableOutput = false;
  if (enableOutput) {
    stringstream ss;
    ss <<endl <<"start initBoundary GR worker (BOX) "
                <<"( " <<extBox.left()   * 1.0 / getTech()->getDBUPerUU() <<" "
                <<extBox.bottom() * 1.0 / getTech()->getDBUPerUU() <<" ) ( "
                <<extBox.right()  * 1.0 / getTech()->getDBUPerUU() <<" "
                <<extBox.top()    * 1.0 / getTech()->getDBUPerUU() <<" )" <<endl;
    cout <<ss.str() <<flush;
  }

  vector<grBlockObject*> result;
  getRegionQuery()->queryGRObj(extBox, result);
  for (auto rptr: result) {
    if (rptr->typeId() == grcPathSeg) {
      auto cptr = static_cast<grPathSeg*>(rptr);
      if (cptr->hasNet()) {
        initBoundary_splitPathSeg(cptr);
      } else {
        cout << "Error: initNetObjs hasNet() empty" <<endl;
      }
    }
  }
}

// split pathSeg at boudnary
void FlexGRWorker::initBoundary_splitPathSeg(grPathSeg* pathSeg) {
  bool enableOutput = false;
  frPoint bp, ep;
  pathSeg->getPoints(bp, ep);
  // skip if both endpoints are inside extBox (i.e., no intersection)
  if (extBox.contains(bp) && extBox.contains(ep)) {
    return;
  }
  frPoint breakPt1, breakPt2;
  initBoundary_splitPathSeg_getBreakPts(bp, ep, breakPt1, breakPt2);
  auto parent = pathSeg->getParent();
  auto child = pathSeg->getChild();



  if (breakPt1 == breakPt2) {
    if (enableOutput) {
      cout << "  @@@ before split " << parent->getNet()->getName() << " has " << parent->getNet()->getNodes().size() << " nodes"
           << " and " << parent->getNet()->getGRShapes().size() << " wires and " << parent->getNet()->getGRVias().size() << " vias\n";
    }
    // break on one side
    auto breakNode = initBoundary_splitPathSeg_split(child, parent, breakPt1);
    if (enableOutput) {
      cout << "  " << static_cast<frNet*>(parent->getNet())->getName() << " breaking at ("
           << breakPt1.x() << ", " << breakPt1.y() << ") on layerNum " << breakNode->getLayerNum() << "\n";
      // cout << "    parent at (" << parent->getLoc().x() << ", " << parent->getLoc().y() << ")\n";
      // for (auto node: parent->getChildren()) {
      //   cout << "    parent's child at (" << node->getLoc().x() << ", " << node->getLoc().y() << ")\n";
      // }
    }
    if (enableOutput) {
      cout << "  after split " << parent->getNet()->getName() << " has " << parent->getNet()->getNodes().size() << " nodes"
           << " and " << parent->getNet()->getGRShapes().size() << " wires and " << parent->getNet()->getGRVias().size() << " vias\n";
    }
  } else {
    // break on both sides

    if (enableOutput) {
      cout << "  @@@ before split " << parent->getNet()->getName() << " has " << parent->getNet()->getNodes().size() << " nodes"
           << " and " << parent->getNet()->getGRShapes().size() << " wires and " << parent->getNet()->getGRVias().size() << " vias\n";
    }

    frPoint childLoc = child->getLoc();
    if (childLoc == bp) {
      auto breakNode1 = initBoundary_splitPathSeg_split(child, parent, breakPt1);
      initBoundary_splitPathSeg_split(breakNode1, parent, breakPt2);
    } else {
      auto breakNode2 = initBoundary_splitPathSeg_split(child, parent, breakPt2);
      initBoundary_splitPathSeg_split(breakNode2, parent, breakPt1);
    }
    if (enableOutput) {
      cout << "  " << static_cast<frNet*>(parent->getNet())->getName() << " breaking at ("
           << breakPt1.x() << ", " << breakPt1.y() << ")\n";
    }
    if (enableOutput) {
      cout << "  " << static_cast<frNet*>(parent->getNet())->getName() << " breaking at ("
           << breakPt2.x() << ", " << breakPt2.y() << ")\n";
    }

    if (enableOutput) {
      cout << "  after split " << parent->getNet()->getName() << " has " << parent->getNet()->getNodes().size() << " nodes"
           << " and " << parent->getNet()->getGRShapes().size() << " wires and " << parent->getNet()->getGRVias().size() << " vias\n";
    }
  }
}

// breakPt1 <= breakPt2 always
void FlexGRWorker::initBoundary_splitPathSeg_getBreakPts(const frPoint &bp, const frPoint &ep, frPoint &breakPt1, frPoint &breakPt2) {
  bool hasBreakPt1 = false;
  bool hasBreakPt2 = false;
  
  if (bp.x() == ep.x()) {
    // V
    // break point at bottom
    if (bp.y() < extBox.bottom()) {
      breakPt1.set(bp.x(), extBox.bottom());
      hasBreakPt1 = true;
    }
    if (ep.y() > extBox.top()) {
      breakPt2.set(ep.x(), extBox.top());
      hasBreakPt2 = true;
    }
  } else if (bp.y() == ep.y()) {
    // H
    if (bp.x() < extBox.left()) {
      breakPt1.set(extBox.left(), bp.y());
      hasBreakPt1 = true;
    }
    if (ep.x() > extBox.right()) {
      breakPt2.set(extBox.right(), ep.y());
      hasBreakPt2 = true;
    }
  } else {
    cout << "Error: jackpot in FlexGRWorker::initBoundary_splitPathSeg_getBreakPts\n";
  }

  if (hasBreakPt1 && !hasBreakPt2) {
    breakPt2 = breakPt1;
  } else if (!hasBreakPt1 && hasBreakPt2) {
    breakPt1 = breakPt2;
  } else if (hasBreakPt1 && hasBreakPt2) {
    ;
  } else {
    cout << "Error: at least one breakPt should be created in FlexGRWorker::initBoundary_splitPathSeg_getBreakPts\n";
  }

}

frNode* FlexGRWorker::initBoundary_splitPathSeg_split(frNode* child, frNode* parent, const frPoint &breakPt) {
  // frNet
  auto pathSeg = static_cast<grPathSeg*>(child->getConnFig());
  auto net = pathSeg->getNet();
  auto lNum = pathSeg->getLayerNum();
  frPoint bp, ep;
  pathSeg->getPoints(bp, ep);
  frPoint childLoc = child->getLoc();
  bool isChildBP = (childLoc == bp);

  auto uBreakNode = make_unique<frNode>();
  auto breakNode = uBreakNode.get();
  breakNode->addToNet(net);
  breakNode->setLoc(breakPt);
  breakNode->setLayerNum(lNum);
  breakNode->setType(frNodeTypeEnum::frcBoundaryPin);

  // update connectivity
  parent->removeChild(child);
  child->setParent(breakNode);
  breakNode->addChild(child);
  breakNode->setParent(parent);
  parent->addChild(breakNode);

  net->addNode(uBreakNode);

  // create new pathSegs
  // child - breakPt
  auto uPathSeg1 = make_unique<grPathSeg>();
  auto pathSeg1 = uPathSeg1.get();
  pathSeg1->setChild(child);
  child->setConnFig(pathSeg1);
  pathSeg1->setParent(breakNode);
  pathSeg1->addToNet(net);
  if (isChildBP) {
    pathSeg1->setPoints(bp, breakPt);
  } else {
    pathSeg1->setPoints(breakPt, ep);
  }
  pathSeg1->setLayerNum(lNum);

  // breakPt - parent
  auto uPathSeg2 = make_unique<grPathSeg>();
  auto pathSeg2 = uPathSeg2.get();
  pathSeg2->setChild(breakNode);
  breakNode->setConnFig(pathSeg2);
  pathSeg2->setParent(parent);
  if (isChildBP) {
    pathSeg2->setPoints(breakPt, ep);
  } else {
    pathSeg2->setPoints(bp, breakPt);
  }
  pathSeg2->setLayerNum(lNum);

  // update region query
  getRegionQuery()->removeGRObj(pathSeg);
  getRegionQuery()->addGRObj(pathSeg1);
  getRegionQuery()->addGRObj(pathSeg2);

  // update net ownership
  unique_ptr<grShape> uGRConnFig1(std::move(uPathSeg1));
  unique_ptr<grShape> uGRConnFig2(std::move(uPathSeg2));

  net->addGRShape(uGRConnFig1);
  net->addGRShape(uGRConnFig2);

  net->removeGRShape(pathSeg);

  return breakNode;
}

void FlexGRWorker::init() {
  // init gridGraph first because in case there are overlapping nodes, we need to remove all existing
  // route objs (subtract from cmap) and merge overlapping nodes and recreate route objs
  // and add back to cmap to make sure cmap is accurate
  initGridGraph();

  initNets();
}

void FlexGRWorker::initNets() {
  set<frNet*, frBlockObjectComp> nets;
  // map<frNet*, vector<unique_ptr<grConnFig> >, frBlockObjectComp> netObjs;
  map<frNet*, vector<frNode*>, frBlockObjectComp> netRoots;

  // initNetObjs(nets, netObjs);
  initNets_roots(nets, netRoots);
  initNets_searchRepair(nets, netRoots);
  // initNets_printFNets(netRoots);

  initNets_regionQuery();
  // initNets_numPinsIn();
  // initNets_printNets();
}

// get all roots of subnets
void FlexGRWorker::initNets_roots(set<frNet*, frBlockObjectComp> &nets,
                                  map<frNet*, vector<frNode*>, frBlockObjectComp> &netRoots) {
  vector<grBlockObject*> result;
  getRegionQuery()->queryGRObj(routeBox, result);

  for (auto rptr: result) {
    if (rptr->typeId() == grcPathSeg) {
      auto cptr = static_cast<grPathSeg*>(rptr);
      if (cptr->hasNet()) {
        initNetObjs_roots_pathSeg(cptr, nets, netRoots);
      } else {
        cout << "Error: initNetObjs hasNet() empty" <<endl;
      }
    } else if (rptr->typeId() == grcVia) {
      auto cptr = static_cast<grVia*>(rptr);
      if (cptr->hasNet()) {
        initNetObjs_roots_via(cptr, nets, netRoots);
      } else {
        cout << "Error: initNetObjs hasNet() empty" <<endl;
      }
    } else {
      cout << rptr->typeId() << endl;
      cout << "Error: initNetObjs unsupported type" << endl;
    }
  }
}

// if parent of a pathSeg is at the boundary of extBox, the parent is a subnet root (i.e., outgoing edge)
void FlexGRWorker::initNetObjs_roots_pathSeg(grPathSeg* pathSeg,
                                             set<frNet*, frBlockObjectComp> &nets,
                                             map<frNet*, vector<frNode*>, frBlockObjectComp> &netRoots) {
  bool enableOutput = false;
  auto net = pathSeg->getNet();
  nets.insert(net);

  auto parent = pathSeg->getParent();
  auto parentLoc = parent->getLoc();

  // boundary pin root
  if (parentLoc.x() == extBox.left() || parentLoc.x() == extBox.right() || 
      parentLoc.y() == extBox.bottom() || parentLoc.y() == extBox.top()) {
    netRoots[net].push_back(parent);
    if (enableOutput) {
      if (net->getName() == string("pin1")) {
        cout << "new boundary net root for " << net->getName() << " from pathSeg " << pathSeg << endl;;
        frPoint bp, ep;
        pathSeg->getPoints(bp, ep);
        cout << "  from (" << bp.x() << ", " << bp.y() << ") to (" << ep.x() << ", " << ep.y() << ")\n";
      }
    }
  }

  // real root
  if (parent->getParent() == net->getRoot()) {
    netRoots[net].push_back(parent->getParent());
    if (enableOutput) {
      if (net->getName() == string("pin1")) {
        cout << "new true net root for " << net->getName() << " from pathSeg " << pathSeg << endl;;
        frPoint bp, ep;
        pathSeg->getPoints(bp, ep);
        cout << "  from (" << bp.x() << ", " << bp.y() << ") to (" << ep.x() << ", " << ep.y() << ")\n";
      }
    }
  }

  // old
  // vertical seg
  // if (begin.x() == end.x()) {
  //   // the pathSeg must go through extBox or at least one end is inside extBox
  //   if (!(begin.y() > workerExtBox.top() || end.y() < workerExtBox.bottom())) {
  //     // parent is frNode
  //     auto parent = pathSeg->getParent();
  //     auto parentLoc = parent->getLoc();
  //     // parent is outside of extBox, which implies there is an outgoing edge (i.e., a subnet root)
  //     if (!workerExtBox.contains(parentLoc)) {
  //       auto uRootNode = make_unique<grNode>(*parent);
  //       auto rootNode = uRootNode.get();
  //       rootNode->setNet(nullptr);
  //       rootNode->setParent(nullptr);
  //       rootNode->setConnFig(nullptr);
  //       rootNode->setType(frNodeTypeEnum::frcBoundaryPin);
  //       rootNode->clearChildren();
  //       frPoint rootLoc = rootNode->getLoc();
  //       // move rootLoc to extBox boundary
  //       if (rootLoc.y() > workerExtBox.top()) {
  //         rootLoc.set(begin.x(), workerExtBox.top());
  //       } else if (rootLoc.y() < workerExtBox.bottom()) {
  //         rootLoc.set(begin.x(), workerExtBox.bottom());
  //       } else {
  //         cout << "Error: something went wrong in FlexGRWorker::initNetObjs_roots_pathSeg\n";
  //       }
  //       rootNode->setLoc(rootLoc);
  //       netRoots[net].push_back(std::move(uRootNode));
  //       netRootChilds[net].push_back(pathSeg->getChild());

  //       // push loc to boundPt
  //       owner2extBoundPtNodes[net][rootLoc].push_back(rootNode);
  //     } else {
  //       // check if parent node is frNet root (i.e., driver pin) when parent is inside extBox
  //       if (parent->getParent() == net->getRoot()) {
  //         auto uRootNode = make_unique<grNode>(*(parent->getParent()));
  //         auto rootNode = uRootNode.get();
  //         rootNode->setNet(nullptr);
  //         rootNode->setParent(nullptr);
  //         rootNode->setConnFig(nullptr);
  //         rootNode->clearChildren();
  //         netRoots[net].push_back(std::move(uRootNode));
  //         netRootChilds[net].push_back(parent);
  //       }
  //     }
  //   } else {
  //     cout << "Error: pathSeg completely outside of extBox found\n";
  //   }
  // } else if (begin.y() == end.y()) {
  //   // the pathSeg must go through extBox or at least one end is inside extBox
  //   if (!(begin.x() >= routeBox.right() || end.x() <= routeBox.left())) {
  //     // parent is frNode
  //     auto parent = pathSeg->getParent();
  //     auto parentLoc = parent->getLoc();
  //     // parent is outside of extBox, which implies there is an outgoing edge (i.e., a subnet root)
  //     if (!workerExtBox.contains(parentLoc)) {
  //       auto uRootNode = make_unique<grNode>(*parent);
  //       auto rootNode = uRootNode.get();
  //       rootNode->setNet(nullptr);
  //       rootNode->setParent(nullptr);
  //       rootNode->setConnFig(nullptr);
  //       rootNode->setType(frNodeTypeEnum::frcBoundaryPin);
  //       rootNode->clearChildren();
  //       frPoint rootLoc = rootNode->getLoc();
  //       // move rootLoc to extBox boundary
  //       if (rootLoc.x() > workerExtBox.right()) {
  //         rootLoc.set(workerExtBox.right(), begin.y());
  //       } else if (rootLoc.x() < workerExtBox.left()) {
  //         rootLoc.set(workerExtBox.left(), begin.y());
  //       } else {
  //         cout << "Error: something went wrong in FlexGRWorker::initNetObjs_roots_pathSeg\n";
  //       }
  //       rootNode->setLoc(rootLoc);
  //       netRoots[net].push_back(std::move(uRootNode));
  //       netRootChilds[net].push_back(pathSeg->getChild());

  //       // push loc to boundPt
  //       owner2extBoundPtNodes[net][rootLoc].push_back(rootNode);
  //     } else {
  //       // check if parent node is frNet root (i.e., driver pin) when parent is inside extBox
  //       if (parent->getParent() == net->getRoot()) {
  //         auto uRootNode = make_unique<grNode>(*(parent->getParent()));
  //         auto rootNode = uRootNode.get();
  //         rootNode->setNet(nullptr);
  //         rootNode->setParent(nullptr);
  //         rootNode->setConnFig(nullptr);
  //         rootNode->clearChildren();
  //         netRoots[net].push_back(std::move(uRootNode));
  //         netRootChilds[net].push_back(parent);
  //       }
  //     }
  //   } else {
  //     cout << "Error: pathSeg completely outside of extBox found\n";
  //   }
  // } else {
  //   cout << "Error: jackpot in FlexGRWorker::initNetObjs_pathSeg\n";
  // }  
}

// all vias from rq must be inside routeBox
// if the parent of the via parent is the same node as frNet root, then the grandparent is a subnet root
void FlexGRWorker::initNetObjs_roots_via(grVia* via,
                                         set<frNet*, frBlockObjectComp> &nets,
                                         map<frNet*, vector<frNode*>, frBlockObjectComp> &netRoots) {
  auto net = via->getNet();
  nets.insert(net);

  // real root
  auto parent = via->getParent();
  if (parent->getParent() == net->getRoot()) {
    netRoots[net].push_back(parent->getParent());
  }

  // old
  // frPoint parentLoc = parent->getLoc();
  // if (workerExtBox.contains(parentLoc)) {
  //   if (parent->getParent() == net->getRoot()) {
  //     auto rootNode = make_unique<grNode>(*(parent->getParent()));
  //     rootNode->setNet(nullptr);
  //     rootNode->setParent(nullptr);
  //     rootNode->setConnFig(nullptr);
  //     rootNode->clearChildren();
  //     netRoots[net].push_back(std::move(rootNode));
  //     netRootChilds[net].push_back(parent);
  //   }
  // } else {
  //   cout << "Error: via outside of extBox should not be found in FlexGRWorker::initNetObjs_roots_via\n";
  // }
}

void FlexGRWorker::initNets_searchRepair(set<frNet*, frBlockObjectComp> &nets,
                                         map<frNet*, vector<frNode*>, frBlockObjectComp> &netRoots) {
  for (auto net: nets) {
    initNet(net, netRoots[net]);
  }
}

void FlexGRWorker::initNet(frNet* net, const vector<frNode*> &netRoots) {
  set<frNode*, frBlockObjectComp> uniqueRoots;
  for (auto fRoot: netRoots) {
    if (uniqueRoots.find(fRoot) != uniqueRoots.end()) {
      continue;
    }
    uniqueRoots.insert(fRoot);

    auto uGRNet = make_unique<grNet>();
    auto gNet = uGRNet.get();
    gNet->setFrNet(net);

    initNet_initNodes(gNet, fRoot);
    initNet_initRoot(gNet);
    initNet_updateCMap(gNet, false);
    initNet_initPinGCellNodes(gNet);
    initNet_updateCMap(gNet, true);
    initNet_initObjs(gNet);
    gNet->setId(nets.size());
    initNet_addNet(uGRNet);
  }

  // old
  // for (unsigned i = 0; i < netRoots.size(); i++) {
  //   auto &fRoot = netRoots[i];
  //   auto rootNode = uRoot.get();
  //   auto rootChild = netRootChilds[i];

  //   auto uGRNet = make_unique<grNet>();
  //   auto gNet = uGRNet.get();
  //   gNet->setFrNet(net);
  //   gNet->setId(nets.size());
  //   gNet->setRoot(uRoot.get());
  //   gNet->addNode(uRoot);

  //   initNet_initNodes(gNet, root, rootChild);

  //   initNet_initObjs(gNet, root);

  //   initNet_addNet(uGRNet);
  // }
}

// bfs from root to deep copy nodes and find all pin nodes (either pin or boundary pin)
void FlexGRWorker::initNet_initNodes(grNet* net, frNode* fRoot) {
  // map from loc to gcell node
  // map<pair<frPoint, frlayerNum>, grNode*> loc2GCellNode;
  vector<pair<frNode*, grNode*> > pinNodePairs;
  map<grNode*, frNode*, frBlockObjectComp> gr2FrPinNode;
  // parent grNode to children frNode
  deque<pair<grNode*, frNode*> > nodeQ;
  nodeQ.push_back(make_pair(nullptr, fRoot));

  grNode* parent = nullptr;
  frNode* child = nullptr;

  // bfs from root to deep copy nodes (and create gcell center nodes as needed)
  while (!nodeQ.empty()) {
    parent = nodeQ.front().first;
    child = nodeQ.front().second;
    nodeQ.pop_front();

    bool isRoot = (parent == nullptr);
    frPoint parentLoc;
    if (!isRoot) {
      parentLoc = parent->getLoc();
    }
    auto childLoc = child->getLoc();

    // if parent is boudnary pin and there is no immediate child at the gcell center location, need to create a gcell center node
    bool needGenParentGCellNode = false;
    // if child is boudnary pin and its parent is not at the gcell center location, need to create a gcell center node
    bool needGenChildGCellNode = false;
    
    // root at boundary
    if (!isRoot && parent->getType() == frNodeTypeEnum::frcBoundaryPin) {
      frPoint gcellLoc = getBoundaryPinGCellNodeLoc(parentLoc);
      // if parent is boundary pin, child must be on the same layer
      if (gcellLoc != childLoc) {
        needGenParentGCellNode = true;
      }
    }
    // leaf at boundary
    if (!needGenParentGCellNode && !isRoot && child->getType() == frNodeTypeEnum::frcBoundaryPin) {
      frPoint gcellLoc = getBoundaryPinGCellNodeLoc(childLoc);
      // no need to gen gcell node if immediate parent is the gcell node we need
      if (gcellLoc != parentLoc) {
        needGenChildGCellNode = true;
      }
    }

    grNode* newNode = nullptr;
    // deep copy node -- if need to generate (either parent or child), gen new node instead of copy
    if (needGenParentGCellNode) {
      auto uGCellNode = make_unique<grNode>();
      auto gcellNode = uGCellNode.get();
      net->addNode(uGCellNode);

      gcellNode->addToNet(net);
      frPoint gcellNodeLoc = getBoundaryPinGCellNodeLoc(parentLoc);

      gcellNode->setLoc(gcellNodeLoc);
      gcellNode->setLayerNum(parent->getLayerNum());
      gcellNode->setType(frNodeTypeEnum::frcSteiner);
      // update connections
      gcellNode->setParent(parent);
      parent->addChild(gcellNode);

      newNode = gcellNode;
    } else if (needGenChildGCellNode) {
      auto uGCellNode = make_unique<grNode>();
      auto gcellNode = uGCellNode.get();
      net->addNode(uGCellNode);

      gcellNode->addToNet(net);
      frPoint gcellNodeLoc = getBoundaryPinGCellNodeLoc(childLoc);

      gcellNode->setLoc(gcellNodeLoc);
      gcellNode->setLayerNum(child->getLayerNum());
      gcellNode->setType(frNodeTypeEnum::frcSteiner);
      // update connectons
      gcellNode->setParent(parent);
      parent->addChild(gcellNode);

      newNode = gcellNode;
    } else {
      auto uChildNode = make_unique<grNode>(*child);
      auto childNode = uChildNode.get();
      net->addNode(uChildNode);

      childNode->addToNet(net);
      // loc, layerNum, type and pin are set by copy constructor
      // root does not have parent node
      if (!isRoot) {
        childNode->setParent(parent);
        parent->addChild(childNode);
      }

      newNode = childNode;

      // add to pinNodePairs
      if (child->getType() == frNodeTypeEnum::frcBoundaryPin || child->getType() == frNodeTypeEnum::frcPin) {
        pinNodePairs.push_back(make_pair(child, childNode));
        gr2FrPinNode[childNode] = child;

        if (isRoot) {
          net->setFrRoot(child);
        }
      }
    }

    // push edge to queue
    if (needGenParentGCellNode) {
      nodeQ.push_back(make_pair(newNode, child));
    } else if (needGenChildGCellNode) {
      nodeQ.push_back(make_pair(newNode, child));
    } else {
      if (isRoot || child->getType() == frNodeTypeEnum::frcSteiner) {
        for (auto grandChild: child->getChildren()) {
          nodeQ.push_back(make_pair(newNode, grandChild));
        }
      }
    }
  }

  net->setPinNodePairs(pinNodePairs);
  net->setGR2FrPinNode(gr2FrPinNode);


  // old
  // bfs from root to deep copy nodes
  // while (!nodeQ.empty()) {
  //   parent = node.front().first;
  //   child = node.front().second;
  //   nodeQ.pop_front();

  //   bool isParentRoot = (parent == net->getRoot());
  //   // if parent is root, need to create gcell node connecting to root (no matter boundary root or pin root)
  //   // and set the newly created node to be parent
  //   if (isParentRoot) {
  //     auto &uGCellNode = make_unique<grNode>(child);
  //     auto gcellNode = uGCellNode.get();
  //     net->addNode(uGCellNode);

  //     gcellNode->setNet(net);
  //     // adjust location
  //     if (parent->getType() == frNodeTypeEnum::frcBoundaryPin) {
  //       frPoint gcellNodeLoc = parent->getLoc();
  //       if (gcellNodeLoc.x() == workerExtBox.left()) {
  //         gcellNodeLoc.set(workerRouteBox.left(), gcellNodeLoc.y());
  //       } else if (gcellNodeLoc.x() == workerExtBox.right()) {
  //         gcellNodeLoc.set(workerRouteBox.right(), gcellNodeLoc.y());
  //       } else if (gcellNodeLoc.y() == workerExtBox.bottom()) {
  //         gcellNodeLoc.set(gcellNodeLoc.x(), workerRouteBox.bottom());
  //       } else if (gcellNodeLoc.y() == workerExtBox.top()) {
  //         gcellNodeLoc.set(gcellNodeLoc.x(), workerRouteBox.top());
  //       } else {
  //         cout << "Error: boundary pin is not on boundary in FlexGRWorker::initNet_initPinNodes\n";
  //       }
  //       gcellNode->setLoc(gcellNodeLoc);
  //     } else if (parent->getType() == frNodeTypeEnum::frcPin) {
  //       frPoint gcellNodeLoc = child->getLoc();
  //       gcellNode->setLoc(gcellNodeLoc);
  //     } else {
  //       cout << "Error: steiner should not be root in FlexGRWorker::initNet_initPinNodes\n";
  //     }
  //     gcellNode->setType(frNodeTypeEnum::frcSteiner);
  //     // update connections
  //     gcellNode->setParent(parent);
  //     parent->addChild(gcellNode);
  //     // add pair for post route add wire, etc.
  //     net->addPinGCellNodePair(make_pair(parent, gcellNode));

  //     loc2PinGCellNode[make_pair(gcellNodeLoc, gcellNode->getLayerNum())] = gcellNode;

  //     // use the newly created node as parent
  //     parent = gcellNode;
  //   }

  //   // if child is outside of extBox, need to create boundary pin node and gcell node
  //   frPoint childLoc = child->getLoc();
  //   if (!workerExtBox.contains(childLoc)) {
  //     // first create gcell node (if it does not exist in loc2PinGCellNode)
  //     frPoint gcellNodeLoc = childLoc;
  //     frPoint boundPinNodeLoc = childLoc;
  //     if (childLoc.x() > workerExtBox.left() && childLoc.x() < workerExtBox.right()) {
  //       if (gcellNodeLoc.y() > workerExtBox.top()) {
  //         gcellNodeLoc.set(gcellNodeLoc.x(), workerRouteBox.top());
  //         boundPinNodeLoc.set(boundPinNodeLoc.x(), workerExtBox.top());
  //       } else if (gcellNodeLoc.y() < workerExtBox.bottom()) {
  //         gcellNodeLoc.set(gcellNodeLoc.x(), workerRouteBox.bottom());
  //         boundPinNodeLoc.set(boundPinNodeLoc.x(), workerExtBox.bottom());
  //       } else {
  //         cout << "Error: contradiction found in FlexGRWorker::initNet_initPinNodes\n";
  //       }
  //     } else if (childLoc.y() > workerExtBox.bottom() && childLoc.y() < workerExtBox.top()) {
  //       if (gcellNodeLoc.x() > workerExtBox.right()) {
  //         gcellNodeLoc.set(workerRouteBox.right(), gcellNodeLoc.y());
  //         boundPinNodeLoc.set(workerExtBox.right(), boundPinNodeLoc.y());
  //       } else if (gcellNodeLoc.x() < workerExtBox.left()) {
  //         gcellNodeLoc.set(workerRouteBox.left(), gcellNodeLoc.y());
  //         boundPinNodeLoc.set(workerExtBox.left(), boundPinNodeLoc.y());
  //       } else {
  //         cout << "Error: contradiction found in FlexGRWorker::initNet_initPinNodes\n";
  //       }
  //     } else {
  //       cout << "Error: child has positive prl on both direction in FlexGRWorker::initNet_initPinNodes\n";
  //     }

  //     grNode *gcellNode = nullptr;
  //     if (loc2PinGCellNode.find(make_pair(gcellNodeLoc, child->getLayerNum())) == loc2PinGCellNode.end()) {
  //       auto &uGCellNode = make_unique<grNode>(child);
  //       gcellNode = uGCellNode.get();
  //       net->addNode(uGCellNode);

  //       gcellNode->setNet(net);
  //       gcellNode->setLoc(gcellNodeLoc);
  //       gcellNode->setType(frNodeTypeEnum::frcSteiner);
  //       // update connections
  //       gcellNode->setParent(parent);
  //       parent->addChild(gcellNode);

  //       loc2PinGCellNode[make_pair(gcellNodeLoc, gcellNode->getLayerNum())] = gcellNode;

  //       // push to owner2routeBoundPtNodes
  //       owner2routeBoundPtNodes[net->getFrNet()][gcellNodeLoc].push_back(gcellNode);
  //     } else {
  //       gcellNode = loc2PinGCellNode[make_pair(gcellNodeLoc, child->getLayerNum())];
  //     }

  //     // create boundary pin node and connect to gcell node
  //     auto &uBoundPinNode = make_unique<grNode>(child);
  //     auto boundPinNode = uBoundPinNode.get();
  //     net->addNode(uBoundPinNode);

  //     boundPinNode->setNet(net);
  //     boundPinNode->setLoc(boundPinNodeLoc);
  //     boundPinNode->setType(frNodeTypeEnum::frcBoundaryPin);
  //     // update connections
  //     boundPinNode->setParent(gcellNode);
  //     gcellNode->addChild(boundPinNode);

  //     // add pair for post route add wire, etc.
  //     net->addPinGCellNodePair(make_pair(boundPinNode, gcellNode));

  //     // push to owner2extBoundPtNodes
  //     owner2extBoundPtNodes[net->getFrNet()][boundPinNodeLoc].push_back(boundPinNode);


  //   } else {
  //     // normal case: child is inside extBox and only need to deep copy child and connect to parent
  //     // also the only case where children of child need to be pushed to queue
  //     grNode* childNode = nullptr;
  //     frPoint childNodeLoc = child->getLoc();
  //     frlayerNum childNodeLNum = child->getLayerNum();

  //     // deep copy child if it does not exist
  //     if (loc2PinGCellNode.find(make_pair(childNodeLoc, childNodeLNum)) == loc2PinGCellNode.end()) {
  //       auto &uChildNode = make_unique<grNode>(child);
  //       childNode = uChildNode.get();
  //       net->addNode(uChildNode)

  //       childNode->setNet(net);

  //       // update connection to parent
  //       childNode->setParent(parent);
  //       parent->addChild(childNode);
  //     } else {
  //       childNode = loc2PinGCellNode[make_pair(childNodeLoc, childNodeLNum)];
  //     }

  //     // push children connection to queue
  //     for (auto grandChild: child->getChildren()) {
  //       nodePairQ.push_back(make_pair(childNode, grandChild));

  //       if (grandChild->getType() == frNodeTypeEnum::frcPin) {
  //         if (loc2PinGCellNode.find(make_pair(childNodeLoc, childNodeLNum)) == loc2PinGCellNode.end()) {
  //           loc2PinGCellNode[make_pair(childNodeLoc, childNodeLNum)] = childNode;
  //         }
  //       }
  //     }
  //   }
  // }
}

frPoint FlexGRWorker::getBoundaryPinGCellNodeLoc(const frPoint &boundaryPinLoc) {
  frPoint gcellNodeLoc;
  if (boundaryPinLoc.x() == extBox.left()) {
    gcellNodeLoc.set(routeBox.left(), boundaryPinLoc.y());
  } else if (boundaryPinLoc.x() == extBox.right()) {
    gcellNodeLoc.set(routeBox.right(), boundaryPinLoc.y());
  } else if (boundaryPinLoc.y() == extBox.bottom()) {
    gcellNodeLoc.set(boundaryPinLoc.x(), routeBox.bottom());
  } else if (boundaryPinLoc.y() == extBox.top()) {
    gcellNodeLoc.set(boundaryPinLoc.x(), routeBox.top());
  } else {
    cout << "Error: non-boundary pin loc in FlexGRWorker::getBoundaryPinGCellNodeLoc\n";
  }
  return gcellNodeLoc;
}

void FlexGRWorker::initNet_initRoot(grNet* net) {
  // set root
  auto rootNode = net->getNodes().front().get();
  net->setRoot(rootNode);
  // sanity check
  if (rootNode->getType() == frNodeTypeEnum::frcBoundaryPin) {
    frPoint rootLoc = rootNode->getLoc();
    if (extBox.contains(rootLoc) && routeBox.contains(rootLoc)) {
      cout << "Error: root should be on an outgoing edge\n";
    }
  } else if (rootNode->getType() == frNodeTypeEnum::frcPin) {
    frPoint rootLoc = rootNode->getLoc();
    frPoint globalRootLoc = rootNode->getNet()->getFrNet()->getRoot()->getLoc();
    if (rootLoc != globalRootLoc) {
      cout << "Error: local root and global root location mismatch\n";
    }
  } else {
    cout << "Error: root should not be steiner\n";
  }
}

// based on topology, add / remove congestion map (in gridGraph)
void FlexGRWorker::initNet_updateCMap(grNet* net, bool isAdd) {
  deque<grNode*> nodeQ;
  nodeQ.push_back(net->getRoot());

  while (!nodeQ.empty()) {
    auto node = nodeQ.front();
    nodeQ.pop_front();

    // push steiner child to nodeQ
    for (auto child: node->getChildren()) {
      if (child->getType() == frNodeTypeEnum::frcSteiner) {
        nodeQ.push_back(child);
      }
    }

    // upadte cmap for connections between steiner nodes
    if (node->getParent() && 
        node->getParent()->getType() == frNodeTypeEnum::frcSteiner && 
        node->getType() == frNodeTypeEnum::frcSteiner) {
      // currently only update for pathSeg
      auto parent = node->getParent();
      if (parent->getLayerNum() != node->getLayerNum()) {
        auto parentLoc = parent->getLoc();
        auto nodeLoc = node->getLoc();
        FlexMazeIdx bi, ei;
        frPoint bp, ep;
        frLayerNum lNum = parent->getLayerNum();
        if (parentLoc < nodeLoc) {
          bp = parentLoc;
          ep = nodeLoc;
        } else {
          bp = nodeLoc;
          ep = parentLoc;
        }
        gridGraph.getMazeIdx(bp, lNum, bi);
        gridGraph.getMazeIdx(ep, lNum, ei);

        if (bi.x() == ei.x()) {
          // vertical pathSeg
          for (auto yIdx = bi.y(); yIdx < ei.y(); yIdx++) {
            if (isAdd) {
              gridGraph.addRawDemand(bi.x(), yIdx, bi.z(), frDirEnum::N);
              gridGraph.addRawDemand(bi.x(), yIdx + 1, bi.z(), frDirEnum::N);
            } else {
              gridGraph.subRawDemand(bi.x(), yIdx, bi.z(), frDirEnum::N);
              gridGraph.subRawDemand(bi.x(), yIdx + 1, bi.z(), frDirEnum::N);
            }
          }
        } else if (bi.y() == ei.y()) {
          // horizontal pathSeg
          for (auto xIdx = bi.x(); xIdx < ei.x(); xIdx++) {
            if (isAdd) {
              gridGraph.addRawDemand(xIdx, bi.y(), bi.z(), frDirEnum::E);
              gridGraph.addRawDemand(xIdx + 1, bi.y(), bi.z(), frDirEnum::E);
            } else {
              gridGraph.subRawDemand(xIdx, bi.y(), bi.z(), frDirEnum::E);
              gridGraph.subRawDemand(xIdx + 1, bi.y(), bi.z(), frDirEnum::E);
            }
          }
        } else {
          cout << "Error: non-colinear pathSeg in updateCMap_net" << endl;
        }
      }
    }
  }
}

// initialize pinGCellNodes as well as removing overlapping pinGCellNodes
void FlexGRWorker::initNet_initPinGCellNodes(grNet* net) {
  vector<pair<grNode*, grNode*> > pinGCellNodePairs;
  map<grNode*, vector<grNode*>, frBlockObjectComp> gcell2PinNodes;
  vector<grNode*> pinGCellNodes;
  map<FlexMazeIdx, grNode*> midx2PinGCellNode;
  grNode* rootGCellNode = nullptr;

  deque<grNode*> nodeQ;
  nodeQ.push_back(net->getRoot());
  while (!nodeQ.empty()) {
    auto node = nodeQ.front();
    nodeQ.pop_front();
    // push children to queue
    for (auto child: node->getChildren()) {
      nodeQ.push_back(child);
    }

    grNode *gcellNode = nullptr;
    if (node == net->getRoot()) {
      gcellNode = node->getChildren().front();
    } else if (!node->hasChildren()) {
      gcellNode = node->getParent();
    } else {
      continue;
    }

    if (!rootGCellNode) {
      rootGCellNode = gcellNode;
    }

    frPoint gcellNodeLoc = gcellNode->getLoc();
    frLayerNum gcellNodeLNum = gcellNode->getLayerNum();
    FlexMazeIdx gcellNodeMIdx;
    gridGraph.getMazeIdx(gcellNodeLoc, gcellNodeLNum, gcellNodeMIdx);

    if (midx2PinGCellNode.find(gcellNodeMIdx) == midx2PinGCellNode.end()) {
      midx2PinGCellNode[gcellNodeMIdx] = gcellNode;
    } else {
      if (gcellNode != midx2PinGCellNode[gcellNodeMIdx]) {
        // disjoint pinGCellNode overlap
        cout << "Warning: overlapping disjoint pinGCellNodes detected of " << net->getFrNet()->getName() << "\n";
        // ripup to make sure congestion map is up-to-date
        net->setRipup(true);
        if (midx2PinGCellNode[gcellNodeMIdx] == rootGCellNode) {
          // loop back to rootGCellNode, need to directly connect child node to root
          cout << "  between rootGCellNode and non-rootGCellNode\n";
          node->getParent()->removeChild(node);
          if (node->getConnFig()) {
            auto pathSeg = static_cast<grPathSeg*>(node->getConnFig());
            pathSeg->setParent(rootGCellNode);
          }
          node->setParent(rootGCellNode);
          rootGCellNode->addChild(node);
          gcellNode = rootGCellNode;
        } else {
          // merge the gcellNode to existing leaf node (directly connect all children of the gcellNode to existing leaf)
          // do not care about their parents because it will be rerouted anyway
          cout << "  between non-rootGCellNode and non-rootGCellNode\n";
          node->getParent()->removeChild(node);
          if (node->getConnFig()) {
            auto pathSeg = static_cast<grPathSeg*>(node->getConnFig());
            pathSeg->setParent(midx2PinGCellNode[gcellNodeMIdx]);
          }
          node->setParent(midx2PinGCellNode[gcellNodeMIdx]);
          midx2PinGCellNode[gcellNodeMIdx]->addChild(node);
          gcellNode = midx2PinGCellNode[gcellNodeMIdx];
        }
      }
    }

    if (gcellNode) {
      pinGCellNodePairs.push_back(make_pair(node, gcellNode));
      if (gcell2PinNodes.find(gcellNode) == gcell2PinNodes.end()) {
        pinGCellNodes.push_back(gcellNode);
      }
      gcell2PinNodes[gcellNode].push_back(node);
    }
  }

  net->setPinGCellNodePairs(pinGCellNodePairs);
  net->setGCell2PinNodes(gcell2PinNodes);
  net->setPinGCellNodes(pinGCellNodes);
  if (pinGCellNodes.size() == 1) {
    net->setTrivial(true);
  } else if (pinGCellNodes.size() > 1) {
    net->setTrivial(false);
  } else {
    cout << "Error: pinGCellNodes should contain at least one element\n";
  }
}

// generate route and ext objs based on subnet tree node information
void FlexGRWorker::initNet_initObjs(grNet* net) {
  deque<grNode*> nodeQ;
  nodeQ.push_back(net->getRoot());
  while (!nodeQ.empty()) {
    auto node = nodeQ.front();
    nodeQ.pop_front();
    // push children to queue
    for (auto child: node->getChildren()) {
      nodeQ.push_back(child);
    }

    // generate and add objects
    // root does not have connFig to parent
    if (node->getParent() == nullptr) {
      continue;
    }
    // no pathSeg will be created between pin node and gcell that contains the pin node
    if (node->getType() == frNodeTypeEnum::frcPin ||
        node->getParent()->getType() == frNodeTypeEnum::frcPin) {
      continue;
    }
    bool isExt = false;
    // short pathSeg between boundary pin and gcell that contains boundary pin are 
    // extConnFig and will not be touched during ripup reroute
    if (node->getType() == frNodeTypeEnum::frcBoundaryPin || 
        node->getParent()->getType() == frNodeTypeEnum::frcBoundaryPin) {
      isExt = true;
    }

    if (node->getLayerNum() == node->getParent()->getLayerNum()) {
      // pathSeg
      auto parent = node->getParent();
      frPoint childLoc, parentLoc;
      frPoint bp, ep;
      node->getLoc(childLoc);
      parent->getLoc(parentLoc);
      if (childLoc < parentLoc) {
        bp = childLoc;
        ep = parentLoc;
      } else {
        bp = parentLoc;
        ep = childLoc;
      }

      auto uPathSeg = make_unique<grPathSeg>();
      uPathSeg->setChild(node);
      uPathSeg->setParent(parent);
      uPathSeg->addToNet(net);
      uPathSeg->setPoints(bp, ep);
      uPathSeg->setLayerNum(node->getLayerNum());

      unique_ptr<grConnFig> uGRConnFig(std::move(uPathSeg));
      if (isExt) {
        net->addExtConnFig(uGRConnFig);
      } else {
        net->addRouteConnFig(uGRConnFig);
      }
    } else {
      // via
      auto parent = node->getParent();
      frPoint loc;
      frLayerNum beginLayerNum, endLayerNum;
      node->getLoc(loc);
      beginLayerNum = node->getLayerNum();
      endLayerNum = parent->getLayerNum();

      auto uVia = make_unique<grVia>();
      uVia->setChild(node);
      uVia->setParent(parent);
      uVia->addToNet(net);
      uVia->setOrigin(loc);
      uVia->setViaDef(design->getTech()->getLayer((beginLayerNum + endLayerNum) / 2)->getDefaultViaDef());

      unique_ptr<grConnFig> uGRConnFig(std::move(uVia));
      if (isExt) {
        net->addExtConnFig(uGRConnFig);
      } else {
        net->addRouteConnFig(uGRConnFig);
      }
    }
  }
}

void FlexGRWorker::initNet_addNet(unique_ptr<grNet> &in) {
  owner2nets[in->getFrNet()].push_back(in.get());
  nets.push_back(std::move(in));
}

void FlexGRWorker::initNets_regionQuery() {
  auto &workerRegionQuery = getWorkerRegionQuery();
  workerRegionQuery.init(false/*not include ext*/);
  // workerRegionQuery.report();
}

// get root gcell nodes (either child node of outgoing edge or root node of a net)
// void FlexGRWorker::initNets_searchRepair_getRootNodes(frNet* net, 
//                                                       vector<unique_ptr<grConnFig> > &netRouteObjs,
//                                                       vector<unique_ptr<frNode> > &netRootGCellNodes) {
//   frPoint bp, ep;
//   frPoint nodeLoc;
//   frNode* parent = nullptr;
//   frNode* child = nullptr;
//   set<pair<frPoint, frlayerNum> > rootGCellNodeLocs;
//   for (auto &uPtr: netRouteObjs) {
//     auto connFig = uPtr.get();
//     if (connFig->typeId() == grcPathSeg) {
//       auto obj = static_cast<grPathSeg*>(connFig);
//       obj->getPoints(bp, ep);
//       parent = obj->getParent();
//       child = obj->getChild();
//       auto lNum = obj->getLayerNum();
//       // create root node if outgoing pathSeg intersects with boundary
//       // vert
//       if (bp.x() == ep.x()) {
//         if (bp.y() == routeBox.bottom()) {
//           nodeLoc = parent->getLoc();
//           nodeLNum = parent->getLayerNum();
//           // outgoing pathSeg shortened at routeBox, create new boundary node
//           if (nodeLoc.y() < routeBox.bottom()) {
//             frPoint rootGCellNodeLoc(bp.x(), gridBBox().bottom()); 
//             if (rootGCellNodeLocs.find(make_pair(rootGCellNodeLoc, nodeLNum)) == rootGCellNodeLocs.end()) {
//               auto rootNode = make_unique<frNode>(*parent);
//               rootNode->setType(frNodeTypeEnum::frcBoundaryPin);
//               rootNode->setLoc(rootGCellNodeLoc);

//             }
//           }
//         }
//         if (ep.y() == routeBox.top()) {

//         }
//       }
//       // horz

//       // create root node if the parent node of the parent node of the pathSeg is the global net root

//     }
//   }
// }

// void FlexGRWorker::initGridGraph() {

//   gridGraph.setCost(workerCongCost, workerHistCost);
//   gridGraph.init()
// }

void FlexGRWorker::initNets_printNets() {
  cout << endl << "printing grNets\n";
  for (auto &net: nets) {
    initNets_printNet(net.get());
  }
}

void FlexGRWorker::initNets_printNet(grNet* net) {
  auto root = net->getRoot();
  deque<grNode*> nodeQ;
  nodeQ.push_back(root);

  cout << "start traversing subnet of " << net->getFrNet()->getName() << endl;

  while (!nodeQ.empty()) {
    auto node = nodeQ.front();
    nodeQ.pop_front();

    cout << "  ";
    if (node->getType() == frNodeTypeEnum::frcSteiner) {
      cout << "steiner node ";
    } else if (node->getType() == frNodeTypeEnum::frcPin) {
      cout << "pin node ";
    } else if (node->getType() == frNodeTypeEnum::frcBoundaryPin) {
      cout << "boundary pin node ";
    }
    cout << "at ";
    frPoint loc = node->getLoc();
    frLayerNum lNum = node->getLayerNum();
    cout << "(" << loc.x() << ", " << loc.y() << ") on layerNum " << lNum << endl;

    for (auto child: node->getChildren()) {
      nodeQ.push_back(child);
    }
  }
}

void FlexGRWorker::initNets_printFNets(map<frNet*, vector<frNode*>, frBlockObjectComp> &netRoots) {
  cout << endl << "printing frNets\n";
  for (auto &[net, roots]: netRoots) {
    for (auto root: roots) {
      initNets_printFNet(root);
    }
  }
}

void FlexGRWorker::initNets_printFNet(frNode* root) {
  deque<frNode*> nodeQ;
  nodeQ.push_back(root);

  cout << "start traversing subnet of " << root->getNet()->getName() << endl;

  bool isRoot = true;

  while (!nodeQ.empty()) {
    auto node = nodeQ.front();
    nodeQ.pop_front();

    cout << "  ";
    if (node->getType() == frNodeTypeEnum::frcSteiner) {
      cout << "steiner node ";
    } else if (node->getType() == frNodeTypeEnum::frcPin) {
      cout << "pin node ";
    } else if (node->getType() == frNodeTypeEnum::frcBoundaryPin) {
      cout << "boundary pin node ";
    }
    cout << "at ";
    frPoint loc = node->getLoc();
    frLayerNum lNum = node->getLayerNum();
    cout << "(" << loc.x() << ", " << loc.y() << ") on layerNum " << lNum << endl;

    if (isRoot || node->getType() == frNodeTypeEnum::frcSteiner) {
      for (auto child: node->getChildren()) {
        nodeQ.push_back(child);
      }
    }

    isRoot = false;
  }
}

void FlexGRWorker::initGridGraph() {
  gridGraph.setCost(workerCongCost, workerHistCost);
  gridGraph.set2D(is2DRouting);
  gridGraph.init();

  initGridGraph_fromCMap();
}

// copy from global cmap
void FlexGRWorker::initGridGraph_fromCMap() {
  bool enableOutput = false;

  if (enableOutput) {
    cout << "start initGridGraph_fromCMap...\n";
  }

  auto cmap = getCMap();
  frPoint gcellIdxLL = getRouteGCellIdxLL();
  frPoint gcellIdxUR = getRouteGCellIdxUR();
  int idxLLX = gcellIdxLL.x();
  int idxLLY = gcellIdxLL.y();
  int idxURX = gcellIdxUR.x();
  int idxURY = gcellIdxUR.y();

  for (int zIdx = 0; zIdx < (int)cmap->getZMap().size(); zIdx++) {
    frLayerNum lNum = (zIdx + 1) * 2;
    if (enableOutput) {
      cout << "  layerNum = " << lNum << endl;
    }

    for (int xIdx = 0; xIdx <= (idxURX - idxLLX); xIdx++) {
      int cmapXIdx = xIdx + idxLLX;
      for (int yIdx = 0; yIdx <= (idxURY - idxLLY); yIdx++) {
        int cmapYIdx = yIdx + idxLLY;
        // copy block
        if (cmap->hasBlock(cmapXIdx, cmapYIdx, zIdx, frDirEnum::E)) {
          gridGraph.setBlock(xIdx, yIdx, zIdx, frDirEnum::E);
        } else {
          gridGraph.resetBlock(xIdx, yIdx, zIdx, frDirEnum::E);
        }
        if (cmap->hasBlock(cmapXIdx, cmapYIdx, zIdx, frDirEnum::N)) {
          gridGraph.setBlock(xIdx, yIdx, zIdx, frDirEnum::N);
        } else {
          gridGraph.resetBlock(xIdx, yIdx, zIdx, frDirEnum::N);
        }
        if (cmap->hasBlock(cmapXIdx, cmapYIdx, zIdx, frDirEnum::U)) {
          gridGraph.setBlock(xIdx, yIdx, zIdx, frDirEnum::U);
        } else {
          gridGraph.resetBlock(xIdx, yIdx, zIdx, frDirEnum::U);
        }
        // copy history cost
        gridGraph.setHistoryCost(xIdx, yIdx, zIdx, cmap->getHistoryCost(cmapXIdx, cmapYIdx, zIdx));

        // copy raw demand
        gridGraph.setRawDemand(xIdx, yIdx, zIdx, frDirEnum::E, cmap->getRawDemand(cmapXIdx, cmapYIdx, zIdx, frDirEnum::E));
        gridGraph.setRawDemand(xIdx, yIdx, zIdx, frDirEnum::N, cmap->getRawDemand(cmapXIdx, cmapYIdx, zIdx, frDirEnum::N));

        // copy supply (raw supply is only used when comparing to raw demand and supply is always integer)
        gridGraph.setSupply(xIdx, yIdx, zIdx, frDirEnum::E, cmap->getSupply(cmapXIdx, cmapYIdx, zIdx, frDirEnum::E));
        gridGraph.setSupply(xIdx, yIdx, zIdx, frDirEnum::N, cmap->getSupply(cmapXIdx, cmapYIdx, zIdx, frDirEnum::N));

        if (enableOutput) {
          cout << "GG (cmapXIdx, cmapYIdx) (rawDemandH / rawSupplyH) (rawDemandV / rawSupplyV) = (" << cmapXIdx << ", " << cmapYIdx << ") (" 
               << cmap->getRawDemand(cmapXIdx, cmapYIdx, zIdx, frDirEnum::E) << " / " << cmap->getRawSupply(cmapXIdx, cmapYIdx, zIdx, frDirEnum::E)
               << ") (" << cmap->getRawDemand(cmapXIdx, cmapYIdx, zIdx, frDirEnum::N) << " / " << cmap->getRawSupply(cmapXIdx, cmapYIdx, zIdx, frDirEnum::N)
               << ")\n";
          cout << "           (xIdx, yIdx) (rawDemandH / rawSupplyH) (rawDemandV / rawSupplyV) = (" << xIdx << ", " << yIdx << ") (" 
               << gridGraph.getRawDemand(xIdx, yIdx, zIdx, frDirEnum::E) << " / " << gridGraph.getRawSupply(xIdx, yIdx, zIdx, frDirEnum::E)
               << ") (" << gridGraph.getRawDemand(xIdx, yIdx, zIdx, frDirEnum::N) << " / " << gridGraph.getRawSupply(xIdx, yIdx, zIdx, frDirEnum::N)
               << ")\n\n";
        }
      }
    }
  }
}
