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

#include "io.h"

using namespace std;
using namespace fr;

void io::Parser::instAnalysis() {
  bool enableOutput = false;
  //bool enableOutput = true;
  if (VERBOSE > 0) {
    logger->info(DRT, 162, "libcell analysis ...");
  }
  trackOffsetMap.clear();
  prefTrackPatterns.clear();
  for (auto &trackPattern: design->getTopBlock()->getTrackPatterns()) {
    auto isVerticalTrack = trackPattern->isHorizontal(); // yes = vertical track
    if (design->getTech()->getLayer(trackPattern->getLayerNum())->getDir() == frcHorzPrefRoutingDir) {
      if (!isVerticalTrack) {
        prefTrackPatterns.push_back(trackPattern);
      }
    } else {
      if (isVerticalTrack) {
        prefTrackPatterns.push_back(trackPattern);
      }
    }
  }

  int numLayers = design->getTech()->getLayers().size();
  map<frBlock*, tuple<frLayerNum, frLayerNum>, frBlockObjectComp> refBlockPinLayerRange;
  for (auto &uRefBlock: design->getRefBlocks()) {
    auto refBlock = uRefBlock.get();
    frLayerNum minLayerNum = numLayers;
    frLayerNum maxLayerNum = 0;
    for (auto &uTerm: refBlock->getTerms()) {
      for (auto &uPin: uTerm->getPins()) {
        for (auto &uPinFig: uPin->getFigs()) {
          auto pinFig = uPinFig.get();
          if (pinFig->typeId() == frcRect) {
            auto lNum = static_cast<frRect*>(pinFig)->getLayerNum();
            minLayerNum = min(minLayerNum, lNum);
            maxLayerNum = max(maxLayerNum, lNum);
          } else {
            cout <<"Error: instAnalysis unsupported pinFig" <<endl;
          }
        }
      }
    }
    maxLayerNum = min(maxLayerNum + 2, numLayers);
    refBlockPinLayerRange[refBlock] = make_tuple(minLayerNum, maxLayerNum);
    if (enableOutput) {
      cout <<"  " <<refBlock->getName() <<" PIN layer ("
           <<design->getTech()->getLayer(minLayerNum)->getName() <<", "
           <<design->getTech()->getLayer(maxLayerNum)->getName() <<")" <<endl;
    }
  }
  //cout <<"  refBlock pin layer range done" <<endl;

  if (VERBOSE > 0) {
    logger->info(DRT, 163, "instance analysis ...");
  }

  vector<frCoord> offset;
  int cnt = 0;
  for (auto &inst: design->getTopBlock()->getInsts()) {
    frPoint origin;
    inst->getOrigin(origin);
    auto orient = inst->getOrient();
    auto [minLayerNum, maxLayerNum] = refBlockPinLayerRange[inst->getRefBlock()];
    offset.clear();
    for (auto &tp: prefTrackPatterns) {
      if (tp->getLayerNum() >= minLayerNum && tp->getLayerNum() <= maxLayerNum) {
        // vertical track
        if (tp->isHorizontal()) {
          offset.push_back(origin.x() % tp->getTrackSpacing());
          //cout <<"inst/offset/layer " <<inst->getName() <<" " <<origin.y() % tp->getTrackSpacing() 
          //     <<" " <<design->getTech()->getLayer(tp->getLayerNum())->getName() <<endl;
        } else {
          offset.push_back(origin.y() % tp->getTrackSpacing());
          //cout <<"inst/offset/layer " <<inst->getName() <<" " <<origin.x() % tp->getTrackSpacing()
          //     <<" " <<design->getTech()->getLayer(tp->getLayerNum())->getName() <<endl;
        }
      }
    }
    trackOffsetMap[inst->getRefBlock()][orient][offset].insert(inst.get());
    cnt++;
    if (VERBOSE > 0) {
      if (cnt < 100000) {
        if (cnt % 10000 == 0) {
          cout <<"  complete " <<cnt <<" instances" <<endl;
        }
      } else {
        if (cnt % 100000 == 0) {
          cout <<"  complete " <<cnt <<" instances" <<endl;
        }
      }
    }
  }

  if (enableOutput) {
    cout <<endl <<"summary: " <<endl;
  }
  cnt = 0;
  frString orientName;
  for (auto &[refBlock, orientMap]: trackOffsetMap) {
    if (enableOutput) {
      cout <<"  " <<refBlock->getName() <<" (ORIENT/#diff patterns)";
    }
    for (auto &[orient, offsetMap]: orientMap) {
      cnt += offsetMap.size();
      if (enableOutput) {
        orient.getName(orientName);
        cout <<" (" <<orientName <<", " <<offsetMap.size() <<")";
        for (auto &[vec, inst]: offsetMap) {
          cout <<" " <<(*inst.begin())->getName();
        }
      }
    }
    if (enableOutput) {
      cout <<endl;
    }
  }
  if (VERBOSE > 0) {
    logger->info(DRT, 164, "# unique instances = {}", cnt);
  }
}
