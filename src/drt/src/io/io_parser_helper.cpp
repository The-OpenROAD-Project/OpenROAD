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

#include <boost/graph/connected_components.hpp>
#include <boost/polygon/polygon.hpp>
#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>

#include "frBaseTypes.h"
#include "frProfileTask.h"
#include "global.h"
#include "io/io.h"

using namespace std;
using namespace fr;
using namespace boost::polygon::operators;

using Rectangle = boost::polygon::rectangle_data<int>;

void io::Parser::initDefaultVias()
{
  for (auto& uViaDef : tech_->getVias()) {
    auto viaDef = uViaDef.get();
    tech_->getLayer(viaDef->getCutLayerNum())->addViaDef(viaDef);
  }
  for (auto& userDefinedVia : design_->getUserSelectedVias()) {
    if (tech_->name2via.find(userDefinedVia) == tech_->name2via.end()) {
      logger_->error(
          DRT, 608, "Could not find user defined via {}", userDefinedVia);
    }
    auto viaDef = tech_->getVia(userDefinedVia);
    tech_->getLayer(viaDef->getCutLayerNum())->setDefaultViaDef(viaDef);
  }
  std::map<frLayerNum, std::map<int, std::map<viaRawPriorityTuple, frViaDef*>>>
      layerNum2ViaDefs;
  for (auto layerNum = design_->getTech()->getBottomLayerNum();
       layerNum <= design_->getTech()->getTopLayerNum();
       ++layerNum) {
    auto layer = design_->getTech()->getLayer(layerNum);
    if (layer->getType() != dbTechLayerType::CUT) {
      continue;
    }
    if (layer->getDefaultViaDef() != nullptr) {
      continue;
    }
    for (auto& viaDef : layer->getViaDefs()) {
      int cutNum = int(viaDef->getCutFigs().size());
      viaRawPriorityTuple priority;
      getViaRawPriority(viaDef, priority);
      layerNum2ViaDefs[layerNum][cutNum][priority] = viaDef;
    }
    if (!layerNum2ViaDefs[layerNum][1].empty()) {
      auto defaultSingleCutVia
          = (layerNum2ViaDefs[layerNum][1].begin())->second;
      tech_->getLayer(layerNum)->setDefaultViaDef(defaultSingleCutVia);
    } else {
      if (layerNum >= BOTTOM_ROUTING_LAYER) {
        logger_->error(DRT,
                       234,
                       "{} does not have single-cut via.",
                       tech_->getLayer(layerNum)->getName());
      }
    }
    // generate via if default via enclosure is not along pref dir
    if (ENABLE_VIA_GEN && layerNum >= BOTTOM_ROUTING_LAYER) {
      auto techDefautlViaDef = tech_->getLayer(layerNum)->getDefaultViaDef();
      frVia via(techDefautlViaDef);
      Rect layer1Box = via.getLayer1BBox();
      Rect layer2Box = via.getLayer2BBox();
      frLayerNum layer1Num = techDefautlViaDef->getLayer1Num();
      frLayerNum layer2Num = techDefautlViaDef->getLayer2Num();
      bool isLayer1Square = (layer1Box.xMax() - layer1Box.xMin())
                            == (layer1Box.yMax() - layer1Box.yMin());
      bool isLayer2Square = (layer2Box.xMax() - layer2Box.xMin())
                            == (layer2Box.yMax() - layer2Box.yMin());
      bool isLayer1EncHorz = (layer1Box.xMax() - layer1Box.xMin())
                             > (layer1Box.yMax() - layer1Box.yMin());
      bool isLayer2EncHorz = (layer2Box.xMax() - layer2Box.xMin())
                             > (layer2Box.yMax() - layer2Box.yMin());
      bool isLayer1Horz = (tech_->getLayer(layer1Num)->getDir()
                           == dbTechLayerDir::HORIZONTAL);
      bool isLayer2Horz = (tech_->getLayer(layer2Num)->getDir()
                           == dbTechLayerDir::HORIZONTAL);
      bool needViaGen = false;
      if ((!isLayer1Square && (isLayer1EncHorz != isLayer1Horz))
          || (!isLayer2Square && (isLayer2EncHorz != isLayer2Horz))) {
        needViaGen = true;
      }

      // generate new via def if needed
      if (needViaGen) {
        string viaDefName
            = tech_->getLayer(techDefautlViaDef->getCutLayerNum())->getName();
        viaDefName += string("_FR");
        logger_->warn(DRT,
                      160,
                      "Warning: {} does not have viaDef aligned with layer "
                      "direction, generating new viaDef {}.",
                      tech_->getLayer(layer1Num)->getName(),
                      viaDefName);
        // routing layer shape
        // rotate if needed
        if (isLayer1EncHorz != isLayer1Horz) {
          layer1Box.init(layer1Box.yMin(),
                         layer1Box.xMin(),
                         layer1Box.yMax(),
                         layer1Box.xMax());
        }
        if (isLayer2EncHorz != isLayer2Horz) {
          layer2Box.init(layer2Box.yMin(),
                         layer2Box.xMin(),
                         layer2Box.yMax(),
                         layer2Box.xMax());
        }

        unique_ptr<frShape> uBotFig = make_unique<frRect>();
        auto botFig = static_cast<frRect*>(uBotFig.get());
        unique_ptr<frShape> uTopFig = make_unique<frRect>();
        auto topFig = static_cast<frRect*>(uTopFig.get());

        botFig->setBBox(layer1Box);
        topFig->setBBox(layer2Box);
        botFig->setLayerNum(layer1Num);
        topFig->setLayerNum(layer2Num);

        // cut layer shape
        unique_ptr<frShape> uCutFig = make_unique<frRect>();
        auto cutFig = static_cast<frRect*>(uCutFig.get());
        Rect cutBox = via.getCutBBox();
        cutFig->setBBox(cutBox);
        cutFig->setLayerNum(techDefautlViaDef->getCutLayerNum());

        // create via
        auto viaDef = make_unique<frViaDef>(viaDefName);
        viaDef->addLayer1Fig(std::move(uBotFig));
        viaDef->addLayer2Fig(std::move(uTopFig));
        viaDef->addCutFig(std::move(uCutFig));
        viaDef->setAddedByRouter(true);
        tech_->getLayer(layerNum)->setDefaultViaDef(viaDef.get());
        tech_->addVia(std::move(viaDef));
      }
    }
  }
}

