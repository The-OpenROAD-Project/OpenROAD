/* Author: Matt Liberty */
/*
 * Copyright (c) 2020, The Regents of the University of California
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

#include "fixture.h"

#include <stdexcept>

using namespace fr;

Fixture::Fixture()
    : logger(std::make_unique<Logger>()),
      design(std::make_unique<frDesign>(logger.get())),
      numBlockages(0),
      numTerms(0),
      numRefBlocks(0),
      numInsts(0)
{
  makeDesign();
}

void Fixture::addLayer(frTechObject* tech,
                       const char* name,
                       frLayerTypeEnum type,
                       frPrefRoutingDirEnum dir)
{
  auto layer = std::make_unique<frLayer>();
  layer->setLayerNum(tech->getTopLayerNum() + 1);
  layer->setName(name);
  layer->setType(type);
  layer->setDir(dir);

  layer->setWidth(100);
  layer->setMinWidth(100);
  layer->setPitch(200);

  // These constraints are mandatory
  if (type == frLayerTypeEnum::ROUTING) {
    auto minWidthConstraint
        = std::make_unique<frMinWidthConstraint>(layer->getMinWidth());
    layer->setMinWidthConstraint(minWidthConstraint.get());
    tech->addUConstraint(std::move(minWidthConstraint));

    auto offGridConstraint = std::make_unique<frOffGridConstraint>();
    layer->setOffGridConstraint(offGridConstraint.get());
    tech->addUConstraint(std::move(offGridConstraint));

    auto nsmetalConstraint = std::make_unique<frNonSufficientMetalConstraint>();
    layer->setNonSufficientMetalConstraint(nsmetalConstraint.get());
    tech->addUConstraint(std::move(nsmetalConstraint));
  }

  auto shortConstraint = std::make_unique<frShortConstraint>();
  layer->setShortConstraint(shortConstraint.get());
  tech->addUConstraint(std::move(shortConstraint));

  tech->addLayer(std::move(layer));
}

void Fixture::setupTech(frTechObject* tech)
{
  tech->setManufacturingGrid(10);
  tech->setDBUPerUU(1000);

  // TR assumes that masterslice always exists
  addLayer(tech, "masterslice", frLayerTypeEnum::MASTERSLICE);
  addLayer(tech, "v0", frLayerTypeEnum::CUT);
  addLayer(tech, "m1", frLayerTypeEnum::ROUTING);
}

frBlock* Fixture::makeMacro(const char* name,
                            frCoord originX,
                            frCoord originY,
                            frCoord sizeX,
                            frCoord sizeY)
{
  auto block = make_unique<frBlock>(name);
  vector<frBoundary> bounds;
  frBoundary bound;
  vector<frPoint> points;
  points.push_back(frPoint(originX, originY));
  points.push_back(frPoint(sizeX, originY));
  points.push_back(frPoint(sizeX, sizeY));
  points.push_back(frPoint(originX, sizeY));
  bound.setPoints(points);
  bounds.push_back(bound);
  block->setBoundaries(bounds);
  block->setMacroClass(MacroClassEnum::CORE);
  block->setId(++numRefBlocks);
  auto blkPtr = block.get();
  design->addRefBlock(std::move(block));
  return blkPtr;
}

frBlockage* Fixture::makeMacroObs(frBlock* refBlock,
                                  frCoord xl,
                                  frCoord yl,
                                  frCoord xh,
                                  frCoord yh,
                                  frLayerNum lNum,
                                  frCoord designRuleWidth)
{
  int id = refBlock->getBlockages().size();
  auto blkIn = make_unique<frBlockage>();
  blkIn->setId(id);
  blkIn->setDesignRuleWidth(designRuleWidth);
  auto pinIn = make_unique<frPin>();
  pinIn->setId(0);
  // pinFig
  unique_ptr<frRect> pinFig = make_unique<frRect>();
  pinFig->setBBox(frBox(xl, yl, xh, yh));
  pinFig->addToPin(pinIn.get());
  pinFig->setLayerNum(lNum);
  unique_ptr<frPinFig> uptr(std::move(pinFig));
  pinIn->addPinFig(std::move(uptr));
  blkIn->setPin(std::move(pinIn));
  auto blk = blkIn.get();
  refBlock->addBlockage(std::move(blkIn));
  return blk;
}

frTerm* Fixture::makeMacroPin(frBlock* refBlock,
                              std::string name,
                              frCoord xl,
                              frCoord yl,
                              frCoord xh,
                              frCoord yh,
                              frLayerNum lNum)
{
  int id = refBlock->getTerms().size();
  unique_ptr<frTerm> uTerm = make_unique<frTerm>(name);
  auto term = uTerm.get();
  term->setId(id);
  refBlock->addTerm(std::move(uTerm));
  frTermEnum termType = frTermEnum::frcNormalTerm;
  term->setType(termType);
  frTermDirectionEnum termDirection = frTermDirectionEnum::INPUT;
  term->setDirection(termDirection);
  auto pinIn = make_unique<frPin>();
  pinIn->setId(0);
  unique_ptr<frRect> pinFig = make_unique<frRect>();
  pinFig->setBBox(frBox(xl, yl, xh, yh));
  pinFig->addToPin(pinIn.get());
  pinFig->setLayerNum(lNum);
  unique_ptr<frPinFig> uptr(std::move(pinFig));
  pinIn->addPinFig(std::move(uptr));
  term->addPin(std::move(pinIn));
  return term;
}

frInst* Fixture::makeInst(const char* name,
                          frBlock* refBlock,
                          frCoord x,
                          frCoord y)
{
  auto uInst = make_unique<frInst>(name, refBlock);
  auto tmpInst = uInst.get();
  tmpInst->setId(numInsts++);
  tmpInst->setOrigin(frPoint(x, y));
  tmpInst->setOrient(frOrientEnum::frcR0);
  for (auto& uTerm : tmpInst->getRefBlock()->getTerms()) {
    auto term = uTerm.get();
    unique_ptr<frInstTerm> instTerm = make_unique<frInstTerm>(tmpInst, term);
    instTerm->setId(numTerms++);
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
  design->getTopBlock()->addInst(std::move(uInst));
  return tmpInst;
}

void Fixture::makeDesign()
{
  setupTech(design->getTech());

  auto block = std::make_unique<frBlock>("test");

  // GC assumes these fake nets exist
  auto vssFakeNet = std::make_unique<frNet>("frFakeVSS");
  vssFakeNet->setType(frNetEnum::frcGroundNet);
  vssFakeNet->setIsFake(true);
  block->addFakeSNet(std::move(vssFakeNet));

  auto vddFakeNet = std::make_unique<frNet>("frFakeVDD");
  vddFakeNet->setType(frNetEnum::frcPowerNet);
  vddFakeNet->setIsFake(true);
  block->addFakeSNet(std::move(vddFakeNet));

  design->setTopBlock(std::move(block));
  USEMINSPACING_OBS = false;
}

void Fixture::makeCornerConstraint(frLayerNum layer_num,
                                   frCoord eolWidth,
                                   frCornerTypeEnum type)
{
  fr1DLookupTbl<frCoord, std::pair<frCoord, frCoord>> cornerSpacingTbl(
      "WIDTH", {0}, {{200, 200}});
  auto con = std::make_unique<frLef58CornerSpacingConstraint>(cornerSpacingTbl);

  con->setCornerType(type);
  con->setSameXY(true);
  if (eolWidth >= 0) {
    con->setEolWidth(eolWidth);
  }

  frTechObject* tech = design->getTech();
  frLayer* layer = tech->getLayer(layer_num);
  layer->addLef58CornerSpacingConstraint(con.get());
  tech->addUConstraint(std::move(con));
}

void Fixture::makeSpacingConstraint(frLayerNum layer_num)
{
  fr2DLookupTbl<frCoord, frCoord, frCoord> tbl("WIDTH",
                                               {0, 200},
                                               "PARALLELRUNLENGTH",
                                               {0, 400},
                                               {{100, 200}, {300, 400}});
  auto con = std::make_unique<frSpacingTablePrlConstraint>(tbl);

  frTechObject* tech = design->getTech();
  frLayer* layer = tech->getLayer(layer_num);
  layer->setMinSpacing(con.get());
  tech->addUConstraint(std::move(con));
}

void Fixture::makeMinStepConstraint(frLayerNum layer_num)
{
  auto con = std::make_unique<frMinStepConstraint>();

  con->setMinstepType(frMinstepTypeEnum::STEP);
  con->setMinStepLength(50);

  frTechObject* tech = design->getTech();
  frLayer* layer = tech->getLayer(layer_num);
  layer->setMinStepConstraint(con.get());
  tech->addUConstraint(std::move(con));
}

void Fixture::makeMinStep58Constraint(frLayerNum layer_num)
{
  auto con = std::make_unique<frLef58MinStepConstraint>();

  con->setMinStepLength(50);
  con->setMaxEdges(1);
  con->setEolWidth(200);

  frTechObject* tech = design->getTech();
  frLayer* layer = tech->getLayer(layer_num);
  layer->addLef58MinStepConstraint(con.get());
  tech->addUConstraint(std::move(con));
}

void Fixture::makeRectOnlyConstraint(frLayerNum layer_num)
{
  auto con = std::make_unique<frLef58RectOnlyConstraint>();

  frTechObject* tech = design->getTech();
  frLayer* layer = tech->getLayer(layer_num);
  layer->setLef58RectOnlyConstraint(con.get());
  tech->addUConstraint(std::move(con));
}

void Fixture::makeMinEnclosedAreaConstraint(frLayerNum layer_num)
{
  auto con = std::make_unique<frMinEnclosedAreaConstraint>(200 * 200);

  frTechObject* tech = design->getTech();
  frLayer* layer = tech->getLayer(layer_num);
  layer->addMinEnclosedAreaConstraint(con.get());
  tech->addUConstraint(std::move(con));
}

void Fixture::makeSpacingEndOfLineConstraint(frLayerNum layer_num,
                                             frCoord par_space,
                                             frCoord par_within,
                                             bool two_edges)
{
  auto con = std::make_unique<frSpacingEndOfLineConstraint>();

  con->setMinSpacing(200);
  con->setEolWidth(200);
  con->setEolWithin(50);

  if (par_space != -1) {
    if (par_within == -1) {
      throw std::invalid_argument("Must give par_within with par_space");
    }
    con->setParSpace(par_space);
    con->setParWithin(par_within);
    con->setTwoEdges(two_edges);
  }

  frTechObject* tech = design->getTech();
  frLayer* layer = tech->getLayer(layer_num);
  layer->addEolSpacing(con.get());
  tech->addUConstraint(std::move(con));
}

frSpacingTableInfluenceConstraint* Fixture::makeSpacingTableInfluenceConstraint(
    frLayerNum layer_num,
    std::vector<frCoord> widthTbl,
    std::vector<std::pair<frCoord, frCoord>> valTbl)
{
  frTechObject* tech = design->getTech();
  frLayer* layer = tech->getLayer(layer_num);
  fr1DLookupTbl<frCoord, std::pair<frCoord, frCoord>> tbl(
      "WIDTH", widthTbl, valTbl);
  unique_ptr<frConstraint> uCon
      = make_unique<frSpacingTableInfluenceConstraint>(tbl);
  auto rptr = static_cast<frSpacingTableInfluenceConstraint*>(uCon.get());
  tech->addUConstraint(std::move(uCon));
  layer->setSpacingTableInfluence(rptr);
  return rptr;
}

frLef58EolExtensionConstraint* Fixture::makeEolExtensionConstraint(
    frLayerNum layer_num,
    frCoord spacing,
    std::vector<frCoord> eol,
    std::vector<frCoord> ext,
    bool parallelOnly)
{
  frTechObject* tech = design->getTech();
  frLayer* layer = tech->getLayer(layer_num);
  fr1DLookupTbl<frCoord, frCoord> tbl("WIDTH", eol, ext, false);
  unique_ptr<frLef58EolExtensionConstraint> uCon
      = make_unique<frLef58EolExtensionConstraint>(tbl);
  uCon->setMinSpacing(spacing);
  uCon->setParallelOnly(parallelOnly);
  auto rptr = uCon.get();
  tech->addUConstraint(std::move(uCon));
  layer->addLef58EolExtConstraint(rptr);
  return rptr;
}

frSpacingTableTwConstraint* Fixture::makeSpacingTableTwConstraint(
    frLayerNum layer_num,
    std::vector<frCoord> widthTbl,
    std::vector<frCoord> prlTbl,
    std::vector<std::vector<frCoord>> spacingTbl)
{
  frTechObject* tech = design->getTech();
  frLayer* layer = tech->getLayer(layer_num);
  frCollection<frSpacingTableTwRowType> rows;
  for (size_t i = 0; i < widthTbl.size(); i++) {
    rows.push_back(frSpacingTableTwRowType(widthTbl[i], prlTbl[i]));
  }
  unique_ptr<frConstraint> uCon
      = make_unique<frSpacingTableTwConstraint>(rows, spacingTbl);
  auto rptr = static_cast<frSpacingTableTwConstraint*>(uCon.get());
  tech->addUConstraint(std::move(uCon));
  layer->setMinSpacing(rptr);
  return rptr;
}

void Fixture::makeLef58EolKeepOutConstraint(frLayerNum layer_num,
                                            bool cornerOnly,
                                            bool exceptWithin,
                                            frCoord withinLow,
                                            frCoord withinHigh,
                                            frCoord forward,
                                            frCoord side,
                                            frCoord backward,
                                            frCoord width)
{
  frTechObject* tech = design->getTech();
  frLayer* layer = tech->getLayer(layer_num);
  auto con = std::make_unique<frLef58EolKeepOutConstraint>();
  auto rptr = con.get();
  rptr->setEolWidth(width);
  rptr->setForwardExt(forward);
  rptr->setBackwardExt(backward);
  rptr->setSideExt(side);
  rptr->setCornerOnly(cornerOnly);
  rptr->setExceptWithin(exceptWithin);
  rptr->setWithinLow(withinLow);
  rptr->setWithinHigh(withinHigh);
  layer->addLef58EolKeepOutConstraint(rptr);
  tech->addUConstraint(std::move(con));
}

std::shared_ptr<frLef58SpacingEndOfLineConstraint>
Fixture::makeLef58SpacingEolConstraint(frLayerNum layer_num,
                                       frCoord space,
                                       frCoord width,
                                       frCoord within)
{
  auto con = std::make_shared<frLef58SpacingEndOfLineConstraint>();
  con->setEol(space, width);
  auto withinCon = std::make_shared<frLef58SpacingEndOfLineWithinConstraint>();
  con->setWithinConstraint(withinCon);
  withinCon->setEolWithin(within);
  frTechObject* tech = design->getTech();
  frLayer* layer = tech->getLayer(layer_num);
  layer->addLef58SpacingEndOfLineConstraint(con);
  tech->addConstraint(con);
  return con;
}

std::shared_ptr<frLef58SpacingEndOfLineWithinParallelEdgeConstraint>
Fixture::makeLef58SpacingEolParEdgeConstraint(
    std::shared_ptr<frLef58SpacingEndOfLineConstraint> con,
    fr::frCoord par_space,
    fr::frCoord par_within,
    bool two_edges)
{
  auto parallelEdge
      = std::make_shared<frLef58SpacingEndOfLineWithinParallelEdgeConstraint>();
  con->getWithinConstraint()->setParallelEdgeConstraint(parallelEdge);
  parallelEdge->setPar(par_space, par_within);
  parallelEdge->setTwoEdges(two_edges);
  return parallelEdge;
}

std::shared_ptr<frLef58SpacingEndOfLineWithinMaxMinLengthConstraint>
Fixture::makeLef58SpacingEolMinMaxLenConstraint(
    std::shared_ptr<frLef58SpacingEndOfLineConstraint> con,
    fr::frCoord min_max_length,
    bool max,
    bool two_sides)
{
  auto minMax
      = std::make_shared<frLef58SpacingEndOfLineWithinMaxMinLengthConstraint>();
  con->getWithinConstraint()->setMaxMinLengthConstraint(minMax);
  minMax->setLength(max, min_max_length, two_sides);
  return minMax;
}

std::shared_ptr<frLef58SpacingEndOfLineWithinEncloseCutConstraint>
Fixture::makeLef58SpacingEolCutEncloseConstraint(
    std::shared_ptr<frLef58SpacingEndOfLineConstraint> con,
    frCoord encloseDist,
    frCoord cutToMetalSpacing,
    bool above,
    bool below,
    bool allCuts)
{
  auto cutEnc
      = std::make_shared<frLef58SpacingEndOfLineWithinEncloseCutConstraint>(
          encloseDist, cutToMetalSpacing);
  con->getWithinConstraint()->setEncloseCutConstraint(cutEnc);
  cutEnc->setAbove(above);
  cutEnc->setBelow(below);
  cutEnc->setAllCuts(allCuts);
  return cutEnc;
}

frNet* Fixture::makeNet(const char* name)
{
  frBlock* block = design->getTopBlock();
  auto net_p = std::make_unique<frNet>(name);
  frNet* net = net_p.get();
  block->addNet(std::move(net_p));
  return net;
}

frViaDef* Fixture::makeViaDef(const char* name,
                              frLayerNum layer_num,
                              const frPoint& ll,
                              const frPoint& ur)
{
  auto tech = design->getTech();
  auto via_p = std::make_unique<frViaDef>(name);
  for (frLayerNum l = layer_num - 1; l <= layer_num + 1; l++) {
    unique_ptr<frRect> pinFig = make_unique<frRect>();
    pinFig->setBBox(frBox(ll, ur));
    pinFig->setLayerNum(l);
    switch (l - layer_num) {
      case -1:
        via_p->addLayer1Fig(std::move(pinFig));
        break;
      case 0:
        via_p->addCutFig(std::move(pinFig));
        break;
      case 1:
        via_p->addLayer2Fig(std::move(pinFig));
        break;
    }
  }

  frViaDef* via = via_p.get();
  tech->addVia(std::move(via_p));
  return via;
}

frVia* Fixture::makeVia(frViaDef* viaDef, frNet* net, const frPoint& origin)
{
  auto via_p = make_unique<frVia>(viaDef);
  via_p->setOrigin(origin);
  via_p->addToNet(net);
  frVia* via = via_p.get();
  net->addVia(std::move(via_p));
  return via;
}

void Fixture::makePathseg(frNet* net,
                          frLayerNum layer_num,
                          const frPoint& begin,
                          const frPoint& end,
                          frUInt4 width,
                          frEndStyleEnum begin_style,
                          frEndStyleEnum end_style)
{
  auto ps = std::make_unique<frPathSeg>();
  ps->setPoints(begin, end);
  ps->setLayerNum(layer_num);

  if (begin_style == frcVariableEndStyle || end_style == frcVariableEndStyle) {
    throw std::invalid_argument("frcVariableEndStyle not supported");
  }

  frCoord begin_ext = begin_style == frcExtendEndStyle ? width / 2 : 0;
  frCoord end_ext = begin_style == frcExtendEndStyle ? width / 2 : 0;

  frSegStyle style;
  style.setWidth(width);
  style.setBeginStyle(begin_style, begin_ext);
  style.setEndStyle(end_style, end_ext);

  ps->setStyle(style);
  net->addShape(std::move(ps));
}

void Fixture::initRegionQuery()
{
  frRegionQuery* query = design->getRegionQuery();
  query->init();
  query->initDRObj();
}
