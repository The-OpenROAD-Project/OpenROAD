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
#include "db/drObj/drShape.h"
#include "db/drObj/drVia.h"

using namespace std;
using namespace fr;

string GUIDE_FILE;
string OUTGUIDE_FILE;
string OUT_MAZE_FILE;
string DRC_RPT_FILE;
string CMAP_FILE;

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

frLayerNum VIAINPIN_BOTTOMLAYERNUM = std::numeric_limits<frLayerNum>::max();
frLayerNum VIAINPIN_TOPLAYERNUM = std::numeric_limits<frLayerNum>::max();
int MINNUMACCESSPOINT_MACROCELLPIN = 3;
int MINNUMACCESSPOINT_STDCELLPIN = 3;
int ACCESS_PATTERN_END_ITERATION_NUM = 5;

frLayerNum VIA_ACCESS_LAYERNUM = 2;

int END_ITERATION = 80;
int NDR_NETS_RIPUP_THRESH = 3;
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
frUInt4 FIXEDSHAPECOST = 30;
frUInt4 ROUTESHAPECOST = 8;
frUInt4 MARKERCOST = 32;
frUInt4 MARKERBLOATWIDTH = 1;
frUInt4 BLOCKCOST = 32;
frUInt4 GUIDECOST = 1;  // disabled change getNextPathCost to enable
float MARKERDECAY = 0.95;
float SHAPEBLOATWIDTH = 3;
int MISALIGNMENTCOST = 8;

int CONGCOST = 8;
int HISTCOST = 32;

namespace fr {

ostream& operator<<(ostream& os, const frPoint& pIn)
{
  os << "( " << pIn.x() << " " << pIn.y() << " )";
  return os;
}

ostream& operator<<(ostream& os, const frRect& pinFigIn)
{
  if (pinFigIn.getPin()) {
    os << "PINFIG (PINNAME/LAYER) " << pinFigIn.getPin()->getTerm()->getName()
       << " " << pinFigIn.getLayerNum() << endl;
  }
  frBox tmpBox;
  pinFigIn.getBBox(tmpBox);
  os << "  RECT " << tmpBox.left() << " " << tmpBox.bottom() << " "
     << tmpBox.right() << " " << tmpBox.top();
  return os;
}

ostream& operator<<(ostream& os, const frPolygon& pinFigIn)
{
  if (pinFigIn.getPin()) {
    os << "PINFIG (NAME/LAYER) " << pinFigIn.getPin()->getTerm()->getName()
       << " " << pinFigIn.getLayerNum() << endl;
  }
  os << "  POLYGON";
  for (auto& m : pinFigIn.getPoints()) {
    os << " ( " << m.x() << " " << m.y() << " )";
  }
  return os;
}

ostream& operator<<(ostream& os, const frPin& pinIn)
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

ostream& operator<<(ostream& os, const frTerm& termIn)
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
  cellName = instTermIn.getInst()->getRefBlock()->getName();
  termName = instTermIn.getTerm()->getName();
  if (instTermIn.getNet()) {
    netName = instTermIn.getNet()->getName();
  }
  os << "INSTTERM (NAME/CELL/TERM/NET) " << name << " " << cellName << " "
     << termName << " " << netName << endl;
  os << *instTermIn.getTerm();
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
  frBox box;
  blockIn.getBBox(box);
  os << "MACRO " << blockIn.getName() << endl
     << "  ORIGIN " << box.left() << " " << box.bottom() << endl
     << "  SIZE " << box.right() << " " << box.top();
  for (auto& m : blockIn.getTerms()) {
    os << endl << *m;
  }
  return os;
}

ostream& operator<<(ostream& os, const frInst& instIn)
{
  frPoint tmpPoint;
  frString tmpString;
  frString tmpName;
  instIn.getOrigin(tmpPoint);
  auto tmpOrient = instIn.getOrient();
  tmpName = instIn.getName();
  tmpString = instIn.getRefBlock()->getName();
  os << "- " << tmpName << " " << tmpString << " + STATUS + ( " << tmpPoint.x()
     << " " << tmpPoint.y() << " ) " << tmpOrient.getName() << endl;
  for (auto& m : instIn.getInstTerms()) {
    os << endl << *m;
  }
  return os;
}

ostream& operator<<(ostream& os, const frBox& box)
{
  os << "( " << box.left() << " " << box.bottom() << " ) ( " << box.right()
     << " " << box.top() << " )";
  return os;
}

ostream& operator<<(ostream& os, const drConnFig& fig)
{
  switch (fig.typeId()) {
    case drcPathSeg: {
      auto p = static_cast<const drPathSeg*>(&fig);
      os << "drPathSeg: begin (" << p->getBeginX() << " " << p->getBeginY()
         << " ) end ( " << p->getEndX() << " " << p->getEndY() << " )";
      break;
    }
    case drcVia: {
      auto p = static_cast<const drVia*>(&fig);
      os << "drVia: at " << p->getOrigin() << "\nVIA DEF:\n" << *p->getViaDef();
      break;
    }
    case drcPatchWire: {
      auto p = static_cast<const drPatchWire*>(&fig);
      frBox b;
      p->getBBox(b);
      os << "drPatchWire: " << b;
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
     << p.getEndPoint().y() << " )";
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
      os << p;
      break;
    }
    case frcVia: {
      auto p = static_cast<const frVia*>(&fig);
      os << "frVia: at " << p->getOrigin() << "\nVIA DEF:\n" << *p->getViaDef();
      break;
    }
    case frcPatchWire: {
      auto p = static_cast<const frPatchWire*>(&fig);
      frBox b;
      p->getBBox(b);
      os << "frPatchWire: " << b;
      break;
    }
    case frcGuide: {
      auto p = static_cast<const frGuide*>(&fig);
      os << p;
      break;
    }
    case frcRect: {
      auto p = static_cast<const frRect*>(&fig);
      frBox b;
      p->getBBox(b);
      os << "frRect: " << b;
      break;
    }
    case frcPolygon: {
      os << "frPolygon";
      break;
    }
    default:
      os << "UNKNOWN frShape, code " << fig.typeId();
  }
  return os;
}

}  // end namespace fr