// initialize secondLayerNum for rules that apply, reset the samenet rule if
// corresponding diffnet rule does not exist
void io::Parser::initConstraintLayerIdx()
{
  for (auto layerNum = design_->getTech()->getBottomLayerNum();
       layerNum <= design_->getTech()->getTopLayerNum();
       ++layerNum) {
    auto layer = design_->getTech()->getLayer(layerNum);
    // non-LEF58
    auto& interLayerCutSpacingConstraints
        = layer->getInterLayerCutSpacingConstraintRef(false);
    // diff-net
    if (interLayerCutSpacingConstraints.empty()) {
      interLayerCutSpacingConstraints.resize(
          design_->getTech()->getTopLayerNum() + 1, nullptr);
    }
    for (auto& [secondLayerName, con] :
         layer->getInterLayerCutSpacingConstraintMap(false)) {
      auto secondLayer = design_->getTech()->getLayer(secondLayerName);
      if (secondLayer == nullptr) {
        logger_->warn(
            DRT, 235, "Second layer {} does not exist.", secondLayerName);
        continue;
      }
      auto secondLayerNum
          = design_->getTech()->getLayer(secondLayerName)->getLayerNum();
      con->setSecondLayerNum(secondLayerNum);
      logger_->info(DRT,
                    236,
                    "Updating diff-net cut spacing rule between {} and {}.",
                    design_->getTech()->getLayer(layerNum)->getName(),
                    design_->getTech()->getLayer(secondLayerNum)->getName());
      interLayerCutSpacingConstraints[secondLayerNum] = con;
    }
    // same-net
    auto& interLayerCutSpacingSamenetConstraints
        = layer->getInterLayerCutSpacingConstraintRef(true);
    if (interLayerCutSpacingSamenetConstraints.empty()) {
      interLayerCutSpacingSamenetConstraints.resize(
          design_->getTech()->getTopLayerNum() + 1, nullptr);
    }
    for (auto& [secondLayerName, con] :
         layer->getInterLayerCutSpacingConstraintMap(true)) {
      auto secondLayer = design_->getTech()->getLayer(secondLayerName);
      if (secondLayer == nullptr) {
        logger_->warn(
            DRT, 237, "Second layer {} does not exist.", secondLayerName);
        continue;
      }
      auto secondLayerNum
          = design_->getTech()->getLayer(secondLayerName)->getLayerNum();
      con->setSecondLayerNum(secondLayerNum);
      logger_->info(DRT,
                    238,
                    "Updating same-net cut spacing rule between {} and {}.",
                    design_->getTech()->getLayer(layerNum)->getName(),
                    design_->getTech()->getLayer(secondLayerNum)->getName());
      interLayerCutSpacingSamenetConstraints[secondLayerNum] = con;
    }
    // reset same-net if diff-net does not exist
    for (int i = 0; i < (int) interLayerCutSpacingConstraints.size(); i++) {
      if (interLayerCutSpacingConstraints[i] == nullptr) {
        // cout << i << endl << flush;
        interLayerCutSpacingSamenetConstraints[i] = nullptr;
      } else {
        interLayerCutSpacingConstraints[i]->setSameNetConstraint(
            interLayerCutSpacingSamenetConstraints[i]);
      }
    }
    // LEF58
    // diff-net
    for (auto& con : layer->getLef58CutSpacingConstraints(false)) {
      if (con->hasSecondLayer()) {
        frLayerNum secondLayerNum = design_->getTech()
                                        ->getLayer(con->getSecondLayerName())
                                        ->getLayerNum();
        con->setSecondLayerNum(secondLayerNum);
      }
    }
    // same-net
    for (auto& con : layer->getLef58CutSpacingConstraints(true)) {
      if (con->hasSecondLayer()) {
        frLayerNum secondLayerNum = design_->getTech()
                                        ->getLayer(con->getSecondLayerName())
                                        ->getLayerNum();
        con->setSecondLayerNum(secondLayerNum);
      }
    }
  }
}

