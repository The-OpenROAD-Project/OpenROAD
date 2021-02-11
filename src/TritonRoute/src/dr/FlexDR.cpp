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

#include <chrono>
#include <fstream>
#include <boost/io/ios_state.hpp>
#include "frProfileTask.h"
#include "dr/FlexDR.h"
#include "db/infra/frTime.h"
#include "dr/FlexDR_graphics.h"
#include <omp.h>

using namespace std;
using namespace fr;

FlexDR::FlexDR(frDesign* designIn, Logger* loggerIn)
  : design_(designIn), logger_(loggerIn)
{
}

FlexDR::~FlexDR()
{
}

void FlexDR::setDebug(frDebugSettings* settings, odb::dbDatabase* db)
{
  bool on = settings->debugDR;
  graphics_ = on && FlexDRGraphics::guiActive() ?
    std::make_unique<FlexDRGraphics>(settings, design_, db)
    : nullptr;
}

int FlexDRWorker::main() {
  using namespace std::chrono;
  high_resolution_clock::time_point t0 = high_resolution_clock::now();
  if (VERBOSE > 1) {
    frBox scaledBox;
    stringstream ss;
    ss <<endl <<"start DR worker (BOX) "
                <<"( " << routeBox_.left()   * 1.0 / getTech()->getDBUPerUU() <<" "
                << routeBox_.bottom() * 1.0 / getTech()->getDBUPerUU() <<" ) ( "
                << routeBox_.right()  * 1.0 / getTech()->getDBUPerUU() <<" "
                << routeBox_.top()    * 1.0 / getTech()->getDBUPerUU() <<" )" <<endl;
    cout <<ss.str() <<flush;
  }

  init();
  high_resolution_clock::time_point t1 = high_resolution_clock::now();
  route();
  high_resolution_clock::time_point t2 = high_resolution_clock::now();
  end();
  high_resolution_clock::time_point t3 = high_resolution_clock::now();

  duration<double> time_span0 = duration_cast<duration<double>>(t1 - t0);
  duration<double> time_span1 = duration_cast<duration<double>>(t2 - t1);
  duration<double> time_span2 = duration_cast<duration<double>>(t3 - t2);

  if (VERBOSE > 1) {
    stringstream ss;
    ss   <<"time (INIT/ROUTE/POST) " <<time_span0.count() <<" " 
                                     <<time_span1.count() <<" "
                                     <<time_span2.count() <<" "
                                     <<endl;
    cout <<ss.str() <<flush;
  }
  return 0;
}

int FlexDRWorker::main_mt() {
  ProfileTask profile("DR:main_mt");
  using namespace std::chrono;
  high_resolution_clock::time_point t0 = high_resolution_clock::now();
  if (VERBOSE > 1) {
    frBox scaledBox;
    stringstream ss;
    ss <<endl <<"start DR worker (BOX) "
                <<"( " << routeBox_.left()   * 1.0 / getTech()->getDBUPerUU() <<" "
                << routeBox_.bottom() * 1.0 / getTech()->getDBUPerUU() <<" ) ( "
                << routeBox_.right()  * 1.0 / getTech()->getDBUPerUU() <<" "
                << routeBox_.top()    * 1.0 / getTech()->getDBUPerUU() <<" )" <<endl;
    cout <<ss.str() <<flush;
  }
  if (graphics_) {
    graphics_->startWorker(this);
  }
  
  init();
  high_resolution_clock::time_point t1 = high_resolution_clock::now();
  if (getFixMode() != 9) {
    route();
  } else {
   route_queue();
  }
  high_resolution_clock::time_point t2 = high_resolution_clock::now();
  cleanup();
  high_resolution_clock::time_point t3 = high_resolution_clock::now();

  duration<double> time_span0 = duration_cast<duration<double>>(t1 - t0);
  duration<double> time_span1 = duration_cast<duration<double>>(t2 - t1);
  duration<double> time_span2 = duration_cast<duration<double>>(t3 - t2);

  if (VERBOSE > 1) {
    stringstream ss;
    ss   <<"time (INIT/ROUTE/POST) " <<time_span0.count() <<" " 
                                     <<time_span1.count() <<" "
                                     <<time_span2.count() <<" "
                                     <<endl;
    cout <<ss.str() <<flush;
  }
  return 0;
}

void FlexDR::initFromTA() {
  bool enableOutput = false;
  // initialize lists
  for (auto &net: getDesign()->getTopBlock()->getNets()) {
    for (auto &guide: net->getGuides()) {
      for (auto &connFig: guide->getRoutes()) {
        if (connFig->typeId() == frcPathSeg) {
          unique_ptr<frShape> ps = make_unique<frPathSeg>(*(static_cast<frPathSeg*>(connFig.get())));
          frPoint bp, ep;
          static_cast<frPathSeg*>(ps.get())->getPoints(bp, ep);
          if (ep.x() - bp.x() + ep.y() - bp.y() == 1) {
            ; // skip TA dummy segment
          } else {
            net->addShape(std::move(ps));
          }
        } else {
          cout <<"Error: initFromTA unsupported shape" <<endl;
        }
      }
    }
  }

  if (enableOutput) {
    for (auto &net: getDesign()->getTopBlock()->getNets()) {
      cout <<"net " <<net->getName() <<" has " <<net->getShapes().size() <<" shape" <<endl;
    }
  }
}

void FlexDR::initGCell2BoundaryPin() {
  bool enableOutput = false;
  // initiailize size
  frBox dieBox;
  getDesign()->getTopBlock()->getBoundaryBBox(dieBox);
  auto gCellPatterns = getDesign()->getTopBlock()->getGCellPatterns();
  auto &xgp = gCellPatterns.at(0);
  auto &ygp = gCellPatterns.at(1);
  auto tmpVec = vector<map<frNet*, set<pair<frPoint, frLayerNum> >, frBlockObjectComp> >((int)ygp.getCount());
  gcell2BoundaryPin_ = vector<vector<map<frNet*, set<pair<frPoint, frLayerNum> >, frBlockObjectComp> > >((int)xgp.getCount(), tmpVec);
  for (auto &net: getDesign()->getTopBlock()->getNets()) {
    auto netPtr = net.get();
    for (auto &guide: net->getGuides()) {
      for (auto &connFig: guide->getRoutes()) {
        if (connFig->typeId() == frcPathSeg) {
          auto ps = static_cast<frPathSeg*>(connFig.get());
          frLayerNum layerNum;
          frPoint bp, ep;
          ps->getPoints(bp, ep);
          layerNum = ps->getLayerNum();
          // skip TA dummy segment
          if (ep.x() - bp.x() + ep.y() - bp.y() == 1 || ep.x() - bp.x() + ep.y() - bp.y() == 0) {
            continue; 
          }
          frPoint idx1, idx2;
          getDesign()->getTopBlock()->getGCellIdx(bp, idx1);
          getDesign()->getTopBlock()->getGCellIdx(ep, idx2);
          // update gcell2BoundaryPin
          // horizontal
          if (bp.y() == ep.y()) {
            int x1 = idx1.x();
            int x2 = idx2.x();
            int y  = idx1.y();
            for (auto x = x1; x <= x2; ++x) {
              frBox gcellBox;
              getDesign()->getTopBlock()->getGCellBox(frPoint(x, y), gcellBox);
              frCoord leftBound  = gcellBox.left();
              frCoord rightBound = gcellBox.right();
              bool hasLeftBound  = true;
              bool hasRightBound = true;
              if (bp.x() < leftBound) {
                hasLeftBound = true;
              } else {
                hasLeftBound = false;
              }
              if (ep.x() >= rightBound) {
                hasRightBound = true;
              } else {
                hasRightBound = false;
              }
              if (hasLeftBound) {
                frPoint boundaryPt(leftBound, bp.y());
                gcell2BoundaryPin_[x][y][netPtr].insert(make_pair(boundaryPt, layerNum));
                if (enableOutput) {
                  cout << "init left boundary pin (" 
                       << boundaryPt.x() * 1.0 / getDesign()->getTopBlock()->getDBUPerUU() << ", " 
                       << boundaryPt.y() * 1.0 / getDesign()->getTopBlock()->getDBUPerUU() << ") at ("
                       << x <<", " <<y <<") " 
                       << getTech()->getLayer(layerNum)->getName() <<" "
                       << string((net == nullptr) ? "null" : net->getName()) <<"\n";
                }
              }
              if (hasRightBound) {
                frPoint boundaryPt(rightBound, ep.y());
                gcell2BoundaryPin_[x][y][netPtr].insert(make_pair(boundaryPt, layerNum));
                if (enableOutput) {
                  cout << "init right boundary pin (" 
                       << boundaryPt.x() * 1.0 / getDesign()->getTopBlock()->getDBUPerUU() << ", " 
                       << boundaryPt.y() * 1.0 / getDesign()->getTopBlock()->getDBUPerUU() << ") at ("
                       << x <<", " <<y <<") " 
                       << getTech()->getLayer(layerNum)->getName() <<" "
                       << string((net == nullptr) ? "null" : net->getName()) <<"\n";
                }
              }
            }
          } else if (bp.x() == ep.x()) {
            int x  = idx1.x();
            int y1 = idx1.y();
            int y2 = idx2.y();
            for (auto y = y1; y <= y2; ++y) {
              frBox gcellBox;
              getDesign()->getTopBlock()->getGCellBox(frPoint(x, y), gcellBox);
              frCoord bottomBound = gcellBox.bottom();
              frCoord topBound    = gcellBox.top();
              bool hasBottomBound = true;
              bool hasTopBound    = true;
              if (bp.y() < bottomBound) {
                hasBottomBound = true;
              } else {
                hasBottomBound = false;
              }
              if (ep.y() >= topBound) {
                hasTopBound = true;
              } else {
                hasTopBound = false;
              }
              if (hasBottomBound) {
                frPoint boundaryPt(bp.x(), bottomBound);
                gcell2BoundaryPin_[x][y][netPtr].insert(make_pair(boundaryPt, layerNum));
                if (enableOutput) {
                  cout << "init bottom boundary pin (" 
                       << boundaryPt.x() * 1.0 / getDesign()->getTopBlock()->getDBUPerUU() << ", " 
                       << boundaryPt.y() * 1.0 / getDesign()->getTopBlock()->getDBUPerUU() << ") at ("
                       << x <<", " <<y <<") " 
                       << getTech()->getLayer(layerNum)->getName() <<" "
                       << string((net == nullptr) ? "null" : net->getName()) <<"\n";
                }
              }
              if (hasTopBound) {
                frPoint boundaryPt(ep.x(), topBound);
                gcell2BoundaryPin_[x][y][netPtr].insert(make_pair(boundaryPt, layerNum));
                if (enableOutput) {
                  cout << "init top boundary pin (" 
                       << boundaryPt.x() * 1.0 / getDesign()->getTopBlock()->getDBUPerUU() << ", " 
                       << boundaryPt.y() * 1.0 / getDesign()->getTopBlock()->getDBUPerUU() << ") at ("
                       << x <<", " <<y <<") " 
                       << getTech()->getLayer(layerNum)->getName() <<" "
                       << string((net == nullptr) ? "null" : net->getName()) <<"\n";
                }
              }
            }
          } else {
            cout << "Error: non-orthogonal pathseg in initGCell2BoundryPin\n";
          }
        }
      }
    }
  }
}

