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

#include <cstddef>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "db/infra/frSegStyle.h"
#include "db/obj/frBlockage.h"
#include "db/obj/frBoundary.h"
#include "db/obj/frFig.h"
#include "db/obj/frInstBlockage.h"
#include "db/obj/frMPin.h"
#include "db/obj/frVia.h"
#include "db/tech/frConstraint.h"
#include "db/tech/frLayer.h"
#include "db/tech/frLookupTbl.h"
#include "db/tech/frTechObject.h"
#include "db/tech/frViaDef.h"
#include "frBaseTypes.h"
#include "frDesign.h"
#include "frRegionQuery.h"
#include "global.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "utl/Logger.h"

using odb::dbTechLayerDir;
using odb::dbTechLayerType;

namespace drt {

Fixture::Fixture()
    : logger(std::make_unique<utl::Logger>()),
      router_cfg(std::make_unique<RouterConfiguration>()),
      design(std::make_unique<frDesign>(logger.get(), router_cfg.get())),
      numBlockages(0),
      numTerms(0),
      numMasters(0),
      numInsts(0)
{
  makeDesign();
}

void Fixture::addLayer(frTechObject* tech,
                       const char* name,
                       dbTechLayerType type,
                       dbTechLayerDir dir)
{
  auto layer = std::make_unique<frLayer>();
  layer->setLayerNum(tech->getTopLayerNum() + 1);
  layer->setDbLayer(odb::dbTechLayer::create(db_tech, name, type));
  layer->getDbLayer()->setDirection(dir);
  layer->setWidth(100);
  layer->setMinWidth(100);
  layer->getDbLayer()->setPitch(200);

  // These constraints are mandatory
  if (type == dbTechLayerType::ROUTING) {
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

  auto db = odb::dbDatabase::create();
  db_tech = odb::dbTech::create(db, "tech");
  // TR assumes that masterslice always exists
  addLayer(tech, "masterslice", dbTechLayerType::MASTERSLICE);
  addLayer(tech, "v0", dbTechLayerType::CUT);
  addLayer(tech, "m1", dbTechLayerType::ROUTING, dbTechLayerDir::HORIZONTAL);
}

std::pair<frMaster*, odb::dbMaster*> Fixture::makeMacro(const char* name,
                                                        frCoord originX,
                                                        frCoord originY,
                                                        frCoord sizeX,
                                                        frCoord sizeY)
{
  auto block = std::make_unique<frMaster>(name);
  std::vector<frBoundary> bounds;
  frBoundary bound;
  std::vector<odb::Point> points;
  points.emplace_back(originX, originY);
  points.emplace_back(sizeX, originY);
  points.emplace_back(sizeX, sizeY);
  points.emplace_back(originX, sizeY);
  bound.setPoints(points);
  bounds.push_back(bound);
  block->setBoundaries(bounds);
  block->setMasterType(odb::dbMasterType::CORE);
  block->setId(++numMasters);
  auto blkPtr = block.get();
  design->addMaster(std::move(block));

  using odb::dbIoType;
  using odb::dbSigType;
  odb::dbMaster* master
      = odb::dbMaster::create(*db_->getLibs().begin(), "dummy");
  master->setWidth(1000);
  master->setHeight(1000);
  master->setType(odb::dbMasterType::CORE);
  odb::dbMTerm::create(master, "a", dbIoType::INPUT, dbSigType::SIGNAL);
  odb::dbMTerm::create(master, "b", dbIoType::INPUT, dbSigType::SIGNAL);
  odb::dbMTerm::create(master, "c", dbIoType::OUTPUT, dbSigType::SIGNAL);
  master->setFrozen();

  return {blkPtr, master};
}

frBlockage* Fixture::makeMacroObs(frMaster* master,
                                  frCoord xl,
                                  frCoord yl,
                                  frCoord xh,
                                  frCoord yh,
                                  frLayerNum lNum,
                                  frCoord designRuleWidth)
{
  int id = master->getBlockages().size();
  auto blkIn = std::make_unique<frBlockage>();
  blkIn->setId(id);
  blkIn->setDesignRuleWidth(designRuleWidth);
  auto pinIn = std::make_unique<frBPin>();
  pinIn->setId(0);
  // pinFig
  std::unique_ptr<frRect> pinFig = std::make_unique<frRect>();
  pinFig->setBBox(odb::Rect(xl, yl, xh, yh));
  pinFig->addToPin(pinIn.get());
  pinFig->setLayerNum(lNum);
  std::unique_ptr<frPinFig> uptr(std::move(pinFig));
  pinIn->addPinFig(std::move(uptr));
  blkIn->setPin(std::move(pinIn));
  auto blk = blkIn.get();
  master->addBlockage(std::move(blkIn));
  return blk;
}

frTerm* Fixture::makeMacroPin(frMaster* master,
                              const std::string& name,
                              frCoord xl,
                              frCoord yl,
                              frCoord xh,
                              frCoord yh,
                              frLayerNum lNum)
{
  int id = master->getTerms().size();
  std::unique_ptr<frMTerm> uTerm = std::make_unique<frMTerm>(name);
  auto term = uTerm.get();
  term->setId(id);
  master->addTerm(std::move(uTerm));
  odb::dbSigType termType = odb::dbSigType::SIGNAL;
  term->setType(termType);
  odb::dbIoType termDirection = odb::dbIoType::INPUT;
  term->setDirection(termDirection);
  auto pinIn = std::make_unique<frMPin>();
  pinIn->setId(0);
  std::unique_ptr<frRect> pinFig = std::make_unique<frRect>();
  pinFig->setBBox(odb::Rect(xl, yl, xh, yh));
  pinFig->addToPin(pinIn.get());
  pinFig->setLayerNum(lNum);
  std::unique_ptr<frPinFig> uptr(std::move(pinFig));
  pinIn->addPinFig(std::move(uptr));
  term->addPin(std::move(pinIn));
  return term;
}

frInst* Fixture::makeInst(const char* name,
                          frMaster* master,
                          odb::dbMaster* db_master)
{
  auto db_inst
      = odb::dbInst::create(db_->getChip()->getBlock(), db_master, "dummy");
  odb::dbTransform trans;
  db_inst->setTransform(trans);
  auto uInst = std::make_unique<frInst>(name, master, db_inst);
  auto tmpInst = uInst.get();
  tmpInst->setId(numInsts++);
  for (auto& uTerm : tmpInst->getMaster()->getTerms()) {
    auto term = uTerm.get();
    std::unique_ptr<frInstTerm> instTerm
        = std::make_unique<frInstTerm>(tmpInst, term);
    instTerm->setId(numTerms++);
    int pinCnt = term->getPins().size();
    instTerm->setAPSize(pinCnt);
    tmpInst->addInstTerm(std::move(instTerm));
  }
  for (auto& uBlk : tmpInst->getMaster()->getBlockages()) {
    auto blk = uBlk.get();
    std::unique_ptr<frInstBlockage> instBlk
        = std::make_unique<frInstBlockage>(tmpInst, blk);
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
  auto vssFakeNet = std::make_unique<frNet>("frFakeVSS", router_cfg.get());
  vssFakeNet->setType(odb::dbSigType::GROUND);
  vssFakeNet->setIsFake(true);
  block->addFakeSNet(std::move(vssFakeNet));

  auto vddFakeNet = std::make_unique<frNet>("frFakeVDD", router_cfg.get());
  vddFakeNet->setType(odb::dbSigType::POWER);
  vddFakeNet->setIsFake(true);
  block->addFakeSNet(std::move(vddFakeNet));

  design->setTopBlock(std::move(block));
  router_cfg->USEMINSPACING_OBS = false;

  utl::Logger* logger = new utl::Logger();
  db_ = odb::dbDatabase::create();
  db_->setLogger(logger);
  odb::dbTech* tech = odb::dbTech::create(db_, "tech");
  odb::dbTechLayer::create(tech, "L1", dbTechLayerType::MASTERSLICE);
  odb::dbLib::create(db_, "lib1", tech, ',');
  odb::dbChip* chip = odb::dbChip::create(db_, tech);
  odb::dbBlock::create(chip, "simple_block");
}

frLef58CornerSpacingConstraint* Fixture::makeCornerConstraint(
    frLayerNum layer_num,
    frCoord eolWidth,
    frCornerTypeEnum type)
{
  fr1DLookupTbl<frCoord, std::pair<frCoord, frCoord>> cornerSpacingTbl(
      "WIDTH", {0}, {{200, 200}});
  auto con = std::make_unique<frLef58CornerSpacingConstraint>(cornerSpacingTbl);
  auto rptr = con.get();
  rptr->setCornerType(type);
  rptr->setSameXY(true);
  if (eolWidth >= 0) {
    rptr->setEolWidth(eolWidth);
  }

  frTechObject* tech = design->getTech();
  frLayer* layer = tech->getLayer(layer_num);
  layer->addLef58CornerSpacingConstraint(rptr);
  tech->addUConstraint(std::move(con));
  return rptr;
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

frLef58MinStepConstraint* Fixture::makeMinStep58Constraint(frLayerNum layer_num)
{
  auto con = std::make_unique<frLef58MinStepConstraint>();

  con->setMinStepLength(50);
  con->setMaxEdges(1);
  auto rptr = con.get();
  frTechObject* tech = design->getTech();
  frLayer* layer = tech->getLayer(layer_num);
  layer->addLef58MinStepConstraint(con.get());
  tech->addUConstraint(std::move(con));
  return rptr;
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
    const std::vector<frCoord>& widthTbl,
    const std::vector<std::pair<frCoord, frCoord>>& valTbl)
{
  frTechObject* tech = design->getTech();
  frLayer* layer = tech->getLayer(layer_num);
  fr1DLookupTbl<frCoord, std::pair<frCoord, frCoord>> tbl(
      "WIDTH", widthTbl, valTbl);
  std::unique_ptr<frConstraint> uCon
      = std::make_unique<frSpacingTableInfluenceConstraint>(tbl);
  auto rptr = static_cast<frSpacingTableInfluenceConstraint*>(uCon.get());
  tech->addUConstraint(std::move(uCon));
  layer->setSpacingTableInfluence(rptr);
  return rptr;
}

frLef58EolExtensionConstraint* Fixture::makeEolExtensionConstraint(
    frLayerNum layer_num,
    frCoord spacing,
    const std::vector<frCoord>& eol,
    const std::vector<frCoord>& ext,
    bool parallelOnly)
{
  frTechObject* tech = design->getTech();
  frLayer* layer = tech->getLayer(layer_num);
  fr1DLookupTbl<frCoord, frCoord> tbl("WIDTH", eol, ext, false);
  std::unique_ptr<frLef58EolExtensionConstraint> uCon
      = std::make_unique<frLef58EolExtensionConstraint>(tbl);
  uCon->setMinSpacing(spacing);
  uCon->setParallelOnly(parallelOnly);
  auto rptr = uCon.get();
  tech->addUConstraint(std::move(uCon));
  layer->addLef58EolExtConstraint(rptr);
  return rptr;
}

frSpacingTableTwConstraint* Fixture::makeSpacingTableTwConstraint(
    frLayerNum layer_num,
    const std::vector<frCoord>& widthTbl,
    const std::vector<frCoord>& prlTbl,
    const std::vector<std::vector<frCoord>>& spacingTbl)
{
  frTechObject* tech = design->getTech();
  frLayer* layer = tech->getLayer(layer_num);
  frCollection<frSpacingTableTwRowType> rows;
  for (size_t i = 0; i < widthTbl.size(); i++) {
    rows.emplace_back(widthTbl[i], prlTbl[i]);
  }
  std::unique_ptr<frConstraint> uCon
      = std::make_unique<frSpacingTableTwConstraint>(rows, spacingTbl);
  auto rptr = static_cast<frSpacingTableTwConstraint*>(uCon.get());
  rptr->setLayer(layer);
  tech->addUConstraint(std::move(uCon));
  layer->setMinSpacing(rptr);
  return rptr;
}

frLef58WidthTableOrthConstraint* Fixture::makeWidthTblOrthConstraint(
    frLayerNum layer_num,
    frCoord horz_spc,
    frCoord vert_spc)
{
  frTechObject* tech = design->getTech();
  frLayer* layer = tech->getLayer(layer_num);
  std::unique_ptr<frConstraint> uCon
      = std::make_unique<frLef58WidthTableOrthConstraint>(horz_spc, vert_spc);
  auto rptr = static_cast<frLef58WidthTableOrthConstraint*>(uCon.get());
  tech->addUConstraint(std::move(uCon));
  layer->setWidthTblOrthCon(rptr);
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

frLef58SpacingEndOfLineConstraint* Fixture::makeLef58SpacingEolConstraint(
    frLayerNum layer_num,
    frCoord space,
    frCoord width,
    frCoord within,
    frCoord end_prl_spacing,
    frCoord end_prl)
{
  auto uCon = std::make_unique<frLef58SpacingEndOfLineConstraint>();
  auto con = uCon.get();
  con->setEol(space, width);
  auto withinCon = std::make_shared<frLef58SpacingEndOfLineWithinConstraint>();
  con->setWithinConstraint(withinCon);
  withinCon->setEolWithin(within);
  withinCon->setEndPrl(end_prl_spacing, end_prl);
  frTechObject* tech = design->getTech();
  frLayer* layer = tech->getLayer(layer_num);
  layer->addLef58SpacingEndOfLineConstraint(con);
  tech->addUConstraint(std::move(uCon));
  return con;
}

frSpacingRangeConstraint* Fixture::makeSpacingRangeConstraint(
    frLayerNum layer_num,
    frCoord spacing,
    frCoord minWidth,
    frCoord maxWidth)
{
  auto uCon = std::make_unique<frSpacingRangeConstraint>();
  auto con = uCon.get();
  con->setMinSpacing(spacing);
  con->setMinWidth(minWidth);
  con->setMaxWidth(maxWidth);
  frTechObject* tech = design->getTech();
  frLayer* layer = tech->getLayer(layer_num);
  layer->addSpacingRangeConstraint(con);
  tech->addUConstraint(std::move(uCon));
  return con;
}

std::shared_ptr<frLef58SpacingEndOfLineWithinParallelEdgeConstraint>
Fixture::makeLef58SpacingEolParEdgeConstraint(
    frLef58SpacingEndOfLineConstraint* con,
    drt::frCoord par_space,
    drt::frCoord par_within,
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
    frLef58SpacingEndOfLineConstraint* con,
    drt::frCoord min_max_length,
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
    frLef58SpacingEndOfLineConstraint* con,
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

void Fixture::makeCutClass(frLayerNum layer_num,
                           std::string name,
                           frCoord width,
                           frCoord height)
{
  auto cutClass = std::make_unique<frLef58CutClass>();
  cutClass->setName(name);
  cutClass->setViaWidth(width);
  cutClass->setViaLength(height);
  design->getTech()->addCutClass(layer_num, std::move(cutClass));
}

void Fixture::makeLef58CutSpcTbl(frLayerNum layer_num,
                                 odb::dbTechLayerCutSpacingTableDefRule* dbRule)
{
  auto con = std::make_unique<frLef58CutSpacingTableConstraint>(dbRule);
  auto layer = design->getTech()->getLayer(layer_num);
  if (dbRule->isLayerValid()) {
    if (dbRule->isSameMetal()) {
      layer->setLef58SameMetalInterCutSpcTblConstraint(con.get());
    } else if (dbRule->isSameNet()) {
      layer->setLef58SameNetInterCutSpcTblConstraint(con.get());
    } else {
      layer->setLef58DefaultInterCutSpcTblConstraint(con.get());
    }
  } else {
    if (dbRule->isSameMetal()) {
      layer->setLef58SameMetalCutSpcTblConstraint(con.get());
    } else if (dbRule->isSameNet()) {
      layer->setLef58SameNetCutSpcTblConstraint(con.get());
    } else {
      layer->setLef58DiffNetCutSpcTblConstraint(con.get());
    }
  }
  design->getTech()->addUConstraint(std::move(con));
}

void Fixture::makeLef58TwoWiresForbiddenSpc(
    frLayerNum layer_num,
    odb::dbTechLayerTwoWiresForbiddenSpcRule* dbRule)
{
  auto con = std::make_unique<frLef58TwoWiresForbiddenSpcConstraint>(dbRule);
  auto layer = design->getTech()->getLayer(layer_num);
  layer->addTwoWiresForbiddenSpacingConstraint(con.get());
  design->getTech()->addUConstraint(std::move(con));
}

void Fixture::makeLef58ForbiddenSpc(
    frLayerNum layer_num,
    odb::dbTechLayerForbiddenSpacingRule* dbRule)
{
  auto con = std::make_unique<frLef58ForbiddenSpcConstraint>(dbRule);
  auto layer = design->getTech()->getLayer(layer_num);
  layer->addForbiddenSpacingConstraint(con.get());
  design->getTech()->addUConstraint(std::move(con));
}

frLef58EnclosureConstraint* Fixture::makeLef58EnclosureConstrainut(
    frLayerNum layer_num,
    int cut_class_idx,
    frCoord width,
    frCoord firstOverhang,
    frCoord secondOverhang)
{
  auto layer = design->getTech()->getLayer(layer_num);
  auto dbRule = odb::dbTechLayerCutEnclosureRule::create(layer->getDbLayer());
  auto con = std::make_unique<frLef58EnclosureConstraint>(dbRule);
  auto rptr = con.get();
  rptr->setCutClassIdx(cut_class_idx);
  dbRule->setMinWidth(width);
  dbRule->setFirstOverhang(firstOverhang);
  dbRule->setSecondOverhang(secondOverhang);
  dbRule->setType(odb::dbTechLayerCutEnclosureRule::DEFAULT);
  layer->addLef58EnclosureConstraint(rptr);
  design->getTech()->addUConstraint(std::move(con));
  return rptr;
}

void Fixture::makeMetalWidthViaMap(frLayerNum layer_num,
                                   odb::dbMetalWidthViaMap* dbRule)
{
  auto con = std::make_unique<frMetalWidthViaConstraint>(dbRule);
  auto layer = design->getTech()->getLayer(layer_num);
  layer->addMetalWidthViaConstraint(con.get());
  design->getTech()->addUConstraint(std::move(con));
}

void Fixture::makeKeepOutZoneRule(frLayerNum layer_num,
                                  odb::dbTechLayerKeepOutZoneRule* dbRule)
{
  auto con = std::make_unique<frLef58KeepOutZoneConstraint>(dbRule);
  auto layer = design->getTech()->getLayer(layer_num);
  layer->addKeepOutZoneConstraint(con.get());
  design->getTech()->addUConstraint(std::move(con));
}

frLef58CutSpacingConstraint*
Fixture::makeLef58CutSpacingConstraint_parallelOverlap(frLayerNum layer_num,
                                                       frCoord spacing)
{
  frTechObject* tech = design->getTech();
  frLayer* layer = tech->getLayer(layer_num);

  auto uCon = std::make_unique<frLef58CutSpacingConstraint>();
  auto con = uCon.get();
  con->setCutSpacing(spacing);
  con->setParallelOverlap(true);
  layer->addLef58CutSpacingConstraint(con);
  design->getTech()->addUConstraint(std::move(uCon));
  return con;
}

frLef58CutSpacingConstraint* Fixture::makeLef58CutSpacingConstraint_adjacentCut(
    frLayerNum layer_num,
    frCoord spacing,
    int adjacent_cuts,
    int two_cuts,
    frCoord within)
{
  frTechObject* tech = design->getTech();
  frLayer* layer = tech->getLayer(layer_num);
  auto rule = odb::dbTechLayerCutSpacingRule::create(layer->getDbLayer());
  rule->setCutSpacing(spacing);
  rule->setAdjacentCuts(adjacent_cuts);
  // con->setTwoCuts(two_cuts);
  rule->setWithin(within);
  rule->setCenterToCenter(true);

  auto uCon = std::make_unique<frLef58CutSpacingConstraint>();
  auto con = uCon.get();
  con->setCutClassIdx(0);
  con->setCutSpacing(rule->getCutSpacing());
  con->setAdjacentCuts(rule->getAdjacentCuts());
  con->setCutWithin(rule->getWithin());
  con->setCenterToCenter(rule->isCenterToCenter());
  layer->addLef58CutSpacingConstraint(con);
  design->getTech()->addUConstraint(std::move(uCon));
  return con;
}

void Fixture::makeMinimumCut(frLayerNum layerNum,
                             frCoord width,
                             frCoord length,
                             frCoord distance,
                             frMinimumcutConnectionEnum connection)
{
  auto con = std::make_unique<frMinimumcutConstraint>();
  auto layer = design->getTech()->getLayer(layerNum);
  auto rptr = con.get();
  con->setWidth(width);
  con->setLength(length, distance);
  con->setConnection(connection);
  design->getTech()->addUConstraint(std::move(con));
  layer->addMinimumcutConstraint(rptr);
}

frNet* Fixture::makeNet(const char* name)
{
  frBlock* block = design->getTopBlock();
  auto net_p = std::make_unique<frNet>(name, router_cfg.get());
  frNet* net = net_p.get();
  block->addNet(std::move(net_p));
  return net;
}

frViaDef* Fixture::makeViaDef(const char* name,
                              frLayerNum layer_num,
                              const odb::Point& ll,
                              const odb::Point& ur)
{
  auto tech = design->getTech();
  auto via_p = std::make_unique<frViaDef>(name);
  for (frLayerNum l = layer_num - 1; l <= layer_num + 1; l++) {
    std::unique_ptr<frRect> pinFig = std::make_unique<frRect>();
    pinFig->setBBox(odb::Rect(ll, ur));
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
      default:
        logger->error(DRT, 624, "Unexpected layer diff {}", l - layer_num);
    }
  }

  return tech->addVia(std::move(via_p));
}

frVia* Fixture::makeVia(frViaDef* viaDef, frNet* net, const odb::Point& origin)
{
  auto via_p = std::make_unique<frVia>(viaDef, origin);
  via_p->addToNet(net);
  frVia* via = via_p.get();
  net->addVia(std::move(via_p));
  return via;
}

void Fixture::makePathseg(frNet* net,
                          frLayerNum layer_num,
                          const odb::Point& begin,
                          const odb::Point& end,
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

void Fixture::makeLef58WrongDirSpcConstraint(
    frLayerNum layer_num,
    odb::dbTechLayerWrongDirSpacingRule* dbRule)
{
  auto con = std::make_unique<frLef58SpacingWrongDirConstraint>(dbRule);
  auto layer = design->getTech()->getLayer(layer_num);
  layer->addLef58SpacingWrongDirConstraint(con.get());
  design->getTech()->addUConstraint(std::move(con));
}

void Fixture::makeSpacingTableOrthConstraint(frLayerNum layer_num,
                                             frCoord within,
                                             frCoord spc)
{
  std::vector<std::pair<frCoord, frCoord>> spc_tbl = {{within, spc}};
  auto con = std::make_unique<frOrthSpacingTableConstraint>(spc_tbl);
  auto layer = design->getTech()->getLayer(layer_num);
  layer->setOrthSpacingTableConstraint(con.get());
  design->getTech()->addUConstraint(std::move(con));
}

}  // namespace drt