// initialize cut layer width for cut OBS DRC check if not specified in LEF
void io::Parser::initCutLayerWidth()
{
  for (auto layerNum = design_->getTech()->getBottomLayerNum();
       layerNum <= design_->getTech()->getTopLayerNum();
       ++layerNum) {
    if (design_->getTech()->getLayer(layerNum)->getType()
        != dbTechLayerType::CUT) {
      continue;
    }
    auto layer = design_->getTech()->getLayer(layerNum);
    // update cut layer width is not specifed in LEF
    if (layer->getWidth() == 0) {
      // first check default via size, if it is square, use that size
      auto viaDef = layer->getDefaultViaDef();
      if (viaDef) {
        auto cutFig = viaDef->getCutFigs()[0].get();
        if (cutFig->typeId() != frcRect) {
          logger_->error(DRT, 239, "Non-rectangular shape in via definition.");
        }
        auto cutRect = static_cast<frRect*>(cutFig);
        auto viaWidth = cutRect->width();
        layer->setWidth(viaWidth);
        if (viaDef->getNumCut() == 1) {
          if (cutRect->width() != cutRect->length()) {
            logger_->warn(DRT,
                          240,
                          "CUT layer {} does not have square single-cut via, "
                          "cut layer width may be set incorrectly.",
                          layer->getName());
          }
        } else {
          logger_->warn(DRT,
                        241,
                        "CUT layer {} does not have single-cut via, cut layer "
                        "width may be set incorrectly.",
                        layer->getName());
        }
      } else {
        if (layerNum >= BOTTOM_ROUTING_LAYER) {
          logger_->error(DRT,
                         242,
                         "CUT layer {} does not have default via.",
                         layer->getName());
        }
      }
    } else {
      auto viaDef = layer->getDefaultViaDef();
      int cutLayerWidth = layer->getWidth();
      if (viaDef) {
        auto cutFig = viaDef->getCutFigs()[0].get();
        if (cutFig->typeId() != frcRect) {
          logger_->error(DRT, 243, "Non-rectangular shape in via definition.");
        }
        auto cutRect = static_cast<frRect*>(cutFig);
        int viaWidth = cutRect->width();
        if (cutLayerWidth < viaWidth) {
          logger_->warn(
              DRT,
              244,
              "CUT layer {} has smaller width defined in LEF compared "
              "to default via.",
              layer->getName());
        }
      }
    }
  }
}

void io::Parser::getViaRawPriority(frViaDef* viaDef,
                                   viaRawPriorityTuple& priority)
{
  bool isNotDefaultVia = !(viaDef->getDefault());
  bool isNotUpperAlign = false;
  bool isNotLowerAlign = false;
  using PolygonSet = std::vector<boost::polygon::polygon_90_data<int>>;
  PolygonSet viaLayerPS1;

  for (auto& fig : viaDef->getLayer1Figs()) {
    Rect bbox = fig->getBBox();
    Rectangle bboxRect(bbox.xMin(), bbox.yMin(), bbox.xMax(), bbox.yMax());
    viaLayerPS1 += bboxRect;
  }
  Rectangle layer1Rect;
  extents(layer1Rect, viaLayerPS1);
  bool isLayer1Horz
      = (xh(layer1Rect) - xl(layer1Rect)) > (yh(layer1Rect) - yl(layer1Rect));
  frCoord layer1Width = std::min((xh(layer1Rect) - xl(layer1Rect)),
                                 (yh(layer1Rect) - yl(layer1Rect)));
  isNotLowerAlign = (isLayer1Horz
                     && (tech_->getLayer(viaDef->getLayer1Num())->getDir()
                         == dbTechLayerDir::VERTICAL))
                    || (!isLayer1Horz
                        && (tech_->getLayer(viaDef->getLayer1Num())->getDir()
                            == dbTechLayerDir::HORIZONTAL));

  PolygonSet viaLayerPS2;
  for (auto& fig : viaDef->getLayer2Figs()) {
    Rect bbox = fig->getBBox();
    Rectangle bboxRect(bbox.xMin(), bbox.yMin(), bbox.xMax(), bbox.yMax());
    viaLayerPS2 += bboxRect;
  }
  Rectangle layer2Rect;
  extents(layer2Rect, viaLayerPS2);
  bool isLayer2Horz
      = (xh(layer2Rect) - xl(layer2Rect)) > (yh(layer2Rect) - yl(layer2Rect));
  frCoord layer2Width = std::min((xh(layer2Rect) - xl(layer2Rect)),
                                 (yh(layer2Rect) - yl(layer2Rect)));
  isNotUpperAlign = (isLayer2Horz
                     && (tech_->getLayer(viaDef->getLayer2Num())->getDir()
                         == dbTechLayerDir::VERTICAL))
                    || (!isLayer2Horz
                        && (tech_->getLayer(viaDef->getLayer2Num())->getDir()
                            == dbTechLayerDir::HORIZONTAL));

  frCoord layer1Area = area(viaLayerPS1);
  frCoord layer2Area = area(viaLayerPS2);

  priority = std::make_tuple(isNotDefaultVia,
                             layer1Width,
                             layer2Width,
                             isNotUpperAlign,
                             layer2Area,
                             layer1Area,
                             isNotLowerAlign,
                             viaDef->getName());
}

