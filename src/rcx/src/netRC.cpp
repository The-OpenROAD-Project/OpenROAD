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
//#define HI_ACC_10312011

#define DEBUG_NET_ID 228157

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

void extMain::print_RC(dbRSeg* rc) {
  dbShape s;
  dbWire* w = rc->getNet()->getWire();
  w->getShape(rc->getShapeId(), s);
  print_shape(s, rc->getSourceNode(), rc->getTargetNode());
}
uint extMain::print_shape(dbShape& shape, uint j1, uint j2) {
  uint dx = shape.xMax() - shape.xMin();
  uint dy = shape.yMax() - shape.yMin();
  if (shape.isVia()) {
    dbTechVia* tech_via = shape.getTechVia();
    std::string vname = tech_via->getName();

    logger_->info(RCX, 438, "VIA {} ( {} {} )  jids= ( {} {} )", vname.c_str(),
                  shape.xMin() + dx / 2, shape.yMin() + dy / 2, j1, j2);
  } else {
    dbTechLayer* layer = shape.getTechLayer();
    std::string lname = layer->getName();
    logger_->info(RCX, 437, "RECT {} ( {} {} ) ( {} {} )  jids= ( {} {} )",
                  lname.c_str(), shape.xMin(), shape.yMin(), shape.xMax(),
                  shape.yMax(), j1, j2);

    if (dx < dy)
      return dy;
    else
      return dx;
  }
  return 0;
}

uint extMain::computePathDir(Point& p1, Point& p2, uint* length) {
  int len;
  if (p2.getX() == p1.getX())
    len = p2.getY() - p1.getY();
  else
    len = p2.getX() - p1.getX();

  if (len > 0) {
    *length = len;
    return 0;
  } else {
    *length = -len;
    return 1;
  }
}

void extMain::resetSumRCtable() {
  _sumUpdated = 0;
  _tmpSumCapTable[0] = 0.0;
  _tmpSumResTable[0] = 0.0;
  if (!_lefRC) {
    for (uint ii = 1; ii < _metRCTable.getCnt(); ii++) {
      _tmpSumCapTable[ii] = 0.0;
      _tmpSumResTable[ii] = 0.0;
    }
  }
}

void extMain::addToSumRCtable() {
  _sumUpdated = 1;
  if (_lefRC) {
    _tmpSumCapTable[0] += _tmpCapTable[0];
    _tmpSumResTable[0] += _tmpResTable[0];
  } else {
    for (uint ii = 0; ii < _metRCTable.getCnt(); ii++) {
      _tmpSumCapTable[ii] += _tmpCapTable[ii];
      _tmpSumResTable[ii] += _tmpResTable[ii];
    }
  }
}

void extMain::copyToSumRCtable() {
  if (_lefRC) {
    _tmpSumCapTable[0] = _tmpCapTable[0];
    _tmpSumResTable[0] = _tmpResTable[0];
  } else {
    for (uint ii = 0; ii < _metRCTable.getCnt(); ii++) {
      _tmpSumCapTable[ii] = _tmpCapTable[ii];
      _tmpSumResTable[ii] = _tmpResTable[ii];
    }
  }
}
void extMain::set_adjust_colinear(bool v) { _adjust_colinear = v; }

double extMain::getViaResistance(dbTechVia* tvia) {
  // if (_viaResHash[tvia->getConstName()])
  double res = 0;
  dbSet<dbBox> boxes = tvia->getBoxes();
  dbSet<dbBox>::iterator bitr;

  for (bitr = boxes.begin(); bitr != boxes.end(); ++bitr) {
    dbBox* box = *bitr;
    dbTechLayer* layer1 = box->getTechLayer();
    if (layer1->getType() == dbTechLayerType::CUT)
      res = layer1->getResistance();

    /* This works

    debug("EXT_RES",
          "V",
          "getViaResistance: {} {} %g ohms\n",
          tvia->getConstName(),
          layer1->getConstName(),
          layer1->getResistance());

          */
  }
  return res;
}
double extMain::getViaResistance_b(dbVia* tvia, dbNet* net) {
  // if (_viaResHash[tvia->getConstName()])
  double tot_res = 0;
  dbSet<dbBox> boxes = tvia->getBoxes();
  dbSet<dbBox>::iterator bitr;

  uint cutCnt = 0;
  for (bitr = boxes.begin(); bitr != boxes.end(); ++bitr) {
    dbBox* box = *bitr;
    dbTechLayer* layer1 = box->getTechLayer();
    if (layer1->getType() == dbTechLayerType::CUT) {
      tot_res += layer1->getResistance();
      cutCnt++;
      // debug("EXT_RES", "R", "getViaResistance_b: {} {} {} %g ohms\n", cutCnt,
      // tvia->getConstName(),  layer1->getConstName(),
      // layer1->getResistance());
    }
  }
  double Res = tot_res;
  if (cutCnt > 1) {
    float avgCutRes = tot_res / cutCnt;
    Res = avgCutRes / cutCnt;
  }
  if (net != NULL && net->getId() == _debug_net_id) {
    debugPrint(logger_, RCX, "extrules", 1,
               "EXT_RES:"
               "R"
               "\tgetViaResistance_b: cutCnt= {} {}  {:g} ohms",
               cutCnt, tvia->getConstName(), Res);
  }
  return Res;
}

void extMain::getViaCapacitance(dbShape svia, dbNet* net) {
  bool USE_DB_UNITS = false;

  uint cnt = 0;

  const char* tcut = "tcut";
  const char* bcut = "bcut";

  std::vector<dbShape> shapes;
  dbShape::getViaBoxes(svia, shapes);

  int Width[32];
  int Len[32];
  int Level[32];
  for (uint jj = 1; jj < 32; jj++) {
    Level[jj] = 0;
    Width[jj] = 0;
    Len[jj] = 0;
  }

  int maxLenBot = 0;
  int maxWidthBot = 0;
  int maxLenTop = 0;
  int maxWidthTop = 0;

  int bot = 0;
  int top = 1000;

  std::vector<dbShape>::iterator shape_itr;
  for (shape_itr = shapes.begin(); shape_itr != shapes.end(); ++shape_itr) {
    dbShape s = *shape_itr;

    if (s.getTechLayer()->getType() == dbTechLayerType::CUT)
      continue;

    int x1 = s.xMin();
    int y1 = s.yMin();
    int x2 = s.xMax();
    int y2 = s.yMax();
    int dx = x2 - x1;
    int dy = y2 - y1;
    int width = MIN(dx, dy);
    int len = MAX(dx, dy);

    uint level = s.getTechLayer()->getRoutingLevel();
    if (Len[level] < len) {
      Len[level] = len;
      Width[level] = width;
      Level[level] = level;
    }
    if (USE_DB_UNITS) {
      width = GetDBcoords2(width);
      len = GetDBcoords2(len);
    }

    if (net->getId() == _debug_net_id) {
      debugPrint(logger_, RCX, "extrules", 1,
                 "VIA_CAP:"
                 "C"
                 "\tgetViaCapacitance: {} {}   {} {}  M{}  W {}  LEN {} n{}",
                 x1, x2, y1, y2, level, width, len, _debug_net_id);
    }
  }
  for (uint jj = 1; jj < 32; jj++) {
    if (Level[jj] == 0)
      continue;

    int w = Width[jj];
    int len = Len[jj];

    if (USE_DB_UNITS) {
      w = GetDBcoords2(w);
      len = GetDBcoords2(len);
    }
    for (uint ii = 0; ii < _metRCTable.getCnt(); ii++) {
      double areaCap;
      double c1 = getFringe(jj, w, ii, areaCap);

      _tmpCapTable[ii] = len * 2 * c1;
      if (net->getId() == _debug_net_id) {
        debugPrint(logger_, RCX, "extrules", 1,
                   "VIA_CAP:"
                   "C"
                   "\tgetViaCapacitance: M{}  W {}  LEN {} eC={:.3f} tC={:.3f} "
                   " {} n{}",
                   jj, w, len, c1, _tmpCapTable[ii], ii, _debug_net_id);
      }
    }
  }
}

void extMain::getShapeRC(dbNet* net, dbShape& s, Point& prevPoint,
                         dbWirePathShape& pshape) {
  bool USE_DB_UNITS = false;
  double res = 0.0;
  double areaCap;
  uint len;
  uint level = 0;
  if (s.isVia()) {
    uint width = 0;
    dbTechVia* tvia = s.getTechVia();
    if (tvia != NULL) {
      int i = 0;
      level = tvia->getBottomLayer()->getRoutingLevel();
      width = tvia->getBottomLayer()->getWidth();
      res = tvia->getResistance();
      if (res == 0)
        res = getViaResistance(tvia);
      if (res > 0)
        tvia->setResistance(res);
      if (res <= 0.0)
        res = getResistance(level, width, width, 0);
    } else {
      dbVia* bvia = s.getVia();
      if (bvia != NULL) {
        level = bvia->getBottomLayer()->getRoutingLevel();
        width = bvia->getBottomLayer()->getWidth();
        len = width;
        res = getViaResistance_b(bvia, net);

        if (res <= 0.0)
          res = getResistance(level, width, len, 0);
      }
    }
    if (level > 0) {
      if (_lefRC) {
        _tmpCapTable[0] = width * 2 * getFringe(level, width, 0, areaCap);
        _tmpCapTable[0] += 2 * areaCap * width * width;
        _tmpResTable[0] = res;
      } else {
        getViaCapacitance(s, net);
        for (uint ii = 0; ii < _metRCTable.getCnt(); ii++) {
          //_tmpCapTable[ii] = width * 2 * getFringe(level, width, ii, areaCap);
          //_tmpCapTable[ii] += 2 * areaCap * width * width;
          _tmpResTable[ii] = res;
        }
      }
    }
  } else {
    computePathDir(prevPoint, pshape.point, &len);
    level = s.getTechLayer()->getRoutingLevel();
    uint width = MIN(pshape.shape.xMax() - pshape.shape.xMin(),
                     pshape.shape.yMax() - pshape.shape.yMin());
    len = MAX(pshape.shape.xMax() - pshape.shape.xMin(),
              pshape.shape.yMax() - pshape.shape.yMin());
    if (_adjust_colinear) {
      len -= width;
      if (len <= 0)
        len += width;
    }

    if (_lefRC) {
      double res = getResistance(level, width, len, 0);
      ;
      double unitCap = getFringe(level, width, 0, areaCap);
      double tot = len * width * unitCap;
      double frTot = len * 2 * unitCap;

      _tmpCapTable[0] = frTot;
      _tmpCapTable[0] += 2 * areaCap * len * width;
      _tmpResTable[0] = res;

    } else if (_lef_res) {
      double res = getResistance(level, width, len, 0);
      _tmpResTable[0] = res;
    } else {
      if (USE_DB_UNITS)
        width = GetDBcoords2(width);
      double SUB_MULT = 1.0;

      for (uint ii = 0; ii < _metRCTable.getCnt(); ii++) {
        double c1 = getFringe(level, width, ii, areaCap);
        c1 *= SUB_MULT;
#ifdef HI_ACC_10312011
        if (width < 400)
          _tmpCapTable[ii] =
              (len + width) * 2 * getFringe(level, width, ii, areaCap);
        else
          _tmpCapTable[ii] = len * 2 * getFringe(level, width, ii, areaCap);
#else
        if (USE_DB_UNITS)
          len = GetDBcoords2(len);

        // DF 720	_tmpCapTable[ii]= len*2*c1;
        _tmpCapTable[ii] = 0;
#endif
        // _tmpCapTable[ii] += 2 * areaCap * len * width;
        bool newResModel = true;
        if (!newResModel) {
          double r = getResistance(level, width, len, ii);
          _tmpResTable[ii] = r;
          _tmpResTable[ii] = 0;
        } else {
          double r = getResistance(level, width, len, ii);
          _tmpResTable[ii] = r;
        }
      }
    }
  }
  if (_shapeRcCnt >= 0) {
    if (_printFile == NULL)
      _printFile = fopen("shapeRc.1", "w");
    _shapeRcCnt++;
    fprintf(_printFile, "%d %g %g\n", _shapeRcCnt, _tmpCapTable[0],
            _tmpResTable[0]);
  }
  if ((!s.isVia()) && (_couplingFlag > 0)) {
    dbTechLayer* layer = s.getTechLayer();
    uint level = layer->getRoutingLevel();

    int x1 = s.xMin();
    int y1 = s.yMin();
    int x2 = s.xMax();
    int y2 = s.yMax();

    if (_usingMetalPlanes && !_alwaysNewGs)
      _geomSeq->box(x1, y1, x2, y2, level);

    if (_useDbSdb) {
      // net->getWire()->setProperty (pshape.junction_id, rc->getId());
      if (_eco)
        _extNetSDB->addBox(x1, y1, x2, y2, level, net->getId(),
                           pshape.junction_id, _dbSignalId);
    }
    // else
    // _extNetSDB->addBox(x1, y1, x2, y2, level, rc->getId(),
    // pshape.junction_id, _dbSignalId);

    if (!_allNet) {
      _ccMinX = MIN(x1, _ccMinX);
      _ccMinY = MIN(y1, _ccMinY);
      _ccMaxX = MAX(x2, _ccMaxX);
      _ccMaxY = MAX(y2, _ccMaxY);
    }
  }
  prevPoint = pshape.point;
}

