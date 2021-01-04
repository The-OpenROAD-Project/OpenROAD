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

#include "gr/FlexGR.h"
#include "db/obj/frNode.h"

using namespace std;
using namespace fr;

void FlexGRWorker::end() {
  bool enableOutput = false;

  if (enableOutput) {
    stringstream ss;
    ss <<endl <<"end GR worker (BOX) "
                <<"( " <<extBox.left()   * 1.0 / getTech()->getDBUPerUU() <<" "
                <<extBox.bottom() * 1.0 / getTech()->getDBUPerUU() <<" ) ( "
                <<extBox.right()  * 1.0 / getTech()->getDBUPerUU() <<" "
                <<extBox.top()    * 1.0 / getTech()->getDBUPerUU() <<" )" <<endl;
    cout <<ss.str() <<flush;
  }
  // if (enableOutput) {
  //   stringstream ss;
  //   ss <<endl <<"end GR worker (BOX) "
  //               <<"( " <<extBox.left() <<" "
  //               <<extBox.bottom()  <<" ) ( "
  //               <<extBox.right()   <<" "
  //               <<extBox.top()     <<" )" <<endl
  //               << " ( " <<routeBox.left() <<" "
  //               <<routeBox.bottom()  <<" ) ( "
  //               <<routeBox.right()   <<" "
  //               <<routeBox.top()     <<" )" <<endl
  //               << " ( " <<routeGCellIdxLL.x() <<" "
  //               <<routeGCellIdxLL.y()  <<" ) ( "
  //               <<routeGCellIdxUR.x()   <<" "
  //               <<routeGCellIdxUR.y()     <<" )" <<endl;
  //   cout <<ss.str() <<flush;
  // }

  set<frNet*, frBlockObjectComp> modNets;
  endGetModNets(modNets);
  endRemoveNets(modNets);
  endAddNets(modNets);
  // boundaries are always splitted, need to stitch boundaries
  endStitchBoundary();
  endWriteBackCMap();

  cleanup();
}

void FlexGRWorker::endGetModNets(set<frNet*, frBlockObjectComp> &modNets) {
  for (auto &net: nets) {
    if (net->isModified()) {
      modNets.insert(net->getFrNet());
    }
  }
  // change modified flag to true if another subnet get routed
  for (auto &net: nets) {
    if (!net->isModified() && modNets.find(net->getFrNet()) != modNets.end()) {
      net->setModified(true);
    }
  }
}

void FlexGRWorker::endRemoveNets(const set<frNet*, frBlockObjectComp> &modNets) {
  endRemoveNets_objs(modNets);
  endRemoveNets_nodes(modNets);
}

void FlexGRWorker::endRemoveNets_objs(const set<frNet*, frBlockObjectComp> &modNets) {
  // remove pathSeg and via (all nets based)
  vector<grBlockObject*> result;
  // if a pathSeg or a via belongs to a modified net, as long as it touches routeBox
  // we need to remove it (will remove all the way to true pin and boundary pin)
  getRegionQuery()->queryGRObj(getRouteBox(), result);
  for (auto rptr: result) {
    if (rptr->typeId() == grcPathSeg) {
      auto cptr = static_cast<grPathSeg*>(rptr);
      if (cptr->hasNet()) {
        if (modNets.find(cptr->getNet()) != modNets.end()) {
          endRemoveNets_pathSeg(cptr);
        }
      } else {
        cout <<"Error: endRemoveNet hasNet() empty" <<endl;
      }
    } else if (rptr->typeId() == grcVia) {
      auto cptr = static_cast<grVia*>(rptr);
      if (cptr->hasNet()) {
        if (modNets.find(cptr->getNet()) != modNets.end()) {
          endRemoveNets_via(cptr);
        }
      } else {
        cout <<"Error: endRemoveNet hasNet() empty" <<endl;
      }
    } else {
      cout <<"Error: endRemoveNets unsupported type" <<endl;
    }
  }
}

