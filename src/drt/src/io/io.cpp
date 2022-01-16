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

#include "io/io.h"

#include <exception>
#include <fstream>
#include <iostream>
#include <sstream>

#include "db/tech/frConstraint.h"
#include "frProfileTask.h"
#include "global.h"
#include "odb/db.h"
#include "odb/dbWireCodec.h"
#include "utl/Logger.h"

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
  vector<Point> points;
  odb::Rect box;
  block->getDieArea(box);
  points.push_back(
      Point(defdist(block, box.xMin()), defdist(block, box.yMin())));
  points.push_back(
      Point(defdist(block, box.xMax()), defdist(block, box.yMax())));
  points.push_back(
      Point(defdist(block, box.xMax()), defdist(block, box.yMin())));
  points.push_back(
      Point(defdist(block, box.xMin()), defdist(block, box.yMax())));
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
      logger->error(
          DRT, 94, "Cannot find layer: {}.", track->getTechLayer()->getName());
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

void io::Parser::setInsts(odb::dbBlock* block)
{
  for (auto inst : block->getInsts()) {
    if (design->name2master_.find(inst->getMaster()->getName())
        == design->name2master_.end())
      logger->error(
          DRT, 95, "Library cell {} not found.", inst->getMaster()->getName());
    if (tmpBlock->name2inst_.find(inst->getName())
        != tmpBlock->name2inst_.end())
      logger->error(DRT, 96, "Same cell name: {}.", inst->getName());
    frMaster* master = design->name2master_.at(inst->getMaster()->getName());
    auto uInst = make_unique<frInst>(inst->getName(), master);
    auto tmpInst = uInst.get();
    tmpInst->setId(numInsts);
    numInsts++;

    int x, y;
    inst->getLocation(x, y);
    x = defdist(block, x);
    y = defdist(block, y);
    tmpInst->setOrigin(Point(x, y));
    tmpInst->setOrient(inst->getOrient());
    for (auto& uTerm : tmpInst->getMaster()->getTerms()) {
      auto term = uTerm.get();
      unique_ptr<frInstTerm> instTerm = make_unique<frInstTerm>(tmpInst, term);
      instTerm->setId(numTerms);
      numTerms++;
      int pinCnt = term->getPins().size();
      instTerm->setAPSize(pinCnt);
      tmpInst->addInstTerm(std::move(instTerm));
    }
    for (auto& uBlk : tmpInst->getMaster()->getBlockages()) {
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
    if (tech->name2layer.find(layerName) == tech->name2layer.end()) {
      logger->warn(
          DRT, 282, "Skipping blockage. Cannot find layer {}.", layerName);
      continue;
    }
    frLayerNum layerNum = tech->name2layer[layerName]->getLayerNum();
    auto blkIn = make_unique<frBlockage>();
    blkIn->setId(numBlockages);
    numBlockages++;
    auto pinIn = make_unique<frBPin>();
    pinIn->setId(0);
    frCoord xl = blockage->getBBox()->xMin();
    frCoord yl = blockage->getBBox()->yMin();
    frCoord xh = blockage->getBBox()->xMax();
    frCoord yh = blockage->getBBox()->yMax();
    // pinFig
    unique_ptr<frRect> pinFig = make_unique<frRect>();
    pinFig->setBBox(Rect(xl, yl, xh, yh));
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
        logger->error(DRT,
                      97,
                      "Cannot find cut layer {}.",
                      params.getCutLayer()->getName());
      else
        cutLayerNum = tech->name2layer.find(params.getCutLayer()->getName())
                          ->second->getLayerNum();

      if (tech->name2layer.find(params.getBottomLayer()->getName())
          == tech->name2layer.end())
        logger->error(DRT,
                      98,
                      "Cannot find bottom layer {}.",
                      params.getBottomLayer()->getName());
      else
        botLayerNum = tech->name2layer.find(params.getBottomLayer()->getName())
                          ->second->getLayerNum();

      if (tech->name2layer.find(params.getTopLayer()->getName())
          == tech->name2layer.end())
        logger->error(DRT,
                      99,
                      "Cannot find top layer {}.",
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
      vector<unique_ptr<frShape>> cutFigs;
      for (int i = 0; i < params.getNumCutRows(); i++) {
        currX = 0;
        for (int j = 0; j < params.getNumCutCols(); j++) {
          auto rect = make_unique<frRect>();
          Rect tmpBox(currX, currY, currX + xSize, currY + ySize);
          rect->setBBox(tmpBox);
          rect->setLayerNum(cutLayerNum);
          cutFigs.push_back(std::move(rect));
          currX += xSize + xCutSpacing;
        }
        currY += ySize + yCutSpacing;
      }
      currX -= xCutSpacing;  // max cut X
      currY -= yCutSpacing;  // max cut Y
      dbTransform cutXform(Point(-currX / 2 + xOffset, -currY / 2 + yOffset));
      for (auto& uShape : cutFigs) {
        auto rect = static_cast<frRect*>(uShape.get());
        rect->move(cutXform);
      }
      unique_ptr<frShape> uBotFig = make_unique<frRect>();
      auto botFig = static_cast<frRect*>(uBotFig.get());
      unique_ptr<frShape> uTopFig = make_unique<frRect>();
      auto topFig = static_cast<frRect*>(uTopFig.get());

      Rect botBox(0 - xBotEnc, 0 - yBotEnc, currX + xBotEnc, currY + yBotEnc);
      Rect topBox(0 - xTopEnc, 0 - yTopEnc, currX + xTopEnc, currY + yTopEnc);

      dbTransform botXform(Point(-currX / 2 + xOffset + xBotOffset,
                                 -currY / 2 + yOffset + yBotOffset));
      dbTransform topXform(Point(-currX / 2 + xOffset + xTopOffset,
                                 -currY / 2 + yOffset + yTopOffset));
      botXform.apply(botBox);
      topXform.apply(topBox);

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
      map<frLayerNum, set<odb::dbBox*>> lNum2Int;
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
        logger->error(DRT, 100, "Unsupported via: {}.", via->getName());
      if (lNum2Int.begin()->first + 2 != (--lNum2Int.end())->first)
        logger->error(
            DRT, 101, "Non-consecutive layers for via: {}.", via->getName());
      auto viaDef = make_unique<frViaDef>(via->getName());
      int cnt = 0;
      for (auto& [layerNum, boxes] : lNum2Int) {
        for (auto box : boxes) {
          unique_ptr<frRect> pinFig = make_unique<frRect>();
          pinFig->setBBox(Rect(defdist(block, box->xMin()),
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

void io::Parser::createNDR(odb::dbTechNonDefaultRule* ndr)
{
  if (design->tech_->getNondefaultRule(ndr->getName())) {
    logger->warn(DRT,
                 256,
                 "Skipping NDR {} because another rule with the same name "
                 "already exists.",
                 ndr->getName());
    return;
  }
  frNonDefaultRule* fnd;
  unique_ptr<frNonDefaultRule> ptnd;
  int z;
  ptnd = make_unique<frNonDefaultRule>();
  fnd = ptnd.get();
  design->tech_->addNDR(std::move(ptnd));
  fnd->setName(ndr->getName().data());
  fnd->setHardSpacing(ndr->getHardSpacing());
  vector<odb::dbTechLayerRule*> lr;
  ndr->getLayerRules(lr);
  for (auto& l : lr) {
    z = design->tech_->getLayer(l->getLayer()->getName())->getLayerNum() / 2
        - 1;
    fnd->setWidth(l->getWidth(), z);
    fnd->setSpacing(l->getSpacing(), z);
    fnd->setWireExtension(l->getWireExtension(), z);
  }
  vector<odb::dbTechVia*> vias;
  ndr->getUseVias(vias);
  for (auto via : vias) {
    fnd->addVia(design->getTech()->getVia(via->getName()),
                via->getBottomLayer()->getNumber() / 2);
  }
  vector<odb::dbTechViaGenerateRule*> viaRules;
  ndr->getUseViaRules(viaRules);
  z = std::numeric_limits<int>().max();
  for (auto via : viaRules) {
    for (int i = 0; i < (int) via->getViaLayerRuleCount(); i++) {
      if (via->getViaLayerRule(i)->getLayer()->getType()
          == odb::dbTechLayerType::CUT)
        continue;
      if (via->getViaLayerRule(i)->getLayer()->getNumber() / 2 < z)
        z = via->getViaLayerRule(i)->getLayer()->getNumber() / 2;
    }
    fnd->addViaRule(design->getTech()->getViaRule(via->getName()), z);
  }
}
void io::Parser::setNDRs(odb::dbDatabase* db)
{
  for (auto ndr : db->getTech()->getNonDefaultRules()) {
    createNDR(ndr);
  }
  for (auto ndr : db->getChip()->getBlock()->getNonDefaultRules()) {
    createNDR(ndr);
  }
  for (auto& layer : design->getTech()->getLayers()) {
    if (layer->getType() != dbTechLayerType::ROUTING)
      continue;
    MTSAFEDIST = max(MTSAFEDIST,
                     design->getTech()->getMaxNondefaultSpacing(
                         layer->getLayerNum() / 2 - 1));
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
        logger->error(DRT, 102, "Odd dimension in both directions.");
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
      logger->error(DRT, 103, "Unknown direction.");
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
    bool is_special = net->isSpecial();
    unique_ptr<frNet> uNetIn = make_unique<frNet>(net->getName());
    auto netIn = uNetIn.get();
    if (net->getNonDefaultRule())
      uNetIn->updateNondefaultRule(design->getTech()->getNondefaultRule(
          net->getNonDefaultRule()->getName()));
    if (net->getSigType() == dbSigType::CLOCK)
      uNetIn->updateIsClock(true);
    if (is_special)
      uNetIn->setIsSpecial(true);
    netIn->setId(numNets);
    numNets++;
    for (auto term : net->getBTerms()) {
      if (tmpBlock->name2term_.find(term->getName())
          == tmpBlock->name2term_.end())
        logger->error(DRT, 104, "Terminal {} not found.", term->getName());
      auto frbterm = tmpBlock->name2term_[term->getName()];  // frBTerm*
      frbterm->addToNet(netIn);
      netIn->addBTerm(frbterm);
      if (!is_special) {
        // graph enablement
        auto termNode = make_unique<frNode>();
        termNode->setPin(frbterm);
        termNode->setType(frNodeTypeEnum::frcPin);
        netIn->addNode(termNode);
      }
    }
    for (auto term : net->getITerms()) {
      if (tmpBlock->name2inst_.find(term->getInst()->getName())
          == tmpBlock->name2inst_.end())
        logger->error(
            DRT, 105, "Component {} not found.", term->getInst()->getName());
      auto inst = tmpBlock->name2inst_[term->getInst()->getName()];
      // gettin inst term
      auto frterm = inst->getMaster()->getTerm(term->getMTerm()->getName());
      if (frterm == nullptr)
        logger->error(DRT,
                      106,
                      "Component pin {}/{} not found.",
                      term->getInst()->getName(),
                      term->getMTerm()->getName());
      int idx = frterm->getOrderId();
      auto& instTerms = inst->getInstTerms();
      auto instTerm = instTerms[idx].get();
      assert(instTerm->getTerm()->getName() == term->getMTerm()->getName());

      instTerm->addToNet(netIn);
      netIn->addInstTerm(instTerm);
      if (!is_special) {
        // graph enablement
        auto instTermNode = make_unique<frNode>();
        instTermNode->setPin(instTerm);
        instTermNode->setType(frNodeTypeEnum::frcPin);
        netIn->addNode(instTermNode);
      }
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
                logger->error(DRT, 107, "Unsupported layer {}.", layerName);
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
            tmpP->setPoints(Point(endX, endY), Point(beginX, beginY));
            swap(beginExt, endExt);
          } else {
            tmpP->setPoints(Point(beginX, beginY), Point(endX, endY));
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
            logger->error(DRT, 108, "Unsupported via in db.");
          } else {
            Point p;
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
            tmpP->setPoints(Point(beginX, beginY), Point(endX, endY));
            tmpP->addToNet(netIn);
            tmpP->setLayerNum(layerNum);
            width = (width) ? width : tech->name2layer[layerName]->getWidth();
            auto defaultExt = width / 2;

            frEndStyleEnum tmpBeginEnum;
            if (box->getWireShapeType() == odb::dbWireShapeType::NONE) {
              tmpBeginEnum = frcExtendEndStyle;
            } else {
              tmpBeginEnum = frcTruncateEndStyle;
            }
            frEndStyle tmpBeginStyle(tmpBeginEnum);
            frEndStyleEnum tmpEndEnum;
            if (box->getWireShapeType() == odb::dbWireShapeType::NONE) {
              tmpEndEnum = frcExtendEndStyle;
            } else {
              tmpEndEnum = frcTruncateEndStyle;
            }
            frEndStyle tmpEndStyle(tmpEndEnum);

            frSegStyle tmpSegStyle;
            tmpSegStyle.setWidth(width);
            tmpSegStyle.setBeginStyle(
                tmpBeginStyle,
                tmpBeginEnum == frcExtendEndStyle ? defaultExt : 0);
            tmpSegStyle.setEndStyle(
                tmpEndStyle, tmpEndEnum == frcExtendEndStyle ? defaultExt : 0);
            tmpP->setStyle(tmpSegStyle);
            netIn->addShape(std::move(tmpP));
          } else {
            if (box->getTechVia())
              viaName = box->getTechVia()->getName();
            else if (box->getBlockVia())
              viaName = box->getBlockVia()->getName();

            if (tech->name2via.find(viaName) == tech->name2via.end())
              logger->error(DRT, 109, "Unsupported via in db.");
            else {
              int x, y;
              box->getViaXY(x, y);
              Point p(defdist(block, x), defdist(block, y));
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
    netIn->setType(net->getSigType());
    if (is_special)
      tmpBlock->addSNet(std::move(uNetIn));
    else
      tmpBlock->addNet(std::move(uNetIn));
  }
}

void io::Parser::setBTerms(odb::dbBlock* block)
{
  for (auto term : block->getBTerms()) {
    switch (term->getSigType()) {
      case odb::dbSigType::POWER:
      case odb::dbSigType::GROUND:
      case odb::dbSigType::TIEOFF:
        // We allow for multuple pins
        break;
      case odb::dbSigType::SIGNAL:
      case odb::dbSigType::CLOCK:
      case odb::dbSigType::ANALOG:
      case odb::dbSigType::RESET:
      case odb::dbSigType::SCAN:
        if (term->getBPins().size() > 1)
          logger->error(utl::DRT,
                        302,
                        "Unsupported multiple pins on bterm {}",
                        term->getName());
        break;
    }
    auto uTermIn = make_unique<frBTerm>(term->getName());
    auto termIn = uTermIn.get();
    termIn->setId(numTerms);
    numTerms++;
    termIn->setType(term->getSigType());
    termIn->setDirection(term->getIoType());
    auto pinIn = make_unique<frBPin>();
    pinIn->setId(0);
    for (auto pin : term->getBPins()) {
      for (auto box : pin->getBoxes()) {
        if (tech->name2layer.find(box->getTechLayer()->getName())
            == tech->name2layer.end())
          logger->error(DRT,
                        112,
                        "Unsupported layer {}.",
                        box->getTechLayer()->getName());
        frLayerNum layerNum
            = tech->name2layer[box->getTechLayer()->getName()]->getLayerNum();
        frCoord xl = defdist(block, box->xMin());
        frCoord yl = defdist(block, box->yMin());
        frCoord xh = defdist(block, box->xMax());
        frCoord yh = defdist(block, box->yMax());
        unique_ptr<frRect> pinFig = make_unique<frRect>();
        pinFig->setBBox(Rect(xl, yl, xh, yh));
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
  if (db->getChip() == nullptr)
    logger->error(DRT, 116, "Load design first.");
  odb::dbBlock* block = db->getChip()->getBlock();
  if (block == nullptr)
    logger->error(DRT, 117, "Load design first.");
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

void io::Parser::addFakeNets()
{
  // add VSS fake net
  auto vssFakeNet = make_unique<frNet>(string("frFakeVSS"));
  vssFakeNet->setType(dbSigType::GROUND);
  vssFakeNet->setIsFake(true);
  design->getTopBlock()->addFakeSNet(std::move(vssFakeNet));
  // add VDD fake net
  auto vddFakeNet = make_unique<frNet>(string("frFakeVDD"));
  vddFakeNet->setType(dbSigType::POWER);
  vddFakeNet->setIsFake(true);
  design->getTopBlock()->addFakeSNet(std::move(vddFakeNet));
}

void io::Parser::setRoutingLayerProperties(odb::dbTechLayer* layer,
                                           frLayer* tmpLayer)
{
  for (auto rule : layer->getTechLayerCornerSpacingRules()) {
    std::string widthName("WIDTH");
    std::vector<frCoord> widths;
    std::vector<std::pair<frCoord, frCoord>> spacings;
    rule->getSpacingTable(spacings);
    rule->getWidthTable(widths);
    bool hasSameXY = true;
    for (auto& [spacing1, spacing2] : spacings)
      if (spacing1 != spacing2)
        hasSameXY = false;

    fr1DLookupTbl<frCoord, std::pair<frCoord, frCoord>> cornerSpacingTbl(
        widthName, widths, spacings);
    unique_ptr<frConstraint> uCon
        = make_unique<frLef58CornerSpacingConstraint>(cornerSpacingTbl);
    auto rptr = static_cast<frLef58CornerSpacingConstraint*>(uCon.get());
    switch (rule->getType()) {
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
    rptr->setSameXY(hasSameXY);
    rptr->setExceptSameNet(rule->isExceptSameNet());
    rptr->setExceptSameMetal(rule->isExceptSameMetal());
    tech->addUConstraint(std::move(uCon));
    tmpLayer->addLef58CornerSpacingConstraint(rptr);
  }
  for (auto rule : layer->getTechLayerSpacingTablePrlRules()) {
    string rowName("WIDTH");
    string colName("PARALLELRUNLENGTH");
    frCollection<frCoord> rowVals, colVals;
    frCollection<frCollection<frCoord>> tblVals;
    map<frCoord, pair<frCoord, frCoord>> ewVals;
    map<frUInt4, pair<frCoord, frCoord>> _ewVals;
    rule->getTable(rowVals, colVals, tblVals, _ewVals);
    for (auto& [key, value] : _ewVals)
      ewVals[key] = value;
    shared_ptr<fr2DLookupTbl<frCoord, frCoord, frCoord>> prlTbl
        = make_shared<fr2DLookupTbl<frCoord, frCoord, frCoord>>(
            rowName, rowVals, colName, colVals, tblVals);
    shared_ptr<frLef58SpacingTableConstraint> spacingTableConstraint
        = make_shared<frLef58SpacingTableConstraint>(prlTbl, ewVals);
    spacingTableConstraint->setWrongDirection(rule->isWrongDirection());
    spacingTableConstraint->setSameMask(rule->isSameMask());
    if (rule->isExceeptEol()) {
      spacingTableConstraint->setEolWidth(rule->getEolWidth());
    }
    tech->addConstraint(spacingTableConstraint);
    tmpLayer->addConstraint(spacingTableConstraint);
  }
  for (auto rule : layer->getTechLayerSpacingEolRules()) {
    if (rule->isExceptExactWidthValid() || rule->isFillConcaveCornerValid()
        || rule->isEndPrlSpacingValid() || rule->isEqualRectWidthValid()) {
      logger->warn(utl::DRT,
                   265,
                   "Unsupported LEF58_SPACING rule for layer {}.",
                   layer->getName());
      continue;
    }
    auto con = make_shared<frLef58SpacingEndOfLineConstraint>();
    con->setEol(
        rule->getEolSpace(), rule->getEolWidth(), rule->isExactWidthValid());
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
      auto endToEnd
          = make_shared<frLef58SpacingEndOfLineWithinEndToEndConstraint>();
      within->setEndToEndConstraint(endToEnd);
      endToEnd->setEndToEndSpace(rule->getEndToEndSpace());
      endToEnd->setCutSpace(rule->getOneCutSpace(), rule->getTwoCutSpace());
      if (rule->isExtensionValid()) {
        if (rule->isWrongDirExtensionValid()) {
          endToEnd->setExtension(rule->getExtension(),
                                 rule->getWrongDirExtension());
        } else {
          endToEnd->setExtension(rule->getExtension());
        }
      }
      if (rule->isOtherEndWidthValid()) {
        endToEnd->setOtherEndWidth(rule->getOtherEndWidth());
      }
    }
    if (rule->isParallelEdgeValid()) {
      auto parallelEdge
          = make_shared<frLef58SpacingEndOfLineWithinParallelEdgeConstraint>();
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
      auto len
          = make_shared<frLef58SpacingEndOfLineWithinMaxMinLengthConstraint>();
      within->setMaxMinLengthConstraint(len);
      if (rule->isMinLengthValid())
        len->setLength(false, rule->getMinLength(), rule->isTwoEdgesValid());
      else
        len->setLength(true, rule->getMaxLength(), rule->isTwoEdgesValid());
    }
    if (rule->isEncloseCutValid()) {
      auto enc = make_shared<frLef58SpacingEndOfLineWithinEncloseCutConstraint>(
          rule->getEncloseDist(), rule->getCutToMetalSpace());
      within->setEncloseCutConstraint(enc);
      enc->setAbove(rule->isAboveValid());
      enc->setBelow(rule->isBelowValid());
      enc->setAllCuts(rule->isAllCutsValid());
    }
    tmpLayer->addLef58SpacingEndOfLineConstraint(con.get());
    tech->addConstraint(con);
  }
  if (layer->isRectOnly()) {
    auto rectOnlyConstraint = make_unique<frLef58RectOnlyConstraint>(
        layer->isRectOnlyExceptNonCorePins());
    tmpLayer->setLef58RectOnlyConstraint(rectOnlyConstraint.get());
    tech->addUConstraint(std::move(rectOnlyConstraint));
  }
  if (layer->isRightWayOnGridOnly() || layer->getNumMasks() > 1) {
    auto rightWayOnGridOnlyConstraint
        = make_unique<frLef58RightWayOnGridOnlyConstraint>(
            layer->isRightWayOnGridOnlyCheckMask());
    tmpLayer->setLef58RightWayOnGridOnlyConstraint(
        rightWayOnGridOnlyConstraint.get());
    tech->addUConstraint(std::move(rightWayOnGridOnlyConstraint));
  }
  for (auto rule : layer->getTechLayerMinStepRules()) {
    auto con = make_unique<frLef58MinStepConstraint>();
    con->setMinStepLength(rule->getMinStepLength());
    con->setMaxEdges(rule->isMaxEdgesValid() ? rule->getMaxEdges() : -1);
    con->setMinAdjacentLength(
        rule->isMinAdjLength1Valid() ? rule->getMinAdjLength1() : -1);
    con->setEolWidth(rule->isNoBetweenEol() ? rule->getEolWidth() : -1);
    tmpLayer->addLef58MinStepConstraint(con.get());
    tech->addUConstraint(std::move(con));
  }
  for (auto rule : layer->getTechLayerEolExtensionRules()) {
    frCollection<frCoord> widthTbl;
    frCollection<frCoord> extTbl;
    frCollection<std::pair<frCoord, frCoord>> dbExtTbl;
    rule->getExtensionTable(dbExtTbl);
    for (auto& [width, ext] : dbExtTbl) {
      widthTbl.push_back(width);
      extTbl.push_back(ext);
    }
    auto con = make_unique<frLef58EolExtensionConstraint>(
        fr1DLookupTbl<frCoord, frCoord>("WIDTH", widthTbl, extTbl, false));
    con->setMinSpacing(rule->getSpacing());
    con->setParallelOnly(rule->isParallelOnly());
    tmpLayer->addLef58EolExtConstraint(con.get());
    tech->addUConstraint(std::move(con));
  }
}

void io::Parser::setCutLayerProperties(odb::dbTechLayer* layer,
                                       frLayer* tmpLayer)
{
  for (auto rule : layer->getTechLayerCutClassRules()) {
    auto cutClass = make_unique<frLef58CutClass>();
    string name = rule->getName();
    cutClass->setName(name);
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
  for (auto rule : layer->getTechLayerCutSpacingRules()) {
    switch (rule->getType()) {
      case odb::dbTechLayerCutSpacingRule::CutSpacingType::ADJACENTCUTS: {
        auto con = make_unique<frLef58CutSpacingConstraint>();
        con->setCutSpacing(rule->getCutSpacing());
        con->setCenterToCenter(rule->isCenterToCenter());
        con->setSameNet(rule->isSameNet());
        con->setSameMetal(rule->isSameMetal());
        con->setSameVia(rule->isSameVia());
        con->setAdjacentCuts(rule->getAdjacentCuts());
        if (rule->isExactAligned())
          con->setExactAlignedCut(rule->getNumCuts());
        if (rule->isTwoCutsValid())
          con->setTwoCuts(rule->getTwoCuts());
        con->setSameCut(rule->isSameCut());
        con->setCutWithin(rule->getWithin());
        con->setExceptSamePGNet(rule->isExceptSamePgnet());
        if (rule->getCutClass() != nullptr) {
          std::string className = rule->getCutClass()->getName();
          con->setCutClassName(className);
          auto cutClassIdx = tmpLayer->getCutClassIdx(className);
          if (cutClassIdx != -1)
            con->setCutClassIdx(cutClassIdx);
          else
            continue;
          con->setToAll(rule->isCutClassToAll());
        }
        con->setNoPrl(rule->isNoPrl());
        con->setSideParallelOverlap(rule->isSideParallelOverlap());
        con->setSameMask(rule->isSameMask());

        tmpLayer->addLef58CutSpacingConstraint(con.get());
        tech->addUConstraint(std::move(con));
        break;
      }

      case odb::dbTechLayerCutSpacingRule::CutSpacingType::LAYER: {
        if (rule->getSecondLayer() == nullptr)
          continue;
        auto con = make_unique<frLef58CutSpacingConstraint>();
        con->setCutSpacing(rule->getCutSpacing());
        con->setCenterToCenter(rule->isCenterToCenter());
        con->setSameNet(rule->isSameNet());
        con->setSameMetal(rule->isSameMetal());
        con->setSameVia(rule->isSameVia());
        con->setSecondLayerName(rule->getSecondLayer()->getName());
        con->setStack(rule->isStack());
        if (rule->isOrthogonalSpacingValid()) {
          con->setOrthogonalSpacing(rule->getOrthogonalSpacing());
        }
        if (rule->getCutClass() != nullptr) {
          std::string className = rule->getCutClass()->getName();
          con->setCutClassName(className);
          auto cutClassIdx = tmpLayer->getCutClassIdx(className);
          if (cutClassIdx != -1)
            con->setCutClassIdx(cutClassIdx);
          else
            continue;
          con->setShortEdgeOnly(rule->isShortEdgeOnly());
          if (rule->isPrlValid())
            con->setPrl(rule->getPrl());
          con->setConcaveCorner(rule->isConcaveCorner());
          if (rule->isConcaveCornerWidth()) {
            con->setWidth(rule->getWidth());
            con->setEnclosure(rule->getEnclosure());
            con->setEdgeLength(rule->getEdgeLength());
          } else if (rule->isConcaveCornerParallel()) {
            con->setParLength(rule->getParLength());
            con->setParWithin(rule->getParWithin());
            con->setEnclosure(rule->getParEnclosure());
          } else if (rule->isConcaveCornerEdgeLength()) {
            con->setEdgeLength(rule->getEdgeLength());
            con->setEdgeEnclosure(rule->getEdgeEnclosure());
            con->setAdjEnclosure(rule->getAdjEnclosure());
          }
          if (rule->isExtensionValid())
            con->setExtension(rule->getExtension());
          if (rule->isNonEolConvexCorner()) {
            con->setEolWidth(rule->getEolWidth());
            if (rule->isMinLengthValid())
              con->setMinLength(rule->getMinLength());
          }
          if (rule->isAboveWidthValid()) {
            rule->setWidth(rule->getAboveWidth());
            if (rule->isAboveWidthEnclosureValid())
              rule->setEnclosure(rule->getAboveEnclosure());
          }
          con->setMaskOverlap(rule->isMaskOverlap());
          con->setWrongDirection(rule->isWrongDirection());
        }
        tmpLayer->addLef58CutSpacingConstraint(con.get());
        tech->addUConstraint(std::move(con));
        break;
      }
      case odb::dbTechLayerCutSpacingRule::CutSpacingType::AREA:
        logger->warn(
            utl::DRT,
            258,
            "Unsupported LEF58_SPACING rule for layer {} of type AREA.",
            layer->getName());
        break;
      case odb::dbTechLayerCutSpacingRule::CutSpacingType::MAXXY:
        logger->warn(
            utl::DRT,
            161,
            "Unsupported LEF58_SPACING rule for layer {} of type MAXXY.",
            layer->getName());
        break;
      case odb::dbTechLayerCutSpacingRule::CutSpacingType::SAMEMASK:
        logger->warn(
            utl::DRT,
            259,
            "Unsupported LEF58_SPACING rule for layer {} of type SAMEMASK.",
            layer->getName());
        break;
      case odb::dbTechLayerCutSpacingRule::CutSpacingType::PARALLELOVERLAP:
        logger->warn(utl::DRT,
                     260,
                     "Unsupported LEF58_SPACING rule for layer {} of type "
                     "PARALLELOVERLAP.",
                     layer->getName());
        break;
      case odb::dbTechLayerCutSpacingRule::CutSpacingType::PARALLELWITHIN:
        logger->warn(utl::DRT,
                     261,
                     "Unsupported LEF58_SPACING rule for layer {} of type "
                     "PARALLELWITHIN.",
                     layer->getName());
        break;
      case odb::dbTechLayerCutSpacingRule::CutSpacingType::SAMEMETALSHAREDEDGE:
        logger->warn(utl::DRT,
                     262,
                     "Unsupported LEF58_SPACING rule for layer {} of type "
                     "SAMEMETALSHAREDEDGE.",
                     layer->getName());
        break;
      default:
        logger->warn(utl::DRT,
                     263,
                     "Unsupported LEF58_SPACING rule for layer {}.",
                     layer->getName());
        break;
    }
  }
  for (auto rule : layer->getTechLayerCutSpacingTableDefRules()) {
    if (rule->isLayerValid() && tmpLayer->getLayerNum() == 1)
      continue;
    if (rule->isSameMask()) {
      logger->warn(utl::DRT,
                   279,
                   "SAMEMASK unsupported for cut LEF58_SPACINGTABLE rule");
      continue;
    }
    auto con = make_shared<frLef58CutSpacingTableConstraint>(rule);
    frCollection<frCollection<std::pair<frCoord, frCoord>>> table;
    std::map<frString, frUInt4> rowMap, colMap;
    rule->getSpacingTable(table, rowMap, colMap);
    frString cutClass1 = colMap.begin()->first;
    if (cutClass1.find("/") != std::string::npos)
      cutClass1 = cutClass1.substr(0, cutClass1.find("/"));
    frString cutClass2 = rowMap.begin()->first;
    if (cutClass2.find("/") != std::string::npos)
      cutClass2 = cutClass2.substr(0, cutClass2.find("/"));
    auto spc = table[0][0];
    con->setDefaultSpacing(spc);
    con->setDefaultCenterToCenter(rule->isCenterToCenter(cutClass1, cutClass2));
    con->setDefaultCenterAndEdge(rule->isCenterAndEdge(cutClass1, cutClass2));
    if (rule->isLayerValid()) {
      if (rule->isSameMetal()) {
        tmpLayer->setLef58SameMetalInterCutSpcTblConstraint(con.get());
      } else if (rule->isSameNet()) {
        tmpLayer->setLef58SameNetInterCutSpcTblConstraint(con.get());
      } else {
        tmpLayer->setLef58DefaultInterCutSpcTblConstraint(con.get());
      }
    } else {
      if (rule->isSameMetal()) {
        tmpLayer->setLef58SameMetalCutSpcTblConstraint(con.get());
      } else if (rule->isSameNet()) {
        tmpLayer->setLef58SameNetCutSpcTblConstraint(con.get());
      } else {
        tmpLayer->setLef58DiffNetCutSpcTblConstraint(con.get());
      }
    }
    tech->addConstraint(con);
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
  tmpMSLayer->setType(dbTechLayerType::MASTERSLICE);
}

void io::Parser::addDefaultCutLayer()
{
  std::string viaLayerName("FR_VIA");
  unique_ptr<frLayer> uCutLayer = make_unique<frLayer>();
  auto tmpCutLayer = uCutLayer.get();
  tmpCutLayer->setLayerNum(readLayerCnt++);
  tmpCutLayer->setName(viaLayerName);
  tech->addLayer(std::move(uCutLayer));
  tmpCutLayer->setType(dbTechLayerType::CUT);
}

void io::Parser::addRoutingLayer(odb::dbTechLayer* layer)
{
  if (layer->getLef58Type() == odb::dbTechLayer::LEF58_TYPE::MIMCAP)
    return;
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
  if (layer->getMinWidth() > layer->getWidth())
    logger->warn(
        DRT,
        210,
        "Layer {} minWidth is larger than width. Using width as minWidth.",
        layer->getName());
  tmpLayer->setMinWidth(std::min(layer->getMinWidth(), layer->getWidth()));
  // add minWidth constraint
  auto minWidthConstraint
      = make_unique<frMinWidthConstraint>(tmpLayer->getMinWidth());
  tmpLayer->setMinWidthConstraint(minWidthConstraint.get());
  tech->addUConstraint(std::move(minWidthConstraint));

  tmpLayer->setType(dbTechLayerType::ROUTING);
  if (layer->getDirection() == odb::dbTechLayerDir::HORIZONTAL)
    tmpLayer->setDir(dbTechLayerDir::HORIZONTAL);
  else if (layer->getDirection() == odb::dbTechLayerDir::VERTICAL)
    tmpLayer->setDir(dbTechLayerDir::VERTICAL);

  tmpLayer->setPitch(layer->getPitch());
  tmpLayer->setNumMasks(layer->getNumMasks());

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
  setRoutingLayerProperties(layer, tmpLayer);
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
          DRT,
          139,
          "minEnclosedArea constraint with width is not supported, skipped.");
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
      logger->warn(DRT, 140, "SpacingRange unsupported.");
    } else if (rule->hasLengthThreshold()) {
      logger->warn(DRT, 141, "SpacingLengthThreshold unsupported.");
    } else if (rule->hasSpacingNotchLength()) {
      logger->warn(DRT, 142, "SpacingNotchLength unsupported.");
    } else if (rule->hasSpacingEndOfNotchWidth()) {
      logger->warn(DRT, 143, "SpacingEndOfNotchWidth unsupported.");
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
        logger->warn(DRT,
                     138,
                     "New SPACING SAMENET overrides old"
                     "SPACING SAMENET rule.");
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
        logger->warn(DRT,
                     144,
                     "New SPACING SAMENET overrides old"
                     "SPACING SAMENET rule.");
      tmpLayer->setMinSpacing(rptr);
    }
  }
  if (!layer->getV55InfluenceEntries().empty()) {
    frCollection<frCoord> widthTbl;
    frCollection<std::pair<frCoord, frCoord>> valTbl;
    for (auto entry : layer->getV55InfluenceEntries()) {
      frUInt4 width, within, spacing;
      entry->getV55InfluenceEntry(width, within, spacing);
      widthTbl.push_back(width);
      valTbl.push_back({within, spacing});
    }
    fr1DLookupTbl<frCoord, std::pair<frCoord, frCoord>> tbl(
        "WIDTH", widthTbl, valTbl);
    unique_ptr<frConstraint> uCon
        = make_unique<frSpacingTableInfluenceConstraint>(tbl);
    auto rptr = static_cast<frSpacingTableInfluenceConstraint*>(uCon.get());
    tech->addUConstraint(std::move(uCon));
    tmpLayer->setSpacingTableInfluence(rptr);
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
          DRT,
          145,
          "New SPACINGTABLE PARALLELRUNLENGTH overrides old SPACING rule.");
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

    frCollection<frSpacingTableTwRowType> rowVals;
    for (uint j = 0; j < layer->getTwoWidthsSpacingTableNumWidths(); ++j) {
      frCoord width = layer->getTwoWidthsSpacingTableWidth(j);
      frCoord prl = layer->getTwoWidthsSpacingTablePRL(j);
      rowVals.push_back(frSpacingTableTwRowType(width, prl));
    }

    unique_ptr<frConstraint> uCon
        = make_unique<frSpacingTableTwConstraint>(rowVals, tblVals);
    auto rptr = static_cast<frSpacingTableTwConstraint*>(uCon.get());
    tech->addUConstraint(std::move(uCon));
    if (tmpLayer->getMinSpacing())
      logger->warn(
          DRT, 146, "New SPACINGTABLE TWOWIDTHS overrides old SPACING rule.");
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

  for (auto rule : layer->getTechLayerEolKeepOutRules()) {
    unique_ptr<frConstraint> uCon = make_unique<frLef58EolKeepOutConstraint>();
    auto rptr = static_cast<frLef58EolKeepOutConstraint*>(uCon.get());
    rptr->setEolWidth(rule->getEolWidth());
    rptr->setBackwardExt(rule->getBackwardExt());
    rptr->setForwardExt(rule->getForwardExt());
    rptr->setSideExt(rule->getSideExt());
    rptr->setCornerOnly(rule->isCornerOnly());
    rptr->setExceptWithin(rule->isExceptWithin());
    rptr->setWithinLow(rule->getWithinLow());
    rptr->setWithinHigh(rule->getWithinHigh());
    tech->addUConstraint(std::move(uCon));
    tmpLayer->addLef58EolKeepOutConstraint(rptr);
  }
}

void io::Parser::addCutLayer(odb::dbTechLayer* layer)
{
  if (layer->getLef58Type() == odb::dbTechLayer::LEF58_TYPE::MIMCAP)
    return;
  if (readLayerCnt == 0)
    addDefaultMasterSliceLayer();

  unique_ptr<frLayer> uLayer = make_unique<frLayer>();
  auto tmpLayer = uLayer.get();
  tmpLayer->setLayerNum(readLayerCnt++);
  tmpLayer->setName(layer->getName());
  tmpLayer->setType(dbTechLayerType::CUT);
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
      logger->warn(DRT,
                   147,
                   "cutWithin is smaller than cutSpacing for ADJACENTCUTS on "
                   "layer {}, please check your rule definition.",
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
  setCutLayerProperties(layer, tmpLayer);
}

void io::Parser::addMasterSliceLayer(odb::dbTechLayer* layer)
{
  if (layer->getLef58Type() != odb::dbTechLayer::LEF58_TYPE::NWELL
      && layer->getLef58Type() != odb::dbTechLayer::LEF58_TYPE::PWELL
      && layer->getLef58Type() != odb::dbTechLayer::LEF58_TYPE::DIFFUSION)
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
      auto tmpMaster = make_unique<frMaster>(master->getName());
      frCoord originX;
      frCoord originY;
      master->getOrigin(originX, originY);
      frCoord sizeX = master->getWidth();
      frCoord sizeY = master->getHeight();
      vector<frBoundary> bounds;
      frBoundary bound;
      vector<Point> points;
      points.push_back(Point(originX, originY));
      points.push_back(Point(sizeX, originY));
      points.push_back(Point(sizeX, sizeY));
      points.push_back(Point(originX, sizeY));
      bound.setPoints(points);
      bounds.push_back(bound);
      tmpMaster->setBoundaries(bounds);
      tmpMaster->setMasterType(master->getType());

      for (auto _term : master->getMTerms()) {
        unique_ptr<frMTerm> uTerm = make_unique<frMTerm>(_term->getName());
        auto term = uTerm.get();
        term->setId(numTerms);
        numTerms++;
        tmpMaster->addTerm(std::move(uTerm));

        term->setType(_term->getSigType());
        term->setDirection(_term->getIoType());

        int i = 0;
        for (auto mpin : _term->getMPins()) {
          auto pinIn = make_unique<frMPin>();
          pinIn->setId(i++);
          for (auto box : mpin->getGeometry()) {
            frLayerNum layerNum = -1;
            string layer = box->getTechLayer()->getName();
            if (tech->name2layer.find(layer) == tech->name2layer.end()) {
              auto type = box->getTechLayer()->getType();
              if (type == odb::dbTechLayerType::ROUTING
                  || type == odb::dbTechLayerType::CUT)
                logger->warn(DRT,
                             122,
                             "Layer {} is skipped for {}/{}.",
                             layer,
                             tmpMaster->getName(),
                             _term->getName());
              continue;
            } else
              layerNum = tech->name2layer.at(layer)->getLayerNum();

            frCoord xl = box->xMin();
            frCoord yl = box->yMin();
            frCoord xh = box->xMax();
            frCoord yh = box->yMax();
            unique_ptr<frRect> pinFig = make_unique<frRect>();
            pinFig->setBBox(Rect(xl, yl, xh, yh));
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
          auto type = obs->getTechLayer()->getType();
          if (type == odb::dbTechLayerType::ROUTING
              || type == odb::dbTechLayerType::CUT)
            logger->warn(DRT,
                         123,
                         "Layer {} is skipped for {}/OBS.",
                         layer,
                         tmpMaster->getName());
          continue;
        } else
          layerNum = tech->name2layer.at(layer)->getLayerNum();
        auto blkIn = make_unique<frBlockage>();
        blkIn->setId(numBlockages);
        blkIn->setDesignRuleWidth(obs->getDesignRuleWidth());
        numBlockages++;
        auto pinIn = make_unique<frBPin>();
        pinIn->setId(0);
        frCoord xl = obs->xMin();
        frCoord yl = obs->yMin();
        frCoord xh = obs->xMax();
        frCoord yh = obs->yMax();
        // pinFig
        unique_ptr<frRect> pinFig = make_unique<frRect>();
        pinFig->setBBox(Rect(xl, yl, xh, yh));
        pinFig->addToPin(pinIn.get());
        pinFig->setLayerNum(layerNum);
        unique_ptr<frPinFig> uptr(std::move(pinFig));
        pinIn->addPinFig(std::move(uptr));
        blkIn->setPin(std::move(pinIn));
        tmpMaster->addBlockage(std::move(blkIn));
      }
      tmpMaster->setId(numMasters + 1);
      design->addMaster(std::move(tmpMaster));
      numMasters++;
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
      logger->error(DRT, 128, "Unsupported viarule {}.", rule->getName());
    map<frLayerNum, int> lNum2Int;
    for (int i = 0; i < count; i++) {
      auto layerRule = rule->getViaLayerRule(i);
      string layerName = layerRule->getLayer()->getName();
      if (tech->name2layer.find(layerName) == tech->name2layer.end())
        logger->error(DRT,
                      129,
                      "Unknown layer {} for viarule {}.",
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
      logger->error(
          DRT, 130, "Non-consecutive layers for viarule {}.", rule->getName());
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
        Point enc(x, y);
        switch (lNum2Int[layerNum]) {
          case 1:
            viaRuleGen->setLayer1Enc(enc);
            break;
          case 2:
            logger->warn(DRT,
                         131,
                         "cutLayer cannot have overhangs in viarule {}, "
                         "skipping enclosure.",
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
        Rect box(xl, yl, xh, yh);
        switch (lNum2Int[layerNum]) {
          case 1:
            logger->warn(
                DRT,
                132,
                "botLayer cannot have rect in viarule {}, skipping rect.",
                rule->getName());
            break;
          case 2:
            viaRuleGen->setCutRect(box);
            break;
          default:
            logger->warn(
                DRT,
                133,
                "topLayer cannot have rect in viarule {}, skipping rect.",
                rule->getName());
            break;
        }
      }
      if (layerRule->hasSpacing()) {
        frCoord x;
        frCoord y;
        layerRule->getSpacing(x, y);
        Point pt(x, y);
        switch (lNum2Int[layerNum]) {
          case 1:
            logger->warn(
                DRT,
                134,
                "botLayer cannot have spacing in viarule {}, skipping spacing.",
                rule->getName());
            break;
          case 2:
            viaRuleGen->setCutSpacing(pt);
            break;
          default:
            logger->warn(
                DRT,
                135,
                "botLayer cannot have spacing in viarule {}, skipping spacing.",
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
    bool has_unknown_layer = false;
    for (auto box : via->getBoxes()) {
      string layerName = box->getTechLayer()->getName();
      if (tech->name2layer.find(layerName) == tech->name2layer.end()) {
        logger->warn(DRT,
                     124,
                     "Via {} with unused layer {} will be ignored.",
                     layerName,
                     via->getName());
        has_unknown_layer = true;
        continue;
      }
      frLayerNum lNum = tech->name2layer[layerName]->getLayerNum();
      lNum2Int[lNum] = 1;
    }
    if (has_unknown_layer) {
      continue;
    }
    if (lNum2Int.size() != 3)
      logger->error(DRT, 125, "Unsupported via {}.", via->getName());
    int curOrder = 0;
    for (auto [lnum, i] : lNum2Int) {
      lNum2Int[lnum] = ++curOrder;
    }

    if (lNum2Int.begin()->first + 2 != (--lNum2Int.end())->first) {
      logger->error(
          DRT, 126, "Non-consecutive layers for via {}.", via->getName());
    }
    auto viaDef = make_unique<frViaDef>(via->getName());
    if (via->isDefault())
      viaDef->setDefault(true);
    for (auto box : via->getBoxes()) {
      frLayerNum layerNum;
      string layer = box->getTechLayer()->getName();
      if (tech->name2layer.find(layer) == tech->name2layer.end())
        logger->error(
            DRT, 127, "Unknown layer {} for via {}.", layer, via->getName());
      else
        layerNum = tech->name2layer.at(layer)->getLayerNum();
      frCoord xl = box->xMin();
      frCoord yl = box->yMin();
      frCoord xh = box->xMax();
      frCoord yh = box->yMax();
      unique_ptr<frRect> pinFig = make_unique<frRect>();
      pinFig->setBBox(Rect(xl, yl, xh, yh));
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
      Rect box;
      cutFig->getBBox(box);
      auto width = box.minDXDY();
      auto length = box.maxDXDY();
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
    logger->error(DRT, 136, "Load design first.");
  tech->setDBUPerUU(_tech->getDbUnitsPerMicron());
  USEMINSPACING_OBS = _tech->getUseMinSpacingObs() == odb::dbOnOffType::ON;
  tech->setManufacturingGrid(frUInt4(_tech->getManufacturingGrid()));
  setLayers(_tech);
  setTechVias(db->getTech());
  setTechViaRules(db->getTech());
  setMacros(db);
  setNDRs(db);
}

void io::Parser::readDb(odb::dbDatabase* db)
{
  if (design->getTopBlock() != nullptr)
    return;
  if (VERBOSE > 0) {
    logger->info(DRT, 149, "Reading tech and libs.");
  }
  readTechAndLibs(db);
  if (VERBOSE > 0) {
    logger->report("");
    logger->report("Units:                {}", tech->getDBUPerUU());
    logger->report("Number of layers:     {}", tech->layers.size());
    logger->report("Number of macros:     {}", design->masters_.size());
    logger->report("Number of vias:       {}", tech->vias.size());
    logger->report("Number of viarulegen: {}", tech->viaRuleGenerates.size());
    logger->report("");
  }

  auto numLefVia = tech->vias.size();

  if (VERBOSE > 0) {
    logger->info(DRT, 150, "Reading design.");
  }

  readDesign(db);

  if (VERBOSE > 0) {
    logger->report("");
    Rect dieBox;
    design->getTopBlock()->getDieBox(dieBox);
    logger->report("Design:                   {}",
                   design->getTopBlock()->getName());
    // TODO Rect can't be logged directly
    stringstream dieBoxSStream;
    dieBoxSStream << dieBox;
    logger->report("Die area:                 {}", dieBoxSStream.str());
    logger->report("Number of track patterns: {}",
                   design->getTopBlock()->getTrackPatterns().size());
    logger->report("Number of DEF vias:       {}",
                   tech->vias.size() - numLefVia);
    logger->report("Number of components:     {}",
                   design->getTopBlock()->insts_.size());
    logger->report("Number of terminals:      {}",
                   design->getTopBlock()->terms_.size());
    logger->report("Number of snets:          {}",
                   design->getTopBlock()->snets_.size());
    logger->report("Number of nets:           {}",
                   design->getTopBlock()->nets_.size());
    logger->report("");
  }
}

void io::Parser::readGuide()
{
  ProfileTask profile("IO:readGuide");

  if (VERBOSE > 0) {
    logger->info(DRT, 151, "Reading guide.");
  }

  int numGuides = 0;

  string netName = "";
  frNet* net = nullptr;

  ifstream fin(GUIDE_FILE.c_str());
  string line;
  Rect box;
  frLayerNum layerNum;

  if (fin.is_open()) {
    while (fin.good()) {
      getline(fin, line);
      // cout <<line <<endl <<line.size() <<endl;
      if (line == "(" || line == "")
        continue;
      if (line == ")") {
        continue;
      }

      stringstream ss(line);
      string word = "";
      vector<string> vLine;
      while (!ss.eof()) {
        ss >> word;
        vLine.push_back(word);
        // cout <<word <<" ";
      }
      // cout <<endl;

      if (vLine.size() == 0) {
        logger->error(DRT, 152, "Error reading guide file {}.", GUIDE_FILE);
      } else if (vLine.size() == 1) {
        netName = vLine[0];
        if (design->topBlock_->name2net_.find(vLine[0])
            == design->topBlock_->name2net_.end()) {
          logger->error(DRT, 153, "Cannot find net {}.", vLine[0]);
        }
        net = design->topBlock_->name2net_[netName];
      } else if (vLine.size() == 5) {
        if (tech->name2layer.find(vLine[4]) == tech->name2layer.end()) {
          logger->error(DRT, 154, "Cannot find layer {}.", vLine[4]);
        }
        layerNum = tech->name2layer[vLine[4]]->getLayerNum();

        if ((layerNum < BOTTOM_ROUTING_LAYER && layerNum != VIA_ACCESS_LAYERNUM)
            || layerNum > TOP_ROUTING_LAYER)
          logger->error(DRT,
                        155,
                        "Guide in net {} uses layer {} ({})"
                        " that is outside the allowed routing range "
                        "[{} ({}), ({})].",
                        netName,
                        vLine[4],
                        layerNum,
                        tech->getLayer(BOTTOM_ROUTING_LAYER)->getName(),
                        BOTTOM_ROUTING_LAYER,
                        tech->getLayer(TOP_ROUTING_LAYER)->getName(),
                        TOP_ROUTING_LAYER);

        box.init(
            stoi(vLine[0]), stoi(vLine[1]), stoi(vLine[2]), stoi(vLine[3]));
        frRect rect;
        rect.setBBox(box);
        rect.setLayerNum(layerNum);
        tmpGuides[net].push_back(rect);
        ++numGuides;
        if (numGuides < 1000000) {
          if (numGuides % 100000 == 0) {
            logger->info(DRT, 156, "guideIn read {} guides.", numGuides);
          }
        } else {
          if (numGuides % 1000000 == 0) {
            logger->info(DRT, 157, "guideIn read {} guides.", numGuides);
          }
        }

      } else {
        logger->error(DRT, 158, "Error reading guide file {}.", GUIDE_FILE);
      }
    }
    fin.close();
  } else {
    logger->error(DRT, 159, "Failed to open guide file {}.", GUIDE_FILE);
  }

  if (VERBOSE > 0) {
    logger->report("");
    logger->report("Number of guides:     {}", numGuides);
    logger->report("");
  }
}

void io::Writer::fillConnFigs_net(frNet* net, bool isTA)
{
  auto netName = net->getName();
  if (isTA) {
    for (auto& uGuide : net->getGuides()) {
      // cout <<"find guide" <<endl;
      for (auto& uConnFig : uGuide->getRoutes()) {
        auto connFig = uConnFig.get();
        if (connFig->typeId() == frcPathSeg) {
          connFigs[netName].push_back(
              make_shared<frPathSeg>(*static_cast<frPathSeg*>(connFig)));
        } else if (connFig->typeId() == frcVia) {
          connFigs[netName].push_back(
              make_shared<frVia>(*static_cast<frVia*>(connFig)));
        } else {
          logger->warn(
              DRT,
              247,
              "io::Writer::fillConnFigs_net does not support this type.");
        }
      }
    }
  } else {
    for (auto& shape : net->getShapes()) {
      if (shape->typeId() == frcPathSeg) {
        auto pathSeg = *static_cast<frPathSeg*>(shape.get());
        Point start, end;
        pathSeg.getPoints(start, end);

        connFigs[netName].push_back(make_shared<frPathSeg>(pathSeg));
      }
    }
    for (auto& via : net->getVias()) {
      connFigs[netName].push_back(make_shared<frVia>(*via));
    }
    for (auto& shape : net->getPatchWires()) {
      auto pwire = static_cast<frPatchWire*>(shape.get());
      connFigs[netName].push_back(make_shared<frPatchWire>(*pwire));
    }
  }
}

void io::Writer::splitVia_helper(
    frLayerNum layerNum,
    int isH,
    frCoord trackLoc,
    frCoord x,
    frCoord y,
    vector<vector<map<frCoord, vector<shared_ptr<frPathSeg>>>>>& mergedPathSegs)
{
  if (layerNum >= 0 && layerNum < (int) (getTech()->getLayers().size())
      && mergedPathSegs.at(layerNum).at(isH).find(trackLoc)
             != mergedPathSegs.at(layerNum).at(isH).end()) {
    for (auto& pathSeg : mergedPathSegs.at(layerNum).at(isH).at(trackLoc)) {
      Point begin, end;
      pathSeg->getPoints(begin, end);
      if ((isH == 0 && (begin.x() < x) && (end.x() > x))
          || (isH == 1 && (begin.y() < y) && (end.y() > y))) {
        frSegStyle style1, style2, style_default;
        pathSeg->getStyle(style1);
        pathSeg->getStyle(style2);
        style_default = getTech()->getLayer(layerNum)->getDefaultSegStyle();
        shared_ptr<frPathSeg> newPathSeg = make_shared<frPathSeg>(*pathSeg);
        pathSeg->setPoints(begin, Point(x, y));
        style1.setEndStyle(style_default.getEndStyle(),
                           style_default.getEndExt());
        pathSeg->setStyle(style1);
        newPathSeg->setPoints(Point(x, y), end);
        style2.setBeginStyle(style_default.getBeginStyle(),
                             style_default.getBeginExt());
        newPathSeg->setStyle(style2);
        mergedPathSegs.at(layerNum).at(isH).at(trackLoc).push_back(newPathSeg);
        // via can only intersect at most one merged pathseg on one track
        break;
      }
    }
  }
}

// merge pathseg, delete redundant via
void io::Writer::mergeSplitConnFigs(list<shared_ptr<frConnFig>>& connFigs)
{
  // if (VERBOSE > 0) {
  //   cout <<endl << "merge and split." <<endl;
  // }
  //  initialzie pathseg and via map
  map<tuple<frLayerNum, bool, frCoord>,
      map<frCoord, vector<tuple<shared_ptr<frPathSeg>, bool>>>>
      pathSegMergeMap;
  map<tuple<frCoord, frCoord, frLayerNum>, shared_ptr<frVia>> viaMergeMap;
  for (auto& connFig : connFigs) {
    if (connFig->typeId() == frcPathSeg) {
      auto pathSeg = dynamic_pointer_cast<frPathSeg>(connFig);
      Point begin, end;
      pathSeg->getPoints(begin, end);
      frLayerNum layerNum = pathSeg->getLayerNum();
      if (begin == end) {
        // std::cout << "Warning: 0 length connFig\n";
        continue;  // if segment length = 0, ignore
      } else {
        // std::cout << "xxx\n";
        bool isH = (begin.x() == end.x()) ? false : true;
        frCoord trackLoc = isH ? begin.y() : begin.x();
        frCoord beginCoord = isH ? begin.x() : begin.y();
        frCoord endCoord = isH ? end.x() : end.y();
        pathSegMergeMap[make_tuple(layerNum, isH, trackLoc)][beginCoord]
            .push_back(make_tuple(pathSeg, true));
        pathSegMergeMap[make_tuple(layerNum, isH, trackLoc)][endCoord]
            .push_back(make_tuple(pathSeg, false));
      }
    } else if (connFig->typeId() == frcVia) {
      auto via = dynamic_pointer_cast<frVia>(connFig);
      auto cutLayerNum = via->getViaDef()->getCutLayerNum();
      Point viaPoint;
      via->getOrigin(viaPoint);
      viaMergeMap[make_tuple(viaPoint.x(), viaPoint.y(), cutLayerNum)] = via;
      // cout <<"found via" <<endl;
    }
  }

  // merge pathSeg
  map<frCoord, vector<shared_ptr<frPathSeg>>> tmp1;
  vector<map<frCoord, vector<shared_ptr<frPathSeg>>>> tmp2(2, tmp1);
  vector<vector<map<frCoord, vector<shared_ptr<frPathSeg>>>>> mergedPathSegs(
      getTech()->getLayers().size(), tmp2);

  for (auto& it1 : pathSegMergeMap) {
    auto layerNum = get<0>(it1.first);
    int isH = get<1>(it1.first);
    auto trackLoc = get<2>(it1.first);
    bool hasSeg = false;
    int cnt = 0;
    shared_ptr<frPathSeg> newPathSeg;
    frSegStyle style;
    Point begin, end;
    for (auto& it2 : it1.second) {
      // cout <<"coord " <<coord <<endl;
      for (auto& pathSegTuple : it2.second) {
        cnt += get<1>(pathSegTuple) ? 1 : -1;
      }
      // newPathSeg begin
      if (!hasSeg && cnt > 0) {
        style.setBeginStyle(frcTruncateEndStyle, 0);
        style.setEndStyle(frcTruncateEndStyle, 0);
        newPathSeg = make_shared<frPathSeg>(*(get<0>(*(it2.second.begin()))));
        for (auto& pathSegTuple : it2.second) {
          auto pathSeg = get<0>(pathSegTuple);
          auto isBegin = get<1>(pathSegTuple);
          if (isBegin) {
            pathSeg->getPoints(begin, end);
            frSegStyle tmpStyle;
            pathSeg->getStyle(tmpStyle);
            if (tmpStyle.getBeginExt() > style.getBeginExt()) {
              style.setBeginStyle(tmpStyle.getBeginStyle(),
                                  tmpStyle.getBeginExt());
            }
          }
        }
        newPathSeg->setStyle(style);
        hasSeg = true;
        // newPathSeg end
      } else if (hasSeg && cnt == 0) {
        newPathSeg->getPoints(begin, end);
        for (auto& pathSegTuple : it2.second) {
          auto pathSeg = get<0>(pathSegTuple);
          auto isBegin = get<1>(pathSegTuple);
          if (!isBegin) {
            Point tmp;
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
  // mergedPathSegs[layerNum][isHorizontal] is a map<frCoord,
  // vector<shared_ptr<frPathSeg> > >
  // map < tuple<frCoord, frCoord, frLayerNum>, shared_ptr<frVia> > viaMergeMap;
  for (auto& it1 : viaMergeMap) {
    auto x = get<0>(it1.first);
    auto y = get<1>(it1.first);
    auto cutLayerNum = get<2>(it1.first);
    frCoord trackLoc;

    auto layerNum = cutLayerNum - 1;
    int isH = 1;
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
  for (auto& it1 : mergedPathSegs) {
    // vertical for mapIt1
    for (auto& mapIt1 : it1.at(0)) {
      // horizontal for mapIt2
      for (auto& mapIt2 : it1.at(1)) {
        // at most split once
        // seg1 is vertical
        for (auto& seg1 : mapIt1.second) {
          bool skip = false;
          // seg2 is horizontal
          Point seg1Begin, seg1End;
          seg1->getPoints(seg1Begin, seg1End);
          for (auto& seg2 : mapIt2.second) {
            Point seg2Begin, seg2End;
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
              seg1->setPoints(seg1Begin, Point(seg1End.x(), seg2End.y()));
              // modify newSeg1
              newSeg1->setPoints(Point(seg1End.x(), seg2Begin.y()), seg1End);
              // modify endstyle
              auto layerNum = seg1->getLayerNum();
              frSegStyle tmpStyle1;
              frSegStyle tmpStyle2;
              frSegStyle style_default;
              seg1->getStyle(tmpStyle1);
              seg1->getStyle(tmpStyle2);
              style_default
                  = getTech()->getLayer(layerNum)->getDefaultSegStyle();
              tmpStyle1.setEndStyle(frcExtendEndStyle,
                                    style_default.getEndExt());
              seg1->setStyle(tmpStyle1);
              tmpStyle2.setBeginStyle(frcExtendEndStyle,
                                      style_default.getBeginExt());
              newSeg1->setStyle(tmpStyle2);
            }
            // check whether seg2 needs to be split, break seg2
            if (seg1Begin.x() > seg2Begin.x() && seg1Begin.x() < seg2End.x()) {
              pushNewSeg2 = true;
              newSeg2 = make_shared<frPathSeg>(*seg1);
              // modify seg2
              seg2->setPoints(seg2Begin, Point(seg1End.x(), seg2End.y()));
              // modify newSeg2
              newSeg2->setPoints(Point(seg1End.x(), seg2Begin.y()), seg2End);
              // modify endstyle
              auto layerNum = seg2->getLayerNum();
              frSegStyle tmpStyle1;
              frSegStyle tmpStyle2;
              frSegStyle style_default;
              seg2->getStyle(tmpStyle1);
              seg2->getStyle(tmpStyle2);
              style_default
                  = getTech()->getLayer(layerNum)->getDefaultSegStyle();
              tmpStyle1.setEndStyle(frcExtendEndStyle,
                                    style_default.getEndExt());
              seg2->setStyle(tmpStyle1);
              tmpStyle2.setBeginStyle(frcExtendEndStyle,
                                      style_default.getBeginExt());
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
            // cout <<"found" <<endl;
          }
          if (skip)
            break;
        }
      }
    }
  }

  // write back pathseg
  connFigs.clear();
  for (auto& it1 : mergedPathSegs) {
    for (auto& it2 : it1) {
      for (auto& it3 : it2) {
        for (auto& it4 : it3.second) {
          connFigs.push_back(it4);
        }
      }
    }
  }

  // write back via
  // map < tuple<frCoord, frCoord, frLayerNum>, shared_ptr<frVia> > viaMergeMap;
  for (auto& it : viaMergeMap) {
    connFigs.push_back(it.second);
  }
}

void io::Writer::fillViaDefs()
{
  viaDefs.clear();
  for (auto& uViaDef : getDesign()->getTech()->getVias()) {
    auto viaDef = uViaDef.get();
    if (viaDef->isAddedByRouter()) {
      viaDefs.push_back(viaDef);
    }
  }
}

void io::Writer::fillConnFigs(bool isTA)
{
  connFigs.clear();
  if (VERBOSE > 0) {
    logger->info(DRT, 180, "Post processing.");
  }
  for (auto& net : getDesign()->getTopBlock()->getNets()) {
    fillConnFigs_net(net.get(), isTA);
  }
  if (isTA) {
    for (auto& it : connFigs) {
      mergeSplitConnFigs(it.second);
    }
  }
}

void io::Writer::updateDbVias(odb::dbBlock* block, odb::dbTech* tech)
{
  Rect box;
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
      logger->error(DRT,
                    113,
                    "Tech layers for via {} not found in db tech.",
                    via->getName());
    }
    odb::dbVia* _db_via = odb::dbVia::create(block, via->getName().c_str());

    for (auto& fig : via->getLayer2Figs()) {
      fig->getBBox(box);
      odb::dbBox::create(
          _db_via, _layer2, box.xMin(), box.yMin(), box.xMax(), box.yMax());
    }
    for (auto& fig : via->getCutFigs()) {
      fig->getBBox(box);
      odb::dbBox::create(
          _db_via, _cut_layer, box.xMin(), box.yMin(), box.xMax(), box.yMax());
    }

    for (auto& fig : via->getLayer1Figs()) {
      fig->getBBox(box);
      odb::dbBox::create(
          _db_via, _layer1, box.xMin(), box.yMin(), box.xMax(), box.yMax());
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
            if (pathSeg->isTapered() || !net->getNonDefaultRule())
              _wire_encoder.newPath(layer, odb::dbWireType("ROUTED"));
            else
              _wire_encoder.newPath(
                  layer,
                  odb::dbWireType("ROUTED"),
                  net->getNonDefaultRule()->getLayerRule(layer));
            Point begin, end;
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
            if (!net->getNonDefaultRule() || via->isTapered())
              _wire_encoder.newPath(layer, odb::dbWireType("ROUTED"));
            else
              _wire_encoder.newPath(
                  layer,
                  odb::dbWireType("ROUTED"),
                  net->getNonDefaultRule()->getLayerRule(layer));
            Point origin;
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
            Point origin;
            Rect offsetBox;
            pwire->getOrigin(origin);
            pwire->getOffsetBox(offsetBox);
            _wire_encoder.addPoint(origin.x(), origin.y());
            _wire_encoder.addRect(offsetBox.xMin(),
                                  offsetBox.yMin(),
                                  offsetBox.xMax(),
                                  offsetBox.yMax());
            break;
          }
          default: {
            _wire_encoder.clear();
            logger->error(DRT,
                          114,
                          "Unknown connFig type while writing net {}.",
                          net->getName());
          }
        }
      }
      _wire_encoder.end();
    }
  }
}

void updateDbAccessPoint(odb::dbAccessPoint* db_ap,
                         frAccessPoint* ap,
                         odb::dbTech* db_tech,
                         frTechObject* tech)
{
  db_ap->setPoint(ap->getPoint());
  if (ap->hasAccess(frDirEnum::N))
    db_ap->setAccess(true, odb::dbDirection::NORTH);
  if (ap->hasAccess(frDirEnum::S))
    db_ap->setAccess(true, odb::dbDirection::SOUTH);
  if (ap->hasAccess(frDirEnum::E))
    db_ap->setAccess(true, odb::dbDirection::EAST);
  if (ap->hasAccess(frDirEnum::W))
    db_ap->setAccess(true, odb::dbDirection::WEST);
  if (ap->hasAccess(frDirEnum::U))
    db_ap->setAccess(true, odb::dbDirection::UP);
  if (ap->hasAccess(frDirEnum::D))
    db_ap->setAccess(true, odb::dbDirection::DOWN);
  auto layer = db_tech->findLayer(
      tech->getLayer(ap->getLayerNum())->getName().c_str());
  db_ap->setLayer(layer);
  db_ap->setLowType((odb::dbAccessType::Value) ap->getType(
      true));  // this works because both enums have the same order
  db_ap->setHighType((odb::dbAccessType::Value) ap->getType(false));
}

void io::Writer::updateDbAccessPoints(odb::dbBlock* block, odb::dbTech* tech)
{
  for (auto ap : block->getAccessPoints())
    odb::dbAccessPoint::destroy(ap);
  auto db = block->getDb();
  std::map<frAccessPoint*, odb::dbAccessPoint*> aps_map;
  for (auto& master : design->getMasters()) {
    auto db_master = db->findMaster(master->getName().c_str());
    if (db_master == nullptr)
      logger->error(DRT, 294, "master {} not found in db", master->getName());
    for (auto& term : master->getTerms()) {
      auto db_mterm = db_master->findMTerm(term->getName().c_str());
      if (db_mterm == nullptr)
        logger->error(DRT, 295, "mterm {} not found in db", term->getName());
      auto db_pins = db_mterm->getMPins();
      if (db_pins.size() != term->getPins().size())
        logger->error(DRT,
                      296,
                      "Mismatch in number of pins for term {}/{}",
                      master->getName(),
                      term->getName());
      frUInt4 i = 0;
      auto& pins = term->getPins();
      for (auto db_pin : db_pins) {
        auto& pin = pins[i++];
        int j = 0;
        int sz = pin->getNumPinAccess();
        while (j < sz) {
          auto pa = pin->getPinAccess(j);
          for (auto& ap : pa->getAccessPoints()) {
            auto db_ap = odb::dbAccessPoint::create(block, db_pin, j);
            updateDbAccessPoint(db_ap, ap.get(), tech, getTech());
            aps_map[ap.get()] = db_ap;
          }
          j++;
        }
      }
    }
  }
  for (auto& inst : design->getTopBlock()->getInsts()) {
    auto db_inst = block->findInst(inst->getName().c_str());
    if (db_inst == nullptr)
      logger->error(DRT, 297, "inst {} not found in db", inst->getName());
    db_inst->setPinAccessIdx(inst->getPinAccessIdx());
    for (auto& term : inst->getInstTerms()) {
      auto aps = term->getAccessPoints();
      auto db_iterm = db_inst->findITerm(term->getTerm()->getName().c_str());
      if (db_iterm == nullptr)
        logger->error(DRT, 298, "iterm {} not found in db", term->getName());
      auto db_pins = db_iterm->getMTerm()->getMPins();
      if (aps.size() != db_pins.size())
        logger->error(DRT,
                      299,
                      "Mismatch in access points size {} and term pins size {}",
                      aps.size(),
                      db_pins.size());
      frUInt4 i = 0;
      for (auto db_pin : db_pins) {
        if (aps[i] != nullptr) {
          if (aps_map.find(aps[i]) != aps_map.end())
            db_iterm->setAccessPoint(db_pin, aps_map[aps[i]]);
          else
            logger->error(DRT, 300, "Preferred access point is not found");
        } else {
          db_iterm->setAccessPoint(db_pin, nullptr);
        }
        i++;
      }
    }
  }
  for (auto& term : design->getTopBlock()->getTerms()) {
    auto db_term = block->findBTerm(term->getName().c_str());
    if (db_term == nullptr)
      logger->error(DRT, 301, "bterm {} not found in db", term->getName());
    if (db_term->getSigType() == odb::dbSigType::POWER
        || db_term->getSigType() == odb::dbSigType::GROUND
        || db_term->getSigType() == odb::dbSigType::TIEOFF)
      continue;
    auto db_pins = db_term->getBPins();
    frUInt4 i = 0;
    auto& pins = term->getPins();
    if (db_pins.size() != pins.size())
      logger->error(
          DRT, 303, "Mismatch in number of pins for bterm {}", term->getName());
    for (auto db_pin : db_pins) {
      auto& pin = pins[i++];
      int j = 0;
      int sz = pin->getNumPinAccess();
      while (j < sz) {
        auto pa = pin->getPinAccess(j);
        for (auto& ap : pa->getAccessPoints()) {
          auto db_ap = odb::dbAccessPoint::create(db_pin);
          updateDbAccessPoint(db_ap, ap.get(), tech, getTech());
        }
        j++;
      }
    }
  }
}

void io::Writer::updateDb(odb::dbDatabase* db, bool pin_access)
{
  if (db->getChip() == nullptr)
    logger->error(DRT, 3, "Load design first.");

  odb::dbBlock* block = db->getChip()->getBlock();
  odb::dbTech* tech = db->getTech();
  if (block == nullptr || tech == nullptr)
    logger->error(DRT, 4, "Load design first.");

  if (pin_access) {
    updateDbAccessPoints(block, tech);
  } else {
    fillConnFigs(false);
    fillViaDefs();
    updateDbVias(block, tech);
    updateDbConn(block, tech);
    updateDbAccessPoints(block, tech);
  }
}
