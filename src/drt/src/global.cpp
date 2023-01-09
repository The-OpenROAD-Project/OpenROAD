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

#include "global.h"

#include <iostream>

#include "db/drObj/drFig.h"
#include "db/drObj/drNet.h"
#include "db/drObj/drShape.h"
#include "db/drObj/drVia.h"
#include "db/obj/frBlock.h"
#include "db/obj/frMaster.h"
#include "frDesign.h"

using namespace std;
using namespace fr;

string OUT_MAZE_FILE;
string DRC_RPT_FILE;
string CMAP_FILE;
string GUIDE_REPORT_FILE;

// to be removed
int OR_SEED = -1;
double OR_K = 0;

string DBPROCESSNODE = "";
int MAX_THREADS = 1;
int BATCHSIZE = 1024;
int BATCHSIZETA = 8;
int MTSAFEDIST = 2000;
int DRCSAFEDIST = 500;
int VERBOSE = 1;
std::string BOTTOM_ROUTING_LAYER_NAME;
std::string TOP_ROUTING_LAYER_NAME;
int BOTTOM_ROUTING_LAYER = 2;
int TOP_ROUTING_LAYER = std::numeric_limits<frLayerNum>::max();
bool ALLOW_PIN_AS_FEEDTHROUGH = false;
bool USENONPREFTRACKS = true;
bool USEMINSPACING_OBS = true;
bool ENABLE_BOUNDARY_MAR_FIX = true;
bool ENABLE_VIA_GEN = true;
bool CLEAN_PATCHES = false;
bool DO_PA = true;
bool SINGLE_STEP_DR = false;
bool SAVE_GUIDE_UPDATES = false;

std::string VIAINPIN_BOTTOMLAYER_NAME;
std::string VIAINPIN_TOPLAYER_NAME;
frLayerNum VIAINPIN_BOTTOMLAYERNUM = std::numeric_limits<frLayerNum>::max();
frLayerNum VIAINPIN_TOPLAYERNUM = std::numeric_limits<frLayerNum>::max();
int MINNUMACCESSPOINT_MACROCELLPIN = 3;
int MINNUMACCESSPOINT_STDCELLPIN = 3;
int ACCESS_PATTERN_END_ITERATION_NUM = 10;
float CONGESTION_THRESHOLD = 0.4;
int MAX_CLIPSIZE_INCREASE = 18;

frLayerNum VIA_ACCESS_LAYERNUM = 2;

int NDR_NETS_ABS_PRIORITY = 2;
int CLOCK_NETS_ABS_PRIORITY = 4;

int END_ITERATION = 80;
int NDR_NETS_RIPUP_HARDINESS = 3;
int CLOCK_NETS_TRUNK_RIPUP_HARDINESS = 100;
int CLOCK_NETS_LEAF_RIPUP_HARDINESS = 10;
bool AUTO_TAPER_NDR_NETS = true;
int TAPERBOX_RADIUS = 3;

frUInt4 TAVIACOST = 1;
frUInt4 TAPINCOST = 4;
frUInt4 TAALIGNCOST = 4;
frUInt4 TADRCCOST = 32;
float TASHAPEBLOATWIDTH = 1.5;

frUInt4 VIACOST = 4;
// new cost used
frUInt4 GRIDCOST = 2;
frUInt4 ROUTESHAPECOST = 8;
frUInt4 MARKERCOST = 32;
frUInt4 MARKERBLOATWIDTH = 1;
frUInt4 BLOCKCOST = 32;
frUInt4 GUIDECOST = 1;  // disabled change getNextPathCost to enable
float SHAPEBLOATWIDTH = 3;
int MISALIGNMENTCOST = 8;

