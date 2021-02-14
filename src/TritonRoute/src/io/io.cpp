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
#include <fstream>
#include <sstream>
#include <exception>

#include "frProfileTask.h"
#include "global.h"
#include "io/io.h"
#include "db/tech/frConstraint.h"

#include "opendb/db.h"
#include "opendb/dbWireCodec.h"
#include "utility/Logger.h"

using namespace std;
using namespace fr;

int defdist(odb::dbBlock* block, int x)
{
  return x * (double) block->getDefUnits()
         / (double) block->getDbUnitsPerMicron();
}

void io::Parser::setDieArea(odb::dbBlock* block)
{
  vector<frBoundary> bounds;
  frBoundary bound;
  vector<frPoint> points;
  odb::Rect box;
  block->getDieArea(box);
  points.push_back(frPoint(defdist(block,box.xMin()), defdist(block,box.yMin())));
  points.push_back(frPoint(defdist(block,box.xMax()), defdist(block,box.yMax())));
  points.push_back(frPoint(defdist(block,box.xMax()), defdist(block,box.yMin())));
  points.push_back(frPoint(defdist(block,box.xMin()), defdist(block,box.yMax())));
  bound.setPoints(points);
  bounds.push_back(bound);
  tmpBlock->setDBUPerUU(block->getDbUnitsPerMicron());
  tmpBlock->setBoundaries(bounds);
}

void io::Parser::setTracks(odb::dbBlock* block)
{
  auto tracks = block->getTrackGrids();
  for (auto track : tracks) {
    if (tech->name2layer.find(track->getTechLayer()->getName())
        == tech->name2layer.end())
      logger->error(utl::DRT,
                    94,
                    "cannot find layer: {}",
                    track->getTechLayer()->getName());
    int xPatternSize = track->getNumGridPatternsX();
    int yPatternSize = track->getNumGridPatternsY();
    for (int i = 0; i < xPatternSize; i++) {
      unique_ptr<frTrackPattern> tmpTrackPattern
          = make_unique<frTrackPattern>();
      tmpTrackPattern->setLayerNum(
          tech->name2layer.at(track->getTechLayer()->getName())->getLayerNum());
      tmpTrackPattern->setHorizontal(true);
      int startCoord, numTracks, step;
      track->getGridPatternX(i, startCoord, numTracks, step);
      tmpTrackPattern->setStartCoord(startCoord);
      tmpTrackPattern->setNumTracks(numTracks);
      tmpTrackPattern->setTrackSpacing(step);
      tmpBlock->trackPatterns_.at(tmpTrackPattern->getLayerNum())
          .push_back(std::move(tmpTrackPattern));
    }
    for (int i = 0; i < yPatternSize; i++) {
      unique_ptr<frTrackPattern> tmpTrackPattern
          = make_unique<frTrackPattern>();
      tmpTrackPattern->setLayerNum(
          tech->name2layer.at(track->getTechLayer()->getName())->getLayerNum());
      tmpTrackPattern->setHorizontal(false);
      int startCoord, numTracks, step;
      track->getGridPatternY(i, startCoord, numTracks, step);
      tmpTrackPattern->setStartCoord(startCoord);
      tmpTrackPattern->setNumTracks(numTracks);
      tmpTrackPattern->setTrackSpacing(step);
      tmpBlock->trackPatterns_.at(tmpTrackPattern->getLayerNum())
          .push_back(std::move(tmpTrackPattern));
    }
  }
}

frOrientEnum getFrOrient(odb::dbOrientType orient)
{
  switch (orient)
  {
  case odb::dbOrientType::R0:
    return frOrientEnum::frcR0;  
  case odb::dbOrientType::R90:
    return frOrientEnum::frcR90;  
  case odb::dbOrientType::R180:
    return frOrientEnum::frcR180;  
  case odb::dbOrientType::R270:
    return frOrientEnum::frcR270;  
  case odb::dbOrientType::MY:
    return frOrientEnum::frcMY;  
  case odb::dbOrientType::MYR90:
    return frOrientEnum::frcMXR90;  
  case odb::dbOrientType::MX:
    return frOrientEnum::frcMX;  
  case odb::dbOrientType::MXR90:
    return frOrientEnum::frcMXR90;  
  }
  return frOrientEnum::frcR0;
}

void io::Parser::setInsts(odb::dbBlock* block)
{
  for (auto inst : block->getInsts()) {
    if (design->name2refBlock_.find(inst->getMaster()->getName())
        == design->name2refBlock_.end())
      logger->error(utl::DRT,
                    95,
                    "library cell {} not found",
                    inst->getMaster()->getName());
    if (tmpBlock->name2inst_.find(inst->getName())
        != tmpBlock->name2inst_.end())
      logger->error(utl::DRT, 96, "same cell name: {}", inst->getName());
    frBlock* refBlock = design->name2refBlock_.at(inst->getMaster()->getName());
    auto uInst = make_unique<frInst>(inst->getName(), refBlock);
    auto tmpInst = uInst.get();
    tmpInst->setId(numInsts);
    numInsts++;

    int x, y;
    inst->getLocation(x, y);
    x = defdist(block, x);
    y = defdist(block, y);
    tmpInst->setOrigin(frPoint(x, y));
    tmpInst->setOrient(getFrOrient(inst->getOrient().getValue()));
    for (auto& uTerm : tmpInst->getRefBlock()->getTerms()) {
      auto term = uTerm.get();
      unique_ptr<frInstTerm> instTerm = make_unique<frInstTerm>(tmpInst, term);
      instTerm->setId(numTerms);
      numTerms++;
      int pinCnt = term->getPins().size();
      instTerm->setAPSize(pinCnt);
      tmpInst->addInstTerm(std::move(instTerm));
    }
    for (auto& uBlk : tmpInst->getRefBlock()->getBlockages()) {
      auto blk = uBlk.get();
      unique_ptr<frInstBlockage> instBlk
          = make_unique<frInstBlockage>(tmpInst, blk);
      instBlk->setId(numBlockages);
      numBlockages++;
      tmpInst->addInstBlockage(std::move(instBlk));
    }
    tmpBlock->addInst(std::move(uInst));
  }
}

void io::Parser::setObstructions(odb::dbBlock* block)
{
  for (auto blockage : block->getObstructions()) {
    string layerName = blockage->getBBox()->getTechLayer()->getName();
    if (tech->name2layer.find(layerName) != tech->name2layer.end()) {
      continue;
    }
    frLayerNum layerNum = tech->name2layer[layerName]->getLayerNum();
    auto blkIn = make_unique<frBlockage>();
    blkIn->setId(numBlockages);
    numBlockages++;
    auto pinIn = make_unique<frPin>();
    pinIn->setId(0);
    frCoord xl = blockage->getBBox()->xMin();
    frCoord yl = blockage->getBBox()->yMin();
    frCoord xh = blockage->getBBox()->xMax();
    frCoord yh = blockage->getBBox()->yMax();
    // pinFig
    unique_ptr<frRect> pinFig = make_unique<frRect>();
    pinFig->setBBox(frBox(xl, yl, xh, yh));
    pinFig->addToPin(pinIn.get());
    pinFig->setLayerNum(layerNum);
    // pinFig completed
    unique_ptr<frPinFig> uptr(std::move(pinFig));
    pinIn->addPinFig(std::move(uptr));

    blkIn->setPin(std::move(pinIn));
    tmpBlock->addBlockage(std::move(blkIn));
  }
}

void io::Parser::setVias(odb::dbBlock* block)
{
  for (auto via : block->getVias()) {
    if (via->getViaGenerateRule() != nullptr && via->hasParams()) {
      odb::dbViaParams params;
      via->getViaParams(params);
      frLayerNum cutLayerNum = 0;
      frLayerNum botLayerNum = 0;
      frLayerNum topLayerNum = 0;

      if (tech->name2layer.find(params.getCutLayer()->getName())
          == tech->name2layer.end())
        logger->error(utl::DRT,
                      97,
                      "cannot find cut layer {}",
                      params.getCutLayer()->getName());
      else
        cutLayerNum = tech->name2layer.find(params.getCutLayer()->getName())
                          ->second->getLayerNum();

      if (tech->name2layer.find(params.getBottomLayer()->getName())
          == tech->name2layer.end())
        logger->error(utl::DRT,
                      98,
                      "cannot find bottom layer {}",
                      params.getBottomLayer()->getName());
      else
        botLayerNum = tech->name2layer.find(params.getBottomLayer()->getName())
                          ->second->getLayerNum();

      if (tech->name2layer.find(params.getTopLayer()->getName())
          == tech->name2layer.end())
        logger->error(utl::DRT,
                      99,
                      "cannot find top layer {}",
                      params.getTopLayer()->getName());
      else
        topLayerNum = tech->name2layer.find(params.getTopLayer()->getName())
                          ->second->getLayerNum();

      int xSize = defdist(block, params.getXCutSize());
      int ySize = defdist(block, params.getYCutSize());
      int xCutSpacing = defdist(block, params.getXCutSpacing());
      int yCutSpacing = defdist(block, params.getYCutSpacing());
      int xOffset = defdist(block, params.getXOrigin());
      int yOffset = defdist(block, params.getYOrigin());
      int xTopEnc = defdist(block, params.getXTopEnclosure());
      int yTopEnc = defdist(block, params.getYTopEnclosure());
      int xBotEnc = defdist(block, params.getXBottomEnclosure());
      int yBotEnc = defdist(block, params.getYBottomEnclosure());
      int xTopOffset = defdist(block, params.getXTopOffset());
      int yTopOffset = defdist(block, params.getYTopOffset());
      int xBotOffset = defdist(block, params.getXBottomOffset());
      int yBotOffset = defdist(block, params.getYBottomOffset());

      frCoord currX = 0;
      frCoord currY = 0;
      vector<unique_ptr<frShape> > cutFigs;
      for (int i = 0; i < params.getNumCutRows(); i++) {
        currX = 0;
        for (int j = 0; j < params.getNumCutCols(); j++) {
          auto rect = make_unique<frRect>();
          frBox tmpBox(currX, currY, currX + xSize, currY + ySize);
          rect->setBBox(tmpBox);
          rect->setLayerNum(cutLayerNum);
          cutFigs.push_back(std::move(rect));
          currX += xSize + xCutSpacing;
        }
        currY += ySize + yCutSpacing;
      }
      currX -= xCutSpacing;  // max cut X
      currY -= yCutSpacing;  // max cut Y
      frTransform cutXform(-currX / 2 + xOffset, -currY / 2 + yOffset);
      for (auto& uShape : cutFigs) {
        auto rect = static_cast<frRect*>(uShape.get());
        rect->move(cutXform);
      }
      unique_ptr<frShape> uBotFig = make_unique<frRect>();
      auto botFig = static_cast<frRect*>(uBotFig.get());
      unique_ptr<frShape> uTopFig = make_unique<frRect>();
      auto topFig = static_cast<frRect*>(uTopFig.get());

      frBox botBox(0 - xBotEnc, 0 - yBotEnc, currX + xBotEnc, currY + yBotEnc);
      frBox topBox(0 - xTopEnc, 0 - yTopEnc, currX + xTopEnc, currY + yTopEnc);

      frTransform botXform(-currX / 2 + xOffset + xBotOffset,
                           -currY / 2 + yOffset + yBotOffset);
      frTransform topXform(-currX / 2 + xOffset + xTopOffset,
                           -currY / 2 + yOffset + yTopOffset);
      botBox.transform(botXform);
      topBox.transform(topXform);

      botFig->setBBox(botBox);
      topFig->setBBox(topBox);
      botFig->setLayerNum(botLayerNum);
      topFig->setLayerNum(topLayerNum);

      auto viaDef = make_unique<frViaDef>(via->getName());
      viaDef->addLayer1Fig(std::move(uBotFig));
      viaDef->addLayer2Fig(std::move(uTopFig));
      for (auto& uShape : cutFigs) {
        viaDef->addCutFig(std::move(uShape));
      }
      tech->addVia(std::move(viaDef));
    } else {
      map<frLayerNum, set<odb::dbBox*> > lNum2Int;
      for (auto box : via->getBoxes()) {
        if (tech->name2layer.find(box->getTechLayer()->getName())
            == tech->name2layer.end()) {
          return;
        }
        auto layerNum = tech->name2layer.at(box->getTechLayer()->getName())
                            ->getLayerNum();
        lNum2Int[layerNum].insert(box);
      }
      if ((int) lNum2Int.size() != 3)
        logger->error(utl::DRT, 100, "unsupported via: {}", via->getName());
      if (lNum2Int.begin()->first + 2 != (--lNum2Int.end())->first)
        logger->error(
            utl::DRT, 101, "non-consecutive layers for via: {}", via->getName());
      auto viaDef = make_unique<frViaDef>(via->getName());
      int cnt = 0;
      for (auto& [layerNum, boxes] : lNum2Int) {
        for (auto box : boxes) {
          unique_ptr<frRect> pinFig = make_unique<frRect>();
          pinFig->setBBox(frBox(defdist(block, box->xMin()),
                                defdist(block, box->yMin()),
                                defdist(block, box->xMax()),
                                defdist(block, box->yMax())));
          pinFig->setLayerNum(layerNum);
          switch (cnt) {
            case 0:
              viaDef->addLayer1Fig(std::move(pinFig));
              break;
            case 1:
              viaDef->addCutFig(std::move(pinFig));
              break;
            default:
              viaDef->addLayer2Fig(std::move(pinFig));
              break;
          }
        }
        cnt++;
      }
      tech->addVia(std::move(viaDef));
    }
  }
}

void io::Parser::getSBoxCoords(odb::dbSBox* box,
                               frCoord& beginX,
                               frCoord& beginY,
                               frCoord& endX,
                               frCoord& endY,
                               frCoord& width)
{
  auto block = box->getDb()->getChip()->getBlock();
  int x1 = box->xMin();
  int y1 = box->yMin();
  int x2 = box->xMax();
  int y2 = box->yMax();
  uint dx = box->getDX();
  uint dy = box->getDY();
  uint w;
  switch (box->getDirection()) {
    case odb::dbSBox::UNDEFINED: {
      bool dx_even = ((dx & 1) == 0);
      bool dy_even = ((dy & 1) == 0);
      if (dx_even && dy_even) {
        if (dy < dx) {
          w = dy;
          uint dw = dy >> 1;
          y1 += dw;
          y2 -= dw;
          assert(y1 == y2);
        } else {
          w = dx;
          uint dw = dx >> 1;
          x1 += dw;
          x2 -= dw;
          assert(x1 == x2);
        }
      } else if (dx_even) {
        w = dx;
        uint dw = dx >> 1;
        x1 += dw;
        x2 -= dw;
        assert(x1 == x2);
      } else if (dy_even) {
        w = dy;
        uint dw = dy >> 1;
        y1 += dw;
        y2 -= dw;
        assert(y1 == y2);
      } else
        logger->error(utl::DRT, 102, "odd dimension in both directions");
      break;
    }
    case odb::dbSBox::HORIZONTAL: {
      w = dy;
      uint dw = dy >> 1;
      y1 += dw;
      y2 -= dw;
      assert(y1 == y2);
      break;
    }
    case odb::dbSBox::VERTICAL: {
      w = dx;
      uint dw = dx >> 1;
      x1 += dw;
      x2 -= dw;
      assert(x1 == x2);
      break;
    }
    case odb::dbSBox::OCTILINEAR: {
      odb::Oct oct = box->getOct();
      x1 = oct.getCenterLow().getX();
      y1 = oct.getCenterLow().getY();
      x2 = oct.getCenterHigh().getX();
      y2 = oct.getCenterHigh().getY();
      w = oct.getWidth();
      break;
    }
    default:
      logger->error(utl::DRT, 103, "unknown direction");
      break;
  }
  beginX = defdist(block, x1);
  endX = defdist(block, x2);
  beginY = defdist(block, y1);
  endY = defdist(block, y2);
  width = defdist(block, w);
}