frCoord FlexDR::init_via2viaMinLen_minimumcut1(frLayerNum lNum, frViaDef* viaDef1, frViaDef* viaDef2) {
  if (!(viaDef1 && viaDef2)) {
    return 0;
  }

  frCoord sol = 0;

  // check min len in lNum assuming pre dir routing
  bool isH = (getDesign()->getTech()->getLayer(lNum)->getDir() == frPrefRoutingDirEnum::frcHorzPrefRoutingDir);

  bool isVia1Above = false;
  frVia via1(viaDef1);
  frBox viaBox1, cutBox1;
  if (viaDef1->getLayer1Num() == lNum) {
    via1.getLayer1BBox(viaBox1);
    isVia1Above = true;
  } else {
    via1.getLayer2BBox(viaBox1);
    isVia1Above = false;
  }
  via1.getCutBBox(cutBox1);
  auto width1    = viaBox1.width();
  auto length1   = viaBox1.length();

  bool isVia2Above = false;
  frVia via2(viaDef2);
  frBox viaBox2, cutBox2;
  if (viaDef2->getLayer1Num() == lNum) {
    via2.getLayer1BBox(viaBox2);
    isVia2Above = true;
  } else {
    via2.getLayer2BBox(viaBox2);
    isVia2Above = false;
  }
  via2.getCutBBox(cutBox2);
  auto width2    = viaBox2.width();
  auto length2   = viaBox2.length();

  for (auto &con: getDesign()->getTech()->getLayer(lNum)->getMinimumcutConstraints()) {
    if ((!con->hasLength() || (con->hasLength() && length1 > con->getLength())) && 
        width1 > con->getWidth()) {
      bool checkVia2 = false;
      if (!con->hasConnection()) {
        checkVia2 = true;
      } else {
        if (con->getConnection() == frMinimumcutConnectionEnum::FROMABOVE && isVia2Above) {
          checkVia2 = true;
        } else if (con->getConnection() == frMinimumcutConnectionEnum::FROMBELOW && !isVia2Above) {
          checkVia2 = true;
        }
      }
      if (!checkVia2) {
        continue;
      }
      if (isH) {
        sol = max(sol, (con->hasLength() ? con->getDistance() : 0) + 
                       max(cutBox2.right() - 0 + 0 - viaBox1.left(), 
                           viaBox1.right() - 0 + 0 - cutBox2.left()));
      } else {
        sol = max(sol, (con->hasLength() ? con->getDistance() : 0) + 
                       max(cutBox2.top() - 0 + 0 - viaBox1.bottom(), 
                           viaBox1.top() - 0 + 0 - cutBox2.bottom()));
      }
    } 
    // check via1cut to via2metal
    if ((!con->hasLength() || (con->hasLength() && length2 > con->getLength())) && 
        width2 > con->getWidth()) {
      bool checkVia1 = false;
      if (!con->hasConnection()) {
        checkVia1 = true;
      } else {
        if (con->getConnection() == frMinimumcutConnectionEnum::FROMABOVE && isVia1Above) {
          checkVia1 = true;
        } else if (con->getConnection() == frMinimumcutConnectionEnum::FROMBELOW && !isVia1Above) {
          checkVia1 = true;
        }
      }
      if (!checkVia1) {
        continue;
      }
      if (isH) {
        sol = max(sol, (con->hasLength() ? con->getDistance() : 0) + 
                       max(cutBox1.right() - 0 + 0 - viaBox2.left(), 
                           viaBox2.right() - 0 + 0 - cutBox1.left()));
      } else {
        sol = max(sol, (con->hasLength() ? con->getDistance() : 0) + 
                       max(cutBox1.top() - 0 + 0 - viaBox2.bottom(), 
                           viaBox2.top() - 0 + 0 - cutBox1.bottom()));
      }
    }
  }

  return sol;
}

bool FlexDR::init_via2viaMinLen_minimumcut2(frLayerNum lNum, frViaDef* viaDef1, frViaDef* viaDef2) {
  if (!(viaDef1 && viaDef2)) {
    return true;
  }
  // skip if same-layer via
  if (viaDef1 == viaDef2) {
    return true;
  }

  bool sol = true;

  bool isVia1Above = false;
  frVia via1(viaDef1);
  frBox viaBox1, cutBox1;
  if (viaDef1->getLayer1Num() == lNum) {
    via1.getLayer1BBox(viaBox1);
    isVia1Above = true;
  } else {
    via1.getLayer2BBox(viaBox1);
    isVia1Above = false;
  }
  via1.getCutBBox(cutBox1);
  auto width1    = viaBox1.width();

  bool isVia2Above = false;
  frVia via2(viaDef2);
  frBox viaBox2, cutBox2;
  if (viaDef2->getLayer1Num() == lNum) {
    via2.getLayer1BBox(viaBox2);
    isVia2Above = true;
  } else {
    via2.getLayer2BBox(viaBox2);
    isVia2Above = false;
  }
  via2.getCutBBox(cutBox2);
  auto width2    = viaBox2.width();

  for (auto &con: getDesign()->getTech()->getLayer(lNum)->getMinimumcutConstraints()) {
    if (con->hasLength()) {
      continue;
    }
    // check via2cut to via1metal
    if (width1 > con->getWidth()) {
      bool checkVia2 = false;
      if (!con->hasConnection()) {
        checkVia2 = true;
      } else {
        // has length rule
        if (con->getConnection() == frMinimumcutConnectionEnum::FROMABOVE && isVia2Above) {
          checkVia2 = true;
        } else if (con->getConnection() == frMinimumcutConnectionEnum::FROMBELOW && !isVia2Above) {
          checkVia2 = true;
        }
      }
      if (!checkVia2) {
        continue;
      }
      sol = false;
      break;
    } 
    // check via1cut to via2metal
    if (width2 > con->getWidth()) {
      bool checkVia1 = false;
      if (!con->hasConnection()) {
        checkVia1 = true;
      } else {
        if (con->getConnection() == frMinimumcutConnectionEnum::FROMABOVE && isVia1Above) {
          checkVia1 = true;
        } else if (con->getConnection() == frMinimumcutConnectionEnum::FROMBELOW && !isVia1Above) {
          checkVia1 = true;
        }
      }
      if (!checkVia1) {
        continue;
      }
      sol = false;
      break;
    }
  }
  return sol;
}

frCoord FlexDR::init_via2viaMinLen_minSpc(frLayerNum lNum, frViaDef* viaDef1, frViaDef* viaDef2) {
  if (!(viaDef1 && viaDef2)) {
    //cout <<"hehehehehe" <<endl;
    return 0;
  }

  frCoord sol = 0;

  // check min len in lNum assuming pre dir routing
  bool isH = (getDesign()->getTech()->getLayer(lNum)->getDir() == frPrefRoutingDirEnum::frcHorzPrefRoutingDir);
  frCoord defaultWidth = getDesign()->getTech()->getLayer(lNum)->getWidth();

  frVia via1(viaDef1);
  frBox viaBox1;
  if (viaDef1->getLayer1Num() == lNum) {
    via1.getLayer1BBox(viaBox1);
  } else {
    via1.getLayer2BBox(viaBox1);
  }
  auto width1    = viaBox1.width();
  bool isVia1Fat = isH ? (viaBox1.top() - viaBox1.bottom() > defaultWidth) : (viaBox1.right() - viaBox1.left() > defaultWidth);
  auto prl1      = isH ? (viaBox1.top() - viaBox1.bottom()) : (viaBox1.right() - viaBox1.left());

  frVia via2(viaDef2);
  frBox viaBox2;
  if (viaDef2->getLayer1Num() == lNum) {
    via2.getLayer1BBox(viaBox2);
  } else {
    via2.getLayer2BBox(viaBox2);
  }
  auto width2    = viaBox2.width();
  bool isVia2Fat = isH ? (viaBox2.top() - viaBox2.bottom() > defaultWidth) : (viaBox2.right() - viaBox2.left() > defaultWidth);
  auto prl2      = isH ? (viaBox2.top() - viaBox2.bottom()) : (viaBox2.right() - viaBox2.left());

  frCoord reqDist = 0;
  if (isVia1Fat && isVia2Fat) {
    auto con = getDesign()->getTech()->getLayer(lNum)->getMinSpacing();
    if (con) {
      if (con->typeId() == frConstraintTypeEnum::frcSpacingConstraint) {
        reqDist = static_cast<frSpacingConstraint*>(con)->getMinSpacing();
      } else if (con->typeId() == frConstraintTypeEnum::frcSpacingTablePrlConstraint) {
        reqDist = static_cast<frSpacingTablePrlConstraint*>(con)->find(max(width1, width2), min(prl1, prl2));
      } else if (con->typeId() == frConstraintTypeEnum::frcSpacingTableTwConstraint) {
        reqDist = static_cast<frSpacingTableTwConstraint*>(con)->find(width1, width2, min(prl1, prl2));
      }
    }
    if (isH) {
      reqDist += max((viaBox1.right() - 0), (0 - viaBox1.left()));
      reqDist += max((viaBox2.right() - 0), (0 - viaBox2.left()));
    } else {
      reqDist += max((viaBox1.top() - 0), (0 - viaBox1.bottom()));
      reqDist += max((viaBox2.top() - 0), (0 - viaBox2.bottom()));
    }
    sol = max(sol, reqDist);
  }

  // check min len in layer2 if two vias are in same layer
  if (viaDef1 != viaDef2) {
    return sol;
  }

  if (viaDef1->getLayer1Num() == lNum) {
    via1.getLayer2BBox(viaBox1);
    lNum = lNum + 2;
  } else {
    via1.getLayer1BBox(viaBox1);
    lNum = lNum - 2;
  }
  width1    = viaBox1.width();
  prl1      = isH ? (viaBox1.top() - viaBox1.bottom()) : (viaBox1.right() - viaBox1.left());
  reqDist   = 0;
  auto con = getDesign()->getTech()->getLayer(lNum)->getMinSpacing();
  if (con) {
    if (con->typeId() == frConstraintTypeEnum::frcSpacingConstraint) {
      reqDist = static_cast<frSpacingConstraint*>(con)->getMinSpacing();
    } else if (con->typeId() == frConstraintTypeEnum::frcSpacingTablePrlConstraint) {
      reqDist = static_cast<frSpacingTablePrlConstraint*>(con)->find(max(width1, width2), prl1);
    } else if (con->typeId() == frConstraintTypeEnum::frcSpacingTableTwConstraint) {
      reqDist = static_cast<frSpacingTableTwConstraint*>(con)->find(width1, width2, prl1);
    }
  }
  if (isH) {
    reqDist += (viaBox1.right() - 0) + (0 - viaBox1.left());
  } else {
    reqDist += (viaBox1.top() - 0) + (0 - viaBox1.bottom());
  }
  sol = max(sol, reqDist);
  
  return sol;
}

void FlexDR::init_via2viaMinLen() {
  //bool enableOutput = false;
  bool enableOutput = true;
  auto bottomLayerNum = getDesign()->getTech()->getBottomLayerNum();
  auto topLayerNum    = getDesign()->getTech()->getTopLayerNum();
  for (auto lNum = bottomLayerNum; lNum <= topLayerNum; lNum++) {
    if (getDesign()->getTech()->getLayer(lNum)->getType() != frLayerTypeEnum::ROUTING) {
      continue;
    }
    vector<frCoord> via2viaMinLenTmp(4, 0);
    vector<bool>    via2viaZeroLen(4, true);
    via2viaMinLen_.push_back(make_pair(via2viaMinLenTmp, via2viaZeroLen));
  }
  // check prl
  int i = 0;
  for (auto lNum = bottomLayerNum; lNum <= topLayerNum; lNum++) {
    if (getDesign()->getTech()->getLayer(lNum)->getType() != frLayerTypeEnum::ROUTING) {
      continue;
    }
    frViaDef* downVia = nullptr;
    frViaDef* upVia   = nullptr;
    if (getDesign()->getTech()->getBottomLayerNum() <= lNum - 1) {
      downVia = getDesign()->getTech()->getLayer(lNum - 1)->getDefaultViaDef();
    }
    if (getDesign()->getTech()->getTopLayerNum() >= lNum + 1) {
      upVia = getDesign()->getTech()->getLayer(lNum + 1)->getDefaultViaDef();
    }
    (via2viaMinLen_[i].first)[0] = max((via2viaMinLen_[i].first)[0], init_via2viaMinLen_minSpc(lNum, downVia, downVia));
    (via2viaMinLen_[i].first)[1] = max((via2viaMinLen_[i].first)[1], init_via2viaMinLen_minSpc(lNum, downVia, upVia));
    (via2viaMinLen_[i].first)[2] = max((via2viaMinLen_[i].first)[2], init_via2viaMinLen_minSpc(lNum, upVia, downVia));
    (via2viaMinLen_[i].first)[3] = max((via2viaMinLen_[i].first)[3], init_via2viaMinLen_minSpc(lNum, upVia, upVia));
    if (enableOutput) {
      cout <<"initVia2ViaMinLen_minSpc " <<getDesign()->getTech()->getLayer(lNum)->getName()
           <<" (d2d, d2u, u2d, u2u) = (" 
           <<(via2viaMinLen_[i].first)[0] <<", "
           <<(via2viaMinLen_[i].first)[1] <<", "
           <<(via2viaMinLen_[i].first)[2] <<", "
           <<(via2viaMinLen_[i].first)[3] <<")" <<endl;
    }
    i++;
  }

  // check minimumcut
  i = 0;
  for (auto lNum = bottomLayerNum; lNum <= topLayerNum; lNum++) {
    if (getDesign()->getTech()->getLayer(lNum)->getType() != frLayerTypeEnum::ROUTING) {
      continue;
    }
    frViaDef* downVia = nullptr;
    frViaDef* upVia   = nullptr;
    if (getDesign()->getTech()->getBottomLayerNum() <= lNum - 1) {
      downVia = getDesign()->getTech()->getLayer(lNum - 1)->getDefaultViaDef();
    }
    if (getDesign()->getTech()->getTopLayerNum() >= lNum + 1) {
      upVia = getDesign()->getTech()->getLayer(lNum + 1)->getDefaultViaDef();
    }
    vector<frCoord> via2viaMinLenTmp(4, 0);
    (via2viaMinLen_[i].first)[0] = max((via2viaMinLen_[i].first)[0], init_via2viaMinLen_minimumcut1(lNum, downVia, downVia));
    (via2viaMinLen_[i].first)[1] = max((via2viaMinLen_[i].first)[1], init_via2viaMinLen_minimumcut1(lNum, downVia, upVia));
    (via2viaMinLen_[i].first)[2] = max((via2viaMinLen_[i].first)[2], init_via2viaMinLen_minimumcut1(lNum, upVia, downVia));
    (via2viaMinLen_[i].first)[3] = max((via2viaMinLen_[i].first)[3], init_via2viaMinLen_minimumcut1(lNum, upVia, upVia));
    (via2viaMinLen_[i].second)[0] = (via2viaMinLen_[i].second)[0] && init_via2viaMinLen_minimumcut2(lNum, downVia, downVia);
    (via2viaMinLen_[i].second)[1] = (via2viaMinLen_[i].second)[1] && init_via2viaMinLen_minimumcut2(lNum, downVia, upVia);
    (via2viaMinLen_[i].second)[2] = (via2viaMinLen_[i].second)[2] && init_via2viaMinLen_minimumcut2(lNum, upVia, downVia);
    (via2viaMinLen_[i].second)[3] = (via2viaMinLen_[i].second)[3] && init_via2viaMinLen_minimumcut2(lNum, upVia, upVia);
    if (enableOutput) {
      cout <<"initVia2ViaMinLen_minimumcut " <<getDesign()->getTech()->getLayer(lNum)->getName()
           <<" (d2d, d2u, u2d, u2u) = (" 
           <<(via2viaMinLen_[i].first)[0] <<", "
           <<(via2viaMinLen_[i].first)[1] <<", "
           <<(via2viaMinLen_[i].first)[2] <<", "
           <<(via2viaMinLen_[i].first)[3] <<")" <<endl;
      cout <<"initVia2ViaMinLen_minimumcut " <<getDesign()->getTech()->getLayer(lNum)->getName()
           <<" zerolen (b, b, b, b) = (" 
           <<(via2viaMinLen_[i].second)[0] <<", "
           <<(via2viaMinLen_[i].second)[1] <<", "
           <<(via2viaMinLen_[i].second)[2] <<", "
           <<(via2viaMinLen_[i].second)[3] <<")" 
           <<endl;
    }
    i++;
  }
}

