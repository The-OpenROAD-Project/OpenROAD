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

namespace drt {

std::ostream& operator<<(std::ostream& os, const frRect& pinFigIn)
{
  //  if (pinFigIn.getPin()) {
  //    os << "PINFIG (PINNAME/LAYER) " <<
  //    pinFigIn.getPin()->getTerm()->getName()
  //       << " " << pinFigIn.getLayerNum() << std::endl;
  //  }
  Rect tmpBox = pinFigIn.getBBox();
  os << "  RECT " << tmpBox.xMin() << " " << tmpBox.yMin() << " "
     << tmpBox.xMax() << " " << tmpBox.yMax();
  return os;
}

std::ostream& operator<<(std::ostream& os, const frPolygon& pinFigIn)
{
  //  if (pinFigIn.getPin()) {
  //    os << "PINFIG (NAME/LAYER) " << pinFigIn.getPin()->getTerm()->getName()
  //       << " " << pinFigIn.getLayerNum() << std::endl;
  //  }
  os << "  POLYGON";
  for (auto& m : pinFigIn.getPoints()) {
    os << " ( " << m.x() << " " << m.y() << " )";
  }
  return os;
}

std::ostream& operator<<(std::ostream& os, const frMPin& pinIn)
{
  os << "PIN (NAME) " << pinIn.getTerm()->getName();
  for (auto& m : pinIn.getFigs()) {
    if (m->typeId() == frcRect) {
      os << std::endl << *(static_cast<frRect*>(m.get()));
    } else if (m->typeId() == frcPolygon) {
      os << std::endl << *(static_cast<frPolygon*>(m.get()));
    } else {
      os << std::endl << "Unsupported pinFig object!";
    }
  }
  return os;
}

std::ostream& operator<<(std::ostream& os, const frBTerm& termIn)
{
  frString name;
  frString netName;
  name = termIn.getName();
  if (termIn.getNet()) {
    netName = termIn.getNet()->getName();
  }
  os << "TERM (NAME/NET) " << name << " " << netName;
  for (auto& m : termIn.getPins()) {
    os << std::endl << *m;
  }
  return os;
}

std::ostream& operator<<(std::ostream& os, const frInstTerm& instTermIn)
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
     << termName << " " << netName << std::endl;
  return os;
}

std::ostream& operator<<(std::ostream& os, const frViaDef& viaDefIn)
{
  frString name;
  name = viaDefIn.getName();
  os << "VIA " << name;
  if (viaDefIn.getDefault()) {
    os << " DEFAULT";
  }
  for (auto& m : viaDefIn.getLayer1Figs()) {
    if (m->typeId() == frcRect) {
      os << std::endl << *(static_cast<frRect*>(m.get()));
    } else if (m->typeId() == frcPolygon) {
      os << std::endl << *(static_cast<frPolygon*>(m.get()));
    } else {
      os << std::endl << "Unsupported pinFig object!";
    }
  }
  for (auto& m : viaDefIn.getCutFigs()) {
    if (m->typeId() == frcRect) {
      os << std::endl << *(static_cast<frRect*>(m.get()));
    } else if (m->typeId() == frcPolygon) {
      os << std::endl << *(static_cast<frPolygon*>(m.get()));
    } else {
      os << std::endl << "Unsupported pinFig object!";
    }
  }
  for (auto& m : viaDefIn.getLayer2Figs()) {
    if (m->typeId() == frcRect) {
      os << std::endl << *(static_cast<frRect*>(m.get()));
    } else if (m->typeId() == frcPolygon) {
      os << std::endl << *(static_cast<frPolygon*>(m.get()));
    } else {
      os << std::endl << "Unsupported pinFig object!";
    }
  }
  return os;
}

std::ostream& operator<<(std::ostream& os, const frBlock& blockIn)
{
  Rect box = blockIn.getBBox();
  os << "MACRO " << blockIn.getName() << std::endl
     << "  ORIGIN " << box.xMin() << " " << box.yMin() << std::endl
     << "  SIZE " << box.xMax() << " " << box.yMax();
  for (auto& m : blockIn.getTerms()) {
    os << std::endl << *m;
  }
  return os;
}

std::ostream& operator<<(std::ostream& os, const frInst& instIn)
{
  Point tmpPoint = instIn.getOrigin();
  auto tmpOrient = instIn.getOrient();
  const frString& tmpName = instIn.getName();
  frString tmpString = instIn.getMaster()->getName();
  os << "- " << tmpName << " " << tmpString << " + STATUS + ( " << tmpPoint.x()
     << " " << tmpPoint.y() << " ) " << tmpOrient.getString() << std::endl;
  for (auto& m : instIn.getInstTerms()) {
    os << std::endl << *m;
  }
  return os;
}

std::ostream& operator<<(std::ostream& os, const drConnFig& fig)
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

std::ostream& operator<<(std::ostream& os, const frPathSeg& p)
{
  os << "frPathSeg: begin (" << p.getBeginPoint().x() << " "
     << p.getBeginPoint().y() << " ) end ( " << p.getEndPoint().x() << " "
     << p.getEndPoint().y() << " ) layerNum " << p.getLayerNum();
  frSegStyle st = p.getStyle();
  os << "\n\tbeginStyle: " << st.getBeginStyle()
     << "\n\tendStyle: " << st.getEndStyle();
  return os;
}

std::ostream& operator<<(std::ostream& os, const frGuide& p)
{
  os << "frGuide: begin " << p.getBeginPoint() << " end " << p.getEndPoint()
     << " begin LayerNum " << p.getBeginLayerNum() << " end layerNum "
     << p.getEndLayerNum();
  return os;
}

std::ostream& operator<<(std::ostream& os, const frConnFig& fig)
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

std::ostream& operator<<(std::ostream& os, const frBlockObject& fig)
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

std::ostream& operator<<(std::ostream& os, const frNet& n)
{
  os << "frNet " << n.getName() << (n.isClock() ? "CLOCK NET " : " ")
     << (n.hasNDR() ? "NDR " + n.getNondefaultRule()->getName() + " " : " ")
     << "abs priority " << n.getAbsPriorityLvl() << "\n";
  return os;
}

std::ostream& operator<<(std::ostream& os, const drNet& n)
{
  os << "drNet " << *n.getFrNet() << "\nnRipupAvoids " << n.getNRipupAvoids()
     << " maxRipupAvoids " << n.getMaxRipupAvoids() << "\n";
  return os;
}

std::ostream& operator<<(std::ostream& os, const frMarker& m)
{
  return os << "MARKER: box " << m.getBBox() << " lNum " << m.getLayerNum()
            << " constraint: " << m.getConstraint()->typeId();
}

}  // end namespace drt