void io::Parser::setNets(odb::dbBlock* block)
{
  for (auto net : block->getNets()) {
    unique_ptr<frNet> uNetIn = make_unique<frNet>(net->getName());
    auto netIn = uNetIn.get();
    netIn->setId(numNets);
    numNets++;
    for (auto term : net->getBTerms()) {
      if (tmpBlock->name2term_.find(term->getName())
          == tmpBlock->name2term_.end())
        logger->error(utl::DRT, 104, "term {} not found", term->getName());
      auto frterm = tmpBlock->name2term_[term->getName()];  // frTerm*
      frterm->addToNet(netIn);
      netIn->addTerm(frterm);
      // graph enablement
      auto termNode = make_unique<frNode>();
      termNode->setPin(frterm);
      termNode->setType(frNodeTypeEnum::frcPin);
      netIn->addNode(termNode);
    }
    for (auto term : net->getITerms()) {
      if (tmpBlock->name2inst_.find(term->getInst()->getName())
          == tmpBlock->name2inst_.end())
        logger->error(
            utl::DRT, 105, "component {} not found", term->getInst()->getName());
      auto inst = tmpBlock->name2inst_[term->getInst()->getName()];
      // gettin inst term
      auto frterm = inst->getRefBlock()->getTerm(term->getMTerm()->getName());
      if(frterm == nullptr)
        logger->error(utl::DRT, 106, "component pin {}/{} not found",term->getInst()->getName(),term->getMTerm()->getName());
      int idx = frterm->getOrderId();
      auto &instTerms = inst->getInstTerms();
      auto instTerm = instTerms[idx].get();
      assert(instTerm->getTerm()->getName() == term->getMTerm()->getName());
      
      instTerm->addToNet(netIn);
      netIn->addInstTerm(instTerm);
      // graph enablement
      auto instTermNode = make_unique<frNode>();
      instTermNode->setPin(instTerm);
      instTermNode->setType(frNodeTypeEnum::frcPin);
      netIn->addNode(instTermNode);
    }
    // initialize
    string layerName = "";
    string viaName = "";
    string shape = "";
    bool hasBeginPoint = false;
    bool hasEndPoint = false;
    frCoord beginX = -1;
    frCoord beginY = -1;
    frCoord beginExt = -1;
    frCoord endX = -1;
    frCoord endY = -1;
    frCoord endExt = -1;
    bool hasRect = false;
    frCoord left = -1;
    frCoord bottom = -1;
    frCoord right = -1;
    frCoord top = -1;
    frCoord width = 0;
    odb::dbWireDecoder decoder;

    if (!net->isSpecial() && net->getWire() != nullptr) {
      decoder.begin(net->getWire());
      odb::dbWireDecoder::OpCode pathId = decoder.next();
      while (pathId != odb::dbWireDecoder::END_DECODE) {
        // for each path start
        layerName = "";
        viaName = "";
        shape = "";
        hasBeginPoint = false;
        hasEndPoint = false;
        beginX = -1;
        beginY = -1;
        beginExt = -1;
        endX = -1;
        endY = -1;
        endExt = -1;
        hasRect = false;
        left = -1;
        bottom = -1;
        right = -1;
        top = -1;
        width = 0;
        bool endpath = false;
        do {
          switch (pathId) {
            case odb::dbWireDecoder::PATH:
            case odb::dbWireDecoder::JUNCTION:
            case odb::dbWireDecoder::SHORT:
            case odb::dbWireDecoder::VWIRE:
              layerName = decoder.getLayer()->getName();
              if (tech->name2layer.find(layerName) == tech->name2layer.end())
                logger->error(utl::DRT, 107, "unsupported layer {}", layerName);
              break;
            case odb::dbWireDecoder::POINT:

              if (!hasBeginPoint) {
                decoder.getPoint(beginX, beginY);
                hasBeginPoint = true;
              } else {
                decoder.getPoint(endX, endY);
                hasEndPoint = true;
              }
              beginX = defdist(block, beginX);
              beginY = defdist(block, beginY);
              endX = defdist(block, endX);
              endY = defdist(block, endY);
              break;
            case odb::dbWireDecoder::POINT_EXT:
              if (!hasBeginPoint) {
                decoder.getPoint(beginX, beginY, beginExt);
                hasBeginPoint = true;
              } else {
                decoder.getPoint(endX, endY, endExt);
                hasEndPoint = true;
              }
              beginX = defdist(block, beginX);
              beginY = defdist(block, beginY);
              beginExt = defdist(block, beginExt);
              endX = defdist(block, endX);
              endY = defdist(block, endY);
              endExt = defdist(block, endExt);

              break;
            case odb::dbWireDecoder::VIA:
              viaName = string(decoder.getVia()->getName());
              break;
            case odb::dbWireDecoder::TECH_VIA:
              viaName = string(decoder.getTechVia()->getName());
              break;
            case odb::dbWireDecoder::RECT:
              decoder.getRect(left, bottom, right, top);
              left = defdist(block, left);
              bottom = defdist(block, bottom);
              right = defdist(block, right);
              top = defdist(block, top);
              hasRect = true;
              break;
            case odb::dbWireDecoder::ITERM:
            case odb::dbWireDecoder::BTERM:
            case odb::dbWireDecoder::RULE:
            case odb::dbWireDecoder::END_DECODE:
              break;
            default:
              break;
          }
          pathId = decoder.next();
          if ((int) pathId <= 3 || pathId == odb::dbWireDecoder::END_DECODE)
            endpath = true;
        } while (!endpath);
        auto layerNum = tech->name2layer[layerName]->getLayerNum();
        if (hasRect) {
          continue;
        }
        if (hasEndPoint) {
          auto tmpP = make_unique<frPathSeg>();
          if (beginX > endX || beginY > endY) {
            tmpP->setPoints(frPoint(endX, endY), frPoint(beginX, beginY));
            swap(beginExt, endExt);
          } else {
            tmpP->setPoints(frPoint(beginX, beginY), frPoint(endX, endY));
          }
          tmpP->addToNet(netIn);
          tmpP->setLayerNum(layerNum);

          width = (width) ? width : tech->name2layer[layerName]->getWidth();
          auto defaultBeginExt = width / 2;
          auto defaultEndExt = width / 2;

          frEndStyleEnum tmpBeginEnum;
          if (beginExt == -1) {
            tmpBeginEnum = frcExtendEndStyle;
          } else if (beginExt == 0) {
            tmpBeginEnum = frcTruncateEndStyle;
          } else {
            tmpBeginEnum = frcVariableEndStyle;
          }
          frEndStyle tmpBeginStyle(tmpBeginEnum);

          frEndStyleEnum tmpEndEnum;
          if (endExt == -1) {
            tmpEndEnum = frcExtendEndStyle;
          } else if (endExt == 0) {
            tmpEndEnum = frcTruncateEndStyle;
          } else {
            tmpEndEnum = frcVariableEndStyle;
          }
          frEndStyle tmpEndStyle(tmpEndEnum);

          frSegStyle tmpSegStyle;
          tmpSegStyle.setWidth(width);
          tmpSegStyle.setBeginStyle(
              tmpBeginStyle,
              tmpBeginEnum == frcExtendEndStyle ? defaultBeginExt : beginExt);
          tmpSegStyle.setEndStyle(
              tmpEndStyle,
              tmpEndEnum == frcExtendEndStyle ? defaultEndExt : endExt);
          tmpP->setStyle(tmpSegStyle);
          netIn->addShape(std::move(tmpP));
        }
        if (viaName != "") {
          if (tech->name2via.find(viaName) == tech->name2via.end()) {
            logger->error(utl::DRT, 108, "unsupported via in db");
          } else {
            frPoint p;
            if (hasEndPoint) {
              p.set(endX, endY);
            } else {
              p.set(beginX, beginY);
            }
            auto viaDef = tech->name2via[viaName];
            auto tmpP = make_unique<frVia>(viaDef);
            tmpP->setOrigin(p);
            tmpP->addToNet(netIn);
            netIn->addVia(std::move(tmpP));
          }
        }
        // for each path end
      }
    }
    if (net->isSpecial()) {
      for (auto swire : net->getSWires()) {
        for (auto box : swire->getWires()) {
          if (!box->isVia()) {
            getSBoxCoords(box, beginX, beginY, endX, endY, width);
            auto layerNum = tech->name2layer[box->getTechLayer()->getName()]
                                ->getLayerNum();
            auto tmpP = make_unique<frPathSeg>();
            tmpP->setPoints(frPoint(beginX, beginY), frPoint(endX, endY));
            tmpP->addToNet(netIn);
            tmpP->setLayerNum(layerNum);
            width = (width) ? width : tech->name2layer[layerName]->getWidth();
            auto defaultExt = width / 2;

            frEndStyleEnum tmpBeginEnum;
            if (box->getWireShapeType()==odb::dbWireShapeType::NONE) {
              tmpBeginEnum = frcExtendEndStyle;
            } else {
              tmpBeginEnum = frcTruncateEndStyle;
            }
            frEndStyle tmpBeginStyle(tmpBeginEnum);
            frEndStyleEnum tmpEndEnum;
            if (box->getWireShapeType()==odb::dbWireShapeType::NONE) {
              tmpEndEnum = frcExtendEndStyle;
            } else {
              tmpEndEnum = frcTruncateEndStyle;
            }
            frEndStyle tmpEndStyle(tmpEndEnum);

            frSegStyle tmpSegStyle;
            tmpSegStyle.setWidth(width);
            tmpSegStyle.setBeginStyle(tmpBeginStyle, tmpBeginEnum == frcExtendEndStyle ? defaultExt : 0);
            tmpSegStyle.setEndStyle(tmpEndStyle, tmpEndEnum == frcExtendEndStyle ? defaultExt : 0);
            tmpP->setStyle(tmpSegStyle);
            netIn->addShape(std::move(tmpP));
          } else {
            if (box->getTechVia())
              viaName = box->getTechVia()->getName();
            else if (box->getBlockVia())
              viaName = box->getBlockVia()->getName();

            if (tech->name2via.find(viaName) == tech->name2via.end())
              logger->error(utl::DRT, 109, "unsupported via in db");
            else {
              int x, y;
              box->getViaXY(x, y);
              frPoint p(defdist(block, x), defdist(block, y));
              auto viaDef = tech->name2via[viaName];
              auto tmpP = make_unique<frVia>(viaDef);
              tmpP->setOrigin(p);
              tmpP->addToNet(netIn);
              netIn->addVia(std::move(tmpP));
            }
          }
        }
      }
    }
    frNetEnum netType;
    switch (net->getSigType()) {
      case odb::dbSigType::SIGNAL:
        netType = frNetEnum::frcNormalNet;
        break;
      case odb::dbSigType::CLOCK:
        netType = frNetEnum::frcClockNet;
        break;
      case odb::dbSigType::POWER:
        netType = frNetEnum::frcPowerNet;
        break;
      case odb::dbSigType::GROUND:
        netType = frNetEnum::frcGroundNet;
        break;
      default:
        logger->error(utl::DRT, 110, "unsupported NET USE in def");
        break;
    }
    netIn->setType(netType);
    if (net->isSpecial())
      tmpBlock->addSNet(std::move(uNetIn));
    else
      tmpBlock->addNet(std::move(uNetIn));
  }
}

void io::Parser::setBTerms(odb::dbBlock* block)
{
  for (auto term : block->getBTerms()) {
    frTermEnum termType;
    switch (term->getSigType().getValue()) {
      case odb::dbSigType::SIGNAL:
        termType = frTermEnum::frcNormalTerm;
        break;
      case odb::dbSigType::POWER:
        termType = frTermEnum::frcPowerTerm;
        break;
      case odb::dbSigType::GROUND:
        termType = frTermEnum::frcGroundTerm;
        break;
      case odb::dbSigType::CLOCK:
        termType = frTermEnum::frcClockTerm;
        break;
      default:
        logger->error(utl::DRT, 111, "unsupported PIN USE in db");
        break;
    }
    frTermDirectionEnum termDirection = frTermDirectionEnum::UNKNOWN;
    switch (term->getIoType().getValue()) {
      case odb::dbIoType::INPUT:
        termDirection = frTermDirectionEnum::INPUT;
        break;
      case odb::dbIoType::OUTPUT:
        termDirection = frTermDirectionEnum::OUTPUT;
        break;
      case odb::dbIoType::INOUT:
        termDirection = frTermDirectionEnum::INOUT;
        break;
      case odb::dbIoType::FEEDTHRU:
        termDirection = frTermDirectionEnum::FEEDTHRU;
        break;
    }
    auto uTermIn = make_unique<frTerm>(term->getName());
    auto termIn = uTermIn.get();
    termIn->setId(numTerms);
    numTerms++;
    termIn->setType(termType);
    termIn->setDirection(termDirection);
    auto pinIn = make_unique<frPin>();
    pinIn->setId(0);
    for (auto pin : term->getBPins()) {
      for (auto box : pin->getBoxes()) {
        if (tech->name2layer.find(box->getTechLayer()->getName())
            == tech->name2layer.end())
          logger->error(utl::DRT,
                        112,
                        "unsupported layer {}",
                        box->getTechLayer()->getName());
        frLayerNum layerNum
            = tech->name2layer[box->getTechLayer()->getName()]->getLayerNum();
        frCoord xl = defdist(block, box->xMin());
        frCoord yl = defdist(block, box->yMin());
        frCoord xh = defdist(block, box->xMax());
        frCoord yh = defdist(block, box->yMax());
        unique_ptr<frRect> pinFig = make_unique<frRect>();
        pinFig->setBBox(frBox(xl, yl, xh, yh));
        pinFig->addToPin(pinIn.get());
        pinFig->setLayerNum(layerNum);
        unique_ptr<frPinFig> uptr(std::move(pinFig));
        pinIn->addPinFig(std::move(uptr));
      }
    }
    termIn->addPin(std::move(pinIn));
    tmpBlock->addTerm(std::move(uTermIn));
  }
}

void io::Parser::readDesign(odb::dbDatabase* db)
{
  ProfileTask profile("IO:readDesign");
  if(db->getChip() == nullptr)
    logger->error(utl::DRT, 116, "load design first");
  odb::dbBlock* block = db->getChip()->getBlock();
  if(block == nullptr)
    logger->error(utl::DRT, 117, "load design first");
  tmpBlock = make_unique<frBlock>(string(block->getName()));
  tmpBlock->trackPatterns_.clear();
  tmpBlock->trackPatterns_.resize(tech->layers.size());
  setDieArea(block);
  setTracks(block);
  setInsts(block);
  setObstructions(block);
  setVias(block);
  setBTerms(block);
  setNets(block);
  tmpBlock->setId(0);
  design->setTopBlock(std::move(tmpBlock));
  addFakeNets();
}

void io::Parser::addFakeNets() {
  // add VSS fake net
  auto vssFakeNet = make_unique<frNet>(string("frFakeVSS"));
  vssFakeNet->setType(frNetEnum::frcGroundNet);
  vssFakeNet->setIsFake(true);
  design->getTopBlock()->addFakeSNet(std::move(vssFakeNet));
  // add VDD fake net
  auto vddFakeNet = make_unique<frNet>(string("frFakeVDD"));
  vddFakeNet->setType(frNetEnum::frcPowerNet);
  vddFakeNet->setIsFake(true);
  design->getTopBlock()->addFakeSNet(std::move(vddFakeNet));
}

// only support GF14 related content

int io::Parser::getLef58CutClass(void *data, frLayer* tmpLayer, const string &sIn) {
  //bool enableOutput = true;
  bool enableOutput = false;
  if (enableOutput) {
    cout <<endl <<"  PROPERTY LEF58_CUTCLASS \"";
  }
  istringstream istr(sIn);
  string word;

  string  name       = "";
  frCoord viaWidth   = 0;
  bool    hViaLength = false;
  frCoord viaLength  = 0;
  bool    hNumCut    = false;
  frUInt4 numCut     = 0;
  io::Parser* parser = (io::Parser*) data;

  while (istr >> word) {
    if (word == string("CUTCLASS")) {
      // initialization
      name       = "";
      viaWidth   = 0;
      hViaLength = false;
      viaLength  = 0;
      hNumCut    = false;
      numCut     = 0;
      if (istr >> name) {
        ;
      } else {
        cout <<"Error: getLef58CutClass" <<endl;
      }
      if (enableOutput) {
        cout <<endl <<"  CUTCLASS " <<name;
      }
    } else if (word == "WIDTH") {
      double tmp;
      if (istr >> tmp) {
        viaWidth = frCoord(round(tmp * parser->tech->getDBUPerUU()));
      } else {
        cout <<"Error: getLef58CutClass" <<endl;
      }
      if (enableOutput) {
        cout <<" WIDTH " <<tmp;
      }
    } else if (word == "LENGTH") {
      double tmp;
      if (istr >> tmp) {
        hViaLength = true;
        viaLength = frCoord(round(tmp * parser->tech->getDBUPerUU()));
      } else {
        cout <<"Error: getLef58CutClass" <<endl;
      }
      if (enableOutput) {
        cout <<" LENGTH " <<tmp;
      }
    } else if (word == "CUTS") {
      if (istr >> numCut) {
        hNumCut = true;
      } else {
        cout <<"Error: getLef58CutClass" <<endl;
      }
      if (enableOutput) {
        cout <<" CUTS " <<numCut;
      }
    } else if (word == ";") {
      if (enableOutput) {
        cout <<" ;";
      }
      // push rule here;
      auto cutClass = make_unique<frLef58CutClass>();
      cutClass->setName(name);
      cutClass->setViaWidth(viaWidth);
      if (hViaLength) {
        cutClass->setViaLength(viaLength);
      } else {
        cutClass->setViaLength(viaWidth);
      }
      if (hNumCut) {
        cutClass->setNumCut(numCut);
      } else {
        cutClass->setNumCut(1);
      }
      parser->tech->addCutClass(tmpLayer->getLayerNum(), std::move((cutClass)));
    }
  }
  if (enableOutput) {
    cout <<"\" ;" <<endl;
  }
  return 0;
}

int io::Parser::getLef58CutSpacing_helper(void *data, frLayer* tmpLayer, const string &sIn) {
  //bool enableOutput = true;
  //bool enableOutput = false;

  string keyword = "";

  istringstream istr(sIn);
  string word;
  stringstream ss;
  while (istr >> word) {
    if (word == string("SPACING")) {
      keyword = "";
      ss.str("");
      ss <<word;
    } else if (keyword == "" && word == string("SAMEMASK")) {
      keyword = "SAMEMASK";
      ss <<" " <<word;
    } else if (word == string("MAXXY")) {
      keyword = "MAXXY";
      ss <<" " <<word;
    } else if (word == string("LAYER")) {
      keyword = "LAYER";
      ss <<" " <<word;
    } else if (word == string("ADJACENTCUTS")) {
      keyword = "ADJACENTCUTS";
      ss <<" " <<word;
    } else if (word == string("PARALLELOVERLAP")) {
      keyword = "PARALLELOVERLAP";
      ss <<" " <<word;
    } else if (word == string("PARALLELWITHIN")) {
      keyword = "PARALLELWITHIN";
      ss <<" " <<word;
    } else if (word == string("SAMEMETALSHAREDEDGE")) {
      keyword = "SAMEMETALSHAREDEDGE";
      ss <<" " <<word;
    } else if (word == string("AREA")) {
      keyword = "AREA";
      ss <<" " <<word;
    } else {
      ss <<" " <<word;
    }
  }

  if (keyword == "LAYER") {
    getLef58CutSpacing_layer(data, tmpLayer, ss.str());
  } else if (keyword == "ADJACENTCUTS") {
    getLef58CutSpacing_adjacentCuts(data, tmpLayer, ss.str());
  } else {
    cout << "Warning: unsupported LEF58_SPACING branch" << keyword << ", skipped...\n"; //skip unsupported rules
    cout << sIn << endl;
  }
  
  return 0;

}

int io::Parser::getLef58CutSpacing(void *data, frLayer* tmpLayer, const string &sIn) {
  //bool enableOutput = true;
  bool enableOutput = false;
  if (enableOutput) {
    cout <<endl <<"  PROPERTY LEF58_SPACING \"";
  }
  istringstream istr(sIn);
  string word;
  stringstream ss;
  while (istr >> word) {
    if (word == string("SPACING")) {
      ss.str("");
      ss <<word;
    } else if (word == ";") {
      ss <<" " <<word;
      getLef58CutSpacing_helper(data, tmpLayer, ss.str());
    } else {
      ss <<" " <<word;
    }
  }
  if (enableOutput) {
    cout <<"\" ;" <<endl;
  }
  return 0;
}