int CONGCOST = 8;
int HISTCOST = 32;
std::string REPAIR_PDN_LAYER_NAME;
frLayerNum GC_IGNORE_PDN_LAYER = -1;
namespace fr {

ostream& operator<<(ostream& os, const frRect& pinFigIn)
{
  //  if (pinFigIn.getPin()) {
  //    os << "PINFIG (PINNAME/LAYER) " <<
  //    pinFigIn.getPin()->getTerm()->getName()
  //       << " " << pinFigIn.getLayerNum() << endl;
  //  }
  Rect tmpBox = pinFigIn.getBBox();
  os << "  RECT " << tmpBox.xMin() << " " << tmpBox.yMin() << " "
     << tmpBox.xMax() << " " << tmpBox.yMax();
  return os;
}

ostream& operator<<(ostream& os, const frPolygon& pinFigIn)
{
  //  if (pinFigIn.getPin()) {
  //    os << "PINFIG (NAME/LAYER) " << pinFigIn.getPin()->getTerm()->getName()
  //       << " " << pinFigIn.getLayerNum() << endl;
  //  }
  os << "  POLYGON";
  for (auto& m : pinFigIn.getPoints()) {
    os << " ( " << m.x() << " " << m.y() << " )";
  }
  return os;
}

ostream& operator<<(ostream& os, const frMPin& pinIn)
{
  os << "PIN (NAME) " << pinIn.getTerm()->getName();
  for (auto& m : pinIn.getFigs()) {
    if (m->typeId() == frcRect) {
      os << endl << *(static_cast<frRect*>(m.get()));
    } else if (m->typeId() == frcPolygon) {
      os << endl << *(static_cast<frPolygon*>(m.get()));
    } else {
      os << endl << "Unsupported pinFig object!";
    }
  }
  return os;
}

ostream& operator<<(ostream& os, const frBTerm& termIn)
{
  frString name;
  frString netName;
  name = termIn.getName();
  if (termIn.getNet()) {
    netName = termIn.getNet()->getName();
  }
  os << "TERM (NAME/NET) " << name << " " << netName;
  for (auto& m : termIn.getPins()) {
    os << endl << *m;
  }
  return os;
}

ostream& operator<<(ostream& os, const frInstTerm& instTermIn)
{
  frString name;
  frString cellName;
  frString termName;
  frString netName;
  name = instTermIn.getInst()->getName();
  cellName = instTermIn.getInst()->getMaster()->getName();
  termName = instTermIn.getTerm()->getName();
  if (instTermIn.getNet()) {
    netName = instTermIn.getNet()->getName();
  }
  os << "INSTTERM: (INST/CELL/TERM/NET) " << name << " " << cellName << " "
     << termName << " " << netName << endl;
  os << *instTermIn.getTerm() << "END_INSTTERM";
  return os;
}

ostream& operator<<(ostream& os, const frViaDef& viaDefIn)
{
  frString name;
  name = viaDefIn.getName();
  os << "VIA " << name;
  if (viaDefIn.getDefault()) {
    os << " DEFAULT";
  }
  for (auto& m : viaDefIn.getLayer1Figs()) {
    if (m->typeId() == frcRect) {
      os << endl << *(static_cast<frRect*>(m.get()));
    } else if (m->typeId() == frcPolygon) {
      os << endl << *(static_cast<frPolygon*>(m.get()));
    } else {
      os << endl << "Unsupported pinFig object!";
    }
  }
  for (auto& m : viaDefIn.getCutFigs()) {
    if (m->typeId() == frcRect) {
      os << endl << *(static_cast<frRect*>(m.get()));
    } else if (m->typeId() == frcPolygon) {
      os << endl << *(static_cast<frPolygon*>(m.get()));
    } else {
      os << endl << "Unsupported pinFig object!";
    }
  }
  for (auto& m : viaDefIn.getLayer2Figs()) {
    if (m->typeId() == frcRect) {
      os << endl << *(static_cast<frRect*>(m.get()));
    } else if (m->typeId() == frcPolygon) {
      os << endl << *(static_cast<frPolygon*>(m.get()));
    } else {
      os << endl << "Unsupported pinFig object!";
    }
  }
  return os;
}

ostream& operator<<(ostream& os, const frBlock& blockIn)
{
  Rect box = blockIn.getBBox();
  os << "MACRO " << blockIn.getName() << endl
     << "  ORIGIN " << box.xMin() << " " << box.yMin() << endl
     << "  SIZE " << box.xMax() << " " << box.yMax();
  for (auto& m : blockIn.getTerms()) {
    os << endl << *m;
  }
  return os;
}

ostream& operator<<(ostream& os, const frInst& instIn)
{
  Point tmpPoint = instIn.getOrigin();
  auto tmpOrient = instIn.getOrient();
  frString tmpName = instIn.getName();
  frString tmpString = instIn.getMaster()->getName();
  os << "- " << tmpName << " " << tmpString << " + STATUS + ( " << tmpPoint.x()
     << " " << tmpPoint.y() << " ) " << tmpOrient.getString() << endl;
  for (auto& m : instIn.getInstTerms()) {
    os << endl << *m;
  }
  return os;
}

ostream& operator<<(ostream& os, const drConnFig& fig)
{
  switch (fig.typeId()) {
    case drcPathSeg: {
      auto p = static_cast<const drPathSeg*>(&fig);
      os << "drPathSeg: begin (" << p->getBeginX() << " " << p->getBeginY()
         << " ) end ( " << p->getEndX() << " " << p->getEndY() << " ) layerNum "
         << p->getLayerNum();
      frSegStyle st = p->getStyle();
      os << "\n\tbeginStyle: " << st.getBeginStyle()
         << "\n\tendStyle: " << st.getEndStyle();
      break;
    }
    case drcVia: {
      auto p = static_cast<const drVia*>(&fig);
      os << "drVia: at " << p->getOrigin() << "\nVIA DEF:\n" << *p->getViaDef();
      break;
    }
    case drcPatchWire: {
      auto p = static_cast<const drPatchWire*>(&fig);
      os << "drPatchWire: " << p->getBBox();
      break;
    }
    default:
      os << "UNKNOWN drConnFig, code " << fig.typeId();
  }

  return os;
}

ostream& operator<<(ostream& os, const frPathSeg& p)
{
  os << "frPathSeg: begin (" << p.getBeginPoint().x() << " "
     << p.getBeginPoint().y() << " ) end ( " << p.getEndPoint().x() << " "
     << p.getEndPoint().y() << " ) layerNum " << p.getLayerNum();
  frSegStyle st = p.getStyle();
  os << "\n\tbeginStyle: " << st.getBeginStyle()
     << "\n\tendStyle: " << st.getEndStyle();
  return os;
}

ostream& operator<<(ostream& os, const frGuide& p)
{
  os << "frGuide: begin " << p.getBeginPoint() << " end " << p.getEndPoint()
     << " begin LayerNum " << p.getBeginLayerNum() << " end layerNum "
     << p.getEndLayerNum();
  return os;
}

ostream& operator<<(ostream& os, const frConnFig& fig)
{
  switch (fig.typeId()) {
    case frcPathSeg: {
      auto p = static_cast<const frPathSeg*>(&fig);
      os << *p;
      break;
    }
    case frcVia: {
      auto p = static_cast<const frVia*>(&fig);
      os << "frVia: at " << p->getOrigin() << "\nVIA DEF:\n" << *p->getViaDef();
      break;
    }
    case frcPatchWire: {
      auto p = static_cast<const frPatchWire*>(&fig);
      os << "frPatchWire: " << p->getBBox();
      break;
    }
    case frcGuide: {
      auto p = static_cast<const frGuide*>(&fig);
      os << p;
      break;
    }
    case frcRect: {
      auto p = static_cast<const frRect*>(&fig);
      os << "frRect: " << p->getBBox();
      break;
    }
    case frcPolygon: {
      os << *static_cast<const frPolygon*>(&fig);
      break;
    }
    default:
      os << "UNKNOWN frShape, code " << fig.typeId();
  }
  return os;
}

ostream& operator<<(ostream& os, const frBlockObject& fig)
{
  switch (fig.typeId()) {
    case frcInstTerm: {
      os << *static_cast<const frInstTerm*>(&fig);
      break;
    }
    case frcMTerm:
    case frcBTerm: {
      os << *static_cast<const frTerm*>(&fig);
      break;
    }
    // case fig is a frConnFig
    case frcPathSeg:
    case frcVia:
    case frcPatchWire:
    case frcGuide:
    case frcRect:
    case frcPolygon: {
      os << *static_cast<const frConnFig*>(&fig);
      break;
    }
    case frcNet: {
      os << *static_cast<const frNet*>(&fig);
      break;
    }
    default:
      os << "UNKNOWN frBlockObject, code " << fig.typeId();
  }
  return os;
}

ostream& operator<<(ostream& os, const frNet& n)
{
  os << "frNet " << n.getName() << (n.isClock() ? "CLOCK NET " : " ")
     << (n.hasNDR() ? "NDR " + n.getNondefaultRule()->getName() + " " : " ")
     << "abs priority " << n.getAbsPriorityLvl() << "\n";
  return os;
}

ostream& operator<<(ostream& os, const drNet& n)
{
  os << "drNet " << *n.getFrNet() << "\nnRipupAvoids " << n.getNRipupAvoids()
     << " maxRipupAvoids " << n.getMaxRipupAvoids() << "\n";
  return os;
}

ostream& operator<<(ostream& os, const frMarker& m)
{
  os << "MARKER: box " << m.getBBox() << " lNum " << m.getLayerNum()
     << " constraint: ";
  switch (m.getConstraint()->typeId()) {
    case frConstraintTypeEnum::frcAreaConstraint:
      return os << "frcAreaConstraint";
    case frConstraintTypeEnum::frcCutSpacingConstraint:
      return os << "frcCutSpacingConstraint";
    case frConstraintTypeEnum::frcLef58CornerSpacingConcaveCornerConstraint:
      return os << "frcLef58CornerSpacingConcaveCornerConstraint";
    case frConstraintTypeEnum::frcLef58CornerSpacingConstraint:
      return os << "frcLef58CornerSpacingConstraint";
    case frConstraintTypeEnum::frcLef58CornerSpacingConvexCornerConstraint:
      return os << "frcLef58CornerSpacingConvexCornerConstraint";
    case frConstraintTypeEnum::frcLef58CornerSpacingSpacing1DConstraint:
      return os << "frcLef58CornerSpacingSpacing1DConstraint";
    case frConstraintTypeEnum::frcLef58CornerSpacingSpacing2DConstraint:
      return os << "frcLef58CornerSpacingSpacing2DConstraint";
    case frConstraintTypeEnum::frcLef58CornerSpacingSpacingConstraint:
      return os << "frcLef58CornerSpacingSpacingConstraint";
    case frConstraintTypeEnum::frcLef58CutClassConstraint:
      return os << "frcLef58CutClassConstraint";
    case frConstraintTypeEnum::frcLef58CutSpacingAdjacentCutsConstraint:
      return os << "frcLef58CutSpacingAdjacentCutsConstraint";
    case frConstraintTypeEnum::frcLef58CutSpacingConstraint:
      return os << "frcLef58CutSpacingConstraint";
    case frConstraintTypeEnum::frcLef58CutSpacingLayerConstraint:
      return os << "frcLef58CutSpacingLayerConstraint";
    case frConstraintTypeEnum::frcLef58CutSpacingParallelWithinConstraint:
      return os << "frcLef58CutSpacingParallelWithinConstraint";
    case frConstraintTypeEnum::frcLef58CutSpacingTableConstraint:
      return os << "frcLef58CutSpacingTableConstraint";
    case frConstraintTypeEnum::frcLef58CutSpacingTableLayerConstraint:
      return os << "frcLef58CutSpacingTableLayerConstraint";
    case frConstraintTypeEnum::frcLef58CutSpacingTablePrlConstraint:
      return os << "frcLef58CutSpacingTablePrlConstraint";
    case frConstraintTypeEnum::frcLef58EolExtensionConstraint:
      return os << "frcLef58EolExtensionConstraint";
    case frConstraintTypeEnum::frcLef58EolKeepOutConstraint:
      return os << "frcLef58EolKeepOutConstraint";

    case frConstraintTypeEnum::frcLef58MinStepConstraint:
      return os << "frcLef58MinStepConstraint";
    case frConstraintTypeEnum::frcLef58RectOnlyConstraint:
      return os << "frcLef58RectOnlyConstraint";
    case frConstraintTypeEnum::frcLef58RightWayOnGridOnlyConstraint:
      return os << "frcLef58RightWayOnGridOnlyConstraint";
    case frConstraintTypeEnum::frcLef58SpacingEndOfLineConstraint:
      return os << "frcLef58SpacingEndOfLineConstraint";
    case frConstraintTypeEnum::frcLef58SpacingEndOfLineWithinConstraint:
      return os << "frcLef58SpacingEndOfLineWithinConstraint";
    case frConstraintTypeEnum::
        frcLef58SpacingEndOfLineWithinEncloseCutConstraint:
      return os << "frcLef58SpacingEndOfLineWithinEncloseCutConstraint";
    case frConstraintTypeEnum::frcLef58SpacingEndOfLineWithinEndToEndConstraint:
      return os << "frcLef58SpacingEndOfLineWithinEndToEndConstraint";
    case frConstraintTypeEnum::
        frcLef58SpacingEndOfLineWithinMaxMinLengthConstraint:
      return os << "frcLef58SpacingEndOfLineWithinMaxMinLengthConstraint";
    case frConstraintTypeEnum::
        frcLef58SpacingEndOfLineWithinParallelEdgeConstraint:
      return os << "frcLef58SpacingEndOfLineWithinParallelEdgeConstraint";
    case frConstraintTypeEnum::frcLef58SpacingTableConstraint:
      return os << "frcLef58SpacingTableConstraint";
    case frConstraintTypeEnum::frcMinEnclosedAreaConstraint:
      return os << "frcMinEnclosedAreaConstraint";
    case frConstraintTypeEnum::frcMinStepConstraint:
      return os << "frcMinStepConstraint";
    case frConstraintTypeEnum::frcMinWidthConstraint:
      return os << "frcMinWidthConstraint";
    case frConstraintTypeEnum::frcMinimumcutConstraint:
      return os << "frcMinimumcutConstraint";
    case frConstraintTypeEnum::frcNonSufficientMetalConstraint:
      return os << "frcNonSufficientMetalConstraint";
    case frConstraintTypeEnum::frcOffGridConstraint:
      return os << "frcOffGridConstraint";
    case frConstraintTypeEnum::frcRecheckConstraint:
      return os << "frcRecheckConstraint";
    case frConstraintTypeEnum::frcShortConstraint:
      return os << "frcShortConstraint";
    case frConstraintTypeEnum::frcSpacingConstraint:
      return os << "frcSpacingConstraint";
    case frConstraintTypeEnum::frcSpacingEndOfLineConstraint:
      return os << "frcSpacingEndOfLineConstraint";
    case frConstraintTypeEnum::frcSpacingEndOfLineParallelEdgeConstraint:
      return os << "frcSpacingEndOfLineParallelEdgeConstraint";
    case frConstraintTypeEnum::frcSpacingSamenetConstraint:
      return os << "frcSpacingSamenetConstraint";
    case frConstraintTypeEnum::frcSpacingTableConstraint:
      return os << "frcSpacingTableConstraint";
    case frConstraintTypeEnum::frcSpacingTableInfluenceConstraint:
      return os << "frcSpacingTableInfluenceConstraint";
    case frConstraintTypeEnum::frcSpacingTablePrlConstraint:
      return os << "frcSpacingTablePrlConstraint";
    case frConstraintTypeEnum::frcSpacingTableTwConstraint:
      return os << "frcSpacingTableTwConstraint";
    default:
      return os << "unknown viol";
  }
}

}  // end namespace fr