void extMain::setResCapFromLef(dbRSeg* rc, uint targetCapId, dbShape& s,
                               uint len) {
  double cap = 0.0;
  double res = 0.0;

  if (s.isVia()) {
    dbTechVia* via = s.getTechVia();

    res = via->getResistance();

    if (_couplingFlag == 0) {
      uint n = via->getBottomLayer()->getRoutingLevel();
      len = 2 * via->getBottomLayer()->getWidth();
      cap = _modelTable->get(0)->getTotCapOverSub(n) * len;
    }
  } else {
    uint level = s.getTechLayer()->getRoutingLevel();
    res = _modelTable->get(0)->getRes(level) * len;

    // if (_couplingFlag==0)
    cap = _modelTable->get(0)->getTotCapOverSub(level) * len;
  }
  for (uint ii = 0; ii < _extDbCnt; ii++) {
    double xmult = 1.0 + 0.1 * ii;
    rc->setResistance(xmult * res, ii);

    // if (_couplingFlag==0)
    rc->setCapacitance(xmult * cap, ii);
  }
}
void extMain::setResAndCap(dbRSeg* rc, double* restbl, double* captbl) {
  int pcdbIdx, sci, scdbIdx;
  double res, cap;
  if (_lefRC) {
    res = _resModify ? restbl[0] * _resFactor : restbl[0];
    rc->setResistance(res, 0);
    cap = _gndcModify ? captbl[0] * _gndcFactor : captbl[0];
    cap = _netGndcCalibration ? cap * _netGndcCalibFactor : cap;
    rc->setCapacitance(cap, 0);
    return;
  }
  // TO_TEST
  for (uint ii = 0; ii < _extDbCnt; ii++) {
    pcdbIdx = getProcessCornerDbIndex(ii);
    res = _resModify ? restbl[ii] * _resFactor : restbl[ii];
    rc->setResistance(res, pcdbIdx);
    cap = _gndcModify ? captbl[ii] * _gndcFactor : captbl[ii];
    cap = _netGndcCalibration ? cap * _netGndcCalibFactor : cap;
    // DF 1120 rc->setCapacitance(cap, pcdbIdx);
    getScaledCornerDbIndex(ii, sci, scdbIdx);
    if (sci == -1)
      continue;
    getScaledRC(sci, res, cap);
    rc->setResistance(res, scdbIdx);
    rc->setCapacitance(cap, scdbIdx);
  }
  /*
  for (uint ii= 1; ii<_extDbCnt; ii++)
  {
          double xmult = 1.0 + 0.1*ii;
          rc->setResistance(xmult*res, ii);
          // if (_couplingFlag==0)
                  rc->setCapacitance(xmult*cap, ii);
  }
  */
}

void extMain::resetMapping(dbBTerm* bterm, dbITerm* iterm, uint junction) {
  if (bterm != NULL) {
    _btermTable->set(bterm->getId(), 0);
  } else if (iterm != NULL) {
    _itermTable->set(iterm->getId(), 0);
  }
  _nodeTable->set(junction, 0);
}
void extMain::setBranchCapNodeId(dbNet* net, uint junction) {
  int capId = _nodeTable->geti(junction);
  if (capId != 0)
    return;

  dbCapNode* cap = dbCapNode::create(net, 0, _foreign);

  cap->setInternalFlag();
  cap->setBranchFlag();

  cap->setNode(junction);

  capId = cap->getId();

  _nodeTable->set(junction, -capId);
  return;
}

bool extMain::isTermPathEnded(dbBTerm* bterm, dbITerm* iterm) {
  int ttttcvbs = 0;
  dbNet* net;
  if (bterm) {
    net = bterm->getNet();
    if (bterm->isSetMark()) {
      if (ttttcvbs)
        logger_->info(RCX, 108, "Net {} multiple-ended at bterm {}",
                      net->getId(), bterm->getId());
      return true;
    }
    _connectedBTerm.push_back(bterm);
    bterm->setMark(1);
  } else if (iterm) {
    net = iterm->getNet();
    if (iterm->isSetMark()) {
      if (ttttcvbs)
        logger_->info(RCX, 109, "Net {} multiple-ended at iterm {}",
                      net->getId(), iterm->getId());
      return true;
    }
    _connectedITerm.push_back(iterm);
    iterm->setMark(1);
  }
  return false;
}

uint extMain::getCapNodeId(dbNet* net, dbBTerm* bterm, dbITerm* iterm,
                           uint junction, bool branch) {
  if (iterm != NULL) {
    uint id = iterm->getId();
    uint capId = _itermTable->geti(id);
    if (capId > 0) {
#ifdef DEBUG_NET_ID
      if (iterm->getNet()->getId() == DEBUG_NET_ID)
        fprintf(fp, "\tOLD I_TERM %d  capNode %d\n", id, capId);
#endif
      //(dbCapNode::getCapNode(_block, capId))->incrChildrenCnt();

      return capId;
    }
    dbCapNode* cap = dbCapNode::create(net, 0, _foreign);

    cap->setNode(iterm->getId());
    cap->setITermFlag();

    capId = cap->getId();

    _itermTable->set(id, capId);
    int tcapId = _nodeTable->geti(junction) == -1 ? -capId : capId;
    _nodeTable->set(junction, tcapId);  // allow get capId using junction
#ifdef DEBUG_NET_ID
    if (iterm->getNet()->getId() == DEBUG_NET_ID)
      fprintf(fp, "\tNEW I_TERM %d capNode %d\n", id, capId);
#endif
    return capId;
  } else if (bterm != NULL) {
    uint id = bterm->getId();
    uint capId = _btermTable->geti(id);
    if (capId > 0) {
//(dbCapNode::getCapNode(_block, capId))->incrChildrenCnt();
#ifdef DEBUG_NET_ID
      if (bterm->getNet()->getId() == DEBUG_NET_ID)
        fprintf(fp, "\tOLD B_TERM %d  capNode %d\n", id, capId);
#endif
      return capId;
    }

    dbCapNode* cap = dbCapNode::create(net, 0, _foreign);

    cap->setNode(bterm->getId());
    cap->setBTermFlag();
    // cap->incrChildrenCnt();

    capId = cap->getId();

    _btermTable->set(id, capId);
    int tcapId = _nodeTable->geti(junction) == -1 ? -capId : capId;
    _nodeTable->set(junction, tcapId);  // allow get capId using junction

#ifdef DEBUG_NET_ID
    if (bterm->getNet()->getId() == DEBUG_NET_ID)
      fprintf(fp, "\tNEW B_TERM %d  capNode %d\n", id, capId);
#endif
    return capId;
  } else {
    int capId = _nodeTable->geti(junction);
    if (capId != 0 && capId != -1) {
      capId = abs(capId);
      dbCapNode* cap = dbCapNode::getCapNode(_block, capId);
      // cap->incrChildrenCnt();
      if (branch) {
        cap->setBranchFlag();
      }
      if (cap->getNet()->getId() == _debug_net_id) {
        if (branch) {
          if (cap->getNet()->getId() == DEBUG_NET_ID)
            debugPrint(logger_, RCX, "rcseg", 1,
                       "RCSEG:"
                       "C"
                       "\tOLD BRANCH {}  capNode {}",
                       junction, cap->getId());
        } else {
          if (cap->getNet()->getId() == DEBUG_NET_ID)
            debugPrint(logger_, RCX, "rcseg", 1,
                       "RCSEG"
                       "C"
                       "\tOLD INTERNAL {}  capNode {}",
                       junction, cap->getId());
        }
      }
      return capId;
    }

    dbCapNode* cap = dbCapNode::create(net, 0, _foreign);

    cap->setInternalFlag();
    //		if (branch)
    //			cap->setBranchFlag();

    cap->setNode(junction);

    if (capId == -1) {
      // cap->incrChildrenCnt();
      if (branch)
        cap->setBranchFlag();
    }
    if (cap->getNet()->getId() == DEBUG_NET_ID) {
      if (branch) {
        debugPrint(logger_, RCX, "rcseg", 1,
                   "RCSEG:"
                   "C"
                   "\tNEW BRANCH {}  capNode {}",
                   junction, cap->getId());
      } else
        debugPrint(logger_, RCX, "rcseg", 1,
                   "RCSEG:"
                   "C"
                   "\tNEW INTERNAL {}  capNode {}",
                   junction, cap->getId());
    }

    uint ncapId = cap->getId();
    int tcapId = capId == 0 ? ncapId : -ncapId;
    _nodeTable->set(junction, tcapId);
    return ncapId;
  }
}
uint extMain::resetMapNodes(dbNet* net) {
  //	uint rcCnt= 0;
  //	uint netId= net->getId();

  dbWire* wire = net->getWire();
  if (wire == NULL) {
    if (_reportNetNoWire)
      logger_->info(RCX, 110, "Net {} has no wires.", net->getName().c_str());
    _netNoWireCnt++;
    return 0;
  }
  uint cnt = 0;

  dbWirePath path;
  dbWirePathShape pshape;

  dbWirePathItr pitr;

  for (pitr.begin(wire); pitr.getNextPath(path);) {
    resetMapping(path.bterm, path.iterm, path.junction_id);

    while (pitr.getNextShape(pshape)) {
      resetMapping(pshape.bterm, pshape.iterm, pshape.junction_id);
      cnt++;
    }
  }
  return cnt;
}
dbRSeg* extMain::addRSeg(dbNet* net, std::vector<uint>& rsegJid, uint& srcId,
                         Point& prevPoint, dbWirePath& path,
                         dbWirePathShape& pshape, bool isBranch, double* restbl,
                         double* captbl) {
  // if (!path.iterm && !path.bterm &&isTermPathEnded(pshape.bterm,
  // pshape.iterm))
  if (!path.bterm && isTermPathEnded(pshape.bterm, pshape.iterm)) {
    rsegJid.clear();
    return NULL;
  }
  uint jidl = rsegJid.size();
  // assert (jidl>0);
  uint dstId = getCapNodeId(net, pshape.bterm, pshape.iterm, pshape.junction_id,
                            isBranch);
  if (dstId == srcId) {
    char tname[200];
    tname[0] = '\0';
    if (pshape.bterm)
      sprintf(&tname[0], ", on bterm %d %s", pshape.bterm->getId(),
              (char*)pshape.bterm->getConstName());
    else if (pshape.iterm)
      sprintf(&tname[0], ", on iterm %d %s/%s", pshape.iterm->getId(),
              (char*)pshape.iterm->getInst()->getConstName(),
              (char*)pshape.iterm->getMTerm()->getConstName());
    logger_->warn(RCX, 111, "Net {} {} has a loop at x={} y={} {}.",
                  net->getId(), (char*)net->getConstName(), pshape.point.getX(),
                  pshape.point.getY(), &tname[0]);
    return NULL;
  }

  if (net->getId() == _debug_net_id)
    print_shape(pshape.shape, srcId, dstId);

  uint length;
  uint pathDir = computePathDir(prevPoint, pshape.point, &length);
  int jx = 0;
  int jy = 0;
  if (pshape.junction_id)
    net->getWire()->getCoord((int)pshape.junction_id, jx, jy);
  dbRSeg* rc = dbRSeg::create(net, jx, jy, pathDir, true);
  // dbRSeg::setRSeg(rc, net, pshape.junction_id, pathDir, true);

  uint rsid = rc->getId();
  for (uint jj = 0; jj < jidl; jj++)
    net->getWire()->setProperty(rsegJid[jj], rsid);
  rsegJid.clear();

  rc->setSourceNode(srcId);
  rc->setTargetNode(dstId);

  if (srcId > 0)
    (dbCapNode::getCapNode(_block, srcId))->incrChildrenCnt();
  if (dstId > 0)
    (dbCapNode::getCapNode(_block, dstId))->incrChildrenCnt();

  setResAndCap(rc, restbl, captbl);

  if (net->getId() == _debug_net_id)
    debugPrint(logger_, RCX, "rcseg", 1,
               "RCSEG:"
               "R"
               "\tshapeId= {}  rseg= {}  ({} {}) {:g}",
               pshape.junction_id, rsid, srcId, dstId, rc->getCapacitance(0));

  srcId = dstId;
  prevPoint = pshape.point;
  return rc;
}
bool extMain::getFirstShape(dbNet* net, dbShape& s) {
  dbWirePath path;
  dbWirePathShape pshape;

  dbWirePathItr pitr;
  dbWire* wire = net->getWire();

  bool status = false;
  for (pitr.begin(wire); pitr.getNextPath(path);) {
    pitr.getNextShape(pshape);
    s = pshape.shape;
    status = true;
    break;
  }
  return status;
}

uint extMain::getShortSrcJid(uint jid) {
  for (uint jj = 0; jj < _shortTgtJid.size(); jj++) {
    if (_shortTgtJid[jj] == jid)
      return _shortSrcJid[jj];
  }
  return jid;
}

void extMain::markPathHeadTerm(dbWirePath& path) {
  if (path.bterm) {
    _connectedBTerm.push_back(path.bterm);
    path.bterm->setMark(1);
  } else if (path.iterm) {
    _connectedITerm.push_back(path.iterm);
    path.iterm->setMark(1);
  }
}

void extMain::make1stRSeg(dbNet* net, dbWirePath& path, uint cnid,
                          bool skipStartWarning) {
  int tx = 0;
  int ty = 0;
  if (path.bterm) {
    if (!path.bterm->getFirstPinLocation(tx, ty))
      logger_->error(RCX, 112, "Can't locate bterm {}",
                     path.bterm->getConstName());
  } else if (path.iterm) {
    if (!path.iterm->getAvgXY(&tx, &ty))
      logger_->error(RCX, 113, "Can't locate iterm {}/{} ( {} )",
                     path.iterm->getInst()->getConstName(),
                     path.iterm->getMTerm()->getConstName(),
                     path.iterm->getInst()->getMaster()->getConstName());
  } else if (!skipStartWarning)
    logger_->warn(RCX, 114,
                  "Net {} {} does not start from an iterm or a bterm.",
                  net->getId(), net->getConstName());
  if (net->get1stRSegId())
    logger_->error(RCX, 115, "Net {} {} already has rseg!", net->getId(),
                   net->getConstName());
  dbRSeg* rc = dbRSeg::create(net, tx, ty, 0, true);
  rc->setTargetNode(cnid);
}