void FlexGRWorker::endRemoveNets_pathSeg(grPathSeg* pathSeg) {
  auto net = pathSeg->getNet();
  getRegionQuery()->removeGRObj(pathSeg);
  net->removeGRShape(pathSeg);
}

void FlexGRWorker::endRemoveNets_via(grVia* via) {
  auto net = via->getNet();
  getRegionQuery()->removeGRObj(via);
  net->removeGRVia(via);
}

void FlexGRWorker::endRemoveNets_nodes(const set<frNet*, frBlockObjectComp> &modNets) {
  for (auto fnet: modNets) {
    for (auto net: owner2nets[fnet]) {
      endRemoveNets_nodes_net(net, fnet);
    }
  }
}

void FlexGRWorker::endRemoveNets_nodes_net(grNet* net, frNet* fnet) {
  auto frRoot = net->getFrRoot();

  deque<frNode*> nodeQ;
  nodeQ.push_back(frRoot);
  bool isRoot = true;
  while (!nodeQ.empty()) {
    auto node = nodeQ.front();
    nodeQ.pop_front();

    if (isRoot || node->getType() == frNodeTypeEnum::frcSteiner) {
      for (auto child: node->getChildren()) {
        nodeQ.push_back(child);
      }
    }

    if (node->getType() == frNodeTypeEnum::frcBoundaryPin || node->getType() == frNodeTypeEnum::frcPin) {
      // boundary pin or real pin, only need to remove connection and connFig
      // no need to remove the node itself
      ;
    } else {
      // remove node
      fnet->removeNode(node);
    }

    isRoot = false;
  }
  
  // remove connection for pin nodes
  auto &gr2FrPinNode = net->getGR2FrPinNode();
  for (auto &[gNode, fNode]: gr2FrPinNode) {
    if (gNode == net->getRoot()) {
      fNode->clearChildren();
    } else {
      fNode->setParent(nullptr);
    }
  }

}

// void FlexGRWorker::endRemoveNets(set<frNet*, frBlockObjectComp> &modNets) {
//   set<frNode*> routeNodes;
//   // remove pathSeg and via (all nets based)
//   vector<grBlockObject*> result;
//   getRegionQuery()->queryGRObj(getRouteBox(), result);
//   for (auto rptr: result) {
//     if (rptr->typeId() == grcPathSeg) {
//       auto cptr = static_cast<grPathSeg*>(rptr);
//       if (cptr->hasNet()) {
//         if (modNets.find(cptr->getNet()) != modNets.end()) {
//           routeNodes.insert(cptr->getChild());
//           routeNodes.insert(cptr->getParent());
//           endRemoveNets_pathSeg(cptr);
//         }
//       } else {
//         cout <<"Error: endRemoveNet hasNet() empty" <<endl;
//       }
//     } else if (rptr->typeId() == grcVia) {
//       auto cptr = static_cast<grVia*>(rptr);
//       if (cptr->hasNet()) {
//         if (modNets.find(cptr->getNet()) != modNets.end()) {
//           routeNodes.insert(cptr->getChild());
//           routeNodes.insert(cptr->getParent());
//           endRemoveNets_via(cptr);
//         }
//       } else {
//         cout <<"Error: endRemoveNet hasNet() empty" <<endl;
//       }
//     } else {
//       cout <<"Error: endRemoveNets unsupported type" <<endl;
//     }
//   }
//   // remove nodes (all nets based)
//   for (auto routeNode: routeNodes) {
//     endRemoveNets_node(routeNode);
//   }
// }

// void FlexGRWorker::endRemoveNet_breakBound(frNet* net) {
//   for (auto &[locLayerPair, grNodes]: owner2extBoundPtNodes[net]) {
//     auto loc = locLayerPair.first;
//     auto lNum = locLayerPair.second;

//     vector<frBlockObject*> result;
//     frBox queryBox(loc.x(), loc.y(), loc.x(), loc.y());
//     getRegionQuery()->queryGRObj(queryBox, lNum, result);
//     for (auto rptr: result) {
//       if (rptr->typeId() == grcPathSeg) {
//         auto cptr = static_cast<frPathSeg*>(rptr);
//         // break the pathSeg at boundary point
//         if (cptr->hasNet() && cptr->getNet() == net) {
//           endRemoveNet_breakBound_pathSeg(cptr, loc);
//         }
//       }
//     }
//   }
// }

