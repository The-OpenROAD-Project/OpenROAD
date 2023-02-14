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

#include <limits>
#include <map>
#include <vector>

#include "rcx/extRCap.h"
#include "rcx/extSpef.h"
#include "utl/Logger.h"
#include "wire.h"

namespace rcx {

#ifdef DEBUG_NET_ID
FILE* fp;
#endif

using utl::RCX;
using namespace odb;

void extMain::print_RC(dbRSeg* rc)
{
  dbShape s;
  dbWire* w = rc->getNet()->getWire();
  w->getShape(rc->getShapeId(), s);
  print_shape(s, rc->getSourceNode(), rc->getTargetNode());
}

uint extMain::print_shape(dbShape& shape, uint j1, uint j2)
{
  uint dx = shape.xMax() - shape.xMin();
  uint dy = shape.yMax() - shape.yMin();
  if (shape.isVia()) {
    dbTechVia* tech_via = shape.getTechVia();
    std::string vname = tech_via->getName();

    logger_->info(RCX,
                  438,
                  "VIA {} ( {} {} )  jids= ( {} {} )",
                  vname.c_str(),
                  shape.xMin() + dx / 2,
                  shape.yMin() + dy / 2,
                  j1,
                  j2);
  } else {
    dbTechLayer* layer = shape.getTechLayer();
    std::string lname = layer->getName();
    logger_->info(RCX,
                  437,
                  "RECT {} ( {} {} ) ( {} {} )  jids= ( {} {} )",
                  lname.c_str(),
                  shape.xMin(),
                  shape.yMin(),
                  shape.xMax(),
                  shape.yMax(),
                  j1,
                  j2);

    if (dx < dy)
      return dy;
    else
      return dx;
  }
  return 0;
}

uint extMain::computePathDir(Point& p1, Point& p2, uint* length)
{
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

void extMain::resetSumRCtable()
{
  _sumUpdated = 0;
  _tmpSumCapTable[0] = 0.0;
  _tmpSumResTable[0] = 0.0;
  for (uint ii = 1; ii < _metRCTable.getCnt(); ii++) {
    _tmpSumCapTable[ii] = 0.0;
    _tmpSumResTable[ii] = 0.0;
  }
}

void extMain::addToSumRCtable()
{
  _sumUpdated = 1;
  for (uint ii = 0; ii < _metRCTable.getCnt(); ii++) {
    _tmpSumCapTable[ii] += _tmpCapTable[ii];
    _tmpSumResTable[ii] += _tmpResTable[ii];
  }
}

void extMain::copyToSumRCtable()
{
  for (uint ii = 0; ii < _metRCTable.getCnt(); ii++) {
    _tmpSumCapTable[ii] = _tmpCapTable[ii];
    _tmpSumResTable[ii] = _tmpResTable[ii];
  }
}

void extMain::set_adjust_colinear(bool v)
{
  _adjust_colinear = v;
}

double extMain::getViaResistance(dbTechVia* tvia)
{
  double res = 0;
  dbSet<dbBox> boxes = tvia->getBoxes();
  dbSet<dbBox>::iterator bitr;

  for (bitr = boxes.begin(); bitr != boxes.end(); ++bitr) {
    dbBox* box = *bitr;
    dbTechLayer* layer1 = box->getTechLayer();
    if (layer1->getType() == dbTechLayerType::CUT)
      res = layer1->getResistance();
  }
  return res;
}

double extMain::getViaResistance_b(dbVia* tvia, dbNet* net)
{
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
    }
  }
  double Res = tot_res;
  if (cutCnt > 1) {
    float avgCutRes = tot_res / cutCnt;
    Res = avgCutRes / cutCnt;
  }
  if (net != NULL && net->getId() == _debug_net_id) {
    debugPrint(logger_,
               RCX,
               "extrules",
               1,
               "EXT_RES:"
               "R"
               "\tgetViaResistance_b: cutCnt= {} {}  {:g} ohms",
               cutCnt,
               tvia->getConstName(),
               Res);
  }
  return Res;
}

void extMain::getViaCapacitance(dbShape svia, dbNet* net)
{
  bool USE_DB_UNITS = false;

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
    int width = std::min(dx, dy);
    int len = std::max(dx, dy);

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
      debugPrint(logger_,
                 RCX,
                 "extrules",
                 1,
                 "VIA_CAP:"
                 "C"
                 "\tgetViaCapacitance: {} {}   {} {}  M{}  W {}  LEN {} n{}",
                 x1,
                 x2,
                 y1,
                 y2,
                 level,
                 width,
                 len,
                 _debug_net_id);
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
        debugPrint(logger_,
                   RCX,
                   "extrules",
                   1,
                   "VIA_CAP:"
                   "C"
                   "\tgetViaCapacitance: M{}  W {}  LEN {} eC={:.3f} tC={:.3f} "
                   " {} n{}",
                   jj,
                   w,
                   len,
                   c1,
                   _tmpCapTable[ii],
                   ii,
                   _debug_net_id);
      }
    }
  }
}