uint extMain::makeNetRCsegs(dbNet* net, bool skipStartWarning) {
  //_debug= true;
  net->setRCgraph(true);

  // uint netId= net->getId();

  uint rcCnt1 = resetMapNodes(net);
  if (rcCnt1 <= 0)
    return 0;

  _netGndcCalibFactor = net->getGndcCalibFactor();
  _netGndcCalibration = _netGndcCalibFactor == 1.0 ? false : true;
  uint rcCnt = 0;
  dbWirePath path;
  dbWirePathShape pshape, ppshape;
  Point prevPoint, sprevPoint;

  // std::vector<uint> rsegJid;
  _rsegJid.clear();
  _shortSrcJid.clear();
  _shortTgtJid.clear();

  dbWirePathItr pitr;
  dbWire* wire = net->getWire();
  uint srcId, srcJid;

  uint netId = net->getId();
#ifdef DEBUG_NET_ID
  if (netId == DEBUG_NET_ID) {
    fp = fopen("rsegs", "w");
    fprintf(fp, "BEGIN NET %d %d\n", netId, path.junction_id);
  }
#endif
  if (netId == _debug_net_id) {
    debugPrint(logger_, RCX, "rcseg", 1,
               "RCSEG:"
               "R "
               "makeNetRCsegs: BEGIN NET {} {}",
               netId, path.junction_id);
  }

  if (_mergeResBound != 0.0 || _mergeViaRes) {
    for (pitr.begin(wire); pitr.getNextPath(path);) {
      if (!path.bterm && !path.iterm && path.is_branch && path.junction_id)
        _nodeTable->set(path.junction_id, -1);
      // setBranchCapNodeId(net, path.junction_id);

      if (path.is_short)  // wfs 090106
      {
        _nodeTable->set(path.short_junction, -1);
        srcJid = path.short_junction;
        for (uint tt = 0; tt < _shortTgtJid.size(); tt++) {
          if (_shortTgtJid[tt] == srcJid) {
            srcJid = _shortSrcJid[tt];  // forward short
            break;
          }
        }
        _shortSrcJid.push_back(srcJid);
        _shortTgtJid.push_back(path.junction_id);
      }
      while (pitr.getNextShape(pshape))
        ;
    }
  }
  bool netHeadMarked = false;
  for (pitr.begin(wire); pitr.getNextPath(path);) {
    if (netId == _debug_net_id)
      debugPrint(logger_, RCX, "rcseg", 1,
                 "RCSEG:"
                 "R "
                 "makeNetRCsegs:  path.junction_id {}",
                 path.junction_id);

    if (!path.iterm && !path.bterm && !path.is_branch && path.is_short)
      srcId =
          getCapNodeId(net, NULL, NULL, getShortSrcJid(path.junction_id), true);
    else
      srcId = getCapNodeId(net, path.bterm, path.iterm,
                           getShortSrcJid(path.junction_id), path.is_branch);
    if (!netHeadMarked) {
      netHeadMarked = true;
      // markPathHeadTerm(path); // may call markPathHeadTerm() later
      make1stRSeg(net, path, srcId, skipStartWarning);
    }

    // srcId= getCapNodeId(net, path.bterm, path.iterm, path.junction_id,
    // path.is_branch);

    // wfs 090106, adding 0.0001 rseg for short
    // if (!path.iterm && !path.bterm && !path.is_branch && path.is_short) {
    //  uint shortId = getCapNodeId(net, NULL, NULL, path.short_junction, true);
    //  dbRSeg *rc= dbRSeg::create(net, path.junction_id, 0, true);
    //  rc->setSourceNode(shortId);
    //  rc->setTargetNode(srcId);
    //  srcId = shortId;
    //  for (int jj=0;jj<_extDbCnt;jj++) {
    //    rc->setResistance(0.0001,jj);
    //    rc->setCapacitance(0.0,jj);
    //  }

    bool ADD_VIA_JUNCTION = false;
    prevPoint = path.point;
    sprevPoint = prevPoint;
    resetSumRCtable();
    while (pitr.getNextShape(pshape)) {
      dbShape s = pshape.shape;

      if (netId == _debug_net_id) {
        if (s.isVia()) {
          debugPrint(logger_, RCX, "rcseg", 1,
                     "RCSEG:"
                     "R "
                     "makeNetRCsegs: {} VIA",
                     pshape.junction_id);
        } else
          debugPrint(logger_, RCX, "rcseg", 1,
                     "RCSEG:"
                     "R "
                     "makeNetRCsegs: {} WIRE",
                     pshape.junction_id);
      }

      getShapeRC(net, s, sprevPoint, pshape);
      if (_mergeResBound == 0.0) {
        if (!s.isVia())
          _rsegJid.push_back(pshape.junction_id);
        else if (ADD_VIA_JUNCTION)
          _rsegJid.push_back(pshape.junction_id);

        addToSumRCtable();

        if (!_mergeViaRes || !s.isVia() || pshape.bterm || pshape.iterm ||
            _nodeTable->geti(pshape.junction_id) < 0) {
          dbRSeg* rc =
              addRSeg(net, _rsegJid, srcId, prevPoint, path, pshape,
                      path.is_branch, _tmpSumResTable, _tmpSumCapTable);
          if (s.isVia() && rc != NULL) {
            // seg->_flags->_spare_bits_29=1;
            createShapeProperty(net, pshape.junction_id, rc->getId());
          }
          resetSumRCtable();
          rcCnt++;
        } else
          ppshape = pshape;
        continue;
      }
      if (_tmpResTable[0] >= _mergeResBound && _tmpSumResTable[0] == 0.0) {
        if (!s.isVia())
          _rsegJid.push_back(pshape.junction_id);
        addRSeg(net, _rsegJid, srcId, prevPoint, path, pshape, path.is_branch,
                _tmpResTable, _tmpCapTable);
        rcCnt++;
        continue;
      }
      if ((_tmpResTable[0] + _tmpSumResTable[0]) >= _mergeResBound) {
        addRSeg(net, _rsegJid, srcId, prevPoint, path, ppshape, path.is_branch,
                _tmpSumResTable, _tmpSumCapTable);
        rcCnt++;
        if (!s.isVia())
          _rsegJid.push_back(pshape.junction_id);
        copyToSumRCtable();
      } else {
        if (!s.isVia())
          _rsegJid.push_back(pshape.junction_id);
        addToSumRCtable();
      }
      if (pshape.bterm || pshape.iterm ||
          _nodeTable->geti(pshape.junction_id) < 0 ||
          _tmpSumResTable[0] >= _mergeResBound) {
        addRSeg(net, _rsegJid, srcId, prevPoint, path, pshape, path.is_branch,
                _tmpSumResTable, _tmpSumCapTable);
        rcCnt++;
        resetSumRCtable();
      } else
        ppshape = pshape;
    }
    // if (_tmpSumResTable[0] != 0.0 || _tmpSumCapTable[0] != 0.0)
    // if (_tmpSumResTable[0] != 0.0)
    if (_sumUpdated) {
      addRSeg(net, _rsegJid, srcId, prevPoint, path, ppshape, path.is_branch,
              _tmpSumResTable, _tmpSumCapTable);
      rcCnt++;
    }
  }
  dbSet<dbRSeg> rSet = net->getRSegs();
  rSet.reverse();

#ifdef DEBUG_NET_ID
  if (netId == DEBUG_NET_ID) {
    fprintf(fp, "END NET %d\n", netId);
    fclose(fp);
  }
#endif

  return rcCnt;
}

void extMain::createShapeProperty(dbNet* net, int id, int id_val) {
  char buff[64];
  sprintf(buff, "%d", id);
  char const* pchar = strdup(buff);
  dbIntProperty::create(net, pchar, id_val);
  sprintf(buff, "RC_%d", id_val);
  pchar = strdup(buff);
  dbIntProperty::create(net, pchar, id);
}
int extMain::getShapeProperty(dbNet* net, int id) {
  char buff[64];
  sprintf(buff, "%d", id);
  char const* pchar = strdup(buff);
  dbIntProperty* p = dbIntProperty::find(net, pchar);
  if (p == NULL)
    return 0;
  int rcid = p->getValue();
  return rcid;
}
int extMain::getShapeProperty_rc(dbNet* net, int rc_id) {
  char buff[64];
  sprintf(buff, "RC_%d", rc_id);
  char const* pchar = strdup(buff);
  dbIntProperty* p = dbIntProperty::find(net, pchar);
  if (p == NULL)
    return 0;
  int sid = p->getValue();
  return sid;
}

uint extMain::getExtBbox(int* x1, int* y1, int* x2, int* y2) {
  *x1 = _x1;
  *x2 = _x2;
  *y1 = _y1;
  *y2 = _y2;

  return 0;
}
bool extMain::getExtAreaCoords(const char* bbox) {
  /*
  const char * token = strtok( bbox, " \t\n" );
      const char * busbitchars = strtok(NULL, "\"");
*/
  return false;
}
ZPtr<ISdb> extMain::getCcSdb() { return _extCcapSDB; }
ZPtr<ISdb> extMain::getNetSdb() { return _extNetSDB; }
void extMain::setExtractionBbox(const char* bbox) {
  if ((bbox == NULL) || (strcmp(bbox, "") == 0)) {
    Rect r;
    _block->getDieArea(r);
    _x1 = r.xMin();
    _y1 = r.yMin();
    _x2 = r.xMax();
    _y2 = r.yMax();

    _extNetSDB->setMaxArea(_x1, _y1, _x2, _y2);
  }

  if (getExtAreaCoords(bbox))
    _extNetSDB->setMaxArea(_x1, _y1, _x2, _y2);
  else {
    Rect r;
    _block->getDieArea(r);

    _x1 = r.xMin();
    _y1 = r.yMin();
    _x2 = r.xMax();
    _y2 = r.yMax();

    _extNetSDB->setMaxArea(_x1, _y1, _x2, _y2);
  }
}

uint extMain::setupSearchDb(const char* bbox, uint debug,
                            ZInterface* Interface) {
  if (_couplingFlag == 0)
    return 0;
  // Ath__overlapAdjust overlapAdj = Z_endAdjust;
  Ath__overlapAdjust overlapAdj = Z_noAdjust;
  if (_reExtract) {
    if (_extNetSDB == NULL || _extNetSDB->getSearchPtr() == NULL) {
      logger_->info(RCX, 116, "No existing extraction sdb. Can't reExtract.");
      return 0;
    }
    _extNetSDB->setExtControl(
        _block, _useDbSdb, (uint)overlapAdj, _CCnoPowerSource, _CCnoPowerTarget,
        _ccUp, _allNet, _ccContextDepth, _ccContextArray, _ccContextLength,
        _dgContextArray, &_dgContextDepth, &_dgContextPlanes, &_dgContextTracks,
        &_dgContextBaseLvl, &_dgContextLowLvl, &_dgContextHiLvl,
        _dgContextBaseTrack, _dgContextLowTrack, _dgContextHiTrack,
        _dgContextTrackBase, _seqPool);
    //		if (_reExtCcapSDB && _reExtCcapSDB->getSearchPtr())
    //			_reExtCcapSDB->cleanSdb();
    //		else
    //		{
    //			if (adsNewComponent( Interface->_context, ZCID(Sdb),
    //_reExtCcapSDB
    //)!= Z_OK)
    //			{
    //				assert(0);
    //			}
    //			ZALLOCATED(_reExtCcapSDB);
    //		}
    //
    //		_reExtCcapSDB->initSearchForNets(_tech, _block);
    //		_reExtCcapSDB->setDefaultWireType(_CCsegId);
    return 0;
  }
  if (adsNewComponent(Interface->_context, ZCID(Sdb), _extNetSDB) != Z_OK) {
    assert(0);
  }
  ZALLOCATED(_extNetSDB);

  _extNetSDB->initSearchForNets(_tech, _block);

  setExtractionBbox(bbox);

  if (_couplingFlag > 1)
    _extNetSDB->setExtControl(
        _block, _useDbSdb, (uint)overlapAdj, _CCnoPowerSource, _CCnoPowerTarget,
        _ccUp, _allNet, _ccContextDepth, _ccContextArray, _ccContextLength,
        _dgContextArray, &_dgContextDepth, &_dgContextPlanes, &_dgContextTracks,
        &_dgContextBaseLvl, &_dgContextLowLvl, &_dgContextHiLvl,
        _dgContextBaseTrack, _dgContextLowTrack, _dgContextHiTrack,
        _dgContextTrackBase, _seqPool);
  if (!_CCnoPowerSource && !_CCnoPowerTarget)
    _extNetSDB->addPowerNets(_block, _dbPowerId, true);
  // TODO-EXT: add obstructions

  if (_debug && (_couplingFlag > 1)) {
    if (adsNewComponent(Interface->_context, ZCID(Sdb), _extCcapSDB) != Z_OK) {
      assert(0);
    }
    ZALLOCATED(_extCcapSDB);

    _extCcapSDB->initSearchForNets(_tech, _block);
    _extCcapSDB->setDefaultWireType(_CCsegId);
  }

  return 0;
}

void extMain::removeSdb(std::vector<dbNet*>& nets) {
  if (_extNetSDB == NULL || _extNetSDB->getSearchPtr() == NULL)
    return;
  dbNet::markNets(nets, _block, true);
  _extNetSDB->removeMarkedNetWires();
  dbNet::markNets(nets, _block, false);
}

void extMain::removeCC(std::vector<dbNet*>& nets) {
  for (uint ii = 0; ii < 15; ii++)
    logger_->warn(RCX, 255, "Need to CODE removeCC.");
  /*
          int cnt = 0;
          bool destroyOnlySrc;
          dbNet *net;
          dbSet<dbNet> bnet = _block->getNets();
          dbSet<dbNet>::iterator nitr;
          if (nets.size() == 0)
          {
                  destroyOnlySrc = true;
                  for( nitr = bnet.begin(); nitr != bnet.end(); ++nitr)
                  {
                          net = (dbNet *) *nitr;
                          cnt += net->destroyCCSegs(destroyOnlySrc);
                  }
                  notice (0, "delete {} CC segments.\n", cnt);
                  return;
          }
          int j;
          for (j=0;j<nets.size();j++)
          {
                  net = nets[j];
                  net->setMark(true);
                  net->selectCCSegs();
          }
          for( nitr = bnet.begin(); nitr != bnet.end(); ++nitr)
          {
                  net = (dbNet *) *nitr;
                  if (net->isSelect()==false)
                          continue;
                  net->setSelect(false);
                  net->detachSelectedCCSegs();
          }
          destroyOnlySrc = false;
          for (j=0;j<nets.size();j++)
          {
                  net = nets[j];
                  net->setMark(false);
                  cnt += net->destroyCCSegs(destroyOnlySrc);
          }
          notice (0, "delete {} CC segments.\n", cnt);
  */
}