// void FlexGRWorker::endRemoveNet_breakBound_pathSeg(grPathSeg* pathSeg, const frPoint &breakPt) {
//   // frNet
//   auto net = pathSeg->getNet();
//   auto lNum = pathSeg->getLayerNum();
//   frPoint bp, ep;
//   pathSeg->getPoints(bp, ep);
//   frNode *childNode = pathSeg->getChild();
//   frNode *parentNode = pathSeg->getParent();
//   frPoint childLoc = childNode->getLoc();
//   bool isChildBP = (childLoc == bp);

//   auto uBreakNode = make_unique<frNode>();
//   auto breakNode = uBreakNode.get();
//   breakNode->addToNet(net);
//   breakNode->setLoc(breakPt);
//   breakNode->setLayerNum(lNum);
//   breakNode->setType(frNodeTypeEnum::frcBoundaryPin);

//   // update connectivity
//   parentNode->removeChild(childNode);
//   childNode->setParent(breakNode);
//   breakNode->addChild(childNode);
//   breakNode->setParent(parentNode);
//   parentNode->addChild(breakNode);

//   net->addNode(uBreakNode);

//   // create new pathSegs
//   // child - breakPt
//   auto uPathSeg1 = make_unique<grPathSeg>();
//   auto pathSeg1 = uPathSeg1.get();
//   pathSeg1->setChild(childNode);
//   childNode->setConnFig(pathSeg1);
//   pathSeg1->setParent(breakNode);
//   pathSeg1->addToNet(net);
//   if (isChildBP) {
//     pathSeg1->setPoints(bp, breakPt);
//   } else {
//     pathSeg1->setPoints(breakPt, ep);
//   }
//   pathSeg1->setLayerNum(lNum);

//   // breakPt - parent
//   auto uPathSeg2 = make_unique<grPathSeg>();
//   auto pathSeg2 = uPathSeg2.get();
//   pathSeg2->setChild(breakNode);
//   breakNode->setConnFig(pathSeg2);
//   pathSeg2->setParent(parentNode);
//   pathSeg2->addToNet(net);
//   if (isChildBP) {
//     pathSeg2->setPoints(breakPt, ep);
//   } else {
//     pathSeg2->setPoints(bp, breakPt);
//   }
//   pathSeg2->setLayerNum(lNum);

//   // update region query
//   getRegionQuery()->removeGRObj(pathSeg);
//   getRegionQuery()->addGRObj(pathSeg1);
//   getRegionQuery()->addGRObj(pathSeg2);

//   // update net ownership
//   unique_ptr<grConnFig> uGRConnFig1(std::move(uPathSeg1));
//   unique_ptr<grConnFig> uGRConnFig2(std::move(uPathSeg2));

//   net->addGRShape(uGRConnFig1);
//   net->addGRShape(uGRConnFig2);

//   net->removeGRObj(pathSeg);

// }



// void FlexGRWorker::endRemoveNets_node(frNode* node) {
//   if (node->getType() == frNodeTypeEnum::frcBoundaryPin) {
//     frPoint parentLoc = node->getParent()->getLoc();
//     frLayerNum parentLNum = node->getParent()->getLayerNum();
//     owner2pinNodes[node->getNet()][make_pair(parentLoc, parentLNum)].push_back(node);
//   } else if (node->getType() == frNodeTypeEnum::frcPin) {
//     frPoint parentLoc = node->getParent()->getLoc();
//     frLayerNum parentLNum = node->getParent()->getLayerNum();
//     owner2pinNodes[node->getNet()][make_pair(parentLoc, parentLNum)].push_back(node);
//   }

//   if (node->getType() == frNodeTypeEnum::frcBoundaryPin || node->getType() == frNodeTypeEnum::frcPin) {
//     return;
//   }