frCoord FlexDR::init_via2viaMinLenNew_minimumcut1(frLayerNum lNum, frViaDef* viaDef1, frViaDef* viaDef2, bool isCurrDirY) {
  if (!(viaDef1 && viaDef2)) {
    return 0;
  }

  frCoord sol = 0;

  // check min len in lNum assuming pre dir routing
  bool isCurrDirX = !isCurrDirY;

  bool isVia1Above = false;
  frVia via1(viaDef1);
  frBox viaBox1, cutBox1;
  if (viaDef1->getLayer1Num() == lNum) {
    via1.getLayer1BBox(viaBox1);
    isVia1Above = true;
  } else {
    via1.getLayer2BBox(viaBox1);
    isVia1Above = false;
  }
  via1.getCutBBox(cutBox1);
  auto width1    = viaBox1.width();
  auto length1   = viaBox1.length();

  bool isVia2Above = false;
  frVia via2(viaDef2);
  frBox viaBox2, cutBox2;
  if (viaDef2->getLayer1Num() == lNum) {
    via2.getLayer1BBox(viaBox2);
    isVia2Above = true;
  } else {
    via2.getLayer2BBox(viaBox2);
    isVia2Above = false;
  }
  via2.getCutBBox(cutBox2);
  auto width2    = viaBox2.width();
  auto length2   = viaBox2.length();

  for (auto &con: getDesign()->getTech()->getLayer(lNum)->getMinimumcutConstraints()) {
    // check via2cut to via1metal
    // no length OR metal1 shape satisfies --> check via2
    if ((!con->hasLength() || (con->hasLength() && length1 > con->getLength())) && 
        width1 > con->getWidth()) {
      bool checkVia2 = false;
      if (!con->hasConnection()) {
        checkVia2 = true;
      } else {
        if (con->getConnection() == frMinimumcutConnectionEnum::FROMABOVE && isVia2Above) {
          checkVia2 = true;
        } else if (con->getConnection() == frMinimumcutConnectionEnum::FROMBELOW && !isVia2Above) {
          checkVia2 = true;
        }
      }
      if (!checkVia2) {
        continue;
      }
      if (isCurrDirX) {
        sol = max(sol, (con->hasLength() ? con->getDistance() : 0) + 
                       max(cutBox2.right() - 0 + 0 - viaBox1.left(), 
                           viaBox1.right() - 0 + 0 - cutBox2.left()));
      } else {
        sol = max(sol, (con->hasLength() ? con->getDistance() : 0) + 
                       max(cutBox2.top() - 0 + 0 - viaBox1.bottom(), 
                           viaBox1.top() - 0 + 0 - cutBox2.bottom()));
      }
    } 
    // check via1cut to via2metal
    if ((!con->hasLength() || (con->hasLength() && length2 > con->getLength())) && 
        width2 > con->getWidth()) {
      bool checkVia1 = false;
      if (!con->hasConnection()) {
        checkVia1 = true;
      } else {
        if (con->getConnection() == frMinimumcutConnectionEnum::FROMABOVE && isVia1Above) {
          checkVia1 = true;
        } else if (con->getConnection() == frMinimumcutConnectionEnum::FROMBELOW && !isVia1Above) {
          checkVia1 = true;
        }
      }
      if (!checkVia1) {
        continue;
      }
      if (isCurrDirX) {
        sol = max(sol, (con->hasLength() ? con->getDistance() : 0) + 
                       max(cutBox1.right() - 0 + 0 - viaBox2.left(), 
                           viaBox2.right() - 0 + 0 - cutBox1.left()));
      } else {
        sol = max(sol, (con->hasLength() ? con->getDistance() : 0) + 
                       max(cutBox1.top() - 0 + 0 - viaBox2.bottom(), 
                           viaBox2.top() - 0 + 0 - cutBox1.bottom()));
      }
    }
  }

  return sol;
}

frCoord FlexDR::init_via2viaMinLenNew_minSpc(frLayerNum lNum, frViaDef* viaDef1, frViaDef* viaDef2, bool isCurrDirY) {
  if (!(viaDef1 && viaDef2)) {
    return 0;
  }

  frCoord sol = 0;

  // check min len in lNum assuming pre dir routing
  bool isCurrDirX = !isCurrDirY;
  frCoord defaultWidth = getDesign()->getTech()->getLayer(lNum)->getWidth();

  frVia via1(viaDef1);
  frBox viaBox1;
  if (viaDef1->getLayer1Num() == lNum) {
    via1.getLayer1BBox(viaBox1);
  } else {
    via1.getLayer2BBox(viaBox1);
  }
  auto width1    = viaBox1.width();
  bool isVia1Fat = isCurrDirX ? (viaBox1.top() - viaBox1.bottom() > defaultWidth) : (viaBox1.right() - viaBox1.left() > defaultWidth);
  auto prl1      = isCurrDirX ? (viaBox1.top() - viaBox1.bottom()) : (viaBox1.right() - viaBox1.left());

  frVia via2(viaDef2);
  frBox viaBox2;
  if (viaDef2->getLayer1Num() == lNum) {
    via2.getLayer1BBox(viaBox2);
  } else {
    via2.getLayer2BBox(viaBox2);
  }
  auto width2    = viaBox2.width();
  bool isVia2Fat = isCurrDirX ? (viaBox2.top() - viaBox2.bottom() > defaultWidth) : (viaBox2.right() - viaBox2.left() > defaultWidth);
  auto prl2      = isCurrDirX ? (viaBox2.top() - viaBox2.bottom()) : (viaBox2.right() - viaBox2.left());

  frCoord reqDist = 0;
  if (isVia1Fat && isVia2Fat) {
    auto con = getDesign()->getTech()->getLayer(lNum)->getMinSpacing();
    if (con) {
      if (con->typeId() == frConstraintTypeEnum::frcSpacingConstraint) {
        reqDist = static_cast<frSpacingConstraint*>(con)->getMinSpacing();
      } else if (con->typeId() == frConstraintTypeEnum::frcSpacingTablePrlConstraint) {
        reqDist = static_cast<frSpacingTablePrlConstraint*>(con)->find(max(width1, width2), min(prl1, prl2));
      } else if (con->typeId() == frConstraintTypeEnum::frcSpacingTableTwConstraint) {
        reqDist = static_cast<frSpacingTableTwConstraint*>(con)->find(width1, width2, min(prl1, prl2));
      }
    }
    if (isCurrDirX) {
      reqDist += max((viaBox1.right() - 0), (0 - viaBox1.left()));
      reqDist += max((viaBox2.right() - 0), (0 - viaBox2.left()));
    } else {
      reqDist += max((viaBox1.top() - 0), (0 - viaBox1.bottom()));
      reqDist += max((viaBox2.top() - 0), (0 - viaBox2.bottom()));
    }
    sol = max(sol, reqDist);
  }

  // check min len in layer2 if two vias are in same layer
  if (viaDef1 != viaDef2) {
    return sol;
  }

  if (viaDef1->getLayer1Num() == lNum) {
    via1.getLayer2BBox(viaBox1);
    lNum = lNum + 2;
  } else {
    via1.getLayer1BBox(viaBox1);
    lNum = lNum - 2;
  }
  width1    = viaBox1.width();
  prl1      = isCurrDirX ? (viaBox1.top() - viaBox1.bottom()) : (viaBox1.right() - viaBox1.left());
  reqDist   = 0;
  auto con = getDesign()->getTech()->getLayer(lNum)->getMinSpacing();
  if (con) {
    if (con->typeId() == frConstraintTypeEnum::frcSpacingConstraint) {
      reqDist = static_cast<frSpacingConstraint*>(con)->getMinSpacing();
    } else if (con->typeId() == frConstraintTypeEnum::frcSpacingTablePrlConstraint) {
      reqDist = static_cast<frSpacingTablePrlConstraint*>(con)->find(max(width1, width2), prl1);
    } else if (con->typeId() == frConstraintTypeEnum::frcSpacingTableTwConstraint) {
      reqDist = static_cast<frSpacingTableTwConstraint*>(con)->find(width1, width2, prl1);
    }
  }
  if (isCurrDirX) {
    reqDist += (viaBox1.right() - 0) + (0 - viaBox1.left());
  } else {
    reqDist += (viaBox1.top() - 0) + (0 - viaBox1.bottom());
  }
  sol = max(sol, reqDist);
  
  return sol;
}

frCoord FlexDR::init_via2viaMinLenNew_cutSpc(frLayerNum lNum, frViaDef* viaDef1, frViaDef* viaDef2, bool isCurrDirY) {
  if (!(viaDef1 && viaDef2)) {
    return 0;
  }

  frCoord sol = 0;

  // check min len in lNum assuming pre dir routing
  frVia via1(viaDef1);
  frBox viaBox1, cutBox1;
  if (viaDef1->getLayer1Num() == lNum) {
    via1.getLayer1BBox(viaBox1);
  } else {
    via1.getLayer2BBox(viaBox1);
  }
  via1.getCutBBox(cutBox1);

  frVia via2(viaDef2);
  frBox viaBox2, cutBox2;
  if (viaDef2->getLayer1Num() == lNum) {
    via2.getLayer1BBox(viaBox2);
  } else {
    via2.getLayer2BBox(viaBox2);
  }
  via2.getCutBBox(cutBox2);

  // same layer (use samenet rule if exist, otherwise use diffnet rule)
  if (viaDef1->getCutLayerNum() == viaDef2->getCutLayerNum()) {
    auto samenetCons = getDesign()->getTech()->getLayer(viaDef1->getCutLayerNum())->getCutSpacing(true);
    auto diffnetCons = getDesign()->getTech()->getLayer(viaDef1->getCutLayerNum())->getCutSpacing(false);
    if (!samenetCons.empty()) {
      // check samenet spacing rule if exists
      for (auto con: samenetCons) {
        if (con == nullptr) {
          continue;
        }
        // filter rule, assuming default via will never trigger cutArea
        if (con->hasSecondLayer() || con->isAdjacentCuts() || con->isParallelOverlap() || con->isArea() || !con->hasSameNet()) {
          continue;
        }
        auto reqSpcVal = con->getCutSpacing();
        if (!con->hasCenterToCenter()) {
          reqSpcVal += isCurrDirY ? (cutBox1.top() - cutBox1.bottom()) : (cutBox1.right() - cutBox1.left());
        }
        sol = max(sol, reqSpcVal);
      }
    } else {
      // check diffnet spacing rule
      // filter rule, assuming default via will never trigger cutArea
      for (auto con: diffnetCons) {
        if (con == nullptr) {
          continue;
        }
        if (con->hasSecondLayer() || con->isAdjacentCuts() || con->isParallelOverlap() || con->isArea() || con->hasSameNet()) {
          continue;
        }
        auto reqSpcVal = con->getCutSpacing();
        if (!con->hasCenterToCenter()) {
          reqSpcVal += isCurrDirY ? (cutBox1.top() - cutBox1.bottom()) : (cutBox1.right() - cutBox1.left());
        }
        sol = max(sol, reqSpcVal);
      }
    }
  // TODO: diff layer 
  } else {
    auto layerNum1 = viaDef1->getCutLayerNum();
    auto layerNum2 = viaDef2->getCutLayerNum();
    frCutSpacingConstraint* samenetCon = nullptr;
    if (getDesign()->getTech()->getLayer(layerNum1)->hasInterLayerCutSpacing(layerNum2, true)) {
      samenetCon = getDesign()->getTech()->getLayer(layerNum1)->getInterLayerCutSpacing(layerNum2, true);
    }
    if (getDesign()->getTech()->getLayer(layerNum2)->hasInterLayerCutSpacing(layerNum1, true)) {
      if (samenetCon) {
        cout <<"Warning: duplicate diff layer samenet cut spacing, skipping cut spacing from " 
             <<layerNum2 <<" to " <<layerNum1 <<endl;
      } else {
        samenetCon = getDesign()->getTech()->getLayer(layerNum2)->getInterLayerCutSpacing(layerNum1, true);
      }
    }
    if (samenetCon == nullptr) {
      if (getDesign()->getTech()->getLayer(layerNum1)->hasInterLayerCutSpacing(layerNum2, false)) {
        samenetCon = getDesign()->getTech()->getLayer(layerNum1)->getInterLayerCutSpacing(layerNum2, false);
      }
      if (getDesign()->getTech()->getLayer(layerNum2)->hasInterLayerCutSpacing(layerNum1, false)) {
        if (samenetCon) {
          cout <<"Warning: duplicate diff layer diffnet cut spacing, skipping cut spacing from " 
               <<layerNum2 <<" to " <<layerNum1 <<endl;
        } else {
          samenetCon = getDesign()->getTech()->getLayer(layerNum2)->getInterLayerCutSpacing(layerNum1, false);
        }
      }
    }
    if (samenetCon) {
      // filter rule, assuming default via will never trigger cutArea
      auto reqSpcVal = samenetCon->getCutSpacing();
      if (reqSpcVal == 0) {
        ;
      } else {
        if (!samenetCon->hasCenterToCenter()) {
          reqSpcVal += isCurrDirY ? (cutBox1.top() - cutBox1.bottom()) : (cutBox1.right() - cutBox1.left());
        }
      }
      sol = max(sol, reqSpcVal);
    }
  }

  return sol;
}