int io::Parser::getLef58CutSpacing_layer(void *data, frLayer* tmpLayer, const string &sIn) {
  // bool enableOutput = true;
  bool enableOutput = false;
  bool isSkip = false;

  frCoord cutSpacing = -1;
  bool isCenterToCenter = false;
  bool isSameNet = false;
  bool isSameMetal = false;
  bool isSameVia = false;

  string secondLayerName;
  bool isStack = false;
  frCoord orthogonalSpacing = -1;
  string className;
  bool isShortEdgeOnly = false;
  frCoord prl = -1;
  bool isConcaveCorner = false;;
  frCoord width = -1;
  frCoord enclosure = -1;
  frCoord edgeLength = -1;
  frCoord parLength = -1;
  frCoord parWithin = -1;
  frCoord edgeEnclosure = -1;
  frCoord adjEnclosure = -1;
  frCoord extension = -1;
  frCoord eolWidth = -1;
  frCoord minLength = -1;
  bool isMaskOverlap = false;
  bool isWrongDirection = false;

  istringstream istr(sIn);
  string word;

  string keyword = "";
  io::Parser* parser = (io::Parser*) data;

  auto con = make_unique<frLef58CutSpacingConstraint>();

  while (istr >> word) {
    if (word == string("SPACING")) {
      double tmp;
      if (istr >> tmp) {
        cutSpacing = frCoord(round(tmp * parser->tech->getDBUPerUU()));
        con->setCutSpacing(cutSpacing);
      } else {
        isSkip = true;
      }
      if (enableOutput) {
        cout <<endl <<"  SPACING " <<tmp;
      }
    } else if (word == string("CENTERTOCENTER")) {
      isCenterToCenter = true;
      con->setCenterToCenter(isCenterToCenter);
    } else if (word == string("SAMENET")) {
      isSameNet = true;
      con->setSameNet(isSameNet);
    } else if (word == string("SAMEMETAL")) {
      isSameMetal = true;
      con->setSameMetal(isSameMetal);
    } else if (word == string("SAMEVIA")) {
      isSameVia = true;
      con->setSameVia(isSameVia);
    } else if (word == string("LAYER")) {
      if (istr >> secondLayerName) {
        con->setSecondLayerName(secondLayerName);
      } else {
        isSkip = true;
      }
      if (enableOutput) {
        cout <<" LAYER " <<secondLayerName;
      }
      if (istr >> word) {
        if (word == string("STACK")) {
          isStack = true;
          con->setStack(isStack);
        } else if (word == string("ORTHOGONALSPACING")) {
          double tmp;
          if (istr >> tmp) {
            orthogonalSpacing = frCoord(round(tmp * parser->tech->getDBUPerUU()));
            con->setOrthogonalSpacing(orthogonalSpacing);
          } else {
            isSkip = true;
          }
        } else if (word == string("CUTCLASS")) {
          if (istr >> className) {
            con->setCutClassName(className);
            auto cutClassIdx = tmpLayer->getCutClassIdx(className);
            if (cutClassIdx != -1) {
              // cout << "cutClassIdx = " << cutClassIdx << endl;
              con->setCutClassIdx(cutClassIdx);
            } else {
              isSkip = true;
            }
          } else {
            isSkip = true;
          }
          if (enableOutput) {
            cout <<" CUTCLASS " <<className;
          }
          // cut class branch
          if (istr >> word) {
            if (word == string("SHORTEDGEONLY")) {
              isShortEdgeOnly = true;
              con->setShortEdgeOnly(isShortEdgeOnly);
              if (istr >> word) {
                if (word == string("PRL")) {
                  double tmp;
                  if (istr >> tmp) {
                    prl = frCoord(round(tmp * parser->tech->getDBUPerUU()));
                    con->setPrl(prl);
                  } else {
                    isSkip = true;
                  }
                }
              }
            } else if (word == string("CONCAVECORNER")) {
              isConcaveCorner = true;
              con->setConcaveCorner(isConcaveCorner);
              if (enableOutput) {
                cout << " CONCAVECORNER";
              }
              if (istr >> word) {
                if (word == string("WIDTH")) {
                  double tmp;
                  if (istr >> tmp) {
                    width = frCoord(round(tmp * parser->tech->getDBUPerUU()));
                    con->setWidth(width);
                    if (istr >> word) {
                      if (word == string("ENCLOSURE")) {
                        if (istr >> tmp) {
                          enclosure = frCoord(round(tmp * parser->tech->getDBUPerUU()));
                          con->setEnclosure(enclosure);
                          if (istr >> word) {
                            if (word == string("EDGELENGTH")) {
                              if (istr >> tmp) {
                                edgeLength = frCoord(round(tmp * parser->tech->getDBUPerUU()));
                                con->setEdgeLength(edgeLength);
                              } else {
                                isSkip = true;
                              }
                            } else {
                              isSkip = true;
                            }
                          } else {
                            isSkip = true;
                          }
                        } else {
                          isSkip = true;
                        }
                      } else {
                        isSkip = true;
                      }
                    } else {
                      isSkip = true;
                    }
                  } else {
                    isSkip = true;
                  }
                } else if (word == string("PARALLEL")) {
                  double tmp;
                  if (istr >> tmp) {
                    parLength = frCoord(round(tmp * parser->tech->getDBUPerUU()));
                    con->setParLength(parLength);
                    if (istr >> word) {
                      if (word == string("WITHIN")) {
                        if (istr >> tmp) {
                          parWithin = frCoord(round(tmp * parser->tech->getDBUPerUU()));
                          con->setParWithin(parWithin);
                          if (istr >> word) {
                            if (word == string("ENCLOSURE")) {
                              if (istr >> tmp) {
                                enclosure = frCoord(round(tmp * parser->tech->getDBUPerUU()));
                                con->setEnclosure(enclosure);
                              } else {
                                isSkip = true;
                              }
                            } else {
                              isSkip = true;
                            }
                          } else {
                            isSkip = true;
                          }
                        } else {
                          isSkip = true;
                        }
                      } else {
                        isSkip = true;
                      }
                    } else {
                      isSkip = true;
                    }
                  } else {
                    isSkip = true;
                  }
                } else if (word == string("EDGELENGTH")) {
                  double tmp;
                  if (istr >> tmp) {
                    edgeLength = frCoord(round(tmp * parser->tech->getDBUPerUU()));
                    con->setEdgeLength(edgeLength);
                    if (istr >> word) {
                      if (word == string("ENCLOSURE")) {
                        if (istr >> tmp) {
                          edgeEnclosure = frCoord(round(tmp * parser->tech->getDBUPerUU()));
                          con->setEdgeEnclosure(edgeEnclosure);
                          if (istr >> tmp) {
                            adjEnclosure = frCoord(round(tmp * parser->tech->getDBUPerUU()));
                            con->setAdjEnclosure(adjEnclosure);
                          } else {
                            isSkip = true;
                          }
                        } else {
                          isSkip = true;
                        }
                      } else {
                        isSkip = true;
                      }
                    } else {
                      isSkip = true;
                    }
                  } else {
                    isSkip = true;
                  }
                }
              } else {
                isSkip = true;
              }
            } else if (word == string("EXTENSION")) {
              double tmp;
              if (istr >> tmp) {
                extension = frCoord(round(tmp * parser->tech->getDBUPerUU()));
                con->setExtension(extension);
              } else {
                isSkip = true;
              }
            } else if (word == string("NONEOLCONVEXCORNER")) {
              double tmp;
              if (istr >> tmp) {
                if (enableOutput) {
                  cout << " NONEOLCONVEXCORNER " << tmp;
                }
                eolWidth = frCoord(round(tmp * parser->tech->getDBUPerUU()));
                con->setEolWidth(eolWidth);
                if (istr >> word) {
                  if (word == string("MINLENGTH")) {
                    if (istr >> tmp) {
                      if (enableOutput) {
                        cout << " MINLENGTH " << tmp;
                      }
                      minLength = frCoord(round(tmp * parser->tech->getDBUPerUU()));
                      con->setMinLength(minLength);
                    } else {
                      isSkip = true;
                    }
                  } else {
                    ;
                  }
                }
              }
            } else if (word == string("ABOVEWIDTH")) {
              double tmp;
              if (istr >> tmp) {
                width = frCoord(round(tmp * parser->tech->getDBUPerUU()));
                con->setWidth(width);
                if (istr >> word) {
                  if (word == string("ENCLOSURE")) {
                    if (istr >> tmp) {
                      enclosure = frCoord(round(tmp * parser->tech->getDBUPerUU()));
                      con->setEnclosure(enclosure);
                    } else {
                      isSkip = true;
                    }
                  } else {
                    isSkip = true;
                  }
                }
              } else {
                isSkip = true;
              }
            } else if (word == string("MASKOVERLAP")) {
              isMaskOverlap = true;
              con->setMaskOverlap(isMaskOverlap);
            } else if (word == string("WRONGDIRECTION")) {
              isWrongDirection = true;
              con->setWrongDirection(isWrongDirection);
            } else {
              isSkip = true;
            }
          }
        }
      }
    } 
  }

  if (isSkip) {
    cout << "Error: getLef58CutSpacing_layer" << endl;
    cout << sIn << endl;
  } else {
    // cout << "addLef58CutSpacingConstraint\n";
    tmpLayer->addLef58CutSpacingConstraint(con.get());
    parser->tech->addUConstraint(std::move(con));

  }
  return 0;
}

// lefdef ref spacing
int io::Parser::getLef58CutSpacing_adjacentCuts(void *data, frLayer* tmpLayer, const string &sIn) {
  bool enableOutput = false;
  // bool enableOutput = true;

  bool isSkip = false;

  frCoord  cutSpacing = -1;
  bool isCenterToCenter = false;
  bool isSameNet = false;
  bool isSameMetal = false;
  bool isSameVia = false;
  int numAdjCuts = -1;
  int exactAlignedCut = -1;
  // two cuts
  int twoCuts = -1;
  frCoord twoCutsSpacing = -1;
  bool isSameCut = false;
  // within
  frCoord cutWithin2 = -1;

  bool isExceptSamePGNet;
  frCoord exceptAllWithin;
  bool isAbove = false;
  bool isBelow = false;
  frCoord enclosure = -1;
  // cutclass
  string cutClassName = "";
  bool isToAll = false;
  bool isNoPrl = false;
  bool isSideParallelOverlap = false;
  bool isSameMask = false;

  istringstream istr(sIn);
  string word;

  string keyword = "";
  io::Parser* parser = (io::Parser*) data;

  auto con = make_unique<frLef58CutSpacingConstraint>();

  while (istr >> word) {
    if (word == string("SPACING")) {
      double tmp;
      if (istr >> tmp) {
        cutSpacing = frCoord(round(tmp * parser->tech->getDBUPerUU()));
        con->setCutSpacing(cutSpacing);
      } else {
        isSkip = true;
      }
      if (enableOutput) {
        cout <<endl <<"  SPACING " <<tmp;
      }
    } else if (word == string("CENTERTOCENTER")) {
      isCenterToCenter = true;
      con->setCenterToCenter(isCenterToCenter);
    } else if (word == string("SAMENET")) {
      isSameNet = true;
      con->setSameNet(isSameNet);
    } else if (word == string("SAMEMETAL")) {
      isSameMetal = true;
      con->setSameMetal(isSameMetal);
    } else if (word == string("SAMEVIA")) {
      isSameVia = true;
      con->setSameVia(isSameVia);
    } else if (word == string("ADJACENTCUTS")) {
      if (istr >> numAdjCuts) {
        con->setAdjacentCuts(numAdjCuts);
        if (enableOutput) {
          cout << " ADJACENTCUTS " << numAdjCuts;
        }
      } else {
        isSkip = true;
      }
      while (istr >> word) {
        if (word == string("EXACTALIGNED")) {
          if (istr >> exactAlignedCut) {
            con->setExactAlignedCut(exactAlignedCut);
          } else {
            isSkip = true;
          }
        } else if (word == string("TWOCUTS")) {
          if (istr >> twoCuts) {
            con->setTwoCuts(twoCuts);
            if (enableOutput) {
              cout << " TWOCUTS " << twoCuts;
            }
            streampos pos = istr.tellg();
            bool done = false;
            while (!done) {
              if (istr >> word) {
                if (word == string("TWOCUTSSPACING")) {
                  double tmp;
                  if (istr >> tmp) {
                    twoCutsSpacing = frCoord(round(tmp * parser->tech->getDBUPerUU()));
                    con->setTwoCutsSpacing(twoCutsSpacing);
                    pos = istr.tellg();
                  } else {
                    isSkip = true;
                  }
                } else if (word == string("SAMECUT")) {
                  isSameCut = true;
                  con->setSameCut(isSameCut);
                  pos = istr.tellg();
                } else {
                  done = true;
                }
              }
            }
            istr.seekg(pos);
          } else {
            isSkip = true;
          }
        } else if (word == string("WITHIN")) {
          streampos pos = istr.tellg();
          double tmp1;
          if (istr >> tmp1) {
            pos = istr.tellg();
          } else {
            isSkip = true;
          }
          // TODO: bring this back after figure out how to correctly use seekg / tellg
          // if (istr >> tmp2) {
          //   cutWithin1 = frCoord(round(tmp1 * parser->tech->getDBUPerUU()));
          //   cutWithin2 = frCoord(round(tmp2 * parser->tech->getDBUPerUU()));
          //   con->setCutWithin1(cutWithin1);
          //   con->setCutWithin2(cutWithin2);
          //   pos = istr.tellg();
          // } else {
            cutWithin2 = frCoord(round(tmp1 * parser->tech->getDBUPerUU()));
            con->setCutWithin(cutWithin2);
            // istr.seekg(pos);
            if (enableOutput) {
              cout << " WITHIN " << tmp1;
            }
          // }
        } else if (word == string("EXCEPTSAMEPGNET")) {
          isExceptSamePGNet = true;
          con->setExceptSamePGNet(isExceptSamePGNet);
        } else if (word == string("EXCEPTALLWITHIN")) {
          double tmp;
          if (istr >> tmp) {
            exceptAllWithin = frCoord(round(tmp * parser->tech->getDBUPerUU()));
            con->setExceptAllWithin(exceptAllWithin);
          } else {
            isSkip = true;
          }
        } else if (word == string("ENCLOSURE")) {
          streampos pos = istr.tellg();
          if (istr >> word) {
            if (word == string("ABOVE")) {
              isAbove = true;
              con->setAbove(isAbove);
            } else if (word == string("BELOW")) {
              isBelow = true;
              con->setBelow(isBelow);
            } else {
              istr.seekg(pos);
            }
          }
          double tmp;
          if (istr >> tmp) {
            enclosure = frCoord(round(tmp * parser->tech->getDBUPerUU()));
            con->setEnclosure(enclosure);
          } else {
            isSkip = true;
          }
        } else if (word == string("CUTCLASS")) {
          if (istr >> cutClassName) {
            con->setCutClassName(cutClassName);
            auto cutClassIdx = tmpLayer->getCutClassIdx(cutClassName);
            if (cutClassIdx != -1) {
              // cout << "cutClassIdx = " << cutClassIdx << endl;
              con->setCutClassIdx(cutClassIdx);
            } else {
              isSkip = true;
            }
            if (enableOutput) {
              cout << " CUTCLASS " << cutClassName;
            }
          } else {
            isSkip = true;
          }
          streampos pos = istr.tellg();
          string word1, word2;
          if (istr >> word1 && istr >> word2) {
            if (word1 == string("TO") && word2 == string("ALL")) {
              isToAll = true;
              con->setToAll(isToAll);
            } else {
              istr.seekg(pos);
            }
          }
        } else if (word == string("NOPRL")) {
          isNoPrl = true;
          con->setNoPrl(isNoPrl);
        } else if (word == string("SIDEPARALLELOVERLAP")) {
          isSideParallelOverlap = true;
          con->setSideParallelOverlap(isSideParallelOverlap);
        } else if (word == string("SAMEMASK")) {
          isSameMask = true;
          con->setSameMask(isSameMask);
        } else {
          isSkip = true;
        }
      }
    }
  }

  if (enableOutput) {
    cout << endl;
  }

  if (isSkip) {
    cout << "Error: getLef58CutSpacing_adjacentCuts" << endl;
  } else {
    // cout << "addLef58CutSpacingConstraint\n";
    tmpLayer->addLef58CutSpacingConstraint(con.get());
    parser->tech->addUConstraint(std::move(con));
  }
  return 0;
}

int io::Parser::getLef58CutSpacingTable(void *data, frLayer* tmpLayer, const string &sIn) {
  //bool enableOutput = true;
  bool enableOutput = false;
  if (enableOutput) {
    cout <<endl <<"  PROPERTY LEF58_SPACINGTABLE \"";
  }
  istringstream istr(sIn);
  string word;
  stringstream ss;
  while (istr >> word) {
    if (word == string("SPACINGTABLE")) {
      ss.str("");
      ss <<word;
    } else if (word == ";") {
      ss <<" " <<word;
      getLef58CutSpacingTable_helper(data, tmpLayer, ss.str());
    } else {
      ss <<" " <<word;
    }
  }
  if (enableOutput) {
    cout <<"\" ;" <<endl;
  }
  return 0;
}

int io::Parser::getLef58CutSpacingTable_helper(void *data, frLayer* tmpLayer, const string &sIn) {
  //bool enableOutput = true;
  //bool enableOutput = false;
  //if (enableOutput) {
  //  cout <<endl <<"  PROPERTY LEF58_SPACINGTABLE \"" <<endl;
  //}

  string keyword = "";

  istringstream istr(sIn);
  string word;
  stringstream ss;
  while (istr >> word) {
    if (word == string("SPACINGTABLE")) {
      keyword = "";
      ss.str("");
      ss <<word;
      //if (enableOutput) {
      //  cout <<"  SPACINGTABLE";
      //}
    } else if (word == string("CENTERSPACING")) {
      //if (enableOutput) {
      //  cout <<" CENTERSPACING";
      //}
      keyword = "CENTERSPACING";
      ss <<" " <<word;
    } else if (word == string("ORTHOGONAL")) {
      //if (enableOutput) {
      //  cout <<" CENTERSPACING";
      //}
      keyword = "ORTHOGONAL";
      ss <<" " <<word;
    } else {
      ss <<" " <<word;
    }
  }

  if (keyword == "CENTERSPACING") {
    ; //skip center spacing rules
  } else if (keyword == "ORTHOGONAL") {
    ; //skip orthogonal rules
  } else {
    getLef58CutSpacingTable_others(data, tmpLayer, ss.str());
  }
  
  //if (enableOutput) {
  //  cout <<"\" ;" <<endl;
  //}

  return 0;

}