/*
void extMain::unlinkCC(std::vector<dbNet *> & nets)
{
        int cnt = 0;
        bool destroyOnlySrc;
        dbNet *net;
        dbSet<dbNet> bnet = _block->getNets();
        dbSet<dbNet>::iterator nitr;
        if (nets.size() == 0)
        {
                destroyOnlySrc = true;
                for( nitr = bnet.begin(); nitr != bnet.end(); ++nitr)
                {
                        net = (dbNet *) *nitr;
                        net->unlinkCCSegs();
                        cnt++;
                }
                notice (0, "unlink CC segments of {} nets.\n", cnt);
                return;
        }
        int j;
        for (j=0;j<nets.size();j++)
        {
                net = nets[j];
                net->setMark(true);
                net->selectCCSegs();
        }
        for( nitr = bnet.begin(); nitr != bnet.end(); ++nitr)
        {
                net = (dbNet *) *nitr;
                if (net->isSelect()==false)
                        continue;
                net->setSelect(false);
                net->detachSelectedCCSegs();
        }
        destroyOnlySrc = false;
        for (j=0;j<nets.size();j++)
        {
                net = nets[j];
                net->setMark(false);
                net->unlinkCCSegs();
                cnt++;
        }
        notice (0, "unlink CC segments of {} nets.\n", cnt);
}

void extMain::removeCapNode(std::vector<dbNet *> & nets)
{
        int j;
        int cnt = 0;
        dbNet *net;
        dbSet<dbNet> bnet = _block->getNets();
        dbSet<dbNet>::iterator nitr;
        if (nets.size() == 0)
        {
                for( nitr = bnet.begin(); nitr != bnet.end(); ++nitr)
                {
                        net = (dbNet *) *nitr;
                        cnt += net->destroyCapNodes();
                }
                notice (0, "delete {} Cap Nodes.\n", cnt);
                return;
        }
        for (j=0;j<nets.size();j++)
        {
                net = nets[j];
                cnt += net->destroyCapNodes();
        }
        notice (0, "delete {} Cap Nodes.\n", cnt);
}

void extMain::unlinkCapNode(std::vector<dbNet *> & nets)
{
        int j;
        int cnt = 0;
        dbNet *net;
        dbSet<dbNet> bnet = _block->getNets();
        dbSet<dbNet>::iterator nitr;
        if (nets.size() == 0)
        {
                for( nitr = bnet.begin(); nitr != bnet.end(); ++nitr)
                {
                        net = (dbNet *) *nitr;
                        net->unlinkCapNodes();
                        cnt++;
                }
                notice (0, "delete {} Cap Nodes.\n", cnt);
                return;
        }
        for (j=0;j<nets.size();j++)
        {
                net = nets[j];
                net->unlinkCapNodes();
                cnt++;
        }
        notice (0, "unlink Cap Nodes of {} nets.\n", cnt);
}

void extMain::removeRSeg(std::vector<dbNet *> & nets)
{
        int j;
        int cnt = 0;
        dbNet *net;
        dbSet<dbNet> bnet = _block->getNets();
        dbSet<dbNet>::iterator nitr;
        if (nets.size() == 0)
        {
                for( nitr = bnet.begin(); nitr != bnet.end(); ++nitr)
                {
                        net = (dbNet *) *nitr;
                        cnt += net->destroyRSegs();
                }
                notice (0, "delete {} RC segments.\n", cnt);
                return;
        }
        for (j=0;j<nets.size();j++)
        {
                net = nets[j];
                cnt += net->destroyRSegs();
        }
        notice (0, "delete {} RC segments.\n", cnt);
}

void extMain::unlinkRSeg(std::vector<dbNet *> & nets)
{
        int j;
        int cnt = 0;
        dbNet *net;
        dbSet<dbNet> bnet = _block->getNets();
        dbSet<dbNet>::iterator nitr;
        if (nets.size() == 0)
        {
                for( nitr = bnet.begin(); nitr != bnet.end(); ++nitr)
                {
                        net = (dbNet *) *nitr;
                        net->unlinkRSegs();
                        cnt++;
                }
                notice (0, "unlink RC segments of {} nets.\n", cnt);
                return;
        }
        for (j=0;j<nets.size();j++)
        {
                net = nets[j];
                net->unlinkRSegs();
                cnt++;
        }
        notice (0, "unlink RC segments of {} nets.\n", cnt);
}
*/
void extMain::removeExt(std::vector<dbNet*>& nets) {
  // removeSdb (nets);
  _block->destroyParasitics(nets);
  // removeCC (nets);
  // removeRSeg (nets);
  // removeCapNode (nets);
  _extracted = false;
  if (_spef)
    _spef->reinit();
}
void extMain::removeExt() {
  std::vector<dbNet*> rnets;
  dbSet<dbNet> bnets = _block->getNets();
  dbSet<dbNet>::iterator net_itr;
  dbNet* net;
  for (net_itr = bnets.begin(); net_itr != bnets.end(); ++net_itr) {
    net = *net_itr;
    dbSigType type = net->getSigType();
    if ((type == dbSigType::POWER) || (type == dbSigType::GROUND))
      continue;
    rnets.push_back(net);
  }
  removeExt(rnets);
}
/*
void extMain::unlinkExt(std::vector<dbNet *> & nets)
{
        unlinkCC (nets);
        unlinkRSeg (nets);
        unlinkCapNode (nets);
        _extracted = false;
        if (_spef)
                _spef->reinit();
}
*/
void v_printWireNeighbor(void* ip, uint met, void* v_swire, void* v_topNeighbor,
                         void* v_botNeighbor) {}
void extCompute(CoupleOptions& inputTable, void* extModel);
void extCompute1(CoupleOptions& inputTable, void* extModel);

uint extMain::readCmpStats(const char* name, uint& tileSize, int& X1, int& Y1,
                           int& X2, int& Y2) {
  int x1 = X1;
  int y1 = Y1;
  int x2 = X2;
  int y2 = Y2;
  uint lcnt = 0;
  Ath__parser parser;
  FILE* fp = fopen(name, "r");
  if (fp == NULL) {
    logger_->warn(RCX, 373, "Can't open file {}.", name);
    return 0;
  }
  bool simple_flavor = false;
  uint nm = 1000;
  parser.setInputFP(fp);
  while (parser.parseNextLine() > 0) {
    if (parser.isKeyword(0, "VERSION")) {
      if (parser.isKeyword(1, "alpha"))
        simple_flavor = true;

      continue;
    }
    if (parser.isKeyword(0, "BLOCK")) {
      continue;
    }
    if (parser.isKeyword(0, "TILE_SIZE")) {
      tileSize = parser.getInt(1);
      continue;
    }
    if (parser.isKeyword(0, "BEGIN") && parser.isKeyword(1, "LAYER_MAPPING")) {
      while (parser.parseNextLine() > 0) {
        if (!parser.isKeyword(0, "END")) {
          continue;
        }
        break;
      }
      continue;
    }
    if (parser.isKeyword(0, "BEGIN") &&
        (simple_flavor || parser.isKeyword(2, "("))) {
      lcnt++;
      while (parser.parseNextLine() > 0) {
        if (parser.isKeyword(0, "END"))
          break;

        int X = Ath__double2int(parser.getDouble(0) * nm);
        int Y = Ath__double2int(parser.getDouble(1) * nm);
        x1 = MIN(x1, X);
        y1 = MIN(y1, Y);
        x2 = MAX(x2, X);
        y2 = MAX(y2, Y);
      }
      break;
    }
  }
  X1 = x1;
  Y1 = y1;
  X2 = x2;
  Y2 = y2;
  // fclose(fp);
  return lcnt;
}

uint extMain::readCmpFile(const char* name) {
  logger_->info(RCX, 117, "Read CMP file {} ...", name);

  uint tileSize;
  int X1 = MAXINT;
  int Y1 = MAXINT;
  int X2 = -MAXINT;
  int Y2 = -MAXINT;
  readCmpStats(name, tileSize, X1, Y1, X2, Y2);

  uint lefLayerCnt = 20;  // need an API here
  _geoThickTable = new extGeoThickTable* [lefLayerCnt + 1];
  for (uint ii = 0; ii <= lefLayerCnt; ii++)
    _geoThickTable[ii] = NULL;

  Ath__array1D<double> widthTable;
  Ath__array1D<double> B;

  bool simple_flavor = false;
  double angstrom2nm = 0.0001;

  uint lCnt = 0;
  uint cnt = 0;
  Ath__parser parser1;
  // parser1.addSeparator("\r");
  parser1.openFile((char*)name);
  while (parser1.parseNextLine() > 0) {
    if (parser1.isKeyword(0, "VERSION")) {
      if (parser1.isKeyword(1, "alpha"))
        simple_flavor = true;

      continue;
    }
    if (parser1.isKeyword(0, "BLOCK")) {
      continue;
    }
    if (parser1.isKeyword(0, "TILE_SIZE")) {
      continue;
    }
    if (parser1.isKeyword(0, "BEGIN") &&
        parser1.isKeyword(1, "LAYER_MAPPING")) {
      while (parser1.parseNextLine() > 0) {
        if (!parser1.isKeyword(0, "END")) {
          continue;
        }
        break;
      }
      continue;
    }
    if (parser1.isKeyword(0, "BEGIN") &&
        (simple_flavor || parser1.isKeyword(2, "("))) {
      lCnt++;
      uint level = lCnt;
      double nominalThickness = 0.5;
      char* layerName = NULL;
      if (simple_flavor) {
        layerName = parser1.get(1);
        nominalThickness = parser1.getDouble(2) * angstrom2nm;
        dbTechLayer* techLayer = _tech->findLayer(layerName);
        if (techLayer == NULL) {
          logger_->error(RCX, 118,
                         "Can't find the corresponding LEF layer for <{}>",
                         layerName);
        }
        level = techLayer->getRoutingLevel();
      }
      widthTable.resetCnt();
      parser1.getDoubleArray(&widthTable, 3);
      _geoThickTable[level] =
          new extGeoThickTable(X1, Y1, X2, Y2, tileSize, &widthTable, 1);

      uint nm = 1000;
      parser1.printWords(stdout);
      while (parser1.parseNextLine() > 0) {
        if (parser1.isKeyword(0, "END"))
          break;

        cnt++;
        // parser1.printWords(stdout);
        int X = Ath__double2int(parser1.getDouble(0) * nm);
        int Y = Ath__double2int(parser1.getDouble(1) * nm);

        double th = 0.0;
        double ild = 0.0;

        if (!simple_flavor) {
          th = parser1.getDouble(2);
          ild = parser1.getDouble(3);
        } else {
          th = nominalThickness;
        }

        B.resetCnt();
        if (simple_flavor)
          parser1.getDoubleArray(&B, 4, angstrom2nm);
        else
          parser1.getDoubleArray(&B, 4);

        _geoThickTable[lCnt]
            ->addVarTable(X, Y, th, ild, &B, simple_flavor, simple_flavor);
      }
      break;
    }
  }
  logger_->info(
      RCX, 119,
      "Finished reading {} variability tiles of size {} nm die: ({} {}) ({} "
      "{}) {}",
      cnt, tileSize, X1, Y1, X2, Y2, name);

  return cnt;
}
int extMain::setMinTypMax(bool min, bool typ, bool max, const char* cmp_file,
                          bool density_model, bool litho, int setMin,
                          int setTyp, int setMax, uint extDbCnt) {
  _modelMap.resetCnt(0);
  _metRCTable.resetCnt(0);
  _currentModel = NULL;
  if (cmp_file != NULL) {
    readCmpFile(cmp_file);
    _currentModel = getRCmodel(0);
    for (uint ii = 0; ii < 2; ii++) {
      _modelMap.add(0);
      _metRCTable.add(_currentModel->getMetRCTable(0));
    }
    if (!_eco)
      _block->setCornerCount(2);
    _extDbCnt = 2;
  } else if (extDbCnt > 1) {  // extract first <extDbCnt>
    if (!_eco)
      _block->setCornerCount(extDbCnt);
    _extDbCnt = extDbCnt;
    //		uint cnt= 0;

    int dbIndex = -1;
    dbIndex = _modelMap.add(_minModelIndex);
    //_block->setExtMinCorner(dbIndex);

    dbIndex = _modelMap.add(_typModelIndex);
    //_block->setExtTypCorner(dbIndex);

    if (extDbCnt > 2) {
      dbIndex = _modelMap.add(_maxModelIndex);
      //_block->setExtMaxCorner(dbIndex);
    }
  } else if (min || max || typ) {
    int dbIndex = -1;
    if (min) {
      dbIndex = _modelMap.add(_minModelIndex);
      //_block->setExtMinCorner(dbIndex);
    }
    if (typ) {
      dbIndex = _modelMap.add(_typModelIndex);
      //_block->setExtTypCorner(dbIndex);
    }
    if (max) {
      dbIndex = _modelMap.add(_maxModelIndex);
      //_block->setExtMaxCorner(dbIndex);
    }
    _extDbCnt = _modelMap.getCnt();

    if (!_eco)
      _block->setCornerCount(_extDbCnt);
  } else if ((setMin >= 0) || (setMax >= 0) || (setTyp >= 0)) {
    int dbIndex = -1;
    if (setMin >= 0) {
      dbIndex = _modelMap.add(setMin);
      //_block->setExtMinCorner(dbIndex);
    }
    if (setTyp >= 0) {
      dbIndex = _modelMap.add(setTyp);
      //_block->setExtMinCorner(dbIndex);
    }
    if (setMax >= 0) {
      dbIndex = _modelMap.add(setMax);
      //_block->setExtMinCorner(dbIndex);
    }
    _extDbCnt = _modelMap.getCnt();

    if (!_eco)
      _block->setCornerCount(_extDbCnt);
  } else if (extDbCnt == 1) {  // extract first <extDbCnt>
    if (!_eco)
      _block->setCornerCount(extDbCnt);
    _extDbCnt = extDbCnt;
    _modelMap.add(0);
    //_block->setExtMaxCorner(dbIndex);
  } else if (density_model) {
  } else if (litho) {
  }

  if (_currentModel == NULL) {
    _currentModel = getRCmodel(0);
    // for (uint ii= 0; (_couplingFlag>0) && (!_lefRC) && ii<_modelMap.getCnt();
    // ii++) {
    for (uint ii = 0; (!_lefRC) && ii < _modelMap.getCnt(); ii++) {
      uint jj = _modelMap.get(ii);
      _metRCTable.add(_currentModel->getMetRCTable(jj));
    }
  }
  _cornerCnt = _extDbCnt;  // the Cnt's are the same in the old flow

  return 0;
}
extCorner::extCorner() {
  _name = NULL;
  _model = 0;
  _dbIndex = -1;
  _scaledCornerIdx = -1;
  _resFactor = 1.0;
  _ccFactor = 1.0;
  _gndFactor = 1.0;
}
void extMain::getExtractedCorners() {
  if (_prevControl == NULL)
    return;
  if (_prevControl->_extractedCornerList == NULL)
    return;
  if (_processCornerTable != NULL)
    return;

  Ath__parser parser;
  uint pCornerCnt = parser.mkWords(_prevControl->_extractedCornerList, " ");
  if (pCornerCnt <= 0)
    return;

  _processCornerTable = new Ath__array1D<extCorner*>();

  uint cornerCnt = 0;
  uint ii, jj;
  char cName[128];
  for (ii = 0; ii < pCornerCnt; ii++) {
    extCorner* t = new extCorner();
    t->_model = parser.getInt(ii);

    t->_dbIndex = cornerCnt++;
    _processCornerTable->add(t);
  }

  if (_prevControl->_derivedCornerList == NULL) {
    makeCornerMapFromExtControl();
    return;
  }

  uint sCornerCnt = parser.mkWords(_prevControl->_derivedCornerList, " ");
  if (sCornerCnt <= 0)
    return;

  if (_scaledCornerTable == NULL)
    _scaledCornerTable = new Ath__array1D<extCorner*>();

  for (ii = 0; ii < sCornerCnt; ii++) {
    extCorner* t = new extCorner();
    t->_model = parser.getInt(ii);
    for (jj = 0; jj < pCornerCnt; jj++) {
      if (t->_model != _processCornerTable->get(jj)->_model)
        continue;
      t->_extCornerPtr = _processCornerTable->get(jj);
      break;
    }
    _block->getExtCornerName(pCornerCnt + ii, &cName[0]);
    if (jj == pCornerCnt)
      logger_->warn(RCX, 120,
                    "No matching process corner for scaled corner {}, model {}",
                    &cName[0], t->_model);
    t->_dbIndex = cornerCnt++;
    _scaledCornerTable->add(t);
  }
  Ath__array1D<double> A;

  parser.mkWords(_prevControl->_resFactorList, " ");
  parser.getDoubleArray(&A, 0);
  for (ii = 0; ii < sCornerCnt; ii++) {
    extCorner* t = _scaledCornerTable->get(ii);
    t->_resFactor = A.get(ii);
  }
  parser.mkWords(_prevControl->_ccFactorList, " ");
  A.resetCnt();
  parser.getDoubleArray(&A, 0);
  for (ii = 0; ii < sCornerCnt; ii++) {
    extCorner* t = _scaledCornerTable->get(ii);
    t->_ccFactor = A.get(ii);
  }
  parser.mkWords(_prevControl->_gndcFactorList, " ");
  A.resetCnt();
  parser.getDoubleArray(&A, 0);
  for (ii = 0; ii < sCornerCnt; ii++) {
    extCorner* t = _scaledCornerTable->get(ii);
    t->_gndFactor = A.get(ii);
  }
  makeCornerMapFromExtControl();
}
void extMain::makeCornerMapFromExtControl() {
  if (_prevControl->_cornerIndexList == NULL)
    return;
  if (_processCornerTable == NULL)
    return;
  // if (_scaledCornerTable==NULL)
  //	return;

  Ath__parser parser;
  uint wordCnt = parser.mkWords(_prevControl->_cornerIndexList, " ");
  if (wordCnt <= 0)
    return;

  char cName[128];
  for (uint ii = 0; ii < wordCnt; ii++) {
    int index = parser.getInt(ii);
    extCorner* t = NULL;
    if (index > 0) {  // extracted corner
      t = _processCornerTable->get(index - 1);
      t->_dbIndex = ii;
    } else {
      t = _scaledCornerTable->get((-index) - 1);
    }
    t->_dbIndex = ii;
    _block->getExtCornerName(ii, &cName[0]);
    t->_name = strdup(&cName[0]);
  }
}
char* extMain::addRCCorner(const char* name, int model, int userDefined) {
  _remote = 0;
  if (model >= 100) {
    _remote = 1;
    model = model - 100;
  }
  if (_processCornerTable == NULL)
    _processCornerTable = new Ath__array1D<extCorner*>();

  uint ii = 0;
  for (; ii < _processCornerTable->getCnt(); ii++) {
    extCorner* s = _processCornerTable->get(ii);
    if (s->_model == model) {
      logger_->info(
          RCX, 433,
          "A process corner {} for Extraction RC Model {} has already been "
          "defined, skipping definition",
          s->_name, model);
      return NULL;
    }
  }
  extCorner* t = new extCorner();
  t->_model = model;
  t->_dbIndex = _cornerCnt++;
  _processCornerTable->add(t);
  if (name != NULL)
    t->_name = strdup(name);
  else {
    char buff[16];
    sprintf(buff, "MinMax%d", model);
    t->_name = strdup(buff);
  }
  t->_extCornerPtr = NULL;

  if (userDefined == 1)
    logger_->info(RCX, 431, "Defined process_corner {} with ext_model_index {}",
                  t->_name, model);
  else if (userDefined == 0)
    logger_->info(RCX, 434,
                  "Defined process_corner {} with ext_model_index {} (using "
                  "extRulesFile defaults)",
                  t->_name, model);

  if (!_remote)
    makeCornerNameMap();
  return t->_name;
}
char* extMain::addRCCornerScaled(const char* name, uint model, float resFactor,
                                 float ccFactor, float gndFactor) {
  if (_processCornerTable == NULL) {
    logger_->info(
        RCX, 472,
        "The corresponding process corner has to be defined using the "
        "command <define_process_corner>");
    return NULL;
  }

  uint jj = 0;
  extCorner* pc = NULL;
  for (; jj < _processCornerTable->getCnt(); jj++) {
    pc = _processCornerTable->get(jj);
    if (pc->_model == (int)model)
      break;
  }
  if (jj == _processCornerTable->getCnt()) {
    logger_->info(
        RCX, 121,
        "The corresponding process corner has to be defined using the "
        "command <define_process_corner>");
    return NULL;
  }
  if (_scaledCornerTable == NULL)
    _scaledCornerTable = new Ath__array1D<extCorner*>();

  uint ii = 0;
  for (; ii < _scaledCornerTable->getCnt(); ii++) {
    extCorner* s = _scaledCornerTable->get(ii);

    if ((name != NULL) && (strcmp(s->_name, name) == 0)) {
      // if (s->_model==model) {
      logger_->info(
          RCX, 122,
          "A process corner for Extraction RC Model {} has already been "
          "defined, skipping definition",
          model);
      return NULL;
    }
  }
  pc->_scaledCornerIdx = _scaledCornerTable->getCnt();
  extCorner* t = new extCorner();
  t->_model = model;
  t->_dbIndex = _cornerCnt++;
  t->_resFactor = resFactor;
  t->_ccFactor = ccFactor;
  t->_gndFactor = gndFactor;
  t->_extCornerPtr = pc;

  _scaledCornerTable->add(t);
  if (name != NULL)
    t->_name = strdup(name);
  else {
    char buff[16];
    sprintf(buff, "derived_MinMax%d", model);
    t->_name = strdup(buff);
  }
  makeCornerNameMap();
  return t->_name;
}
void extMain::cleanCornerTables() {
  if (_scaledCornerTable != NULL) {
    for (uint ii = 0; ii < _scaledCornerTable->getCnt(); ii++) {
      extCorner* s = _scaledCornerTable->get(ii);

      free(s->_name);
      delete s;
    }
    delete _scaledCornerTable;
  }
  _scaledCornerTable = NULL;
  if (_processCornerTable != NULL) {
    for (uint ii = 0; ii < _processCornerTable->getCnt(); ii++) {
      extCorner* s = _processCornerTable->get(ii);

      free(s->_name);
      delete s;
    }
    delete _processCornerTable;
  }
  _processCornerTable = NULL;
}