void FlexDR::init_via2viaMinLenNew() {
  //bool enableOutput = false;
  bool enableOutput = true;
  auto bottomLayerNum = getDesign()->getTech()->getBottomLayerNum();
  auto topLayerNum    = getDesign()->getTech()->getTopLayerNum();
  for (auto lNum = bottomLayerNum; lNum <= topLayerNum; lNum++) {
    if (getDesign()->getTech()->getLayer(lNum)->getType() != frLayerTypeEnum::ROUTING) {
      continue;
    }
    vector<frCoord> via2viaMinLenTmp(8, 0);
    via2viaMinLenNew_.push_back(via2viaMinLenTmp);
  }
  // check prl
  int i = 0;
  for (auto lNum = bottomLayerNum; lNum <= topLayerNum; lNum++) {
    if (getDesign()->getTech()->getLayer(lNum)->getType() != frLayerTypeEnum::ROUTING) {
      continue;
    }
    frViaDef* downVia = nullptr;
    frViaDef* upVia   = nullptr;
    if (getDesign()->getTech()->getBottomLayerNum() <= lNum - 1) {
      downVia = getDesign()->getTech()->getLayer(lNum - 1)->getDefaultViaDef();
    }
    if (getDesign()->getTech()->getTopLayerNum() >= lNum + 1) {
      upVia = getDesign()->getTech()->getLayer(lNum + 1)->getDefaultViaDef();
    }
    via2viaMinLenNew_[i][0] = max(via2viaMinLenNew_[i][0], init_via2viaMinLenNew_minSpc(lNum, downVia, downVia, false));
    via2viaMinLenNew_[i][1] = max(via2viaMinLenNew_[i][1], init_via2viaMinLenNew_minSpc(lNum, downVia, downVia, true ));
    via2viaMinLenNew_[i][2] = max(via2viaMinLenNew_[i][2], init_via2viaMinLenNew_minSpc(lNum, downVia, upVia,   false));
    via2viaMinLenNew_[i][3] = max(via2viaMinLenNew_[i][3], init_via2viaMinLenNew_minSpc(lNum, downVia, upVia,   true ));
    via2viaMinLenNew_[i][4] = max(via2viaMinLenNew_[i][4], init_via2viaMinLenNew_minSpc(lNum, upVia,   downVia, false));
    via2viaMinLenNew_[i][5] = max(via2viaMinLenNew_[i][5], init_via2viaMinLenNew_minSpc(lNum, upVia,   downVia, true ));
    via2viaMinLenNew_[i][6] = max(via2viaMinLenNew_[i][6], init_via2viaMinLenNew_minSpc(lNum, upVia,   upVia,   false));
    via2viaMinLenNew_[i][7] = max(via2viaMinLenNew_[i][7], init_via2viaMinLenNew_minSpc(lNum, upVia,   upVia,   true ));
    if (enableOutput) {
      cout <<"initVia2ViaMinLenNew_minSpc " <<getDesign()->getTech()->getLayer(lNum)->getName()
           <<" (d2d-x, d2d-y, d2u-x, d2u-y, u2d-x, u2d-y, u2u-x, u2u-y) = (" 
           <<via2viaMinLenNew_[i][0] <<", "
           <<via2viaMinLenNew_[i][1] <<", "
           <<via2viaMinLenNew_[i][2] <<", "
           <<via2viaMinLenNew_[i][3] <<", "
           <<via2viaMinLenNew_[i][4] <<", "
           <<via2viaMinLenNew_[i][5] <<", "
           <<via2viaMinLenNew_[i][6] <<", "
           <<via2viaMinLenNew_[i][7] <<")" <<endl;
    }
    i++;
  }

  // check minimumcut
  i = 0;
  for (auto lNum = bottomLayerNum; lNum <= topLayerNum; lNum++) {
    if (getDesign()->getTech()->getLayer(lNum)->getType() != frLayerTypeEnum::ROUTING) {
      continue;
    }
    frViaDef* downVia = nullptr;
    frViaDef* upVia   = nullptr;
    if (getDesign()->getTech()->getBottomLayerNum() <= lNum - 1) {
      downVia = getDesign()->getTech()->getLayer(lNum - 1)->getDefaultViaDef();
    }
    if (getDesign()->getTech()->getTopLayerNum() >= lNum + 1) {
      upVia = getDesign()->getTech()->getLayer(lNum + 1)->getDefaultViaDef();
    }
    via2viaMinLenNew_[i][0] = max(via2viaMinLenNew_[i][0], init_via2viaMinLenNew_minimumcut1(lNum, downVia, downVia, false));
    via2viaMinLenNew_[i][1] = max(via2viaMinLenNew_[i][1], init_via2viaMinLenNew_minimumcut1(lNum, downVia, downVia, true ));
    via2viaMinLenNew_[i][2] = max(via2viaMinLenNew_[i][2], init_via2viaMinLenNew_minimumcut1(lNum, downVia, upVia,   false));
    via2viaMinLenNew_[i][3] = max(via2viaMinLenNew_[i][3], init_via2viaMinLenNew_minimumcut1(lNum, downVia, upVia,   true ));
    via2viaMinLenNew_[i][4] = max(via2viaMinLenNew_[i][4], init_via2viaMinLenNew_minimumcut1(lNum, upVia,   downVia, false));
    via2viaMinLenNew_[i][5] = max(via2viaMinLenNew_[i][5], init_via2viaMinLenNew_minimumcut1(lNum, upVia,   downVia, true ));
    via2viaMinLenNew_[i][6] = max(via2viaMinLenNew_[i][6], init_via2viaMinLenNew_minimumcut1(lNum, upVia,   upVia,   false));
    via2viaMinLenNew_[i][7] = max(via2viaMinLenNew_[i][7], init_via2viaMinLenNew_minimumcut1(lNum, upVia,   upVia,   true ));
    if (enableOutput) {
      cout <<"initVia2ViaMinLenNew_minimumcut " <<getDesign()->getTech()->getLayer(lNum)->getName()
           <<" (d2d-x, d2d-y, d2u-x, d2u-y, u2d-x, u2d-y, u2u-x, u2u-y) = (" 
           <<via2viaMinLenNew_[i][0] <<", "
           <<via2viaMinLenNew_[i][1] <<", "
           <<via2viaMinLenNew_[i][2] <<", "
           <<via2viaMinLenNew_[i][3] <<", "
           <<via2viaMinLenNew_[i][4] <<", "
           <<via2viaMinLenNew_[i][5] <<", "
           <<via2viaMinLenNew_[i][6] <<", "
           <<via2viaMinLenNew_[i][7] <<")" <<endl;
    }
    i++;
  }

  // check cut spacing
  i = 0;
  for (auto lNum = bottomLayerNum; lNum <= topLayerNum; lNum++) {
    if (getDesign()->getTech()->getLayer(lNum)->getType() != frLayerTypeEnum::ROUTING) {
      continue;
    }
    frViaDef* downVia = nullptr;
    frViaDef* upVia   = nullptr;
    if (getDesign()->getTech()->getBottomLayerNum() <= lNum - 1) {
      downVia = getDesign()->getTech()->getLayer(lNum - 1)->getDefaultViaDef();
    }
    if (getDesign()->getTech()->getTopLayerNum() >= lNum + 1) {
      upVia = getDesign()->getTech()->getLayer(lNum + 1)->getDefaultViaDef();
    }
    via2viaMinLenNew_[i][0] = max(via2viaMinLenNew_[i][0], init_via2viaMinLenNew_cutSpc(lNum, downVia, downVia, false));
    via2viaMinLenNew_[i][1] = max(via2viaMinLenNew_[i][1], init_via2viaMinLenNew_cutSpc(lNum, downVia, downVia, true ));
    via2viaMinLenNew_[i][2] = max(via2viaMinLenNew_[i][2], init_via2viaMinLenNew_cutSpc(lNum, downVia, upVia,   false));
    via2viaMinLenNew_[i][3] = max(via2viaMinLenNew_[i][3], init_via2viaMinLenNew_cutSpc(lNum, downVia, upVia,   true ));
    via2viaMinLenNew_[i][4] = max(via2viaMinLenNew_[i][4], init_via2viaMinLenNew_cutSpc(lNum, upVia,   downVia, false));
    via2viaMinLenNew_[i][5] = max(via2viaMinLenNew_[i][5], init_via2viaMinLenNew_cutSpc(lNum, upVia,   downVia, true ));
    via2viaMinLenNew_[i][6] = max(via2viaMinLenNew_[i][6], init_via2viaMinLenNew_cutSpc(lNum, upVia,   upVia,   false));
    via2viaMinLenNew_[i][7] = max(via2viaMinLenNew_[i][7], init_via2viaMinLenNew_cutSpc(lNum, upVia,   upVia,   true ));
    if (enableOutput) {
      cout <<"initVia2ViaMinLenNew_cutSpc " <<getDesign()->getTech()->getLayer(lNum)->getName()
           <<" (d2d-x, d2d-y, d2u-x, d2u-y, u2d-x, u2d-y, u2u-x, u2u-y) = (" 
           <<via2viaMinLenNew_[i][0] <<", "
           <<via2viaMinLenNew_[i][1] <<", "
           <<via2viaMinLenNew_[i][2] <<", "
           <<via2viaMinLenNew_[i][3] <<", "
           <<via2viaMinLenNew_[i][4] <<", "
           <<via2viaMinLenNew_[i][5] <<", "
           <<via2viaMinLenNew_[i][6] <<", "
           <<via2viaMinLenNew_[i][7] <<")" <<endl;
    }
    i++;
  }

}

void FlexDR::init_halfViaEncArea() {
  auto bottomLayerNum = getDesign()->getTech()->getBottomLayerNum();
  auto topLayerNum    = getDesign()->getTech()->getTopLayerNum();
  for (int i = bottomLayerNum; i <= topLayerNum; i++) {
    if (getDesign()->getTech()->getLayer(i)->getType() != frLayerTypeEnum::ROUTING) {
      continue;
    }
    if (i + 1 <= topLayerNum && getDesign()->getTech()->getLayer(i + 1)->getType() == frLayerTypeEnum::CUT) {
      auto viaDef = getTech()->getLayer(i + 1)->getDefaultViaDef();
      frVia via(viaDef);
      frBox layer1Box;
      frBox layer2Box;
      via.getLayer1BBox(layer1Box);
      via.getLayer2BBox(layer2Box);
      auto layer1HalfArea = layer1Box.width() * layer1Box.length() / 2;
      auto layer2HalfArea = layer2Box.width() * layer2Box.length() / 2;
      halfViaEncArea_.push_back(make_pair(layer1HalfArea, layer2HalfArea));
    } else {
      halfViaEncArea_.push_back(make_pair(0,0));
    }
  }
}