int io::Parser::getLef58CutSpacingTable_default(void *data, frLayer* tmpLayer, const string &sIn, 
  const shared_ptr<frLef58CutSpacingTableConstraint> &con) {
  //cout <<sIn <<endl;
  //bool enableOutput = true;
  bool enableOutput = false;
  
  frCoord defaultCutSpacing = 0;

  istringstream istr(sIn);
  string word;
  io::Parser* parser = (io::Parser*) data;

  while (istr >> word) {
    if (word == string("DEFAULT")) {
      double tmp;
      if (istr >> tmp) {
        defaultCutSpacing = frCoord(round(tmp * parser->tech->getDBUPerUU()));
      } else {
        cout <<"Error: getLef58CutSpacingTable_default" <<endl;
      }
      if (enableOutput) {
        cout <<endl <<"  DEFAULT " <<tmp;
      }
    } else {
      ; // skip unknown rules
    }
  }

  con->setDefaultCutSpacing(defaultCutSpacing);
  return 0;
}

int io::Parser::getLef58CutSpacingTable_prl(void *data, frLayer* tmpLayer, const string &sIn, 
    const shared_ptr<frLef58CutSpacingTableConstraint> &con) {
  //cout <<sIn <<endl;
  //bool enableOutput = true;
  bool enableOutput = false;
  
  frCoord prl          = 0;
  bool    isHorizontal = false;
  bool    isVertical   = false;
  bool    isMaxXY      = false;

  istringstream istr(sIn);
  string word;
  io::Parser* parser = (io::Parser*) data;

  while (istr >> word) {
    if (word == string("PRL")) {
      double tmp;
      if (istr >> tmp) {
        prl = frCoord(round(tmp * parser->tech->getDBUPerUU()));
      } else {
        cout <<"Error: getLef58CutSpacingTable_prl" <<endl;
      }
      if (enableOutput) {
        cout <<" PRL " <<tmp;
      }
    } else if (word == string("HORIZONTAL")) {
      isHorizontal = true;
      if (enableOutput) {
        cout <<" HORIZONTAL";
      }
    } else if (word == string("VERTICAL")) {
      isVertical = true;
      if (enableOutput) {
        cout <<" VERTICAL";
      }
    } else if (word == string("MAXXY")) {
      isMaxXY = true;
      if (enableOutput) {
        cout <<" MAXXY";
      }
    } else {
      ; // skip unknown rules
    }
  }

  auto ptr = make_shared<frLef58CutSpacingTablePrlConstraint>();
  ptr->setPrl(prl);
  ptr->setHorizontal(isHorizontal);
  ptr->setVertical(isVertical);
  ptr->setMaxXY(isMaxXY);
  con->setPrlConstraint(ptr);

  return 0;
}

int io::Parser::getLef58CutSpacingTable_layer(void *data, frLayer* tmpLayer, const string &sIn, 
    const shared_ptr<frLef58CutSpacingTableConstraint> &con, frLayerNum &secondLayerNum) {

  //bool enableOutput = true;
  bool enableOutput = false;

  frString secondLayerName    = "";
  bool     isNonZeroEnclosure = false;

  istringstream istr(sIn);
  string word;

  while (istr >> word) {
    if (word == string("LAYER")) {
      if (istr >> secondLayerName) {
        ;
      } else {
        cout <<"Error: getLef58CutSpacingTable_layer" <<endl;
      }
      if (enableOutput) {
        cout <<" LAYER " <<secondLayerName;
      }
    } else if (word == string("NONZEROENCLOSURE")) {
      isNonZeroEnclosure = true;
      if (enableOutput) {
        cout <<endl <<"  NONZEROENCLOSURE";
      }
    } else {
      ; // skip unknown rules
    }
  }

  io::Parser* parser = (io::Parser*) data;
  auto ptr = make_shared<frLef58CutSpacingTableLayerConstraint>();
  //cout <<secondLayerName <<endl <<flush;
  secondLayerNum = parser->tech->name2layer.at(secondLayerName)->getLayerNum();
  ptr->setSecondLayerNum(secondLayerNum);
  ptr->setNonZeroEnc(isNonZeroEnclosure);
  con->setLayerConstraint(ptr);

  return 0;
}


int io::Parser::getLef58CutSpacingTable_cutClass(void *data, frLayer* tmpLayer, const string &sIn, 
    const shared_ptr<frLef58CutSpacingTableConstraint> &con, bool hasSecondLayer, frLayerNum secondLayerNum) {
  //bool enableOutput = true;
  bool enableOutput = false;

  auto defaultCutSpacing = con->getDefaultCutSpacing();
  // 2d spacing table
  frCollection<frCollection<pair<frCoord, frCoord> > > tblVals;

  //cout <<endl <<sIn <<endl;
  // check numRows and numCols
  istringstream istr1(sIn);
  string word;
  int numCols = 0;
  int numRows = 0;
  bool isPrevNum   = false;
  while (istr1 >> word) {
    // "-" is treated as number 0
    if (word == "-") {
      word = "0";
    }
    stringstream tmpss(word);
    double tmpd;
    // is a number
    if (tmpss >> tmpd) {
      isPrevNum = true;
      numCols++;
    // is a string
    } else {
      if (word == ";") {
        numRows++;
      } else if (isPrevNum) {
        numRows++;
        numCols = 0;
      }
      isPrevNum = false;
    }
  }

  numCols /= 2;
  //cout <<endl <<"#rows/cols = " <<numRows <<"/" <<numCols <<endl;
  
  vector<frString> colNames;
  vector<int>      dupColNames; //duplicate side and all
  
  vector<frString> rowNames;
  vector<int>      dupRowNames; //duplicate side and all
  
  vector<vector<pair<frCoord, frCoord> > > tmpTbl;
  vector<pair<frCoord, frCoord> > tmpTblRow;
  
  io::Parser* parser = (io::Parser*) data;
  istringstream istr2(sIn);
  word = "";
  int stage = 0; // 0 = read columns; 1 = read rows
  int readNum = 0; // numbers read in a row
  while (istr2 >> word) {
    // "-" is treated as number 0
    //if (word == ";") {
    //  cout <<"found ;" <<endl;
    //}
    if (word == "-") {
      word = to_string(defaultCutSpacing * 1.0 / parser->tech->getDBUPerUU());
    }
    if (word == "CUTCLASS") {
      if (enableOutput) {
        cout <<endl <<"  CUTCLASS";
      }
    // read numCols times
    } else if ((int)colNames.size() < numCols) {
      if (word == "SIDE" || word == "END") {
        *(--colNames.end()) = *(--colNames.end()) + word;
        *(--dupColNames.end()) = 1;
      } else {
        colNames.push_back(word);
        dupColNames.push_back(2);
      }
      if (enableOutput) {
        cout <<" " <<word;
      }
      //cout <<"testX" <<endl;
    } else if (stage == 0 && (int)colNames.size() == numCols) {
      // last word of column
      if (word == "SIDE" || word == "END") {
        *(--colNames.end()) = *(--colNames.end()) + word;
        *(--dupColNames.end()) = 1;
        if (enableOutput) {
          cout <<" " <<word;
        }
      // first word of row
      } else {
        rowNames.push_back(word);
        dupRowNames.push_back(2);
        if (enableOutput) {
          cout <<endl <<"  " <<word;
        }
      }
      stage = 1;
      //cout <<"testXX" <<endl;
    } else if (word == ";") {
      if (enableOutput) {
        cout <<" ;";
      }
      tmpTbl.push_back(tmpTblRow);
      //cout <<"testXXX" <<endl;
    } else if (stage == 1) {
      if (word == "SIDE" || word == "END") {
        *(--rowNames.end()) = *(--rowNames.end()) + word;
        *(--dupRowNames.end()) = 1;
        if (enableOutput) {
          cout <<" " <<word;
        }
      } else {
        stringstream ss(word);
        double firstNum;
        //cout <<"test: " <<word <<endl;
        // number
        if (ss >> firstNum) {
          frCoord val1 = frCoord(round(firstNum * parser->tech->getDBUPerUU()));
          string tmpS;
          if (istr2 >> tmpS) {
            ;
          } else {
            cout <<"Error: getLef58CutSpacingTable_cutClass" <<endl;
          }
          stringstream tmpSS(tmpS);
          double secondNum;
          if (tmpSS >> secondNum) {
            frCoord val2 = frCoord(round(secondNum * parser->tech->getDBUPerUU()));
            tmpTblRow.push_back(make_pair(val1, val2));
            if (enableOutput) {
              cout <<" " <<firstNum <<" " <<secondNum;
            }
          } else {
            // the number is "-", use default spacing
            frCoord val2 = defaultCutSpacing;
            tmpTblRow.push_back(make_pair(val1, val2));
            if (enableOutput) {
              cout <<" " <<firstNum <<" " <<defaultCutSpacing * 1.0 / parser->tech->getDBUPerUU();
            }
          }
          readNum += 1;
        // first word
        } else {
          rowNames.push_back(word);
          dupRowNames.push_back(2);
          if (enableOutput) {
            cout <<endl <<"  " <<word;
          }
          if (readNum) {
            tmpTbl.push_back(tmpTblRow);
            tmpTblRow.clear();
          }
          readNum = 0;
        }
      }
      //cout <<"testXXXX" <<endl;
    }
  }

  //cout <<endl <<"column:";
  //for (auto &str: colNames) {
  //  cout <<" " <<str;
  //}
  //
  //cout <<endl <<"row:";
  //for (auto &str: rowNames) {
  //  cout <<" " <<str;
  //}

  //cout <<endl <<"table: ";
  //for (auto &v1: tmpTbl) {
  //  cout <<"test here";
  //  cout <<endl <<"    ";
  //  for (auto &v2: v1) {
  //    cout <<"test here";
  //    cout <<" " <<v2.first <<" " <<v2.second;
  //  }
  //}
  //cout <<flush;

  vector<frString> expColNames;
  //cout <<endl <<"new expand column:";
  for (int i = 0; i < (int)colNames.size(); i++) {
    if (dupColNames.at(i) == 2) {
      string name1 = colNames.at(i) + "SIDE";
      string name2 = colNames.at(i) + "END";
      expColNames.push_back(name1);
      expColNames.push_back(name2);
      //cout <<" " <<name1 <<" " <<name2;
    } else {
      string name = colNames.at(i);
      expColNames.push_back(name);
      //cout <<" " <<name;
    };
  }
  
  vector<frString> expRowNames;
  //cout <<endl <<"new expand rows:";
  for (int i = 0; i < (int)rowNames.size(); i++) {
    if (dupRowNames.at(i) == 2) {
      string name1 = rowNames.at(i) + "SIDE";
      string name2 = rowNames.at(i) + "END";
      expRowNames.push_back(name1);
      expRowNames.push_back(name2);
      //cout <<" " <<name1 <<" " <<name2;
    } else {
      string name = rowNames.at(i);
      expRowNames.push_back(name);
      //cout <<" " <<name;
    };
  }

  vector<vector<pair<frCoord, frCoord> > > expTmpTbl;
  for (int i = 0; i < (int)rowNames.size(); i++) {
    vector<pair<frCoord, frCoord> > expTmpTblRow;
    for (int j = 0; j < (int)colNames.size(); j++) {
      expTmpTblRow.push_back(tmpTbl.at(i).at(j));
      if (dupColNames.at(j) == 2) {
        expTmpTblRow.push_back(tmpTbl.at(i).at(j));
      }
    }
    expTmpTbl.push_back(expTmpTblRow);
    if (dupRowNames.at(i) == 2) {
      expTmpTbl.push_back(expTmpTblRow);
    }
  }
  //cout <<"new expand table: ";
  //for (auto &v1: expTmpTbl) {
  //  cout <<endl <<"    ";
  //  for (auto &v2: v1) {
  //    cout <<" " <<v2.first <<" " <<v2.second;
  //  }
  //}

  vector<pair<frString, int> > expColNames_helper;
  for (int i = 0; i < (int)expColNames.size(); i++) {
    expColNames_helper.push_back(make_pair(expColNames.at(i), i));
  }
  sort(expColNames_helper.begin(), expColNames_helper.end(), 
       [](const pair<frString, int> &a, const pair<frString, int> &b) 
       {return a.first < b.first;});
  sort(expColNames.begin(), expColNames.end());

  //cout <<endl <<"sorted expand column:";
  //for (auto &it: expColNames_helper) {
  //  cout <<" " <<it.first;
  //}

  vector<pair<frString, int> > expRowNames_helper;
  for (int i = 0; i < (int)expRowNames.size(); i++) {
    expRowNames_helper.push_back(make_pair(expRowNames.at(i), i));
  }
  sort(expRowNames_helper.begin(), expRowNames_helper.end(), 
       [](const pair<frString, int> &a, const pair<frString, int> &b) 
       {return a.first < b.first;});
  sort(expRowNames.begin(), expRowNames.end());

  //cout <<endl <<"sorted expand row:";
  //for (auto &it: expRowNames_helper) {
  //  cout <<" " <<it.first;
  //}

  tblVals = expTmpTbl;
  //cout <<endl <<"sorted tbl:";
  for (int i = 0; i < (int)expRowNames_helper.size(); i++) {
    //cout <<endl;
    for (int j = 0; j < (int)expColNames_helper.size(); j++) {
      int orig_i = expRowNames_helper.at(i).second;
      int orig_j = expColNames_helper.at(j).second;
      tblVals.at(i).at(j) = expTmpTbl.at(orig_i).at(orig_j);
      //cout <<" " <<tblVals.at(i).at(j).first <<" " <<tblVals.at(i).at(j).second;
    }
  }

  string rowName("CUTCLASS");
  string colName("CUTCLASS");
  auto ptr = make_shared<fr2DLookupTbl<frString, frString, pair<frCoord, frCoord> > >(rowName, 
      expRowNames, colName, expColNames, tblVals);
  con->setCutClassTbl(ptr);

  return 0;
}

// lefdef ref 2nd spacing table, no orthogonal case
int io::Parser::getLef58CutSpacingTable_others(void *data, frLayer* tmpLayer, const string &sIn) {
  //bool enableOutput = true;
  bool enableOutput = false;

  istringstream istr(sIn);
  string word;

  stringstream ssDefault;
  stringstream ssLayer;    
  stringstream ssPrl;
  stringstream ssCutClass; 
  
  string keyword = "";
  while (istr >> word) {
    if (word == string("SPACINGTABLE")) {
      if (enableOutput) {
        cout <<endl <<"  SPACINGTABLE";
      }
    } else if (word == string("DEFAULT")) {
      keyword = "DEFAULT";
      ssDefault <<word;
    } else if (word == string("SAMEMASK")) {
      keyword = "SAMEMASK";
    } else if (word == string("SAMENET") || word == string("SAMEMETAL") || word == string("SAMEVIA")) {
      keyword = "SAMENETMETALVIA";
    } else if (word == string("LAYER")) {
      keyword = "LAYER";
      ssLayer <<word;
    } else if (word == string("CENTERTOCENTER")) {
      keyword = "CENTERTOCENTER";
    } else if (word == string("CENTERANDEDGE")) {
      keyword = "CENTERANDEDGE";
    } else if (word == string("PRL")) {
      keyword = "PRL";
      ssPrl <<"PRL";
    } else if (word == string("PRLTWOSIDES")) {
      keyword = "PRLTWOSIDES";
    } else if (word == string("ENDEXTENSION")) {
      keyword = "ENDEXTENSION";
    } else if (word == string("EXACTALIGNEDSPACING")) {
      keyword = "EXACTALIGNEDSPACING";
    } else if (word == string("NONOPPOSITEENCLOSURESPACING")) {
      keyword = "NONOPPOSITEENCLOSURESPACING";
    } else if (word == string("OPPOSITEENCLOSURERESIZESPACING")) {
      keyword = "OPPOSITEENCLOSURERESIZESPACING";
    } else if (word == string("CUTCLASS")) {
      keyword = "CUTCLASS";
      ssCutClass <<word;
    } else {
      if (keyword == "DEFAULT") {
        ssDefault <<" " <<word;
      } else if (keyword == "CUTCLASS") {
        ssCutClass <<" " <<word;
      } else if (keyword == "PRL") {
        ssPrl <<" " <<word;
      } else if (keyword == "LAYER") {
        ssLayer <<" " <<word;
      } else {
        ;
      }
    }
  }

  auto con = make_shared<frLef58CutSpacingTableConstraint>();

  bool hasSecondLayer       = false;
  frLayerNum secondLayerNum = 0;

  bool isFirstViaLayerHavingSecondLayerNum = false;

  if (ssDefault.str() != "") {
    getLef58CutSpacingTable_default(data, tmpLayer, ssDefault.str(), con);
  }
  if (ssPrl.str() != "") {
    getLef58CutSpacingTable_prl(data, tmpLayer, ssPrl.str(), con);
  }
  if (ssLayer.str() != "") {
    if (tmpLayer->getLayerNum() == 1) {
      isFirstViaLayerHavingSecondLayerNum = true;
    } else {
      getLef58CutSpacingTable_layer(data, tmpLayer, ssLayer.str(), con, secondLayerNum);
    }
  }
  if (ssCutClass.str() != "" && !isFirstViaLayerHavingSecondLayerNum) {
    getLef58CutSpacingTable_cutClass(data, tmpLayer, ssCutClass.str(), con, hasSecondLayer, secondLayerNum);
  }

  if (isFirstViaLayerHavingSecondLayerNum) {
    ;
  } else {
    tmpLayer->lef58CutSpacingTableConstraints.push_back(con);
    io::Parser* parser = (io::Parser*) data;
    parser->tech->addConstraint(con);
  }

  return 0;
}