int extMain::getProcessCornerDbIndex(int pcidx) {
  if (_processCornerTable == NULL)
    return pcidx;
  assert(pcidx >= 0 && pcidx < (int)_processCornerTable->getCnt());
  return (_processCornerTable->get(pcidx)->_dbIndex);
}

void extMain::getScaledCornerDbIndex(int pcidx, int& scidx, int& scdbIdx) {
  scidx = -1;
  if (_batchScaleExt || _processCornerTable == NULL)
    return;
  assert(pcidx >= 0 && pcidx < (int)_processCornerTable->getCnt());
  scidx = _processCornerTable->get(pcidx)->_scaledCornerIdx;
  if (scidx != -1)
    scdbIdx = _scaledCornerTable->get(scidx)->_dbIndex;
}

void extMain::getScaledRC(int sidx, double& res, double& cap) {
  extCorner* sc = _scaledCornerTable->get(sidx);
  res = sc->_resFactor == 1.0 ? res : res * sc->_resFactor;
  cap = sc->_gndFactor == 1.0 ? cap : cap * sc->_gndFactor;
}

void extMain::getScaledGndC(int sidx, double& cap) {
  extCorner* sc = _scaledCornerTable->get(sidx);
  cap = sc->_gndFactor == 1.0 ? cap : cap * sc->_gndFactor;
}

void extMain::getScaledCC(int sidx, double& cap) {
  extCorner* sc = _scaledCornerTable->get(sidx);
  cap = sc->_ccFactor == 1.0 ? cap : cap * sc->_ccFactor;
}

void extMain::deleteCorners() {
  cleanCornerTables();
  _block->setCornerCount(1);
}

void extMain::getCorners(std::list<std::string>& ecl) {
  char buffer[64];
  uint ii;
  for (ii = 0; _processCornerTable && ii < _processCornerTable->getCnt();
       ii++) {
    std::string s1c("");
    extCorner* ec = _processCornerTable->get(ii);
    sprintf(buffer, "%s %d %d", ec->_name, ec->_model + 1, ec->_dbIndex);
    s1c += buffer;
    ecl.push_back(s1c);
  }
  for (ii = 0; _scaledCornerTable && ii < _scaledCornerTable->getCnt(); ii++) {
    std::string s1c("");
    extCorner* ec = _scaledCornerTable->get(ii);
    sprintf(buffer, "%s %d %d", ec->_name, -(ec->_model + 1), ec->_dbIndex);
    s1c += buffer;
    ecl.push_back(s1c);
  }
}

int extMain::getDbCornerIndex(const char* name) {
  if (_scaledCornerTable != NULL) {
    for (uint ii = 0; ii < _scaledCornerTable->getCnt(); ii++) {
      extCorner* s = _scaledCornerTable->get(ii);

      if (strcmp(s->_name, name) == 0)
        return s->_dbIndex;
    }
  }
  if (_processCornerTable != NULL) {
    for (uint ii = 0; ii < _processCornerTable->getCnt(); ii++) {
      extCorner* s = _processCornerTable->get(ii);

      if (strcmp(s->_name, name) == 0)
        return s->_dbIndex;
    }
  }
  return -1;
}
int extMain::getDbCornerModel(const char* name) {
  if (_scaledCornerTable != NULL) {
    for (uint ii = 0; ii < _scaledCornerTable->getCnt(); ii++) {
      extCorner* s = _scaledCornerTable->get(ii);

      if (strcmp(s->_name, name) == 0)
        return s->_model;
    }
  }
  if (_processCornerTable != NULL) {
    for (uint ii = 0; ii < _processCornerTable->getCnt(); ii++) {
      extCorner* s = _processCornerTable->get(ii);

      if (strcmp(s->_name, name) == 0)
        return s->_model;
    }
  }
  return -1;
}
// void extMain::makeCornerNameMap(char *buff, int cornerCnt, bool spef)
void extMain::makeCornerNameMap() {
  // This function updates the dbExtControl object and
  // creates the corner information to be stored at dbBlock object

  int A[128];
  extCorner** map = new extCorner* [_cornerCnt];
  for (uint jj = 0; jj < _cornerCnt; jj++) {
    map[jj] = NULL;
    A[jj] = 0;
  }

  char cornerList[128];
  strcpy(cornerList, "");

  if (_scaledCornerTable != NULL) {
    char extList[128];
    char resList[128];
    char ccList[128];
    char gndcList[128];
    strcpy(extList, "");
    strcpy(ccList, "");
    strcpy(resList, "");
    strcpy(gndcList, "");
    for (uint ii = 0; ii < _scaledCornerTable->getCnt(); ii++) {
      extCorner* s = _scaledCornerTable->get(ii);

      // if (spef) {
      //	map[s->_model]= s;
      //	A[s->_model]= -(ii+1);
      //}
      // else {
      //	map[s->_dbIndex]= s;
      //	A[s->_dbIndex]= -(ii+1);
      //}
      map[s->_dbIndex] = s;
      A[s->_dbIndex] = -(ii + 1);

      sprintf(extList, "%s %d", extList, s->_model);
      sprintf(resList, "%s %g", resList, s->_resFactor);
      sprintf(ccList, "%s %g", ccList, s->_ccFactor);
      sprintf(gndcList, "%s %g", gndcList, s->_gndFactor);
    }
    if (_prevControl->_derivedCornerList)
      free(_prevControl->_derivedCornerList);
    if (_prevControl->_resFactorList)
      free(_prevControl->_resFactorList);
    if (_prevControl->_ccFactorList)
      free(_prevControl->_ccFactorList);
    if (_prevControl->_gndcFactorList)
      free(_prevControl->_gndcFactorList);
    _prevControl->_derivedCornerList = strdup(extList);
    _prevControl->_resFactorList = strdup(resList);
    _prevControl->_ccFactorList = strdup(ccList);
    _prevControl->_gndcFactorList = strdup(gndcList);
  }
  if (_processCornerTable != NULL) {
    char extList[128];
    strcpy(extList, "");

    for (uint ii = 0; ii < _processCornerTable->getCnt(); ii++) {
      extCorner* s = _processCornerTable->get(ii);

      // if (spef) {
      //	A[s->_model]= ii+1;
      //	map[s->_model]= s;
      //}
      // else {
      //	A[s->_dbIndex]= ii+1;
      //	map[s->_dbIndex]= s;
      //}
      A[s->_dbIndex] = ii + 1;
      map[s->_dbIndex] = s;

      sprintf(extList, "%s %d", extList, s->_model);
    }
    if (_prevControl->_extractedCornerList)
      free(_prevControl->_extractedCornerList);
    _prevControl->_extractedCornerList = strdup(extList);
  }
  // if (_extDbCnt<=0) {
  // 	delete [] map;
  // 	return;
  // }
  char aList[128];
  strcpy(aList, "");

  for (uint k = 0; k < _cornerCnt; k++) {
    sprintf(aList, "%s %d", aList, A[k]);
  }
  if (_prevControl->_cornerIndexList)
    free(_prevControl->_cornerIndexList);
  _prevControl->_cornerIndexList = strdup(aList);

  char buff[1024];
  if (map[0] == NULL)
    sprintf(buff, "%s %d", buff, 0);
  else
    sprintf(buff, "%s", map[0]->_name);

  for (uint ii = 1; ii < _cornerCnt; ii++) {
    extCorner* s = map[ii];
    if (s == NULL)
      sprintf(buff, "%s %d", buff, ii);
    else
      sprintf(buff, "%s %s", buff, s->_name);
  }
  if (!_remote) {
    _block->setCornerCount(_cornerCnt);
    _block->setCornerNameList(buff);
  }

  delete[] map;
  updatePrevControl();
}
bool extMain::setCorners(const char* rulesFileName, const char* cmp_file) {
  if (cmp_file != NULL)
    readCmpFile(cmp_file);

  _modelMap.resetCnt(0);
  uint ii;
  /*	for (ii= 0; ii<_processCornerTable->getCnt(); ii++) {
                  extCorner *s= _processCornerTable->get(ii);
                  _modelMap.add(s->_model);
          }*/
  _metRCTable.resetCnt(0);

  if (rulesFileName != NULL) {  // read rules

    int dbunit = _block->getDbUnitsPerMicron();
    double dbFactor = 1;
    if (dbunit > 1000)
      dbFactor = dbunit * 0.001;

    extRCModel* m = new extRCModel("MINTYPMAX", logger_);
    _modelTable->add(m);

    uint cornerTable[10];
    uint extDbCnt = 0;

    _minModelIndex = 0;
    _maxModelIndex = 0;
    _typModelIndex = 0;
    if (_processCornerTable != NULL) {
      for (uint ii = 0; ii < _processCornerTable->getCnt(); ii++) {
        extCorner* s = _processCornerTable->get(ii);
        cornerTable[extDbCnt++] = s->_model;
        _modelMap.add(ii);
      }
    }

    logger_->info(RCX, 435, "Reading extraction model file {} ...",
                  rulesFileName);

    if (!fopen(rulesFileName, "r"))
      logger_->error(RCX, 468, "Can't open extraction model file {}",
                     rulesFileName);

    if (!(m->readRules((char*)rulesFileName, false, true, true, true, true,
                       extDbCnt, cornerTable, dbFactor))) {
      delete m;
      return false;
    }

    int modelCnt = getRCmodel(0)->getModelCnt();

    // If RCX reads wrong extRules file format
    if (modelCnt == 0)
      logger_->error(RCX, 469,
                     "No RC model read from the extraction model! "
                     "Ensure the right extRules file is used!");

    if (cmp_file != NULL) {  // find 0.0% variability and make it first
      _currentModel = getRCmodel(0);

      int n_0 = _currentModel->findVariationZero(0.0);
      if (_processCornerTable != NULL) {
        cleanCornerTables();
        logger_->info(RCX, 124,
                      "Deleted already defined corners, only one corner will "
                      "automatically defined when option cmp_file is used");
      }
      addRCCorner("CMP", n_0, 0);
      _modelMap.add(n_0);
      _metRCTable.add(_currentModel->getMetRCTable(n_0));
      _extDbCnt = _processCornerTable->getCnt();

      assert(_cornerCnt == _extDbCnt);

      _block->setCornerCount(_cornerCnt, _extDbCnt, NULL);
      return true;
    } else if (_processCornerTable == NULL) {
      for (int ii = 0; ii < modelCnt; ii++) {
        addRCCorner(NULL, ii, 0);
        _modelMap.add(ii);
      }
    }
  }
  _currentModel = getRCmodel(0);
  for (ii = 0; (_couplingFlag > 0) && (!_lefRC) && ii < _modelMap.getCnt();
       ii++) {
    uint jj = _modelMap.get(ii);
    _metRCTable.add(_currentModel->getMetRCTable(jj));
  }
  _extDbCnt = _processCornerTable->getCnt();

  uint scaleCornerCnt = 0;
  if (_scaledCornerTable != NULL)
    scaleCornerCnt = _scaledCornerTable->getCnt();

  assert(_cornerCnt == _extDbCnt + scaleCornerCnt);

  // char cornerNameList[1024];
  // makeCornerNameMap(cornerNameList, _cornerCnt, false);

  _block->setCornerCount(_cornerCnt, _extDbCnt, NULL);
  return true;
}