// 13M_3Mx_2Cx_4Kx_2Hx_2Gx_LB
void io::Parser::initDefaultVias_GF14(const string& node)
{
  for (int layerNum = 1; layerNum < (int) tech_->getLayers().size();
       layerNum += 2) {
    for (auto& uViaDef : tech_->getVias()) {
      auto viaDef = uViaDef.get();
      if (viaDef->getCutLayerNum() == layerNum
          && node == "GF14_13M_3Mx_2Cx_4Kx_2Hx_2Gx_LB") {
        switch (layerNum) {
          case 3:  // VIA1
            if (viaDef->getName() == "V1_0_15_0_25_VH_Vx") {
              tech_->getLayer(layerNum)->setDefaultViaDef(viaDef);
            }
            break;
          case 5:  // VIA2
            if (viaDef->getName() == "V2_0_25_0_25_HV_Vx") {
              tech_->getLayer(layerNum)->setDefaultViaDef(viaDef);
            }
            break;
          case 7:  // VIA3
            if (viaDef->getName() == "J3_0_25_4_40_VH_Jy") {
              tech_->getLayer(layerNum)->setDefaultViaDef(viaDef);
            }
            break;
          case 9:  // VIA4
            if (viaDef->getName() == "A4_0_50_0_50_HV_Ax") {
              tech_->getLayer(layerNum)->setDefaultViaDef(viaDef);
            }
            break;
          case 11:  // VIA5
            if (viaDef->getName() == "CK_23_28_0_26_VH_CK") {
              tech_->getLayer(layerNum)->setDefaultViaDef(viaDef);
            }
            break;
          case 13:  // VIA6
            if (viaDef->getName() == "U1_0_26_0_26_HV_Ux") {
              tech_->getLayer(layerNum)->setDefaultViaDef(viaDef);
            }
            break;
          case 15:  // VIA7
            if (viaDef->getName() == "U2_0_26_0_26_VH_Ux") {
              tech_->getLayer(layerNum)->setDefaultViaDef(viaDef);
            }
            break;
          case 17:  // VIA8
            if (viaDef->getName() == "U3_0_26_0_26_HV_Ux") {
              tech_->getLayer(layerNum)->setDefaultViaDef(viaDef);
            }
            break;
          case 19:  // VIA9
            if (viaDef->getName() == "KH_18_45_0_45_VH_KH") {
              tech_->getLayer(layerNum)->setDefaultViaDef(viaDef);
            }
            break;
          case 21:  // VIA10
            if (viaDef->getName() == "N1_0_45_0_45_HV_Nx") {
              tech_->getLayer(layerNum)->setDefaultViaDef(viaDef);
            }
            break;
          case 23:  // VIA11
            if (viaDef->getName() == "HG_18_72_18_72_VH_HG") {
              tech_->getLayer(layerNum)->setDefaultViaDef(viaDef);
            }
            break;
          case 25:  // VIA12
            if (viaDef->getName() == "T1_18_72_18_72_HV_Tx") {
              tech_->getLayer(layerNum)->setDefaultViaDef(viaDef);
            }
            break;
          case 27:  // VIA13
            if (viaDef->getName() == "VV_450_450_450_450_XX_VV") {
              tech_->getLayer(layerNum)->setDefaultViaDef(viaDef);
            }
            break;
          default:;
        }
      }
    }
  }
}

