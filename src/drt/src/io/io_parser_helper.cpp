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
#include <set>
#include <sstream>

#include "frBaseTypes.h"
#include "frProfileTask.h"
#include "global.h"
#include "io/io.h"

namespace drt {

using Rectangle = boost::polygon::rectangle_data<int>;
namespace gtl = boost::polygon;

int io::Parser::getTopPinLayer()
{
  frLayerNum topPinLayer = 0;
  if (getBlock()) {
    for (const auto& bTerm : getBlock()->getTerms()) {
      if (bTerm->getNet() && !bTerm->getNet()->isSpecial()) {
        for (const auto& pin : bTerm->getPins()) {
          for (const auto& fig : pin->getFigs()) {
            topPinLayer = std::max(topPinLayer,
                                   ((frShape*) (fig.get()))->getLayerNum());
          }
        }
      }
    }
    for (const auto& inst : getBlock()->getInsts()) {
      for (const auto& iTerm : inst->getInstTerms()) {
        if (iTerm->getNet() && !iTerm->getNet()->isSpecial()) {
          for (const auto& pin : iTerm->getTerm()->getPins()) {
            for (const auto& fig : pin->getFigs()) {
              topPinLayer = std::max(topPinLayer,
                                     ((frShape*) (fig.get()))->getLayerNum());
            }
          }
        }
      }
    }
  }
  return topPinLayer;
}

void io::Parser::initDefaultVias()
{
  for (auto& uViaDef : getTech()->getVias()) {
    auto viaDef = uViaDef.get();
    getTech()->getLayer(viaDef->getCutLayerNum())->addViaDef(viaDef);
  }
  for (auto& userDefinedVia : getDesign()->getUserSelectedVias()) {
    if (getTech()->name2via_.find(userDefinedVia)
        == getTech()->name2via_.end()) {
      logger_->error(
          DRT, 608, "Could not find user defined via {}", userDefinedVia);
    }
    auto viaDef = getTech()->getVia(userDefinedVia);
    getTech()->getLayer(viaDef->getCutLayerNum())->setDefaultViaDef(viaDef);
  }
  // Check whether there are pins above top routing layer
  frLayerNum topPinLayer = getTopPinLayer();

  for (auto layerNum = getTech()->getBottomLayerNum();
       layerNum <= getTech()->getTopLayerNum();
       ++layerNum) {
    auto layer = getTech()->getLayer(layerNum);
    if (layer->getType() != dbTechLayerType::CUT) {
      continue;
    }
    if (layer->getDefaultViaDef() != nullptr) {
      continue;
    }
    // Check whether viaDefs set is empty
    std::set<frViaDef*> viaDefs = layer->getViaDefs();
    if (!viaDefs.empty()) {
      std::map<int, std::map<viaRawPriorityTuple, frViaDef*>> cuts2ViaDefs;
      for (auto& viaDef : viaDefs) {
        int cutNum = int(viaDef->getCutFigs().size());
        viaRawPriorityTuple priority;
        getViaRawPriority(viaDef, priority);
        cuts2ViaDefs[cutNum][priority] = viaDef;
      }
      auto iter_1cut = cuts2ViaDefs.find(1);
      if (iter_1cut != cuts2ViaDefs.end() && !iter_1cut->second.empty()) {
        auto defaultSingleCutVia = iter_1cut->second.begin()->second;
        getTech()->getLayer(layerNum)->setDefaultViaDef(defaultSingleCutVia);
      } else if (layerNum > router_cfg_->TOP_ROUTING_LAYER) {
        // We may need vias here to stack up to bumps.  However there
        // may not be a single cut via.  Since we aren't routing, but
        // just stacking, we'll use the best via we can find.
        auto via_map = cuts2ViaDefs.begin()->second;
        getTech()->getLayer(layerNum)->setDefaultViaDef(
            via_map.begin()->second);
      } else if (layerNum >= router_cfg_->BOTTOM_ROUTING_LAYER) {
        logger_->error(DRT,
                       234,
                       "{} does not have single-cut via.",
                       getTech()->getLayer(layerNum)->getName());
      }
    } else {
      if (layerNum >= router_cfg_->BOTTOM_ROUTING_LAYER
          && (layerNum
              <= std::max(router_cfg_->TOP_ROUTING_LAYER, topPinLayer))) {
        logger_->error(DRT,
                       233,
                       "{} does not have any vias.",
                       getTech()->getLayer(layerNum)->getName());
      }
    }
    // generate via if default via enclosure is not along pref dir
    if (router_cfg_->ENABLE_VIA_GEN
        && layerNum >= router_cfg_->BOTTOM_ROUTING_LAYER
        && layerNum <= router_cfg_->TOP_ROUTING_LAYER) {
      auto techDefautlViaDef
          = getTech()->getLayer(layerNum)->getDefaultViaDef();
      frVia via(techDefautlViaDef);
      Rect layer1Box = via.getLayer1BBox();
      Rect layer2Box = via.getLayer2BBox();
      frLayerNum layer1Num = techDefautlViaDef->getLayer1Num();
      frLayerNum layer2Num = techDefautlViaDef->getLayer2Num();
      bool isLayer1Square = layer1Box.dx() == layer1Box.dy();
      bool isLayer2Square = layer2Box.dx() == layer2Box.dy();
      bool isLayer1EncHorz = layer1Box.dx() > layer1Box.dy();
      bool isLayer2EncHorz = layer2Box.dx() > layer2Box.dy();
      bool isLayer1Horz = (getTech()->getLayer(layer1Num)->getDir()
                           == dbTechLayerDir::HORIZONTAL);
      bool isLayer2Horz = (getTech()->getLayer(layer2Num)->getDir()
                           == dbTechLayerDir::HORIZONTAL);
      bool needViaGen = false;
      if ((!isLayer1Square && (isLayer1EncHorz != isLayer1Horz))
          || (!isLayer2Square && (isLayer2EncHorz != isLayer2Horz))) {
        needViaGen = true;
      }

      // generate new via def if needed
      if (needViaGen) {
        std::string viaDefName
            = getTech()
                  ->getLayer(techDefautlViaDef->getCutLayerNum())
                  ->getName();
        viaDefName += std::string("_FR");
        logger_->warn(DRT,
                      160,
                      "Warning: {} does not have viaDef aligned with layer "
                      "direction, generating new viaDef {}.",
                      getTech()->getLayer(layer1Num)->getName(),
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

        std::unique_ptr<frShape> uBotFig = std::make_unique<frRect>();
        auto botFig = static_cast<frRect*>(uBotFig.get());
        std::unique_ptr<frShape> uTopFig = std::make_unique<frRect>();
        auto topFig = static_cast<frRect*>(uTopFig.get());

        botFig->setBBox(layer1Box);
        topFig->setBBox(layer2Box);
        botFig->setLayerNum(layer1Num);
        topFig->setLayerNum(layer2Num);

        // cut layer shape
        std::unique_ptr<frShape> uCutFig = std::make_unique<frRect>();
        auto cutFig = static_cast<frRect*>(uCutFig.get());
        Rect cutBox = via.getCutBBox();
        cutFig->setBBox(cutBox);
        cutFig->setLayerNum(techDefautlViaDef->getCutLayerNum());

        // create via
        auto viaDef = std::make_unique<frViaDef>(viaDefName);
        viaDef->addLayer1Fig(std::move(uBotFig));
        viaDef->addLayer2Fig(std::move(uTopFig));
        viaDef->addCutFig(std::move(uCutFig));
        viaDef->setAddedByRouter(true);
        auto vdfPtr = getTech()->addVia(std::move(viaDef));
        if (vdfPtr == nullptr) {
          logger_->error(
              utl::DRT, 336, "Duplicated via definition for {}", viaDefName);
        }
        getTech()->getLayer(layerNum)->setDefaultViaDef(vdfPtr);
      }
    }
  }
}

namespace {
std::pair<frCoord, frCoord> getBloatingDist(frTechObject* tech,
                                            const frVia& via,
                                            bool above)
{
  auto cut_layer = tech->getLayer(via.getViaDef()->getCutLayerNum());
  auto enc_box = above ? via.getLayer2BBox() : via.getLayer1BBox();
  auto cut_box = via.getCutBBox();
  frCoord horz_overhang = 0;
  frCoord vert_overhang = 0;
  horz_overhang = std::min(enc_box.xMax() - cut_box.xMax(),
                           cut_box.xMin() - enc_box.xMin());
  vert_overhang = std::min(enc_box.yMax() - cut_box.yMax(),
                           cut_box.yMin() - enc_box.yMin());
  bool eol_is_horz = enc_box.dx() < enc_box.dy();
  for (auto con : cut_layer->getLef58EnclosureConstraints(
           via.getViaDef()->getCutClassIdx(), 0, above, true)) {
    if (!con->isEolOnly()) {
      break;
    }
    if ((eol_is_horz ? enc_box.dx() : enc_box.dy()) > con->getEolLength()) {
      continue;
    }
    std::pair<frCoord, frCoord> bloats;
    if (eol_is_horz) {
      bloats = {std::max(0, con->getFirstOverhang() - vert_overhang),
                std::max(0, con->getSecondOverhang() - horz_overhang)};
    } else {
      bloats = {std::max(0, con->getSecondOverhang() - vert_overhang),
                std::max(0, con->getFirstOverhang() - horz_overhang)};
    }
    return bloats;
  }
  return std::make_pair(0, 0);
}

}  // namespace

void io::Parser::initSecondaryVias()
{
  for (auto layerNum = getTech()->getBottomLayerNum();
       layerNum <= getTech()->getTopLayerNum();
       ++layerNum) {
    auto layer = getTech()->getLayer(layerNum);
    if (layer->getType() != dbTechLayerType::CUT) {
      continue;
    }
    const auto default_viadef = layer->getDefaultViaDef();
    const bool has_default_viadef = default_viadef != nullptr;
    const bool has_max_spacing_constraints
        = layer->hasLef58MaxSpacingConstraints();
    if (!has_default_viadef || !has_max_spacing_constraints) {
      continue;
    }
    std::set<frViaDef*> viadefs = layer->getViaDefs();
    if (!viadefs.empty()) {
      std::map<int, std::map<viaRawPriorityTuple, frViaDef*>> cuts_to_viadefs;
      for (auto& viadef : viadefs) {
        int cut_num = int(viadef->getCutFigs().size());
        viaRawPriorityTuple priority;
        getViaRawPriority(viadef, priority);
        cuts_to_viadefs[cut_num][priority] = viadef;
      }
      for (auto [cuts, viadefs] : cuts_to_viadefs) {
        for (auto [priority, viadef] : viadefs) {
          if (viadef->getCutClassIdx() == default_viadef->getCutClassIdx()) {
            continue;
          }
          frVia secondary_via(viadef);
          auto layer1_bloats = getBloatingDist(getTech(), secondary_via, false);
          auto layer2_bloats = getBloatingDist(getTech(), secondary_via, true);
          int dx = secondary_via.getCutBBox().xCenter();
          int dy = secondary_via.getCutBBox().yCenter();
          if (layer1_bloats != std::pair<int, int>(0, 0)
              || layer2_bloats != std::pair<int, int>(0, 0) || dx != 0
              || dy != 0) {
            std::string viadef_name = viadef->getName() + "_FR";
            std::unique_ptr<frShape> u_botfig = std::make_unique<frRect>();
            auto botfig = static_cast<frRect*>(u_botfig.get());
            std::unique_ptr<frShape> u_topfig = std::make_unique<frRect>();
            auto topfig = static_cast<frRect*>(u_topfig.get());
            Rect layer1_box = secondary_via.getLayer1BBox();
            Rect layer2_box = secondary_via.getLayer2BBox();
            layer1_box = layer1_box.bloat(layer1_bloats.first,
                                          odb::Orientation2D::Vertical);
            layer1_box = layer1_box.bloat(layer1_bloats.second,
                                          odb::Orientation2D::Horizontal);
            layer2_box = layer2_box.bloat(layer2_bloats.first,
                                          odb::Orientation2D::Vertical);
            layer2_box = layer2_box.bloat(layer2_bloats.second,
                                          odb::Orientation2D::Horizontal);
            layer1_box.moveDelta(-dx, -dy);
            layer2_box.moveDelta(-dx, -dy);
            frLayerNum layer1Num = viadef->getLayer1Num();
            frLayerNum layer2Num = viadef->getLayer2Num();
            botfig->setBBox(layer1_box);
            topfig->setBBox(layer2_box);
            botfig->setLayerNum(layer1Num);
            topfig->setLayerNum(layer2Num);
            // cut layer shape
            std::unique_ptr<frShape> u_cutfig = std::make_unique<frRect>();
            auto cutfig = static_cast<frRect*>(u_cutfig.get());
            Rect cut_box = secondary_via.getCutBBox();
            cut_box.moveDelta(-dx, -dy);
            cutfig->setBBox(cut_box);
            cutfig->setLayerNum(viadef->getCutLayerNum());

            // create via
            auto new_viadef = std::make_unique<frViaDef>(viadef_name);
            new_viadef->addLayer1Fig(std::move(u_botfig));
            new_viadef->addLayer2Fig(std::move(u_topfig));
            new_viadef->addCutFig(std::move(u_cutfig));
            new_viadef->setAddedByRouter(true);
            auto vdf_ptr = getTech()->addVia(std::move(new_viadef));
            if (vdf_ptr != nullptr) {
              layer->addSecondaryViaDef(vdf_ptr);
            }
          } else {
            layer->addSecondaryViaDef(viadef);
          }
        }
      }
    }
  }
}

// initialize secondLayerNum for rules that apply, reset the samenet rule if
// corresponding diffnet rule does not exist
void io::Parser::initConstraintLayerIdx()
{
  for (auto layerNum = getTech()->getBottomLayerNum();
       layerNum <= getTech()->getTopLayerNum();
       ++layerNum) {
    auto layer = getTech()->getLayer(layerNum);
    // non-LEF58
    auto& interLayerCutSpacingConstraints
        = layer->getInterLayerCutSpacingConstraintRef(false);
    // diff-net
    if (interLayerCutSpacingConstraints.empty()) {
      interLayerCutSpacingConstraints.resize(getTech()->getTopLayerNum() + 1,
                                             nullptr);
    }
    for (auto& [secondLayerName, con] :
         layer->getInterLayerCutSpacingConstraintMap(false)) {
      auto secondLayer = getTech()->getLayer(secondLayerName);
      if (secondLayer == nullptr) {
        logger_->warn(
            DRT, 235, "Second layer {} does not exist.", secondLayerName);
        continue;
      }
      auto secondLayerNum = getTech()->getLayer(secondLayerName)->getLayerNum();
      con->setSecondLayerNum(secondLayerNum);
      logger_->info(DRT,
                    236,
                    "Updating diff-net cut spacing rule between {} and {}.",
                    getTech()->getLayer(layerNum)->getName(),
                    getTech()->getLayer(secondLayerNum)->getName());
      interLayerCutSpacingConstraints[secondLayerNum] = con;
    }
    // same-net
    auto& interLayerCutSpacingSamenetConstraints
        = layer->getInterLayerCutSpacingConstraintRef(true);
    if (interLayerCutSpacingSamenetConstraints.empty()) {
      interLayerCutSpacingSamenetConstraints.resize(
          getTech()->getTopLayerNum() + 1, nullptr);
    }
    for (auto& [secondLayerName, con] :
         layer->getInterLayerCutSpacingConstraintMap(true)) {
      auto secondLayer = getTech()->getLayer(secondLayerName);
      if (secondLayer == nullptr) {
        logger_->warn(
            DRT, 237, "Second layer {} does not exist.", secondLayerName);
        continue;
      }
      auto secondLayerNum = getTech()->getLayer(secondLayerName)->getLayerNum();
      con->setSecondLayerNum(secondLayerNum);
      logger_->info(DRT,
                    238,
                    "Updating same-net cut spacing rule between {} and {}.",
                    getTech()->getLayer(layerNum)->getName(),
                    getTech()->getLayer(secondLayerNum)->getName());
      interLayerCutSpacingSamenetConstraints[secondLayerNum] = con;
    }
    // reset same-net if diff-net does not exist
    for (int i = 0; i < (int) interLayerCutSpacingConstraints.size(); i++) {
      if (interLayerCutSpacingConstraints[i] == nullptr) {
        // std::cout << i << std::endl << std::flush;
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
        frLayerNum secondLayerNum = getDesign()
                                        ->getTech()
                                        ->getLayer(con->getSecondLayerName())
                                        ->getLayerNum();
        con->setSecondLayerNum(secondLayerNum);
      }
    }
    // same-net
    for (auto& con : layer->getLef58CutSpacingConstraints(true)) {
      if (con->hasSecondLayer()) {
        frLayerNum secondLayerNum = getDesign()
                                        ->getTech()
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
  for (auto layerNum = getTech()->getBottomLayerNum();
       layerNum <= getTech()->getTopLayerNum();
       ++layerNum) {
    if (getTech()->getLayer(layerNum)->getType() != dbTechLayerType::CUT) {
      continue;
    }
    auto layer = getTech()->getLayer(layerNum);
    // update cut layer width is not specified in LEF
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
      } else {
        if (layerNum >= router_cfg_->BOTTOM_ROUTING_LAYER
            && layerNum <= router_cfg_->TOP_ROUTING_LAYER) {
          logger_->error(DRT,
                         242,
                         "CUT layer {} does not have default via.",
                         layer->getName());
        }
      }
    } else {
      auto viaDef = layer->getDefaultViaDef();
      if (viaDef) {
        auto cutFig = viaDef->getCutFigs()[0].get();
        if (cutFig->typeId() != frcRect) {
          logger_->error(DRT, 243, "Non-rectangular shape in via definition.");
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

  using boost::polygon::operators::operator+=;
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
  isNotLowerAlign
      = (isLayer1Horz
         && (getTech()->getLayer(viaDef->getLayer1Num())->getDir()
             == dbTechLayerDir::VERTICAL))
        || (!isLayer1Horz
            && (getTech()->getLayer(viaDef->getLayer1Num())->getDir()
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
  isNotUpperAlign
      = (isLayer2Horz
         && (getTech()->getLayer(viaDef->getLayer2Num())->getDir()
             == dbTechLayerDir::VERTICAL))
        || (!isLayer2Horz
            && (getTech()->getLayer(viaDef->getLayer2Num())->getDir()
                == dbTechLayerDir::HORIZONTAL));

  frCoord layer1Area = area(viaLayerPS1);
  frCoord layer2Area = area(viaLayerPS2);

  frCoord cutArea = 0;
  for (auto& fig : viaDef->getCutFigs()) {
    cutArea += fig->getBBox().area();
  }

  priority = std::make_tuple(isNotDefaultVia,
                             layer1Width,
                             layer2Width,
                             isNotUpperAlign,
                             cutArea,
                             layer2Area,
                             layer1Area,
                             isNotLowerAlign,
                             viaDef->getName());

  debugPrint(logger_,
             DRT,
             "via_selection",
             1,
             "via {} !default={} w1={} w2={} !align2={} area2={} area1={} "
             "!align1={}",
             viaDef->getName(),
             isNotDefaultVia,
             layer1Width,
             layer2Width,
             isNotUpperAlign,
             layer2Area,
             layer1Area,
             isNotLowerAlign);
}

// 13M_3Mx_2Cx_4Kx_2Hx_2Gx_LB
void io::Parser::initDefaultVias_GF14(const std::string& node)
{
  for (int layerNum = 1; layerNum < (int) getTech()->getLayers().size();
       layerNum += 2) {
    for (auto& uViaDef : getTech()->getVias()) {
      auto viaDef = uViaDef.get();
      if (viaDef->getCutLayerNum() == layerNum
          && node == "GF14_13M_3Mx_2Cx_4Kx_2Hx_2Gx_LB") {
        switch (layerNum) {
          case 3:  // VIA1
            if (viaDef->getName() == "V1_0_15_0_25_VH_Vx") {
              getTech()->getLayer(layerNum)->setDefaultViaDef(viaDef);
            }
            break;
          case 5:  // VIA2
            if (viaDef->getName() == "V2_0_25_0_25_HV_Vx") {
              getTech()->getLayer(layerNum)->setDefaultViaDef(viaDef);
            }
            break;
          case 7:  // VIA3
            if (viaDef->getName() == "J3_0_25_4_40_VH_Jy") {
              getTech()->getLayer(layerNum)->setDefaultViaDef(viaDef);
            }
            break;
          case 9:  // VIA4
            if (viaDef->getName() == "A4_0_50_0_50_HV_Ax") {
              getTech()->getLayer(layerNum)->setDefaultViaDef(viaDef);
            }
            break;
          case 11:  // VIA5
            if (viaDef->getName() == "CK_23_28_0_26_VH_CK") {
              getTech()->getLayer(layerNum)->setDefaultViaDef(viaDef);
            }
            break;
          case 13:  // VIA6
            if (viaDef->getName() == "U1_0_26_0_26_HV_Ux") {
              getTech()->getLayer(layerNum)->setDefaultViaDef(viaDef);
            }
            break;
          case 15:  // VIA7
            if (viaDef->getName() == "U2_0_26_0_26_VH_Ux") {
              getTech()->getLayer(layerNum)->setDefaultViaDef(viaDef);
            }
            break;
          case 17:  // VIA8
            if (viaDef->getName() == "U3_0_26_0_26_HV_Ux") {
              getTech()->getLayer(layerNum)->setDefaultViaDef(viaDef);
            }
            break;
          case 19:  // VIA9
            if (viaDef->getName() == "KH_18_45_0_45_VH_KH") {
              getTech()->getLayer(layerNum)->setDefaultViaDef(viaDef);
            }
            break;
          case 21:  // VIA10
            if (viaDef->getName() == "N1_0_45_0_45_HV_Nx") {
              getTech()->getLayer(layerNum)->setDefaultViaDef(viaDef);
            }
            break;
          case 23:  // VIA11
            if (viaDef->getName() == "HG_18_72_18_72_VH_HG") {
              getTech()->getLayer(layerNum)->setDefaultViaDef(viaDef);
            }
            break;
          case 25:  // VIA12
            if (viaDef->getName() == "T1_18_72_18_72_HV_Tx") {
              getTech()->getLayer(layerNum)->setDefaultViaDef(viaDef);
            }
            break;
          case 27:  // VIA13
            if (viaDef->getName() == "VV_450_450_450_450_XX_VV") {
              getTech()->getLayer(layerNum)->setDefaultViaDef(viaDef);
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
  auto bottomLayerNum = getTech()->getBottomLayerNum();
  auto topLayerNum = getTech()->getTopLayerNum();
  for (auto lNum = bottomLayerNum; lNum <= topLayerNum; lNum++) {
    frLayer* layer = getTech()->getLayer(lNum);
    if (layer->getType() != dbTechLayerType::ROUTING) {
      continue;
    }
    if (!layer->hasLef58Minimumcut()) {
      continue;
    }
    for (auto con : layer->getLef58MinimumcutConstraints()) {
      auto dbRule = con->getODBRule();
      std::unique_ptr<frConstraint> uCon
          = std::make_unique<frMinimumcutConstraint>();
      auto rptr = static_cast<frMinimumcutConstraint*>(uCon.get());
      if (dbRule->isPerCutClass()) {
        frViaDef* viaDefBelow = nullptr;
        if (lNum > bottomLayerNum) {
          viaDefBelow = getTech()->getLayer(lNum - 1)->getDefaultViaDef();
        }
        frViaDef* viaDefAbove = nullptr;
        if (lNum < topLayerNum) {
          viaDefAbove = getTech()->getLayer(lNum + 1)->getDefaultViaDef();
        }
        bool found = false;
        rptr->setNumCuts(dbRule->getNumCuts());
        for (const auto& [cutclass, numcuts] : dbRule->getCutClassCutsMap()) {
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
        if (!found) {
          continue;
        }
      }

      if (dbRule->isLengthValid()) {
        router_cfg_->MTSAFEDIST
            = std::max(router_cfg_->MTSAFEDIST, dbRule->getLengthWithinDist());
        rptr->setLength(dbRule->getLength(), dbRule->getLengthWithinDist());
      }
      rptr->setWidth(dbRule->getWidth());
      if (dbRule->isWithinCutDistValid()) {
        rptr->setWithin(dbRule->getWithinCutDist());
      }
      if (dbRule->isFromAbove()) {
        rptr->setConnection(frMinimumcutConnectionEnum::FROMABOVE);
      }
      if (dbRule->isFromBelow()) {
        rptr->setConnection(frMinimumcutConnectionEnum::FROMBELOW);
      }
      getTech()->addUConstraint(std::move(uCon));
      layer->addMinimumcutConstraint(rptr);
    }
  }
}

inline void getTrackLocs(bool isHorzTracks,
                         frLayer* layer,
                         frBlock* block,
                         frCoord low,
                         frCoord high,
                         std::set<frCoord>& trackLocs)
{
  for (auto& tp : block->getTrackPatterns(layer->getLayerNum())) {
    if (tp->isHorizontal() != isHorzTracks) {
      int trackNum = (low - tp->getStartCoord()) / (int) tp->getTrackSpacing();
      if (trackNum < 0) {
        trackNum = 0;
      }
      if (trackNum * (int) tp->getTrackSpacing() + tp->getStartCoord() < low) {
        ++trackNum;
      }
      for (; trackNum < (int) tp->getNumTracks()
             && trackNum * (int) tp->getTrackSpacing() + tp->getStartCoord()
                    <= high;
           ++trackNum) {
        frCoord trackLoc
            = trackNum * tp->getTrackSpacing() + tp->getStartCoord();
        trackLocs.insert(trackLoc);
      }
    }
  }
}

void io::Parser::checkFig(frPinFig* uFig,
                          const frString& term_name,
                          const dbTransform& xform,
                          bool& foundTracks,
                          bool& foundCenterTracks,
                          bool& hasPolys)
{
  int grid = getTech()->getManufacturingGrid();
  if (uFig->typeId() == frcRect) {
    frRect* shape = static_cast<frRect*>(uFig);
    Rect box = shape->getBBox();
    xform.apply(box);
    if (box.xMin() % grid || box.yMin() % grid || box.xMax() % grid
        || box.yMax() % grid) {
      logger_->error(DRT,
                     416,
                     "Term {} contains offgrid pin shape. Pin shape {} is "
                     "not a multiple of the manufacturing grid {}.",
                     term_name,
                     box,
                     grid);
    }
    if (foundTracks && foundCenterTracks) {
      return;
    }
    auto layer = getTech()->getLayer(shape->getLayerNum());
    std::set<int> horzTracks, vertTracks;
    getTrackLocs(true, layer, getBlock(), box.yMin(), box.yMax(), horzTracks);
    getTrackLocs(false, layer, getBlock(), box.xMin(), box.xMax(), vertTracks);
    bool allowWrongWayRouting
        = (router_cfg_->USENONPREFTRACKS && !layer->isUnidirectional());
    if (allowWrongWayRouting) {
      foundTracks |= (!horzTracks.empty() || !vertTracks.empty());
      foundCenterTracks
          |= horzTracks.find(box.yCenter()) != horzTracks.end()
             || vertTracks.find(box.xCenter()) != vertTracks.end();
    } else {
      if (layer->getDir() == odb::dbTechLayerDir::HORIZONTAL) {
        foundTracks |= !horzTracks.empty();
        foundCenterTracks |= horzTracks.find(box.yCenter()) != horzTracks.end();
      } else {
        foundTracks |= !vertTracks.empty();
        foundCenterTracks |= vertTracks.find(box.xCenter()) != vertTracks.end();
      }
    }
    if (foundTracks && box.minDXDY() > layer->getMinWidth()) {
      foundCenterTracks = true;
    }
  } else if (uFig->typeId() == frcPolygon) {
    hasPolys = true;
    auto polygon = static_cast<frPolygon*>(uFig);
    std::vector<gtl::point_data<frCoord>> points;
    for (Point pt : polygon->getPoints()) {
      xform.apply(pt);
      points.emplace_back(pt.x(), pt.y());
      if (pt.getX() % grid || pt.getY() % grid) {
        logger_->error(DRT,
                       417,
                       "Term {} contains offgrid pin shape. Polygon point "
                       "{} is not a multiple of the manufacturing grid {}.",
                       term_name,
                       pt,
                       grid);
      }
    }
    if (foundTracks) {
      return;
    }
    auto layer = getTech()->getLayer(polygon->getLayerNum());
    std::vector<gtl::rectangle_data<frCoord>> rects;
    gtl::polygon_90_data<frCoord> poly;
    poly.set(points.begin(), points.end());
    gtl::get_max_rectangles(rects, poly);
    for (const auto& rect : rects) {
      std::set<int> horzTracks, vertTracks;
      getTrackLocs(
          true, layer, getBlock(), gtl::yl(rect), gtl::yh(rect), horzTracks);
      getTrackLocs(
          false, layer, getBlock(), gtl::xl(rect), gtl::xh(rect), vertTracks);
      bool allowWrongWayRouting
          = (router_cfg_->USENONPREFTRACKS && !layer->isUnidirectional());
      if (allowWrongWayRouting) {
        foundTracks |= (!horzTracks.empty() || !vertTracks.empty());
      } else {
        if (layer->getDir() == odb::dbTechLayerDir::HORIZONTAL) {
          foundTracks |= !horzTracks.empty();
        } else {
          foundTracks |= !vertTracks.empty();
        }
      }
    }
  }
}

void io::Parser::checkPins()
{
  bool foundTracks = false;
  bool foundCenterTracks = false;
  bool hasPolys = false;
  // Check BTerms on grid
  for (const auto& bTerm : getBlock()->getTerms()) {
    foundTracks = false;
    foundCenterTracks = false;
    hasPolys = false;
    dbTransform xform;
    for (auto& pin : bTerm->getPins()) {
      for (auto& uFig : pin->getFigs()) {
        checkFig(uFig.get(),
                 bTerm->getName(),
                 xform,
                 foundTracks,
                 foundCenterTracks,
                 hasPolys);
      }
    }
    if (!foundTracks) {
      logger_->warn(
          DRT, 421, "Term {} has no pins on routing grid", bTerm->getName());
    } else if (!foundCenterTracks && !hasPolys) {
      logger_->warn(DRT,
                    422,
                    "No routing tracks pass through the center of Term {}.",
                    bTerm->getName());
    }
  }

  for (const auto& inst : getBlock()->getInsts()) {
    if (!inst->getMaster()->getMasterType().isBlock()) {
      continue;
    }
    dbTransform xform = inst->getUpdatedXform();
    for (auto& iTerm : inst->getInstTerms()) {
      if (!iTerm->hasNet() || iTerm->getNet()->isSpecial()) {
        continue;
      }
      foundTracks = false;
      foundCenterTracks = false;
      hasPolys = false;
      auto uTerm = iTerm->getTerm();
      for (auto& pin : uTerm->getPins()) {
        for (auto& uFig : pin->getFigs()) {
          checkFig(uFig.get(),
                   uTerm->getName(),
                   xform,
                   foundTracks,
                   foundCenterTracks,
                   hasPolys);
          if ((foundTracks && foundCenterTracks && uFig->typeId() == frcRect)
              || (foundTracks && uFig->typeId() == frcPolygon)) {
            continue;
          }
        }
      }
      if (!foundTracks) {
        logger_->warn(
            DRT, 418, "Term {} has no pins on routing grid", iTerm->getName());
      } else if (!foundCenterTracks && !hasPolys) {
        logger_->warn(DRT,
                      419,
                      "No routing tracks pass through the center of Term {}",
                      iTerm->getName());
      }
    }
  }
}

void io::Parser::postProcess()
{
  checkPins();
  initDefaultVias();
  if (router_cfg_->DBPROCESSNODE == "GF14_13M_3Mx_2Cx_4Kx_2Hx_2Gx_LB") {
    initDefaultVias_GF14(router_cfg_->DBPROCESSNODE);
  }
  initCutLayerWidth();
  initConstraintLayerIdx();
  getTech()->printDefaultVias(logger_, router_cfg_);
  instAnalysis();
  convertLef58MinCutConstraints();
  // init region query
  logger_->info(DRT, 168, "Init region query.");
  getDesign()->getRegionQuery()->init();
  getDesign()->getRegionQuery()->print();
  getDesign()->getRegionQuery()->initDRObj();  // second init from FlexDR.cpp
}

// instantiate RPin and region query for RPin
void io::Parser::initRPin()
{
  if (router_cfg_->VERBOSE > 0) {
    logger_->info(DRT, 185, "Post process initialize RPin region query.");
  }
  initRPin_rpin();
  initRPin_rq();
}

void io::Parser::initRPin_rpin()
{
  for (auto& net : getBlock()->getNets()) {
    // instTerm
    for (auto& instTerm : net->getInstTerms()) {
      auto inst = instTerm->getInst();
      int pinIdx = 0;
      auto trueTerm = instTerm->getTerm();
      for (auto& pin : trueTerm->getPins()) {
        auto rpin = std::make_unique<frRPin>();
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
          } else {
            continue;
          }
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
        auto rpin = std::make_unique<frRPin>();
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
  getDesign()->getRegionQuery()->initRPin();
}
}  // namespace drt