void extMain::addDummyCorners(uint cornerCnt) {
  for (uint ii = 0; ii < cornerCnt; ii++)
    addRCCorner(NULL, ii, -1);
}

void extMain::updatePrevControl() {
  _prevControl->_independentExtCorners = _independentExtCorners;
  _prevControl->_foreign = _foreign;
  _prevControl->_rsegCoord = _rsegCoord;
  _prevControl->_overCell = _overCell;
  _prevControl->_extracted = _extracted;
  _prevControl->_lefRC = _lefRC;
  // _prevControl->_extDbCnt = _extDbCnt;
  _prevControl->_cornerCnt = _cornerCnt;
  _prevControl->_ccPreseveGeom = _ccPreseveGeom;
  _prevControl->_ccUp = _ccUp;
  _prevControl->_couplingFlag = _couplingFlag;
  _prevControl->_coupleThreshold = _coupleThreshold;
  _prevControl->_mergeResBound = _mergeResBound;
  _prevControl->_mergeViaRes = _mergeViaRes;
  _prevControl->_mergeParallelCC = _mergeParallelCC;
  _prevControl->_useDbSdb = _useDbSdb;
  _prevControl->_CCnoPowerSource = _CCnoPowerSource;
  _prevControl->_CCnoPowerTarget = _CCnoPowerTarget;
  _prevControl->_usingMetalPlanes = _usingMetalPlanes;
  if (_prevControl->_ruleFileName)
    free(_prevControl->_ruleFileName);
  if (_currentModel && _currentModel->getRuleFileName())
    _prevControl->_ruleFileName = strdup(_currentModel->getRuleFileName());
}

void extMain::getPrevControl() {
  if (!_prevControl)
    return;
  _independentExtCorners = _prevControl->_independentExtCorners;
  _foreign = _prevControl->_foreign;
  _rsegCoord = _prevControl->_rsegCoord;
  _overCell = _prevControl->_overCell;
  _extracted = _prevControl->_extracted;
  _lefRC = _prevControl->_lefRC;
  // _extDbCnt = _prevControl->_extDbCnt;
  _cornerCnt = _prevControl->_cornerCnt;
  _ccPreseveGeom = _prevControl->_ccPreseveGeom;
  _ccUp = _prevControl->_ccUp;
  _couplingFlag = _prevControl->_couplingFlag;
  _coupleThreshold = _prevControl->_coupleThreshold;
  _mergeResBound = _prevControl->_mergeResBound;
  _mergeViaRes = _prevControl->_mergeViaRes;
  _mergeParallelCC = _prevControl->_mergeParallelCC;
  _useDbSdb = _prevControl->_useDbSdb;
  _CCnoPowerSource = _prevControl->_CCnoPowerSource;
  _CCnoPowerTarget = _prevControl->_CCnoPowerTarget;
  _usingMetalPlanes = _prevControl->_usingMetalPlanes;
  //	if ((_prevControl->_ruleFileName && !_currentModel->getRuleFileName())
  //||
  //	    (!_prevControl->_ruleFileName && !_currentModel->getRuleFileName())
  //|| (strcmp(_prevControl->_ruleFileName,_currentModel->getRuleFileName())))
}

