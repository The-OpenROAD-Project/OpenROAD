// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include <cstdint>
#include <cstdio>

#include "odb/db.h"
#include "rcx/dbUtil.h"
#include "rcx/extRCap.h"
#include "utl/Logger.h"

namespace rcx {

using utl::RCX;

using odb::dbCCSeg;
using odb::dbNet;
using odb::dbRSeg;

void extMeasure::printTraceNetInfo(const char* msg, uint32_t netId, int rsegId)
{
  if (rsegId <= 0) {
    return;
  }
  dbRSeg* rseg = dbRSeg::getRSeg(_block, rsegId);

  uint32_t shapeId = rseg->getTargetCapNode()->getShapeId();

  int x, y;
  rseg->getCoords(x, y);
  debugPrint(logger_,
             RCX,
             "debug_net",
             1,
             "Trace:C"
             "         {} {}   {}_S{}__{} {}  {:g}\n",
             x,
             y,
             netId,
             shapeId,
             rsegId,
             msg,
             rseg->getCapacitance(0, 1.0));
}

double extMeasure::GetDBcoords(uint32_t coord)
{
  int db_factor = _extMain->_block->getDbUnitsPerMicron();
  return 1.0 * coord / db_factor;
}

double extMeasure::GetDBcoords(int coord)
{
  int db_factor = _extMain->_block->getDbUnitsPerMicron();
  return 1.0 * coord / db_factor;
}

bool extMeasure::IsDebugNet()
{
  if (_no_debug) {
    return false;
  }

  if (!(_extMain->_debug_net_id > 0)) {
    return false;
  }

  if (_netSrcId <= 0 && _netTgtId <= 0) {
    return false;
  }

  if (_netSrcId == _extMain->_debug_net_id
      || _netTgtId == _extMain->_debug_net_id) {
    return true;
  }
  return false;
}

void extMeasure::printNetCaps()
{
  if (_netId <= 0) {
    return;
  }
  dbNet* net = dbNet::getNet(_block, _netId);
  double gndCap = net->getTotalCapacitance(0, false);
  double ccCap = net->getTotalCouplingCap(0);
  double totaCap = gndCap + ccCap;

  debugPrint(logger_,
             RCX,
             "debug_net",
             1,
             "Trace:"
             "C"
             " netCap  {:g} CC {:g} {:g} R {:g}  {}",
             totaCap,
             ccCap,
             gndCap,
             net->getTotalResistance(),
             net->getConstName());
}

bool extMeasure::printTraceNet(const char* msg,
                               bool init,
                               dbCCSeg* cc,
                               uint32_t overSub,
                               uint32_t covered)
{
  if (!IsDebugNet()) {
    return false;
  }

  if (init) {
    if (overSub + covered > 0) {
      fprintf(_debugFP, "SUB %d GS %d ", overSub, covered);
    }

    if (_netSrcId == _netId) {
      printTraceNetInfo("", _netSrcId, _rsegSrcId);
    } else {
      printTraceNetInfo("", _netTgtId, _rsegTgtId);
    }

    return true;
  }

  fprintf(_debugFP, "%s   ", msg);
  if (overSub + covered > 0) {
    fprintf(_debugFP, " L %d SUB %d GS %d ", _len, overSub, covered);
  }

  if (cc != nullptr) {
    fprintf(_debugFP, " %g ", cc->getCapacitance());
  }

  printTraceNetInfo("", _netSrcId, _rsegSrcId);
  printTraceNetInfo(" ", _netTgtId, _rsegTgtId);
  fprintf(_debugFP, "\n");
  return true;
}

// -----------------------------------------------------------------
void extMeasure::segInfo(const char* msg, uint32_t netId, int rsegId)
{
  if (rsegId <= 0) {
    return;
  }
  dbRSeg* rseg = dbRSeg::getRSeg(_block, rsegId);

  uint32_t shapeId = rseg->getTargetCapNode()->getShapeId();

  const char* wire
      = rseg != nullptr
                && extMain::getShapeProperty_rc(rseg->getNet(), rseg->getId())
                       > 0
            ? "Via "
            : "Wire";

  int x, y;
  rseg->getCoords(x, y);
  debugPrint(
      logger_,
      RCX,
      "debug_net",
      1,
      "Trace:C"
      " ------  Total RC Wire Values --- {} \n\t{}  : {}\n\txHi   : {}\n\tyHi  "
      " : "
      "{}\n\tnetId : {}\n\tshapId: {}\n\trsegId: {}\n\tCap   : {:.5f}\n\tRes   "
      ": {:.5f}\n",
      msg,
      wire,
      wire,
      x,
      y,
      netId,
      shapeId,
      rsegId,
      rseg->getCapacitance(0, 1.0),
      rseg->getResistance(0));
}

void extMeasure::rcNetInfo()
{
  if (_netId <= 0) {
    return;
  }
  if (!IsDebugNet()) {
    return;
  }
  dbNet* net = dbNet::getNet(_block, _netId);
  double gndCap = net->getTotalCapacitance(0, false);
  double ccCap = net->getTotalCouplingCap(0);
  double totaCap = gndCap + ccCap;
  debugPrint(
      logger_,
      RCX,
      "debug_net",
      1,
      "Trace:C"
      " ---- Total Net RC Values \n\tNetId  : {}\n\tNetName: {}\n\tCCap   : "
      "{}\n\tGndCap : {}\n\tnetCap : {}\n\tNetRes : {}\n",
      _netId,
      net->getConstName(),
      ccCap,
      gndCap,
      totaCap,
      net->getTotalResistance());
}

bool extMeasure::rcSegInfo()
{
  if (!IsDebugNet()) {
    return false;
  }

  rcNetInfo();

  if (_netSrcId == _netId) {
    segInfo("SRC", _netSrcId, _rsegSrcId);
  } else {
    segInfo("DST", _netTgtId, _rsegTgtId);
  }

  return true;
}

bool extMeasure::ouCovered_debug(int covered)
{
  if (!IsDebugNet()) {
    return false;
  }
  int sub = (int) _len - covered;
  debugPrint(logger_,
             RCX,
             "debug_net",
             1,
             "Trace:C"
             " ----- OverUnder Lengths\n\tLevel : M{}\n\tWidth : {}\n\tDist "
             " : {}\n\tLen   : {}\n\tOU_len: {}\n\tSubLen: {}\n\tDiag  : {}\n",
             _met,
             _width,
             _dist,
             _len,
             covered,
             sub,
             _diag);

  return true;
}

bool extMeasure::isVia(uint32_t rsegId)
{
  dbRSeg* rseg1 = dbRSeg::getRSeg(_block, rsegId);

  bool rvia1
      = rseg1 != nullptr
                && extMain::getShapeProperty_rc(rseg1->getNet(), rseg1->getId())
                       > 0
            ? true
            : false;
  return rvia1;
}

bool extMeasure::ouRCvalues(const char* msg, uint32_t jj)
{
  if (!IsDebugNet()) {
    return false;
  }

  debugPrint(
      logger_,
      RCX,
      "debug_net",
      1,
      "Trace:C"
      " --- OverUnder Cap/Res Values\n\t{}: {}\n\tfrCap :{:g}\n\tcCap  : "
      "{:g}\n\tdgCap : {:g}\n\tRes  : {:g}\n\n",
      msg,
      msg,
      _rc[jj]->fringe_,
      _rc[jj]->coupling_,
      _rc[jj]->diag_,
      _rc[jj]->res_);

  return true;
}

bool extMeasure::OverSubDebug(extDistRC* rc,
                              int lenOverSub,
                              int lenOverSub_res,
                              double res,
                              double cap,
                              const char* openDist)
{
  if (!IsDebugNet()) {
    return false;
  }

  char buf[100];
  sprintf(buf, "Over SUB %s", openDist);

  rc->printDebugRC(buf, logger_);

  debugPrint(logger_,
             RCX,
             "debug_net",
             1,
             "Trace:C"
             " ------ Wire Type: {} \n\tLevel : M{}\n\tWidth : {}\n\tLen   : "
             "{}\n\tSubLen: {}\n\tCap   : {:g}\n\tRes    : {:g}\n",
             openDist,
             _met,
             _width,
             _len,
             lenOverSub,
             cap,
             res);

  rcSegInfo();

  return true;
}

bool extMeasure::Debug_DiagValues(double res, double cap, const char* openDist)
{
  if (!IsDebugNet()) {
    return false;
  }

  debugPrint(logger_,
             RCX,
             "debug_net",
             1,
             "Trace:C"
             " ------ {} --------- Res:  {:g}\tDiagCap:  {:g}\n",
             openDist,
             res,
             cap);

  return true;
}

bool extMeasure::OverSubDebug(extDistRC* rc, int lenOverSub, int lenOverSub_res)
{
  if (!IsDebugNet()) {
    return false;
  }

  rc->printDebugRC("Over SUB", logger_);
  rcSegInfo();

  return true;
}

bool extMeasure::DebugStart(bool allNets)
{
  if (!IsDebugNet() && !allNets) {
    return false;
  }
  if (_dist < 0) {
    debugPrint(logger_,
               RCX,
               "debug_net",
               1,
               "[BEGIN DEBUGGING] measureRC:RC ----- BEGIN -- M{} Len={} "
               "({:.3f}) MaxDist",
               _met,
               _len,
               GetDBcoords(_len));
  } else {
    debugPrint(logger_,
               RCX,
               "debug_net",
               1,
               "[BEGIN DEBUGGING] measureRC:RC ----- BEGIN -- M{} Dist={} "
               "({:.3f}) Len={} ({:.3f})",
               _met,
               _dist,
               GetDBcoords(_dist),
               _len,
               GetDBcoords(_len));
  }
  uint32_t debugTgtId = _netSrcId == _netId ? _netSrcId : _netTgtId;

  dbNet* net = dbNet::getNet(_block, debugTgtId);
  debugPrint(logger_,
             RCX,
             "debug_net",
             1,
             "measureRC:C"
             "\tCoupling Segment: Coords \n\tNeighboring Net : {} {}\n\tloX : "
             "{} {:3f} \n\thiX : {} "
             "{:.3f} \n\tloY : {} {:.3f} \n\thiY : {} {:.3f} \n\tDX  : {} "
             "{:3f} \n\tDY  "
             ": {} {:3f}",
             net->getConstName(),
             debugTgtId,
             _ll[0],
             GetDBcoords(_ll[0]),
             _ur[0],
             GetDBcoords(_ur[0]),
             _ll[1],
             GetDBcoords(_ll[1]),
             _ur[1],
             GetDBcoords(_ur[1]),
             _ur[0] - _ll[0],
             GetDBcoords(_ur[0]) - GetDBcoords(_ll[0]),
             _ur[1] - _ll[1],
             GetDBcoords(_ur[1]) - GetDBcoords(_ll[1]));
  return true;
}
/* DELETE ???
bool extMeasure::DebugDiagCoords(int met,
                                 int targetMet,
                                 int len1,
                                 int diagDist,
                                 int ll[2],
                                 int ur[2])
{
  if (!IsDebugNet()) {
    return false;
  }
  debugPrint(
      logger_,
      RCX,
      "debug_net",
      1,
      "[DIAG_EXT:C]"
      "\t----- Diagonal Coupling Coords ----- \n\tDiag: M{} to M{}\n\tDist: {}
{:.3f}\n\tLen : {} {:.3f} \
        \n\tloX : {} {:.3f} \n\thiX : {} {:.3f} \n\tloY : {} {:.3f} \n\thiY : {}
{:.3f} \ \n\tDX  : {} {:.3f} \n\tDY  : {} {:.3f} \n", met, targetMet, diagDist,
      GetDBcoords(diagDist),
      len1,
      GetDBcoords(len1),
      ll[0],
      GetDBcoords(ll[0]),
      ur[0],
      GetDBcoords(ur[0]),
      ll[1],
      GetDBcoords(ll[1]),
      ur[1],
      GetDBcoords(ur[1]),
      ur[0] - ll[0],
      GetDBcoords(ur[0]) - GetDBcoords(ll[0]),
      ur[1] - ll[1],
      GetDBcoords(ur[1]) - GetDBcoords(ll[1]));
  return true;
}
*/
// -----------------------------------------------------------------
//
// from extRCmodel.cpp

void extDistRC::printDebug(const char* from,
                           const char* name,
                           uint32_t len,
                           uint32_t dist,
                           extDistRC* rcUnit)
{
  if (rcUnit != nullptr) {
    debugPrint(
        logger_, RCX, "debug_net", 1, "[DistRC:C]\t--\trcUnit is not nullptr");
  }

  debugPrint(logger_,
             RCX,
             "debug_net",
             1,
             "[DistRC:C]"
             "\t{}: {} {:g} {:g} {:g} R {:g} {} {}",
             from,
             name,
             coupling_,
             fringe_,
             diag_,
             res_,
             len,
             dist);
  if (rcUnit == nullptr) {
    debugPrint(
        logger_, RCX, "debug_net", 1, "[DistRC:C]\t--\trcUnit is nullptr");
  } else {
    rcUnit->printDebugRC("   ", logger_);
  }
}

void extDistRC::printDebugRC(const char* from, utl::Logger* logger)
{
  debugPrint(logger,
             RCX,
             "debug_net",
             1,
             "[DistRC:C]"
             " ---- {}: extRule\n\tDist  : {}\n\tCouple: {:g}\n\tFringe: "
             "{:g}\n\tDiagC "
             ": {:g}\n\ttotCap: {:g}\n\tRes  : {:g}\n",
             from,
             sep_,
             coupling_,
             fringe_,
             diag_,
             coupling_ + fringe_ + diag_,
             res_);
}

double extDistRC::GetDBcoords(int x, int db_factor)
{
  return 1.0 * x / db_factor;
}

void extDistRC::printDebugRC_diag(int met,
                                  int overMet,
                                  int underMet,
                                  int width,
                                  int dist,
                                  int dbUnit,
                                  utl::Logger* logger)
{
  char tmp[100];

  sprintf(tmp, "M%d diag to M%d", met, overMet);

  char tmp1[100];
  sprintf(tmp1,
          "Width=%d (%.3f) Dist=%d (%.3f) ",
          width,
          GetDBcoords(width, dbUnit),
          dist,
          GetDBcoords(dist, dbUnit));

  debugPrint(logger,
             RCX,
             "debug_net",
             1,
             "[EXTRULE:C]"
             " ----- extRule ----- {} {}\n\t\t\tDist  : {}\n\t\t\tCouple: "
             "{:g}\n\t\t\tFringe: {:g}\n\t\t\tDiagC "
             ": {:g}\n\t\t\ttotCap: {:g}\n\t\t\tRes   : {:g}\n",
             tmp,
             tmp1,
             sep_,
             coupling_,
             fringe_,
             diag_,
             coupling_ + fringe_ + diag_,
             res_);
}

void extDistRC::printDebugRC(int met,
                             int overMet,
                             int underMet,
                             int width,
                             int dist,
                             int dbUnit,
                             utl::Logger* logger)
{
  char tmp[100];
  if (overMet > 0 && underMet > 0) {
    sprintf(tmp, "M%d over M%d under M%d ", met, underMet, overMet);
  } else if (underMet > 0) {
    sprintf(tmp, "M%d over M%d ", met, underMet);
  } else if (overMet > 0) {
    sprintf(tmp, "M%d under M%d", met, overMet);
  }

  char tmp1[100];
  sprintf(tmp1,
          "Width=%d (%.3f) Dist=%d (%.3f) ",
          width,
          GetDBcoords(width, dbUnit),
          dist,
          GetDBcoords(dist, dbUnit));
  if (dist < 0) {
    sprintf(
        tmp1, "Width=%d (%.3f) MaxDist ", width, GetDBcoords(width, dbUnit));
  }

  debugPrint(logger,
             RCX,
             "debug_net",
             1,
             "[EXTRULE:C]"
             " ----- extRule ----- {} {}\n\t\t\tDist  : {}\n\t\t\tCouple: "
             "{:g}\n\t\t\tFringe: {:g}\n\t\t\tDiagC "
             ": {:g}\n\t\t\ttotCap: {:g}\n\t\t\tRes   : {:g}\n",
             tmp,
             tmp1,
             sep_,
             coupling_,
             fringe_,
             diag_,
             coupling_ + fringe_ + diag_,
             res_);
}

void extDistRC::printDebugRC_sum(int len, int dbUnit, utl::Logger* logger)
{
  debugPrint(logger,
             RCX,
             "debug_net",
             1,
             "[DistRC:C]"
             " ----- OverUnder Sum ----- Len={} ({:.3f}) \n\t\t\tCouple: "
             "{:.6f}\n\t\t\tFringe: {:.6f}\n\t\t\tDiagC "
             ": {:.6f}\n\t\t\ttotCap: {:.6f}\n\t\t\tRes   : {:.6f}\n",
             len,
             GetDBcoords(len, dbUnit),
             coupling_,
             fringe_,
             diag_,
             coupling_ + fringe_ + diag_,
             res_);
}

void extDistRC::printDebugRC_values(const char* msg)
{
  debugPrint(logger_,
             RCX,
             "debug_net",
             1,
             "[DistRC:C]"
             " ---- {} ------------ \n\t\t\tCouple: {:.6f}\n\t\t\tFringe: "
             "{:.6f}\n\t\t\tDiagC "
             ": {:.6f}\n\t\t\ttotCap: {:.6f}\n\t\t\tRes   : {:.6f}\n",
             msg,
             coupling_,
             fringe_,
             diag_,
             coupling_ + fringe_ + diag_,
             res_);
}

}  // namespace rcx