frCoord FlexDR::init_via2turnMinLen_minSpc(frLayerNum lNum, frViaDef* viaDef, bool isCurrDirY) {
  if (!viaDef) {
    return 0;
  }

  frCoord sol = 0;

  // check min len in lNum assuming pre dir routing
  bool isCurrDirX = !isCurrDirY;
  frCoord defaultWidth = getDesign()->getTech()->getLayer(lNum)->getWidth();

  frVia via1(viaDef);
  frBox viaBox1;
  if (viaDef->getLayer1Num() == lNum) {
    via1.getLayer1BBox(viaBox1);
  } else {
    via1.getLayer2BBox(viaBox1);
  }
  auto width1    = viaBox1.width();
  bool isVia1Fat = isCurrDirX ? (viaBox1.top() - viaBox1.bottom() > defaultWidth) : (viaBox1.right() - viaBox1.left() > defaultWidth);
  auto prl1      = isCurrDirX ? (viaBox1.top() - viaBox1.bottom()) : (viaBox1.right() - viaBox1.left());

  frCoord reqDist = 0;
  if (isVia1Fat) {
    auto con = getDesign()->getTech()->getLayer(lNum)->getMinSpacing();
    if (con) {
      if (con->typeId() == frConstraintTypeEnum::frcSpacingConstraint) {
        reqDist = static_cast<frSpacingConstraint*>(con)->getMinSpacing();
      } else if (con->typeId() == frConstraintTypeEnum::frcSpacingTablePrlConstraint) {
        reqDist = static_cast<frSpacingTablePrlConstraint*>(con)->find(max(width1, defaultWidth), prl1);
      } else if (con->typeId() == frConstraintTypeEnum::frcSpacingTableTwConstraint) {
        reqDist = static_cast<frSpacingTableTwConstraint*>(con)->find(width1, defaultWidth, prl1);
      }
    }
    if (isCurrDirX) {
      reqDist += max((viaBox1.right() - 0), (0 - viaBox1.left()));
      reqDist += defaultWidth;
    } else {
      reqDist += max((viaBox1.top() - 0), (0 - viaBox1.bottom()));
      reqDist += defaultWidth;
    }
    sol = max(sol, reqDist);
  }

  return sol;
}

frCoord FlexDR::init_via2turnMinLen_minStp(frLayerNum lNum, frViaDef* viaDef, bool isCurrDirY) {
  if (!viaDef) {
    return 0;
  }

  frCoord sol = 0;

  // check min len in lNum assuming pre dir routing
  bool isCurrDirX = !isCurrDirY;
  frCoord defaultWidth = getDesign()->getTech()->getLayer(lNum)->getWidth();

  frVia via1(viaDef);
  frBox viaBox1;
  if (viaDef->getLayer1Num() == lNum) {
    via1.getLayer1BBox(viaBox1);
  } else {
    via1.getLayer2BBox(viaBox1);
  }
  bool isVia1Fat = isCurrDirX ? (viaBox1.top() - viaBox1.bottom() > defaultWidth) : (viaBox1.right() - viaBox1.left() > defaultWidth);

  frCoord reqDist = 0;
  if (isVia1Fat) {
    auto con = getDesign()->getTech()->getLayer(lNum)->getMinStepConstraint();
    if (con && con->hasMaxEdges()) { // currently only consider maxedge violation
      reqDist = con->getMinStepLength();
      if (isCurrDirX) {
        reqDist += max((viaBox1.right() - 0), (0 - viaBox1.left()));
        reqDist += defaultWidth;
      } else {
        reqDist += max((viaBox1.top() - 0), (0 - viaBox1.bottom()));
        reqDist += defaultWidth;
      }
      sol = max(sol, reqDist);
    }
  }
  return sol;
}


void FlexDR::init_via2turnMinLen() {
  bool enableOutput = false;
  //bool enableOutput = true;
  auto bottomLayerNum = getDesign()->getTech()->getBottomLayerNum();
  auto topLayerNum    = getDesign()->getTech()->getTopLayerNum();
  for (auto lNum = bottomLayerNum; lNum <= topLayerNum; lNum++) {
    if (getDesign()->getTech()->getLayer(lNum)->getType() != frLayerTypeEnum::ROUTING) {
      continue;
    }
    vector<frCoord> via2turnMinLenTmp(4, 0);
    via2turnMinLen_.push_back(via2turnMinLenTmp);
  }
  // check prl
  int i = 0;
  for (auto lNum = bottomLayerNum; lNum <= topLayerNum; lNum++) {
    if (getDesign()->getTech()->getLayer(lNum)->getType() != frLayerTypeEnum::ROUTING) {
      continue;
    }
    frViaDef* downVia = nullptr;
    frViaDef* upVia   = nullptr;
    if (getDesign()->getTech()->getBottomLayerNum() <= lNum - 1) {
      downVia = getDesign()->getTech()->getLayer(lNum - 1)->getDefaultViaDef();
    }
    if (getDesign()->getTech()->getTopLayerNum() >= lNum + 1) {
      upVia = getDesign()->getTech()->getLayer(lNum + 1)->getDefaultViaDef();
    }
    via2turnMinLen_[i][0] = max(via2turnMinLen_[i][0], init_via2turnMinLen_minSpc(lNum, downVia, false));
    via2turnMinLen_[i][1] = max(via2turnMinLen_[i][1], init_via2turnMinLen_minSpc(lNum, downVia, true));
    via2turnMinLen_[i][2] = max(via2turnMinLen_[i][2], init_via2turnMinLen_minSpc(lNum, upVia, false));
    via2turnMinLen_[i][3] = max(via2turnMinLen_[i][3], init_via2turnMinLen_minSpc(lNum, upVia, true));
    if (enableOutput) {
      cout <<"initVia2TurnMinLen_minSpc " <<getDesign()->getTech()->getLayer(lNum)->getName()
           <<" (down->x->y, down->y->x, up->x->y, up->y->x) = (" 
           <<via2turnMinLen_[i][0] <<", "
           <<via2turnMinLen_[i][1] <<", "
           <<via2turnMinLen_[i][2] <<", "
           <<via2turnMinLen_[i][3] <<")" <<endl;
    }
    i++;
  }

  // check minstep
  i = 0;
  for (auto lNum = bottomLayerNum; lNum <= topLayerNum; lNum++) {
    if (getDesign()->getTech()->getLayer(lNum)->getType() != frLayerTypeEnum::ROUTING) {
      continue;
    }
    frViaDef* downVia = nullptr;
    frViaDef* upVia   = nullptr;
    if (getDesign()->getTech()->getBottomLayerNum() <= lNum - 1) {
      downVia = getDesign()->getTech()->getLayer(lNum - 1)->getDefaultViaDef();
    }
    if (getDesign()->getTech()->getTopLayerNum() >= lNum + 1) {
      upVia = getDesign()->getTech()->getLayer(lNum + 1)->getDefaultViaDef();
    }
    vector<frCoord> via2turnMinLenTmp(4, 0);
    via2turnMinLen_[i][0] = max(via2turnMinLen_[i][0], init_via2turnMinLen_minStp(lNum, downVia, false));
    via2turnMinLen_[i][1] = max(via2turnMinLen_[i][1], init_via2turnMinLen_minStp(lNum, downVia, true));
    via2turnMinLen_[i][2] = max(via2turnMinLen_[i][2], init_via2turnMinLen_minStp(lNum, upVia, false));
    via2turnMinLen_[i][3] = max(via2turnMinLen_[i][3], init_via2turnMinLen_minStp(lNum, upVia, true));
    if (enableOutput) {
      cout <<"initVia2TurnMinLen_minstep " <<getDesign()->getTech()->getLayer(lNum)->getName()
           <<" (down->x->y, down->y->x, up->x->y, up->y->x) = (" 
           <<via2turnMinLen_[i][0] <<", "
           <<via2turnMinLen_[i][1] <<", "
           <<via2turnMinLen_[i][2] <<", "
           <<via2turnMinLen_[i][3] <<")" <<endl;
    }
    i++;
  }
}



void FlexDR::init() {
  ProfileTask profile("DR:init");
  frTime t;
  if (VERBOSE > 0) {
    cout <<endl <<"start routing data preparation" <<endl;
  }
  initGCell2BoundaryPin();
  getRegionQuery()->initDRObj(getTech()->getLayers().size()); // first init in postProcess

  init_halfViaEncArea();
  init_via2viaMinLen();
  init_via2viaMinLenNew();
  init_via2turnMinLen();

  if (VERBOSE > 0) {
    t.print();
  }
}

void FlexDR::removeGCell2BoundaryPin() {
  gcell2BoundaryPin_.clear();
  gcell2BoundaryPin_.shrink_to_fit();
}

map<frNet*, set<pair<frPoint, frLayerNum> >, frBlockObjectComp> FlexDR::initDR_mergeBoundaryPin(int startX, int startY, int size, const frBox &routeBox) {
  map<frNet*, set<pair<frPoint, frLayerNum> >, frBlockObjectComp> bp;
  auto gCellPatterns = getDesign()->getTopBlock()->getGCellPatterns();
  auto &xgp = gCellPatterns.at(0);
  auto &ygp = gCellPatterns.at(1);
  for (int i = startX; i < (int)xgp.getCount() && i < startX + size; i++) {
    for (int j = startY; j < (int)ygp.getCount() && j < startY + size; j++) {
      auto &currBp = gcell2BoundaryPin_[i][j];
      for (auto &[net, s]: currBp) {
        for (auto &[pt, lNum]: s) {
          if (pt.x() == routeBox.left()   || pt.x() == routeBox.right() ||
              pt.y() == routeBox.bottom() || pt.y() == routeBox.top()) {
            bp[net].insert(make_pair(pt, lNum));
          }
        }
      }
    }
  }
  return bp;
}