//   // remove the node
//   auto loc = node->getLoc();
//   auto lNum = node->getLayerNum();
//   auto net = node->getNet();

//   net->removeNode(node);
// }

void FlexGRWorker::endAddNets(set<frNet*, frBlockObjectComp> &modNets) {
  for (auto fnet: modNets) {
    for (auto net: owner2nets[fnet]) {
      endAddNets_stitchRouteBound(net);
      endAddNets_addNet(net, fnet);
    }
  }
}

void FlexGRWorker::endAddNets_stitchRouteBound(grNet* net) {
  auto &pinNodePairs = net->getPinNodePairs();
  for (auto pinNodePair: pinNodePairs) {
    auto pinNode = pinNodePair.second;
    
    if (pinNode->getType() == frNodeTypeEnum::frcPin) {
      continue;
    }

    grNode *routeBoundNode = nullptr;
    if (pinNode == net->getRoot()) {
      routeBoundNode = pinNode->getChildren().front();
    } else {
      routeBoundNode = pinNode->getParent();
    }

    endAddNets_stitchRouteBound_node(routeBoundNode);

  }
}

void FlexGRWorker::endAddNets_stitchRouteBound_node(grNode* node) {
  if (node->getChildren().size() > 1) {
    return;
  }
  auto child = node->getChildren().front();
  auto parent = node->getParent();
  auto childLoc = child->getLoc();
  auto parentLoc = parent->getLoc();
  auto childLNum = child->getLayerNum();
  auto parentLNum = parent->getLayerNum();

  if (childLNum != parentLNum) {
    return;
  }
  // return if not colinear
  if (childLoc.x() != parentLoc.x() && childLoc.y() != parentLoc.y()) {
    return;
  }

  // update connectivity (directly connect parent and child)
  // no need to update grConnFig between child and parent because it will not be used
  // connFig update for frNet is purely based on node (i.e., no grConnFig copy)
  parent->removeChild(node);
  parent->addChild(child);
  child->setParent(parent);

  // remove node from net
  node->getNet()->removeNode(node);
}


// deep copy grNode to frNode
void FlexGRWorker::endAddNets_addNet(grNet* net, frNet* fnet) {
  grNode* rootNode = net->getRoot();
  auto &gr2FrPinNode = net->getGR2FrPinNode();

  // start bfs deep copy nodes from gr to fr
  deque<pair<frNode*, grNode*> > nodeQ;
  nodeQ.push_back(make_pair(nullptr, rootNode));

  while (!nodeQ.empty()) {
    auto parentFrNode = nodeQ.front().first;
    auto childGRNode = nodeQ.front().second;
    nodeQ.pop_front();

    frNode *childFrNode = nullptr;
    if (childGRNode->getType() == frNodeTypeEnum::frcBoundaryPin || childGRNode->getType() == frNodeTypeEnum::frcPin) {
      if (gr2FrPinNode.find(childGRNode) == gr2FrPinNode.end()) {
        cout << "Error: corresponding fr pin node not found\n";
      }
      childFrNode = gr2FrPinNode[childGRNode];
    } else {
      // net, loc, layerNum and type are handled by copy constructor
      auto uChildFrNode = make_unique<frNode>(*childGRNode);
      childFrNode = uChildFrNode.get();

      fnet->addNode(uChildFrNode); 
    }
    // update connectivity
    if (parentFrNode) {
      parentFrNode->addChild(childFrNode);
      childFrNode->setParent(parentFrNode);
    }

    // update connFig
    if (parentFrNode != nullptr && 
        parentFrNode->getType() != frNodeTypeEnum::frcPin && 
        childFrNode->getType() != frNodeTypeEnum::frcPin) {
      auto childLoc = childFrNode->getLoc();
      auto parentLoc = parentFrNode->getLoc();
      auto childLNum = childFrNode->getLayerNum();
      auto parentLNum = parentFrNode->getLayerNum();
      if (childLNum == parentLNum) {
        // pathSeg
        auto uPathSeg = make_unique<grPathSeg>();
        auto pathSeg = uPathSeg.get();
        pathSeg->setLayerNum(childLNum);
        if (childLoc < parentLoc) {
          pathSeg->setPoints(childLoc, parentLoc);
        } else {
          pathSeg->setPoints(parentLoc, childLoc);
        }
        pathSeg->setChild(childFrNode);
        pathSeg->setParent(parentFrNode);
        pathSeg->addToNet(fnet);

        // update region query
        getRegionQuery()->addGRObj(pathSeg);

        // update ownership
        unique_ptr<grShape> uShape(std::move(uPathSeg));
        fnet->addGRShape(uShape);

        // add to child node
        childFrNode->setConnFig(pathSeg);
      } else {
        // via
        auto uVia = make_unique<grVia>();
        auto via = uVia.get();
        via->setChild(childFrNode);
        via->setParent(parentFrNode);
        via->addToNet(fnet);
        via->setOrigin(childLoc);
        via->setViaDef(design->getTech()->getLayer((childLNum + parentLNum) / 2)->getDefaultViaDef());

        // update region query
        getRegionQuery()->addGRObj(via);

        // update ownership
        fnet->addGRVia(uVia);

        // add to child node
        childFrNode->setConnFig(via);
      }
    }

    // push grand children to queue
    for (auto grandChild: childGRNode->getChildren()) {
      nodeQ.push_back(make_pair(childFrNode, grandChild));
    }
  }
}