void io::Parser::convertLef58MinCutConstraints()
{
  auto bottomLayerNum = tech_->getBottomLayerNum();
  auto topLayerNum = tech_->getTopLayerNum();
  for (auto lNum = bottomLayerNum; lNum <= topLayerNum; lNum++) {
    frLayer* layer = tech_->getLayer(lNum);
    if (layer->getType() != dbTechLayerType::ROUTING)
      continue;
    if (!layer->hasLef58Minimumcut())
      continue;
    for (auto con : layer->getLef58MinimumcutConstraints()) {
      auto dbRule = con->getODBRule();
      unique_ptr<frConstraint> uCon = make_unique<frMinimumcutConstraint>();
      auto rptr = static_cast<frMinimumcutConstraint*>(uCon.get());
      if (dbRule->isPerCutClass()) {
        frViaDef* viaDefBelow = nullptr;
        if (lNum > bottomLayerNum)
          viaDefBelow = tech_->getLayer(lNum - 1)->getDefaultViaDef();
        frViaDef* viaDefAbove = nullptr;
        if (lNum < topLayerNum)
          viaDefAbove = tech_->getLayer(lNum + 1)->getDefaultViaDef();
        bool found = false;
        rptr->setNumCuts(dbRule->getNumCuts());
        for (auto [cutclass, numcuts] : dbRule->getCutClassCutsMap()) {
          if (viaDefBelow && !dbRule->isFromAbove()
              && viaDefBelow->getCutClass()->getName() == cutclass) {
            found = true;
            rptr->setNumCuts(numcuts);
            break;
          }
          if (viaDefAbove && !dbRule->isFromBelow()
              && viaDefAbove->getCutClass()->getName() == cutclass) {
            found = true;

            rptr->setNumCuts(numcuts);
            break;
          }
        }
        if (!found)
          continue;
      }

      if (dbRule->isLengthValid()) {
        MTSAFEDIST = std::max(MTSAFEDIST, dbRule->getLengthWithinDist());
        rptr->setLength(dbRule->getLength(), dbRule->getLengthWithinDist());
      }
      rptr->setWidth(dbRule->getWidth());
      if (dbRule->isWithinCutDistValid())
        rptr->setWithin(dbRule->getWithinCutDist());
      if (dbRule->isFromAbove())
        rptr->setConnection(frMinimumcutConnectionEnum::FROMABOVE);
      if (dbRule->isFromBelow())
        rptr->setConnection(frMinimumcutConnectionEnum::FROMBELOW);
      tech_->addUConstraint(std::move(uCon));
      layer->addMinimumcutConstraint(rptr);
    }
  }
}

void io::Parser::postProcess()
{
  initDefaultVias();
  if (DBPROCESSNODE == "GF14_13M_3Mx_2Cx_4Kx_2Hx_2Gx_LB") {
    initDefaultVias_GF14(DBPROCESSNODE);
  }
  initCutLayerWidth();
  initConstraintLayerIdx();
  tech_->printDefaultVias(logger_);
  instAnalysis();
  convertLef58MinCutConstraints();
  // init region query
  logger_->info(DRT, 168, "Init region query.");
  design_->getRegionQuery()->init();
  design_->getRegionQuery()->print();
  design_->getRegionQuery()->initDRObj();  // second init from FlexDR.cpp
}

void io::Parser::postProcessGuide()
{
  if (tmpGuides_.empty())
    return;
  ProfileTask profile("IO:postProcessGuide");
  if (VERBOSE > 0) {
    logger_->info(DRT, 169, "Post process guides.");
  }
  buildGCellPatterns(db_);

  design_->getRegionQuery()->initOrigGuide(tmpGuides_);
  int cnt = 0;
  // for (auto &[netName, rects]:tmpGuides) {
  //   if (design_->getTopBlock()->name2net.find(netName) ==
  //   design_->getTopBlock()->name2net.end()) {
  //     cout <<"Error: postProcessGuide cannot find net" <<endl;
  //     exit(1);
  //   }
  for (auto& [net, rects] : tmpGuides_) {
    net->setOrigGuides(rects);
    genGuides(net, rects);
    cnt++;
    if (VERBOSE > 0) {
      if (cnt < 100000) {
        if (cnt % 10000 == 0) {
          logger_->report("  complete {} nets.", cnt);
        }
      } else {
        if (cnt % 100000 == 0) {
          logger_->report("  complete {} nets.", cnt);
        }
      }
    }
  }

  // global unique id for guides
  int currId = 0;
  for (auto& net : design_->getTopBlock()->getNets()) {
    for (auto& guide : net->getGuides()) {
      guide->setId(currId);
      currId++;
    }
  }

  logger_->info(DRT, 178, "Init guide query.");
  design_->getRegionQuery()->initGuide();
  design_->getRegionQuery()->printGuide();
  logger_->info(DRT, 179, "Init gr pin query.");
  design_->getRegionQuery()->initGRPin(tmpGRPins_);

  if (!SAVE_GUIDE_UPDATES) {
    if (VERBOSE > 0) {
      logger_->info(DRT, 245, "skipped writing guide updates to database.");
    }
  } else {
    saveGuidesUpdates();
  }
}

// instantiate RPin and region query for RPin
void io::Parser::initRPin()
{
  if (VERBOSE > 0) {
    logger_->info(DRT, 185, "Post process initialize RPin region query.");
  }
  initRPin_rpin();
  initRPin_rq();
}

