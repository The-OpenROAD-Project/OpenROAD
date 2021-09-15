///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2019, Nefelus Inc
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#include <wire.h>

#include <map>
#include <vector>

#include "rcx/extRCap.h"
#include "rcx/extSpef.h"
#include "utl/Logger.h"

#define MAXINT 0x7FFFFFFF;

namespace rcx {

#ifdef DEBUG_NET_ID
FILE* fp;
#endif

using utl::RCX;

using odb::dbBlock;
using odb::dbBox;
using odb::dbBTerm;
using odb::dbCapNode;
using odb::dbCCSeg;
using odb::dbChip;
using odb::dbNet;
using odb::dbRSeg;
using odb::dbSet;
using odb::dbShape;
using odb::dbSigType;
using odb::dbTech;
using odb::dbTechLayer;
using odb::dbTechLayerDir;
using odb::dbTechLayerType;
using odb::dbWire;
using odb::dbWirePath;
using odb::dbWirePathItr;
using odb::dbWirePathShape;
using odb::ISdb;
using odb::Rect;
using odb::ZPtr;

extDebugNet::extDebugNet(dbNet* net, dbBlock* block)
{
  _debug = false;
  _net = net;
  _block = block;
  _nodeMap = new extCapNodeHash(100000);

  _levelCnt = 32;
  _rects = new Ath__array1D<extListWire*>*[_levelCnt];
  _vias = new Ath__array1D<extListWire*>*[_levelCnt];
  for (int ii = 0; ii < _levelCnt; ii++) {
    _rects[ii] = new Ath__array1D<extListWire*>(8);
    _vias[ii] = new Ath__array1D<extListWire*>(4);
  }
  _shapes = new Ath__array1D<extListWire*>(4096);
  _hashNodeRC = new HashNode(1000000, 4000, 100000);
}
bool extDebugNet::writeCapNode(uint capNodeId)
{
  odb::dbCapNode* capNode = odb::dbCapNode::getCapNode(_block, capNodeId);
  return writeCapNode(capNode);
}
bool extDebugNet::writeBTerm(uint node)
{
  odb::dbBTerm* bterm = odb::dbBTerm::getBTerm(_block, node);
  fprintf(_connFP, "B%d %s ", bterm->getId(), bterm->getName().c_str());
}
void extDebugNet::writeITermNode(uint node)
{
  dbITerm* iterm = dbITerm::getITerm(_block, node);
  dbInst* inst = iterm->getInst();
  fprintf(_connFP,
          "I%d %s %s ",
          iterm->getId(),
          iterm->getMTerm()->getConstName(),
          inst->getConstName());
}
bool extDebugNet::writeCapNode(dbCapNode* capNode, const char* postFix)
{
  uint nodeNum = capNode->getNode();
  if (capNode->isITerm()) {
    writeITermNode(nodeNum);
  } else if (capNode->isBTerm()) {
    writeBTerm(nodeNum);
  } else if (capNode->isInternal()) {
    fprintf(_connFP, "%d ", nodeNum);
  }
  if (postFix != NULL)
    fprintf(_connFP, "%s", postFix);

  return true;
}
void extDebugNet::printBTerm(uint node)
{
  odb::dbBTerm* bterm = odb::dbBTerm::getBTerm(_block, node);
  fprintf(stdout, "B%d %s ", bterm->getId(), bterm->getName().c_str());
}
void extDebugNet::printITermNode(uint node)
{
  dbITerm* iterm = dbITerm::getITerm(_block, node);
  dbInst* inst = iterm->getInst();
  fprintf(stdout,
          "I%d %s %s ",
          iterm->getId(),
          iterm->getMTerm()->getConstName(),
          inst->getConstName());
}
bool extDebugNet::printCapNode(dbCapNode* capNode)
{
  if (capNode->isITerm()) {
    printITermNode(capNode->getNode());
  } else if (capNode->isBTerm()) {
    printBTerm(capNode->getNode());
  } else if (capNode->isInternal()) {
    fprintf(stdout, "%d ", capNode->getNode());
  }
  return true;
}

void extDebugNet::printRC(dbRSeg* rc)
{
  fprintf(_connFP, "\t%d %d\n", rc->getSourceNode(), rc->getTargetNode());
  fprintf(_connFP, "\t");
  if (rc->getSourceNode() > 0)
    writeCapNode(rc->getSourceNode());
  fprintf(_connFP, " -- ");
  writeCapNode(rc->getTargetNode());
  fprintf(_connFP, "\n");

  dbShape s;
  // dbWire* w = rc->getNet()->getWire(); // NOT WORKING right
  dbWire* w = _net->getWire();

  int shapeId = rc->getShapeId();
  if (shapeId == 0) {
    fprintf(_connFP, "\tWARN shapeId=0\n");
    return;
  }
  w->getShape(shapeId, s);
  printShape(s, shapeId, false);
  printShape(s, shapeId, true);
}
void extDebugNet::printShape(dbShape& s, int shapeId, bool dbFactor)
{
  Rect r;
  s.getBox(r);
  uint level = 0;
  uint topLevel = 0;
  const char* viaName = "   ";
  if (s.isVia()) {
    viaName = "VIA";
  } else {
    level = s.getTechLayer()->getRoutingLevel();
  }
  printRect(r, shapeId, viaName, level, topLevel, dbFactor);
}
void extDebugNet::printRect(Rect& r,
                            int shapeId,
                            const char* viaName,
                            uint level,
                            uint topLevel,
                            bool dbFactor)
{
  int dx = r.dx();
  int dy = r.dy();
  if (!dbFactor) {
    fprintf(_connFP,
            "\t\t%d %d  %d %d  DX=%d DY=%d L=%d %d %s  shapeId= %d\n",
            r.xMin(),
            r.xMax(),
            r.yMin(),
            r.yMax(),
            dx,
            dy,
            level,
            topLevel,
            viaName,
            shapeId);
  } else {
    fprintf(
        _connFP,
        "\t\t%.3f %.3f  %.3f %.3f  DX=%.3f DY=%.3f L=%d %d %s  shapeId= %d\n",
        GetDBcoords(r.xMin()),
        GetDBcoords(r.xMax()),
        GetDBcoords(r.yMin()),
        GetDBcoords(r.yMax()),
        GetDBcoords(dx),
        GetDBcoords(dy),
        level,
        topLevel,
        viaName,
        shapeId);
  }
}
double extDebugNet::GetDBcoords(int coord)
{
  int db_factor = _block->getDbUnitsPerMicron();
  return 1.0 * coord / db_factor;
}
void extDebugNet::printShapes(bool dbunits)
{
  if (!_debug)
    return;
  fprintf(_connFP, "SHAPES %d %s ", _net->getId(), _net->getConstName());
  if (dbunits)
    fprintf(_connFP, " DB UNITS\n");
  else
    fprintf(_connFP, " COORDS\n");
  dbWire* wire = _net->getWire();
  dbWireShapeItr shapes;
  dbShape s;
  for (shapes.begin(wire); shapes.next(s);) {
    int shapeId = shapes.getShapeId();
    char buff[32];
    uint level = 0;
    if (s.isVia()) {
      sprintf(buff, "VIA");
    } else {
      level = s.getTechLayer()->getRoutingLevel();
      sprintf(buff, "M%d", level);
    }
    printShape(s, shapeId, dbunits);
  }
}
void extDebugNet::getRects()
{
  dbWire* wire = _net->getWire();
  dbWireShapeItr shapes;
  dbShape s;
  for (shapes.begin(wire); shapes.next(s);) {
    int shapeId = shapes.getShapeId();

    extListWire* a = new extListWire(shapeId, s, _block->getDbUnitsPerMicron());
    if (a->_topLevel == 0)
      _rects[a->level()]->add(a);
    else
      _vias[a->level()]->add(a);
    _shapes->add(a);
  }
  printViasWires(
      "\n------------------------------ Wires and Vias "
      "------------------------------\n");
  // vias2vias2wires2terms2wires();
  termsViasWiresWires();
}
void extDebugNet::vias2vias2wires2terms2wires()
{
  // DO NOT CHANGE ORDER
  uint nodeCnt = 1;
  intersectVias(nodeCnt);
  // nodeCnt= 2000;
  intersectRects_top(nodeCnt);
  // nodeCnt= 3000;
  intersectRects_bottom(nodeCnt);

  printViasWires(
      "\n--------- Net Connectivity ------------------------------\n");

  connectIterms();
  connectBterms();
  printViasWires(
      "\n----------- Net Connectivity after Bterms "
      "------------------------------\n");
  intersectRect2Rect(nodeCnt);
  printViasWires(
      "\n--------- Net Connectivity after wire to wire "
      "------------------------------\n");
  intersectRect2Rect_open(nodeCnt);
  printViasWires(
      "\n------ Net Connectivity after wire to wire top "
      "------------------------------\n");
}
void extDebugNet::termsViasWiresWires()
{
  connectIterms();
  connectBterms();
  printViasWires(
      "\n------------------------------ Net Connectivity after Iterms/Bterms "
      "------------------------------\n");
  uint nodeCnt = 1;
  intersectVias(nodeCnt);
  printViasWires(
      "\n------------------------------ Net Connectivity after stacked vias "
      "------------------------------\n");

  intersectRects_top(nodeCnt);
  intersectRects_bottom(nodeCnt);
  printViasWires(
      "\n------------------------------ Net Connectivity after Vias/Wires "
      "------------------------------\n");
  intersectRect2Rect(nodeCnt);
  printViasWires(
      "\n------------------------------ Net Connectivity after wire to wire "
      "------------------------------\n");
  intersectRect2Rect_open(nodeCnt);
  printViasWires(
      "\n------------------------------ Net Connectivity after wire to wire "
      "Open ------------------------------\n");
}
bool extDebugNet::connectIterms(uint options)
{
  uint cnt = 0;
  dbSet<dbITerm> iterms = _net->getITerms();
  dbSet<dbITerm>::iterator iterm_itr;
  for (iterm_itr = iterms.begin(); iterm_itr != iterms.end(); ++iterm_itr) {
    dbITerm* iterm = *iterm_itr;
    if (_debug) {
      fprintf(_connFP,
              "connectIterms: %s %s \n",
              iterm->getMTerm()->getConstName(),
              iterm->getInst()->getConstName());
      fprintf(stdout,
              "connectIterms: %s %s \n",
              iterm->getMTerm()->getConstName(),
              iterm->getInst()->getConstName());
    }
    bool found = false;
    for (uint level = 1; level < _levelCnt; level++) {
      if (connectIterm(iterm, level)) {
        found = true;
        break;
      }
    }
    if (found)
      cnt++;
    else {
      fprintf(_connFP,
              "\t====> UNCONNECTED ITERM: %s %s \n",
              iterm->getMTerm()->getConstName(),
              iterm->getInst()->getConstName());
    }
  }
  if (iterms.size() == cnt)
    return true;

  return false;
}
bool extDebugNet::connectIterm(dbITerm* iterm, uint level)
{
  bool found = false;
  dbITermShapeItr term_shapes;
  dbShape s;
  for (term_shapes.begin(iterm); term_shapes.next(s);) {
    if (s.isVia())
      continue;

    if (level != s.getTechLayer()->getRoutingLevel())
      continue;

    Rect pinRect;
    s.getBox(pinRect);
    if (_debug) {
      printShape(s, 0, false);
      printShape(s, 0, true);
    }
    if (intersectRect(iterm, s, true)) {
      found = true;
      break;
    }
  }
  return found;
}
bool extDebugNet::connectBterms(uint options)
{
  uint cnt = 0;
  dbSet<dbBTerm> bterms = _net->getBTerms();
  dbSet<dbBTerm>::iterator bterm_itr;
  for (bterm_itr = bterms.begin(); bterm_itr != bterms.end(); ++bterm_itr) {
    dbBTerm* bt = *bterm_itr;
    dbShape s;
    if (bt->getFirstPin(s)) {
      Rect pinRect;
      s.getBox(pinRect);
      uint level = s.getTechLayer()->getRoutingLevel();
      if (intersectRect(bt, s, true))
        cnt++;
      else {
        fprintf(_connFP, "====> UNCONNECTED BTERM: %s \n", bt->getConstName());
      }
    }
  }
  if (bterms.size() == cnt)
    return true;

  return false;
}
bool extListWire::overlapPinShape(int id, Rect pinRect)
{
  Rect r2 = *_rect;
  Point loCoord = r2.low();
  Point hiCoord = r2.high();

  if (_src == 0) {
    if (pinRect.intersects(loCoord)) {
      _src = -id;
      _btermFlag_src = true;
      return true;
    }
  }
  if (_dst == 0) {
    if (pinRect.intersects(hiCoord)) {
      _dst = -id;
      _btermFlag_dst = true;
      return true;
    }
  }
  return false;
}
bool extDebugNet::intersectRect(dbBTerm* b, dbShape& s, bool openEndedWires)
{
  // TODO: Vias
  bool found = false;
  Rect pinRect;
  s.getBox(pinRect);
  uint level = s.getTechLayer()->getRoutingLevel();

  for (uint kk = 0; kk < _rects[level]->getCnt(); kk++) {
    extListWire* t = _rects[level]->get(kk);
    if (openEndedWires && (t->_src == 0 || t->_dst == 0)) {
      if (t->overlapPinShape(b->getId(), pinRect)) {
        fprintf(_connFP, "\nBterm Wire Conn ------------------------------\n");
        printRect(pinRect, b->getId(), "BTERM", level, 0, true);
        t->printRect(_connFP, true);
        found = true;
        break;
      }
      continue;
    }
  }
  return found;
}
bool extListWire::overlapItermShape_via(int id, Rect pinRect, bool bottom)
{
  // Assumption: bottom via layer connecting with iterm layer so bottom NOT used
  Rect r2 = *_rect;  // this is via
  // Point loCoord= r2.low();
  // Point hiCoord= r2.high();
  bool overlap = r2.overlaps(pinRect);
  if (overlap) {
    if (_src == 0) {
      _src = -id;
      _itermFlag_src = true;
      return true;
    } else {
      return false;
    }
  }
  return false;
}
bool extListWire::overlapItermShape(int id, Rect pinRect)
{
  Rect r2 = *_rect;
  Point loCoord = r2.low();
  Point hiCoord = r2.high();
  bool overlap = r2.overlaps(pinRect);
  if (!overlap)
    return false;

  bool loConn = pinRect.intersects(loCoord);
  bool hiConn = pinRect.intersects(hiCoord);
  if (loConn && hiConn) {
    if (_src == 0) {
      _src = -id;
      _itermFlag_src = true;
    } else if (_dst == 0) {
      _dst = -id;
      _itermFlag_dst = true;
    } else {
      _src = -id;
      _itermFlag_src = true;
    }
  } else if (loConn) {
    if (_src == 0) {
      _src = -id;
      _itermFlag_src = true;
    } else if (_dst == 0) {
      _dst = -id;
      _itermFlag_dst = true;
    } else {
      _src = -id;
      _itermFlag_src = true;
      // TODO: Need to change all nodes=_src with iterm id
    }
  } else if (hiConn) {
    if (_dst == 0) {
      _dst = -id;
      _itermFlag_dst = true;
    } else if (_src == 0) {
      _src = -id;
      _itermFlag_src = true;
    } else {
      _dst = -id;
      _itermFlag_dst = true;
      // TODO: Need to change all nodes=_dst with iterm id
    }
  } else {
    return false;
  }
  return true;
}
bool extDebugNet::intersectRect(dbITerm* b, dbShape& s, bool openEndedWires)
{
  bool found = false;
  Rect pinRect;
  s.getBox(pinRect);
  uint level = s.getTechLayer()->getRoutingLevel();

  for (uint kk = 0; kk < _vias[level]->getCnt(); kk++) {
    extListWire* t = _vias[level]->get(kk);
    if (t->_src != 0)
      continue;  // TODO: Assume bottom layer connecetion with iterm shape
    if (_debug) {
      t->printRect(_connFP, true);
      t->printRect(stdout, true);
    }
    // if (t->overlapItermShape(b->getId(), pinRect)) {
    if (t->overlapItermShape_via(b->getId(), pinRect, level == t->level())) {
      fprintf(_connFP, "\nIterm Wire Conn ------------------------------\n");
      printRect(pinRect, b->getId(), "iTERM", level, 0, true);
      t->printRect(_connFP, true);
      found = true;
      break;
    }
    continue;
  }
  if (found)
    return true;

  for (uint kk = 0; kk < _rects[level]->getCnt(); kk++) {
    extListWire* t = _rects[level]->get(kk);
    if (_debug) {
      fprintf(_connFP, "WIRE: ");
      t->printRect(_connFP, true);
    }
    if (openEndedWires && (t->_src == 0 || t->_dst == 0)) {
      if (t->overlapItermShape(b->getId(), pinRect)) {
        fprintf(_connFP, "\nIterm Wire Conn ------------------------------\n");
        printRect(pinRect, b->getId(), "I TERM", level, 0, true);
        t->printRect(_connFP, true);
        found = true;
        break;
      }
      continue;
    }
  }
  return found;
}
void extDebugNet::printViasWires(const char* header)
{
  if (!_debug)
    return;

  fprintf(_connFP, "%s", header);
  printRects(false, _rects);
  printRects(false, _vias);
  fprintf(_connFP, "\n");
  printRects(true, _rects);
  printRects(true, _vias);
}
void extDebugNet::printRects(bool dbFactor, Ath__array1D<extListWire*>** rects)
{
  for (uint jj = 0; jj < _levelCnt; jj++) {
    for (uint ii = 0; ii < rects[jj]->getCnt(); ii++) {
      extListWire* a = rects[jj]->get(ii);
      uint l = a->level();
      a->printRect(_connFP, dbFactor);
    }
  }
}

HashNode::HashNode(uint itermCnt, uint btermCnt, uint nodeCnt)
{
  btermCnt = getMultiples(btermCnt, 1024);
  itermCnt = getMultiples(itermCnt, 1024);

  _btermTable = new Ath__array1D<int>(btermCnt);
  _itermTable = new Ath__array1D<int>(itermCnt);
  _nodeTable = new Ath__array1D<int>(16000);
}
void HashNode::resetMapping(int btermId, int itermId, uint junction)
{
  _btermTable->set(btermId, 0);
  _itermTable->set(itermId, 0);
  _nodeTable->set(junction, 0);
}
void HashNode::setMapping(bool bterm, bool iterm, int n, int v)
{
  if (bterm)
    _btermTable->set(-n, v);
  else if (iterm)
    _itermTable->set(-n, v);
  else
    _nodeTable->set(n, v);
}
int HashNode::getMapping(bool bterm, bool iterm, int n)
{
  if (bterm)
    return _btermTable->geti(-n);
  else if (iterm)
    return _itermTable->geti(-n);
  else
    return _nodeTable->geti(n);
}
void extDebugNet::makeRsegs(Ath__array1D<extListWire*>* rects)
{
  make1stRseg(rects);
  for (uint ii = 0; ii < rects->getCnt(); ii++) {
    extListWire* a = rects->get(ii);

    /*
    dbShape s;
    dbWire* w = _net->getWire();

    w->getShape(a->_shapeId, s);
    printShape(s, a->_shapeId, false);
    */

    if (a->_src == 0 && a->_dst == 0)
      continue;  // TODO
    int n1 = a->getCapNode_src(_net, _hashNodeRC);
    int n2 = a->getCapNode_dst(_net, _hashNodeRC);

    uint length;
    uint pathDir = 0;
    // uint pathDir = computePathDir(prevPoint, pshape.point, &length);
    int jx = 0;
    int jy = 0;
    // if (pshape.junction_id)
    //  net->getWire()->getCoord((int) pshape.junction_id, jx, jy);
    dbRSeg* rc = dbRSeg::create(_net, jx, jy, pathDir, true);

    rc->setSourceNode(n1);
    rc->setTargetNode(n2);

    rc->setShapeId(a->_shapeId);
  }
  dbSet<dbRSeg> rSet = _net->getRSegs();
  rSet.reverse();
}
uint extDebugNet::getTermCapNode(extListWire* a, bool src, bool anyIOtype)
{
  uint capId = 0;
  int n1 = src ? a->isOutTerm_src() : a->isOutTerm_dst();
  if (n1 < 0) {
    dbITerm* it1 = dbITerm::getITerm(_block, -n1);
    if (it1->isOutputSignal() || anyIOtype) {
      capId = src ? a->getCapNode_src(_net, _hashNodeRC)
                  : a->getCapNode_dst(_net, _hashNodeRC);
    }
  }
  return capId;
}
uint extDebugNet::getBTermCapNode(extListWire* a, bool src)
{
  uint capId = 0;
  int n1 = src ? a->isOutBTerm_src() : a->isOutBTerm_dst();
  if (n1 < 0) {
    dbBTerm* it1 = dbBTerm::getBTerm(_block, -n1);
    capId = src ? a->getCapNode_src(_net, _hashNodeRC)
                : a->getCapNode_dst(_net, _hashNodeRC);
  }
  return capId;
}

bool extDebugNet::make1stRseg(Ath__array1D<extListWire*>* rects)
{
  extListWire* a = NULL;
  uint capId = 0;
  for (uint ii = 0; ii < rects->getCnt(); ii++) {
    a = rects->get(ii);
    if (!a->isTerm())
      continue;

    bool src = true;
    capId = getTermCapNode(a, src);
    if (capId > 0)
      break;
    capId = getTermCapNode(a, !src);
    if (capId > 0)
      break;

    src = true;
    capId = getBTermCapNode(a, src);
    if (capId > 0)
      break;
    capId = getBTermCapNode(a, !src);
    if (capId > 0)
      break;

    src = true;
    capId = getTermCapNode(a, src, true);
    if (capId > 0)
      break;
    capId = getTermCapNode(a, !src, true);
    if (capId > 0)
      break;
  }
  if (capId > 0) {
    if (_net->get1stRSegId())
      fprintf(_connFP,
              "Net %d %s already has rseg!\n",
              _net->getId(),
              _net->getConstName());

    int tx = 0, ty = 0;
    dbRSeg* rc = dbRSeg::create(_net, tx, ty, 0, true);
    rc->setTargetNode(capId);
    rc->setShapeId(a->_shapeId);
    return true;
  } else {
    fprintf(_connFP,
            "Net %d %s not connected to iterm or bterm\n",
            _net->getId(),
            _net->getConstName());

    for (uint ii = 0; ii < rects->getCnt(); ii++) {
      extListWire* a = rects->get(ii);
      int n1 = a->getCapNode_src(_net, _hashNodeRC);
      int tx = 0, ty = 0;
      dbRSeg* rc = dbRSeg::create(_net, tx, ty, 0, true);
      rc->setTargetNode(n1);
      rc->setShapeId(a->_shapeId);
      return true;
    }
  }
  return false;
}
int extListWire::getCapNode_src(dbNet* net, HashNode* nodeTable)
{
  int capId = getSrcNode(nodeTable);
  if (capId != 0)
    return capId;

  bool foreign = false;
  dbCapNode* cap = dbCapNode::create(net, 0, foreign);
  capId = cap->getId();

  if (_btermFlag_src)
    cap->setBTermFlag();
  else if (_itermFlag_src)
    cap->setITermFlag();
  else {
    cap->setInternalFlag();
    cap->setBranchFlag();
  }
  if (_src > 0)
    cap->setNode(_src);
  else
    cap->setNode(-_src);
  if (_src != 0)
    setSrcNode(nodeTable, capId);
  return capId;
}
int extListWire::getCapNode_dst(dbNet* net, HashNode* nodeTable)
{
  int capId = getDstNode(nodeTable);
  if (capId != 0)
    return capId;

  bool foreign = false;
  dbCapNode* cap = dbCapNode::create(net, 0, foreign);
  capId = cap->getId();

  if (_btermFlag_dst)
    cap->setBTermFlag();
  else if (_itermFlag_dst)
    cap->setITermFlag();
  else {
    cap->setInternalFlag();
    cap->setBranchFlag();
  }
  if (_dst > 0)
    cap->setNode(_dst);
  else
    cap->setNode(-_dst);
  if (_dst != 0)
    setDstNode(nodeTable, capId);
  return capId;
}
void extDebugNet::intersectVias(uint& nodeCnt)
{
  // NOTE: not two vias of same bot,top levels overlap!
  for (uint bot = 1; bot < _levelCnt - 1; bot++) {
    int cnt = _vias[bot]->getCnt();
    for (uint ii = 0; ii < cnt; ii++) {
      extListWire* v = _vias[bot]->get(ii);
      uint top = v->_topLevel;
      for (uint kk = 0; kk < _vias[top]->getCnt(); kk++) {
        extListWire* t = _vias[top]->get(kk);
        // if (v->overlapVias(t, nodeCnt)) {
        if (v->overlapVias2(t, nodeCnt)) {
          if (_debug) {
            fprintf(
                _connFP,
                "\n------------Stacked Vias ------------------------------\n");
            v->printRect(_connFP, true);
            t->printRect(_connFP, true);
          }
        }
      }
    }
  }
}
void extDebugNet::intersectRects_top(uint& nodeCnt)
{
  // NOTE: not two vias of same bot,top levels overlap!
  for (uint bot = 1; bot < _levelCnt - 1; bot++) {
    int cnt = _vias[bot]->getCnt();
    for (uint ii = 0; ii < cnt; ii++) {
      extListWire* v = _vias[bot]->get(ii);
      uint top = v->_topLevel;
      for (uint kk = 0; kk < _rects[top]->getCnt(); kk++) {
        extListWire* t = _rects[top]->get(kk);
        // if (v->overlapWires(t, true, nodeCnt)) {
        if (v->overlapVia2Wire(t, true, nodeCnt)) {
          if (_debug) {
            fprintf(
                _connFP,
                "\n------------ Via Wire TOP ------------------------------\n");
            v->printRect(_connFP, true);
            t->printRect(_connFP, true);
          }
        }
      }
    }
  }
}
void extDebugNet::intersectRects_bottom(uint& nodeCnt)
{
  // NOTE: not two vias of same bot,top levels overlap!
  for (uint bot = 1; bot < _levelCnt - 1; bot++) {
    int cnt = _vias[bot]->getCnt();
    for (uint ii = 0; ii < cnt; ii++) {
      extListWire* v = _vias[bot]->get(ii);
      for (uint kk = 0; kk < _rects[bot]->getCnt(); kk++) {
        extListWire* t = _rects[bot]->get(kk);
        // if (v->overlapWires(t, false, nodeCnt)) {
        if (v->overlapVia2Wire(t, false, nodeCnt)) {
          if (_debug) {
            fprintf(_connFP,
                    "\n------------ Via Wire BOTTOM "
                    "------------------------------\n");
            v->printRect(_connFP, true);
            t->printRect(_connFP, true);
          }
        }
      }
    }
  }
}
void extDebugNet::intersectRect2Rect(uint& nodeCnt)
{
  // TODO: optimize loop for only unconnected rects and sort by x or y
  for (uint bot = 1; bot < _levelCnt; bot++) {
    int cnt = _rects[bot]->getCnt();
    for (int ii = 0; ii < cnt - 1; ii++) {
      extListWire* v = _rects[bot]->get(ii);
      if (v->_src != 0 && v->_dst != 0)
        continue;
      bool found = true;
      for (uint kk = ii + 1; kk < cnt; kk++) {
        extListWire* t = _rects[bot]->get(kk);
        if (t->_src != 0 && t->_dst != 0)
          continue;

        // if (v->connectWires(t, nodeCnt)) {
        if (v->connectSquareWires(t, nodeCnt)) {
          found = true;
          break;
        }
      }
      if (_debug) {
        fprintf(_connFP,
                "\n------------ Rect Disconnected "
                "------------------------------\n");
        v->printRect(stdout, true);
        v->printRect(_connFP, true);
      }
    }
  }
}
void extDebugNet::intersectRect2Rect_open(uint& nodeCnt)
{
  // TODO: optimize loop for only unconnected rects and sort by x or y
  for (uint bot = 1; bot < _levelCnt; bot++) {
    int cnt = _rects[bot]->getCnt();
    for (int ii = 0; ii < cnt; ii++) {
      extListWire* v = _rects[bot]->get(ii);
      if (v->_src != 0 && v->_dst != 0)
        continue;
      bool found = true;
      for (uint kk = 0; kk < cnt; kk++) {
        extListWire* t = _rects[bot]->get(kk);
        if (v == t)
          continue;
        // if (t->_src != 0 && t->_dst !=0)
        // continue;

        // if (v->connectWires(t, nodeCnt)) {
        if (v->connectSquareWires(t, nodeCnt)) {
          found = true;
          break;
        }
      }
      if (_debug) {
        fprintf(_connFP,
                "\n------------ Rect Disconnected "
                "------------------------------\n");
        v->printRect(stdout, true);
        v->printRect(_connFP, true);
      }
    }
  }
}
bool extListWire::overlapVias2(extListWire* w, uint& nodeCnt)
{
  Rect r1 = *_rect;
  Rect r2 = *w->_rect;
  if (r1.overlaps(r2)) {
    _dst = nodeCnt++;
    w->_src = _dst;
  }
  return false;
}
bool extListWire::overlapVias(extListWire* w, uint& nodeCnt)
{
  Rect r1 = *_rect;
  Rect r2 = *w->_rect;
  if (r1.overlaps(r2)) {
    if (!useShapeId) {
      _dst = nodeCnt++;
      w->_src = _dst;
      return true;
    }
    if (_src == 0) {
      _src = _shapeId;
      _dst = nodeCnt++;
    } else if (_dst == 0) {
      _dst = nodeCnt++;
    }
    w->_src = _dst;
    w->_dst = w->_shapeId;
    return true;
  }
  return false;
}
bool extListWire::connectWires(extListWire* w, uint& nodeCnt)
{
  Rect r1 = *_rect;
  Rect r2 = *w->_rect;
  uint width = r2.minDXDY();
  Point loCoord = r1.low();
  Point hiCoord = r1.high();
  if (!r1.overlaps(r2))
    return false;
  // TODO: overlap unit rects around lo and hi coords
  if (_src == 0) {
    Rect* lo = new Rect(r1);
    lo->set_xhi(loCoord.getX() + width / 2);
    lo->set_yhi(loCoord.getY() + width / 2);
    if (lo->intersects(r2)) {
      _src = nodeCnt++;
      if (w->_src == 0)
        w->_src = _src;
      else if (w->_dst == 0)
        w->_dst = _src;
      return true;
    }
    return false;
  } else if (_dst == 0) {
    Rect* hi = new Rect(r1);
    hi->set_xhi(hiCoord.getX() - width / 2);
    hi->set_yhi(hiCoord.getY() - width / 2);
    if (hi->intersects(r2)) {
      _dst = nodeCnt++;
      if (w->_src == 0)
        w->_src = _dst;
      else if (w->_dst == 0)
        w->_dst = _dst;
      return true;
    }
    return false;
  }
  return false;
}
bool extListWire::connectSquareWires(extListWire* w, uint& nodeCnt)
{
  Rect r1 = *_rect;
  Rect r2 = *w->_rect;
  uint width = r2.minDXDY();
  Point loCoord = r1.low();
  Point hiCoord = r1.high();

  if (!r1.overlaps(r2))
    return false;

  Rect* lo2 = new Rect(r2);
  lo2->set_xhi(r2.low().getX() + width);
  lo2->set_yhi(r2.low().getY() + width);

  Rect* hi2 = new Rect(r2);
  hi2->set_xlo(r2.high().getX() - width);
  hi2->set_ylo(r2.high().getY() - width);

  if (_src == 0) {
    Rect* lo = new Rect(r1);
    lo->set_xhi(loCoord.getX() + width);
    lo->set_yhi(loCoord.getY() + width);
    if (lo->intersects(*lo2)) {
      if (w->_src == 0) {
        _src = nodeCnt++;
        w->_src = _src;
      } else
        _src = w->_src;
      return true;
    }
    if (lo->intersects(*hi2)) {
      if (w->_dst == 0) {
        _src = nodeCnt++;
        w->_dst = _src;
      } else
        _src = w->_dst;
      return true;
    }
    return false;
  } else if (_dst == 0) {
    Rect* hi = new Rect(r1);
    hi->set_xhi(hiCoord.getX() - width);
    hi->set_yhi(hiCoord.getY() - width);
    if (hi->intersects(*lo2)) {
      if (w->_src == 0) {
        _dst = nodeCnt++;
        w->_src = _dst;
      } else
        _dst = w->_src;
      return true;
    }
    if (hi->intersects(*hi2)) {
      if (w->_dst == 0) {
        _dst = nodeCnt++;
        w->_dst = _dst;
      } else
        _dst = w->_dst;
      return true;
    }
    return false;
  }
  return false;
}
int extListWire::setViaNode(bool upLayerConnection, uint& nodeCnt)
{
  // ASSUMPTION: stacked vias done first

  if (!useShapeId) {
    if (upLayerConnection) {
      if (_dst == 0)
        _dst = nodeCnt++;
      return _dst;
    } else {
      if (_src == 0)
        _src = nodeCnt++;
      return _src;
    }
  }
  if (_src == 0 && _dst == 0) {
    if (upLayerConnection) {
      _dst = nodeCnt++;
      _src = _shapeId;
      return _dst;
    } else {
      _src = _shapeId;
      return _src;
    }
  } else if (_src == 0) {
    if (!upLayerConnection) {
      return _dst;
    } else {
      _src = _shapeId;
      return _src;
    }
  } else {
    if (upLayerConnection) {
      if (_dst == 0)
        _dst = nodeCnt++;
      return _dst;
    } else {
      return _src;
    }
  }
  return 0;
}
bool extListWire::overlapWires(extListWire* w,
                               bool upLayerConnection,
                               uint& nodeCnt)
{
  Rect via = *_rect;
  Rect r2 = *w->_rect;
  Point loCoord = r2.low();
  Point hiCoord = r2.high();
  if (via.overlaps(loCoord)) {
    uint n = this->setViaNode(upLayerConnection, nodeCnt);
    if (n != w->_dst)
      w->_src = n;
    else
      ;
    return true;
  } else if (via.overlaps(hiCoord)) {
    uint n = this->setViaNode(upLayerConnection, nodeCnt);
    if (n != w->_src)
      w->_dst = n;
    else
      ;
    return true;
  }
  return false;
}
bool extListWire::overlapVia2Wire(extListWire* w,
                                  bool upLayerConnection,
                                  uint& nodeCnt)
{
  Rect via = *_rect;  // this == via
  Rect r2 = *w->_rect;
  uint width = r2.minDXDY();

  if (!via.overlaps(r2))
    return false;

  Rect* lo = new Rect(r2);
  lo->set_xhi(r2.low().getX() + width);
  lo->set_yhi(r2.low().getY() + width);
  if (via.contains(*lo)) {
    int n = this->setViaNode(upLayerConnection, nodeCnt);
    if (n != w->_dst) {
      w->_src = n;
      if (n < 0) {
        w->_btermFlag_src = isBTermSrc(n) || isBTermDst(n);
        w->_itermFlag_src = isITermSrc(n) || isITermDst(n);
      }
    }

    return true;
  }
  Rect* hi = new Rect(r2);
  hi->set_xlo(r2.high().getX() - width);
  hi->set_ylo(r2.high().getY() - width);
  if (via.contains(*hi)) {
    int n = this->setViaNode(upLayerConnection, nodeCnt);
    if (n != w->_src) {
      w->_dst = n;
      if (n < 0) {
        w->_btermFlag_dst = isBTermSrc(n) || isBTermDst(n);
        w->_itermFlag_dst = isITermSrc(n) || isITermDst(n);
      }
    }
    return true;
  }
  return false;
}
double extListWire::GetDBcoords(int coord)
{
  return 1.0 * coord / DB_FACTOR;
}
void extListWire::printRect(FILE* _connFP, bool dbFactor)
{
  const char* term = "";
  if (_btermFlag_src || _btermFlag_dst)
    term = "BTERM";
  else if (_itermFlag_src || _itermFlag_dst)
    term = "iTERM";

  Rect r = *_rect;
  int dx = r.dx();
  int dy = r.dy();
  fprintf(_connFP,
          "\t%12d %12d  %6ds L=%d %d",
          _src,
          _dst,
          _shapeId,
          _level,
          _topLevel);
  if (!dbFactor) {
    fprintf(_connFP,
            "   %d %d  %d %d  DX=%d DY=%d %s %s\n",
            r.xMin(),
            r.xMax(),
            r.yMin(),
            r.yMax(),
            dx,
            dy,
            _viaName,
            term);
  } else {
    fprintf(_connFP,
            "  %.3f %.3f  %.3f %.3f  DX=%.3f DY=%.3f %s %s\n",
            GetDBcoords(r.xMin()),
            GetDBcoords(r.xMax()),
            GetDBcoords(r.yMin()),
            GetDBcoords(r.yMax()),
            GetDBcoords(dx),
            GetDBcoords(dy),
            _viaName,
            term);
  }
}
extListWire::extListWire(int shapeId, dbShape& s, int units)
{
  _btermFlag_src = false;
  _btermFlag_dst = false;
  _itermFlag_src = false;
  _itermFlag_dst = false;
  DB_FACTOR = units;
  _shapeId = shapeId;
  _src = 0;
  _dst = 0;

  _level = 0;
  _topLevel = 0;

  if (s.isVia()) {
    uint width = 0;
    dbTechVia* tvia = s.getTechVia();
    if (tvia != NULL) {
      _level = tvia->getBottomLayer()->getRoutingLevel();
      _topLevel = tvia->getTopLayer()->getRoutingLevel();
      // width = tvia->getBottomLayer()->getWidth();
      // res = tvia->getResistance();
      _viaName = tvia->getConstName();

    } else {
      dbVia* bvia = s.getVia();
      if (bvia != NULL) {
        _level = bvia->getBottomLayer()->getRoutingLevel();
        _topLevel = bvia->getTopLayer()->getRoutingLevel();

        // width = bvia->getBottomLayer()->getWidth();
        _viaName = bvia->getConstName();
      }
    }
  } else {
    _level = s.getTechLayer()->getRoutingLevel();
    _viaName = "   ";
  }
  Rect r;
  s.getBox(r);
  _rect = new Rect(r);
}
void extListWire::setSrcNode(HashNode* hashNode, int n)
{
  hashNode->setMapping(_btermFlag_src, _itermFlag_src, _src, n);
}
void extListWire::setDstNode(HashNode* hashNode, int n)
{
  hashNode->setMapping(_btermFlag_dst, _itermFlag_dst, _dst, n);
}
int extListWire::getSrcNode(HashNode* hashNode)
{
  return hashNode->getMapping(_btermFlag_src, _itermFlag_src, _src);
}
int extListWire::getDstNode(HashNode* hashNode)
{
  return hashNode->getMapping(_btermFlag_dst, _itermFlag_dst, _dst);
}
void extDebugNet::resetNodes()
{
  for (uint ii = 0; ii < _shapes->getCnt(); ii++) {
    extListWire* a = _shapes->get(ii);
    a->setSrcNode(_hashNodeRC, 0);
    a->setDstNode(_hashNodeRC, 0);
  }
  _shapes->resetCnt();
}
void extDebugNet::resetRects()
{
  for (uint ii = 0; ii < _levelCnt; ii++) {
    _rects[ii]->resetCnt();
    _vias[ii]->resetCnt();
  }
  _shapes->resetCnt();
  resetNodes();
}
uint extDebugNet::debugNet(dbNet* net, uint debug_net_id, const char* name)
{
  resetRects();
  OpenConnFile(name);
  setNet(net);
  checkNet(debug_net_id);
  fclose(_connFP);
  return 0;
}
uint extDebugNet::netStats(FILE* fp, const char* prefix)
{
  uint wireCnt = 0;
  uint viaCnt = 0;
  _net->getSignalWireCount(wireCnt, viaCnt);
  dbSet<dbITerm> iterms = _net->getITerms();
  dbSet<dbBTerm> bterms = _net->getBTerms();
  uint termCnt = iterms.size() + bterms.size();

  fprintf(fp,
          "\n%s termCnt=%d bterm=%d wireCnt=%d viaCnt=%d  %d %s\n",
          prefix,
          _net->getTermCount(),
          bterms.size(),
          wireCnt,
          viaCnt,
          _net->getId(),
          _net->getConstName());
  return termCnt;
}
uint extDebugNet::checkNet(int debug_net_id)
{
  resetRects();
  bool verbose = true;
  // if (!_net->isDisconnected())
  //    return 0;
  if (debug_net_id != 0) {
    if (_net->getId() != debug_net_id)
      return 0;
  }
  if (_debug) {
    _net->printWnP("net");
    checkConnOrdered(_net, verbose);
  }

  printShapes(false);
  printShapes(true);

  getRects();
  makeRsegs(_shapes);
  dbSet<dbRSeg> rSet = _net->getRSegs();
  // rSet.reverse();

  uint wireCnt = 0;
  uint viaCnt = 0;
  _net->getSignalWireCount(wireCnt, viaCnt);
  dbSet<dbITerm> iterms = _net->getITerms();
  dbSet<odb::dbBTerm> bterms = _net->getBTerms();
  uint termCnt = iterms.size() + bterms.size();

  if (_debug) {
    fprintf(_connFP,
            "\nNET termCount=%d wireCnt=%d viaCnt=%d rCnt=%d %d %s\n",
            _net->getTermCount(),
            wireCnt,
            viaCnt,
            rSet.size(),
            _net->getId(),
            _net->getConstName());
    uint cnt = 1;
    odb::dbSet<odb::dbRSeg>::iterator rc_itr;
    for (rc_itr = rSet.begin(); rc_itr != rSet.end(); ++rc_itr) {
      odb::dbRSeg* rc = *rc_itr;
      printRC(rc);
      cnt++;
    }
  }
  _nodeMap->init(_net, this, _connFP);
  if (_debug)
    _nodeMap->print(_connFP);
  _nodeMap->_debug = _debug;
  return _nodeMap->traverse();
}
uint extDebugNet::CheckConnectivity(bool verbose)
{
  if (_net->getTermCount() > 2)
    return 0;
  _nodeMap->init(_net, this, _connFP);
  if (verbose)
    _nodeMap->print(_connFP);
  uint cnt = _nodeMap->traverse();
  return cnt;
}
void extCapNodeHash::alloc(uint n)
{
  _maxSize = n;
  _table = new extListNode*[n];
  for (uint ii = 0; ii < n; ii++)
    _table[ii] = NULL;
}
extCapNodeHash::extCapNodeHash(uint n)
{
  _maxSize = n;
  _table = new extListNode*[n];
  for (uint ii = 0; ii < n; ii++)
    _table[ii] = NULL;
}
void extCapNodeHash::init(dbNet* net, extDebugNet* debugNet, FILE* fp)
{
  _connFP = fp;
  _net = net;
  _debugNet = debugNet;
  _sourceIndex = -1;
  _min = MAXINT;
  _max = 0;
  dbSet<dbRSeg> rSet = net->getRSegs();
  dbSet<dbRSeg>::iterator rc_itr;
  for (rc_itr = rSet.begin(); rc_itr != rSet.end(); ++rc_itr) {
    dbRSeg* rc = *rc_itr;

    uint n1 = rc->getSourceNode();
    uint node1 = rc->getSourceCapNode()->getNode();
    uint n2 = rc->getTargetNode();
    uint node2 = rc->getTargetCapNode()->getNode();
    getMin(n1);
    getMin(n2);
    getMax(n1);
    getMax(n2);
  }
  if (_max == 0)
    return;
  _count = _max - _min + 1;

  int cnt = _count;
  if (_count > _maxSize)
    cnt = _maxSize;

  for (uint ii = 0; ii < cnt; ii++) {
    if (_table[ii] != NULL) {
      // delete _table[ii];
      _table[ii] = NULL;
    }
  }
  if (_count >= _maxSize)
    alloc(_count);

  for (rc_itr = rSet.begin(); rc_itr != rSet.end(); ++rc_itr) {
    dbRSeg* rc = *rc_itr;
    uint n1 = rc->getSourceNode();
    addNode(n1, rc, rc->getSourceCapNode());
    uint n2 = rc->getTargetNode();
    addNode(n2, rc, rc->getTargetCapNode());
  }
}
void extCapNodeHash::print(FILE* fp)
{
  fprintf(fp, "Node Map ------------------------------ \n");
  for (uint ii = 0; ii < _count; ii++) {
    if (_table[ii] == NULL)
      break;

    fprintf(fp, "CapNode: map=%d %d : ", ii, ii + _min);
    extListNode* e = _table[ii];
    if (e->getNode()->isSourceTerm())
      fprintf(fp, "   SOURCE ===> ");
    _debugNet->writeCapNode(e->getNode());

    for (; e != NULL; e = e->getNext()) {
      _debugNet->printRC(e->getRC());
    }
  }
}
void extCapNodeHash::resetVisited()
{
  for (uint ii = 0; ii < _count; ii++) {
    if (_table[ii] == NULL)
      break;

    extListNode* e = _table[ii];
    e->resetVisited();
  }
}
extListNode::extListNode(dbRSeg* rc, extListNode* next, dbCapNode* node)
{
  _rc = rc;
  _next = next;
  _capNode = node;
  _nextCapNode = _rc->getTargetCapNode();
  if (_nextCapNode == _capNode) {
    _nextCapNode = _rc->getSourceCapNode();
  }
  _visited = false;
}
void extCapNodeHash::unmarkAllTerms()
{
  dbSet<dbITerm> iterms = _net->getITerms();
  dbSet<dbITerm>::iterator term_itr;
  for (term_itr = iterms.begin(); term_itr != iterms.end(); ++term_itr) {
    dbITerm* t = *term_itr;
    t->setMark(0);
  }
  dbSet<dbBTerm> bterms = _net->getBTerms();
  dbSet<dbBTerm>::iterator bterm_itr;
  for (bterm_itr = bterms.begin(); bterm_itr != bterms.end(); ++bterm_itr) {
    dbBTerm* t = *bterm_itr;
    t->setMark(0);
  }
}
void extCapNodeHash::checkUmarkedTerms()
{
  fprintf(_connFP, "Unmarked Terms:\n");
  dbSet<dbITerm> iterms = _net->getITerms();
  dbSet<dbITerm>::iterator term_itr;
  for (term_itr = iterms.begin(); term_itr != iterms.end(); ++term_itr) {
    dbITerm* iterm = *term_itr;
    if (!iterm->isSetMark()) {
      fprintf(_connFP,
              "I%d %s %s \n",
              iterm->getId(),
              iterm->getMTerm()->getConstName(),
              iterm->getInst()->getConstName());
    }
  }
  dbSet<dbBTerm> bterms = _net->getBTerms();
  dbSet<dbBTerm>::iterator bterm_itr;
  for (bterm_itr = bterms.begin(); bterm_itr != bterms.end(); ++bterm_itr) {
    dbBTerm* bterm = *bterm_itr;
    if (!bterm->isSetMark())
      fprintf(_connFP, "B%d %s \n", bterm->getId(), bterm->getName().c_str());
  }
}
uint extCapNodeHash::countUmarkedTerms()
{
  uint cnt = 0;
  dbSet<dbITerm> iterms = _net->getITerms();
  dbSet<dbITerm>::iterator term_itr;
  for (term_itr = iterms.begin(); term_itr != iterms.end(); ++term_itr) {
    dbITerm* iterm = *term_itr;
    if (!iterm->isSetMark()) {
      cnt++;
    }
  }
  dbSet<dbBTerm> bterms = _net->getBTerms();
  dbSet<dbBTerm>::iterator bterm_itr;
  for (bterm_itr = bterms.begin(); bterm_itr != bterms.end(); ++bterm_itr) {
    dbBTerm* bterm = *bterm_itr;
    if (!bterm->isSetMark())
      cnt++;
  }
  return cnt;
}
uint extCapNodeHash::traverse()
{
  if (_sourceIndex < 0)
    return 0;

  unmarkAllTerms();
  resetVisited();
  dfs(_sourceIndex);
  uint cnt = countUmarkedTerms();
  if (cnt > 0) {
    dbSet<dbRSeg> rSet = _net->getRSegs();
    uint wireCnt = 0;
    uint viaCnt = 0;
    fprintf(_connFP,
            "\nNET termCount=%d wireCnt=%d viaCnt=%d rCnt=%d %d %s\n",
            _net->getTermCount(),
            wireCnt,
            viaCnt,
            rSet.size(),
            _net->getId(),
            _net->getConstName());
    unmarkAllTerms();
    resetVisited();
    _debug = true;
    dfs(_sourceIndex);
  }
  unmarkAllTerms();

  return cnt;
}
void extCapNodeHash::markTerm(dbCapNode* capNode)
{
  if (capNode->isITerm()) {
    dbITerm* iterm = dbITerm::getITerm(_net->getBlock(), capNode->getNode());
    iterm->setMark(1);
  } else if (capNode->isBTerm()) {
    dbBTerm* bterm
        = odb::dbBTerm::getBTerm(_net->getBlock(), capNode->getNode());
    bterm->setMark(1);
  }
}
void extCapNodeHash::dfs(int n)
{
  extListNode* e = _table[n];
  if (e == NULL)
    return;
  if (e->isVisited())
    return;
  e->setVisited();
  if (e->getNextNodeNum() > 0)
    markTerm(e->getNode());
  if (_debug) {
    fprintf(_connFP, "\tdfs: map=%d --> %d  ", n, n + _min);
    _debugNet->writeCapNode(e->getNode(), "\n");
    /*
    fprintf(stdout, "\tdfs: map=%d --> %d  ",n, n+_min);
    _debugNet->printCapNode(e->getNode());
    fprintf(stdout, "\n");
    */
  }
  for (extListNode* f = e; f != NULL; f = f->getNext()) {
    int n1 = f->getNextNodeId();
    int ii = getIndex(n1);
    dfs(ii);
  }
}
extListNode* extCapNodeHash::addNode(uint n1, dbRSeg* rc, dbCapNode* node)
{
  if (node->getNode() == 0)
    return NULL;
  int ii = getIndex(n1);
  extListNode* e = new extListNode(rc, _table[ii], node);
  _table[ii] = e;
  if (e->getNode()->isSourceTerm())
    _sourceIndex = ii;
}
uint extDebugNet::netCheckConn(int debug_id)
{
  bool verbose = true;
  OpenConnFile("ConnWarnings");
  uint cnt = 0;
  odb::dbSet<odb::dbNet> nets = _block->getNets();
  odb::dbSet<odb::dbNet>::iterator net_itr;
  for (net_itr = nets.begin(); net_itr != nets.end(); ++net_itr) {
    dbNet* net = *net_itr;
    _net = net;
    cnt += checkNet(debug_id);
  }
  fclose(_connFP);
  return cnt;
}
FILE* extDebugNet::OpenConnFile(const char* name)
{
  _connFP = ATH__openFile(name, "w");
  return _connFP;
}
void extDebugNet::CloseConnFile()
{
  fclose(_connFP);
}

bool extDebugNet::checkConnOrdered(odb::dbNet* net, bool verbose)
{
  if (verbose)
    fprintf(_connFP,
            "checkConnOrdered: net %d %s\n",
            net->getId(),
            net->getName().c_str());
  dbITerm* drv_iterm = NULL;
  dbBTerm* drv_bterm = NULL;
  dbITerm* itermV[1024];
  dbBTerm* btermV[1024];
  int itermN = 0;
  int btermN = 0;
  int j;
  bool connected = true;
  dbWire* wire = net->getWire();
  dbWirePathItr pitr;
  dbWirePath path;
  dbWirePathShape pathShape;
  pitr.begin(wire);
  int first = 1;
  while (pitr.getNextPath(path)) {
    if (!path.is_branch) {
      if (verbose) {
        fprintf(_connFP, "path %d", path.junction_id);
        if (path.iterm) {
          fprintf(_connFP,
                  " iterm I%d/%s %s",
                  path.iterm->getInst()->getId(),
                  path.iterm->getMTerm()->getName().c_str(),
                  path.iterm->getInst()->getConstName());
        } else if (path.bterm) {
          fprintf(_connFP, " bterm %d", path.bterm->getId());
        }
        fprintf(_connFP, "\n");
      }
      if (!path.iterm && !path.bterm) {
        fprintf(_connFP, "NO TERM\n");
        connected = false;
      }
      if (first) {
        first = 0;
        if (path.iterm) {
          drv_iterm = path.iterm;
          itermV[itermN++] = drv_iterm;
        }
        if (path.bterm) {
          drv_bterm = path.bterm;
          btermV[btermN++] = drv_bterm;
        }
      } else {
        if (verbose && !(drv_iterm && path.iterm == drv_iterm)
            && !(drv_bterm && path.bterm == drv_bterm)) {
          fprintf(_connFP, "PATH NOT FROM DRIVER\n");
          fprintf(_connFP, "path.short_junction = %d\n", path.short_junction);
        }
        if (path.iterm) {
          for (j = 0; j < itermN; j++)
            if (itermV[j] == path.iterm)
              break;
          if (j == itermN) {
            fprintf(_connFP, "DISC\n");
            connected = false;
            itermV[itermN++] = path.iterm;
          }
        }
        if (path.bterm) {
          for (j = 0; j < btermN; j++)
            if (btermV[j] == path.bterm)
              break;
          if (j == btermN) {
            fprintf(_connFP, "DISC\n");
            connected = false;
            btermV[btermN++] = path.bterm;
          }
        }
      }
    } else {
      if (verbose)
        fprintf(_connFP, "branch %d\n", path.junction_id);
    }
    while (pitr.getNextShape(pathShape)) {
      if (1 && verbose)
        fprintf(_connFP, "shape %d", pathShape.junction_id);
      if (pathShape.iterm) {
        if (1 && verbose)
          fprintf(_connFP,
                  " iterm I%d/%s",
                  pathShape.iterm->getInst()->getId(),
                  pathShape.iterm->getMTerm()->getName().c_str());
        for (j = 0; j < itermN; j++)
          if (itermV[j] == pathShape.iterm)
            break;
        if (j == itermN) {
          itermV[itermN++] = pathShape.iterm;
        }
      } else if (pathShape.bterm) {
        if (0 && verbose)
          fprintf(_connFP, " bterm %d", pathShape.bterm->getId());
        for (j = 0; j < btermN; j++)
          if (btermV[j] == pathShape.bterm)
            break;
        if (j == btermN)
          btermV[btermN++] = pathShape.bterm;
      }
      if (1 && verbose) {
        dbShape s = pathShape.shape;
        if (s.getTechVia())
          fprintf(_connFP, " %s", s.getTechVia()->getName().c_str());
        else if (s.getVia())
          fprintf(_connFP, " %s", s.getVia()->getName().c_str());
        else if (s.getTechLayer())
          fprintf(_connFP, " %s", s.getTechLayer()->getName().c_str());
        fprintf(_connFP, "\n");
      }
    }
  }
  if (!connected) {
    fprintf(_connFP,
            "disconnected net %d %s\n",
            net->getId(),
            net->getName().c_str());
    return false;
  }
  return true;
}

}  // namespace rcx
