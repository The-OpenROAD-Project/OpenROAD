
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

#include "dbUtil.h"
#include "rcx/extMeasureRC.h"
#include "rcx/extRCap.h"
#include "rcx/extSegment.h"
#include "utl/Logger.h"

#ifdef HI_ACC_1
#define FRINGE_UP_DOWN
#endif
// #define CHECK_SAME_NET
// #define MIN_FOR_LOOPS

namespace rcx {

using utl::RCX;
using namespace odb;

void extMeasureRC::GetOUname(char buf[200], int met, int metOver, int metUnder)
{
  if (metUnder > 0 && metOver > 0)
    sprintf(buf, "M%doM%duM%d", met, metUnder, metOver);
  else if (metUnder > 0)
    sprintf(buf, "M%doM%d", met, metUnder);
  else if (metOver > 0)
    sprintf(buf, "M%duM%d", met, metOver);
  else
    sprintf(buf, "M%d", met);
}
void extMeasureRC::PrintCrossSeg(FILE* fp,
                                 int x1,
                                 int len,
                                 int met,
                                 int metOver,
                                 int metUnder,
                                 const char* prefix)
{
  if (fp == NULL)
    return;
  char buf[200];
  GetOUname(buf, met, metOver, metUnder);
  fprintf(fp,
          "%s%7.3f %7.3f %7s %dL\n",
          prefix,
          GetDBcoords(x1),
          GetDBcoords(x1 + len),
          buf,
          len);
}
void extMeasureRC::PrintOUSeg(FILE* fp,
                              int x1,
                              int len,
                              int met,
                              int metOver,
                              int metUnder,
                              const char* prefix,
                              int up_dist,
                              int down_dist)
{
  if (fp == NULL)
    return;
  char buf[200];
  GetOUname(buf, met, metOver, metUnder);
  fprintf(fp,
          "%s%7.3f %7.3f %7s %dL up%d down%d\n",
          prefix,
          GetDBcoords(x1),
          GetDBcoords(x1 + len),
          buf,
          len,
          up_dist,
          down_dist);
}

void extMeasureRC::PrintCrossOvelaps(Wire* w,
                                     uint tgt_met,
                                     int x1,
                                     int len,
                                     Ath__array1D<extSegment*>* segTable,
                                     int totLen,
                                     const char* prefix,
                                     int metOver,
                                     int metUnder)
{
  if (_segFP != NULL && segTable->getCnt() > 0) {
    fprintf(_segFP, "%s %dL cnt %d \n", prefix, totLen, segTable->getCnt());
    PrintCrossSeg(_segFP, x1, len, w->getLevel(), -1, -1, "\t");
    for (uint ii = 0; ii < segTable->getCnt(); ii++) {
      extSegment* s = segTable->get(ii);
      PrintCrossSeg(_segFP,
                    s->_ll[!_dir],
                    s->_len,
                    tgt_met,
                    s->_metOver,
                    s->_metUnder,
                    "\t");
    }
  }
}
void extMeasureRC::PrintOverlapSeg(FILE* fp,
                                   extSegment* s,
                                   int tgt_met,
                                   const char* prefix)
{
  if (fp != NULL)
    fprintf(fp,
            "%s%7.3f %7.3f  %dL\n",
            prefix,
            GetDBcoords(s->_xy),
            GetDBcoords(s->_xy + s->_len),
            s->_len);
}

void extMeasureRC::PrintOvelaps(extSegment* w,
                                uint met,
                                uint tgt_met,
                                Ath__array1D<extSegment*>* segTable,
                                const char* ou)
{
  if (_segFP != NULL && segTable->getCnt() > 0) {
    fprintf(_segFP,
            "\n%7.3f %7.3f  %dL M%d%sM%d cnt=%d\n",
            GetDBcoords(w->_xy),
            GetDBcoords(w->_xy + w->_len),
            w->_len,
            met,
            ou,
            tgt_met,
            segTable->getCnt());
    for (uint ii = 0; ii < segTable->getCnt(); ii++) {
      extSegment* s = segTable->get(ii);
      PrintOverlapSeg(_segFP, s, tgt_met, "");
    }
  }
}
void extMeasureRC::PrintCrossOvelapsOU(Wire* w,
                                       uint tgt_met,
                                       int x1,
                                       int len,
                                       Ath__array1D<extSegment*>* segTable,
                                       int totLen,
                                       const char* prefix,
                                       int metOver,
                                       int metUnder)
{
  if (_segFP != NULL && segTable->getCnt() > 0) {
    for (uint ii = 0; ii < segTable->getCnt(); ii++) {
      extSegment* s = segTable->get(ii);
      if (s->_metOver == metOver && s->_metUnder == metUnder)
        PrintCrossSeg(
            _segFP, s->_xy, s->_len, tgt_met, s->_metOver, s->_metUnder, "\t");
    }
  }
}

void extMeasureRC::printCaps(FILE* fp,
                             double totCap,
                             double cc,
                             double fr,
                             double dfr,
                             double res,
                             const char* msg)
{
  fprintf(fp,
          "%s tot %7.3f  CC  %7.3f  fr %7.3f  wfr %7.3f Res %7.3f",
          msg,
          totCap,
          cc,
          fr,
          dfr,
          res);
}
void extMeasureRC::printNetCaps(FILE* fp, const char* msg)
{
  if (_netId <= 0)
    return;
  dbNet* net = dbNet::getNet(_block, _netId);
  double gndCap = net->getTotalCapacitance(0, false);
  double ccCap = net->getTotalCouplingCap(0);
  double totCap = gndCap + ccCap;
  double res = net->getTotalResistance(0);

  fprintf(fp, "%s netCap", msg);
  printCaps(fp, totCap, ccCap, gndCap, 0, res, "");
  fprintf(fp, " %s\n", net->getConstName());
}
const char* extMeasureRC::srcWord(int& rsegId)
{
  rsegId = _netSrcId;
  if (_netTgtId == _netId) {
    rsegId = rsegId;
    return "TGT";
  }
  return "SRC";
}
const char* extMeasureRC::GetSrcWord(int rsegId)
{
  if (_rsegTgtId == rsegId)
    return "TGT";

  return "SRC";
}
void extMeasureRC::PrintCoord(FILE* fp, int x, const char* xy)
{
  fprintf(fp, "%s %5d %7.3f ", xy, x, GetDBcoords(x));
}
void extMeasureRC::PrintCoords(FILE* fp, int x, int y, const char* xy)
{
  PrintCoord(fp, x, xy);
  PrintCoord(fp, y, "");
}
void extMeasureRC::PrintCoords(FILE* fp, int ll[2], const char* xy)
{
  PrintCoords(fp, ll[0], ll[1], xy);
}
bool extMeasureRC::PrintCurrentCoords(FILE* fp, const char* msg, uint rseg)
{
  int dx = _ur[0] - _ll[0];
  int dy = _ur[1] - _ll[1];

  fprintf(fp, "%s", msg);
  PrintCoords(fp, _ll, "x");
  PrintCoords(fp, _ur, "y");
  fprintf(fp, " dx %d  dy %d rseg %d\n", dx, dy, rseg);
  return true;
}
FILE* extMeasureRC::OpenDebugFile()
{
  if (_debugFP == NULL && _netId > 0) {
    char buff[100];
    sprintf(buff, "%d.rc", _netId);
    _debugFP = fopen(buff, "w");
    if (_debugFP == NULL) {
      // warning(0, "Cannot Open file %s with permissions w", buff);
      exit(0);
    }
  }
  return _debugFP;
}
bool extMeasureRC::DebugStart(FILE* fp, bool allNets)
{
  if (!IsDebugNet() && !allNets)
    return false;

  // DELETE uint debugTgtId = _netSrcId == _netId ? _netSrcId : _netTgtId;
  dbNet* net1 = dbNet::getNet(_block, _netId);
  int rsegId;
  const char* src = srcWord(rsegId);

  fprintf(fp,
          "BEGIN met %d dist %d %7.3f  len %d %7.3f %s rseg %d net %d %s\n",
          _met,
          _dist,
          GetDBcoords(_dist),
          _len,
          GetDBcoords(_len),
          src,
          rsegId,
          _netId,
          net1->getConstName());

  DebugCoords(fp, _rsegSrcId, _ll, _ur, "BEGIN-coords");
  DebugCoords(fp, _rsegTgtId, _ll_tgt, _ur_tgt, "BEGIN-coords");

  printNetCaps(fp, "BEGIN");
  PrintCurrentCoords(fp, "BEGIN coords ", rsegId);
  segInfo(fp, "BEGIN", _netId, rsegId);

  return true;
}
bool extMeasureRC::DebugEnd(FILE* fp, int OU_covered)
{
  if (!IsDebugNet())
    return false;

  // DELETE uint debugTgtId = _netSrcId == _netId ? _netSrcId : _netTgtId;
  dbNet* net1 = dbNet::getNet(_block, _netId);
  int rsegId;
  const char* src = srcWord(rsegId);

  fprintf(fp,
          "\tOU_covered %d %7.3f    met %d D%d %7.3f  len %d %7.3f %s rseg %d "
          "net %d %s\n",
          OU_covered,
          GetDBcoords(OU_covered),
          _met,
          _dist,
          GetDBcoords(_dist),
          _len,
          GetDBcoords(_len),
          src,
          rsegId,
          _netId,
          net1->getConstName());

  segInfo(fp, "END", _netId, rsegId);
  printNetCaps(fp, "END");

  return true;
}
void extMeasureRC::DebugStart_res(FILE* fp)
{
  fprintf(fp, "\tBEGIN_res ");
  PrintCoords(fp, _ll, "x");
  PrintCoords(fp, _ur, "y");
  dbNet* net1 = dbNet::getNet(_block, _netId);

  fprintf(fp,
          " M%d  D%d  L%d   N%d N%d %s\n",
          _met,
          _dist,
          _len,
          _netSrcId,
          _netTgtId,
          net1->getConstName());
}
void extMeasureRC::DebugRes_calc(FILE* fp,
                                 const char* msg,
                                 int rsegId1,
                                 const char* msg_len,
                                 uint len,
                                 int dist1,
                                 int dist2,
                                 int tgtMet,
                                 double tot,
                                 double R,
                                 double unit,
                                 double prev)
{
  fprintf(fp,
          "\t\%s tot %g addR %g unit %g prevR %g -- M%d %s %d d1 %d d2 %d rseg "
          "%d\n",
          msg,
          tot,
          R,
          unit,
          prev,
          tgtMet,
          msg_len,
          len,
          dist1,
          dist2,
          rsegId1);
}
bool extMeasureRC::DebugDiagCoords(FILE* fp,
                                   int met,
                                   int targetMet,
                                   int len1,
                                   int diagDist,
                                   int ll[2],
                                   int ur[2],
                                   const char* msg)
{
  if (!IsDebugNet())
    return false;
  fprintf(fp,
          "%s M%d M%d L%d dist %d %.3f ",
          msg,
          met,
          targetMet,
          len1,
          diagDist,
          GetDBcoords(diagDist));
  PrintCoords(fp, ll[0], ur[0], " x ");
  PrintCoords(fp, ll[1], ur[1], " y ");
  fprintf(fp, "\n");

  return true;
}
bool extMeasureRC::DebugCoords(FILE* fp,
                               int rsegId,
                               int ll[2],
                               int ur[2],
                               const char* msg)
{
  if (!IsDebugNet())
    return false;

  uint dx = ur[0] - ll[0];
  uint dy = ur[1] - ll[1];
  fprintf(fp, "%s DX %d  DY %d  ", msg, dx, dy);
  PrintCoords(fp, ll[0], ur[0], " x ");
  PrintCoords(fp, ll[1], ur[1], " y ");
  fprintf(fp, " r%d %s\n", rsegId, GetSrcWord(rsegId));

  return true;
}
bool extMeasureRC::DebugDiagCoords(int met,
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
      _extMain->getLogger(),
      RCX,
      "debug_net",
      1,
      "[DIAG_EXT:C]"
      "\t----- Diagonal Coupling Coords ----- \n\tDiag: M{} to M{}\n\tDist: {} {:.3f}\n\tLen : {} {:.3f} \
        \n\tloX : {} {:.3f} \n\thiX : {} {:.3f} \n\tloY : {} {:.3f} \n\thiY : {} {:.3f} \
        \n\tDX  : {} {:.3f} \n\tDY  : {} {:.3f} \n",
      met,
      targetMet,
      diagDist,
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
void extMeasureRC::DebugEnd_res(FILE* fp,
                                int rseg1,
                                int len_covered,
                                const char* msg)
{
  dbRSeg* rseg = dbRSeg::getRSeg(_block, rseg1);
  dbNet* net = rseg->getNet();
  double R0 = rseg->getResistance();
  double R = net->getTotalResistance();
  // DELETE double res= rseg->getResistance();
  fprintf(fp,
          "%s len_down %d rsegR %g  netR %g  %d %d %s\n",
          msg,
          len_covered,
          R0,
          R,
          rseg1,
          net->getId(),
          net->getConstName());
}
double extMeasureRC::getCC(int rsegId)
{
  dbRSeg* rseg = dbRSeg::getRSeg(_block, rsegId);
  double tot_cc = 0;
  dbCapNode* n = rseg->getTargetCapNode();
  dbSet<dbCCSeg> ccSegs = n->getCCSegs();
  dbSet<dbCCSeg>::iterator ccitr;
  for (ccitr = ccSegs.begin(); ccitr != ccSegs.end(); ++ccitr) {
    dbCCSeg* cc = *ccitr;
    tot_cc += cc->getCapacitance();
  }
  return tot_cc;
}

void extMeasureRC::segInfo(FILE* fp, const char* msg, uint netId, int rsegId)
{
  if (rsegId <= 0) {
    return;
  }
  dbRSeg* rseg = dbRSeg::getRSeg(_block, rsegId);
  uint shapeId = rseg->getTargetCapNode()->getShapeId();
  const char* wire
      = rseg != NULL
                && extMain::getShapeProperty_rc(rseg->getNet(), rseg->getId())
                       > 0
            ? "Via "
            : "Wire";
  dbNet* net1 = dbNet::getNet(_block, netId);

  double gndCap = rseg->getCapacitance(0);
  double cc = getCC(rsegId);
  double totCap = gndCap + cc;
  double res = rseg->getResistance();

  int x, y;
  rseg->getCoords(x, y);
  fprintf(fp, "%s %s ", msg, "rseg ");
  printCaps(fp, totCap, cc, gndCap, 0, res, "");
  fprintf(fp, "\n");

  fprintf(fp, "%s %s ", msg, wire);
  PrintCoords(fp, x, y, " ");
  fprintf(fp,
          " sh %d rseg %d net %d %s\n",
          shapeId,
          rsegId,
          netId,
          net1->getConstName());
}
void extMeasureRC::GetPatName(int met, int overMet, int underMet, char tmp[50])
{
  if (overMet > 0 && underMet > 0)
    sprintf(tmp, "M%doM%duM%d ", met, underMet, overMet);
  else if (underMet > 0)
    sprintf(tmp, "M%doM%d ", met, underMet);
  else if (overMet > 0)
    sprintf(tmp, "M%duM%d", met, overMet);
  else
    sprintf(tmp, "M%doM%d", met, 0);
}
void extMeasureRC::printDebugRC(FILE* fp,
                                extDistRC* rc,
                                const char* msg,
                                const char* post)
{
  fprintf(fp,
          "%s s %d  CC %g  fr %g wfr %g dg %g  TC %g  R %g %s",
          msg,
          rc->_sep,
          rc->_coupling,
          rc->_fringe,
          rc->_fringeW,
          rc->_diag,
          rc->_coupling + rc->_fringe + rc->_diag + rc->_fringeW,
          rc->_res,
          post);
}
void extMeasureRC::printDebugRC(FILE* fp,
                                extDistRC* rc,
                                int met,
                                int overMet,
                                int underMet,
                                int width,
                                int dist,
                                int len,
                                const char* msg)
{
  char tmp[50];
  GetPatName(met, overMet, underMet, tmp);
  fprintf(fp,
          "\t%s L%d %.3f W%d %.3f D%d %.3f ",
          tmp,
          len,
          GetDBcoords(len),
          width,
          GetDBcoords(width),
          dist,
          GetDBcoords(dist));
  printDebugRC(fp, rc, msg);
  fprintf(fp, "\n");
}
void extMeasureRC::printDebugRC_diag(FILE* fp,
                                     extDistRC* rc,
                                     int met,
                                     int overMet,
                                     int underMet,
                                     int width,
                                     int dist,
                                     int len,
                                     const char* msg)
{
  char tmp[100];
  GetPatName(met, overMet, underMet, tmp);
  fprintf(fp,
          "\t%s_diag L%d %.3f W%d %.3f D%d %.3f ",
          tmp,
          len,
          GetDBcoords(len),
          width,
          GetDBcoords(width),
          dist,
          GetDBcoords(dist));
  printDebugRC(fp, rc, msg);
  fprintf(fp, "\n");
}
void extMeasureRC::DebugPrintNetids(FILE* fp,
                                    const char* msg,
                                    int rsegId,
                                    const char* eol)
{
  if (rsegId <= 0)
    return;

  const char* src = rsegId == _rsegSrcId ? "SRC" : "DST";
  dbRSeg* rseg = dbRSeg::getRSeg(_block, rsegId);
  uint shapeId = rseg->getTargetCapNode()->getShapeId();
  // DELETE const char* wire = rseg != NULL &&
  // extMain::getShapeProperty_rc(rseg->getNet(), rseg->getId())> 0 ? "Via " :
  // "Wire";
  dbNet* net = rseg->getNet();

  fprintf(fp,
          "%s%s s%d r%d N%d %s %s",
          msg,
          src,
          shapeId,
          rsegId,
          net->getId(),
          net->getConstName(),
          eol);
}
void extMeasureRC::DebugUpdateValue(FILE* fp,
                                    const char* msg,
                                    const char* cap_type,
                                    int rsegId,
                                    double v,
                                    double tot)
{
  if (rsegId <= 0)
    return;
  const char* new_val = (tot - v) * 100000 > 0 ? "NEW" : "";
  fprintf(fp, "%s %s %.4f tot %.5f %s ", msg, cap_type, v, tot, new_val);
  DebugPrintNetids(fp, "", rsegId);
  fprintf(fp, "\n");
}
void extMeasureRC::DebugUpdateCC(FILE* fp,
                                 const char* msg,
                                 int rseg1,
                                 int rseg2,
                                 double v,
                                 double tot)
{
  DebugUpdateValue(fp, msg, "CC", rseg1, v, tot);
  DebugUpdateValue(fp, msg, "CC", rseg2, v, tot);
}

}  // namespace rcx