void FlexDR::initDR(int size, bool enableDRC) {
  bool TEST = false;
  // bool TEST = true;
  //FlexGridGraph gg(getTech(), getDesign());
  ////frBox testBBox(225000, 228100, 228000, 231100); // net1702 in ispd19_test1
  //frBox testBBox(0, 0, 2000, 2000); // net1702 in ispd19_test1
  ////gg.setBBox(testBBox);
  //gg.init(testBBox);
  //gg.print();
  //exit(1);

  frTime t;

  if (VERBOSE > 0) {
    cout <<endl <<"start initial detail routing ..." <<endl;
  }
  frBox dieBox;
  getDesign()->getTopBlock()->getBoundaryBBox(dieBox);

  auto gCellPatterns = getDesign()->getTopBlock()->getGCellPatterns();
  auto &xgp = gCellPatterns.at(0);
  auto &ygp = gCellPatterns.at(1);

  int cnt = 0;
  int tot = (((int)xgp.getCount() - 1) / size + 1) * (((int)ygp.getCount() - 1) / size + 1);
  int prev_perc = 0;
  bool isExceed = false;

  int numQuickMarkers = 0;
  if (TEST) {
    //FlexDRWorker worker(getDesign());
    FlexDRWorker worker(this, logger_);
    //frBox routeBox;
    //routeBox.set(0*2000, 0*2000, 1*2000, 1*2000);
    //frCoord xl = 21 * 2000;
    //frCoord yl = 42 * 2000;
    frCoord xl = 241.92 * 2000;
    frCoord yl = 241.92 * 2000;
    //frCoord xh = 129 * 2000;
    //frCoord yh = 94.05 * 2000;
    frPoint idx;
    getDesign()->getTopBlock()->getGCellIdx(frPoint(xl, yl), idx);
    if (VERBOSE > 1) {
      cout <<"(i,j) = (" <<idx.x() <<", " <<idx.y() <<")" <<endl;
    }
    //getDesign()->getTopBlock()->getGCellBox(idx, routeBox);
    frBox routeBox1;
    getDesign()->getTopBlock()->getGCellBox(frPoint(idx.x(), idx.y()), routeBox1);
    frBox routeBox2;
    getDesign()->getTopBlock()->getGCellBox(frPoint(min((int)xgp.getCount() - 1, idx.x() + size-1), 
                                                    min((int)ygp.getCount(), idx.y() + size-1)), routeBox2);
    frBox routeBox(routeBox1.left(), routeBox1.bottom(), routeBox2.right(), routeBox2.top());
    auto bp = initDR_mergeBoundaryPin(idx.x(), idx.y(), size, routeBox);
    //routeBox.set(129*2000, 94.05*2000, 132*2000, 96.9*2000);
    worker.setRouteBox(routeBox);
    frBox extBox;
    routeBox.bloat(2000, extBox);
    frBox drcBox;
    routeBox.bloat(500, drcBox);
    worker.setRouteBox(routeBox);
    worker.setExtBox(extBox);
    worker.setDrcBox(drcBox);
    worker.setFollowGuide(false);
    worker.setCost(DRCCOST, 0, 0, 0);
    //int i = (129   * 2000 - xgp.getStartCoord()) / xgp.getSpacing();
    //int j = (94.05 * 2000 - ygp.getStartCoord()) / ygp.getSpacing();
    //worker.setDRIter(0, gcell2BoundaryPin[idx.x()][idx.y()]);
    worker.setDRIter(0, bp);
    worker.setEnableDRC(enableDRC);
    //worker.setTest(true);
    worker.main();
    cout <<"done"  <<endl <<flush;
  } else {
    vector<unique_ptr<FlexDRWorker> > uworkers;
    int batchStepX, batchStepY;

    getBatchInfo(batchStepX, batchStepY);

    vector<vector<vector<unique_ptr<FlexDRWorker> > > > workers(batchStepX * batchStepY);

    int xIdx = 0, yIdx = 0;
    // sequential init
    for (int i = 0; i < (int)xgp.getCount(); i += size) {
      for (int j = 0; j < (int)ygp.getCount(); j += size) {
        auto worker = make_unique<FlexDRWorker>(this, logger_);
        frBox routeBox1;
        getDesign()->getTopBlock()->getGCellBox(frPoint(i, j), routeBox1);
        frBox routeBox2;
        getDesign()->getTopBlock()->getGCellBox(frPoint(min((int)xgp.getCount() - 1, i + size-1), 
                                                        min((int)ygp.getCount(), j + size-1)), routeBox2);
        //frBox routeBox;
        frBox routeBox(routeBox1.left(), routeBox1.bottom(), routeBox2.right(), routeBox2.top());
        frBox extBox;
        routeBox.bloat(MTSAFEDIST, extBox);
        frBox drcBox;
        routeBox.bloat(DRCSAFEDIST, drcBox);
        worker->setRouteBox(routeBox);
        worker->setExtBox(extBox);
        worker->setDrcBox(drcBox);

        auto bp = initDR_mergeBoundaryPin(i, j, size, routeBox);
        worker->setDRIter(0, bp);
        // set boundary pin
        worker->setEnableDRC(enableDRC);
        worker->setFollowGuide(false);
        //worker->setFollowGuide(true);
        worker->setCost(DRCCOST, 0, 0, 0);
        // int workerIdx = xIdx * batchSizeY + yIdx;
        int batchIdx = (xIdx % batchStepX) * batchStepY + yIdx % batchStepY;
        // workers[batchIdx][workerIdx] = worker;
        if (workers[batchIdx].empty() || (int)workers[batchIdx].back().size() >= BATCHSIZE) {
          workers[batchIdx].push_back(vector<unique_ptr<FlexDRWorker> >());
        }
        workers[batchIdx].back().push_back(std::move(worker));

        yIdx++;
      }
      yIdx = 0;
      xIdx++;
    }

    omp_set_num_threads(MAX_THREADS);

    // parallel execution
    for (auto &workerBatch: workers) {
      for (auto &workersInBatch: workerBatch) {
        // multi thread
        #pragma omp parallel for schedule(dynamic)
        for (int i = 0; i < (int)workersInBatch.size(); i++) {
          workersInBatch[i]->main_mt();
          #pragma omp critical 
          {
            cnt++;
            if (VERBOSE > 0) {
              if (cnt * 1.0 / tot >= prev_perc / 100.0 + 0.1 && prev_perc < 90) {
                if (prev_perc == 0 && t.isExceed(0)) {
                  isExceed = true;
                }
                prev_perc += 10;
                //if (true) {
                if (isExceed) {
                  if (enableDRC) {
                    cout <<"    completing " <<prev_perc <<"% with " <<getDesign()->getTopBlock()->getNumMarkers() <<" violations" <<endl;
                  } else {
                    cout <<"    completing " <<prev_perc <<"% with " <<numQuickMarkers <<" quick violations" <<endl;
                  }
                  cout <<"    " <<t <<endl <<flush;
                }
              }
            }
          }
        }
        // single thread
        for (int i = 0; i < (int)workersInBatch.size(); i++) {
          workersInBatch[i]->end();
        }
        workersInBatch.clear();
      }
    }
    checkConnectivity();
  }
  if (VERBOSE > 0) {
    if (cnt * 1.0 / tot >= prev_perc / 100.0 + 0.1 && prev_perc >= 90) {
      if (prev_perc == 0 && t.isExceed(0)) {
        isExceed = true;
      }
      prev_perc += 10;
      //if (true) {
      if (isExceed) {
        if (enableDRC) {
          cout <<"    completing " <<prev_perc <<"% with " <<getDesign()->getTopBlock()->getNumMarkers() <<" violations" <<endl;
        } else {
          cout <<"    completing " <<prev_perc <<"% with " <<numQuickMarkers <<" quick violations" <<endl;
        }
        cout <<"    " <<t <<endl <<flush;
      }
    }
  }

  //cout <<"  number of violations = " <<numMarkers <<endl;
  removeGCell2BoundaryPin();
  numViols_.push_back(getDesign()->getTopBlock()->getNumMarkers());
  if (VERBOSE > 0) {
    if (enableDRC) {
      cout <<"  number of violations = "       <<getDesign()->getTopBlock()->getNumMarkers() <<endl;
    } else {
      cout <<"  number of quick violations = " <<numQuickMarkers <<endl;
    }
    t.print();
    cout <<flush;
  }
}



void FlexDR::getBatchInfo(int &batchStepX, int &batchStepY) {
  batchStepX = 2;
  batchStepY = 2;
}

void FlexDR::searchRepair(int iter, int size, int offset, int mazeEndIter, 
                          frUInt4 workerDRCCost, frUInt4 workerMarkerCost, 
                          frUInt4 workerMarkerBloatWidth, frUInt4 workerMarkerBloatDepth,
                          bool enableDRC, int ripupMode, bool followGuide, 
                          int fixMode, bool TEST) {
  std::string profile_name("DR:searchRepair");
  profile_name += std::to_string(iter);
  ProfileTask profile(profile_name.c_str());
  if (iter > END_ITERATION) {
    return;
  }
  if (ripupMode != 1 && getDesign()->getTopBlock()->getMarkers().size() == 0) {
    return;
  } 

  frTime t;
  //bool TEST = false;
  //bool TEST = true;
  if (VERBOSE > 0) {
    cout <<endl <<"start " <<iter;
    string suffix;
    if (iter == 1 || (iter > 20 && iter % 10 == 1)) {
      suffix = "st";
    } else if (iter == 2 || (iter > 20 && iter % 10 == 2)) {
      suffix = "nd";
    } else if (iter == 3 || (iter > 20 && iter % 10 == 3)) {
      suffix = "rd";
    } else {
      suffix = "th";
    }
    cout <<suffix <<" optimization iteration ..." <<endl;
  }
  if (graphics_) {
    graphics_->startIter(iter);
  }
  frBox dieBox;
  getDesign()->getTopBlock()->getBoundaryBBox(dieBox);
  auto gCellPatterns = getDesign()->getTopBlock()->getGCellPatterns();
  auto &xgp = gCellPatterns.at(0);
  auto &ygp = gCellPatterns.at(1);
  int numQuickMarkers = 0;
  int clipSize = size;
  int cnt = 0;
  int tot = (((int)xgp.getCount() - 1 - offset) / clipSize + 1) * (((int)ygp.getCount() - 1 - offset) / clipSize + 1);
  int prev_perc = 0;
  bool isExceed = false;
  if (TEST) {
    cout <<"search and repair test mode" <<endl <<flush;
    //FlexDRWorker worker(getDesign());
    FlexDRWorker worker(this, logger_);
    frBox routeBox;
    //frCoord xl = 148.5 * 2000;
    //frCoord yl = 570 * 2000;
    //frPoint idx;
    //getDesign()->getTopBlock()->getGCellIdx(frPoint(xl, yl), idx);
    //if (VERBOSE > 1) {
    //  cout <<"(i,j) = (" <<idx.x() <<", " <<idx.y() <<")" <<endl;
    //}
    //getDesign()->getTopBlock()->getGCellBox(idx, routeBox);
    //routeBox.set(156*2000, 108.3*2000, 177*2000, 128.25*2000);
    // routeBox.set(175*2000, 3.5*2000, 185*2000, 13.5*2000);
    // routeBox.set(0*2000, 0*2000, 200*2000, 200*2000);
    // routeBox.set(420*1000, 816.1*1000, 441*1000, 837.1*1000);
    routeBox.set(441*1000, 816.1*1000, 462*1000, 837.1*1000);
    worker.setRouteBox(routeBox);
    frBox extBox;
    frBox drcBox;
    routeBox.bloat(2000, extBox);
    routeBox.bloat(500, drcBox);
    worker.setRouteBox(routeBox);
    worker.setExtBox(extBox);
    worker.setDrcBox(drcBox);
    worker.setMazeEndIter(mazeEndIter);
    worker.setTest(true);
    //worker.setQuickDRCTest(true);
    //worker.setDRCTest(true);
    worker.setDRIter(iter);
    if (!iter) {
      // set boundary pin (need to manulally calculate for test mode)
      auto bp = initDR_mergeBoundaryPin(147, 273, size, routeBox);
      worker.setDRIter(0, bp);
    }
    worker.setEnableDRC(enableDRC);
    worker.setRipupMode(ripupMode);
    worker.setFollowGuide(followGuide);
    worker.setFixMode(fixMode);
    //worker.setNetOrderingMode(netOrderingMode);
    worker.setCost(workerDRCCost, workerMarkerCost, workerMarkerBloatWidth, workerMarkerBloatDepth);
    worker.main_mt();
    numQuickMarkers += worker.getNumQuickMarkers();
    cout <<"done"  <<endl <<flush;
  } else {

    vector<unique_ptr<FlexDRWorker> > uworkers;
    int batchStepX, batchStepY;

    getBatchInfo(batchStepX, batchStepY);

    vector<vector<vector<unique_ptr<FlexDRWorker> > > > workers(batchStepX * batchStepY);

    int xIdx = 0, yIdx = 0;
    for (int i = offset; i < (int)xgp.getCount(); i += clipSize) {
      for (int j = offset; j < (int)ygp.getCount(); j += clipSize) {
        auto worker = make_unique<FlexDRWorker>(this, logger_);
        frBox routeBox1;
        getDesign()->getTopBlock()->getGCellBox(frPoint(i, j), routeBox1);
        frBox routeBox2;
        const int max_i = min((int)xgp.getCount() - 1, i + clipSize-1);
        const int max_j = min((int)ygp.getCount(), j + clipSize-1);
        getDesign()->getTopBlock()->getGCellBox(frPoint(max_i, max_j),
                                                routeBox2);
        frBox routeBox(routeBox1.left(), routeBox1.bottom(), routeBox2.right(), routeBox2.top());
        frBox extBox;
        frBox drcBox;
        routeBox.bloat(MTSAFEDIST, extBox);
        routeBox.bloat(DRCSAFEDIST, drcBox);
        worker->setRouteBox(routeBox);
        worker->setExtBox(extBox);
        worker->setDrcBox(drcBox);
        worker->setGCellBox(frBox(i, j, max_i, max_j));
        worker->setMazeEndIter(mazeEndIter);
        worker->setDRIter(iter);
        if (!iter) {
          // if (routeBox.left() == 441000 && routeBox.bottom() == 816100) {
          //   cout << "@@@ debug: " << i << " " << j << endl;
          // }
          // set boundary pin
          auto bp = initDR_mergeBoundaryPin(i, j, size, routeBox);
          worker->setDRIter(0, bp);
        }
        worker->setEnableDRC(enableDRC);
        worker->setRipupMode(ripupMode);
        worker->setFollowGuide(followGuide);
        worker->setFixMode(fixMode);
        // TODO: only pass to relevant workers
        worker->setGraphics(graphics_.get());
        worker->setCost(workerDRCCost, workerMarkerCost, workerMarkerBloatWidth, workerMarkerBloatDepth);

        int batchIdx = (xIdx % batchStepX) * batchStepY + yIdx % batchStepY;
        if (workers[batchIdx].empty() || (int)workers[batchIdx].back().size() >= BATCHSIZE) {
          workers[batchIdx].push_back(vector<unique_ptr<FlexDRWorker>>());
        }
        workers[batchIdx].back().push_back(std::move(worker));

        yIdx++;
      }
      yIdx = 0;
      xIdx++;
    }


    omp_set_num_threads(MAX_THREADS);

    // parallel execution
    for (auto &workerBatch: workers) {
      ProfileTask profile("DR:checkerboard");
      for (auto &workersInBatch: workerBatch) {
        {
          ProfileTask profile("DR:batch");
          // multi thread
          #pragma omp parallel for schedule(dynamic)
          for (int i = 0; i < (int)workersInBatch.size(); i++) {
            workersInBatch[i]->main_mt();
            #pragma omp critical 
            {
              cnt++;
              if (VERBOSE > 0) {
                if (cnt * 1.0 / tot >= prev_perc / 100.0 + 0.1 && prev_perc < 90) {
                  if (prev_perc == 0 && t.isExceed(0)) {
                    isExceed = true;
                  }
                  prev_perc += 10;
                  //if (true) {
                  if (isExceed) {
                    if (enableDRC) {
                      cout <<"    completing " <<prev_perc <<"% with " <<getDesign()->getTopBlock()->getNumMarkers() <<" violations" <<endl;
                    } else {
                      cout <<"    completing " <<prev_perc <<"% with " <<numQuickMarkers <<" quick violations" <<endl;
                    }
                    cout <<"    " <<t <<endl <<flush;
                  }
                }
              }
            }
          }
        }
        {
          ProfileTask profile("DR:end_batch");
          // single thread
          for (int i = 0; i < (int)workersInBatch.size(); i++) {
            workersInBatch[i]->end();
          }
          workersInBatch.clear();
        }
      }
    }
  }
  if (!iter) {
    removeGCell2BoundaryPin();
  }
  if (VERBOSE > 0) {
    if (cnt * 1.0 / tot >= prev_perc / 100.0 + 0.1 && prev_perc >= 90) {
      if (prev_perc == 0 && t.isExceed(0)) {
        isExceed = true;
      }
      prev_perc += 10;
      //if (true) {
      if (isExceed) {
        if (enableDRC) {
          cout <<"    completing " <<prev_perc <<"% with " <<getDesign()->getTopBlock()->getNumMarkers() <<" violations" <<endl;
        } else {
          cout <<"    completing " <<prev_perc <<"% with " <<numQuickMarkers <<" quick violations" <<endl;
        }
        cout <<"    " <<t <<endl <<flush;
      }
    }
  }
  checkConnectivity(iter);
  numViols_.push_back(getDesign()->getTopBlock()->getNumMarkers());
  if (VERBOSE > 0) {
    if (enableDRC) {
      cout <<"  number of violations = " <<getDesign()->getTopBlock()->getNumMarkers() <<endl;
    } else {
      cout <<"  number of quick violations = " <<numQuickMarkers <<endl;
    }
    t.print();
    cout <<flush;
  }
  end();
}