uint extMain::makeBlockRCsegs(bool btermThresholdFlag, const char* cmp_file,
                              bool density_model, bool litho,
                              const char* netNames, const char* bbox,
                              const char* ibox, uint cc_up, uint ccFlag,
                              int ccBandTracks, uint use_signal_table,
                              double resBound, bool mergeViaRes, uint debug,
                              int preserve_geom, bool re_extract, bool eco,
                              bool gs, bool rlog, ZPtr<ISdb> netSdb,
                              double ccThres, int contextDepth, bool overCell,
                              const char* extRules, ZInterface* Interface) {
  uint debugNetId = 0;
  if (preserve_geom < 0) {
    debugNetId = -preserve_geom;
    preserve_geom = 0;
  }

  _cc_band_tracks = ccBandTracks;
  _use_signal_tables = use_signal_table;

  if (debug == 703) {
    logger_->info(RCX, 473, "Initial Tiling {} ...",
                  getBlock()->getName().c_str());

    Rect maxRect;
    _block->getDieArea(maxRect);
    logger_->info(RCX, 125, "Tiling for die area {}x{} = {} {}  {} {}",
                  maxRect.dx(), maxRect.dy(), maxRect.xMin(), maxRect.yMin(),
                  maxRect.xMax(), maxRect.yMax());

    _use_signal_tables = 3;
    createWindowsDB(rlog, maxRect, ccBandTracks, ccFlag, use_signal_table);
    return 0;
  }
  if (debug == 803) {
    Rect maxRect;
    _block->getDieArea(maxRect);

    _use_signal_tables = 3;
    uint wireCnt = fillWindowsDB(rlog, maxRect, use_signal_table);
    logger_->info(RCX, 126, "Block {} has {} ext Wires",
                  getBlock()->getName().c_str(), wireCnt);
    return wireCnt;
  }
  bool initTiling = false;
  bool windowFlow = false;
  bool skipExtractionAfterRcGen = false;
  bool doExt = false;
  if (debug == 101) {
    _batchScaleExt = false;
    debug = 0;
  } else if (debug == 100) {
    _reuseMetalFill = true;
    debug = 0;
  } else if (debug == 99) {
    _extNetSDB = netSdb;
    return 0;  // "-test 99", to check sdb tracks and wires
  } else if (debug == 103) {
    _getBandWire = true;
    debug = 0;
  } else if (debug == 104) {
    _printBandInfo = true;
    debug = 0;
  } else if (debug == 105) {
    _getBandWire = true;
    _printBandInfo = true;
    debug = 0;
  } else if (debug == 106) {
    _extRun = 0;
    removeExt();
    debug = 0;
  } else if (debug == 104) {
    _extRun = 0;
    removeExt();
    debug = 0;
  } else if (debug == 501) {
    windowFlow = true;
    debug = 0;
    _getBandWire = false;
  } else if (debug == 503) {
    windowFlow = true;
    debug = 0;
    _getBandWire = false;
    doExt = true;
  } else if (debug == 603) {
    windowFlow = false;
    debug = 0;
    _getBandWire = false;
    doExt = false;
    skipExtractionAfterRcGen = true;
  } else if (debug == 773) {
    windowFlow = false;
    debug = 0;
    _getBandWire = false;
    doExt = false;
    skipExtractionAfterRcGen = false;
    initTiling = true;
  }
  _diagFlow = true;

  int detailRlog = 0;
  int ttttRemoveSdb = 1;
  int ttttRemoveGs = 1;
  int setBlockPtfile = 0;
  std::vector<dbNet*> inets;
  if ((_prevControl->_ruleFileName == NULL) &&
      (!_lefRC && (getRCmodel(0) == NULL) && (extRules == NULL))) {
    logger_->warn(RCX, 127,
                  "No RC model was read with command <load_model>, "
                  "will not perform extraction!");
    return 0;
  }
  if (!re_extract) {
    _couplingFlag = ccFlag;
    _coupleThreshold = ccThres;
    _reExtract = re_extract;
    _ccPreseveGeom = preserve_geom;

    _debug = debug;
    if (!_lefRC) {
      _usingMetalPlanes = gs;
      _ccUp = cc_up;
      _couplingFlag = ccFlag;
      _ccContextDepth = contextDepth;
      if (_usingMetalPlanes)
        _overCell = overCell;
    } else {
      _usingMetalPlanes = 0;
      _ccUp = 0;
      _couplingFlag = 0;
      _ccContextDepth = 0;
    }
    _mergeViaRes = mergeViaRes;
    _mergeResBound = resBound;
  }
  _eco = eco;
  if (eco && _extRun == 0) {
    getPrevControl();
    getExtractedCorners();
  }
  if (!eco) {
    if (netSdb && netSdb == _extNetSDB)
      netSdb->reMakeSdb(_tech, _block);
  }
  // if ( _extRun==0 || (!re_extract && !eco )) {
  if ((_processCornerTable != NULL) ||
      ((_processCornerTable == NULL) && (extRules != NULL))) {
    char* rulesfile = extRules ? (char*)extRules : _prevControl->_ruleFileName;
    if (debug != 777) {
      if (!setCorners(rulesfile,
                      cmp_file)) {  // DKF:12/22 -- cmp_file for testing,
                                    // eventually: rulesFileName
        logger_->info(RCX, 128, "skipping Extraction ...");
        return 0;
      }
    }
    // if (cmp_file!=NULL)
    //	readCmpFile(cmp_file);
  }
  // removed opptions
  // else if (setMinTypMax(minModel, typModel, maxModel, cmp_file,
  // density_model, litho, 	setMin, setTyp, setMax, extDbCnt)<0) {
  else if (setMinTypMax(false, false, false, cmp_file, density_model, litho, -1,
                        -1, -1, 1) < 0) {
    logger_->warn(RCX, 129, "Wrong combination of corner related options!");
    return 0;
  }
  _foreign = false;  // extract after read_spef
  // }

  if (ibox) {
    logger_->info(RCX, 130, "Ibox = {}", ibox);
    Ath__parser* parser = new Ath__parser();
    parser->mkWords(ibox, NULL);
    _ibox = new Rect(atoi(parser->get(0)), atoi(parser->get(1)),
                     atoi(parser->get(2)), atoi(parser->get(3)));
    logger_->info(RCX, 131, "_ibox = {} {} {} {}", _ibox->xMin(), _ibox->yMin(),
                  _ibox->xMax(), _ibox->yMax());
    eco = true;
  } else
    _ibox = NULL;
  if (eco) {
    _block->getWireUpdatedNets(inets, _ibox);
    if (inets.size() != 0)
      logger_->info(RCX, 132, "Eco extract {} nets.", inets.size());
    else {
      logger_->info(RCX, 133, "No nets to eco extract.");
      return 1;
    }
    removeExt(inets);
    _reExtract = true;
    _allNet = false;
    preserve_geom = 1;
  } else {
    _allNet = !((dbBlock*)_block)->findSomeNet(netNames, inets);
  }

  // if (remove_ext)
  //{
  //	removeExt (inets);
  //	return 1;
  //}
  // if (unlink_ext)
  //{
  // unlinkExt (inets);
  //	return 1;
  //}
  // if (remove_cc)
  //{
  //	removeCC (inets);
  //	return 1;
  //}

  if (!_reExtract || _extRun == 0 || _alwaysNewGs) {
    //_block->setCornerCount(extDbCnt);
    //_extDbCnt = extDbCnt;

    //		if (log)
    //			AthResourceLog ("Start Planes", 0);

    if (!_lefRC) {
      if (_cc_band_tracks == 0) {
        //#ifndef NEW_GS_FLOW
        if (_usingMetalPlanes && (_geoThickTable == NULL)) {
          if (rlog)
            AthResourceLog("before initPlanes", detailRlog);
          initPlanes(_currentModel->getLayerCnt() + 1);
          if (rlog)
            AthResourceLog("after initPlanes", detailRlog);
          addPowerGs();
          if (rlog)
            AthResourceLog("after addPowerGs", detailRlog);
          if (_alwaysNewGs)
            addSignalGs();
          if (rlog)
            AthResourceLog("after addSignalGs", detailRlog);
          if (_overCell)
            addInstShapesOnPlanes();
          if (rlog)
            AthResourceLog("after addInstShapesOnPlanes", detailRlog);
        }
      }
      //#endif
      if (_ccContextDepth)
        initContextArray();
    }
    initDgContextArray();
  }
  _extRun++;
  if (rlog)
    AthResourceLog("start extract", detailRlog);

#ifdef ZDEBUG
  Interface->event("CC", "_couplingFlag", Z_INT, _couplingFlag, NULL);
#endif

  extMeasure m;
  m.setLogger(logger_);

  _seqPool = m._seqPool;
  _useDbSdb = false;
  if (netSdb) {
    _extNetSDB = netSdb;
    Ath__overlapAdjust overlapAdj = Z_noAdjust;
    _useDbSdb = true;
    _extNetSDB->setExtControl(
        _block, _useDbSdb, (uint)overlapAdj, _CCnoPowerSource, _CCnoPowerTarget,
        _ccUp, _allNet, _ccContextDepth, _ccContextArray, _ccContextLength,
        _dgContextArray, &_dgContextDepth, &_dgContextPlanes, &_dgContextTracks,
        &_dgContextBaseLvl, &_dgContextLowLvl, &_dgContextHiLvl,
        _dgContextBaseTrack, _dgContextLowTrack, _dgContextHiTrack,
        _dgContextTrackBase, _seqPool);
    setExtractionBbox(bbox);
  }
  // else
  else if (_cc_band_tracks == 0) {
    setupSearchDb(bbox, debug, Interface);
  }

  dbNet* net;
  uint j;
  for (j = 0; j < inets.size(); j++) {
    net = inets[j];
    net->setMark(true);
  }
  dbSet<dbNet> bnets = _block->getNets();
  dbSet<dbNet>::iterator net_itr;

  uint cnt = 0;

  if (!_allNet) {
    _ccMinX = MAX_INT;
    _ccMinY = MAX_INT;
    _ccMaxX = -MAX_INT;
    _ccMaxY = -MAX_INT;
  }
  if (_couplingFlag > 1 && !_lefRC) {
    // if (!getResCapTable(true))
    //	return 1;
    getResCapTable(true);
  }

  logger_->info(RCX, 436, "RC segment generation {} (max_merge_res {}) ...",
                getBlock()->getName().c_str(), _mergeResBound);
  uint itermCntEst = 3 * bnets.size();
  setupMapping(itermCntEst);

  if (_reuseMetalFill)
    _extNetSDB->adjustMetalFill();
  //	uint netCnt= 0;
  //	for( net_itr = bnets.begin(); !windowFlow && net_itr != bnets.end();
  //++net_itr ) {

  dbIntProperty* p = (dbIntProperty*)dbProperty::find(_block, "_currentDir");

  if ((p == NULL) && (debug != 777) && !initTiling) {
    if (_power_extract_only) {
      powerRCGen();
      return 1;
    }

    for (net_itr = bnets.begin(); net_itr != bnets.end(); ++net_itr) {
      net = *net_itr;

      dbSigType type = net->getSigType();
      if ((type == dbSigType::POWER) || (type == dbSigType::GROUND))
        continue;
      if (!_allNet && !net->isMarked())
        continue;

      _connectedBTerm.clear();
      _connectedITerm.clear();
      cnt += makeNetRCsegs(net);
      uint tt;
      for (tt = 0; tt < _connectedBTerm.size(); tt++)
        ((dbBTerm*)_connectedBTerm[tt])->setMark(0);
      for (tt = 0; tt < _connectedITerm.size(); tt++)
        ((dbITerm*)_connectedITerm[tt])->setMark(0);
      // break;
    }

    logger_->info(RCX, 40, "Final {} rc segments", cnt);
  } else if (debug == 777) {
    _debug = 0;
  } else
    _debug = 0;

  if (rlog)
    AthResourceLog("after makeNetRCsegs", detailRlog);
  int ttttPrintDgContext = 0;
  if (!skipExtractionAfterRcGen && (_couplingFlag > 1)) {
    logger_->info(RCX, 439, "Coupling Cap extraction {} ...",
                  getBlock()->getName().c_str());

    uint CCflag = _couplingFlag;
    //		if (_couplingFlag>20) {
    //			CCflag= _couplingFlag % 10;
    //		}

    if (!_allNet)
      _extNetSDB->setMaxArea(_ccMinX, _ccMinY, _ccMaxX, _ccMaxY);

    // ZPtr<ISdb> ccCapSdb = _reExtract ? _reExtCcapSDB : _extCcapSDB;
    ZPtr<ISdb> ccCapSdb = _extCcapSDB;

    _totCCcnt = 0;
    _totSmallCCcnt = 0;
    _totBigCCcnt = 0;
    _totSegCnt = 0;
    _totSignalSegCnt = 0;

    uint intersectCnt;
    if (_usingMetalPlanes && (_geoThickTable == NULL)) {
      intersectCnt = makeIntersectPlanes(0);
      if (rlog)
        AthResourceLog("after makeIntersectPlanes", detailRlog);
    }

    uint ccCnt;
    if (_debug) {
      if (_debug != 102)
        ccCnt =
            _extNetSDB->couplingCaps(ccCapSdb, CCflag, Interface, NULL, NULL);
    } else {
      logger_->info(RCX, 440,
                    "Coupling threshhold is {:.4f} fF, coupling capacitance "
                    "less than {:.4f} "
                    "fF will be grounded.",
                    _coupleThreshold, _coupleThreshold);
      if (_unifiedMeasureInit) {
        m._extMain = this;
        m._block = _block;
        m._diagFlow = _diagFlow;

        m._resFactor = _resFactor;
        m._resModify = _resModify;
        m._ccFactor = _ccFactor;
        m._ccModify = _ccModify;
        m._gndcFactor = _gndcFactor;
        m._gndcModify = _gndcModify;

        m._dgContextArray = _dgContextArray;
        m._dgContextDepth = &_dgContextDepth;
        m._dgContextPlanes = &_dgContextPlanes;
        m._dgContextTracks = &_dgContextTracks;
        m._dgContextBaseLvl = &_dgContextBaseLvl;
        m._dgContextLowLvl = &_dgContextLowLvl;
        m._dgContextHiLvl = &_dgContextHiLvl;
        m._dgContextBaseTrack = _dgContextBaseTrack;
        m._dgContextLowTrack = _dgContextLowTrack;
        m._dgContextHiTrack = _dgContextHiTrack;
        m._dgContextTrackBase = _dgContextTrackBase;
        if (ttttPrintDgContext)
          m._dgContextFile = fopen("dgCtxtFile", "w");
        m._dgContextCnt = 0;

        m._ccContextLength = _ccContextLength;
        m._ccContextArray = _ccContextArray;

        m._ouPixelTableIndexMap = _overUnderPlaneLayerMap;
        m._pixelTable = _geomSeq;
        m._minModelIndex = 0;  // couplimg threshold will be appled to this cap
        m._maxModelIndex = 0;
        m._currentModel = _currentModel;
        m._diagModel = _currentModel[0].getDiagModel();
        for (uint ii = 0; !_lefRC && ii < _modelMap.getCnt(); ii++) {
          uint jj = _modelMap.get(ii);
          m._metRCTable.add(_currentModel->getMetRCTable(jj));
        }
        // m._layerCnt= getExtLayerCnt(_tech); // TEST
        // m._layerCnt= _currentModel->getLayerCnt(); // TEST
        uint techLayerCnt = getExtLayerCnt(_tech) + 1;
        uint modelLayerCnt = _currentModel->getLayerCnt();
        m._layerCnt =
            techLayerCnt < modelLayerCnt ? techLayerCnt : modelLayerCnt;
        if (techLayerCnt == 5 && modelLayerCnt == 8)
          m._layerCnt = modelLayerCnt;
        //				m._cornerMapTable[0]= 1;
        //				m._cornerMapTable[1]= 0;
        m.getMinWidth(_tech);
        m.allocOUpool();

        m._btermThreshold = btermThresholdFlag;

        m._debugFP = NULL;
        m._netId = 0;
        debugNetId = 0;
        if (debugNetId > 0) {
          m._netId = debugNetId;
          char bufName[32];
          sprintf(bufName, "%d", debugNetId);
          m._debugFP = fopen(bufName, "w");
        }

        //#ifndef NEW_GS_FLOW
        if (_cc_band_tracks == 0) {
          ccCnt = _extNetSDB->couplingCaps(ccCapSdb, _couplingFlag, Interface,
                                           extCompute1, &m);
        } else {
          //#else
          Rect maxRect;
          _block->getDieArea(maxRect);

          if (initTiling) {
            logger_->info(RCX, 123, "Initial Tiling {} ...",
                          getBlock()->getName().c_str());
            _use_signal_tables = 3;
            createWindowsDB(rlog, maxRect, ccBandTracks, ccFlag,
                            use_signal_table);
            return 0;
          }
          if (windowFlow)
            couplingWindowFlow(rlog, maxRect, _cc_band_tracks, _couplingFlag,
                               doExt, &m, extCompute1);
          else
            couplingFlow(rlog, maxRect, _cc_band_tracks, _couplingFlag, &m,
                         extCompute1);

          if (m._debugFP != NULL)
            fclose(m._debugFP);
        }
        //#endif
      } else {
        ccCnt = _extNetSDB->couplingCaps(ccCapSdb, CCflag, Interface,
                                         extCompute, this);
      }
      if (m._dgContextFile) {
        fclose(m._dgContextFile);
        m._dgContextFile = NULL;
      }
    }
    removeDgContextArray();
    if (_printFile) {
      fclose(_printFile);
      _printFile = NULL;
      if (setBlockPtfile)
        _block->setPtFile(NULL);
      _measureRcCnt = _shapeRcCnt = _updateTotalCcnt = -1;
    }

    // than
    // and {} caps greater than %g)", 			_totCCcnt,
    // _totSmallCCcnt, _totBigCCcnt, _coupleThreshold);

    if (rlog)
      AthResourceLog("after couplingCaps", detailRlog);

    if (_debug)
      computeXcaps(0);

    //		if (rlog)
    //			AthResourceLog ("CCcap", detailRlog);

    if (preserve_geom != 1 && !_useDbSdb) {
      if ((_extNetSDB != NULL) && (preserve_geom == 3 || preserve_geom == 0)) {
        _extNetSDB->cleanSdb();
        _extNetSDB = NULL;
      }
      if (ccCapSdb != NULL && (preserve_geom == 2 || preserve_geom == 0))
        ccCapSdb->cleanSdb();
      if (rlog)
        AthResourceLog("FreeCCgeom", detailRlog);
    }
  }

  if (ttttRemoveSdb)
    _extNetSDB = NULL;
  if (ttttRemoveGs) {
    if (rlog)
      AthResourceLog("before removeSeq", detailRlog);
    if (_geomSeq)
      delete _geomSeq;
    if (rlog)
      AthResourceLog("after removeSeq", detailRlog);
    _geomSeq = NULL;
  }
  _extracted = true;
  updatePrevControl();
  int numOfNet, numOfRSeg, numOfCapNode, numOfCCSeg;
  _block->getExtCount(numOfNet, numOfRSeg, numOfCapNode, numOfCCSeg);
  if (numOfRSeg)
    logger_->info(RCX, 45, "Extract {} nets, {} rsegs, {} caps, {} ccs",
                  numOfNet - 2, numOfRSeg, numOfCapNode, numOfCCSeg);
  else
    logger_->warn(RCX, 107, "Nothing is extracted out of {} nets!",
                  numOfNet - 2);
  if (_allNet) {
    for (net_itr = bnets.begin(); net_itr != bnets.end(); ++net_itr) {
      net = *net_itr;

      dbSigType type = net->getSigType();
      if ((type == dbSigType::POWER) || (type == dbSigType::GROUND))
        continue;
      net->setWireAltered(false);
    }
  } else {
    for (j = 0; j < inets.size(); j++) {
      net = inets[j];
      net->setMark(false);
      net->setWireAltered(false);
    }
  }
  if (rlog)
    AthResourceLog("before remove Model", detailRlog);

  if (!windowFlow) {
    // delete _currentModel;
    _modelTable->resetCnt(0);
    if (rlog)
      AthResourceLog("After remove Model", detailRlog);
  }
  if (_batchScaleExt)
    genScaledExt();

  return 1;
}
/*
void extMain::genScaledExt()
{
        if (_processCornerTable == NULL)
                return;
        uint frdbid, todbid, scid;
        uint ii= 0;
        extCorner *pc, *sc;
        for (; ii< _processCornerTable->getCnt(); ii++) {
                pc = _processCornerTable->get(ii);
                scid = pc->_scaledCornerIdx;
                if (scid == -1)
                        continue;
                frdbid = pc->_dbIndex;
                sc = _scaledCornerTable->get(scid);
                todbid = sc->_dbIndex;
                _block->copyExtDb(frdbid, todbid, _cornerCnt, sc->_resFactor,
sc->_ccFactor, sc->_gndFactor);
        }
}
*/
void extMain::genScaledExt() {
  if (_processCornerTable == NULL || _scaledCornerTable == NULL)
    return;

  uint ii = 0;
  for (; ii < _scaledCornerTable->getCnt(); ii++) {
    extCorner* sc = _scaledCornerTable->get(ii);
    extCorner* pc = sc->_extCornerPtr;
    if (pc == NULL)
      continue;

    uint frdbid = pc->_dbIndex;
    uint todbid = sc->_dbIndex;

    _block->copyExtDb(frdbid, todbid, _cornerCnt, sc->_resFactor, sc->_ccFactor,
                      sc->_gndFactor);
  }
}