void io::Parser::setRoutingLayerProperties(odb::dbTechLayer* layer, frLayer* tmpLayer)
{
  if(layer->isRectOnly())
  {
    auto rectOnlyConstraint = make_unique<frLef58RectOnlyConstraint>(layer->isExceptNonCorePins());
    tmpLayer->setLef58RectOnlyConstraint(rectOnlyConstraint.get());
    tech->addUConstraint(std::move(rectOnlyConstraint));
  }
  if(layer->isRightWayOnGridOnly())
  {
    auto rightWayOnGridOnlyConstraint = make_unique<frLef58RightWayOnGridOnlyConstraint>(layer->isCheckMask());
    tmpLayer->setLef58RightWayOnGridOnlyConstraint(rightWayOnGridOnlyConstraint.get());
    tech->addUConstraint(std::move(rightWayOnGridOnlyConstraint));
  }
  for(auto rule : layer->getTechLayerMinStepRules())
  {
    auto con = make_unique<frLef58MinStepConstraint>();
    con->setMinStepLength(rule->getMinStepLength());
    con->setMaxEdges(rule->isMaxEdgesValid()?rule->getMaxEdges():-1);
    con->setMinAdjacentLength(rule->isMinAdjLength1Valid()?rule->getMinAdjLength1():-1);
    con->setEolWidth(rule->isNoBetweenEol()?rule->getEolWidth():-1);
    tmpLayer->addLef58MinStepConstraint(con.get());
    tech->addUConstraint(std::move(con));
  }
  for(auto rule : layer->getTechLayerCornerSpacingRules())
  {
    std::string widthName("WIDTH");
    std::vector<frCoord> widths;
    std::vector<std::pair<frCoord, frCoord> > spacings;
    rule->getSpacingTable(spacings);
    rule->getWidthTable(widths);

    fr1DLookupTbl<frCoord, std::pair<frCoord, frCoord> > cornerSpacingTbl(widthName, widths, spacings);
    unique_ptr<frConstraint> uCon = make_unique<frLef58CornerSpacingConstraint>(cornerSpacingTbl);
    auto rptr = static_cast<frLef58CornerSpacingConstraint*>(uCon.get());
    switch (rule->getType())
    {
      case odb::dbTechLayerCornerSpacingRule::CornerType::CONVEXCORNER:
        rptr->setCornerType(frCornerTypeEnum::CONVEX);
        rptr->setSameMask(rule->isSameMask());
        if (rule->isCornerOnly()) {
          rptr->setWithin(rule->getWithin());
        }
        if (rule->isExceptEol()) {
          rptr->setEolWidth(rule->getEolWidth());
          if (rule->isExceptJogLength()) {
            rptr->setLength(rule->getJogLength());
            rptr->setEdgeLength(rule->isEdgeLengthValid());
            rptr->setIncludeLShape(rule->isIncludeShape());
          }
        }
        
        break;
      
      default:
        rptr->setCornerType(frCornerTypeEnum::CONCAVE);
        if (rule->isMinLengthValid()) {
          rptr->setMinLength(rule->getMinLength());
        }
        rptr->setExceptNotch(rule->isExceptNotch());
        if (rule->isExceptNotchLengthValid()) {
          rptr->setExceptNotchLength(rule->getExceptNotchLength());
        }
        break;
    }
    rptr->setExceptSameNet(rule->isExceptSameNet());
    rptr->setExceptSameMetal(rule->isExceptSameMetal());
    rptr->setSameXY(rule->isSameXY());
    tech->addUConstraint(std::move(uCon));
    tmpLayer->addLef58CornerSpacingConstraint(rptr);
  }
  for(auto rule : layer->getTechLayerSpacingTablePrlRules())
  {
    string rowName("WIDTH");
    string colName("PARALLELRUNLENGTH");
    frCollection<frCoord> rowVals, colVals;
    frCollection<frCollection<frCoord> > tblVals;
    map<frCoord, pair<frCoord, frCoord> > ewVals;
    map<frUInt4, pair<frCoord, frCoord> > _ewVals;
    rule->getTable(rowVals, colVals, tblVals, _ewVals);
    for(auto &[key, value] : _ewVals) ewVals[key] = value;
    shared_ptr<fr2DLookupTbl<frCoord, frCoord, frCoord> > prlTbl = make_shared<
      fr2DLookupTbl<frCoord, frCoord, frCoord> >(rowName, rowVals, colName, colVals, tblVals);
    shared_ptr<frLef58SpacingTableConstraint> spacingTableConstraint = make_shared<frLef58SpacingTableConstraint>(prlTbl, ewVals);
    spacingTableConstraint->setWrongDirection(rule->isWrongDirection());
    spacingTableConstraint->setSameMask(rule->isSameMask());
    if (rule->isExceeptEol()) {
      spacingTableConstraint->setEolWidth(rule->getEolWidth());
    }
    
    tech->addConstraint(spacingTableConstraint);
    tmpLayer->addConstraint(spacingTableConstraint);
  }
  for(auto rule : layer->getTechLayerSpacingEolRules())
  {
    auto con = make_shared<frLef58SpacingEndOfLineConstraint>();
    con->setEol(rule->getEolSpace(), rule->getEolWidth(), rule->isExactWidthValid());
    if (rule->isWrongDirSpacingValid()) {
      con->setWrongDirSpace(rule->getWrongDirSpace());
    }

    auto within = make_shared<frLef58SpacingEndOfLineWithinConstraint>();
    con->setWithinConstraint(within);
    if (rule->isOppositeWidthValid()) {
      within->setOppositeWidth(rule->getOppositeWidth());
    }
    within->setEolWithin(rule->getEolWithin());
    if (rule->isWrongDirWithinValid()) {
      within->setWrongDirWithin(rule->getWrongDirWithin());
    }
    if (rule->isSameMaskValid()) {
      within->setSameMask(rule->isSameMaskValid());
    }
    if (rule->isEndToEndValid()) {
      auto endToEnd = make_shared<frLef58SpacingEndOfLineWithinEndToEndConstraint>();
      within->setEndToEndConstraint(endToEnd);
      endToEnd->setEndToEndSpace(rule->getEndToEndSpace());
      if (rule->isCutSpacingValid()) {
        endToEnd->setCutSpace(rule->getOneCutSpace(), rule->getTwoCutSpace());
      }
      if (rule->isExtensionValid()) {
        if (rule->isWrongDirExtensionValid()) {
          endToEnd->setExtension(rule->getExtension(), rule->getWrongDirExtension());
        } else {
          endToEnd->setExtension(rule->getExtension());
        }
      }
      if (rule->isOtherEndWidthValid()) {
        endToEnd->setOtherEndWidth(rule->getOtherEndWidth());
      }
    }
    if (rule->isParallelEdgeValid()) {
      auto parallelEdge = make_shared<frLef58SpacingEndOfLineWithinParallelEdgeConstraint>();
      within->setParallelEdgeConstraint(parallelEdge);
      if (rule->isSubtractEolWidthValid()) {
        parallelEdge->setSubtractEolWidth(rule->isSubtractEolWidthValid());
      }
      parallelEdge->setPar(rule->getParSpace(), rule->getParWithin());
      if (rule->isParPrlValid()) {
        parallelEdge->setPrl(rule->getParPrl());
      }
      if (rule->isParMinLengthValid()) {
        parallelEdge->setMinLength(rule->getParMinLength());
      }
      if (rule->isTwoEdgesValid()) {
        parallelEdge->setTwoEdges(rule->isTwoEdgesValid());
      }
      if (rule->isSameMetalValid()) {
        parallelEdge->setSameMetal(rule->isSameMetalValid());
      }
      if (rule->isNonEolCornerOnlyValid()) {
        parallelEdge->setNonEolCornerOnly(rule->isNonEolCornerOnlyValid());
      }
      if (rule->isParallelSameMaskValid()) {
        parallelEdge->setParallelSameMask(rule->isParallelSameMaskValid());
      }
    }
    if (rule->isMinLengthValid() || rule->isMaxLengthValid()) {
      auto len = make_shared<frLef58SpacingEndOfLineWithinMaxMinLengthConstraint>();
      within->setMaxMinLengthConstraint(len);
      if(rule->isMinLengthValid())
        len->setLength(false, rule->getMinLength(), rule->isTwoEdgesValid());
      else
        len->setLength(true, rule->getMaxLength(), rule->isTwoEdgesValid());
    }
    tech->addConstraint(con);
    tmpLayer->lef58SpacingEndOfLineConstraints.push_back(con);
  }
}

void io::Parser::setCutLayerProperties(odb::dbTechLayer* layer, frLayer* tmpLayer)
{
    for(auto rule : layer->getTechLayerCutClassRules())
    {
      auto cutClass = make_unique<frLef58CutClass>();
      cutClass->setName(frString(rule->getName()));
      cutClass->setViaWidth(rule->getWidth());
      if (rule->isLengthValid()) {
        cutClass->setViaLength(rule->getLength());
      } else {
        cutClass->setViaLength(rule->getWidth());
      }
      if (rule->isCutsValid()) {
        cutClass->setNumCut(rule->getNumCuts());
      } else {
        cutClass->setNumCut(1);
      }
      tech->addCutClass(tmpLayer->getLayerNum(), std::move((cutClass)));
    }
    for(auto rule : layer->getTechLayerCutSpacingRules())
    {
      switch (rule->getType())
      {
      case odb::dbTechLayerCutSpacingRule::CutSpacingType::ADJACENTCUTS:
        auto con = make_unique<frLef58CutSpacingConstraint>();
        con->setCutSpacing(rule->getCutSpacing());
        con->setCenterToCenter(rule->isCenterToCenter());
        con->setSameNet(rule->isSameNet());
        con->setSameMetal(rule->isSameMetal());
        con->setSameVia(rule->isSameVias());
        con->setAdjacentCuts(rule->getNumCuts());
        if(rule->isExactAligned())
        con->setExactAlignedCut(rule->getNumCuts());
        con->setTwoCuts(rule->isTwoCutsValid());
        con->setTwoCutsSpacing(rule->getTwoCuts());
        con->setSameCut(rule->isSameCut());
        con->setCutWithin(rule->getWithin());
        con->setExceptSamePGNet(rule->isExceptSamePgnet());
        con->setExceptAllWithin(rule->isExcept);
        con->setAbove(isAbove);
        con->setBelow(isBelow);
        con->setEnclosure(enclosure);
        con->setCutClassName(cutClassName);
        con->setCutClassIdx(cutClassIdx);
        con->setToAll(isToAll);
        con->setNoPrl(isNoPrl);
        con->setSideParallelOverlap(isSideParallelOverlap);
        con->setSameMask(isSameMask);

        tmpLayer->addLef58CutSpacingConstraint(con.get());
        parser->tech->addUConstraint(std::move(con));
        break;
      
      default:
        if(rule->getSecondLayer() == nullptr)
          continue;
        auto con = make_unique<frLef58CutSpacingConstraint>();
        con->setCutSpacing(rule->getCutSpacing());
        con->setCenterToCenter(rule->isCenterToCenter());
        con->setSameNet(rule->isSameNet());
        con->setSameMetal(rule->isSameMetal());
        con->setSameVia(rule->isSameVia());
        con->setSecondLayerName(rule->getSecondLayer()->getName());
        con->setStack(rule->isStack());
        if(rule->isOrthogonalSpacingValid())
        {
          con->setOrthogonalSpacing(rule->getOrthogonalSpacing());
        }
        if(rule->getCutClass()!=nullptr)
        {
          std::string className = rule->getCutClass()->getName();
          con->setCutClassName(className);
          auto cutClassIdx = tmpLayer->getCutClassIdx(className);
          if (cutClassIdx != -1)
              con->setCutClassIdx(cutClassIdx);
          else
            continue;
          con->setShortEdgeOnly(rule->isShortEdgeOnly());
          con->
        }

        break;
      }
    }
}

void io::Parser::addDefaultMasterSliceLayer()
{
  // TODO REPLACE WITH ACTUAL MASTERSLICE NAME
  unique_ptr<frLayer> uMSLayer = make_unique<frLayer>();
  auto tmpMSLayer = uMSLayer.get();
  tmpMSLayer->setLayerNum(readLayerCnt++);
  tmpMSLayer->setName("FR_MASTERSLICE");
  tech->addLayer(std::move(uMSLayer));
  tmpMSLayer->setType(frLayerTypeEnum::MASTERSLICE);
}

void io::Parser::addDefaultCutLayer()
{
  std::string viaLayerName("FR_VIA");
  unique_ptr<frLayer> uCutLayer = make_unique<frLayer>();
  auto tmpCutLayer = uCutLayer.get();
  tmpCutLayer->setLayerNum(readLayerCnt++);
  tmpCutLayer->setName(viaLayerName);
  tech->addLayer(std::move(uCutLayer));
  tmpCutLayer->setType(frLayerTypeEnum::CUT);
}