void FlexDR::end(bool writeMetrics) {
  vector<unsigned long long> wlen(getTech()->getLayers().size(), 0);
  vector<unsigned long long> sCut(getTech()->getLayers().size(), 0);
  vector<unsigned long long> mCut(getTech()->getLayers().size(), 0);
  unsigned long long totWlen = 0;
  unsigned long long totSCut = 0;
  unsigned long long totMCut = 0;
  frPoint bp, ep;
  for (auto &net: getDesign()->getTopBlock()->getNets()) {
    for (auto &shape: net->getShapes()) {
      if (shape->typeId() == frcPathSeg) {
        auto obj = static_cast<frPathSeg*>(shape.get());
        obj->getPoints(bp, ep);
        auto lNum = obj->getLayerNum();
        frCoord psLen = ep.x() - bp.x() + ep.y() - bp.y();
        wlen[lNum] += psLen;
        totWlen += psLen;
      }
    }
    for (auto &via: net->getVias()) {
      auto lNum = via->getViaDef()->getCutLayerNum();
      if (via->getViaDef()->isMultiCut()) {
        ++mCut[lNum];
        ++totMCut;
      } else {
        ++sCut[lNum];
        ++totSCut;
      }
    }
  }

  if (writeMetrics) {
    logger_->metric(DRT, "wire length::total",
                    totWlen / getDesign()->getTopBlock()->getDBUPerUU());
    logger_->metric(DRT, "vias::total", totSCut + totMCut);
  }

  if (VERBOSE > 0) {
    boost::io::ios_all_saver guard(std::cout);
    cout <<endl <<"total wire length = " <<totWlen / getDesign()->getTopBlock()->getDBUPerUU() <<" um" <<endl;
    for (int i = getTech()->getBottomLayerNum(); i <= getTech()->getTopLayerNum(); i++) {
      if (getTech()->getLayer(i)->getType() == frLayerTypeEnum::ROUTING) {
        cout <<"total wire length on LAYER " <<getTech()->getLayer(i)->getName() <<" = " 
             <<wlen[i] / getDesign()->getTopBlock()->getDBUPerUU() <<" um" <<endl;
      }
    }
    cout <<"total number of vias = " <<totSCut + totMCut <<endl;
    if (totMCut > 0) {
      cout <<"total number of multi-cut vias = " <<totMCut 
           << " (" <<setw(5) <<fixed <<setprecision(1) <<totMCut * 100.0 / (totSCut + totMCut) <<"%)" <<endl;
      cout <<"total number of single-cut vias = " <<totSCut 
           << " (" <<setw(5) <<fixed <<setprecision(1) <<totSCut * 100.0 / (totSCut + totMCut) <<"%)" <<endl;
    }
    cout <<"up-via summary (total " <<totSCut + totMCut <<"):" <<endl;
    int nameLen = 0;
    for (int i = getTech()->getBottomLayerNum(); i <= getTech()->getTopLayerNum(); i++) {
      if (getTech()->getLayer(i)->getType() == frLayerTypeEnum::CUT) {
        nameLen = max(nameLen, (int)getTech()->getLayer(i-1)->getName().size());
      }
    }
    int maxL = 1 + nameLen + 4 + (int)to_string(totSCut).length();
    if (totMCut) {
      maxL += 9 + 4 + (int)to_string(totMCut).length() + 9 + 4 + (int)to_string(totSCut + totMCut).length();
    }
    if (totMCut) {
      cout <<" " <<setw(nameLen + 4 + (int)to_string(totSCut).length() + 9) <<"single-cut";
      cout <<setw(4 + (int)to_string(totMCut).length() + 9) <<"multi-cut" 
           <<setw(4 + (int)to_string(totSCut + totMCut).length()) <<"total";
    }
    cout <<endl;
    for (int i = 0; i < maxL; i++) {
      cout <<"-";
    }
    cout <<endl;
    for (int i = getTech()->getBottomLayerNum(); i <= getTech()->getTopLayerNum(); i++) {
      if (getTech()->getLayer(i)->getType() == frLayerTypeEnum::CUT) {
        cout <<" "    <<setw(nameLen) <<getTech()->getLayer(i-1)->getName() 
             <<"    " <<setw((int)to_string(totSCut).length()) <<sCut[i];
        if (totMCut) {
          cout <<" ("   <<setw(5) <<(double)((sCut[i] + mCut[i]) ? sCut[i] * 100.0 / (sCut[i] + mCut[i]) : 0.0) <<"%)";
          cout <<"    " <<setw((int)to_string(totMCut).length()) <<mCut[i] 
               <<" ("   <<setw(5) <<(double)((sCut[i] + mCut[i]) ? mCut[i] * 100.0 / (sCut[i] + mCut[i]) : 0.0) <<"%)"
               <<"    " <<setw((int)to_string(totSCut + totMCut).length()) <<sCut[i] + mCut[i];
        }
        cout <<endl;
      }
    }
    for (int i = 0; i < maxL; i++) {
      cout <<"-";
    }
    cout <<endl;
    cout <<" "    <<setw(nameLen) <<""
         <<"    " <<setw((int)to_string(totSCut).length()) <<totSCut;
    if (totMCut) {
      cout <<" ("   <<setw(5) <<(double)((totSCut + totMCut) ? totSCut * 100.0 / (totSCut + totMCut) : 0.0) <<"%)";
      cout <<"    " <<setw((int)to_string(totMCut).length()) <<totMCut 
           <<" ("   <<setw(5) <<(double)((totSCut + totMCut) ? totMCut * 100.0 / (totSCut + totMCut) : 0.0) <<"%)"
           <<"    " <<setw((int)to_string(totSCut + totMCut).length()) <<totSCut + totMCut;
    }
    cout <<endl <<endl <<flush;
    guard.restore();
  }
}

void FlexDR::reportDRC() {
  double dbu = design_->getTech()->getDBUPerUU();

  if (DRC_RPT_FILE == string("")) {
    if (VERBOSE > 0) {
      cout <<"Waring: no DRC report specified, skipped writing DRC report" <<endl;
    }
    return;
  }
  //cout << DRC_RPT_FILE << "\n";
  ofstream drcRpt(DRC_RPT_FILE.c_str());
  if (drcRpt.is_open()) {
    for (auto &marker: getDesign()->getTopBlock()->getMarkers()) {
      auto con = marker->getConstraint();
      drcRpt << "  violation type: ";
      if (con) {
        if (con->typeId() == frConstraintTypeEnum::frcShortConstraint) {
          if (getTech()->getLayer(marker->getLayerNum())->getType() == frLayerTypeEnum::ROUTING) {
            drcRpt <<"Short";
          } else if (getTech()->getLayer(marker->getLayerNum())->getType() == frLayerTypeEnum::CUT) {
            drcRpt <<"CShort";
          }
        } else if (con->typeId() == frConstraintTypeEnum::frcMinWidthConstraint) {
          drcRpt <<"MinWid";
        } else if (con->typeId() == frConstraintTypeEnum::frcSpacingConstraint) {
          drcRpt <<"MetSpc";
        } else if (con->typeId() == frConstraintTypeEnum::frcSpacingEndOfLineConstraint) {
          drcRpt <<"EOLSpc";
        } else if (con->typeId() == frConstraintTypeEnum::frcSpacingTablePrlConstraint) {
          drcRpt <<"MetSpc";
        } else if (con->typeId() == frConstraintTypeEnum::frcCutSpacingConstraint) {
          drcRpt <<"CutSpc";
        } else if (con->typeId() == frConstraintTypeEnum::frcMinStepConstraint) {
          drcRpt <<"MinStp";
        } else if (con->typeId() == frConstraintTypeEnum::frcNonSufficientMetalConstraint) {
          drcRpt <<"NSMet";
        } else if (con->typeId() == frConstraintTypeEnum::frcSpacingSamenetConstraint) {
          drcRpt <<"MetSpc";
        } else if (con->typeId() == frConstraintTypeEnum::frcOffGridConstraint) {
          drcRpt <<"OffGrid";
        } else if (con->typeId() == frConstraintTypeEnum::frcMinEnclosedAreaConstraint) {
          drcRpt <<"MinHole";
        } else if (con->typeId() == frConstraintTypeEnum::frcAreaConstraint) {
          drcRpt <<"MinArea";
        } else if (con->typeId() == frConstraintTypeEnum::frcLef58CornerSpacingConstraint) {
          drcRpt <<"CornerSpc";
        } else if (con->typeId() == frConstraintTypeEnum::frcLef58CutSpacingConstraint) {
          drcRpt <<"CutSpc";
        } else if (con->typeId() == frConstraintTypeEnum::frcLef58RectOnlyConstraint) {
          drcRpt <<"RectOnly";
        } else if (con->typeId() == frConstraintTypeEnum::frcLef58RightWayOnGridOnlyConstraint) {
          drcRpt <<"RightWayOnGridOnly";
        } else if (con->typeId() == frConstraintTypeEnum::frcLef58MinStepConstraint) {
          drcRpt <<"MinStp";
        } else {
          drcRpt << "unknown";
        }
      } else {
        drcRpt << "nullptr";
      }
      drcRpt <<endl;
      // get source(s) of violation
      drcRpt << "    srcs: ";
      for (auto src: marker->getSrcs()) {
        if (src) {
          switch (src->typeId()) {
            case frcNet:
              drcRpt << (static_cast<frNet*>(src))->getName() << " ";
              break;
            case frcInstTerm: {
              frInstTerm* instTerm = (static_cast<frInstTerm*>(src));
              drcRpt <<instTerm->getInst()->getName() <<"/" <<instTerm->getTerm()->getName() << " ";
              break;
            }
            case frcTerm: {
              frTerm* term = (static_cast<frTerm*>(src));
              drcRpt <<"PIN/" << term->getName() << " ";
              break;
            }
            case frcInstBlockage: {
              frInstBlockage* instBlockage = (static_cast<frInstBlockage*>(src));
              drcRpt <<instBlockage->getInst()->getName() <<"/OBS" << " ";
              break;
            }
            case frcBlockage: {
              drcRpt << "PIN/OBS" << " ";
              break;
            }
            default:
              std::cout << "Error: unexpected src type in marker\n";
          }
        }
      }
      drcRpt << "\n";
      // get violation bbox
      frBox bbox;
      marker->getBBox(bbox);
      drcRpt << "    bbox = ( " << bbox.left() / dbu << ", " << bbox.bottom() / dbu << " ) - ( "
             << bbox.right() / dbu << ", " << bbox.top() / dbu << " ) on Layer ";
      if (getTech()->getLayer(marker->getLayerNum())->getType() == frLayerTypeEnum::CUT && 
          marker->getLayerNum() - 1 >= getTech()->getBottomLayerNum()) {
        drcRpt << getTech()->getLayer(marker->getLayerNum() - 1)->getName() << "\n";
      } else {
        drcRpt << getTech()->getLayer(marker->getLayerNum())->getName() << "\n";
      }
    }
  } else {
    cout << "Error: Fail to open DRC report file\n";
  }
  
}