void io::Parser::initRPin_rpin()
{
  for (auto& net : design_->getTopBlock()->getNets()) {
    // instTerm
    for (auto& instTerm : net->getInstTerms()) {
      auto inst = instTerm->getInst();
      int pinIdx = 0;
      auto trueTerm = instTerm->getTerm();
      for (auto& pin : trueTerm->getPins()) {
        auto rpin = make_unique<frRPin>();
        rpin->setFrTerm(instTerm);
        rpin->addToNet(net.get());
        frAccessPoint* prefAp = (instTerm->getAccessPoints())[pinIdx];

        // MACRO does not go through PA
        if (prefAp == nullptr) {
          dbMasterType masterType = inst->getMaster()->getMasterType();
          if (masterType.isBlock() || masterType.isPad()
              || masterType == dbMasterType::RING) {
            prefAp = (pin->getPinAccess(inst->getPinAccessIdx())
                          ->getAccessPoints())[0]
                         .get();
          } else
            continue;
        }

        if (prefAp == nullptr) {
          logger_->warn(DRT,
                        246,
                        "{}/{} from {} has nullptr as prefAP.",
                        instTerm->getInst()->getName(),
                        trueTerm->getName(),
                        net->getName());
        }

        rpin->setAccessPoint(prefAp);

        net->addRPin(rpin);

        pinIdx++;
      }
    }
    // term
    for (auto& term : net->getBTerms()) {
      auto trueTerm = term;
      for (auto& pin : trueTerm->getPins()) {
        auto rpin = make_unique<frRPin>();
        rpin->setFrTerm(term);
        rpin->addToNet(net.get());
        frAccessPoint* prefAp
            = (pin->getPinAccess(0)->getAccessPoints())[0].get();
        rpin->setAccessPoint(prefAp);

        net->addRPin(rpin);
      }
    }
  }
}

void io::Parser::initRPin_rq()
{
  design_->getRegionQuery()->initRPin();
}

void io::Parser::buildGCellPatterns_helper(frCoord& GCELLGRIDX,
                                           frCoord& GCELLGRIDY,
                                           frCoord& GCELLOFFSETX,
                                           frCoord& GCELLOFFSETY)
{
  buildGCellPatterns_getWidth(GCELLGRIDX, GCELLGRIDY);
  buildGCellPatterns_getOffset(
      GCELLGRIDX, GCELLGRIDY, GCELLOFFSETX, GCELLOFFSETY);
}

void io::Parser::buildGCellPatterns_getWidth(frCoord& GCELLGRIDX,
                                             frCoord& GCELLGRIDY)
{
  map<frCoord, int> guideGridXMap, guideGridYMap;
  // get GCell size information loop
  for (auto& [netName, rects] : tmpGuides_) {
    for (auto& rect : rects) {
      frLayerNum layerNum = rect.getLayerNum();
      Rect guideBBox = rect.getBBox();
      frCoord guideWidth
          = (tech_->getLayer(layerNum)->getDir() == dbTechLayerDir::HORIZONTAL)
                ? (guideBBox.yMax() - guideBBox.yMin())
                : (guideBBox.xMax() - guideBBox.xMin());
      if (tech_->getLayer(layerNum)->getDir() == dbTechLayerDir::HORIZONTAL) {
        if (guideGridYMap.find(guideWidth) == guideGridYMap.end()) {
          guideGridYMap[guideWidth] = 0;
        }
        guideGridYMap[guideWidth]++;
      } else if (tech_->getLayer(layerNum)->getDir()
                 == dbTechLayerDir::VERTICAL) {
        if (guideGridXMap.find(guideWidth) == guideGridXMap.end()) {
          guideGridXMap[guideWidth] = 0;
        }
        guideGridXMap[guideWidth]++;
      }
    }
  }
  frCoord tmpGCELLGRIDX = -1, tmpGCELLGRIDY = -1;
  int tmpGCELLGRIDXCnt = -1, tmpGCELLGRIDYCnt = -1;
  for (auto mapIt = guideGridXMap.begin(); mapIt != guideGridXMap.end();
       ++mapIt) {
    auto cnt = mapIt->second;
    if (cnt > tmpGCELLGRIDXCnt) {
      tmpGCELLGRIDXCnt = cnt;
      tmpGCELLGRIDX = mapIt->first;
    }
  }
  for (auto mapIt = guideGridYMap.begin(); mapIt != guideGridYMap.end();
       ++mapIt) {
    auto cnt = mapIt->second;
    if (cnt > tmpGCELLGRIDYCnt) {
      tmpGCELLGRIDYCnt = cnt;
      tmpGCELLGRIDY = mapIt->first;
    }
  }
  if (tmpGCELLGRIDX != -1) {
    GCELLGRIDX = tmpGCELLGRIDX;
  } else {
    logger_->error(DRT, 170, "No GCELLGRIDX.");
  }
  if (tmpGCELLGRIDY != -1) {
    GCELLGRIDY = tmpGCELLGRIDY;
  } else {
    logger_->error(DRT, 171, "No GCELLGRIDY.");
  }
}