void FlexGRWorker::endStitchBoundary() {
  for (auto &net: nets) {
    endStitchBoundary_net(net.get());
  }
}

// grNet remembers boundary pin frNode
void FlexGRWorker::endStitchBoundary_net(grNet* net) {
  bool enableOutput = false;

  auto fnet = net->getFrNet();

  if (enableOutput) {
    cout << "grNet " << net << endl;
  }

  auto &pinNodePairs = net->getPinNodePairs();
  for (auto &pinNodePair: pinNodePairs) {
    // frNode
    auto node = pinNodePair.first;
    // auto gNode = pinNodePair.second;

    if (node->getType() != frNodeTypeEnum::frcBoundaryPin) {
      continue;
    }
    if (node->getChildren().size() > 1) {
      cout << "Error: root boundary pin has more than one child (" << net->getFrNet()->getName() << ")\n";
      auto loc = node->getLoc();
      cout << "  node loc = (" << loc.x() << ", " << loc.y() << ")\n";
      for (auto child: node->getChildren()) {
        auto loc = child->getLoc();
        cout << "    child loc = (" << loc.x() << ", " << loc.y() << ")\n";
      }
    }
    auto child = node->getChildren().front();
    auto parent = node->getParent();

    if (child->getLayerNum() != parent->getLayerNum()) {
      cout << "Error: boundary pin has different parent and child layerNum\n";
    }
    auto childLoc = child->getLoc();
    auto parentLoc = parent->getLoc();
    if (childLoc.x() != parentLoc.x() && childLoc.y() != parentLoc.y()) {
      cout << "Error: boundary pin has non-colinear parent and child loc\n";
    }
    if (enableOutput) {
      cout << "  @@@ before stitching " << fnet->getName() << " has " << fnet->getNodes().size() << " nodes"
           << " and " << fnet->getGRShapes().size() << " wires and " << fnet->getGRVias().size() << " vias\n";
    }
    if (enableOutput) {
      cout << "  stitching net " << fnet->getName() << " at (" 
           << node->getLoc().x() << ", " << node->getLoc().y() << ") on layerNum " << node->getLayerNum() << endl;
    }


    // update connectivity
    parent->removeChild(node);
    parent->addChild(child);
    child->setParent(parent);

    // update connFig
    // remove node connFig to parent
    auto vicitmConnFig = static_cast<grShape*>(node->getConnFig());
    getRegionQuery()->removeGRObj(vicitmConnFig);
    fnet->removeGRShape(vicitmConnFig);

    // extend child connfig to parent
    auto childConnFig = static_cast<grShape*>(child->getConnFig());
    getRegionQuery()->removeGRObj(childConnFig);
    auto childPathSeg = static_cast<grPathSeg*>(child->getConnFig());
    frPoint bp, ep;
    childPathSeg->getPoints(bp, ep);
    if (bp == childLoc) {
      ep.set(parentLoc);
    } else {
      // ep == childLoc
      bp.set(parentLoc);
    }
    childPathSeg->setPoints(bp, ep);
    getRegionQuery()->addGRObj(childConnFig);

    // update connFig connectivity
    childConnFig->setParent(parent);

    // remove node from fnet
    fnet->removeNode(node);

    if (enableOutput) {
      cout << "  after stitching " << fnet->getName() << " has " << fnet->getNodes().size() << " nodes"
           << " and " << fnet->getGRShapes().size() << " wires and " << fnet->getGRVias().size() << " vias\n";
    }
  }
}