void io::Parser::addRoutingLayer(odb::dbTechLayer* layer)
{
  if (readLayerCnt == 0) {
    addDefaultMasterSliceLayer();
    addDefaultCutLayer();
  }
  unique_ptr<frLayer> uLayer = make_unique<frLayer>();
  auto tmpLayer = uLayer.get();
  tmpLayer->setLayerNum(readLayerCnt++);
  tmpLayer->setName(layer->getName());
  tech->addLayer(std::move(uLayer));

  tmpLayer->setWidth(layer->getWidth());
  tmpLayer->setMinWidth(layer->getMinWidth());
  // add minWidth constraint
  auto minWidthConstraint
      = make_unique<frMinWidthConstraint>(tmpLayer->getMinWidth());
  tmpLayer->setMinWidthConstraint(minWidthConstraint.get());
  tech->addUConstraint(std::move(minWidthConstraint));

  tmpLayer->setType(frLayerTypeEnum::ROUTING);
  if (layer->getDirection() == odb::dbTechLayerDir::HORIZONTAL)
    tmpLayer->setDir(frcHorzPrefRoutingDir);
  else if (layer->getDirection() == odb::dbTechLayerDir::VERTICAL)
    tmpLayer->setDir(frcVertPrefRoutingDir);

  tmpLayer->setPitch(layer->getPitch());

  // Add off grid rule for every layer
  auto recheckConstraint = make_unique<frRecheckConstraint>();
  tmpLayer->setRecheckConstraint(recheckConstraint.get());
  tech->addUConstraint(std::move(recheckConstraint));

  // Add short rule for every layer
  auto shortConstraint = make_unique<frShortConstraint>();
  tmpLayer->setShortConstraint(shortConstraint.get());
  tech->addUConstraint(std::move(shortConstraint));

  // Add off grid rule for every layer
  auto offGridConstraint = make_unique<frOffGridConstraint>();
  tmpLayer->setOffGridConstraint(offGridConstraint.get());
  tech->addUConstraint(std::move(offGridConstraint));

  // Add nsmetal rule for every layer
  auto nsmetalConstraint = make_unique<frNonSufficientMetalConstraint>();
  tmpLayer->setNonSufficientMetalConstraint(nsmetalConstraint.get());

  tech->addUConstraint(std::move(nsmetalConstraint));
  for (auto prop : odb::dbProperty::getProperties(layer))
    if (prop->getType() == odb::dbProperty::Type::STRING_PROP) {
      odb::dbStringProperty* strProp = (odb::dbStringProperty*) prop;
      if (string(strProp->getName()) == string("LEF58_PROTRUSIONWIDTH")
          || string(strProp->getName()) == string("LEF58_ENCLOSURESPACING")
          || string(strProp->getName()) == string("LEF58_VOLTAGESPACING")
          || string(strProp->getName()) == string("LEF58_ANTENNAGATEPLUSDIFF")
          || string(strProp->getName()) == string("LEF58_ANTENNAGATEPWL")
          || string(strProp->getName()) == string("LEF58_ANTENNAGATEPWL")
          || string(strProp->getName()) == string("LEF58_FORBIDDENSPACING")) {
        ;
      } else {
        if (!strcmp(strProp->getName().c_str(), "LEF58_CORNERSPACING")) {
          getLef58CornerSpacing(this, tmpLayer, strProp->getValue());
        } else if (!strcmp(strProp->getName().c_str(), "LEF58_SPACING")) {
          getLef58Spacing(this, tmpLayer, strProp->getValue());
        } else if (!strcmp(strProp->getName().c_str(), "LEF57_SPACING")) {
          getLef58Spacing(this, tmpLayer, strProp->getValue());
        } else if (!strcmp(strProp->getName().c_str(), "LEF58_SPACINGTABLE")) {
          getLef58SpacingTable(this, tmpLayer, strProp->getValue());
        } else if (!strcmp(strProp->getName().c_str(), "LEF58_RIGHTWAYONGRIDONLY")) {
          getLef58RightWayOnGridOnly(this, tmpLayer, strProp->getValue());
        } else if (!strcmp(strProp->getName().c_str(), "LEF58_RECTONLY")) {
          getLef58RectOnly(this, tmpLayer, strProp->getValue());
        } else if (!strcmp(strProp->getName().c_str(), "LEF58_MINSTEP")) {
          getLef58MinStep(this, tmpLayer, strProp->getValue());
        } else {
          ;
        }
      }
    }

  // read minArea rule
  if (layer->hasArea()) {
    frCoord minArea = frCoord(
        round(layer->getArea() * tech->getDBUPerUU() * tech->getDBUPerUU()));
    unique_ptr<frConstraint> uCon = make_unique<frAreaConstraint>(minArea);
    auto rptr = static_cast<frAreaConstraint*>(uCon.get());
    tech->addUConstraint(std::move(uCon));
    tmpLayer->setAreaConstraint(rptr);
  }

  if (layer->hasMinStep()) {
    unique_ptr<frConstraint> uCon = make_unique<frMinStepConstraint>();
    auto rptr = static_cast<frMinStepConstraint*>(uCon.get());
    rptr->setInsideCorner(layer->getMinStepType()
                          == odb::dbTechLayerMinStepType::INSIDE_CORNER);
    rptr->setOutsideCorner(layer->getMinStepType()
                           == odb::dbTechLayerMinStepType::OUTSIDE_CORNER);
    rptr->setStep(layer->getMinStepType() == odb::dbTechLayerMinStepType::STEP);
    switch (layer->getMinStepType()) {
      case odb::dbTechLayerMinStepType::INSIDE_CORNER:
        rptr->setMinstepType(frMinstepTypeEnum::INSIDECORNER);
        break;
      case odb::dbTechLayerMinStepType::OUTSIDE_CORNER:
        rptr->setMinstepType(frMinstepTypeEnum::OUTSIDECORNER);
        break;
      case odb::dbTechLayerMinStepType::STEP:
        rptr->setMinstepType(frMinstepTypeEnum::STEP);
        break;
      default:
        break;
    }
    if (layer->hasMinStepMaxLength())
      rptr->setMaxLength(layer->getMinStepMaxLength());
    if (layer->hasMinStepMaxEdges()) {
      rptr->setMaxEdges(layer->getMinStepMaxEdges());
      rptr->setInsideCorner(true);
      rptr->setOutsideCorner(true);
      rptr->setStep(true);
      rptr->setMinstepType(frMinstepTypeEnum::UNKNOWN);
    }
    rptr->setMinStepLength(layer->getMinStep());
    tech->addUConstraint(std::move(uCon));
    tmpLayer->setMinStepConstraint(rptr);
  }

  // read minHole rule
  std::vector<odb::dbTechMinEncRule*> minEncRules;
  layer->getMinEnclosureRules(minEncRules);
  for (odb::dbTechMinEncRule* rule : minEncRules) {
    frUInt4 _minEnclosedWidth = -1;
    bool hasMinenclosedareaWidth = rule->getEnclosureWidth(_minEnclosedWidth);
    if (hasMinenclosedareaWidth) {
      logger->warn(
          utl::DRT,
          139,
          "minEnclosedArea constraint with width is not supported, skipped");
      continue;
    }
    frUInt4 _minEnclosedArea;
    rule->getEnclosure(_minEnclosedArea);
    frCoord minEnclosedArea = _minEnclosedArea;
    auto minEnclosedAreaConstraint
        = make_unique<frMinEnclosedAreaConstraint>(minEnclosedArea);
    tmpLayer->addMinEnclosedAreaConstraint(minEnclosedAreaConstraint.get());
    tech->addUConstraint(std::move(minEnclosedAreaConstraint));
  }

  // read spacing rule
  odb::dbSet<odb::dbTechLayerSpacingRule> sp_rules;
  layer->getV54SpacingRules(sp_rules);
  for (auto rule : sp_rules) {
    frCoord minSpacing = rule->getSpacing();
    frUInt4 _eolWidth = 0, _eolWithin = 0, _parSpace = 0, _parWithin = 0;
    bool hasSpacingParellelEdge = false;
    bool hasSpacingTwoEdges = false;
    bool hasSpacingEndOfLine = rule->getEol(_eolWidth,
                                            _eolWithin,
                                            hasSpacingParellelEdge,
                                            _parSpace,
                                            _parWithin,
                                            hasSpacingTwoEdges);
    frCoord eolWidth(_eolWidth), eolWithin(_eolWithin), parSpace(_parSpace),
        parWithin(_parWithin);
    if (rule->hasRange()) {
      logger->warn(utl::DRT, 140, "SpacingRange unsupported");
    } else if (rule->hasLengthThreshold()) {
      logger->warn(utl::DRT, 141, "SpacingLengthThreshold unsupported");
    } else if (rule->hasSpacingNotchLength()) {
      logger->warn(utl::DRT, 142, "SpacingNotchLength unsupported");
    } else if (rule->hasSpacingEndOfNotchWidth()) {
      logger->warn(utl::DRT, 143, "SpacingEndOfNotchWidth unsupported");
    } else if (hasSpacingEndOfLine) {
      unique_ptr<frConstraint> uCon
          = make_unique<frSpacingEndOfLineConstraint>();
      auto rptr = static_cast<frSpacingEndOfLineConstraint*>(uCon.get());
      rptr->setMinSpacing(minSpacing);
      rptr->setEolWidth(eolWidth);
      rptr->setEolWithin(eolWithin);
      if (hasSpacingParellelEdge) {
        rptr->setParSpace(parSpace);
        rptr->setParWithin(parWithin);
        rptr->setTwoEdges(hasSpacingTwoEdges);
      }
      tech->addUConstraint(std::move(uCon));
      tmpLayer->addEolSpacing(rptr);
    } else if (rule->getCutSameNet()) {
      bool pgOnly = rule->getSameNetPgOnly();
      unique_ptr<frConstraint> uCon
          = make_unique<frSpacingSamenetConstraint>(minSpacing, pgOnly);
      auto rptr = uCon.get();
      tech->addUConstraint(std::move(uCon));
      if (tmpLayer->hasSpacingSamenet()) {
        logger->warn(utl::DRT,
                     138,
                     "new SPACING SAMENET overrides old SPACING SAMENET rule");
      }
      tmpLayer->setSpacingSamenet(
          static_cast<frSpacingSamenetConstraint*>(rptr));
    } else {
      frCollection<frCoord> rowVals(1, 0), colVals(1, 0);
      frCollection<frCollection<frCoord>> tblVals(1, {minSpacing});
      frString rowName("WIDTH"), colName("PARALLELRUNLENGTH");
      unique_ptr<frConstraint> uCon = make_unique<frSpacingTablePrlConstraint>(
          fr2DLookupTbl(rowName, rowVals, colName, colVals, tblVals));
      auto rptr = static_cast<frSpacingTablePrlConstraint*>(uCon.get());
      tech->addUConstraint(std::move(uCon));
      if (tmpLayer->getMinSpacing())
        logger->warn(utl::DRT,
                     144,
                     "new SPACING SAMENET overrides old SPACING SAMENET rule");
      tmpLayer->setMinSpacing(rptr);
    }
  }

  // read prl spacingTable
  if (layer->hasV55SpacingRules()) {
    frCollection<frUInt4> _rowVals, _colVals;
    frCollection<frCollection<frUInt4>> _tblVals;
    layer->getV55SpacingWidthsAndLengths(_rowVals, _colVals);
    layer->getV55SpacingTable(_tblVals);
    frCollection<frCoord> rowVals(_rowVals.begin(), _rowVals.end());
    frCollection<frCoord> colVals(_colVals.begin(), _colVals.end());
    frCollection<frCollection<frCoord>> tblVals;
    tblVals.resize(_tblVals.size());
    for (size_t i = 0; i < _tblVals.size(); i++)
      for (size_t j = 0; j < _tblVals[i].size(); j++)
        tblVals[i].push_back(_tblVals[i][j]);

    std::shared_ptr<frSpacingTableConstraint> spacingTableConstraint;
    shared_ptr<fr2DLookupTbl<frCoord, frCoord, frCoord>> prlTbl;
    frString rowName("WIDTH"), colName("PARALLELRUNLENGTH");

    // old
    prlTbl = make_shared<fr2DLookupTbl<frCoord, frCoord, frCoord>>(
        rowName, rowVals, colName, colVals, tblVals);
    spacingTableConstraint = make_shared<frSpacingTableConstraint>(prlTbl);
    tech->addConstraint(spacingTableConstraint);
    tmpLayer->addConstraint(spacingTableConstraint);
    // new
    unique_ptr<frConstraint> uCon = make_unique<frSpacingTablePrlConstraint>(
        fr2DLookupTbl(rowName, rowVals, colName, colVals, tblVals));
    auto rptr = static_cast<frSpacingTablePrlConstraint*>(uCon.get());
    tech->addUConstraint(std::move(uCon));
    if (tmpLayer->getMinSpacing())
      logger->warn(
          utl::DRT,
          145,
          "new SPACINGTABLE PARALLELRUNLENGTH overrides old SPACING rule");
    tmpLayer->setMinSpacing(rptr);
  }

  if (layer->hasTwoWidthsSpacingRules()) {
    frCollection<frCollection<frUInt4>> _tblVals;
    layer->getTwoWidthsSpacingTable(_tblVals);
    frCollection<frCollection<frCoord>> tblVals;
    tblVals.resize(_tblVals.size());
    for (size_t i = 0; i < _tblVals.size(); i++)
      for (size_t j = 0; j < _tblVals[i].size(); j++)
        tblVals[i].push_back(_tblVals[i][j]);

    frCoord defaultPrl = -abs(tblVals[0][0]);

    frCollection<frSpacingTableTwRowType> rowVals, colVals;
    frString rowName("WIDTH1PRL"), colName("WIDTH2PRL");

    for (uint j = 0; j < layer->getTwoWidthsSpacingTableNumWidths(); ++j) {
      frCoord width = layer->getTwoWidthsSpacingTableWidth(j);
      frCoord prl = defaultPrl;

      if (layer->getTwoWidthsSpacingTableHasPRL(j)) {
        prl = layer->getTwoWidthsSpacingTablePRL(j);
        defaultPrl = prl;
      }
      colVals.push_back(frSpacingTableTwRowType(width, prl));
      rowVals.push_back(frSpacingTableTwRowType(width, prl));
    }
    unique_ptr<frConstraint> uCon = make_unique<frSpacingTableTwConstraint>(
        fr2DLookupTbl(rowName, rowVals, colName, colVals, tblVals));
    auto rptr = static_cast<frSpacingTableTwConstraint*>(uCon.get());
    tech->addUConstraint(std::move(uCon));
    if (tmpLayer->getMinSpacing())
      logger->warn(utl::DRT,
                   146,
                   "new SPACINGTABLE TWOWIDTHS overrides old SPACING rule");
    tmpLayer->setMinSpacing(rptr);
  }

  for (auto rule : layer->getMinCutRules()) {
    frUInt4 numCuts, width, within, length, distance;
    if (!rule->getMinimumCuts(numCuts, width))
      continue;
    unique_ptr<frConstraint> uCon = make_unique<frMinimumcutConstraint>();
    auto rptr = static_cast<frMinimumcutConstraint*>(uCon.get());
    rptr->setNumCuts(numCuts);
    rptr->setWidth(width);
    if (rule->getCutDistance(within))
      rptr->setWithin(within);
    if (rule->isAboveOnly())
      rptr->setConnection(frMinimumcutConnectionEnum::FROMABOVE);
    if (rule->isBelowOnly())
      rptr->setConnection(frMinimumcutConnectionEnum::FROMBELOW);
    if (rule->getLengthForCuts(length, distance))
      rptr->setLength(length, distance);
    tech->addUConstraint(std::move(uCon));
    tmpLayer->addMinimumcutConstraint(rptr);
  }
}

void io::Parser::addCutLayer(odb::dbTechLayer* layer)
{
  if (readLayerCnt == 0)
    addDefaultMasterSliceLayer();

  unique_ptr<frLayer> uLayer = make_unique<frLayer>();
  auto tmpLayer = uLayer.get();
  tmpLayer->setLayerNum(readLayerCnt++);
  tmpLayer->setName(layer->getName());
  tmpLayer->setType(frLayerTypeEnum::CUT);
  tech->addLayer(std::move(uLayer));

  auto shortConstraint = make_shared<frShortConstraint>();
  tech->addConstraint(shortConstraint);
  tmpLayer->addConstraint(shortConstraint);
  tmpLayer->setShortConstraint(shortConstraint.get());

  // read spacing constraint
  odb::dbSet<odb::dbTechLayerSpacingRule> spRules;
  layer->getV54SpacingRules(spRules);
  for (odb::dbTechLayerSpacingRule* rule : spRules) {
    std::shared_ptr<frCutSpacingConstraint> cutSpacingConstraint;
    frCoord cutArea = rule->getCutArea();
    frCoord cutSpacing = rule->getSpacing();
    bool centerToCenter = rule->getCutCenterToCenter();
    bool sameNet = rule->getCutSameNet();
    bool stack = rule->getCutStacking();
    bool exceptSamePGNet = rule->getSameNetPgOnly();
    bool parallelOverlap = rule->getCutParallelOverlap();
    odb::dbTechLayer* outly;
    frString secondLayerName = string("");
    if (rule->getCutLayer4Spacing(outly))
      secondLayerName = string(outly->getName());
    frUInt4 _adjacentCuts;
    frUInt4 within;
    frUInt4 spacing;
    bool except_same_pgnet;
    frCoord cutWithin = 0;
    int adjacentCuts = 0;
    if (rule->getAdjacentCuts(
            _adjacentCuts, within, spacing, except_same_pgnet)) {
      adjacentCuts = _adjacentCuts;
      cutWithin = within;
    }

    // initialize for invalid variables
    cutArea = (cutArea == 0) ? -1 : cutArea;
    cutWithin = (cutWithin == 0) ? -1 : cutWithin;
    adjacentCuts = (adjacentCuts == 0) ? -1 : adjacentCuts;

    if (cutWithin != -1 && cutWithin < cutSpacing) {
      logger->warn(utl::DRT,
                   147,
                   "cutWithin is smaller than cutSpacing for ADJACENTCUTS on "
                   "layer {}, please check your rule definition",
                   layer->getName());
    }
    cutSpacingConstraint = make_shared<frCutSpacingConstraint>(cutSpacing,
                                                               centerToCenter,
                                                               sameNet,
                                                               secondLayerName,
                                                               stack,
                                                               adjacentCuts,
                                                               cutWithin,
                                                               exceptSamePGNet,
                                                               parallelOverlap,
                                                               cutArea);

    tech->addConstraint(cutSpacingConstraint);
    tmpLayer->addConstraint(cutSpacingConstraint);
    tmpLayer->addCutSpacingConstraint(cutSpacingConstraint.get());
  }

  // lef58
  for (auto prop : odb::dbProperty::getProperties(layer))
    if (prop->getType() == odb::dbProperty::Type::STRING_PROP) {
      odb::dbStringProperty* strProp = (odb::dbStringProperty*) prop;

      if (string(strProp->getName().c_str()) == string("LEF58_ENCLOSUREEDGE")
          || string(strProp->getName().c_str()) == string("LEF58_ENCLOSURE")
          || string(strProp->getName().c_str())
                 == string("LEF58_ENCLOSURETABLE")) {
        ;
      } else {
        if (!strcmp(strProp->getName().c_str(), "LEF58_CUTCLASS")) {
          getLef58CutClass(this, tmpLayer, strProp->getValue());
        } else if (!strcmp(strProp->getName().c_str(), "LEF58_SPACING")) {
          getLef58CutSpacing(this, tmpLayer, strProp->getValue());
        } else if (!strcmp(strProp->getName().c_str(), "LEF58_SPACINGTABLE")) {
          getLef58CutSpacingTable(this, tmpLayer, strProp->getValue());
        } else {
          ;
        }
      }
    }
}

void io::Parser::addMasterSliceLayer(odb::dbTechLayer* layer)
{
  if (layer->getLef58Type() != odb::dbTechLayer::LEF58_TYPE::NWELL &&
      layer->getLef58Type() != odb::dbTechLayer::LEF58_TYPE::PWELL)
    masterSliceLayerName = string(layer->getName());
}

void io::Parser::setLayers(odb::dbTech* tech)
{
  masterSliceLayerName = "FR_MASTERSLICE";
  for (auto layer : tech->getLayers()) {
    switch (layer->getType().getValue()) {
      case odb::dbTechLayerType::ROUTING:
        addRoutingLayer(layer);
        break;
      case odb::dbTechLayerType::CUT:
        addCutLayer(layer);
        break;
      case odb::dbTechLayerType::MASTERSLICE:
        addMasterSliceLayer(layer);
        break;
      default:
        break;
    }
  }
}

void io::Parser::setMacros(odb::dbDatabase* db)
{
  for (auto lib : db->getLibs()) {
    for (odb::dbMaster* master : lib->getMasters()) {
      tmpBlock = make_unique<frBlock>(master->getName());
      frCoord originX;
      frCoord originY;
      master->getOrigin(originX, originY);
      frCoord sizeX = master->getWidth();
      frCoord sizeY = master->getHeight();
      vector<frBoundary> bounds;
      frBoundary bound;
      vector<frPoint> points;
      points.push_back(frPoint(originX, originY));
      points.push_back(frPoint(sizeX, originY));
      points.push_back(frPoint(sizeX, sizeY));
      points.push_back(frPoint(originX, sizeY));
      bound.setPoints(points);
      bounds.push_back(bound);
      tmpBlock->setBoundaries(bounds);
      const char* macroClass = master->getType().getString();
      if (strcmp(macroClass, "NONE")) {
        if (strcmp(macroClass, "CORE") == 0) {
          tmpBlock->setMacroClass(MacroClassEnum::CORE);
        } else if (strcmp(macroClass, "CORE TIEHIGH") == 0) {
          tmpBlock->setMacroClass(MacroClassEnum::CORE_TIEHIGH);
        } else if (strcmp(macroClass, "CORE TIELOW") == 0) {
          tmpBlock->setMacroClass(MacroClassEnum::CORE_TIELOW);
        } else if (strcmp(macroClass, "CORE WELLTAP") == 0) {
          tmpBlock->setMacroClass(MacroClassEnum::CORE_WELLTAP);
        } else if (strcmp(macroClass, "CORE SPACER") == 0) {
          tmpBlock->setMacroClass(MacroClassEnum::CORE_SPACER);
        } else if (strcmp(macroClass, "CORE ANTENNACELL") == 0) {
          tmpBlock->setMacroClass(MacroClassEnum::CORE_ANTENNACELL);
        } else if (strcmp(macroClass, "COVER") == 0) {
          tmpBlock->setMacroClass(MacroClassEnum::COVER);
        } else if (strcmp(macroClass, "BLOCK") == 0) {
          tmpBlock->setMacroClass(MacroClassEnum::BLOCK);
        } else if (strcmp(macroClass, "PAD") == 0) {
          tmpBlock->setMacroClass(MacroClassEnum::PAD);
        } else if (strcmp(macroClass, "RING") == 0) {
          tmpBlock->setMacroClass(MacroClassEnum::RING);
        } else if (strcmp(macroClass, "PAD POWER") == 0) {
          tmpBlock->setMacroClass(MacroClassEnum::PAD_POWER);
        } else if (strcmp(macroClass, "PAD SPACER") == 0) {
          tmpBlock->setMacroClass(MacroClassEnum::PAD_SPACER);
        } else if (strcmp(macroClass, "ENDCAP") == 0) {
          tmpBlock->setMacroClass(MacroClassEnum::ENDCAP);
        } else if (strcmp(macroClass, "ENDCAP PRE") == 0) {
          tmpBlock->setMacroClass(MacroClassEnum::ENDCAP_PRE);
        } else if (strcmp(macroClass, "ENDCAP POST") == 0) {
          tmpBlock->setMacroClass(MacroClassEnum::ENDCAP_POST);
        } else if (strcmp(macroClass, "ENDCAP TOPLEFT") == 0) {
          tmpBlock->setMacroClass(MacroClassEnum::ENDCAP_TOPLEFT);
        } else if (strcmp(macroClass, "ENDCAP TOPRIGHT") == 0) {
          tmpBlock->setMacroClass(MacroClassEnum::ENDCAP_TOPRIGHT);
        } else if (strcmp(macroClass, "ENDCAP BOTTOMLEFT") == 0) {
          tmpBlock->setMacroClass(MacroClassEnum::ENDCAP_BOTTOMLEFT);
        } else if (strcmp(macroClass, "ENDCAP BOTTOMRIGHT") == 0) {
          tmpBlock->setMacroClass(MacroClassEnum::ENDCAP_BOTTOMRIGHT);
        } 
        else {
          logger->warn(utl::DRT,
                       137,
                       "unknown macroClass {}, skipped macroClass property",
                       macroClass);
        }
      }

      for (auto _term : master->getMTerms()) {
        unique_ptr<frTerm> uTerm = make_unique<frTerm>(_term->getName());
        auto term = uTerm.get();
        term->setId(numTerms);
        numTerms++;
        tmpBlock->addTerm(std::move(uTerm));

        frTermEnum termType = frTermEnum::frcNormalTerm;
        string str(_term->getSigType().getString());
        if (str == "SIGNAL") {
          ;
        } else if (str == "CLOCK") {
          termType = frTermEnum::frcClockTerm;
        } else if (str == "POWER") {
          termType = frTermEnum::frcPowerTerm;
        } else if (str == "GROUND") {
          termType = frTermEnum::frcGroundTerm;
        } else {
          logger->error(utl::DRT, 120, "unsupported PIN USE in lef");
        }
        term->setType(termType);
        frTermDirectionEnum termDirection = frTermDirectionEnum::UNKNOWN;
        str = string(_term->getIoType().getString());
        if (str == "INPUT") {
          termDirection = frTermDirectionEnum::INPUT;
        } else if (str == "OUTPUT") {
          termDirection = frTermDirectionEnum::OUTPUT;
        } else if (str == "OUTPUT TRISTATE") {
          termDirection = frTermDirectionEnum::OUTPUT;
        } else if (str == "INOUT") {
          termDirection = frTermDirectionEnum::INOUT;
        } else if (str == "FEEDTHRU") {
          termDirection = frTermDirectionEnum::FEEDTHRU;
        } else {
          logger->error(
              utl::DRT, 121, "unsupported term direction {} in lef", str);
        }
        term->setDirection(termDirection);

        int i = 0;
        for (auto mpin : _term->getMPins()) {
          auto pinIn = make_unique<frPin>();
          pinIn->setId(i++);
          for (auto box : mpin->getGeometry()) {
            frLayerNum layerNum = -1;
            string layer = box->getTechLayer()->getName();
            if (tech->name2layer.find(layer) == tech->name2layer.end()){
              logger->warn(utl::DRT,
                           122,
                           "layer {} is skipped for {}/{}",
                           layer,
                           tmpBlock->getName(),
                           _term->getName());
              continue;
            }
            else
              layerNum = tech->name2layer.at(layer)->getLayerNum();

            frCoord xl = box->xMin();
            frCoord yl = box->yMin();
            frCoord xh = box->xMax();
            frCoord yh = box->yMax();
            unique_ptr<frRect> pinFig = make_unique<frRect>();
            pinFig->setBBox(frBox(xl, yl, xh, yh));
            pinFig->addToPin(pinIn.get());
            pinFig->setLayerNum(layerNum);
            unique_ptr<frPinFig> uptr(std::move(pinFig));
            pinIn->addPinFig(std::move(uptr));
          }
          term->addPin(std::move(pinIn));
        }
      }

      for (auto obs : master->getObstructions()) {
        frLayerNum layerNum = -1;
        string layer = obs->getTechLayer()->getName();
        if (tech->name2layer.find(layer) == tech->name2layer.end()) {
          logger->warn(utl::DRT,
                       123,
                       "layer {} is skipped for {}/OBS",
                       layer,
                       tmpBlock->getName());
          continue;
        } else
          layerNum = tech->name2layer.at(layer)->getLayerNum();
        auto blkIn = make_unique<frBlockage>();
        blkIn->setId(numBlockages);
        numBlockages++;
        auto pinIn = make_unique<frPin>();
        pinIn->setId(0);
        frCoord xl = obs->xMin();
        frCoord yl = obs->yMin();
        frCoord xh = obs->xMax();
        frCoord yh = obs->yMax();
        // pinFig
        unique_ptr<frRect> pinFig = make_unique<frRect>();
        pinFig->setBBox(frBox(xl, yl, xh, yh));
        pinFig->addToPin(pinIn.get());
        pinFig->setLayerNum(layerNum);
        unique_ptr<frPinFig> uptr(std::move(pinFig));
        pinIn->addPinFig(std::move(uptr));
        blkIn->setPin(std::move(pinIn));
        tmpBlock->addBlockage(std::move(blkIn));
      }
      tmpBlock->setId(numRefBlocks + 1);
      design->addRefBlock(std::move(tmpBlock));
      numRefBlocks++;
      numTerms = 0;
      numBlockages = 0;
    }
  }
}