void io::Parser::buildGCellPatterns_getOffset(frCoord GCELLGRIDX,
                                              frCoord GCELLGRIDY,
                                              frCoord& GCELLOFFSETX,
                                              frCoord& GCELLOFFSETY)
{
  std::map<frCoord, int> guideOffsetXMap, guideOffsetYMap;
  // get GCell offset information loop
  for (auto& [netName, rects] : tmpGuides_) {
    for (auto& rect : rects) {
      // frLayerNum layerNum = rect.getLayerNum();
      Rect guideBBox = rect.getBBox();
      frCoord guideXOffset = guideBBox.xMin() % GCELLGRIDX;
      frCoord guideYOffset = guideBBox.yMin() % GCELLGRIDY;
      if (guideXOffset < 0) {
        guideXOffset = GCELLGRIDX - guideXOffset;
      }
      if (guideYOffset < 0) {
        guideYOffset = GCELLGRIDY - guideYOffset;
      }
      if (guideOffsetXMap.find(guideXOffset) == guideOffsetXMap.end()) {
        guideOffsetXMap[guideXOffset] = 0;
      }
      guideOffsetXMap[guideXOffset]++;
      if (guideOffsetYMap.find(guideYOffset) == guideOffsetYMap.end()) {
        guideOffsetYMap[guideYOffset] = 0;
      }
      guideOffsetYMap[guideYOffset]++;
    }
  }
  frCoord tmpGCELLOFFSETX = -1, tmpGCELLOFFSETY = -1;
  int tmpGCELLOFFSETXCnt = -1, tmpGCELLOFFSETYCnt = -1;
  for (auto mapIt = guideOffsetXMap.begin(); mapIt != guideOffsetXMap.end();
       ++mapIt) {
    auto cnt = mapIt->second;
    if (cnt > tmpGCELLOFFSETXCnt) {
      tmpGCELLOFFSETXCnt = cnt;
      tmpGCELLOFFSETX = mapIt->first;
    }
  }
  for (auto mapIt = guideOffsetYMap.begin(); mapIt != guideOffsetYMap.end();
       ++mapIt) {
    auto cnt = mapIt->second;
    if (cnt > tmpGCELLOFFSETYCnt) {
      tmpGCELLOFFSETYCnt = cnt;
      tmpGCELLOFFSETY = mapIt->first;
    }
  }
  if (tmpGCELLOFFSETX != -1) {
    GCELLOFFSETX = tmpGCELLOFFSETX;
  } else {
    logger_->error(DRT, 172, "No GCELLGRIDX.");
  }
  if (tmpGCELLOFFSETY != -1) {
    GCELLOFFSETY = tmpGCELLOFFSETY;
  } else {
    logger_->error(DRT, 173, "No GCELLGRIDY.");
  }
}

