// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include <string.h>  // NOLINT(modernize-deprecated-headers): for strdup()

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <list>
#include <string>
#include <vector>

#include "find_some_net.h"
#include "odb/db.h"
#include "odb/dbSet.h"
#include "odb/dbShape.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "parse.h"
#include "rcx/array1.h"
#include "rcx/extRCap.h"
#include "rcx/extSpef.h"
#include "rcx/extViaModel.h"
#include "rcx/grids.h"
#include "utl/Logger.h"

namespace rcx {

#ifdef DEBUG_NET_ID
FILE* fp;
#endif

using odb::dbBox;
using odb::dbBTerm;
using odb::dbCapNode;
using odb::dbCCSeg;
using odb::dbIntProperty;
using odb::dbITerm;
using odb::dbNet;
using odb::dbRSeg;
using odb::dbSet;
using odb::dbShape;
using odb::dbTechLayer;
using odb::dbTechLayerType;
using odb::dbTechVia;
using odb::dbVia;
using odb::dbWire;
using odb::dbWirePath;
using odb::dbWirePathItr;
using odb::dbWirePathShape;
using odb::MAX_INT;
using odb::Point;
using odb::Rect;
using utl::RCX;

void extMain::print_RC(dbRSeg* rc)
{
  dbShape s;
  dbWire* w = rc->getNet()->getWire();
  w->getShape(rc->getShapeId(), s);
  print_shape(s, rc->getSourceNode(), rc->getTargetNode());
}

uint32_t extMain::print_shape(const dbShape& shape,
                              const uint32_t j1,
                              const uint32_t j2)
{
  const uint32_t dx = shape.xMax() - shape.xMin();
  const uint32_t dy = shape.yMax() - shape.yMin();
  if (shape.isVia()) {
    dbTechVia* tech_via = shape.getTechVia();
    const std::string vname = tech_via->getName();

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
    const std::string lname = layer->getName();
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

    return std::max(dx, dy);
  }
  return 0;
}

uint32_t extMain::computePathDir(const Point& p1,
                                 const Point& p2,
                                 uint32_t* length)
{
  int len;
  if (p2.getX() == p1.getX()) {
    len = p2.getY() - p1.getY();
  } else {
    len = p2.getX() - p1.getX();
  }

  if (len > 0) {
    *length = len;
    return 0;
  }
  *length = -len;
  return 1;
}

void extMain::resetSumRCtable()
{
  _sumUpdated = 0;
  _tmpSumCapTable[0] = 0.0;
  _tmpSumResTable[0] = 0.0;
  for (uint32_t ii = 1; ii < _metRCTable.getCnt(); ii++) {
    _tmpSumCapTable[ii] = 0.0;
    _tmpSumResTable[ii] = 0.0;
  }
}

void extMain::addToSumRCtable()
{
  _sumUpdated = 1;
  uint32_t ii = 0;
  _tmpSumCapTable[ii] += _tmpCapTable[ii];  // _lefRC option
  _tmpSumResTable[ii] += _tmpResTable[ii];  // _lefRC option
  for (uint32_t ii = 1; ii < _metRCTable.getCnt(); ii++) {
    _tmpSumCapTable[ii] += _tmpCapTable[ii];
    _tmpSumResTable[ii] += _tmpResTable[ii];
  }
}

void extMain::copyToSumRCtable()
{
  uint32_t ii = 0;
  _tmpSumCapTable[ii] = _tmpCapTable[ii];  // _lefRC option
  _tmpSumResTable[ii] = _tmpResTable[ii];  // _lefRC option
  for (uint32_t ii = 0; ii < _metRCTable.getCnt(); ii++) {
    _tmpSumCapTable[ii] = _tmpCapTable[ii];
    _tmpSumResTable[ii] = _tmpResTable[ii];
  }
}

double extMain::getViaResistance(dbTechVia* tvia)
{
  for (dbBox* box : tvia->getBoxes()) {
    dbTechLayer* layer = box->getTechLayer();
    if (layer->getType() == dbTechLayerType::CUT) {
      return layer->getResistance();
    }
  }
  return 0;
}

double extMain::getViaResistance_b(dbVia* tvia, dbNet* net)
{
  double tot_res = 0;
  dbSet<dbBox> boxes = tvia->getBoxes();
  dbSet<dbBox>::iterator bitr;

  uint32_t cutCnt = 0;
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
  if (net != nullptr && net->getId() == _debug_net_id) {
    debugPrint(logger_,
               RCX,
               "extrules",
               1,
               "EXT_RES:R getViaResistance_b: cutCnt= {} {}  {:g} ohms",
               cutCnt,
               tvia->getConstName(),
               Res);
  }
  return Res;
}

void extMain::getViaCapacitance(dbShape svia, dbNet* net)
{
  std::vector<dbShape> shapes;
  dbShape::getViaBoxes(svia, shapes);

  int Width[32];
  int Len[32];
  int Level[32];
  for (uint32_t jj = 1; jj < 32; jj++) {
    Level[jj] = 0;
    Width[jj] = 0;
    Len[jj] = 0;
  }

  std::vector<dbShape>::iterator shape_itr;
  for (shape_itr = shapes.begin(); shape_itr != shapes.end(); ++shape_itr) {
    dbShape s = *shape_itr;

    if (s.getTechLayer()->getType() == dbTechLayerType::CUT) {
      continue;
    }

    int x1 = s.xMin();
    int y1 = s.yMin();
    int x2 = s.xMax();
    int y2 = s.yMax();
    int dx = x2 - x1;
    int dy = y2 - y1;
    int width = std::min(dx, dy);
    int len = std::max(dx, dy);

    uint32_t level = s.getTechLayer()->getRoutingLevel();
    if (Len[level] < len) {
      Len[level] = len;
      Width[level] = width;
      Level[level] = level;
    }

    if (net->getId() == _debug_net_id) {
      debugPrint(
          logger_,
          RCX,
          "extrules",
          1,
          "VIA_CAP: C getViaCapacitance: {} {}   {} {}  M{}  W {}  LEN {} n{}",
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
  for (uint32_t jj = 1; jj < 32; jj++) {
    if (Level[jj] == 0) {
      continue;
    }

    int w = Width[jj];
    int len = Len[jj];

    for (uint32_t ii = 0; ii < _metRCTable.getCnt(); ii++) {
      double areaCap;
      double c1 = getFringe(jj, w, ii, areaCap);

      _tmpCapTable[ii] = len * 2 * c1;
      if (net->getId() == _debug_net_id) {
        debugPrint(logger_,
                   RCX,
                   "extrules",
                   1,
                   "VIA_CAP: C getViaCapacitance: M{}  W {}  LEN {} eC={:.3f} "
                   "tC={:.3f}  {} n{}",
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
                         const dbShape& s,
                         Point& prevPoint,
                         const dbWirePathShape& pshape)
{
  double viaResTable[1000];
  bool viaModelFound = false;
  double areaCap;

  if (s.isVia()) {
    double res = 0.0;
    uint32_t level = 0;
    if (dbTechVia* tvia = s.getTechVia()) {
      const char* viaName = tvia->getConstName();
      // find res from ViaModelTable
      for (uint32_t ii = 0; ii < _metRCTable.getCnt(); ii++) {
        extMetRCTable* rcTable = _metRCTable.get(ii);
        extViaModel* viaModel = rcTable->getViaModel((char*) viaName);
        if (viaModel != nullptr) {
          viaResTable[ii] = viaModel->_res;
          viaModelFound = true;
        }
      }
      level = tvia->getBottomLayer()->getRoutingLevel();
      res = tvia->getResistance();
      if (res == 0) {
        res = getViaResistance(tvia);
      }
      if (res > 0) {
        tvia->setResistance(res);
      }
      if (res <= 0.0) {
        const uint32_t width = tvia->getBottomLayer()->getWidth();
        res = getResistance(level, width, width, 0);
      }
    } else {
      dbVia* bvia = s.getVia();
      if (bvia != nullptr) {
        level = bvia->getBottomLayer()->getRoutingLevel();
        res = getViaResistance_b(bvia, net);

        if (res <= 0.0) {
          const uint32_t width = bvia->getBottomLayer()->getWidth();
          res = getResistance(level, width, width, 0);
        }
      }
    }
    if (level > 0) {
      if (_lefRC) {
        dbVia* bvia = s.getVia();
        uint32_t width = bvia->getBottomLayer()->getWidth();
        double areaCap;
        _tmpCapTable[0] = width * 2 * getFringe(level, width, 0, areaCap);
        _tmpCapTable[0] += 2 * areaCap * width * width;
        _tmpResTable[0] = res;
      } else {
        getViaCapacitance(s, net);
        for (uint32_t ii = 0; ii < _metRCTable.getCnt(); ii++) {
          if (viaModelFound) {
            _tmpResTable[ii] = viaResTable[ii];
          } else {
            _tmpResTable[ii] = res;
          }
        }
      }
    }
  } else {
    uint32_t len;
    computePathDir(prevPoint, pshape.point, &len);

    const uint32_t level = s.getTechLayer()->getRoutingLevel();
    uint32_t width = std::min(pshape.shape.getDX(), pshape.shape.getDY());
    len = std::max(pshape.shape.getDX(), pshape.shape.getDY());
    //    const auto [width, len]= std::minmax({pshape.shape.getDX(),
    //    pshape.shape.getDY()});
    if (_lefRC) {
      double res = getResistance(level, width, len, 0);
      double unitCap = getFringe(level, width, 0, areaCap);
      double frTot = len * 2 * unitCap;

      _tmpCapTable[0] = frTot;
      _tmpCapTable[0] += 2 * areaCap * len * width;
      _tmpResTable[0] = res;

    } else if (_lef_res) {
      const double res = getResistance(level, width, len, 0);
      _tmpResTable[0] = res;
    } else {
      for (uint32_t ii = 0; ii < _metRCTable.getCnt(); ii++) {
        double areaCap;
        getFringe(level, width, ii, areaCap);

        _tmpCapTable[ii] = 0;
        getResistance(level, width, len, ii);
        bool newResModel = true;
        if (!newResModel) {
          double r = getResistance(level, width, len, ii);
          _tmpResTable[ii] = r;
          //  _tmpResTable[ii] = 0;
        } else {
          double r = getResistance(level, width, len, ii);
          _tmpResTable[ii] = r;
        }
      }
    }
    if (!_allNet && _couplingFlag > 0) {
      _ccMinX = std::min(s.xMin(), _ccMinX);
      _ccMinY = std::min(s.yMin(), _ccMinY);
      _ccMaxX = std::max(s.xMax(), _ccMaxX);
      _ccMaxY = std::max(s.yMax(), _ccMaxY);
    }
  }
  prevPoint = pshape.point;
}

void extMain::setResCapFromLef(dbRSeg* rc,
                               uint32_t targetCapId,
                               dbShape& s,
                               uint32_t len)
{
  double cap = 0.0;
  double res = 0.0;

  if (s.isVia()) {
    dbTechVia* via = s.getTechVia();

    res = via->getResistance();

    if (_couplingFlag == 0) {
      uint32_t n = via->getBottomLayer()->getRoutingLevel();
      len = 2 * via->getBottomLayer()->getWidth();
      cap = _modelTable->get(0)->getTotCapOverSub(n) * len;
    }
  } else {
    uint32_t level = s.getTechLayer()->getRoutingLevel();
    res = _modelTable->get(0)->getRes(level) * len;

    cap = _modelTable->get(0)->getTotCapOverSub(level) * len;
  }
  for (uint32_t ii = 0; ii < _extDbCnt; ii++) {
    double xmult = 1.0 + 0.1 * ii;
    rc->setResistance(xmult * res, ii);

    rc->setCapacitance(xmult * cap, ii);
  }
}

void extMain::setResAndCap(dbRSeg* rc,
                           const double* restbl,
                           const double* captbl)
{
  if (_lefRC) {
    double res, cap;
    res = _resModify ? restbl[0] * _resFactor : restbl[0];
    rc->setResistance(res, 0);
    cap = _gndcModify ? captbl[0] * _gndcFactor : captbl[0];
    cap = _netGndcCalibration ? cap * _netGndcCalibFactor : cap;
    rc->setCapacitance(cap, 0);
    return;
  }
  for (uint32_t ii = 0; ii < _extDbCnt; ii++) {
    const int pcdbIdx = getProcessCornerDbIndex(ii);
    double res = _resModify ? restbl[ii] * _resFactor : restbl[ii];
    rc->setResistance(res, pcdbIdx);
    double cap = _gndcModify ? captbl[ii] * _gndcFactor : captbl[ii];
    cap = _netGndcCalibration ? cap * _netGndcCalibFactor : cap;
    int sci, scdbIdx;
    getScaledCornerDbIndex(ii, sci, scdbIdx);
    if (sci == -1) {
      continue;
    }
    getScaledRC(sci, res, cap);
    rc->setResistance(res, scdbIdx);
    rc->setCapacitance(cap, scdbIdx);
  }
}

void extMain::resetMapping(dbBTerm* bterm, dbITerm* iterm, uint32_t junction)
{
  if (bterm != nullptr) {
    _btermTable->set(bterm->getId(), 0);
  } else if (iterm != nullptr) {
    _itermTable->set(iterm->getId(), 0);
  }
  _nodeTable->set(junction, 0);
}

bool extMain::isTermPathEnded(dbBTerm* bterm, dbITerm* iterm)
{
  if (bterm) {
    if (bterm->isSetMark()) {
      return true;
    }
    _connectedBTerm.push_back(bterm);
    bterm->setMark(1);
  } else if (iterm) {
    if (iterm->isSetMark()) {
      return true;
    }
    _connectedITerm.push_back(iterm);
    iterm->setMark(1);
  }
  return false;
}

uint32_t extMain::getCapNodeId(dbNet* net,
                               dbBTerm* bterm,
                               dbITerm* iterm,
                               const uint32_t junction,
                               const bool branch)
{
  if (iterm != nullptr) {
    const uint32_t id = iterm->getId();
    uint32_t capId = _itermTable->geti(id);
    if (capId > 0) {
#ifdef DEBUG_NET_ID
      if (iterm->getNet()->getId() == DEBUG_NET_ID) {
        fprintf(fp, "\tOLD I_TERM %d  capNode %d\n", id, capId);
      }
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
    if (iterm->getNet()->getId() == DEBUG_NET_ID) {
      fprintf(fp, "\tNEW I_TERM %d capNode %d\n", id, capId);
    }
#endif
    return capId;
  }
  if (bterm != nullptr) {
    uint32_t id = bterm->getId();
    uint32_t capId = _btermTable->geti(id);
    if (capId > 0) {
#ifdef DEBUG_NET_ID
      if (bterm->getNet()->getId() == DEBUG_NET_ID) {
        fprintf(fp, "\tOLD B_TERM %d  capNode %d\n", id, capId);
      }
#endif
      return capId;
    }

    dbCapNode* cap = dbCapNode::create(net, 0, _foreign);

    cap->setNode(bterm->getId());
    cap->setBTermFlag();

    capId = cap->getId();

    _btermTable->set(id, capId);
    const int tcapId = _nodeTable->geti(junction) == -1 ? -capId : capId;
    _nodeTable->set(junction, tcapId);  // allow get capId using junction

#ifdef DEBUG_NET_ID
    if (bterm->getNet()->getId() == DEBUG_NET_ID) {
      fprintf(fp, "\tNEW B_TERM %d  capNode %d\n", id, capId);
    }
#endif
    return capId;
  }

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
    if (branch) {
      cap->setBranchFlag();
    }
  }
  if (cap->getNet()->getId() == _debug_net_id) {
    if (branch) {
      debugPrint(logger_,
                 RCX,
                 "rcseg",
                 1,
                 "RCSEG:C NEW BRANCH {}  capNode {}",
                 junction,
                 cap->getId());
    } else {
      debugPrint(logger_,
                 RCX,
                 "rcseg",
                 1,
                 "RCSEG:C NEW INTERNAL {}  capNode {}",
                 junction,
                 cap->getId());
    }
  }

  uint32_t ncapId = cap->getId();
  int tcapId = capId == 0 ? ncapId : -ncapId;
  _nodeTable->set(junction, tcapId);
  return ncapId;
}

uint32_t extMain::resetMapNodes(dbNet* net)
{
  dbWire* wire = net->getWire();
  if (wire == nullptr) {
    if (_reportNetNoWire) {
      logger_->info(RCX, 110, "Net {} has no wires.", net->getName().c_str());
    }
    _netNoWireCnt++;
    return 0;
  }
  uint32_t cnt = 0;

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
                         std::vector<uint32_t>& rsegJid,
                         uint32_t& srcId,
                         Point& prevPoint,
                         const dbWirePath& path,
                         const dbWirePathShape& pshape,
                         const bool isBranch,
                         const double* restbl,
                         const double* captbl)
{
  if (!path.bterm && isTermPathEnded(pshape.bterm, pshape.iterm)) {
    rsegJid.clear();
    return nullptr;
  }
  const uint32_t dstId = getCapNodeId(
      net, pshape.bterm, pshape.iterm, pshape.junction_id, isBranch);
  if (dstId == srcId) {
    std::string tname;
    if (pshape.bterm) {
      tname += fmt::format(", on bterm {}", pshape.bterm->getConstName());
    } else if (pshape.iterm) {
      tname += fmt::format(", on iterm {}/{}",
                           pshape.iterm->getInst()->getConstName(),
                           pshape.iterm->getMTerm()->getConstName());
    }
    logger_->warn(RCX,
                  111,
                  "Net {} {} has a loop at x={} y={} {}.",
                  net->getId(),
                  net->getConstName(),
                  pshape.point.getX(),
                  pshape.point.getY(),
                  tname);
    return nullptr;
  }

  if (net->getId() == _debug_net_id) {
    print_shape(pshape.shape, srcId, dstId);
  }

  uint32_t length;
  const uint32_t pathDir = computePathDir(prevPoint, pshape.point, &length);

  Point pt;
  if (pshape.junction_id) {
    pt = net->getWire()->getCoord(pshape.junction_id);
  }
  dbRSeg* rc = dbRSeg::create(net, pt.x(), pt.y(), pathDir, true);

  const uint32_t jidl = rsegJid.size();
  const uint32_t rsid = rc->getId();
  for (uint32_t jj = 0; jj < jidl; jj++) {
    net->getWire()->setProperty(rsegJid[jj], rsid);
  }
  rsegJid.clear();

  rc->setSourceNode(srcId);
  rc->setTargetNode(dstId);

  if (srcId > 0) {
    dbCapNode::getCapNode(_block, srcId)->incrChildrenCnt();
  }
  if (dstId > 0) {
    dbCapNode::getCapNode(_block, dstId)->incrChildrenCnt();
  }

  setResAndCap(rc, restbl, captbl);

  if (net->getId() == _debug_net_id) {
    debugPrint(logger_,
               RCX,
               "rcseg",
               1,
               "RCSEG:R shapeId= {}  rseg= {}  ({} {}) {:g}",
               pshape.junction_id,
               rsid,
               srcId,
               dstId,
               rc->getCapacitance(0));
  }

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

uint32_t extMain::getShortSrcJid(uint32_t jid)
{
  for (uint32_t jj = 0; jj < _shortTgtJid.size(); jj++) {
    if (_shortTgtJid[jj] == jid) {
      return _shortSrcJid[jj];
    }
  }
  return jid;
}

void extMain::make1stRSeg(dbNet* net,
                          dbWirePath& path,
                          uint32_t cnid,
                          bool skipStartWarning)
{
  int tx = 0;
  int ty = 0;
  if (path.bterm) {
    if (!path.bterm->getFirstPinLocation(tx, ty)) {
      logger_->error(
          RCX, 112, "Can't locate bterm {}", path.bterm->getConstName());
    }
  } else if (path.iterm) {
    if (!path.iterm->getAvgXY(&tx, &ty)) {
      logger_->error(RCX,
                     113,
                     "Can't locate iterm {}/{} ( {} )",
                     path.iterm->getInst()->getConstName(),
                     path.iterm->getMTerm()->getConstName(),
                     path.iterm->getInst()->getMaster()->getConstName());
    }
  } else if (!skipStartWarning) {
    logger_->warn(RCX,
                  114,
                  "Net {} {} does not start from an iterm or a bterm.",
                  net->getId(),
                  net->getConstName());
  }
  if (net->get1stRSegId()) {
    logger_->error(RCX,
                   115,
                   "Net {} {} already has rseg!",
                   net->getId(),
                   net->getConstName());
  }
  dbRSeg* rc = dbRSeg::create(net, tx, ty, 0, true);
  rc->setTargetNode(cnid);
}

uint32_t extMain::makeNetRCsegs(dbNet* net, bool skipStartWarning)
{
  net->setRCgraph(true);

  const uint32_t rcCnt1 = resetMapNodes(net);
  if (rcCnt1 <= 0) {
    return 0;
  }

  _netGndcCalibFactor = net->getGndcCalibFactor();
  _netGndcCalibration = _netGndcCalibFactor == 1.0 ? false : true;

  _rsegJid.clear();
  _shortSrcJid.clear();
  _shortTgtJid.clear();

  uint32_t netId = net->getId();
#ifdef DEBUG_NET_ID
  if (netId == DEBUG_NET_ID) {
    fp = fopen("rsegs", "w");
    fprintf(fp, "BEGIN NET %d %d\n", netId, path.junction_id);
  }
#endif
  if (netId == _debug_net_id) {
    debugPrint(
        logger_, RCX, "rcseg", 1, "RCSEG:R makeNetRCsegs: BEGIN NET {}", netId);
  }

  uint32_t srcJid;
  dbWire* wire = net->getWire();
  dbWirePathItr pitr;
  if (_mergeResBound != 0.0 || _mergeViaRes) {
    dbWirePath path;
    for (pitr.begin(wire); pitr.getNextPath(path);) {
      if (!path.bterm && !path.iterm && path.is_branch && path.junction_id) {
        _nodeTable->set(path.junction_id, -1);
      }

      if (path.is_short) {
        _nodeTable->set(path.short_junction, -1);
        srcJid = path.short_junction;
        for (uint32_t tt = 0; tt < _shortTgtJid.size(); tt++) {
          if (_shortTgtJid[tt] == srcJid) {
            srcJid = _shortSrcJid[tt];  // forward short
            break;
          }
        }
        _shortSrcJid.push_back(srcJid);
        _shortTgtJid.push_back(path.junction_id);
      }
      dbWirePathShape pshape;
      while (pitr.getNextShape(pshape)) {
        ;
      }
    }
  }
  uint32_t srcId;
  dbWirePathShape ppshape;
  uint32_t rcCnt = 0;
  bool netHeadMarked = false;
  dbWirePath path;
  for (pitr.begin(wire); pitr.getNextPath(path);) {
    if (netId == _debug_net_id) {
      debugPrint(logger_,
                 RCX,
                 "rcseg",
                 1,
                 "RCSEG:R makeNetRCsegs:  path.junction_id {}",
                 path.junction_id);
    }

    if (!path.iterm && !path.bterm && !path.is_branch && path.is_short) {
      srcId = getCapNodeId(
          net, nullptr, nullptr, getShortSrcJid(path.junction_id), true);
    } else {
      srcId = getCapNodeId(net,
                           path.bterm,
                           path.iterm,
                           getShortSrcJid(path.junction_id),
                           path.is_branch);
    }
    if (!netHeadMarked) {
      netHeadMarked = true;
      make1stRSeg(net, path, srcId, skipStartWarning);
    }

    Point prevPoint = path.point;
    Point sprevPoint = prevPoint;
    resetSumRCtable();
    dbWirePathShape pshape;
    while (pitr.getNextShape(pshape)) {
      dbShape s = pshape.shape;

      if (netId == _debug_net_id) {
        debugPrint(logger_,
                   RCX,
                   "rcseg",
                   1,
                   "RCSEG:R makeNetRCsegs: {} {}",
                   pshape.junction_id,
                   s.isVia() ? "VIA" : "WIRE");
      }

      getShapeRC(net, s, sprevPoint, pshape);
      sprevPoint = pshape.point;
      if (_mergeResBound == 0.0) {
        if (!s.isVia()) {
          _rsegJid.push_back(pshape.junction_id);
        }

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
          if (s.isVia() && rc != nullptr) {
            createShapeProperty(net, pshape.junction_id, rc->getId());
          }
          resetSumRCtable();
          rcCnt++;
        } else {
          ppshape = pshape;
        }
        continue;
      }
      if (_tmpResTable[0] >= _mergeResBound && _tmpSumResTable[0] == 0.0) {
        if (!s.isVia()) {
          _rsegJid.push_back(pshape.junction_id);
        }
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
        if (!s.isVia()) {
          _rsegJid.push_back(pshape.junction_id);
        }
        copyToSumRCtable();
      } else {
        if (!s.isVia()) {
          _rsegJid.push_back(pshape.junction_id);
        }
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
      } else {
        ppshape = pshape;
      }
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
  net->getRSegs().reverse();

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
  dbIntProperty::create(net, buff, id_val);
  sprintf(buff, "RC_%d", id_val);
  dbIntProperty::create(net, buff, id);
}

int extMain::getShapeProperty(dbNet* net, int id)
{
  char buff[64];
  sprintf(buff, "%d", id);
  dbIntProperty* p = dbIntProperty::find(net, buff);
  if (p == nullptr) {
    return 0;
  }
  int rcid = p->getValue();
  return rcid;
}

int extMain::getShapeProperty_rc(dbNet* net, int rc_id)
{
  char buff[64];
  sprintf(buff, "RC_%d", rc_id);
  dbIntProperty* p = dbIntProperty::find(net, buff);
  if (p == nullptr) {
    return 0;
  }
  int sid = p->getValue();
  return sid;
}

uint32_t extMain::getExtBbox(int* x1, int* y1, int* x2, int* y2)
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
  if (_spef) {
    _spef->reinit();
  }
}

void extCompute(CoupleOptions& inputTable, void* extModel);
void extCompute1(CoupleOptions& inputTable, void* extModel);

int extMain::setMinTypMax(bool min,
                          bool typ,
                          bool max,
                          int setMin,
                          int setTyp,
                          int setMax,
                          uint32_t extDbCnt)
{
  _modelMap.resetCnt(0);
  _metRCTable.resetCnt(0);
  _currentModel = nullptr;
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

  if (_currentModel == nullptr) {
    _currentModel = getRCmodel(0);
    for (uint32_t ii = 0; ii < _modelMap.getCnt(); ii++) {
      uint32_t jj = _modelMap.get(ii);
      _metRCTable.add(_currentModel->getMetRCTable(jj));
    }
  }
  _cornerCnt = _extDbCnt;  // the Cnt's are the same in the old flow

  return 0;
}

extCorner::extCorner()
{
  _name = nullptr;
  _model = 0;
  _dbIndex = -1;
  _scaledCornerIdx = -1;
  _resFactor = 1.0;
  _ccFactor = 1.0;
  _gndFactor = 1.0;
}

void extMain::getExtractedCorners()
{
  if (_prevControl == nullptr) {
    return;
  }
  if (_prevControl->_extractedCornerList.empty()) {
    return;
  }
  if (_processCornerTable != nullptr) {
    return;
  }

  Parser parser(logger_);
  uint32_t pCornerCnt
      = parser.mkWords(_prevControl->_extractedCornerList.c_str(), " ");
  if (pCornerCnt <= 0) {
    return;
  }

  _processCornerTable = new Array1D<extCorner*>();

  uint32_t cornerCnt = 0;
  uint32_t ii, jj;
  std::string cName;
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

  uint32_t sCornerCnt
      = parser.mkWords(_prevControl->_derivedCornerList.c_str(), " ");
  if (sCornerCnt <= 0) {
    return;
  }

  if (_scaledCornerTable == nullptr) {
    _scaledCornerTable = new Array1D<extCorner*>();
  }

  for (ii = 0; ii < sCornerCnt; ii++) {
    extCorner* t = new extCorner();
    t->_model = parser.getInt(ii);
    for (jj = 0; jj < pCornerCnt; jj++) {
      if (t->_model != _processCornerTable->get(jj)->_model) {
        continue;
      }
      t->_extCornerPtr = _processCornerTable->get(jj);
      break;
    }
    cName = _block->getExtCornerName(pCornerCnt + ii);
    if (jj == pCornerCnt) {
      logger_->warn(RCX,
                    120,
                    "No matching process corner for scaled corner {}, model {}",
                    cName,
                    t->_model);
    }
    t->_dbIndex = cornerCnt++;
    _scaledCornerTable->add(t);
  }
  Array1D<double> A;

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
  if (_prevControl->_cornerIndexList.empty()) {
    return;
  }
  if (_processCornerTable == nullptr) {
    return;
  }

  Parser parser(logger_);
  uint32_t wordCnt
      = parser.mkWords(_prevControl->_cornerIndexList.c_str(), " ");
  if (wordCnt <= 0) {
    return;
  }

  std::string cName;
  for (uint32_t ii = 0; ii < wordCnt; ii++) {
    int index = parser.getInt(ii);
    extCorner* t = nullptr;
    if (index > 0) {  // extracted corner
      t = _processCornerTable->get(index - 1);
      t->_dbIndex = ii;
    } else {
      t = _scaledCornerTable->get((-index) - 1);
    }
    t->_dbIndex = ii;
    cName = _block->getExtCornerName(ii);
    free(t->_name);
    t->_name = strdup(cName.c_str());
  }
}

char* extMain::addRCCorner(const char* name, int model, int userDefined)
{
  _remote = 0;
  if (model >= 100) {
    _remote = 1;
    model = model - 100;
  }
  if (_processCornerTable == nullptr) {
    _processCornerTable = new Array1D<extCorner*>();
  }

  for (uint32_t ii = 0; ii < _processCornerTable->getCnt(); ii++) {
    extCorner* s = _processCornerTable->get(ii);
    if (s->_model == model) {
      logger_->info(
          RCX,
          433,
          "A process corner {} for Extraction RC Model {} has already been "
          "defined, skipping definition",
          s->_name,
          model);
      return nullptr;
    }
  }
  extCorner* t = new extCorner();
  t->_model = model;
  t->_dbIndex = _cornerCnt++;
  _processCornerTable->add(t);
  if (name != nullptr) {
    t->_name = strdup(name);
  } else {
    char buff[32];
    sprintf(buff, "MinMax%d", model);
    t->_name = strdup(buff);
  }
  t->_extCornerPtr = nullptr;

  if (userDefined == 1) {
    logger_->info(RCX,
                  431,
                  "Defined process_corner {} with ext_model_index {}",
                  t->_name,
                  model);
  } else if (userDefined == 0) {
    logger_->info(RCX,
                  434,
                  "Defined process_corner {} with ext_model_index {} (using "
                  "extRulesFile defaults)",
                  t->_name,
                  model);
  }
  if (!_remote) {
    makeCornerNameMap();
  }
  return t->_name;
}

char* extMain::addRCCornerScaled(const char* name,
                                 uint32_t model,
                                 float resFactor,
                                 float ccFactor,
                                 float gndFactor)
{
  if (_processCornerTable == nullptr) {
    logger_->info(
        RCX,
        472,
        "The corresponding process corner has to be defined using the "
        "command <define_process_corner>");
    return nullptr;
  }

  uint32_t jj = 0;
  extCorner* pc = nullptr;
  for (; jj < _processCornerTable->getCnt(); jj++) {
    pc = _processCornerTable->get(jj);
    if (pc->_model == (int) model) {
      break;
    }
  }
  if (jj == _processCornerTable->getCnt()) {
    logger_->info(
        RCX,
        121,
        "The corresponding process corner has to be defined using the "
        "command <define_process_corner>");
    return nullptr;
  }
  if (_scaledCornerTable == nullptr) {
    _scaledCornerTable = new Array1D<extCorner*>();
  }

  uint32_t ii = 0;
  for (; ii < _scaledCornerTable->getCnt(); ii++) {
    extCorner* s = _scaledCornerTable->get(ii);

    if ((name != nullptr) && (strcmp(s->_name, name) == 0)) {
      logger_->info(
          RCX,
          122,
          "A process corner for Extraction RC Model {} has already been "
          "defined, skipping definition",
          model);
      return nullptr;
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
  if (name != nullptr) {
    t->_name = strdup(name);
  } else {
    char buff[16];
    sprintf(buff, "derived_MinMax%d", model);
    t->_name = strdup(buff);
  }
  makeCornerNameMap();
  return t->_name;
}

void extMain::cleanCornerTables()
{
  if (_scaledCornerTable != nullptr) {
    for (uint32_t ii = 0; ii < _scaledCornerTable->getCnt(); ii++) {
      extCorner* s = _scaledCornerTable->get(ii);

      free(s->_name);
      delete s;
    }
    delete _scaledCornerTable;
  }
  _scaledCornerTable = nullptr;
  if (_processCornerTable != nullptr) {
    for (uint32_t ii = 0; ii < _processCornerTable->getCnt(); ii++) {
      extCorner* s = _processCornerTable->get(ii);

      free(s->_name);
      delete s;
    }
    delete _processCornerTable;
  }
  _processCornerTable = nullptr;
}

int extMain::getProcessCornerDbIndex(int pcidx)
{
  if (_processCornerTable == nullptr) {
    return pcidx;
  }
  assert(pcidx >= 0 && pcidx < (int) _processCornerTable->getCnt());
  return (_processCornerTable->get(pcidx)->_dbIndex);
}

void extMain::getScaledCornerDbIndex(int pcidx, int& scidx, int& scdbIdx)
{
  scidx = -1;
  if (_batchScaleExt || _processCornerTable == nullptr) {
    return;
  }
  assert(pcidx >= 0 && pcidx < (int) _processCornerTable->getCnt());
  scidx = _processCornerTable->get(pcidx)->_scaledCornerIdx;
  if (scidx != -1) {
    scdbIdx = _scaledCornerTable->get(scidx)->_dbIndex;
  }
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
  uint32_t ii;
  for (ii = 0; _processCornerTable && ii < _processCornerTable->getCnt();
       ii++) {
    std::string s1c;
    extCorner* ec = _processCornerTable->get(ii);
    sprintf(buffer, "%s %d %d", ec->_name, ec->_model + 1, ec->_dbIndex);
    s1c += buffer;
    ecl.push_back(s1c);
  }
  for (ii = 0; _scaledCornerTable && ii < _scaledCornerTable->getCnt(); ii++) {
    std::string s1c;
    extCorner* ec = _scaledCornerTable->get(ii);
    sprintf(buffer, "%s %d %d", ec->_name, -(ec->_model + 1), ec->_dbIndex);
    s1c += buffer;
    ecl.push_back(s1c);
  }
}

int extMain::getDbCornerIndex(const char* name)
{
  if (_scaledCornerTable != nullptr) {
    for (uint32_t ii = 0; ii < _scaledCornerTable->getCnt(); ii++) {
      extCorner* s = _scaledCornerTable->get(ii);

      if (strcmp(s->_name, name) == 0) {
        return s->_dbIndex;
      }
    }
  }
  if (_processCornerTable != nullptr) {
    for (uint32_t ii = 0; ii < _processCornerTable->getCnt(); ii++) {
      extCorner* s = _processCornerTable->get(ii);

      if (strcmp(s->_name, name) == 0) {
        return s->_dbIndex;
      }
    }
  }
  return -1;
}

int extMain::getDbCornerModel(const char* name)
{
  if (_scaledCornerTable != nullptr) {
    for (uint32_t ii = 0; ii < _scaledCornerTable->getCnt(); ii++) {
      extCorner* s = _scaledCornerTable->get(ii);

      if (strcmp(s->_name, name) == 0) {
        return s->_model;
      }
    }
  }
  if (_processCornerTable != nullptr) {
    for (uint32_t ii = 0; ii < _processCornerTable->getCnt(); ii++) {
      extCorner* s = _processCornerTable->get(ii);

      if (strcmp(s->_name, name) == 0) {
        return s->_model;
      }
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
  for (uint32_t jj = 0; jj < _cornerCnt; jj++) {
    map[jj] = nullptr;
    A[jj] = 0;
  }

  char cornerList[128];
  strcpy(cornerList, "");

  if (_scaledCornerTable != nullptr) {
    char buf[128];
    std::string extList;
    std::string resList;
    std::string ccList;
    std::string gndcList;
    for (uint32_t ii = 0; ii < _scaledCornerTable->getCnt(); ii++) {
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
  if (_processCornerTable != nullptr) {
    std::string extList;
    char buf[128];

    for (uint32_t ii = 0; ii < _processCornerTable->getCnt(); ii++) {
      extCorner* s = _processCornerTable->get(ii);

      A[s->_dbIndex] = ii + 1;
      map[s->_dbIndex] = s;

      sprintf(buf, " %d", s->_model);
      extList += buf;
    }
    _prevControl->_extractedCornerList = extList;
  }
  std::string aList;

  for (uint32_t k = 0; k < _cornerCnt; k++) {
    aList += " " + std::to_string(A[k]);
  }
  _prevControl->_cornerIndexList = aList;

  std::string buff;
  if (map[0] == nullptr) {
    buff += " 0";
  } else {
    buff += map[0]->_name;
  }

  for (uint32_t ii = 1; ii < _cornerCnt; ii++) {
    extCorner* s = map[ii];
    if (s == nullptr) {
      buff += " " + std::to_string(ii);
    } else {
      buff += std::string(" ") + s->_name;
    }
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
  _metRCTable.resetCnt(0);

  if (rulesFileName != nullptr) {  // read rules

    int dbunit = _block->getDbUnitsPerMicron();
    double dbFactor = 1;
    if (dbunit > 1000) {
      dbFactor = dbunit * 0.001;
    }

    extRCModel* m = new extRCModel("MINTYPMAX", logger_);
    _modelTable->add(m);

    uint32_t cornerTable[10];
    uint32_t extDbCnt = 0;

    _minModelIndex = 0;
    _maxModelIndex = 0;
    _typModelIndex = 0;
    if (_processCornerTable != nullptr) {
      for (uint32_t ii = 0; ii < _processCornerTable->getCnt(); ii++) {
        extCorner* s = _processCornerTable->get(ii);
        cornerTable[extDbCnt++] = s->_model;
        _modelMap.add(ii);
      }
    }

    logger_->info(
        RCX, 435, "Reading extraction model file {} ...", rulesFileName);

    FILE* rules_file = fopen(rulesFileName, "r");
    if (rules_file == nullptr) {
      logger_->error(
          RCX, 468, "Can't open extraction model file {}", rulesFileName);
    }
    fclose(rules_file);
    bool v2_rules_file = m->isRulesFile_v2((char*) rulesFileName, false);

    if (_v2 || v2_rules_file) {
      m->_v2_flow = _v2;

      if (!(m->readRules((char*) rulesFileName,
                         false,
                         true,
                         true,
                         true,
                         true,
                         extDbCnt,
                         cornerTable,
                         dbFactor))) {
        return false;
      }
    } else {
      if (!(m->readRules_v1((char*) rulesFileName,
                            false,
                            true,
                            true,
                            true,
                            true,
                            extDbCnt,
                            cornerTable,
                            dbFactor))) {
        return false;
      }
    }
    int modelCnt = getRCmodel(0)->getModelCnt();

    // If RCX reads wrong extRules file format
    if (modelCnt == 0) {
      logger_->error(RCX,
                     487,
                     "No RC model read from the extraction model! "
                     "Ensure the right extRules file is used!");
    }
    if (_processCornerTable == nullptr) {
      for (int ii = 0; ii < modelCnt; ii++) {
        addRCCorner(nullptr, ii, 0);
        _modelMap.add(ii);
      }
    }
  }
  _currentModel = getRCmodel(0);
  if (_v2) {
    if (_processCornerTable != nullptr && _couplingFlag > 0) {
      for (uint32_t ii = 0; ii < _processCornerTable->getCnt(); ii++) {
        extCorner* s = _processCornerTable->get(ii);
        _modelMap.add(s->_model);
        _metRCTable.add(_currentModel->getMetRCTable(s->_model));
      }
    }
  } else {
    for (uint32_t ii = 0; (_couplingFlag > 0) && ii < _modelMap.getCnt();
         ii++) {
      uint32_t jj = _modelMap.get(ii);
      _metRCTable.add(_currentModel->getMetRCTable(jj));
    }
  }
  _extDbCnt = _processCornerTable->getCnt();

#ifndef NDEBUG
  uint32_t scaleCornerCnt = 0;
  if (_scaledCornerTable != nullptr) {
    scaleCornerCnt = _scaledCornerTable->getCnt();
  }
  assert(_cornerCnt == _extDbCnt + scaleCornerCnt);
#endif

  _block->setCornerCount(_cornerCnt, _extDbCnt, nullptr);
  return true;
}

void extMain::addDummyCorners(uint32_t cornerCnt)
{
  for (uint32_t ii = 0; ii < cornerCnt; ii++) {
    addRCCorner(nullptr, ii, -1);
  }
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
  _prevControl->_ccNoPowerSource = _ccNoPowerSource;
  _prevControl->_ccNoPowerTarget = _ccNoPowerTarget;
  _prevControl->_usingMetalPlanes = _usingMetalPlanes;
  if (_currentModel && _currentModel->getRuleFileName()) {
    _prevControl->_ruleFileName = _currentModel->getRuleFileName();
  }
}

void extMain::getPrevControl()
{
  if (!_prevControl) {
    return;
  }
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
  _ccNoPowerSource = _prevControl->_ccNoPowerSource;
  _ccNoPowerTarget = _prevControl->_ccNoPowerTarget;
  _usingMetalPlanes = _prevControl->_usingMetalPlanes;
}
bool extMain::modelExists(const char* extRules)
{
  if ((_prevControl->_ruleFileName.empty()) && (getRCmodel(0) == nullptr)
      && (extRules == nullptr)) {
    logger_->warn(RCX,
                  127,
                  "No RC model was read with command <load_model>, "
                  "will not perform extraction!");
    return false;
  }
  return true;
}

void extMain::makeBlockRCsegs(const char* netNames,
                              uint32_t cc_up,
                              uint32_t ccFlag,
                              double resBound,
                              bool mergeViaRes,
                              double ccThres,
                              int contextDepth,
                              const char* extRules)
{
  if (!modelExists(extRules)) {
    return;
  }

  uint32_t debugNetId = 0;

  _diagFlow = true;
  _couplingFlag = ccFlag;
  _coupleThreshold = ccThres;
  _usingMetalPlanes = true;
  _ccUp = cc_up;
  _couplingFlag = ccFlag;
  _ccContextDepth = contextDepth;
  _mergeViaRes = mergeViaRes;
  _mergeResBound = resBound;

  if ((_processCornerTable != nullptr)
      || ((_processCornerTable == nullptr) && (extRules != nullptr))) {
    const char* rulesfile
        = extRules ? extRules : _prevControl->_ruleFileName.c_str();

    // Reading model file
    if (!setCorners(rulesfile)) {
      logger_->info(RCX, 128, "skipping Extraction ...");
      return;
    }
  } else if (setMinTypMax(false, false, false, -1, -1, -1, 1) < 0) {
    logger_->warn(RCX, 129, "Wrong combination of corner related options!");
    return;
  }

  _foreign = false;  // extract after read_spef

  std::vector<dbNet*> inets;
  _allNet = !findSomeNet(_block, netNames, inets, logger_);
  for (auto net : inets) {
    net->setMark(true);
  }

  if (_ccContextDepth) {
    initContextArray();
  }
  initDgContextArray();

  _extRun++;
  _useDbSdb = false;

  extMeasure m(logger_);

  _seqPool = m._seqPool;

  getPeakMemory("Start makeNetRCsegs");
  if (!_allNet) {
    _ccMinX = MAX_INT;
    _ccMinY = MAX_INT;
    _ccMaxX = -MAX_INT;
    _ccMaxY = -MAX_INT;
  }
  setupMapping();

  if (_couplingFlag > 1) {
    calcMinMaxRC();
    getResCapTable();
  }

  logger_->info(RCX,
                436,
                "RC segment generation {} (max_merge_res {:.1f}) ...",
                getBlock()->getName().c_str(),
                _mergeResBound);

  uint32_t cnt = 0;
  for (dbNet* net : _block->getNets()) {
    if (net->getSigType().isSupply()) {
      continue;
    }
    if (!_allNet && !net->isMarked()) {
      continue;
    }

    _connectedBTerm.clear();
    _connectedITerm.clear();
    cnt += makeNetRCsegs(net);
    for (dbBTerm* bterm : _connectedBTerm) {
      bterm->setMark(0);
    }
    for (dbITerm* iterm : _connectedITerm) {
      iterm->setMark(0);
    }
  }
  getPeakMemory("End  makeNetRCsegs");

  logger_->info(RCX, 40, "Final {} rc segments", cnt);

  const int ttttPrintDgContext = 0;
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
                  "less than {:.4f} fF will be grounded.",
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
    if (ttttPrintDgContext) {
      m._dgContextFile = fopen("dgCtxtFile", "w");
    }
    m._dgContextCnt = 0;

    m._ccContextArray = _ccContextArray;

    m._pixelTable = _geomSeq;
    m._minModelIndex = 0;  // couplimg threshold will be appled to this cap
    m._maxModelIndex = 0;
    m._currentModel = _currentModel;
    m._diagModel = _currentModel[0].getDiagModel();
    for (uint32_t ii = 0; ii < _modelMap.getCnt(); ii++) {
      uint32_t jj = _modelMap.get(ii);
      m._metRCTable.add(_currentModel->getMetRCTable(jj));
    }
    const uint32_t techLayerCnt = getExtLayerCnt(_tech) + 1;
    const uint32_t modelLayerCnt = _currentModel->getLayerCnt();
    m._layerCnt = techLayerCnt < modelLayerCnt ? techLayerCnt : modelLayerCnt;
    if (techLayerCnt == 5 && modelLayerCnt == 8) {
      m._layerCnt = modelLayerCnt;
    }
    m.getMinWidth(_tech);
    m.allocOUpool();

    m._debugFP = nullptr;
    m._netId = 0;
    debugNetId = 0;
    if (debugNetId > 0) {
      m._netId = debugNetId;
      char bufName[32];
      sprintf(bufName, "%d", debugNetId);
      m._debugFP = fopen(bufName, "w");
    }

    getPeakMemory("Start CouplingFlow");
    Rect maxRect = _block->getDieArea();
    if (_v2) {
      couplingFlow_v2(maxRect, _couplingFlag, &m);
    } else {
      couplingFlow(maxRect, _couplingFlag, &m, extCompute1);
    }

    getPeakMemory("End CouplingFlow");

    if (m._debugFP != nullptr) {
      fclose(m._debugFP);
    }

    if (m._dgContextFile) {
      fclose(m._dgContextFile);
      m._dgContextFile = nullptr;
    }

    // removeDgContextArray();
  }
  _extracted = true;
  updatePrevControl();
  int numOfNet;
  int numOfRSeg;
  int numOfCapNode;
  int numOfCCSeg;
  _block->getExtCount(numOfNet, numOfRSeg, numOfCapNode, numOfCCSeg);
  if (numOfRSeg) {
    logger_->info(RCX,
                  45,
                  "Extract {} nets, {} rsegs, {} caps, {} ccs",
                  numOfNet - 2,
                  numOfRSeg,
                  numOfCapNode,
                  numOfCCSeg);
  } else {
    logger_->warn(
        RCX, 107, "Nothing is extracted out of {} nets!", numOfNet - 2);
  }
  if (_allNet) {
    for (dbNet* net : _block->getNets()) {
      if (net->getSigType().isSupply()) {
        continue;
      }
      net->setWireAltered(false);
    }
  } else {
    for (dbNet* net : inets) {
      net->setMark(false);
      net->setWireAltered(false);
    }
  }

  /*
    if (_geomSeq != nullptr) {
      delete _geomSeq;
      _geomSeq = nullptr;
    }

  */
  while (_modelTable->notEmpty()) {
    delete _modelTable->pop();
  }
  if (_batchScaleExt) {
    genScaledExt();
  }
}

void extMain::genScaledExt()
{
  if (_processCornerTable == nullptr || _scaledCornerTable == nullptr) {
    return;
  }

  uint32_t ii = 0;
  for (; ii < _scaledCornerTable->getCnt(); ii++) {
    extCorner* sc = _scaledCornerTable->get(ii);
    extCorner* pc = sc->_extCornerPtr;
    if (pc == nullptr) {
      continue;
    }

    uint32_t frdbid = pc->_dbIndex;
    uint32_t todbid = sc->_dbIndex;

    _block->copyExtDb(frdbid,
                      todbid,
                      _cornerCnt,
                      sc->_resFactor,
                      sc->_ccFactor,
                      sc->_gndFactor);
  }
}

double extMain::getTotalNetCap(uint32_t netId, uint32_t cornerNum)
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

uint32_t extMain::openSpefFile(char* filename, uint32_t mode)
{
  uint32_t debug = 0;
  if (filename == nullptr) {
    debug = 1;
  } else if (strcmp(filename, "") == 0) {
    debug = 1;
  }

  if (debug > 0) {
    filename = nullptr;
  }

  if (mode > 0) {
    if (!_spef->setOutSpef(filename)) {
      return 1;
    }
  } else {
    if (!_spef->setInSpef(filename)) {
      return 1;
    }
  }
  return 0;
}

void extMain::writeSPEF(bool stop)
{
  if (stop) {
    _spef->stopWrite();
  }
}

extSpef* extMain::getSpef()
{
  return _spef;
}

void extMain::write_spef_nets(bool flatten, bool parallel)
{
  _spef->write_spef_nets(flatten, parallel);
}

uint32_t extMain::writeSPEF(uint32_t netId,
                            bool single_pi,
                            uint32_t debug,
                            int corner,
                            const char* corner_name,
                            const char* spef_version)
{
  if (_block == nullptr) {
    logger_->info(
        RCX, 474, "Can't execute write_spef command. There's no block in db!");
    return 0;
  }
  if (!_spef || _spef->getBlock() != _block) {
    delete _spef;
    _spef = new extSpef(_tech, _block, logger_, spef_version, this);
  }
  dbNet* net = dbNet::getNet(_block, netId);

  int n = _spef->getWriteCorner(corner, corner_name);
  if (n < -10) {
    return 0;
  }

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

  if (names == nullptr || names[0] == '\0')  // all corners
  {
    _active_corner_cnt = cCnt;
    for (int kk = 0; kk < cCnt; kk++) {
      _active_corner_number[kk] = kk;
    }
    return -1;
  }

  _active_corner_cnt = 0;
  int cn = 0;
  Parser parser(logger_);
  parser.mkWords(names, nullptr);
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

void extMain::writeSPEF(char* filename,
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
                        const char* spef_version,
                        bool parallel)
{
  if (_block == nullptr) {
    logger_->info(
        RCX, 475, "Can't execute write_spef command. There's no block in db");
    return;
  }
  if (!_spef || _spef->getBlock() != _block) {
    delete _spef;
    _spef = new extSpef(_tech, _block, logger_, spef_version, this);
  }
  _spef->_termJxy = termJxy;

  _writeNameMap = noNameMap ? false : true;
  _spef->_writeNameMap = _writeNameMap;
  _spef->setUseIdsFlag(false);
  int cntnet, cntrseg, cntcapn, cntcc;
  _block->getExtCount(cntnet, cntrseg, cntcapn, cntcc);
  if (cntrseg == 0 || cntcapn == 0) {
    logger_->info(
        RCX,
        134,
        "Can't execute write_spef command. There's no extraction data.");
    return;
  }
  if (_extRun == 0) {
    getPrevControl();
    getExtractedCorners();
  }
  _spef->preserveFlag(_foreign);

  if (gzFlag) {
    _spef->setGzipFlag(gzFlag);
  }

  _spef->setDesign((char*) _block->getName().c_str());

  if (openSpefFile(filename, 1) > 0) {
    logger_->info(RCX, 137, "Can't open file \"{}\" to write spef.", filename);
  } else {
    _spef->set_single_pi(single_pi);
    int n = _spef->getWriteCorner(corner, corner_name);
    if (n < -1) {
      return;
    }
    _spef->_db_ext_corner = n;

    std::vector<dbNet*> inets;
    findSomeNet(_block, netNames, inets, logger_);
    _spef->writeBlock(nodeCoord,
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
    if (initOnly) {
      return;
    }
  }
  delete _spef;
  _spef = nullptr;
}

uint32_t extMain::readSPEF(char* filename,
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
                           uint32_t testParsing,
                           bool moreToRead,
                           bool diff,
                           bool calib,
                           int app_print_limit)
{
  if (!_spef || _spef->getBlock() != _block) {
    delete _spef;
    _spef = new extSpef(_tech, _block, logger_, "", this);
  }
  _spef->_moreToRead = moreToRead;
  _spef->incr_rRun();

  if (_extRun == 0) {
    getPrevControl();
  }
  _spef->setCornerCnt(_cornerCnt);
  if (!diff && !calib) {
    _foreign = true;
  }
  if (diff) {
    if (!_extracted) {
      logger_->warn(RCX, 8, "There is no extraction db!");
      return 0;
    }
  } else if (_extracted && !force && !keepLoadedCorner) {
    logger_->warn(RCX, 3, "Read SPEF into extracted db!");
  }

  if (openSpefFile(filename, 0) > 0) {
    return 0;
  }

  _spef->_noCapNumCollapse = noCapNumCollapse;
  _spef->_capNodeFile = nullptr;
  if (capNodeMapFile && capNodeMapFile[0] != '\0') {
    _spef->_capNodeFile = fopen(capNodeMapFile, "w");
    if (_spef->_capNodeFile == nullptr) {
      logger_->warn(
          RCX, 5, "Can't open SPEF file {} to write.", capNodeMapFile);
    }
  }
  std::vector<dbNet*> inets;

  if (_block != nullptr) {
    findSomeNet(_block, netNames, inets, logger_);
  }

  uint32_t cnt = _spef->readBlock(0,
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

  if (_spef->_capNodeFile) {
    fclose(_spef->_capNodeFile);
  }

  if (diff || cnt == 0) {
    delete _spef;
    _spef = nullptr;
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
    for (int ii = 0; ii < 16; ii++) {
      appcnt[ii] = 0;
    }
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
    if (cc_gnd_factor != 0.0) {
      _block->groundCC(cc_gnd_factor);
    }
    delete _spef;
    _spef = nullptr;
  }
  _extRun++;
  updatePrevControl();

  return cnt;
}

uint32_t extMain::readSPEFincr(char* filename)
{
  // assume header/name_map/ports same as first file

  if (!_spef->setInSpef(filename, true)) {
    return 0;
  }

  uint32_t cnt = _spef->readBlockIncr(0);

  return cnt;
}

uint32_t extMain::calibrate(char* filename,
                            bool m_map,
                            float upperLimit,
                            float lowerLimit,
                            const char* dbCornerName,
                            int corner,
                            int spefCorner)
{
  if (!_spef || _spef->getBlock() != _block) {
    delete _spef;
    _spef = new extSpef(_tech, _block, logger_, "", this);
  }
  _spef->setCalibLimit(upperLimit, lowerLimit);
  readSPEF(filename,
           nullptr /*netNames*/,
           false /*force*/,
           false /*rConn*/,
           nullptr /*N*/,
           false /*rCap*/,
           false /*rOnlyCCcap*/,
           false /*rRes*/,
           -1.0 /*cc_thres*/,
           0.0 /*cc_gnd_factor*/,
           1.0 /*length_unit*/,
           m_map,
           false /*noCapNumCollapse*/,
           nullptr /*capNodeMapFile*/,
           false /*log*/,
           corner,
           0.0 /*low*/,
           0.0 /*up*/,
           nullptr /*excludeSubWord*/,
           nullptr /*subWord*/,
           nullptr /*statsFile*/,
           dbCornerName,
           nullptr /*calibrateBaseCorner*/,
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