void FlexGRWorker::endWriteBackCMap() {
  bool enableOutput = false;
  bool hasChange = false;

  auto cmap = getCMap();

  frPoint gcellIdxLL = getRouteGCellIdxLL();
  frPoint gcellIdxUR = getRouteGCellIdxUR();
  int idxLLX = gcellIdxLL.x();
  int idxLLY = gcellIdxLL.y();
  int idxURX = gcellIdxUR.x();
  int idxURY = gcellIdxUR.y();

  for (int zIdx = 0; zIdx < (int)cmap->getZMap().size(); zIdx++) {
    for (int xIdx = 0; xIdx <= (idxURX - idxLLX); xIdx++) {
      int cmapXIdx = xIdx + idxLLX;
      for (int yIdx = 0; yIdx <= (idxURY - idxLLY); yIdx++) {
        int cmapYIdx = yIdx + idxLLY;
        // copy history cost
        cmap->setHistoryCost(cmapXIdx, cmapYIdx, zIdx, gridGraph.getHistoryCost(xIdx, yIdx, zIdx));
        // gridGraph.setHistoryCost(xIdx, yIdx, zIdx, cmap->getHistoryCost(cmapXIdx, cmapYIdx, zIdx));

        // debug
        if (gridGraph.getRawDemand(xIdx, yIdx, zIdx, frDirEnum::E) != cmap->getRawDemand(cmapXIdx, cmapYIdx, zIdx, frDirEnum::E) ||
            gridGraph.getRawDemand(xIdx, yIdx, zIdx, frDirEnum::N) != cmap->getRawDemand(cmapXIdx, cmapYIdx, zIdx, frDirEnum::N)) {
          hasChange = true;
        }

        // copy raw demand
        cmap->setRawDemand(cmapXIdx, cmapYIdx, zIdx, frDirEnum::E, gridGraph.getRawDemand(xIdx, yIdx, zIdx, frDirEnum::E));
        cmap->setRawDemand(cmapXIdx, cmapYIdx, zIdx, frDirEnum::N, gridGraph.getRawDemand(xIdx, yIdx, zIdx, frDirEnum::N));
        // gridGraph.setRawDemand(xIdx, yIdx, zIdx, frDirEnum::E, cmap->getRawDemand(cmapXIdx, cmapYIdx, zIdx, frDirEnum::E));
        // gridGraph.setRawDemand(xIdx, yIdx, zIdx, frDirEnum::N, cmap->getRawDemand(cmapXIdx, cmapYIdx, zIdx, frDirEnum::N));

        // copy supply (raw supply is only used when comparing to raw demand and supply is always integer)
        // gridGraph.setSupply(xIdx, yIdx, zIdx, frDirEnum::E, cmap->getSupply(cmapXIdx, cmapYIdx, zIdx, frDirEnum::E));
        // gridGraph.setSupply(xIdx, yIdx, zIdx, frDirEnum::N, cmap->getSupply(cmapXIdx, cmapYIdx, zIdx, frDirEnum::N));
      }
    }
  }

  if (enableOutput) {
    if (hasChange) {
      cout << "    Congestion changed.\n";
    }
  }
}

void FlexGRWorker::cleanup() {
  nets.clear();
  nets.shrink_to_fit();
  owner2nets.clear();
  rq.cleanup();
}