int FlexDR::main() {
  ProfileTask profile("DR:main");
  init();
  frTime t;
  if (VERBOSE > 0) {
    cout <<endl <<endl <<"start detail routing ...";
  }
  // search and repair: iter, size, offset, mazeEndIter, workerDRCCost, workerMarkerCost, 
  //                    markerBloatWidth, markerBloatDepth, enableDRC, ripupMode, followGuide, fixMode, TEST
  // fixMode:
  //   0 - general fix
  //   1 - fat via short, spc to wire fix (keep via net), no permutation, increasing DRCCOST
  //   2 - fat via short, spc to wire fix (ripup via net), no permutation, increasing DRCCOST
  //   3 - general fix, ripup everything (bloat)
  //   4 - general fix, ripup left/bottom net (touching), currently DISABLED
  //   5 - general fix, ripup right/top net (touching), currently DISABLED
  //   6 - two-net viol
  //   9 - search-and-repair queue
  // assume only mazeEndIter > 1 if enableDRC and ripupMode == 0 (partial ripup)
  //end();
  //searchRepair(1,  7, -4,  1, DRCCOST, 0,          0, 0, true, 1, false, 0, true); // test mode

  // need three different offsets to resolve boundary corner issues

  int iterNum = 0;
  searchRepair(iterNum++/*  0 */,  7,  0, 3, DRCCOST, 0/*MAARKERCOST*/,  0, 0, true, 1, true, 9); // true search and repair
  searchRepair(iterNum++/*  1 */,  7, -2, 3, DRCCOST, DRCCOST/*MAARKERCOST*/,  0, 0, true, 1, true, 9); // true search and repair
  searchRepair(iterNum++/*  1 */,  7, -5, 3, DRCCOST, DRCCOST/*MAARKERCOST*/,  0, 0, true, 1, true, 9); // true search and repair
  searchRepair(iterNum++/*  3 */,  7,  0, 8, DRCCOST, MARKERCOST,  0, 0, true, 0, false, 9); // true search and repair
  searchRepair(iterNum++/*  4 */,  7, -1, 8, DRCCOST, MARKERCOST,  0, 0, true, 0, false, 9); // true search and repair
  searchRepair(iterNum++/*  5 */,  7, -2, 8, DRCCOST, MARKERCOST,  0, 0, true, 0, false, 9); // true search and repair
  searchRepair(iterNum++/*  6 */,  7, -3, 8, DRCCOST, MARKERCOST,  0, 0, true, 0, false, 9); // true search and repair
  searchRepair(iterNum++/*  7 */,  7, -4, 8, DRCCOST, MARKERCOST,  0, 0, true, 0, false, 9); // true search and repair
  searchRepair(iterNum++/*  8 */,  7, -5, 8, DRCCOST, MARKERCOST,  0, 0, true, 0, false, 9); // true search and repair
  searchRepair(iterNum++/*  9 */,  7, -6, 8, DRCCOST, MARKERCOST,  0, 0, true, 0, false, 9); // true search and repair
  searchRepair(iterNum++/* 10 */,  7,  0, 8, DRCCOST*2, MARKERCOST,  0, 0, true, 0, false, 9); // true search and repair
  searchRepair(iterNum++/* 11 */,  7, -1, 8, DRCCOST*2, MARKERCOST,  0, 0, true, 0, false, 9); // true search and repair
  searchRepair(iterNum++/* 12 */,  7, -2, 8, DRCCOST*2, MARKERCOST,  0, 0, true, 0, false, 9); // true search and repair
  searchRepair(iterNum++/* 13 */,  7, -3, 8, DRCCOST*2, MARKERCOST,  0, 0, true, 0, false, 9); // true search and repair
  searchRepair(iterNum++/* 14 */,  7, -4, 8, DRCCOST*2, MARKERCOST,  0, 0, true, 0, false, 9); // true search and repair
  searchRepair(iterNum++/* 15 */,  7, -5, 8, DRCCOST*2, MARKERCOST,  0, 0, true, 0, false, 9); // true search and repair
  searchRepair(iterNum++/* 16 */,  7, -6, 8, DRCCOST*2, MARKERCOST,  0, 0, true, 0, false, 9); // true search and repair
  searchRepair(iterNum++/* ra'*/,  7, -3, 8, DRCCOST, MARKERCOST,  0, 0, true, 1, false, 9); // true search and repair
  searchRepair(iterNum++/* 17 */,  7,  0, 8, DRCCOST*4, MARKERCOST,  0, 0, true, 0, false, 9); // true search and repair
  searchRepair(iterNum++/* 18 */,  7, -1, 8, DRCCOST*4, MARKERCOST,  0, 0, true, 0, false, 9); // true search and repair
  searchRepair(iterNum++/* 19 */,  7, -2, 8, DRCCOST*4, MARKERCOST,  0, 0, true, 0, false, 9); // true search and repair
  searchRepair(iterNum++/* 20 */,  7, -3, 8, DRCCOST*4, MARKERCOST,  0, 0, true, 0, false, 9); // true search and repair
  searchRepair(iterNum++/* 21 */,  7, -4, 8, DRCCOST*4, MARKERCOST,  0, 0, true, 0, false, 9); // true search and repair
  searchRepair(iterNum++/* 22 */,  7, -5, 8, DRCCOST*4, MARKERCOST,  0, 0, true, 0, false, 9); // true search and repair
  searchRepair(iterNum++/* 23 */,  7, -6, 8, DRCCOST*4, MARKERCOST,  0, 0, true, 0, false, 9); // true search and repair
  searchRepair(iterNum++/* ra'*/,  5, -2, 8, DRCCOST, MARKERCOST,  0, 0, true, 1, false, 9); // true search and repair
  searchRepair(iterNum++/* 24 */,  7,  0, 8, DRCCOST*8, MARKERCOST*2,  0, 0, true, 0, false, 9); // true search and repair
  searchRepair(iterNum++/* 25 */,  7, -1, 8, DRCCOST*8, MARKERCOST*2,  0, 0, true, 0, false, 9); // true search and repair
  searchRepair(iterNum++/* 26 */,  7, -2, 8, DRCCOST*8, MARKERCOST*2,  0, 0, true, 0, false, 9); // true search and repair
  searchRepair(iterNum++/* 27 */,  7, -3, 8, DRCCOST*8, MARKERCOST*2,  0, 0, true, 0, false, 9); // true search and repair
  searchRepair(iterNum++/* 28 */,  7, -4, 8, DRCCOST*8, MARKERCOST*2,  0, 0, true, 0, false, 9); // true search and repair
  searchRepair(iterNum++/* 29 */,  7, -5, 8, DRCCOST*8, MARKERCOST*2,  0, 0, true, 0, false, 9); // true search and repair
  searchRepair(iterNum++/* 30 */,  7, -6, 8, DRCCOST*8, MARKERCOST*2,  0, 0, true, 0, false, 9); // true search and repair
  searchRepair(iterNum++/* ra'*/,  3, -1, 8, DRCCOST, MARKERCOST,  0, 0, true, 1, false, 9); // true search and repair
  searchRepair(iterNum++/* 31 */,  7,  0, 8, DRCCOST*16, MARKERCOST*4,  0, 0, true, 0, false, 9); // true search and repair
  searchRepair(iterNum++/* 32 */,  7, -1, 8, DRCCOST*16, MARKERCOST*4,  0, 0, true, 0, false, 9); // true search and repair
  searchRepair(iterNum++/* 33 */,  7, -2, 8, DRCCOST*16, MARKERCOST*4,  0, 0, true, 0, false, 9); // true search and repair
  searchRepair(iterNum++/* 34 */,  7, -3, 8, DRCCOST*16, MARKERCOST*4,  0, 0, true, 0, false, 9); // true search and repair
  searchRepair(iterNum++/* 35 */,  7, -4, 8, DRCCOST*16, MARKERCOST*4,  0, 0, true, 0, false, 9); // true search and repair
  searchRepair(iterNum++/* 36 */,  7, -5, 8, DRCCOST*16, MARKERCOST*4,  0, 0, true, 0, false, 9); // true search and repair
  searchRepair(iterNum++/* 37 */,  7, -6, 8, DRCCOST*16, MARKERCOST*4,  0, 0, true, 0, false, 9); // true search and repair
  searchRepair(iterNum++/* ra'*/,  3, -2, 8, DRCCOST, MARKERCOST,  0, 0, true, 1, false, 9); // true search and repair
  searchRepair(iterNum++/* 38 */,  7,  0, 16, DRCCOST*16, MARKERCOST*4,  0, 0, true, 0, false, 9); // true search and repair
  searchRepair(iterNum++/* 39 */,  7, -1, 16, DRCCOST*16, MARKERCOST*4,  0, 0, true, 0, false, 9); // true search and repair
  searchRepair(iterNum++/* 40 */,  7, -2, 16, DRCCOST*16, MARKERCOST*4,  0, 0, true, 0, false, 9); // true search and repair
  searchRepair(iterNum++/* 41 */,  7, -3, 16, DRCCOST*16, MARKERCOST*4,  0, 0, true, 0, false, 9); // true search and repair
  searchRepair(iterNum++/* 42 */,  7, -4, 16, DRCCOST*16, MARKERCOST*4,  0, 0, true, 0, false, 9); // true search and repair
  searchRepair(iterNum++/* 43 */,  7, -5, 16, DRCCOST*16, MARKERCOST*4,  0, 0, true, 0, false, 9); // true search and repair
  searchRepair(iterNum++/* 44 */,  7, -6, 16, DRCCOST*16, MARKERCOST*4,  0, 0, true, 0, false, 9); // true search and repair
  searchRepair(iterNum++/* ra'*/,  3, -0, 8, DRCCOST, MARKERCOST,  0, 0, true, 1, false, 9); // true search and repair
  searchRepair(iterNum++/* 45 */,  7,  0, 32, DRCCOST*32, MARKERCOST*8,  0, 0, true, 0, false, 9); // true search and repair
  searchRepair(iterNum++/* 46 */,  7, -1, 32, DRCCOST*32, MARKERCOST*8,  0, 0, true, 0, false, 9); // true search and repair
  searchRepair(iterNum++/* 47 */,  7, -2, 32, DRCCOST*32, MARKERCOST*8,  0, 0, true, 0, false, 9); // true search and repair
  searchRepair(iterNum++/* 48 */,  7, -3, 32, DRCCOST*32, MARKERCOST*8,  0, 0, true, 0, false, 9); // true search and repair
  searchRepair(iterNum++/* 49 */,  7, -4, 32, DRCCOST*32, MARKERCOST*8,  0, 0, true, 0, false, 9); // true search and repair
  searchRepair(iterNum++/* 50 */,  7, -5, 32, DRCCOST*32, MARKERCOST*8,  0, 0, true, 0, false, 9); // true search and repair
  searchRepair(iterNum++/* 51 */,  7, -6, 32, DRCCOST*32, MARKERCOST*8,  0, 0, true, 0, false, 9); // true search and repair
  searchRepair(iterNum++/* ra'*/,  3, -1, 8, DRCCOST, MARKERCOST,  0, 0, true, 1, false, 9); // true search and repair
  searchRepair(iterNum++/* 52 */,  7,  0, 64, DRCCOST*64, MARKERCOST*16,  0, 0, true, 0, false, 9); // true search and repair
  searchRepair(iterNum++/* 53 */,  7, -1, 64, DRCCOST*64, MARKERCOST*16,  0, 0, true, 0, false, 9); // true search and repair
  searchRepair(iterNum++/* 54 */,  7, -2, 64, DRCCOST*64, MARKERCOST*16,  0, 0, true, 0, false, 9); // true search and repair
  searchRepair(iterNum++/* 55 */,  7, -3, 64, DRCCOST*64, MARKERCOST*16,  0, 0, true, 0, false, 9); // true search and repair
  searchRepair(iterNum++/* 56 */,  7, -4, 64, DRCCOST*64, MARKERCOST*16,  0, 0, true, 0, false, 9); // true search and repair
  searchRepair(iterNum++/* 57 */,  7, -5, 64, DRCCOST*64, MARKERCOST*16,  0, 0, true, 0, false, 9); // true search and repair
  searchRepair(iterNum++/* 58 */,  7, -6, 64, DRCCOST*64, MARKERCOST*16,  0, 0, true, 0, false, 9); // true search and repair

  if (DRC_RPT_FILE != string("")) {
    reportDRC();
  }
  if (VERBOSE > 0) {
    cout <<endl <<"complete detail routing";
    end(/* writeMetrics */ true);
  }
  if (VERBOSE > 0) {
    t.print();
    cout <<endl;
  }
  return 0;
}