void io::Parser::setTechViaRules(odb::dbTech* _tech)
{
  for (auto rule : _tech->getViaGenerateRules()) {
    int count = rule->getViaLayerRuleCount();
    if (count != 3)
      logger->error(utl::DRT, 128, "unsupported viarule {}", rule->getName());
    map<frLayerNum, int> lNum2Int;
    for (int i = 0; i < count; i++) {
      auto layerRule = rule->getViaLayerRule(i);
      string layerName = layerRule->getLayer()->getName();
      if (tech->name2layer.find(layerName) == tech->name2layer.end())
        logger->error(utl::DRT,
                      129,
                      "unknown layer {} for viarule {}",
                      layerName,
                      rule->getName());
      frLayerNum lNum = tech->name2layer[layerName]->getLayerNum();
      lNum2Int[lNum] = 1;
    }
    int curOrder = 0;
    for (auto [lnum, i] : lNum2Int) {
      lNum2Int[lnum] = ++curOrder;
    }
    if (lNum2Int.begin()->first + count - 1 != (--lNum2Int.end())->first) {
      logger->error(utl::DRT,
                    130,
                    "non consecutive layers for viarule {}",
                    rule->getName());
    }
    auto viaRuleGen = make_unique<frViaRuleGenerate>(rule->getName());
    if (rule->isDefault()) {
      viaRuleGen->setDefault(1);
    }
    for (int i = 0; i < count; i++) {
      auto layerRule = rule->getViaLayerRule(i);
      frLayerNum layerNum
          = tech->name2layer[layerRule->getLayer()->getName()]->getLayerNum();
      if (layerRule->hasEnclosure()) {
        frCoord x;
        frCoord y;
        layerRule->getEnclosure(x, y);
        frPoint enc(x, y);
        switch (lNum2Int[layerNum]) {
          case 1:
            viaRuleGen->setLayer1Enc(enc);
            break;
          case 2:
            logger->warn(utl::DRT,
                         131,
                         "cutLayer cannot have overhangs in viarule {}, "
                         "skipping enclosure",
                         rule->getName());
            break;
          default:
            viaRuleGen->setLayer2Enc(enc);
            break;
        }
      }
      if (layerRule->hasRect()) {
        odb::Rect rect;
        layerRule->getRect(rect);
        frCoord xl = rect.xMin();
        frCoord yl = rect.yMin();
        frCoord xh = rect.xMax();
        frCoord yh = rect.yMax();
        frBox box(xl, yl, xh, yh);
        switch (lNum2Int[layerNum]) {
          case 1:
            logger->warn(
                utl::DRT,
                132,
                "botLayer cannot have rect in viarule {}, skipping rect",
                rule->getName());
            break;
          case 2:
            viaRuleGen->setCutRect(box);
            break;
          default:
            logger->warn(
                utl::DRT,
                133,
                "topLayer cannot have rect in viarule {}, skipping rect",
                rule->getName());
            break;
        }
      }
      if (layerRule->hasSpacing()) {
        frCoord x;
        frCoord y;
        layerRule->getSpacing(x, y);
        frPoint pt(x, y);
        switch (lNum2Int[layerNum]) {
          case 1:
            logger->warn(
                utl::DRT,
                134,
                "botLayer cannot have spacing in viarule {}, skipping spacing",
                rule->getName());
            break;
          case 2:
            viaRuleGen->setCutSpacing(pt);
            break;
          default:
            logger->warn(
                utl::DRT,
                135,
                "botLayer cannot have spacing in viarule {}, skipping spacing",
                rule->getName());
            break;
        }
      }
    }
    tech->addViaRuleGenerate(std::move(viaRuleGen));
  }
}

void io::Parser::setTechVias(odb::dbTech* _tech)
{
  for (auto via : _tech->getVias()) {
    map<frLayerNum, int> lNum2Int;
    for (auto box : via->getBoxes()) {
      string layerName = box->getTechLayer()->getName();
      if (tech->name2layer.find(layerName) == tech->name2layer.end())
        logger->error(utl::DRT,
                      124,
                      "unknown layer {} for via {}",
                      layerName,
                      via->getName());
      frLayerNum lNum = tech->name2layer[layerName]->getLayerNum();
      lNum2Int[lNum] = 1;
    }
    if (lNum2Int.size() != 3)
      logger->error(utl::DRT, 125, "unsupported via {}", via->getName());
    int curOrder = 0;
    for (auto [lnum, i] : lNum2Int) {
      lNum2Int[lnum] = ++curOrder;
    }

    if (lNum2Int.begin()->first + 2 != (--lNum2Int.end())->first) {
      logger->error(
          utl::DRT, 126, "non consecutive layers for via {}", via->getName());
    }
    auto viaDef = make_unique<frViaDef>(via->getName());
    if (via->isDefault())
      viaDef->setDefault(true);
    for (auto box : via->getBoxes()) {
      frLayerNum layerNum;
      string layer = box->getTechLayer()->getName();
      if (tech->name2layer.find(layer) == tech->name2layer.end())
        logger->error(utl::DRT,
                      127,
                      "unknown layer {} for via {}",
                      layer,
                      via->getName());
      else
        layerNum = tech->name2layer.at(layer)->getLayerNum();
      frCoord xl = box->xMin();
      frCoord yl = box->yMin();
      frCoord xh = box->xMax();
      frCoord yh = box->yMax();
      unique_ptr<frRect> pinFig = make_unique<frRect>();
      pinFig->setBBox(frBox(xl, yl, xh, yh));
      pinFig->setLayerNum(layerNum);
      if (lNum2Int[layerNum] == 1) {
        viaDef->addLayer1Fig(std::move(pinFig));
      } else if (lNum2Int[layerNum] == 3) {
        viaDef->addLayer2Fig(std::move(pinFig));
      } else if (lNum2Int[layerNum] == 2) {
        viaDef->addCutFig(std::move(pinFig));
      }
    }
    auto cutLayerNum = viaDef->getCutLayerNum();
    auto cutLayer = tech->getLayer(cutLayerNum);
    int cutClassIdx = -1;
    frLef58CutClass* cutClass = nullptr;

    for (auto& cutFig : viaDef->getCutFigs()) {
      frBox box;
      cutFig->getBBox(box);
      auto width = box.width();
      auto length = box.length();
      cutClassIdx = cutLayer->getCutClassIdx(width, length);
      if (cutClassIdx != -1) {
        cutClass = cutLayer->getCutClass(cutClassIdx);
        break;
      }
    }
    if (cutClass) {
      viaDef->setCutClass(cutClass);
      viaDef->setCutClassIdx(cutClassIdx);
    }
    tech->addVia(std::move(viaDef));
  }
}

void io::Parser::readTechAndLibs(odb::dbDatabase* db)
{
  auto _tech = db->getTech();
  if (_tech == nullptr)
    logger->error(utl::DRT, 136, "load design first");
  tech->setDBUPerUU(_tech->getDbUnitsPerMicron());
  USEMINSPACING_OBS = _tech->getUseMinSpacingObs() == odb::dbOnOffType::ON;
  tech->setManufacturingGrid(frUInt4(_tech->getManufacturingGrid()));
  setLayers(_tech);
  setTechVias(db->getTech());
  setTechViaRules(db->getTech());
  setMacros(db);
}

void io::Parser::readDb(odb::dbDatabase* db)
{
  if (VERBOSE > 0) {
    logger->info(utl::DRT, 149, "Reading Tech And Libs");
  }
  readTechAndLibs(db);
  if (VERBOSE > 0) {
    logger->report("");
    logger->report("units:       {}", tech->getDBUPerUU());
    logger->report("#layers:     {}", tech->layers.size());
    logger->report("#macros:     {}", design->refBlocks_.size());
    logger->report("#vias:       {}", tech->vias.size());
    logger->report("#viarulegen: {}", tech->viaRuleGenerates.size());
    logger->report("");
  }

  auto numLefVia = tech->vias.size();

  if (VERBOSE > 0) {
    logger->info(utl::DRT, 150, "Reading Design");
  }

  readDesign(db);

  if (VERBOSE > 0) {
    logger->report("");
    frBox dieBox;
    design->getTopBlock()->getBoundaryBBox(dieBox);
    logger->report("design:      {}", design->getTopBlock()->getName());
    logger->report("die area:    {}", dieBox);
    logger->report("trackPts:    {}",
                   design->getTopBlock()->getTrackPatterns().size());
    logger->report("defvias:     {}", tech->vias.size() - numLefVia);
    logger->report("#components: {}", design->getTopBlock()->insts_.size());
    logger->report("#terminals:  {}", design->getTopBlock()->terms_.size());
    logger->report("#snets:      {}", design->getTopBlock()->snets_.size());
    logger->report("#nets:       {}", design->getTopBlock()->nets_.size());
    logger->report("");
  }
}

void io::Parser::readGuide() {
  ProfileTask profile("IO:readGuide");

  if (VERBOSE > 0) {
    logger->info(utl::DRT, 151, "Reading Guide");
  }

  int numGuides = 0;

  string netName = "";
  frNet* net = nullptr;

  ifstream fin(GUIDE_FILE.c_str());
  string line;
  frBox  box;
  frLayerNum layerNum;

  if (fin.is_open()){
    while (fin.good()) {
      getline(fin, line);
      //cout <<line <<endl <<line.size() <<endl;
      if (line == "(" || line == "") continue;
      if (line == ")") {
        continue;
      }

      stringstream ss(line);
      string word = "";
      vector<string> vLine;
      while (!ss.eof()) {
        ss >>word;
        vLine.push_back(word);
        //cout <<word <<" ";
      }
      //cout <<endl;

      if (vLine.size() == 0) {
        logger->error(utl::DRT, 152, "Error reading guide file!");
      } else if (vLine.size() == 1) {
        netName = vLine[0];
        if (design->topBlock_->name2net_.find(vLine[0]) == design->topBlock_->name2net_.end()) {
          logger->error(utl::DRT, 153, "cannot find net {}",vLine[0]);
        }
        net = design->topBlock_->name2net_[netName]; 
      } else if (vLine.size() == 5) {
        if (tech->name2layer.find(vLine[4]) == tech->name2layer.end()) {
          logger->error(utl::DRT, 154, "cannot find layer {}",vLine[4]);
        }
        layerNum = tech->name2layer[vLine[4]]->getLayerNum();

        if (layerNum < (BOTTOM_ROUTING_LAYER && layerNum != VIA_ACCESS_LAYERNUM)
            || layerNum > TOP_ROUTING_LAYER)
            logger->error(utl::DRT,
                        155,
                        "guide in net {} uses layer {} ({})"
                        " that is outside the allowed routing range "
                        "[{} ({}), ({})]",
                        netName,
                        vLine[4],
                        layerNum,
                        tech->getLayer(BOTTOM_ROUTING_LAYER)->getName(),
                        BOTTOM_ROUTING_LAYER,
                        tech->getLayer(TOP_ROUTING_LAYER)->getName(),
                        TOP_ROUTING_LAYER);

        box.set(stoi(vLine[0]), stoi(vLine[1]), stoi(vLine[2]), stoi(vLine[3]));
        frRect rect;
        rect.setBBox(box);
        rect.setLayerNum(layerNum);
        tmpGuides[net].push_back(rect);
        ++numGuides;
        if (numGuides < 1000000) {
          if (numGuides % 100000 == 0) {
            logger->info(utl::DRT, 156, "guideIn read {} guides", numGuides);
          }
        } else {
          if (numGuides % 1000000 == 0) {
            logger->info(utl::DRT, 157, "guideIn read {} guides", numGuides);
          }
        }

      } else {
        logger->error(utl::DRT, 158, "Error reading guide file!");
      }
    }
    fin.close();
  } else {
    logger->error(utl::DRT, 159, "failed to open guide file");
  }


  if (VERBOSE > 0) {
    logger->report("");
    logger->report("#guides:     {}", numGuides);
    logger->report("");
  }

}

void io::Writer::fillConnFigs_net(frNet* net, bool isTA) {
  //bool enableOutput = true;
  bool enableOutput = false;
  auto netName = net->getName();
  if (isTA) {
    for (auto &uGuide: net->getGuides()) {
      //cout <<"find guide" <<endl;
      for (auto &uConnFig: uGuide->getRoutes()) {
        auto connFig = uConnFig.get();
        if (connFig->typeId() == frcPathSeg) {
          connFigs[netName].push_back(make_shared<frPathSeg>(*static_cast<frPathSeg*>(connFig)));
        } else if (connFig->typeId() == frcVia) {
          connFigs[netName].push_back(make_shared<frVia>(*static_cast<frVia*>(connFig)));
        } else {
          cout <<"Error: io::Writer::filliConnFigs does not support this type" <<endl;
        }
      }
    }
  } else {
    if (enableOutput) {
      cout << netName << ":\n";
    }
    for (auto &shape: net->getShapes()) {
      if (shape->typeId() == frcPathSeg) {
        auto pathSeg = *static_cast<frPathSeg*>(shape.get());
        frPoint start, end;
        pathSeg.getPoints(start, end);

        if (enableOutput) {
          frLayerNum currLayerNum = pathSeg.getLayerNum();
          cout << "  connfig pathseg (" << start.x() / 2000.0<< ", " << start.y() / 2000.0 
               << ") - (" << end.x() / 2000.0 << ", " << end.y() / 2000.0 << ") " << currLayerNum  <<"\n"; 
        }
        connFigs[netName].push_back(make_shared<frPathSeg>(pathSeg));
      }
    }
    for (auto &via: net->getVias()) {
      connFigs[netName].push_back(make_shared<frVia>(*via));
    }
    for (auto &shape: net->getPatchWires()) {
      auto pwire = static_cast<frPatchWire*>(shape.get());
      connFigs[netName].push_back(make_shared<frPatchWire>(*pwire));
    }
  }
}

void io::Writer::splitVia_helper(frLayerNum layerNum, int isH, frCoord trackLoc, frCoord x, frCoord y, 
  vector< vector< map<frCoord, vector<shared_ptr<frPathSeg> > > > > &mergedPathSegs) {
  if (layerNum >= 0 && layerNum < (int)(getTech()->getLayers().size()) &&
      mergedPathSegs.at(layerNum).at(isH).find(trackLoc) != mergedPathSegs.at(layerNum).at(isH).end()) {
    for (auto &pathSeg: mergedPathSegs.at(layerNum).at(isH).at(trackLoc)) {
      frPoint begin, end;
      pathSeg->getPoints(begin, end);
      if ((isH == 0 && (begin.x() < x) && (end.x() > x)) ||
          (isH == 1 && (begin.y() < y) && (end.y() > y))) {
        frSegStyle style1, style2, style_default;
        pathSeg->getStyle(style1);
        pathSeg->getStyle(style2);
        style_default = getTech()->getLayer(layerNum)->getDefaultSegStyle();
        shared_ptr<frPathSeg> newPathSeg = make_shared<frPathSeg>(*pathSeg);
        pathSeg->setPoints(begin, frPoint(x,y));
        style1.setEndStyle(style_default.getEndStyle(), style_default.getEndExt());
        pathSeg->setStyle(style1);
        newPathSeg->setPoints(frPoint(x,y), end);
        style2.setBeginStyle(style_default.getBeginStyle(), style_default.getBeginExt());
        newPathSeg->setStyle(style2);
        mergedPathSegs.at(layerNum).at(isH).at(trackLoc).push_back(newPathSeg);
        // via can only intersect at most one merged pathseg on one track
        break;
      }
    }
  }
}