void extMain::getShapeRC(dbNet* net,
                         dbShape& s,
                         Point& prevPoint,
                         dbWirePathShape& pshape)
{
  bool USE_DB_UNITS = false;
  double res = 0.0;
  double areaCap;
  uint len;
  uint level = 0;
  if (s.isVia()) {
    uint width = 0;
    dbTechVia* tvia = s.getTechVia();
    if (tvia != NULL) {
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
      getViaCapacitance(s, net);
      for (uint ii = 0; ii < _metRCTable.getCnt(); ii++) {
        _tmpResTable[ii] = res;
      }
    }
  } else {
    computePathDir(prevPoint, pshape.point, &len);
    level = s.getTechLayer()->getRoutingLevel();
    uint width = std::min(pshape.shape.xMax() - pshape.shape.xMin(),
                          pshape.shape.yMax() - pshape.shape.yMin());
    len = std::max(pshape.shape.xMax() - pshape.shape.xMin(),
                   pshape.shape.yMax() - pshape.shape.yMin());
    if (_adjust_colinear) {
      len -= width;
      if (len <= 0)
        len += width;
    }

    if (_lef_res) {
      double res = getResistance(level, width, len, 0);
      _tmpResTable[0] = res;
    } else {
      if (USE_DB_UNITS)
        width = GetDBcoords2(width);

      for (uint ii = 0; ii < _metRCTable.getCnt(); ii++) {
        getFringe(level, width, ii, areaCap);
        if (USE_DB_UNITS)
          len = GetDBcoords2(len);

        _tmpCapTable[ii] = 0;
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
  if ((!s.isVia()) && (_couplingFlag > 0)) {
    int x1 = s.xMin();
    int y1 = s.yMin();
    int x2 = s.xMax();
    int y2 = s.yMax();

    if (!_allNet) {
      _ccMinX = std::min(x1, _ccMinX);
      _ccMinY = std::min(y1, _ccMinY);
      _ccMaxX = std::max(x2, _ccMaxX);
      _ccMaxY = std::max(y2, _ccMaxY);
    }
  }
  prevPoint = pshape.point;
}

void extMain::setResCapFromLef(dbRSeg* rc,
                               uint targetCapId,
                               dbShape& s,
                               uint len)
{
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

    cap = _modelTable->get(0)->getTotCapOverSub(level) * len;
  }
  for (uint ii = 0; ii < _extDbCnt; ii++) {
    double xmult = 1.0 + 0.1 * ii;
    rc->setResistance(xmult * res, ii);

    rc->setCapacitance(xmult * cap, ii);
  }
}

void extMain::setResAndCap(dbRSeg* rc, double* restbl, double* captbl)
{
  int pcdbIdx, sci, scdbIdx;
  double res, cap;
  for (uint ii = 0; ii < _extDbCnt; ii++) {
    pcdbIdx = getProcessCornerDbIndex(ii);
    res = _resModify ? restbl[ii] * _resFactor : restbl[ii];
    rc->setResistance(res, pcdbIdx);
    cap = _gndcModify ? captbl[ii] * _gndcFactor : captbl[ii];
    cap = _netGndcCalibration ? cap * _netGndcCalibFactor : cap;
    getScaledCornerDbIndex(ii, sci, scdbIdx);
    if (sci == -1)
      continue;
    getScaledRC(sci, res, cap);
    rc->setResistance(res, scdbIdx);
    rc->setCapacitance(cap, scdbIdx);
  }
}

void extMain::resetMapping(dbBTerm* bterm, dbITerm* iterm, uint junction)
{
  if (bterm != NULL) {
    _btermTable->set(bterm->getId(), 0);
  } else if (iterm != NULL) {
    _itermTable->set(iterm->getId(), 0);
  }
  _nodeTable->set(junction, 0);
}

bool extMain::isTermPathEnded(dbBTerm* bterm, dbITerm* iterm)
{
  int ttttcvbs = 0;
  dbNet* net;
  if (bterm) {
    net = bterm->getNet();
    if (bterm->isSetMark()) {
      if (ttttcvbs)
        logger_->info(RCX,
                      108,
                      "Net {} multiple-ended at bterm {}",
                      net->getId(),
                      bterm->getId());
      return true;
    }
    _connectedBTerm.push_back(bterm);
    bterm->setMark(1);
  } else if (iterm) {
    net = iterm->getNet();
    if (iterm->isSetMark()) {
      if (ttttcvbs)
        logger_->info(RCX,
                      109,
                      "Net {} multiple-ended at iterm {}",
                      net->getId(),
                      iterm->getId());
      return true;
    }
    _connectedITerm.push_back(iterm);
    iterm->setMark(1);
  }
  return false;
}

uint extMain::getCapNodeId(dbNet* net,
                           dbBTerm* bterm,
                           dbITerm* iterm,
                           uint junction,
                           bool branch)
{
  if (iterm != NULL) {
    uint id = iterm->getId();
    uint capId = _itermTable->geti(id);
    if (capId > 0) {
#ifdef DEBUG_NET_ID
      if (iterm->getNet()->getId() == DEBUG_NET_ID)
        fprintf(fp, "\tOLD I_TERM %d  capNode %d\n", id, capId);
#endif

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
#ifdef DEBUG_NET_ID
      if (bterm->getNet()->getId() == DEBUG_NET_ID)
        fprintf(fp, "\tOLD B_TERM %d  capNode %d\n", id, capId);
#endif
      return capId;
    }

    dbCapNode* cap = dbCapNode::create(net, 0, _foreign);

    cap->setNode(bterm->getId());
    cap->setBTermFlag();

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
      if (branch) {
        cap->setBranchFlag();
      }
      if (cap->getNet()->getId() == _debug_net_id) {
        if (branch) {
          debugPrint(logger_,
                     RCX,
                     "rcseg",
                     1,
                     "RCSEG:C\tOLD BRANCH {}  capNode {}",
                     junction,
                     cap->getId());
        } else {
          debugPrint(logger_,
                     RCX,
                     "rcseg",
                     1,
                     "RCSEG:C\tOLD INTERNAL {}  capNode {}",
                     junction,
                     cap->getId());
        }
      }
      return capId;
    }

    dbCapNode* cap = dbCapNode::create(net, 0, _foreign);
    cap->setInternalFlag();
    cap->setNode(junction);

    if (capId == -1) {
      if (branch)
        cap->setBranchFlag();
    }
    if (cap->getNet()->getId() == _debug_net_id) {
      if (branch) {
        debugPrint(logger_,
                   RCX,
                   "rcseg",
                   1,
                   "RCSEG:"
                   "C"
                   "\tNEW BRANCH {}  capNode {}",
                   junction,
                   cap->getId());
      } else
        debugPrint(logger_,
                   RCX,
                   "rcseg",
                   1,
                   "RCSEG:"
                   "C"
                   "\tNEW INTERNAL {}  capNode {}",
                   junction,
                   cap->getId());
    }

    uint ncapId = cap->getId();
    int tcapId = capId == 0 ? ncapId : -ncapId;
    _nodeTable->set(junction, tcapId);
    return ncapId;
  }
}

uint extMain::resetMapNodes(dbNet* net)
{
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

dbRSeg* extMain::addRSeg(dbNet* net,
                         std::vector<uint>& rsegJid,
                         uint& srcId,
                         Point& prevPoint,
                         dbWirePath& path,
                         dbWirePathShape& pshape,
                         bool isBranch,
                         double* restbl,
                         double* captbl)
{
  if (!path.bterm && isTermPathEnded(pshape.bterm, pshape.iterm)) {
    rsegJid.clear();
    return NULL;
  }
  uint jidl = rsegJid.size();
  uint dstId = getCapNodeId(
      net, pshape.bterm, pshape.iterm, pshape.junction_id, isBranch);
  if (dstId == srcId) {
    char tname[200];
    tname[0] = '\0';
    if (pshape.bterm)
      sprintf(&tname[0],
              ", on bterm %d %s",
              pshape.bterm->getId(),
              (char*) pshape.bterm->getConstName());
    else if (pshape.iterm)
      sprintf(&tname[0],
              ", on iterm %d %s/%s",
              pshape.iterm->getId(),
              (char*) pshape.iterm->getInst()->getConstName(),
              (char*) pshape.iterm->getMTerm()->getConstName());
    logger_->warn(RCX,
                  111,
                  "Net {} {} has a loop at x={} y={} {}.",
                  net->getId(),
                  (char*) net->getConstName(),
                  pshape.point.getX(),
                  pshape.point.getY(),
                  &tname[0]);
    return NULL;
  }

  if (net->getId() == _debug_net_id)
    print_shape(pshape.shape, srcId, dstId);

  uint length;
  uint pathDir = computePathDir(prevPoint, pshape.point, &length);
  int jx = 0;
  int jy = 0;
  if (pshape.junction_id)
    net->getWire()->getCoord((int) pshape.junction_id, jx, jy);
  dbRSeg* rc = dbRSeg::create(net, jx, jy, pathDir, true);

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
    debugPrint(logger_,
               RCX,
               "rcseg",
               1,
               "RCSEG:"
               "R"
               "\tshapeId= {}  rseg= {}  ({} {}) {:g}",
               pshape.junction_id,
               rsid,
               srcId,
               dstId,
               rc->getCapacitance(0));

  srcId = dstId;
  prevPoint = pshape.point;
  return rc;
}

bool extMain::getFirstShape(dbNet* net, dbShape& s)
{
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

uint extMain::getShortSrcJid(uint jid)
{
  for (uint jj = 0; jj < _shortTgtJid.size(); jj++) {
    if (_shortTgtJid[jj] == jid)
      return _shortSrcJid[jj];
  }
  return jid;
}

void extMain::make1stRSeg(dbNet* net,
                          dbWirePath& path,
                          uint cnid,
                          bool skipStartWarning)
{
  int tx = 0;
  int ty = 0;
  if (path.bterm) {
    if (!path.bterm->getFirstPinLocation(tx, ty))
      logger_->error(
          RCX, 112, "Can't locate bterm {}", path.bterm->getConstName());
  } else if (path.iterm) {
    if (!path.iterm->getAvgXY(&tx, &ty))
      logger_->error(RCX,
                     113,
                     "Can't locate iterm {}/{} ( {} )",
                     path.iterm->getInst()->getConstName(),
                     path.iterm->getMTerm()->getConstName(),
                     path.iterm->getInst()->getMaster()->getConstName());
  } else if (!skipStartWarning)
    logger_->warn(RCX,
                  114,
                  "Net {} {} does not start from an iterm or a bterm.",
                  net->getId(),
                  net->getConstName());
  if (net->get1stRSegId())
    logger_->error(RCX,
                   115,
                   "Net {} {} already has rseg!",
                   net->getId(),
                   net->getConstName());
  dbRSeg* rc = dbRSeg::create(net, tx, ty, 0, true);
  rc->setTargetNode(cnid);
}

uint extMain::makeNetRCsegs(dbNet* net, bool skipStartWarning)
{
  net->setRCgraph(true);

  uint rcCnt1 = resetMapNodes(net);
  if (rcCnt1 <= 0)
    return 0;

  _netGndcCalibFactor = net->getGndcCalibFactor();
  _netGndcCalibration = _netGndcCalibFactor == 1.0 ? false : true;
  uint rcCnt = 0;
  dbWirePath path;
  dbWirePathShape pshape, ppshape;
  Point prevPoint, sprevPoint;

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
    debugPrint(logger_,
               RCX,
               "rcseg",
               1,
               "RCSEG:"
               "R "
               "makeNetRCsegs: BEGIN NET {} {}",
               netId,
               path.junction_id);
  }

  if (_mergeResBound != 0.0 || _mergeViaRes) {
    for (pitr.begin(wire); pitr.getNextPath(path);) {
      if (!path.bterm && !path.iterm && path.is_branch && path.junction_id)
        _nodeTable->set(path.junction_id, -1);

      if (path.is_short) {
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
      debugPrint(logger_,
                 RCX,
                 "rcseg",
                 1,
                 "RCSEG:"
                 "R "
                 "makeNetRCsegs:  path.junction_id {}",
                 path.junction_id);

    if (!path.iterm && !path.bterm && !path.is_branch && path.is_short)
      srcId = getCapNodeId(
          net, NULL, NULL, getShortSrcJid(path.junction_id), true);
    else
      srcId = getCapNodeId(net,
                           path.bterm,
                           path.iterm,
                           getShortSrcJid(path.junction_id),
                           path.is_branch);
    if (!netHeadMarked) {
      netHeadMarked = true;
      make1stRSeg(net, path, srcId, skipStartWarning);
    }

    prevPoint = path.point;
    sprevPoint = prevPoint;
    resetSumRCtable();
    while (pitr.getNextShape(pshape)) {
      dbShape s = pshape.shape;

      if (netId == _debug_net_id) {
        if (s.isVia()) {
          debugPrint(logger_,
                     RCX,
                     "rcseg",
                     1,
                     "RCSEG:"
                     "R "
                     "makeNetRCsegs: {} VIA",
                     pshape.junction_id);
        } else
          debugPrint(logger_,
                     RCX,
                     "rcseg",
                     1,
                     "RCSEG:"
                     "R "
                     "makeNetRCsegs: {} WIRE",
                     pshape.junction_id);
      }

      getShapeRC(net, s, sprevPoint, pshape);
      if (_mergeResBound == 0.0) {
        if (!s.isVia())
          _rsegJid.push_back(pshape.junction_id);

        addToSumRCtable();

        if (!_mergeViaRes || !s.isVia() || pshape.bterm || pshape.iterm
            || _nodeTable->geti(pshape.junction_id) < 0) {
          dbRSeg* rc = addRSeg(net,
                               _rsegJid,
                               srcId,
                               prevPoint,
                               path,
                               pshape,
                               path.is_branch,
                               _tmpSumResTable,
                               _tmpSumCapTable);
          if (s.isVia() && rc != NULL) {
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
        addRSeg(net,
                _rsegJid,
                srcId,
                prevPoint,
                path,
                pshape,
                path.is_branch,
                _tmpResTable,
                _tmpCapTable);
        rcCnt++;
        continue;
      }
      if ((_tmpResTable[0] + _tmpSumResTable[0]) >= _mergeResBound) {
        addRSeg(net,
                _rsegJid,
                srcId,
                prevPoint,
                path,
                ppshape,
                path.is_branch,
                _tmpSumResTable,
                _tmpSumCapTable);
        rcCnt++;
        if (!s.isVia())
          _rsegJid.push_back(pshape.junction_id);
        copyToSumRCtable();
      } else {
        if (!s.isVia())
          _rsegJid.push_back(pshape.junction_id);
        addToSumRCtable();
      }
      if (pshape.bterm || pshape.iterm
          || _nodeTable->geti(pshape.junction_id) < 0
          || _tmpSumResTable[0] >= _mergeResBound) {
        addRSeg(net,
                _rsegJid,
                srcId,
                prevPoint,
                path,
                pshape,
                path.is_branch,
                _tmpSumResTable,
                _tmpSumCapTable);
        rcCnt++;
        resetSumRCtable();
      } else
        ppshape = pshape;
    }
    if (_sumUpdated) {
      addRSeg(net,
              _rsegJid,
              srcId,
              prevPoint,
              path,
              ppshape,
              path.is_branch,
              _tmpSumResTable,
              _tmpSumCapTable);
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

void extMain::createShapeProperty(dbNet* net, int id, int id_val)
{
  char buff[64];
  sprintf(buff, "%d", id);
  char const* pchar = strdup(buff);
  dbIntProperty::create(net, pchar, id_val);
  sprintf(buff, "RC_%d", id_val);
  pchar = strdup(buff);
  dbIntProperty::create(net, pchar, id);
}

int extMain::getShapeProperty(dbNet* net, int id)
{
  char buff[64];
  sprintf(buff, "%d", id);
  char const* pchar = strdup(buff);
  dbIntProperty* p = dbIntProperty::find(net, pchar);
  if (p == NULL)
    return 0;
  int rcid = p->getValue();
  return rcid;
}

int extMain::getShapeProperty_rc(dbNet* net, int rc_id)
{
  char buff[64];
  sprintf(buff, "RC_%d", rc_id);
  char const* pchar = strdup(buff);
  dbIntProperty* p = dbIntProperty::find(net, pchar);
  if (p == NULL)
    return 0;
  int sid = p->getValue();
  return sid;
}

uint extMain::getExtBbox(int* x1, int* y1, int* x2, int* y2)
{
  *x1 = _x1;
  *x2 = _x2;
  *y1 = _y1;
  *y2 = _y2;

  return 0;
}

void extMain::removeExt(std::vector<dbNet*>& nets)
{
  _block->destroyParasitics(nets);
  _extracted = false;
  if (_spef)
    _spef->reinit();
}

void extCompute(CoupleOptions& inputTable, void* extModel);
void extCompute1(CoupleOptions& inputTable, void* extModel);

int extMain::setMinTypMax(bool min,
                          bool typ,
                          bool max,
                          int setMin,
                          int setTyp,
                          int setMax,
                          uint extDbCnt)
{
  _modelMap.resetCnt(0);
  _metRCTable.resetCnt(0);
  _currentModel = NULL;
  if (extDbCnt > 1) {  // extract first <extDbCnt>
    _block->setCornerCount(extDbCnt);
    _extDbCnt = extDbCnt;

    _modelMap.add(_minModelIndex);

    _modelMap.add(_typModelIndex);

    if (extDbCnt > 2) {
      _modelMap.add(_maxModelIndex);
    }
  } else if (min || max || typ) {
    if (min) {
      _modelMap.add(_minModelIndex);
    }
    if (typ) {
      _modelMap.add(_typModelIndex);
    }
    if (max) {
      _modelMap.add(_maxModelIndex);
    }
    _extDbCnt = _modelMap.getCnt();

    _block->setCornerCount(_extDbCnt);
  } else if ((setMin >= 0) || (setMax >= 0) || (setTyp >= 0)) {
    if (setMin >= 0) {
      _modelMap.add(setMin);
    }
    if (setTyp >= 0) {
      _modelMap.add(setTyp);
    }
    if (setMax >= 0) {
      _modelMap.add(setMax);
    }
    _extDbCnt = _modelMap.getCnt();

    _block->setCornerCount(_extDbCnt);
  } else if (extDbCnt == 1) {  // extract first <extDbCnt>
    _block->setCornerCount(extDbCnt);
    _extDbCnt = extDbCnt;
    _modelMap.add(0);
  }

  if (_currentModel == NULL) {
    _currentModel = getRCmodel(0);
    for (uint ii = 0; ii < _modelMap.getCnt(); ii++) {
      uint jj = _modelMap.get(ii);
      _metRCTable.add(_currentModel->getMetRCTable(jj));
    }
  }
  _cornerCnt = _extDbCnt;  // the Cnt's are the same in the old flow

  return 0;
}

extCorner::extCorner()
{
  _name = NULL;
  _model = 0;
  _dbIndex = -1;
  _scaledCornerIdx = -1;
  _resFactor = 1.0;
  _ccFactor = 1.0;
  _gndFactor = 1.0;
}

void extMain::getExtractedCorners()
{
  if (_prevControl == NULL)
    return;
  if (_prevControl->_extractedCornerList.empty())
    return;
  if (_processCornerTable != NULL)
    return;

  Ath__parser parser;
  uint pCornerCnt
      = parser.mkWords(_prevControl->_extractedCornerList.c_str(), " ");
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

  if (_prevControl->_derivedCornerList.empty()) {
    makeCornerMapFromExtControl();
    return;
  }

  uint sCornerCnt
      = parser.mkWords(_prevControl->_derivedCornerList.c_str(), " ");
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
      logger_->warn(RCX,
                    120,
                    "No matching process corner for scaled corner {}, model {}",
                    &cName[0],
                    t->_model);
    t->_dbIndex = cornerCnt++;
    _scaledCornerTable->add(t);
  }
  Ath__array1D<double> A;

  parser.mkWords(_prevControl->_resFactorList.c_str(), " ");
  parser.getDoubleArray(&A, 0);
  for (ii = 0; ii < sCornerCnt; ii++) {
    extCorner* t = _scaledCornerTable->get(ii);
    t->_resFactor = A.get(ii);
  }
  parser.mkWords(_prevControl->_ccFactorList.c_str(), " ");
  A.resetCnt();
  parser.getDoubleArray(&A, 0);
  for (ii = 0; ii < sCornerCnt; ii++) {
    extCorner* t = _scaledCornerTable->get(ii);
    t->_ccFactor = A.get(ii);
  }
  parser.mkWords(_prevControl->_gndcFactorList.c_str(), " ");
  A.resetCnt();
  parser.getDoubleArray(&A, 0);
  for (ii = 0; ii < sCornerCnt; ii++) {
    extCorner* t = _scaledCornerTable->get(ii);
    t->_gndFactor = A.get(ii);
  }
  makeCornerMapFromExtControl();
}

void extMain::makeCornerMapFromExtControl()
{
  if (_prevControl->_cornerIndexList.empty())
    return;
  if (_processCornerTable == NULL)
    return;

  Ath__parser parser;
  uint wordCnt = parser.mkWords(_prevControl->_cornerIndexList.c_str(), " ");
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

char* extMain::addRCCorner(const char* name, int model, int userDefined)
{
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
          RCX,
          433,
          "A process corner {} for Extraction RC Model {} has already been "
          "defined, skipping definition",
          s->_name,
          model);
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
    char buff[32];
    sprintf(buff, "MinMax%d", model);
    t->_name = strdup(buff);
  }
  t->_extCornerPtr = NULL;

  if (userDefined == 1)
    logger_->info(RCX,
                  431,
                  "Defined process_corner {} with ext_model_index {}",
                  t->_name,
                  model);
  else if (userDefined == 0)
    logger_->info(RCX,
                  434,
                  "Defined process_corner {} with ext_model_index {} (using "
                  "extRulesFile defaults)",
                  t->_name,
                  model);

  if (!_remote)
    makeCornerNameMap();
  return t->_name;
}

char* extMain::addRCCornerScaled(const char* name,
                                 uint model,
                                 float resFactor,
                                 float ccFactor,
                                 float gndFactor)
{
  if (_processCornerTable == NULL) {
    logger_->info(
        RCX,
        472,
        "The corresponding process corner has to be defined using the "
        "command <define_process_corner>");
    return NULL;
  }

  uint jj = 0;
  extCorner* pc = NULL;
  for (; jj < _processCornerTable->getCnt(); jj++) {
    pc = _processCornerTable->get(jj);
    if (pc->_model == (int) model)
      break;
  }
  if (jj == _processCornerTable->getCnt()) {
    logger_->info(
        RCX,
        121,
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
      logger_->info(
          RCX,
          122,
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

void extMain::cleanCornerTables()
{
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

int extMain::getProcessCornerDbIndex(int pcidx)
{
  if (_processCornerTable == NULL)
    return pcidx;
  assert(pcidx >= 0 && pcidx < (int) _processCornerTable->getCnt());
  return (_processCornerTable->get(pcidx)->_dbIndex);
}

void extMain::getScaledCornerDbIndex(int pcidx, int& scidx, int& scdbIdx)
{
  scidx = -1;
  if (_batchScaleExt || _processCornerTable == NULL)
    return;
  assert(pcidx >= 0 && pcidx < (int) _processCornerTable->getCnt());
  scidx = _processCornerTable->get(pcidx)->_scaledCornerIdx;
  if (scidx != -1)
    scdbIdx = _scaledCornerTable->get(scidx)->_dbIndex;
}

void extMain::getScaledRC(int sidx, double& res, double& cap)
{
  extCorner* sc = _scaledCornerTable->get(sidx);
  res = sc->_resFactor == 1.0 ? res : res * sc->_resFactor;
  cap = sc->_gndFactor == 1.0 ? cap : cap * sc->_gndFactor;
}

void extMain::getScaledGndC(int sidx, double& cap)
{
  extCorner* sc = _scaledCornerTable->get(sidx);
  cap = sc->_gndFactor == 1.0 ? cap : cap * sc->_gndFactor;
}

void extMain::getScaledCC(int sidx, double& cap)
{
  extCorner* sc = _scaledCornerTable->get(sidx);
  cap = sc->_ccFactor == 1.0 ? cap : cap * sc->_ccFactor;
}

void extMain::deleteCorners()
{
  cleanCornerTables();
  _block->setCornerCount(1);
}

void extMain::getCorners(std::list<std::string>& ecl)
{
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

int extMain::getDbCornerIndex(const char* name)
{
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

int extMain::getDbCornerModel(const char* name)
{
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

void extMain::makeCornerNameMap()
{
  // This function updates the dbExtControl object and
  // creates the corner information to be stored at dbBlock object

  int A[128];
  extCorner** map = new extCorner*[_cornerCnt];
  for (uint jj = 0; jj < _cornerCnt; jj++) {
    map[jj] = NULL;
    A[jj] = 0;
  }

  char cornerList[128];
  strcpy(cornerList, "");

  if (_scaledCornerTable != NULL) {
    char buf[128];
    std::string extList;
    std::string resList;
    std::string ccList;
    std::string gndcList;
    for (uint ii = 0; ii < _scaledCornerTable->getCnt(); ii++) {
      extCorner* s = _scaledCornerTable->get(ii);

      map[s->_dbIndex] = s;
      A[s->_dbIndex] = -(ii + 1);

      sprintf(buf, " %d", s->_model);
      extList += buf;
      sprintf(buf, " %g", s->_resFactor);
      resList += buf;
      sprintf(buf, " %g", s->_ccFactor);
      ccList += buf;
      sprintf(buf, " %g", s->_gndFactor);
      gndcList += buf;
    }
    _prevControl->_derivedCornerList = extList;
    _prevControl->_resFactorList = resList;
    _prevControl->_ccFactorList = ccList;
    _prevControl->_gndcFactorList = gndcList;
  }
  if (_processCornerTable != NULL) {
    std::string extList;
    char buf[128];

    for (uint ii = 0; ii < _processCornerTable->getCnt(); ii++) {
      extCorner* s = _processCornerTable->get(ii);

      A[s->_dbIndex] = ii + 1;
      map[s->_dbIndex] = s;

      sprintf(buf, " %d", s->_model);
      extList += buf;
    }
    _prevControl->_extractedCornerList = extList;
  }
  std::string aList;

  for (uint k = 0; k < _cornerCnt; k++) {
    aList += " " + std::to_string(A[k]);
  }
  _prevControl->_cornerIndexList = aList;

  std::string buff;
  if (map[0] == NULL)
    buff += " 0";
  else
    buff += map[0]->_name;

  for (uint ii = 1; ii < _cornerCnt; ii++) {
    extCorner* s = map[ii];
    if (s == NULL)
      buff += " " + std::to_string(ii);
    else
      buff += std::string(" ") + s->_name;
  }
  if (!_remote) {
    _block->setCornerCount(_cornerCnt);
    _block->setCornerNameList(buff.c_str());
  }

  delete[] map;
  updatePrevControl();
}

bool extMain::setCorners(const char* rulesFileName)
{
  _modelMap.resetCnt(0);
  uint ii;
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

    logger_->info(
        RCX, 435, "Reading extraction model file {} ...", rulesFileName);

    FILE* rules_file = fopen(rulesFileName, "r");
    if (rules_file == nullptr)
      logger_->error(
          RCX, 468, "Can't open extraction model file {}", rulesFileName);
    fclose(rules_file);

    if (!(m->readRules((char*) rulesFileName,
                       false,
                       true,
                       true,
                       true,
                       true,
                       extDbCnt,
                       cornerTable,
                       dbFactor))) {
      delete m;
      return false;
    }

    int modelCnt = getRCmodel(0)->getModelCnt();

    // If RCX reads wrong extRules file format
    if (modelCnt == 0)
      logger_->error(RCX,
                     487,
                     "No RC model read from the extraction model! "
                     "Ensure the right extRules file is used!");

    if (_processCornerTable == NULL) {
      for (int ii = 0; ii < modelCnt; ii++) {
        addRCCorner(NULL, ii, 0);
        _modelMap.add(ii);
      }
    }
  }
  _currentModel = getRCmodel(0);
  for (ii = 0; (_couplingFlag > 0) && ii < _modelMap.getCnt(); ii++) {
    uint jj = _modelMap.get(ii);
    _metRCTable.add(_currentModel->getMetRCTable(jj));
  }
  _extDbCnt = _processCornerTable->getCnt();

#ifndef NDEBUG
  uint scaleCornerCnt = 0;
  if (_scaledCornerTable != NULL)
    scaleCornerCnt = _scaledCornerTable->getCnt();
  assert(_cornerCnt == _extDbCnt + scaleCornerCnt);
#endif

  _block->setCornerCount(_cornerCnt, _extDbCnt, NULL);
  return true;
}

void extMain::addDummyCorners(uint cornerCnt)
{
  for (uint ii = 0; ii < cornerCnt; ii++)
    addRCCorner(NULL, ii, -1);
}

void extMain::updatePrevControl()
{
  _prevControl->_foreign = _foreign;
  _prevControl->_rsegCoord = _rsegCoord;
  _prevControl->_extracted = _extracted;
  _prevControl->_cornerCnt = _cornerCnt;
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
  if (_currentModel && _currentModel->getRuleFileName())
    _prevControl->_ruleFileName = _currentModel->getRuleFileName();
}

void extMain::getPrevControl()
{
  if (!_prevControl)
    return;
  _foreign = _prevControl->_foreign;
  _rsegCoord = _prevControl->_rsegCoord;
  _extracted = _prevControl->_extracted;
  _cornerCnt = _prevControl->_cornerCnt;
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
}

uint extMain::makeBlockRCsegs(const char* netNames,
                              uint cc_up,
                              uint ccFlag,
                              double resBound,
                              bool mergeViaRes,
                              double ccThres,
                              int contextDepth,
                              const char* extRules)
{
  uint debugNetId = 0;

  _diagFlow = true;

  std::vector<dbNet*> inets;
  if ((_prevControl->_ruleFileName.empty()) && (getRCmodel(0) == NULL)
      && (extRules == NULL)) {
    logger_->warn(RCX,
                  127,
                  "No RC model was read with command <load_model>, "
                  "will not perform extraction!");
    return 0;
  }

  _couplingFlag = ccFlag;
  _coupleThreshold = ccThres;

  _usingMetalPlanes = true;
  _ccUp = cc_up;
  _couplingFlag = ccFlag;
  _ccContextDepth = contextDepth;

  _mergeViaRes = mergeViaRes;
  _mergeResBound = resBound;
  if ((_processCornerTable != NULL)
      || ((_processCornerTable == NULL) && (extRules != NULL))) {
    const char* rulesfile
        = extRules ? extRules : _prevControl->_ruleFileName.c_str();
    if (!setCorners(rulesfile)) {
      logger_->info(RCX, 128, "skipping Extraction ...");
      return 0;
    }
  } else if (setMinTypMax(false, false, false, -1, -1, -1, 1) < 0) {
    logger_->warn(RCX, 129, "Wrong combination of corner related options!");
    return 0;
  }
  _foreign = false;  // extract after read_spef

  _allNet = !((dbBlock*) _block)->findSomeNet(netNames, inets);

  if (_ccContextDepth)
    initContextArray();

  initDgContextArray();
  _extRun++;

  extMeasure m;
  m.setLogger(logger_);

  _seqPool = m._seqPool;
  _useDbSdb = false;

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
  if (_couplingFlag > 1) {
    getResCapTable();
  }

  logger_->info(RCX,
                436,
                "RC segment generation {} (max_merge_res {:.1f}) ...",
                getBlock()->getName().c_str(),
                _mergeResBound);
  uint itermCntEst = 3 * bnets.size();
  setupMapping(itermCntEst);

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
      ((dbBTerm*) _connectedBTerm[tt])->setMark(0);
    for (tt = 0; tt < _connectedITerm.size(); tt++)
      ((dbITerm*) _connectedITerm[tt])->setMark(0);
  }

  logger_->info(RCX, 40, "Final {} rc segments", cnt);

  int ttttPrintDgContext = 0;
  if (_couplingFlag > 1) {
    logger_->info(RCX,
                  439,
                  "Coupling Cap extraction {} ...",
                  getBlock()->getName().c_str());

    _totCCcnt = 0;
    _totSmallCCcnt = 0;
    _totBigCCcnt = 0;
    _totSegCnt = 0;
    _totSignalSegCnt = 0;

    logger_->info(RCX,
                  440,
                  "Coupling threshhold is {:.4f} fF, coupling capacitance "
                  "less than {:.4f} "
                  "fF will be grounded.",
                  _coupleThreshold,
                  _coupleThreshold);

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

    m._ccContextArray = _ccContextArray;

    m._pixelTable = _geomSeq;
    m._minModelIndex = 0;  // couplimg threshold will be appled to this cap
    m._maxModelIndex = 0;
    m._currentModel = _currentModel;
    m._diagModel = _currentModel[0].getDiagModel();
    for (uint ii = 0; ii < _modelMap.getCnt(); ii++) {
      uint jj = _modelMap.get(ii);
      m._metRCTable.add(_currentModel->getMetRCTable(jj));
    }
    uint techLayerCnt = getExtLayerCnt(_tech) + 1;
    uint modelLayerCnt = _currentModel->getLayerCnt();
    m._layerCnt = techLayerCnt < modelLayerCnt ? techLayerCnt : modelLayerCnt;
    if (techLayerCnt == 5 && modelLayerCnt == 8)
      m._layerCnt = modelLayerCnt;
    m.getMinWidth(_tech);
    m.allocOUpool();

    m._debugFP = NULL;
    m._netId = 0;
    debugNetId = 0;
    if (debugNetId > 0) {
      m._netId = debugNetId;
      char bufName[32];
      sprintf(bufName, "%d", debugNetId);
      m._debugFP = fopen(bufName, "w");
    }

    Rect maxRect = _block->getDieArea();

    couplingFlow(maxRect, _couplingFlag, &m, extCompute1);

    if (m._debugFP != NULL)
      fclose(m._debugFP);

    if (m._dgContextFile) {
      fclose(m._dgContextFile);
      m._dgContextFile = NULL;
    }

    removeDgContextArray();
  }

  if (_geomSeq)
    delete _geomSeq;
  _geomSeq = NULL;
  _extracted = true;
  updatePrevControl();
  int numOfNet, numOfRSeg, numOfCapNode, numOfCCSeg;
  _block->getExtCount(numOfNet, numOfRSeg, numOfCapNode, numOfCCSeg);
  if (numOfRSeg)
    logger_->info(RCX,
                  45,
                  "Extract {} nets, {} rsegs, {} caps, {} ccs",
                  numOfNet - 2,
                  numOfRSeg,
                  numOfCapNode,
                  numOfCCSeg);
  else
    logger_->warn(
        RCX, 107, "Nothing is extracted out of {} nets!", numOfNet - 2);
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

  _modelTable->resetCnt(0);
  if (_batchScaleExt)
    genScaledExt();

  return 1;
}

void extMain::genScaledExt()
{
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

    _block->copyExtDb(frdbid,
                      todbid,
                      _cornerCnt,
                      sc->_resFactor,
                      sc->_ccFactor,
                      sc->_gndFactor);
  }
}

double extMain::getTotalNetCap(uint netId, uint cornerNum)
{
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

uint extMain::openSpefFile(char* filename, uint mode)
{
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

uint extMain::writeSPEF(bool stop)
{
  if (stop)
    return _spef->stopWrite();
  return 0;
}

extSpef* extMain::getSpef()
{
  return _spef;
}

uint extMain::write_spef_nets(bool flatten, bool parallel)
{
  return _spef->write_spef_nets(flatten, parallel);
}

uint extMain::writeSPEF(uint netId,
                        bool single_pi,
                        uint debug,
                        int corner,
                        const char* corner_name)
{
  if (_block == NULL) {
    logger_->info(
        RCX, 474, "Can't execute write_spef command. There's no block in db!");
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

int extSpef::getWriteCorner(int corner, const char* names)
{
  int cCnt = _block->getCornerCount();
  if ((corner >= 0) && (corner < cCnt)) {
    _active_corner_cnt = 1;
    _active_corner_number[0] = corner;
    return corner;
  }

  if ((corner >= 0) && (corner >= cCnt)) {
    logger_->warn(RCX,
                  135,
                  "Corner {} is out of range; There are {} corners in DB!",
                  corner,
                  cCnt);
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
      logger_->info(RCX,
                    136,
                    "Can't find corner name {} in the parasitics DB!",
                    parser.get(ii));
      continue;
    }
    _active_corner_number[_active_corner_cnt++] = cn;
  }
  return cn;
}

uint extMain::writeSPEF(char* filename,
                        char* netNames,
                        bool noNameMap,
                        char* nodeCoord,
                        bool termJxy,
                        const char* capUnit,
                        const char* resUnit,
                        bool gzFlag,
                        bool stopAfterMap,
                        bool wClock,
                        bool wConn,
                        bool wCap,
                        bool wOnlyCCcap,
                        bool wRes,
                        bool noCnum,
                        bool initOnly,
                        bool single_pi,
                        bool noBackSlash,
                        int corner,
                        const char* corner_name,
                        bool parallel)
{
  if (_block == NULL) {
    logger_->info(
        RCX, 475, "Can't execute write_spef command. There's no block in db");
    return 0;
  }
  if (!_spef || _spef->getBlock() != _block) {
    if (_spef)
      delete _spef;
    _spef = new extSpef(_tech, _block, logger_, this);
  }
  _spef->_termJxy = termJxy;
  _spef->incr_wRun();

  _writeNameMap = noNameMap ? false : true;
  _spef->_writeNameMap = _writeNameMap;
  _spef->setUseIdsFlag();
  int cntnet, cntrseg, cntcapn, cntcc;
  _block->getExtCount(cntnet, cntrseg, cntcapn, cntcc);
  if (cntrseg == 0 || cntcapn == 0) {
    logger_->info(
        RCX,
        134,
        "Can't execute write_spef command. There's no extraction data.");
    return 0;
  }
  if (_extRun == 0) {
    getPrevControl();
    getExtractedCorners();
  }
  _spef->preserveFlag(_foreign);

  if (gzFlag)
    _spef->setGzipFlag(gzFlag);

  _spef->setDesign((char*) _block->getName().c_str());

  uint cnt = 0;
  if (openSpefFile(filename, 1) > 0)
    logger_->info(RCX, 137, "Can't open file \"{}\" to write spef.", filename);
  else {
    _spef->set_single_pi(single_pi);
    int n = _spef->getWriteCorner(corner, corner_name);
    if (n < -1)
      return 0;
    _spef->_db_ext_corner = n;

    std::vector<dbNet*> inets;
    ((dbBlock*) _block)->findSomeNet(netNames, inets);
    cnt = _spef->writeBlock(nodeCoord,
                            capUnit,
                            resUnit,
                            stopAfterMap,
                            inets,
                            wClock,
                            wConn,
                            wCap,
                            wOnlyCCcap,
                            wRes,
                            noCnum,
                            initOnly,
                            noBackSlash,
                            parallel);
    if (initOnly)
      return cnt;
  }
  delete _spef;
  _spef = NULL;

  return cnt;
}

uint extMain::readSPEF(char* filename,
                       char* netNames,
                       bool force,
                       bool rConn,
                       char* nodeCoord,
                       bool rCap,
                       bool rOnlyCCcap,
                       bool rRes,
                       float cc_thres,
                       float cc_gnd_factor,
                       float length_unit,
                       bool m_map,
                       bool noCapNumCollapse,
                       char* capNodeMapFile,
                       bool log,
                       int corner,
                       double low,
                       double up,
                       char* excludeSubWord,
                       char* subWord,
                       char* statsFile,
                       const char* dbCornerName,
                       const char* calibrateBaseCorner,
                       int spefCorner,
                       int fixLoop,
                       bool keepLoadedCorner,
                       bool stampWire,
                       uint testParsing,
                       bool moreToRead,
                       bool diff,
                       bool calib,
                       int app_print_limit)
{
  if (!_spef || _spef->getBlock() != _block) {
    if (_spef)
      delete _spef;
    _spef = new extSpef(_tech, _block, logger_, this);
  }
  _spef->_moreToRead = moreToRead;
  _spef->incr_rRun();

  _spef->setUseIdsFlag(diff, calib);
  if (_extRun == 0)
    getPrevControl();
  _spef->setCornerCnt(_cornerCnt);
  if (!diff && !calib)
    _foreign = true;
  if (diff) {
    if (!_extracted) {
      logger_->warn(RCX, 4, "There is no extraction db!");
      return 0;
    }
  } else if (_extracted && !force && !keepLoadedCorner) {
    logger_->warn(RCX, 3, "Read SPEF into extracted db!");
  }

  if (openSpefFile(filename, 0) > 0)
    return 0;

  _spef->_noCapNumCollapse = noCapNumCollapse;
  _spef->_capNodeFile = NULL;
  if (capNodeMapFile && capNodeMapFile[0] != '\0') {
    _spef->_capNodeFile = fopen(capNodeMapFile, "w");
    if (_spef->_capNodeFile == NULL)
      logger_->warn(
          RCX, 5, "Can't open SPEF file {} to write.", capNodeMapFile);
  }
  if (log)
    AthResourceLog("start readSpef", 0);
  std::vector<dbNet*> inets;

  if (_block != NULL)
    _block->findSomeNet(netNames, inets);

  uint cnt = _spef->readBlock(0,
                              inets,
                              force,
                              rConn,
                              nodeCoord,
                              rCap,
                              rOnlyCCcap,
                              rRes,
                              cc_thres,
                              length_unit,
                              _extracted,
                              keepLoadedCorner,
                              stampWire,
                              testParsing,
                              app_print_limit,
                              m_map,
                              corner,
                              low,
                              up,
                              excludeSubWord,
                              subWord,
                              statsFile,
                              dbCornerName,
                              calibrateBaseCorner,
                              spefCorner,
                              fixLoop,
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
  logger_->info(RCX,
                376,
                "DB created {} nets, {} rsegs, {} caps, {} ccs",
                numOfNet - 2,
                numOfRSeg,
                numOfCapNode,
                numOfCCSeg);
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
    logger_->info(RCX,
                  480,
                  "    cc appearance count -- 1:{} 2:{} 3:{} 4:{} 5:{} 6:{} "
                  "7:{} 8:{} 9:{} 10:{} 11:{} 12:{} 13:{} 14:{} 15:{} 16:{}",
                  appcnt[0],
                  appcnt[1],
                  appcnt[2],
                  appcnt[3],
                  appcnt[4],
                  appcnt[5],
                  appcnt[6],
                  appcnt[7],
                  appcnt[8],
                  appcnt[9],
                  appcnt[10],
                  appcnt[11],
                  appcnt[12],
                  appcnt[13],
                  appcnt[14],
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

uint extMain::readSPEFincr(char* filename)
{
  // assume header/name_map/ports same as first file

  if (!_spef->setInSpef(filename, true))
    return 0;

  uint cnt = _spef->readBlockIncr(0);

  return cnt;
}

uint extMain::calibrate(char* filename,
                        bool m_map,
                        float upperLimit,
                        float lowerLimit,
                        const char* dbCornerName,
                        int corner,
                        int spefCorner)
{
  if (!_spef || _spef->getBlock() != _block) {
    if (_spef)
      delete _spef;
    _spef = new extSpef(_tech, _block, logger_, this);
  }
  _spef->setCalibLimit(upperLimit, lowerLimit);
  readSPEF(filename,
           NULL /*netNames*/,
           false /*force*/,
           false /*rConn*/,
           NULL /*N*/,
           false /*rCap*/,
           false /*rOnlyCCcap*/,
           false /*rRes*/,
           -1.0 /*cc_thres*/,
           0.0 /*cc_gnd_factor*/,
           1.0 /*length_unit*/,
           m_map,
           false /*noCapNumCollapse*/,
           NULL /*capNodeMapFile*/,
           false /*log*/,
           corner,
           0.0 /*low*/,
           0.0 /*up*/,
           NULL /*excludeSubWord*/,
           NULL /*subWord*/,
           NULL /*statsFile*/,
           dbCornerName,
           NULL /*calibrateBaseCorner*/,
           spefCorner,
           0 /*fix_loop*/,
           false /*keepLoadedCorner*/,
           false /*stampWire*/,
           0 /*testParsing*/,
           false /*moreToRead*/,
           true /*diff*/,
           true /*calibrate*/,
           0 /*app_print_limit*/);
  return 0;
}

}  // namespace rcx