double extMain::getTotalNetCap(uint netId, uint cornerNum) {
  dbNet* net = dbNet::getNet(_block, netId);

  dbSet<dbCapNode> nodeSet = net->getCapNodes();

  dbSet<dbCapNode>::iterator rc_itr;

  double cap = 0.0;
  for (rc_itr = nodeSet.begin(); rc_itr != nodeSet.end(); ++rc_itr) {
    dbCapNode* node = *rc_itr;

    cap += node->getCapacitance(cornerNum);
  }
  return cap;
}
uint extMain::openSpefFile(char* filename, uint mode) {
  uint debug = 0;
  if (filename == NULL)
    debug = 1;
  else if (strcmp(filename, "") == 0)
    debug = 1;

  if (debug > 0)
    filename = NULL;

  if (mode > 0) {
    if (!_spef->setOutSpef(filename))
      return 1;
  } else {
    if (!_spef->setInSpef(filename))
      return 1;
  }
  return 0;
}
uint extMain::writeSPEF(bool stop) {
  if (stop)
    return _spef->stopWrite();
  return 0;
}
extSpef* extMain::getSpef() { return _spef; }
uint extMain::write_spef_nets(bool flatten, bool parallel) {
  return _spef->write_spef_nets(flatten, parallel);
}

uint extMain::writeSPEF(uint netId, bool single_pi, uint debug, int corner,
                        const char* corner_name) {
  if (_block == NULL) {
    logger_->info(RCX, 474,
                  "Can't execute write_spef command. There's no block in db!");
    return 0;
  }
  if (!_spef || _spef->getBlock() != _block) {
    if (_spef)
      delete _spef;
    _spef = new extSpef(_tech, _block, logger_, this);
  }
  dbNet* net = dbNet::getNet(_block, netId);

  int n = _spef->getWriteCorner(corner, corner_name);
  if (n < -10)
    return 0;

  _spef->_db_ext_corner = n;

  _spef->set_single_pi(single_pi);
  _spef->writeNet(net, 0.0, debug);
  _spef->set_single_pi(false);

  std::vector<dbNet*> nets;
  nets.push_back(net);
  _block->destroyCCs(nets);

  return 0;
}
int extSpef::getWriteCorner(int corner, const char* names) {
  int cCnt = _block->getCornerCount();
  if ((corner >= 0) && (corner < cCnt)) {
    _active_corner_cnt = 1;
    _active_corner_number[0] = corner;
    return corner;
  }

  if ((corner >= 0) && (corner >= cCnt)) {
    logger_->warn(RCX, 135,
                  "Corner {} is out of range; There are {} corners in DB!",
                  corner, cCnt);
    return -10;
  }

  if (names == NULL || names[0] == '\0')  // all corners
  {
    _active_corner_cnt = cCnt;
    for (int kk = 0; kk < cCnt; kk++)
      _active_corner_number[kk] = kk;
    return -1;
  }

  _active_corner_cnt = 0;
  int cn = 0;
  Ath__parser parser;
  parser.mkWords(names, NULL);
  for (int ii = 0; ii < parser.getWordCnt(); ii++) {
    cn = _block->getExtCornerIndex(parser.get(ii));
    if (cn < 0) {
      logger_->info(RCX, 136, "Can't find corner name {} in the parasitics DB!",
                    parser.get(ii));
      continue;
    }
    _active_corner_number[_active_corner_cnt++] = cn;
  }
  return cn;
}

uint extMain::writeSPEF(char* filename, char* netNames, bool useIds,
                        bool noNameMap, char* nodeCoord, bool termJxy,
                        const char* excludeCells, const char* capUnit,
                        const char* resUnit, bool gzFlag, bool stopAfterMap,
                        bool wClock, bool wConn, bool wCap, bool wOnlyCCcap,
                        bool wRes, bool noCnum, bool initOnly, bool single_pi,
                        bool noBackSlash, int corner, const char* corner_name,
                        bool flatten, bool parallel) {
  if (_block == NULL) {
    logger_->info(RCX, 475,
                  "Can't execute write_spef command. There's no block in db");
    return 0;
  }
  if (!_spef || _spef->getBlock() != _block) {
    if (_spef)
      delete _spef;
    _spef = new extSpef(_tech, _block, logger_, this);
  }
  _spef->_termJxy = termJxy;
  _spef->incr_wRun();

  if (excludeCells && strcmp(excludeCells, "FULLINCRSPEF") == 0) {
    excludeCells = NULL;
    _fullIncrSpef = true;
  }
  if (excludeCells && strcmp(excludeCells, "NOFULLINCRSPEF") == 0) {
    excludeCells = NULL;
    _noFullIncrSpef = true;
  }
  _writeNameMap = noNameMap ? false : true;
  _spef->_writeNameMap = _writeNameMap;
  _spef->setUseIdsFlag(useIds);
  int cntnet, cntrseg, cntcapn, cntcc;
  _block->getExtCount(cntnet, cntrseg, cntcapn, cntcc);
  if (cntrseg == 0 || cntcapn == 0) {
    logger_->info(
        RCX, 134,
        "Can't execute write_spef command. There's no extraction data.");
    return 0;
  }
  if (_extRun == 0) {
    getPrevControl();
    getExtractedCorners();
  }
  // if (_block->getRSegs() == NULL)

  // _spef->preserveFlag(preserveCapValues);
  _spef->preserveFlag(_foreign);

  if (gzFlag)
    _spef->setGzipFlag(gzFlag);

  /*	if ( (! preserveCapValues)&& (! useIds))
                  _spef->preserveFlag(true);
  */
  _spef->setDesign((char*)_block->getName().c_str());

  uint cnt = 0;
  if (openSpefFile(filename, 1) > 0)
    logger_->info(RCX, 137, "Can't open file \"{}\" to write spef.", filename);
  else {
    _spef->set_single_pi(single_pi);
    int n = _spef->getWriteCorner(corner, corner_name);
    if (n < -1)
      return 0;
    _spef->_db_ext_corner = n;
    _spef->_independentExtCorners = _independentExtCorners;

    std::vector<dbNet*> inets;
    ((dbBlock*)_block)->findSomeNet(netNames, inets);
    cnt = _spef->writeBlock(nodeCoord, excludeCells, capUnit, resUnit,
                            stopAfterMap, inets, wClock, wConn, wCap,
                            wOnlyCCcap, wRes, noCnum, initOnly, noBackSlash,
                            flatten, parallel);
    if (initOnly)
      return cnt;
  }
  delete _spef;
  _spef = NULL;

  return cnt;
}

uint extMain::readSPEF(char* filename, char* netNames, bool force, bool useIds,
                       bool rConn, char* nodeCoord, bool rCap, bool rOnlyCCcap,
                       bool rRes, float cc_thres, float cc_gnd_factor,
                       float length_unit, bool m_map, bool noCapNumCollapse,
                       char* capNodeMapFile, bool log, int corner, double low,
                       double up, char* excludeSubWord, char* subWord,
                       char* statsFile, const char* dbCornerName,
                       const char* calibrateBaseCorner, int spefCorner,
                       int fixLoop, bool keepLoadedCorner, bool stampWire,
                       ZPtr<ISdb> netSdb, uint testParsing, bool moreToRead,
                       bool diff, bool calib, int app_print_limit) {
  if (!_spef || _spef->getBlock() != _block) {
    if (_spef)
      delete _spef;
    _spef = new extSpef(_tech, _block, logger_, this);
  }
  _spef->_moreToRead = moreToRead;
  _spef->incr_rRun();

  _spef->setUseIdsFlag(useIds, diff, calib);
  if (_extRun == 0)
    getPrevControl();
  _spef->_independentExtCorners = _independentExtCorners;
  _spef->setCornerCnt(_cornerCnt);
  if (!diff && !calib)
    _foreign = true;
  if (diff) {
    if (!_extracted) {
      logger_->warn(RCX, 4, "There is no extraction db!");
      return 0;
    }
  } else if (_extracted && !force && !keepLoadedCorner &&
             !_independentExtCorners) {
    logger_->warn(RCX, 3, "Read SPEF into extracted db!");
    // return 0;
  }

  if (openSpefFile(filename, 0) > 0)
    return 0;

  _spef->_noCapNumCollapse = noCapNumCollapse;
  _spef->_capNodeFile = NULL;
  if (capNodeMapFile && capNodeMapFile[0] != '\0') {
    _spef->_capNodeFile = fopen(capNodeMapFile, "w");
    if (_spef->_capNodeFile == NULL)
      logger_->warn(RCX, 5, "Can't open SPEF file {} to write.",
                    capNodeMapFile);
  }
  if (log)
    AthResourceLog("start readSpef", 0);
  std::vector<dbNet*> inets;

  if (_block != NULL)
    _block->findSomeNet(netNames, inets);

  uint cnt = _spef->readBlock(
      0, inets, force, rConn, nodeCoord, rCap, rOnlyCCcap, rRes, cc_thres,
      length_unit, _extracted, keepLoadedCorner, stampWire, netSdb, testParsing,
      app_print_limit, m_map, corner, low, up, excludeSubWord, subWord,
      statsFile, dbCornerName, calibrateBaseCorner, spefCorner, fixLoop,
      _rsegCoord);
  genScaledExt();

  if (_spef->_capNodeFile)
    fclose(_spef->_capNodeFile);

  if (log)
    AthResourceLog("finish readSpef", 0);

  if (diff || cnt == 0) {
    delete _spef;
    _spef = NULL;
    return 0;
  }
  int numOfNet, numOfRSeg, numOfCapNode, numOfCCSeg;
  _block->getExtCount(numOfNet, numOfRSeg, numOfCapNode, numOfCCSeg);
  logger_->info(RCX, 376, "DB created {} nets, {} rsegs, {} caps, {} ccs",
                numOfNet - 2, numOfRSeg, numOfCapNode, numOfCCSeg);
  if (_spef->getAppPrintLimit()) {
    int* appcnt = _spef->getAppCnt();
    for (int ii = 0; ii < 16; ii++)
      appcnt[ii] = 0;
    dbSet<dbCCSeg> ccSet = _block->getCCSegs();
    dbSet<dbCCSeg>::iterator cc_itr;
    for (cc_itr = ccSet.begin(); cc_itr != ccSet.end(); ++cc_itr) {
      dbCCSeg* cc = *cc_itr;
      appcnt[cc->getInfileCnt()]++;
    }
    logger_->info(RCX, 7,
                  "    cc appearance count -- 1:{} 2:{} 3:{} 4:{} 5:{} 6:{} "
                  "7:{} 8:{} 9:{} 10:{} 11:{} 12:{} 13:{} 14:{} 15:{} 16:{}",
                  appcnt[0], appcnt[1], appcnt[2], appcnt[3], appcnt[4],
                  appcnt[5], appcnt[6], appcnt[7], appcnt[8], appcnt[9],
                  appcnt[10], appcnt[11], appcnt[12], appcnt[13], appcnt[14],
                  appcnt[15]);
    _spef->printAppearance(appcnt, 16);
  }
  if (!moreToRead) {
    _extracted = true;
    if (cc_gnd_factor != 0.0)
      _block->groundCC(cc_gnd_factor);
    delete _spef;
    _spef = NULL;
  }
  _extRun++;
  updatePrevControl();

  return cnt;
}
uint extMain::readSPEFincr(char* filename) {
  // assume header/name_map/ports same as first file

  if (!_spef->setInSpef(filename, true))
    return 0;

  uint cnt = _spef->readBlockIncr(0);

  // writeSPEF("out.spef", true, true, true);

  return cnt;
}
uint extMain::match(char* filename, bool m_map, const char* dbCornerName,
                    int corner, int spefCorner) {
  if (!_spef || _spef->getBlock() != _block) {
    if (_spef)
      delete _spef;
    _spef = new extSpef(_tech, _block, logger_, this);
  }
  _spef->setCalibLimit(0.0, 0.0);
  readSPEF(filename, NULL /*netNames*/, false /*force*/, false /*useIds*/,
           false /*rConn*/, NULL /*N*/, false /*rCap*/, false /*rOnlyCCcap*/,
           false /*rRes*/, -1.0 /*cc_thres*/, 0.0 /*cc_gnd_factor*/,
           1.0 /*length_unit*/, m_map, false /*noCapNumCollapse*/,
           NULL /*capNodeMapFile*/, false /*log*/, corner, 0.0 /*low*/,
           0.0 /*up*/, NULL /*excludeSubWord*/, NULL /*subWord*/,
           NULL /*statsFile*/, dbCornerName, NULL /*calibrateBaseCorner*/,
           spefCorner, 0 /*fix_loop*/, false /*keepLoadedCorner*/,
           false /*stampWire*/, NULL /*netSdb*/, 0 /*testParsing*/,
           false /*moreToRead*/, true /*diff*/, true /*calibrate*/,
           0 /*app_print_limit*/);
  return 0;
}

uint extMain::calibrate(char* filename, bool m_map, float upperLimit,
                        float lowerLimit, const char* dbCornerName, int corner,
                        int spefCorner) {
  if (!_spef || _spef->getBlock() != _block) {
    if (_spef)
      delete _spef;
    _spef = new extSpef(_tech, _block, logger_, this);
  }
  _spef->setCalibLimit(upperLimit, lowerLimit);
  readSPEF(filename, NULL /*netNames*/, false /*force*/, false /*useIds*/,
           false /*rConn*/, NULL /*N*/, false /*rCap*/, false /*rOnlyCCcap*/,
           false /*rRes*/, -1.0 /*cc_thres*/, 0.0 /*cc_gnd_factor*/,
           1.0 /*length_unit*/, m_map, false /*noCapNumCollapse*/,
           NULL /*capNodeMapFile*/, false /*log*/, corner, 0.0 /*low*/,
           0.0 /*up*/, NULL /*excludeSubWord*/, NULL /*subWord*/,
           NULL /*statsFile*/, dbCornerName, NULL /*calibrateBaseCorner*/,
           spefCorner, 0 /*fix_loop*/, false /*keepLoadedCorner*/,
           false /*stampWire*/, NULL /*netSdb*/, 0 /*testParsing*/,
           false /*moreToRead*/, true /*diff*/, true /*calibrate*/,
           0 /*app_print_limit*/);
  return 0;
}

void extMain::setUniqueExttreeCorner() {
  getPrevControl();
  _independentExtCorners = true;
  updatePrevControl();
  _block->setCornersPerBlock(1);
}

}  // namespace rcx