// merge pathseg, delete redundant via
void io::Writer::mergeSplitConnFigs(list<shared_ptr<frConnFig> > &connFigs) {
  //if (VERBOSE > 0) {
  //  cout <<endl <<"merge and split ..." <<endl;
  //}
  // initialzie pathseg and via map
  map < tuple<frLayerNum, bool, frCoord>,
        map<frCoord, vector< tuple<shared_ptr<frPathSeg>, bool> > 
           >
      > pathSegMergeMap;
  map < tuple<frCoord, frCoord, frLayerNum>, shared_ptr<frVia> > viaMergeMap;
  for (auto &connFig: connFigs) {
    if (connFig->typeId() == frcPathSeg) {
      auto pathSeg = dynamic_pointer_cast<frPathSeg>(connFig);
      frPoint begin, end;
      pathSeg->getPoints(begin, end);
      frLayerNum layerNum = pathSeg->getLayerNum();
      if (begin == end) {
        // std::cout << "Warning: 0 length connfig\n";
        continue; // if segment length = 0, ignore
      } else {
        // std::cout << "xxx\n";
        bool isH = (begin.x() == end.x()) ? false : true;
        frCoord trackLoc   = isH ? begin.y() : begin.x();
        frCoord beginCoord = isH ? begin.x() : begin.y();
        frCoord endCoord   = isH ? end.x()   : end.y();
        pathSegMergeMap[make_tuple(layerNum, isH, trackLoc)][beginCoord].push_back(make_tuple(pathSeg, true));
        pathSegMergeMap[make_tuple(layerNum, isH, trackLoc)][endCoord].push_back(make_tuple(pathSeg, false));
      }
    } else if (connFig->typeId() == frcVia) {
      auto via = dynamic_pointer_cast<frVia>(connFig);
      auto cutLayerNum = via->getViaDef()->getCutLayerNum();
      frPoint viaPoint;
      via->getOrigin(viaPoint);
      viaMergeMap[make_tuple(viaPoint.x(), viaPoint.y(), cutLayerNum)] = via;
      //cout <<"found via" <<endl;
    }
  }

  // merge pathSeg
  map<frCoord, vector<shared_ptr<frPathSeg> > > tmp1;
  vector< map<frCoord, vector<shared_ptr<frPathSeg> > > > tmp2(2, tmp1);
  vector< vector< map<frCoord, vector<shared_ptr<frPathSeg> > > > > mergedPathSegs(getTech()->getLayers().size(), tmp2);

  for (auto &it1: pathSegMergeMap) {
    auto layerNum = get<0>(it1.first);
    int  isH = get<1>(it1.first);
    auto trackLoc = get<2>(it1.first);
    bool hasSeg = false;
    int cnt = 0;
    shared_ptr<frPathSeg> newPathSeg;
    frSegStyle style;
    frPoint begin, end;
    for (auto &it2: it1.second) {
      //cout <<"coord " <<coord <<endl;
      for (auto &pathSegTuple: it2.second) {
        cnt += get<1>(pathSegTuple)? 1 : -1;
      }
      // newPathSeg begin
      if (!hasSeg && cnt > 0) {
        style.setBeginStyle(frcTruncateEndStyle, 0);
        style.setEndStyle(frcTruncateEndStyle, 0);
        newPathSeg = make_shared<frPathSeg>(*(get<0>(*(it2.second.begin()))));
        for (auto &pathSegTuple: it2.second) {
          auto pathSeg = get<0>(pathSegTuple);
          auto isBegin = get<1>(pathSegTuple);
          if (isBegin) {
            pathSeg->getPoints(begin, end);
            frSegStyle tmpStyle;
            pathSeg->getStyle(tmpStyle);
            if (tmpStyle.getBeginExt() > style.getBeginExt()) {
              style.setBeginStyle(tmpStyle.getBeginStyle(), tmpStyle.getBeginExt());
            }
          }
        }
        newPathSeg->setStyle(style);
        hasSeg = true;
      // newPathSeg end
      } else if (hasSeg && cnt == 0) {
        newPathSeg->getPoints(begin, end);
        for (auto &pathSegTuple: it2.second) {
          auto pathSeg = get<0>(pathSegTuple);
          auto isBegin = get<1>(pathSegTuple);
          if (!isBegin) {
            frPoint tmp;
            pathSeg->getPoints(tmp, end);
            frSegStyle tmpStyle;
            pathSeg->getStyle(tmpStyle);
            if (tmpStyle.getEndExt() > style.getEndExt()) {
              style.setEndStyle(tmpStyle.getEndStyle(), tmpStyle.getEndExt());
            }
          }
        }
        newPathSeg->setPoints(begin, end);
        newPathSeg->setStyle(style);
        hasSeg = false;
        (mergedPathSegs.at(layerNum).at(isH))[trackLoc].push_back(newPathSeg);
      }
    }
  }

  // split pathseg from via
  // mergedPathSegs[layerNum][isHorizontal] is a map<frCoord, vector<shared_ptr<frPathSeg> > >
  //map < tuple<frCoord, frCoord, frLayerNum>, shared_ptr<frVia> > viaMergeMap;
  for (auto &it1: viaMergeMap) {
    auto x           = get<0>(it1.first);
    auto y           = get<1>(it1.first);
    auto cutLayerNum = get<2>(it1.first);
    frCoord trackLoc;

    auto layerNum = cutLayerNum - 1;
    int  isH      = 1;
    trackLoc = (isH == 1) ? y : x;
    splitVia_helper(layerNum, isH, trackLoc, x, y, mergedPathSegs);

    layerNum = cutLayerNum - 1;
    isH = 0;
    trackLoc = (isH == 1) ? y : x;
    splitVia_helper(layerNum, isH, trackLoc, x, y, mergedPathSegs);

    layerNum = cutLayerNum + 1;
    trackLoc = (isH == 1) ? y : x;
    splitVia_helper(layerNum, isH, trackLoc, x, y, mergedPathSegs);

    layerNum = cutLayerNum + 1;
    isH = 0;
    trackLoc = (isH == 1) ? y : x;
    splitVia_helper(layerNum, isH, trackLoc, x, y, mergedPathSegs);
  }

  // split intersecting pathSegs
  for (auto &it1: mergedPathSegs) {
    // vertical for mapIt1
    for (auto &mapIt1: it1.at(0)) {
      // horizontal for mapIt2
      for (auto &mapIt2: it1.at(1)) {
        // at most split once
        // seg1 is vertical
        for (auto &seg1: mapIt1.second) {
          bool skip = false;
          // seg2 is horizontal
          frPoint seg1Begin, seg1End;
          seg1->getPoints(seg1Begin, seg1End);
          for (auto &seg2: mapIt2.second) {
            frPoint seg2Begin, seg2End;
            seg2->getPoints(seg2Begin, seg2End);
            bool pushNewSeg1 = false;
            bool pushNewSeg2 = false;
            shared_ptr<frPathSeg> newSeg1;
            shared_ptr<frPathSeg> newSeg2;
            // check whether seg1 needs to be split, break seg1
            if (seg2Begin.y() > seg1Begin.y() && seg2Begin.y() < seg1End.y()) {
              pushNewSeg1 = true;
              newSeg1 = make_shared<frPathSeg>(*seg1);
              // modify seg1
              seg1->setPoints(seg1Begin, frPoint(seg1End.x(), seg2End.y()));
              // modify newSeg1
              newSeg1->setPoints(frPoint(seg1End.x(), seg2Begin.y()), seg1End);
              // modify endstyle
              auto layerNum = seg1->getLayerNum();
              frSegStyle tmpStyle1;
              frSegStyle tmpStyle2;
              frSegStyle style_default;
              seg1->getStyle(tmpStyle1);
              seg1->getStyle(tmpStyle2);
              style_default = getTech()->getLayer(layerNum)->getDefaultSegStyle();
              tmpStyle1.setEndStyle(frcExtendEndStyle, style_default.getEndExt());
              seg1->setStyle(tmpStyle1);
              tmpStyle2.setBeginStyle(frcExtendEndStyle, style_default.getBeginExt());
              newSeg1->setStyle(tmpStyle2);
            }
            // check whether seg2 needs to be split, break seg2
            if (seg1Begin.x() > seg2Begin.x() && seg1Begin.x() < seg2End.x()) {
              pushNewSeg2 = true;
              newSeg2 = make_shared<frPathSeg>(*seg1);
              // modify seg2
              seg2->setPoints(seg2Begin, frPoint(seg1End.x(), seg2End.y()));
              // modify newSeg2
              newSeg2->setPoints(frPoint(seg1End.x(), seg2Begin.y()), seg2End);
              // modify endstyle
              auto layerNum = seg2->getLayerNum();
              frSegStyle tmpStyle1;
              frSegStyle tmpStyle2;
              frSegStyle style_default;
              seg2->getStyle(tmpStyle1);
              seg2->getStyle(tmpStyle2);
              style_default = getTech()->getLayer(layerNum)->getDefaultSegStyle();
              tmpStyle1.setEndStyle(frcExtendEndStyle, style_default.getEndExt());
              seg2->setStyle(tmpStyle1);
              tmpStyle2.setBeginStyle(frcExtendEndStyle, style_default.getBeginExt());
              newSeg2->setStyle(tmpStyle2);
            }
            if (pushNewSeg1) {
              mapIt1.second.push_back(newSeg1);
            }
            if (pushNewSeg2) {
              mapIt2.second.push_back(newSeg2);
            }
            if (pushNewSeg1 || pushNewSeg2) {
              skip = true;
              break;
            }
            //cout <<"found" <<endl;
          }
          if (skip) break;
        }
      }
    }
  }


  // write back pathseg
  connFigs.clear();
  for (auto &it1: mergedPathSegs) {
    for (auto &it2: it1) {
      for (auto &it3: it2) {
        for (auto &it4: it3.second) {
          connFigs.push_back(it4);
        }
      }
    }
  }

  // write back via
  //map < tuple<frCoord, frCoord, frLayerNum>, shared_ptr<frVia> > viaMergeMap;
  for (auto &it: viaMergeMap) {
    connFigs.push_back(it.second);
  }

}

void io::Writer::fillViaDefs() {
  viaDefs.clear();
  for (auto &uViaDef: getDesign()->getTech()->getVias()) {
    auto viaDef = uViaDef.get();
    if (viaDef->isAddedByRouter()) {
      viaDefs.push_back(viaDef);
    }
  }
}

void io::Writer::fillConnFigs(bool isTA) {
  connFigs.clear();
  if (VERBOSE > 0) {
    cout <<endl <<"post processing ..." <<endl;
  }
  for (auto &net: getDesign()->getTopBlock()->getNets()) {
    fillConnFigs_net(net.get(), isTA);
  }
  if (isTA) {
    for (auto &it: connFigs) {
      mergeSplitConnFigs(it.second);
    }
  }
}

void io::Writer::writeFromTA() {
  ProfileTask profile("IO:writeFromTA");
  if (OUTTA_FILE == string("")) {
    if (VERBOSE > 0) {
      cout <<"Waring: no output def specified, skipped writing track assignment def" <<endl;
    }
  } else {
    //if (VERBOSE > 0) {
    //  cout <<endl <<"start writing track assignment def" <<endl;
    //}
    fillConnFigs(true);
    fillViaDefs();
    writeDef(true);
  }
}

void io::Writer::writeFromDR(const string &str) {
  ProfileTask profile("IO:writeFromDR");
  if (OUT_FILE == string("")) {
    if (VERBOSE > 0) {
      cout <<"Waring: no output def specified, skipped writing routed def" <<endl;
    }
  } else {
    //if (VERBOSE > 0) {
    //  cout <<endl <<"start writing routed def" <<endl;
    //}
  }
  fillConnFigs(false);
  fillViaDefs();
  writeDef(false, str);
}

void io::Writer::updateDbVias(odb::dbBlock* block, odb::dbTech* tech)
{
  frBox box;
  for (auto via : viaDefs) {
    if (block->findVia(via->getName().c_str()) != nullptr)
      continue;
    auto layer1Name = getTech()->getLayer(via->getLayer1Num())->getName();
    auto layer2Name = getTech()->getLayer(via->getLayer2Num())->getName();
    auto cutName = getTech()->getLayer(via->getCutLayerNum())->getName();
    odb::dbTechLayer* _layer1 = tech->findLayer(layer1Name.c_str());
    odb::dbTechLayer* _layer2 = tech->findLayer(layer2Name.c_str());
    odb::dbTechLayer* _cut_layer = tech->findLayer(cutName.c_str());
    if (_layer1 == nullptr || _layer2 == nullptr || _cut_layer == nullptr) {
      logger->error(utl::DRT,
                    113,
                    "techlayers for via {} not found in db tech",
                    via->getName());
    }
    odb::dbVia* _db_via = odb::dbVia::create(block, via->getName().c_str());

    for (auto& fig : via->getLayer2Figs()) {
      fig->getBBox(box);
      odb::dbBox::create(
          _db_via, _layer2, box.left(), box.bottom(), box.right(), box.top());
    }
    for (auto& fig : via->getCutFigs()) {
      fig->getBBox(box);
      odb::dbBox::create(_db_via,
                         _cut_layer,
                         box.left(),
                         box.bottom(),
                         box.right(),
                         box.top());
    }

    for (auto& fig : via->getLayer1Figs()) {
      fig->getBBox(box);
      odb::dbBox::create(
          _db_via, _layer1, box.left(), box.bottom(), box.right(), box.top());
    }
  }
}

void io::Writer::updateDbConn(odb::dbBlock* block, odb::dbTech* tech)
{
  odb::dbWireEncoder _wire_encoder;
  for (auto net : block->getNets()) {
    if (connFigs.find(net->getName()) != connFigs.end()) {
      odb::dbWire* wire = net->getWire();
      if (wire == nullptr)
        wire = odb::dbWire::create(net);
      _wire_encoder.begin(wire);
      for (auto& connFig : connFigs.at(net->getName())) {
        switch (connFig->typeId()) {
          case frcPathSeg: {
            auto pathSeg = std::dynamic_pointer_cast<frPathSeg>(connFig);
            auto layerName
                = getTech()->getLayer(pathSeg->getLayerNum())->getName();
            auto layer = tech->findLayer(layerName.c_str());
            _wire_encoder.newPath(layer, odb::dbWireType("ROUTED"));
            frPoint begin, end;
            frSegStyle segStyle;
            pathSeg->getPoints(begin, end);
            pathSeg->getStyle(segStyle);
            if (segStyle.getBeginStyle() == frEndStyle(frcExtendEndStyle)) {
              _wire_encoder.addPoint(begin.x(), begin.y());
            } else if (segStyle.getBeginStyle()
                       == frEndStyle(frcTruncateEndStyle)) {
              _wire_encoder.addPoint(begin.x(), begin.y(), 0);
            } else if (segStyle.getBeginStyle()
                       == frEndStyle(frcVariableEndStyle)) {
              _wire_encoder.addPoint(
                  begin.x(), begin.y(), segStyle.getBeginExt());
            }
            if (segStyle.getEndStyle() == frEndStyle(frcExtendEndStyle)) {
              _wire_encoder.addPoint(end.x(), end.y());
            } else if (segStyle.getEndStyle()
                       == frEndStyle(frcTruncateEndStyle)) {
              _wire_encoder.addPoint(end.x(), end.y(), 0);
            } else if (segStyle.getBeginStyle()
                       == frEndStyle(frcVariableEndStyle)) {
              _wire_encoder.addPoint(end.x(), end.y(), segStyle.getEndExt());
            }
            break;
          }
          case frcVia: {
            auto via = std::dynamic_pointer_cast<frVia>(connFig);
            auto layerName = getTech()
                                 ->getLayer(via->getViaDef()->getLayer1Num())
                                 ->getName();
            auto viaName = via->getViaDef()->getName();
            auto layer = tech->findLayer(layerName.c_str());
            _wire_encoder.newPath(layer, odb::dbWireType("ROUTED"));
            frPoint origin;
            via->getOrigin(origin);
            _wire_encoder.addPoint(origin.x(), origin.y());
            odb::dbTechVia* tech_via = tech->findVia(viaName.c_str());
            if (tech_via != nullptr) {
              _wire_encoder.addTechVia(tech_via);
            } else {
              odb::dbVia* db_via = block->findVia(viaName.c_str());
              _wire_encoder.addVia(db_via);
            }
            break;
          }
          case frcPatchWire: {
            auto pwire = std::dynamic_pointer_cast<frPatchWire>(connFig);
            auto layerName
                = getTech()->getLayer(pwire->getLayerNum())->getName();
            auto layer = tech->findLayer(layerName.c_str());
            _wire_encoder.newPath(layer, odb::dbWireType("ROUTED"));
            frPoint origin;
            frBox offsetBox;
            pwire->getOrigin(origin);
            pwire->getOffsetBox(offsetBox);
            _wire_encoder.addPoint(origin.x(), origin.y());
            _wire_encoder.addRect(offsetBox.left(),
                                  offsetBox.bottom(),
                                  offsetBox.right(),
                                  offsetBox.top());
            break;
          }
          default: {
            _wire_encoder.clear();
            logger->error(utl::DRT,
                          114,
                          "unknown connfig type while writing net {}",
                          net->getName());
          }
        }
      }
      _wire_encoder.end();
    }
  }
}

void io::Writer::updateDb(odb::dbDatabase* db)
{
  if (db->getChip() == nullptr)
    logger->error(utl::DRT, 3, "load design first");

  odb::dbBlock* block = db->getChip()->getBlock();
  odb::dbTech* tech = db->getTech();
  if (block == nullptr || tech == nullptr)
    logger->error(utl::DRT, 4, "load design first");

  updateDbVias(block, tech);
  updateDbConn(block, tech);
}