void io::Parser::buildGCellPatterns(odb::dbDatabase* db)
{
  // horizontal = false is gcell lines along y direction (x-grid)
  frGCellPattern xgp, ygp;
  frCoord GCELLOFFSETX, GCELLOFFSETY, GCELLGRIDX, GCELLGRIDY;
  auto gcellGrid = db->getChip()->getBlock()->getGCellGrid();
  if (gcellGrid != nullptr && gcellGrid->getNumGridPatternsX() == 1
      && gcellGrid->getNumGridPatternsY() == 1) {
    frCoord COUNTX, COUNTY;
    gcellGrid->getGridPatternX(0, GCELLOFFSETX, COUNTX, GCELLGRIDX);
    gcellGrid->getGridPatternY(0, GCELLOFFSETY, COUNTY, GCELLGRIDY);
    xgp.setStartCoord(GCELLOFFSETX);
    xgp.setSpacing(GCELLGRIDX);
    xgp.setCount(COUNTX);
    xgp.setHorizontal(false);

    ygp.setStartCoord(GCELLOFFSETY);
    ygp.setSpacing(GCELLGRIDY);
    ygp.setCount(COUNTY);
    ygp.setHorizontal(true);

  } else {
    Rect dieBox = design_->getTopBlock()->getDieBox();
    buildGCellPatterns_helper(
        GCELLGRIDX, GCELLGRIDY, GCELLOFFSETX, GCELLOFFSETY);
    xgp.setHorizontal(false);
    // find first coord >= dieBox.xMin()
    frCoord startCoordX
        = dieBox.xMin() / (frCoord) GCELLGRIDX * (frCoord) GCELLGRIDX
          + GCELLOFFSETX;
    if (startCoordX > dieBox.xMin()) {
      startCoordX -= (frCoord) GCELLGRIDX;
    }
    xgp.setStartCoord(startCoordX);
    xgp.setSpacing(GCELLGRIDX);
    if ((dieBox.xMax() - (frCoord) GCELLOFFSETX) / (frCoord) GCELLGRIDX < 1) {
      logger_->error(DRT, 174, "GCell cnt x < 1.");
    }
    xgp.setCount((dieBox.xMax() - (frCoord) startCoordX)
                 / (frCoord) GCELLGRIDX);

    ygp.setHorizontal(true);
    // find first coord >= dieBox.yMin()
    frCoord startCoordY
        = dieBox.yMin() / (frCoord) GCELLGRIDY * (frCoord) GCELLGRIDY
          + GCELLOFFSETY;
    if (startCoordY > dieBox.yMin()) {
      startCoordY -= (frCoord) GCELLGRIDY;
    }
    ygp.setStartCoord(startCoordY);
    ygp.setSpacing(GCELLGRIDY);
    if ((dieBox.yMax() - (frCoord) GCELLOFFSETY) / (frCoord) GCELLGRIDY < 1) {
      logger_->error(DRT, 175, "GCell cnt y < 1.");
    }
    ygp.setCount((dieBox.yMax() - startCoordY) / (frCoord) GCELLGRIDY);
  }

  if (VERBOSE > 0 || logger_->debugCheck(DRT, "autotuner", 1)) {
    logger_->info(DRT,
                  176,
                  "GCELLGRID X {} DO {} STEP {} ;",
                  xgp.getStartCoord(),
                  xgp.getCount(),
                  xgp.getSpacing());
    logger_->info(DRT,
                  177,
                  "GCELLGRID Y {} DO {} STEP {} ;",
                  ygp.getStartCoord(),
                  ygp.getCount(),
                  ygp.getSpacing());
  }

  design_->getTopBlock()->setGCellPatterns({xgp, ygp});

  for (int layerNum = 0; layerNum <= (int) tech_->getLayers().size();
       layerNum += 2) {
    for (int i = 0; i < (int) xgp.getCount(); i++) {
      for (int j = 0; j < (int) ygp.getCount(); j++) {
        Rect gcellBox = design_->getTopBlock()->getGCellBox(Point(i, j));
        bool isH = (tech_->getLayers().at(layerNum)->getDir()
                    == dbTechLayerDir::HORIZONTAL);
        frCoord gcLow = isH ? gcellBox.yMin() : gcellBox.xMax();
        frCoord gcHigh = isH ? gcellBox.yMax() : gcellBox.xMin();
        for (auto& tp : design_->getTopBlock()->getTrackPatterns(layerNum)) {
          if ((tech_->getLayer(layerNum)->getDir() == dbTechLayerDir::HORIZONTAL
               && tp->isHorizontal() == false)
              || (tech_->getLayer(layerNum)->getDir()
                      == dbTechLayerDir::VERTICAL
                  && tp->isHorizontal() == true)) {
            int trackNum
                = (gcLow - tp->getStartCoord()) / (int) tp->getTrackSpacing();
            if (trackNum < 0) {
              trackNum = 0;
            }
            if (trackNum * (int) tp->getTrackSpacing() + tp->getStartCoord()
                < gcLow) {
              trackNum++;
            }
            for (;
                 trackNum < (int) tp->getNumTracks()
                 && trackNum * (int) tp->getTrackSpacing() + tp->getStartCoord()
                        < gcHigh;
                 trackNum++) {
            }
          }
        }
      }
    }
  }
}

void io::Parser::saveGuidesUpdates()
{
  auto block = db_->getChip()->getBlock();
  auto dbTech = db_->getTech();
  for (auto& net : design_->topBlock_->getNets()) {
    auto dbNet = block->findNet(net->getName().c_str());
    dbNet->clearGuides();
    for (auto& guide : net->getGuides()) {
      auto [bp, ep] = guide->getPoints();
      Point bpIdx = design_->getTopBlock()->getGCellIdx(bp);
      Point epIdx = design_->getTopBlock()->getGCellIdx(ep);
      Rect bbox = design_->getTopBlock()->getGCellBox(bpIdx);
      Rect ebox = design_->getTopBlock()->getGCellBox(epIdx);
      frLayerNum bNum = guide->getBeginLayerNum();
      frLayerNum eNum = guide->getEndLayerNum();
      if (bNum != eNum) {
        for (auto lNum = min(bNum, eNum); lNum <= max(bNum, eNum); lNum += 2) {
          auto layer = tech_->getLayer(lNum);
          auto dbLayer = dbTech->findLayer(layer->getName().c_str());
          odb::dbGuide::create(dbNet, dbLayer, bbox);
        }
      } else {
        auto layerName = tech_->getLayer(bNum)->getName();
        auto dbLayer = dbTech->findLayer(layerName.c_str());
        odb::dbGuide::create(
            dbNet,
            dbLayer,
            {bbox.xMin(), bbox.yMin(), ebox.xMax(), ebox.yMax()});
      }
    }
    auto dbGuides = dbNet->getGuides();
    if (dbGuides.orderReversed() && dbGuides.reversible())
      dbGuides.reverse();
  }
}
