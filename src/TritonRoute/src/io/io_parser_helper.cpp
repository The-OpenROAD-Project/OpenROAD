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
#include <iostream>
#include <boost/graph/connected_components.hpp>
#include "global.h"
#include "io/io.h"
#include "frBaseTypes.h"
#include "frProfileTask.h"
#include <fstream>
#include <sstream>
#include <boost/polygon/polygon.hpp>

using namespace std;
using namespace fr;
using namespace boost::polygon::operators;

using Rectangle = boost::polygon::rectangle_data<int>;

void io::Parser::initDefaultVias() {
  for (auto &uViaDef: tech->getVias()) {
    auto viaDef = uViaDef.get();
    tech->getLayer(viaDef->getCutLayerNum())->addViaDef(viaDef);
  }



  std::map<frLayerNum, std::map<int, std::map<viaRawPriorityTuple, frViaDef*> > > layerNum2ViaDefs;
  for (auto layerNum = design->getTech()->getBottomLayerNum(); layerNum <= design->getTech()->getTopLayerNum(); ++layerNum) {
    if (design->getTech()->getLayer(layerNum)->getType() != frLayerTypeEnum::CUT) {
      continue;
    }
    for (auto &viaDef: design->getTech()->getLayer(layerNum)->getViaDefs()) {
      int cutNum = int(viaDef->getCutFigs().size());
      viaRawPriorityTuple priority;
      getViaRawPriority(viaDef, priority);
      layerNum2ViaDefs[layerNum][cutNum][priority] = viaDef;
    }
    if (!layerNum2ViaDefs[layerNum][1].empty()) {
      auto defaultSingleCutVia = (layerNum2ViaDefs[layerNum][1].begin())->second;
      tech->getLayer(layerNum)->setDefaultViaDef(defaultSingleCutVia);
    } else {
      if (layerNum >= BOTTOM_ROUTING_LAYER) {
        std::cout << "Error: " << tech->getLayer(layerNum)->getName() << " does not have single-cut via\n";
        exit(1);
      }
    }
    // generate via if default via enclosure is not along pref dir
    if (ENABLE_VIA_GEN && layerNum >= BOTTOM_ROUTING_LAYER) {
      auto techDefautlViaDef = tech->getLayer(layerNum)->getDefaultViaDef();
      frVia via(techDefautlViaDef);
      frBox layer1Box;
      frBox layer2Box;
      frLayerNum layer1Num;
      frLayerNum layer2Num;
      via.getLayer1BBox(layer1Box);
      via.getLayer2BBox(layer2Box);
      layer1Num = techDefautlViaDef->getLayer1Num();
      layer2Num = techDefautlViaDef->getLayer2Num();
      bool isLayer1EncHorz = (layer1Box.right() - layer1Box.left()) > (layer1Box.top() - layer1Box.bottom());
      bool isLayer2EncHorz = (layer2Box.right() - layer2Box.left()) > (layer2Box.top() - layer2Box.bottom());
      bool isLayer1Horz = (tech->getLayer(layer1Num)->getDir() == frcHorzPrefRoutingDir);
      bool isLayer2Horz = (tech->getLayer(layer2Num)->getDir() == frcHorzPrefRoutingDir);
      bool needViaGen = false;
      if (isLayer1EncHorz != isLayer1Horz || isLayer2EncHorz != isLayer2Horz) {
        needViaGen = true;
      }

      // generate new via def if needed
      if (needViaGen) {
        // reference output file for writing def hack
        REF_OUT_FILE = OUT_FILE + string(".ref");
        string viaDefName = tech->getLayer(techDefautlViaDef->getCutLayerNum())->getName();
        viaDefName += string("_FR");
        cout << "Warning: " << tech->getLayer(layer1Num)->getName() << " does not have viaDef align with layer direction, generating new viaDef " << viaDefName << "...\n";
        // routing layer shape
        // rotate if needed
        if (isLayer1EncHorz != isLayer1Horz) {
          layer1Box.set(layer1Box.bottom(), layer1Box.left(), layer1Box.top(), layer1Box.right());
        }
        if (isLayer2EncHorz != isLayer2Horz) {
          layer2Box.set(layer2Box.bottom(), layer2Box.left(), layer2Box.top(), layer2Box.right());
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
        frBox cutBox;
        via.getCutBBox(cutBox);
        cutFig->setBBox(cutBox);
        cutFig->setLayerNum(techDefautlViaDef->getCutLayerNum());

        // create via
        auto viaDef = make_unique<frViaDef>(viaDefName);
        viaDef->addLayer1Fig(std::move(uBotFig));
        viaDef->addLayer2Fig(std::move(uTopFig));
        viaDef->addCutFig(std::move(uCutFig));
        viaDef->setAddedByRouter(true);
        tech->getLayer(layerNum)->setDefaultViaDef(viaDef.get());
        tech->addVia(std::move(viaDef));
      }
    }
  }
}

// initialize secondLayerNum for rules that apply, reset the samenet rule if corresponding diffnet rule does not exist
void io::Parser::initConstraintLayerIdx() {
  for (auto layerNum = design->getTech()->getBottomLayerNum(); layerNum <= design->getTech()->getTopLayerNum(); ++layerNum) {
    auto layer = design->getTech()->getLayer(layerNum);
    // non-LEF58
    auto &interLayerCutSpacingConstraints = layer->getInterLayerCutSpacingConstraintRef(false);
    // diff-net
    if (interLayerCutSpacingConstraints.empty()) {
      interLayerCutSpacingConstraints.resize(design->getTech()->getTopLayerNum() + 1, nullptr);
    }
    for (auto &[secondLayerName, con]: layer->getInterLayerCutSpacingConstraintMap(false)) {
      auto secondLayer = design->getTech()->getLayer(secondLayerName);
      if (secondLayer == nullptr) {
        cout << "Error: second layer " << secondLayerName << " does not exist\n";
        continue;
      }
      auto secondLayerNum = design->getTech()->getLayer(secondLayerName)->getLayerNum();
      con->setSecondLayerNum(secondLayerNum);
      cout << "  updating diff-net cut spacing rule between " << design->getTech()->getLayer(layerNum)->getName()
           << " and " << design->getTech()->getLayer(secondLayerNum)->getName() << "\n";
      interLayerCutSpacingConstraints[secondLayerNum] = con;
    }
    // same-net
    auto &interLayerCutSpacingSamenetConstraints = layer->getInterLayerCutSpacingConstraintRef(true);
    if (interLayerCutSpacingSamenetConstraints.empty()) {
      interLayerCutSpacingSamenetConstraints.resize(design->getTech()->getTopLayerNum() + 1, nullptr);
    }
    for (auto &[secondLayerName, con]: layer->getInterLayerCutSpacingConstraintMap(true)) {
      auto secondLayer = design->getTech()->getLayer(secondLayerName);
      if (secondLayer == nullptr) {
        cout << "Error: second layer " << secondLayerName << " does not exist\n";
        continue;
      }
      auto secondLayerNum = design->getTech()->getLayer(secondLayerName)->getLayerNum();
      con->setSecondLayerNum(secondLayerNum);
      cout << "  updating same-net cut spacing rule between " << design->getTech()->getLayer(layerNum)->getName()
           << " and " << design->getTech()->getLayer(secondLayerNum)->getName() << "\n";
      interLayerCutSpacingSamenetConstraints[secondLayerNum] = con;
    }
    // reset same-net if diff-net does not exist
    for (int i = 0; i < (int)interLayerCutSpacingConstraints.size(); i++) {
      if (interLayerCutSpacingConstraints[i] == nullptr) {
        // cout << i << endl << flush; 
        interLayerCutSpacingSamenetConstraints[i] = nullptr;
      } else {
        interLayerCutSpacingConstraints[i]->setSameNetConstraint(interLayerCutSpacingSamenetConstraints[i]);
      }
    }
    // LEF58
    // diff-net
    for (auto &con: layer->getLef58CutSpacingConstraints(false)) {
      if (con->hasSecondLayer()) {
        frLayerNum secondLayerNum = design->getTech()->getLayer(con->getSecondLayerName())->getLayerNum();
        con->setSecondLayerNum(secondLayerNum);
      }
    }
    // same-net
    for (auto &con: layer->getLef58CutSpacingConstraints(true)) {
      if (con->hasSecondLayer()) {
        frLayerNum secondLayerNum = design->getTech()->getLayer(con->getSecondLayerName())->getLayerNum();
        con->setSecondLayerNum(secondLayerNum);
      }
    }
  }
  cout << "done initConstraintLayerIdx\n" << flush;
}

// initialize cut layer width for cut OBS DRC check if not specified in LEF
void io::Parser::initCutLayerWidth() {
  for (auto layerNum = design->getTech()->getBottomLayerNum(); layerNum <= design->getTech()->getTopLayerNum(); ++layerNum) {
    if (design->getTech()->getLayer(layerNum)->getType() != frLayerTypeEnum::CUT) {
      continue;
    }
    auto layer = design->getTech()->getLayer(layerNum);
    // update cut layer width is not specifed in LEF
    if (layer->getWidth() == 0) {
      // first check default via size, if it is square, use that size
      auto viaDef = layer->getDefaultViaDef();
      if (viaDef) {
        auto cutFig = viaDef->getCutFigs()[0].get();
        if (cutFig->typeId() != frcRect) {
          cout << "Error: non-rect shape in via definition\n";
          exit(1);
        }
        auto cutRect = static_cast<frRect*>(cutFig);
        auto viaWidth = cutRect->width();
        layer->setWidth(viaWidth);
        if (viaDef->getNumCut() == 1) {
          if (cutRect->width() != cutRect->length()) {
            cout << "Warning: CUT layer " << layer->getName() << " does not have square single-cut via, cut layer width may be set incorrectly\n";
          }
        } else {
          cout << "Warning: CUT layer " << layer->getName() << " does not have single-cut via as default via, cut layer width may be set incorrectly\n";
        }
      } else {
        if (layerNum >= BOTTOM_ROUTING_LAYER) {
          cout << "Error: CUT layer " << layer->getName() << " does not have default via\n";
          exit(1);
        }
      }
    } else {
      auto viaDef = layer->getDefaultViaDef();
      int cutLayerWidth = layer->getWidth();
      if (viaDef) {
        auto cutFig = viaDef->getCutFigs()[0].get();
        if (cutFig->typeId() != frcRect) {
          cout << "Error: non-rect shape in via definition\n";
          exit(1);
        }
        auto cutRect = static_cast<frRect*>(cutFig);
        int viaWidth = cutRect->width();
        if (cutLayerWidth < viaWidth) {
          cout << "Warning: CUT layer " << layer->getName() << " has smaller width defined in LEF compared to default via\n";
        }
      }
    }
  }
}

void io::Parser::getViaRawPriority(frViaDef* viaDef, viaRawPriorityTuple &priority) {
  bool isNotDefaultVia = !(viaDef->getDefault());
  bool isNotUpperAlign = false;
  bool isNotLowerAlign = false;
  using PolygonSet = std::vector<boost::polygon::polygon_90_data<int>>;
  PolygonSet viaLayerPS1;

  for (auto &fig: viaDef->getLayer1Figs()) {
    frBox bbox;
    fig->getBBox(bbox);
    Rectangle bboxRect(bbox.left(), bbox.bottom(), bbox.right(), bbox.top());
    viaLayerPS1 += bboxRect;
  }
  Rectangle layer1Rect;
  extents(layer1Rect, viaLayerPS1);
  bool isLayer1Horz = (xh(layer1Rect) - xl(layer1Rect)) > (yh(layer1Rect) - yl(layer1Rect));
  frCoord layer1Width = std::min((xh(layer1Rect) - xl(layer1Rect)), (yh(layer1Rect) - yl(layer1Rect)));
  isNotLowerAlign = (isLayer1Horz && (tech->getLayer(viaDef->getLayer1Num())->getDir() == frcVertPrefRoutingDir)) ||
                    (!isLayer1Horz && (tech->getLayer(viaDef->getLayer1Num())->getDir() == frcHorzPrefRoutingDir));

  PolygonSet viaLayerPS2;
  for (auto &fig: viaDef->getLayer2Figs()) {
    frBox bbox;
    fig->getBBox(bbox);
    Rectangle bboxRect(bbox.left(), bbox.bottom(), bbox.right(), bbox.top());
    viaLayerPS2 += bboxRect;
  }
  Rectangle layer2Rect;
  extents(layer2Rect, viaLayerPS2);
  bool isLayer2Horz = (xh(layer2Rect) - xl(layer2Rect)) > (yh(layer2Rect) - yl(layer2Rect));
  frCoord layer2Width = std::min((xh(layer2Rect) - xl(layer2Rect)), (yh(layer2Rect) - yl(layer2Rect)));
  isNotUpperAlign = (isLayer2Horz && (tech->getLayer(viaDef->getLayer2Num())->getDir() == frcVertPrefRoutingDir)) ||
                    (!isLayer2Horz && (tech->getLayer(viaDef->getLayer2Num())->getDir() == frcHorzPrefRoutingDir));

  frCoord layer1Area = area(viaLayerPS1);
  frCoord layer2Area = area(viaLayerPS2);

  priority = std::make_tuple(isNotDefaultVia, layer1Width, layer2Width, isNotUpperAlign, layer2Area, layer1Area, isNotLowerAlign);
}

// 13M_3Mx_2Cx_4Kx_2Hx_2Gx_LB
void io::Parser::initDefaultVias_GF14(const string &node) {
  for (int layerNum = 1; layerNum < (int)tech->getLayers().size(); layerNum += 2) {
    for (auto &uViaDef: tech->getVias()) {
      auto viaDef = uViaDef.get();
      if (viaDef->getCutLayerNum() == layerNum && node == "GF14_13M_3Mx_2Cx_4Kx_2Hx_2Gx_LB") {
        switch(layerNum) {
          case 3 : // VIA1
                   if (viaDef->getName() == "V1_0_15_0_25_VH_Vx") {
                     tech->getLayer(layerNum)->setDefaultViaDef(viaDef);
                   }
                   break;
          case 5 : // VIA2
                   if (viaDef->getName() == "V2_0_25_0_25_HV_Vx") {
                     tech->getLayer(layerNum)->setDefaultViaDef(viaDef);
                   }
                   break;
          case 7 : // VIA3
                   if (viaDef->getName() == "J3_0_25_4_40_VH_Jy") {
                     tech->getLayer(layerNum)->setDefaultViaDef(viaDef);
                   }
                   break;
          case 9 : // VIA4
                   if (viaDef->getName() == "A4_0_50_0_50_HV_Ax") {
                     tech->getLayer(layerNum)->setDefaultViaDef(viaDef);
                   }
                   break;
          case 11: // VIA5
                   if (viaDef->getName() == "CK_23_28_0_26_VH_CK") {
                     tech->getLayer(layerNum)->setDefaultViaDef(viaDef);
                   }
                   break;
          case 13: // VIA6
                   if (viaDef->getName() == "U1_0_26_0_26_HV_Ux") {
                     tech->getLayer(layerNum)->setDefaultViaDef(viaDef);
                   }
                   break;
          case 15: // VIA7
                   if (viaDef->getName() == "U2_0_26_0_26_VH_Ux") {
                     tech->getLayer(layerNum)->setDefaultViaDef(viaDef);
                   }
                   break;
          case 17: // VIA8
                   if (viaDef->getName() == "U3_0_26_0_26_HV_Ux") {
                     tech->getLayer(layerNum)->setDefaultViaDef(viaDef);
                   }
                   break;
          case 19: // VIA9
                   if (viaDef->getName() == "KH_18_45_0_45_VH_KH") {
                     tech->getLayer(layerNum)->setDefaultViaDef(viaDef);
                   }
                   break;
          case 21: // VIA10
                   if (viaDef->getName() == "N1_0_45_0_45_HV_Nx") {
                     tech->getLayer(layerNum)->setDefaultViaDef(viaDef);
                   }
                   break;
          case 23: // VIA11
                   if (viaDef->getName() == "HG_18_72_18_72_VH_HG") {
                     tech->getLayer(layerNum)->setDefaultViaDef(viaDef);
                   }
                   break;
          case 25: // VIA12
                   if (viaDef->getName() == "T1_18_72_18_72_HV_Tx") {
                     tech->getLayer(layerNum)->setDefaultViaDef(viaDef);
                   }
                   break;
          case 27: // VIA13
                   if (viaDef->getName() == "VV_450_450_450_450_XX_VV") {
                     tech->getLayer(layerNum)->setDefaultViaDef(viaDef);
                   }
                   break;
          default: ;
        }
      }
    }
  }
}

// 11m_2xa1xd3xe2y2r
void io::Parser::initDefaultVias_N16(const string &node) {
  for (int layerNum = 1; layerNum < (int)tech->getLayers().size(); layerNum += 2) {
    for (auto &uViaDef: tech->getVias()) {
      auto viaDef = uViaDef.get();
      if (viaDef->getCutLayerNum() == layerNum && node == "N16_11m_2xa1xd3xe2y2r_utrdl") {
        switch(layerNum) {
          case 1 : // VIA1
                   if (viaDef->getName() == "NR_VIA1_PH") {
                     tech->getLayer(layerNum)->setDefaultViaDef(viaDef);
                   }
                   break;
          case 3 : // VIA2
                   if (viaDef->getName() == "VIA2_0_25_3_32_HV_Vxa") {
                     tech->getLayer(layerNum)->setDefaultViaDef(viaDef);
                   }
                   break;
          case 5 : // VIA3
                   if (viaDef->getName() == "VIA3_3_34_4_35_VH_Vxd") {
                     tech->getLayer(layerNum)->setDefaultViaDef(viaDef);
                   }
                   break;
          case 7 : // VIA4
                   if (viaDef->getName() == "VIA4_0_50_0_50_HV_Vxe") {
                     tech->getLayer(layerNum)->setDefaultViaDef(viaDef);
                   }
                   break;
          case 9 : // VIA5
                   if (viaDef->getName() == "VIA5_0_50_0_50_VH_Vxe") {
                     tech->getLayer(layerNum)->setDefaultViaDef(viaDef);
                   }
                   break;
          case 11: // VIA6
                   if (viaDef->getName() == "VIA6_0_50_0_50_HV_Vxe") {
                     tech->getLayer(layerNum)->setDefaultViaDef(viaDef);
                   }
                   break;
          case 13: // VIA7
                   if (viaDef->getName() == "NR_VIA7_VH") {
                     tech->getLayer(layerNum)->setDefaultViaDef(viaDef);
                   }
                   break;
          case 15: // VIA8
                   if (viaDef->getName() == "VIA8_0_27_0_27_HV_Vy") {
                     tech->getLayer(layerNum)->setDefaultViaDef(viaDef);
                   }
                   break;
          case 17: // VIA9
                   if (viaDef->getName() == "VIA9_18_72_18_72_VH_Vr") {
                     tech->getLayer(layerNum)->setDefaultViaDef(viaDef);
                   }
                   break;
          case 19: // VIA10
                   if (viaDef->getName() == "VIA10_18_72_18_72_HV_Vr") {
                     tech->getLayer(layerNum)->setDefaultViaDef(viaDef);
                   }
                   break;
          default: ;
        }
      } else if (viaDef->getCutLayerNum() == layerNum && node == "N16_9m_2xa1xd4xe1z_utrdl") {
        switch(layerNum) {
          case 1 : // VIA1
                   if (viaDef->getName() == "NR_VIA1_PH") {
                     tech->getLayer(layerNum)->setDefaultViaDef(viaDef);
                   }
                   break;
          case 3 : // VIA2
                   if (viaDef->getName() == "VIA2_0_25_3_32_HV_Vxa") {
                     tech->getLayer(layerNum)->setDefaultViaDef(viaDef);
                   }
                   break;
          case 5 : // VIA3
                   if (viaDef->getName() == "VIA3_3_34_4_35_VH_Vxd") {
                     tech->getLayer(layerNum)->setDefaultViaDef(viaDef);
                   }
                   break;
          case 7 : // VIA4
                   if (viaDef->getName() == "VIA4_0_50_0_50_HV_Vxe") {
                     tech->getLayer(layerNum)->setDefaultViaDef(viaDef);
                   }
                   break;
          case 9 : // VIA5
                   if (viaDef->getName() == "VIA5_0_50_0_50_VH_Vxe") {
                     tech->getLayer(layerNum)->setDefaultViaDef(viaDef);
                   }
                   break;
          case 11: // VIA6
                   if (viaDef->getName() == "VIA6_0_50_0_50_HV_Vxe") {
                     tech->getLayer(layerNum)->setDefaultViaDef(viaDef);
                   }
                   break;
          case 13: // VIA7
                   if (viaDef->getName() == "VIA7_0_50_0_50_VH_Vxe") {
                     tech->getLayer(layerNum)->setDefaultViaDef(viaDef);
                   }
                   break;
          case 15: // VIA8
                   if (viaDef->getName() == "VIA8_18_72_18_72_HV_Vz") {
                     tech->getLayer(layerNum)->setDefaultViaDef(viaDef);
                   }
                   break;
          case 17: // VIA9
                   if (viaDef->getName() == "RV_450_450_450_450_XX_RV") {
                     tech->getLayer(layerNum)->setDefaultViaDef(viaDef);
                   }
                   break;
          default: ;
        }
      } else if (viaDef->getCutLayerNum() == layerNum && node == "N16_9m_2xa1xd3xe2z_utrdl") {
        switch(layerNum) {
          case 1 : // VIA1
                   if (viaDef->getName() == "NR_VIA1_PH") {
                     tech->getLayer(layerNum)->setDefaultViaDef(viaDef);
                   }
                   break;
          case 3 : // VIA2
                   if (viaDef->getName() == "VIA2_0_25_3_32_HV_Vxa") {
                     tech->getLayer(layerNum)->setDefaultViaDef(viaDef);
                   }
                   break;
          case 5 : // VIA3
                   if (viaDef->getName() == "VIA3_3_34_4_35_VH_Vxd") {
                     tech->getLayer(layerNum)->setDefaultViaDef(viaDef);
                   }
                   break;
          case 7 : // VIA4
                   if (viaDef->getName() == "VIA4_0_50_0_50_HV_Vxe") {
                     tech->getLayer(layerNum)->setDefaultViaDef(viaDef);
                   }
                   break;
          case 9 : // VIA5
                   if (viaDef->getName() == "VIA5_0_50_0_50_VH_Vxe") {
                     tech->getLayer(layerNum)->setDefaultViaDef(viaDef);
                   }
                   break;
          case 11: // VIA6
                   if (viaDef->getName() == "VIA6_0_50_0_50_HV_Vxe") {
                     tech->getLayer(layerNum)->setDefaultViaDef(viaDef);
                   }
                   break;
          case 13: // VIA7
                   if (viaDef->getName() == "VIA7_18_72_18_72_VH_Vz") {
                     tech->getLayer(layerNum)->setDefaultViaDef(viaDef);
                   }
                   break;
          case 15: // VIA8
                   if (viaDef->getName() == "VIA8_18_72_18_72_HV_Vz") {
                     tech->getLayer(layerNum)->setDefaultViaDef(viaDef);
                   }
                   break;
          case 17: // VIA9
                   if (viaDef->getName() == "RV_450_450_450_450_XX_RV") {
                     tech->getLayer(layerNum)->setDefaultViaDef(viaDef);
                   }
                   break;
          default: ;
        }
      }
    }
  }
}

void io::Parser::writeRefDef() {

  vector<frViaDef*> genViaDefs;
  for (auto &uViaDef: tech->getVias()) {
    auto viaDef = uViaDef.get();
    if (viaDef->isAddedByRouter()) {
      genViaDefs.push_back(viaDef);
    }
  }

  if (REF_OUT_FILE != DEF_FILE) {
    cout << "Writing reference output def (" << REF_OUT_FILE << ")...\n";
    bool hasVia = false;
    ifstream fin(DEF_FILE);
    ofstream fout(REF_OUT_FILE);
    if (!fin.is_open()) {
      cout << "Error: cannot open input DEF\n";
      exit(1);
    }
    if (!fout.is_open()) {
      cout << "Error: cannot open reference output DEF\n";
      exit(1);
    }
    string line;
    string a;
    int b;
    while (getline(fin, line)) {
      istringstream iss(line);
      if (!(iss >> a >> b)) {
        continue;
      }
      if (a == string("VIAS")) {
        hasVia = true;
        break;
      }
    }

    // reset and write ref
    fin.clear();
    fin.seekg(0, ios::beg);

    while (getline(fin, line)) {
      bool skip = false;
      istringstream iss(line);
      if (iss >> a >> b) {
        // write empty VIAS section if does not exist
        if (!hasVia) {
          if (a == string("COMPONENTS")) {
            fout << "\nVIAS " << genViaDefs.size() << " ;\n";
            for (auto viaDef: genViaDefs) {
              frVia via(viaDef);
              frBox layer1Box, layer2Box, cutBox;
              via.getLayer1BBox(layer1Box);
              via.getCutBBox(cutBox);
              via.getLayer2BBox(layer2Box);

              fout << "- " << viaDef->getName() << endl;
              fout << "  + RECT " << tech->getLayer(viaDef->getLayer1Num())->getName() 
                   << " ( " << layer1Box.left() << " " << layer1Box.bottom() << " )"
                   << " ( " << layer1Box.right() << " " << layer1Box.top() << " )\n";
              fout << "  + RECT " << tech->getLayer(viaDef->getCutLayerNum())->getName() 
                   << " ( " << cutBox.left() << " " << cutBox.bottom() << " )"
                   << " ( " << cutBox.right() << " " << cutBox.top() << " )\n";
              fout << "  + RECT " << tech->getLayer(viaDef->getLayer2Num())->getName() 
                   << " ( " << layer2Box.left() << " " << layer2Box.bottom() << " )"
                   << " ( " << layer2Box.right() << " " << layer2Box.top() << " )\n";
              fout << "  ;\n";
            }
            fout << "END VIAS\n\n";
          }
        } else {
          if (a == string("VIAS")) {
            skip = true;
            fout << "\nVIAS " << b + (int)genViaDefs.size() << " ;\n";
            for (auto viaDef: genViaDefs) {
              frVia via(viaDef);
              frBox layer1Box, layer2Box, cutBox;
              via.getLayer1BBox(layer1Box);
              via.getCutBBox(cutBox);
              via.getLayer2BBox(layer2Box);

              fout << "- " << viaDef->getName() << endl;
              fout << "  + RECT " << tech->getLayer(viaDef->getLayer1Num())->getName() 
                   << " ( " << layer1Box.left() << " " << layer1Box.bottom() << " )"
                   << " ( " << layer1Box.right() << " " << layer1Box.top() << " )\n";
              fout << "  + RECT " << tech->getLayer(viaDef->getCutLayerNum())->getName() 
                   << " ( " << cutBox.left() << " " << cutBox.bottom() << " )"
                   << " ( " << cutBox.right() << " " << cutBox.top() << " )\n";
              fout << "  + RECT " << tech->getLayer(viaDef->getLayer2Num())->getName() 
                   << " ( " << layer2Box.left() << " " << layer2Box.bottom() << " )"
                   << " ( " << layer2Box.right() << " " << layer2Box.top() << " )\n";
              fout << "  ;\n";
            }
          }
        }
        // append generated via 

      }
      if (!skip) {
        fout << line << endl;
      }
    }

    fin.close();
    fout.close();
  }
}

void io::Parser::postProcess() {
  initDefaultVias();
  if (DBPROCESSNODE == "N16_11m_2xa1xd3xe2y2r_utrdl") {
    initDefaultVias_N16(DBPROCESSNODE);
  } else if (DBPROCESSNODE == "GF14_13M_3Mx_2Cx_4Kx_2Hx_2Gx_LB") {
    initDefaultVias_GF14(DBPROCESSNODE);
  } else {
    ;
  }
  initCutLayerWidth();
  initConstraintLayerIdx();
  tech->printDefaultVias();

  // write reference def for def writing hack flow if needed
  //writeRefDef();

  instAnalysis();

  // init region query
  cout <<endl <<"init region query ..." <<endl;
  design->getRegionQuery()->init(design->getTech()->getLayers().size());
  design->getRegionQuery()->print();
  design->getRegionQuery()->initDRObj(design->getTech()->getLayers().size()); // second init from FlexDR.cpp
}

void io::Parser::postProcessGuide() {
  ProfileTask profile("IO:postProcessGuide");
  if (VERBOSE > 0) {
    cout <<endl <<"post process guides ..." <<endl;
  }
  buildGCellPatterns();
  
  design->getRegionQuery()->initOrigGuide(design->getTech()->getLayers().size(), tmpGuides);
  int cnt = 0;
  //for (auto &[netName, rects]:tmpGuides) {
  //  if (design->getTopBlock()->name2net.find(netName) == design->getTopBlock()->name2net.end()) {
  //    cout <<"Error: postProcessGuide cannot find net" <<endl;
  //    exit(1);
  //  }
  for (auto &[net, rects]:tmpGuides) {
    genGuides(net, rects);
    cnt++;
    if (VERBOSE > 0) {
      if (cnt < 100000) {
        if (cnt % 10000 == 0) {
          cout <<"  complete " <<cnt <<" nets" <<endl;
        }
      } else {
        if (cnt % 100000 == 0) {
          cout <<"  complete " <<cnt <<" nets" <<endl;
        }
      }
    }
  }

  // global unique id for guides
  int currId = 0;
  for (auto &net: design->getTopBlock()->getNets()) {
    for (auto &guide: net->getGuides()) {
      guide->setId(currId);
      currId++;
    }
  }

  cout <<endl <<"init guide query ..." <<endl;
  design->getRegionQuery()->initGuide(design->getTech()->getLayers().size());
  design->getRegionQuery()->printGuide();
  cout <<endl <<"init gr pin query ..." <<endl;
  design->getRegionQuery()->initGRPin(tmpGRPins);

  if (OUTGUIDE_FILE == string("")) {
    if (VERBOSE > 0) {
      cout <<"Waring: no output guide specified, skipped writing guide" <<endl;
    }
  } else {
    //if (VERBOSE > 0) {
    //  cout <<endl <<"start writing output guide" <<endl;
    //}
    writeGuideFile();
  }
}

// instantiate RPin and region query for RPin
void io::Parser::initRPin() {
  if (VERBOSE > 0) {
    cout << endl << "post process initialize RPin region query ..." << endl;
  }
  initRPin_rpin();
  initRPin_rq();
}

void io::Parser::initRPin_rpin() {
  for (auto &net: design->getTopBlock()->getNets()) {
    // instTerm
    for (auto &instTerm: net->getInstTerms()) {
      auto inst = instTerm->getInst();
      int pinIdx = 0;
      auto trueTerm = instTerm->getTerm();
      for (auto &pin: trueTerm->getPins()) {
        auto rpin = make_unique<frRPin>();
        rpin->setFrTerm(instTerm);
        rpin->addToNet(net.get());
        frAccessPoint* prefAp = (instTerm->getAccessPoints())[pinIdx];

        // MACRO does not go through PA
        if (prefAp == nullptr) {
          if (inst->getRefBlock()->getMacroClass() == MacroClassEnum::BLOCK ||
              isPad(inst->getRefBlock()->getMacroClass()) ||
              inst->getRefBlock()->getMacroClass() == MacroClassEnum::RING) {
            prefAp = (pin->getPinAccess(inst->getPinAccessIdx())->getAccessPoints())[0].get();
          }
        }

        if (prefAp == nullptr) {
          cout << "Error: " << instTerm->getInst()->getName() << "/" << trueTerm->getName() 
               << " from " << net->getName() << " has nullptr as prefAP\n";  
        }

        rpin->setAccessPoint(prefAp);

        net->addRPin(rpin);

        pinIdx++;
      }
    }
    // term
    for (auto &term: net->getTerms()) {
      int pinIdx = 0;
      auto trueTerm = term;
      for (auto &pin: trueTerm->getPins()) {
        auto rpin = make_unique<frRPin>();
        rpin->setFrTerm(term);
        rpin->addToNet(net.get());
        frAccessPoint* prefAp = (pin->getPinAccess(0)->getAccessPoints())[0].get();
        rpin->setAccessPoint(prefAp);

        net->addRPin(rpin);

        pinIdx++;
      }
    }
  }
}

void io::Parser::initRPin_rq() {
  design->getRegionQuery()->initRPin(design->getTech()->getLayers().size());
}

void io::Parser::buildGCellPatterns_helper(frCoord &GCELLGRIDX, frCoord &GCELLGRIDY,
                                           frCoord &GCELLOFFSETX, frCoord &GCELLOFFSETY) {
  buildGCellPatterns_getWidth(GCELLGRIDX, GCELLGRIDY);
  buildGCellPatterns_getOffset(GCELLGRIDX, GCELLGRIDY, GCELLOFFSETX, GCELLOFFSETY);
}

void io::Parser::buildGCellPatterns_getWidth(frCoord &GCELLGRIDX, frCoord &GCELLGRIDY) {
  map<frCoord, int> guideGridXMap, guideGridYMap;
  // get GCell size information loop
  for (auto &[netName, rects]: tmpGuides) {
    for (auto &rect: rects) {
      frLayerNum layerNum = rect.getLayerNum();
      frBox guideBBox;
      rect.getBBox(guideBBox);
      frCoord guideWidth = (tech->getLayer(layerNum)->getDir() == frcHorzPrefRoutingDir) ?
                           (guideBBox.top() - guideBBox.bottom()):
                           (guideBBox.right() - guideBBox.left());
      if (tech->getLayer(layerNum)->getDir() == frcHorzPrefRoutingDir) {
        if (guideGridYMap.find(guideWidth) == guideGridYMap.end()) {
          guideGridYMap[guideWidth] = 0;
        }
        guideGridYMap[guideWidth]++;
      } else if (tech->getLayer(layerNum)->getDir() == frcVertPrefRoutingDir) {
        if (guideGridXMap.find(guideWidth) == guideGridXMap.end()) {
          guideGridXMap[guideWidth] = 0;
        }
        guideGridXMap[guideWidth]++;
      }
    }
  }
  frCoord tmpGCELLGRIDX = -1, tmpGCELLGRIDY = -1;
  int tmpGCELLGRIDXCnt = -1, tmpGCELLGRIDYCnt = -1;
  for (auto mapIt = guideGridXMap.begin(); mapIt != guideGridXMap.end(); ++mapIt) {
    auto cnt = mapIt->second;
    if (cnt > tmpGCELLGRIDXCnt) {
      tmpGCELLGRIDXCnt = cnt;
      tmpGCELLGRIDX = mapIt->first;
    }
    //cout <<"X width=" <<mapIt->first <<"/" <<mapIt->second <<endl;
  }
  for (auto mapIt = guideGridYMap.begin(); mapIt != guideGridYMap.end(); ++mapIt) {
    auto cnt = mapIt->second;
    if (cnt > tmpGCELLGRIDYCnt) {
      tmpGCELLGRIDYCnt = cnt;
      tmpGCELLGRIDY = mapIt->first;
    }
    //cout <<"Y width=" <<mapIt->first <<"/" <<mapIt->second <<endl;
  }
  if (tmpGCELLGRIDX != -1) {
    GCELLGRIDX = tmpGCELLGRIDX;
  } else {
    cout <<"Error: no GCELLGRIDX" <<endl;
    exit(1);
  }
  if (tmpGCELLGRIDY != -1) {
    GCELLGRIDY = tmpGCELLGRIDY;
  } else {
    cout <<"Error: no GCELLGRIDY" <<endl;
    exit(1);
  }
}

void io::Parser::buildGCellPatterns_getOffset(frCoord GCELLGRIDX, frCoord GCELLGRIDY,
                                              frCoord &GCELLOFFSETX, frCoord &GCELLOFFSETY) {
  std::map<frCoord, int> guideOffsetXMap, guideOffsetYMap;
  // get GCell offset information loop
  for (auto &[netName, rects]: tmpGuides) {
    for (auto &rect: rects) {
      //frLayerNum layerNum = rect.getLayerNum();
      frBox guideBBox;
      rect.getBBox(guideBBox);
      frCoord guideXOffset = guideBBox.left() % GCELLGRIDX;
      frCoord guideYOffset = guideBBox.bottom() % GCELLGRIDY;
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
  for (auto mapIt = guideOffsetXMap.begin(); mapIt != guideOffsetXMap.end(); ++mapIt) {
    auto cnt = mapIt->second;
    if (cnt > tmpGCELLOFFSETXCnt) {
      tmpGCELLOFFSETXCnt = cnt;
      tmpGCELLOFFSETX = mapIt->first;
    }
  }
  for (auto mapIt = guideOffsetYMap.begin(); mapIt != guideOffsetYMap.end(); ++mapIt) {
    auto cnt = mapIt->second;
    if (cnt > tmpGCELLOFFSETYCnt) {
      tmpGCELLOFFSETYCnt = cnt;
      tmpGCELLOFFSETY = mapIt->first;
    }
  }
  if (tmpGCELLOFFSETX != -1) {
    GCELLOFFSETX = tmpGCELLOFFSETX;
  } else {
    cout <<"Error: no GCELLGRIDX" <<endl;
    exit(1);
  } 
  if (tmpGCELLOFFSETY != -1) {
    GCELLOFFSETY = tmpGCELLOFFSETY;
  } else {
    cout <<"Error: no GCELLGRIDX" <<endl;
    exit(1);
  }
}

void io::Parser::buildGCellPatterns() {
  // horizontal = false is gcell lines along y direction (x-grid)
  frBox dieBox;
  design->getTopBlock()->getBoundaryBBox(dieBox);

  frCoord GCELLOFFSETX, GCELLOFFSETY, GCELLGRIDX, GCELLGRIDY;
  buildGCellPatterns_helper(GCELLGRIDX, GCELLGRIDY, GCELLOFFSETX, GCELLOFFSETY);

  frGCellPattern xgp;
  xgp.setHorizontal(false);
  // find first coord >= dieBox.left()
  frCoord startCoordX = dieBox.left() / (frCoord)GCELLGRIDX * (frCoord)GCELLGRIDX + GCELLOFFSETX;
  if (startCoordX > dieBox.left()) {
    startCoordX -= (frCoord)GCELLGRIDX;
  }
  xgp.setStartCoord(startCoordX);
  xgp.setSpacing(GCELLGRIDX);
  if ((dieBox.right() - (frCoord)GCELLOFFSETX) / (frCoord)GCELLGRIDX < 1) {
    cout <<"Error: gcell cnt < 1" <<endl;
    exit(1);
  }
  xgp.setCount((dieBox.right() - (frCoord)startCoordX) / (frCoord)GCELLGRIDX);
  
  frGCellPattern ygp;
  ygp.setHorizontal(true);
  // find first coord >= dieBox.bottom()
  frCoord startCoordY = dieBox.bottom() / (frCoord)GCELLGRIDY * (frCoord)GCELLGRIDY + GCELLOFFSETY;
  if (startCoordY > dieBox.bottom()) {
    startCoordY -= (frCoord)GCELLGRIDY;
  }
  ygp.setStartCoord(startCoordY);
  ygp.setSpacing(GCELLGRIDY);
  if ((dieBox.top() - (frCoord)GCELLOFFSETY) / (frCoord)GCELLGRIDY < 1) {
    cout <<"Error: gcell cnt < 1" <<endl;
    exit(1);
  }
  ygp.setCount((dieBox.top() - startCoordY) / (frCoord)GCELLGRIDY);

  if (VERBOSE > 0) {
    cout <<"GCELLGRID X " <<ygp.getStartCoord() <<" DO " <<ygp.getCount() <<" STEP " <<ygp.getSpacing() <<" ;" <<endl;
    cout <<"GCELLGRID Y " <<xgp.getStartCoord() <<" DO " <<xgp.getCount() <<" STEP " <<xgp.getSpacing() <<" ;" <<endl;
  }

  design->getTopBlock()->setGCellPatterns({xgp, ygp});

  for (int layerNum = 0; layerNum <= (int)tech->getLayers().size(); layerNum += 2) {
    for (int i = 0; i < (int)xgp.getCount(); i++) {
      for (int j = 0; j < (int)ygp.getCount(); j++) {
        frBox gcellBox;
        design->getTopBlock()->getGCellBox(frPoint(i, j), gcellBox);
        bool isH = (tech->getLayers().at(layerNum)->getDir() == frcHorzPrefRoutingDir);
        frCoord gcLow  = isH ? gcellBox.bottom() : gcellBox.right();
        frCoord gcHigh = isH ? gcellBox.top()    : gcellBox.left();
        int trackCnt = 0;
        for (auto &tp: design->getTopBlock()->getTrackPatterns(layerNum)) {
          if ((tech->getLayer(layerNum)->getDir() == frcHorzPrefRoutingDir &&
               tp->isHorizontal() == false) ||
              (tech->getLayer(layerNum)->getDir() == frcVertPrefRoutingDir &&
               tp->isHorizontal() == true)) {
            int trackNum = (gcLow - tp->getStartCoord()) / (int)tp->getTrackSpacing();
            if (trackNum < 0) {
              trackNum = 0;
            }
            if (trackNum * (int)tp->getTrackSpacing() + tp->getStartCoord() < gcLow) {
              trackNum++;
            }
            for (; trackNum < (int)tp->getNumTracks() && trackNum * (int)tp->getTrackSpacing() + tp->getStartCoord() < gcHigh; trackNum++) {
              trackCnt++;
            }

          }
        }
      }
    }
  }
}

void io::Parser::writeGuideFile() {
  ofstream outputGuide(OUTGUIDE_FILE.c_str());
  if (outputGuide.is_open()) {
    for (auto &net: design->topBlock_->getNets()) {
      auto netName = net->getName();
      outputGuide << netName << endl;
      outputGuide << "(\n"; 
      for (auto &guide: net->getGuides()) {
        frPoint bp, ep;
        guide->getPoints(bp, ep);
        frPoint bpIdx, epIdx;
        design->getTopBlock()->getGCellIdx(bp, bpIdx);
        design->getTopBlock()->getGCellIdx(ep, epIdx);
        frBox bbox, ebox;
        design->getTopBlock()->getGCellBox(bpIdx, bbox);
        design->getTopBlock()->getGCellBox(epIdx, ebox);
        frLayerNum bNum = guide->getBeginLayerNum();
        frLayerNum eNum = guide->getEndLayerNum();
        // append unit guide in case of stacked via
        if (bNum != eNum) {
          auto startLayerName = tech->getLayer(bNum)->getName();
          outputGuide << bbox.left()  << " " << bbox.bottom() << " "
                      << bbox.right() << " " << bbox.top()    << " "
                      << startLayerName <<".5" << endl;
        } else {
          auto layerName = tech->getLayer(bNum)->getName();
          outputGuide << bbox.left()  << " " << bbox.bottom() << " "
                      << ebox.right() << " " << ebox.top()    << " "
                      << layerName << endl;
        }
      }
      outputGuide << ")\n";
    }
  }
}
