
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

#include "grids.h"
#include "rcx/extRCap.h"
#include "rcx/extprocess.h"

#ifdef _WIN32
#include "direct.h"
#endif

#include <map>
#include <vector>

#include "utl/Logger.h"

// #define SKIP_SOLVER
namespace rcx {

using namespace odb;
using utl::RCX;

uint extRCModel::measureWithVar(extMeasure* measure)
{
  uint cnt = 0;
  int met = measure->_met;
  extVariation* xvar = _process->getVariation(measure->_met);

  extConductor* cond = _process->getConductor(met);
  double t = cond->_thickness;
  double h = cond->_height;
  double ro = cond->_p;
  double res = 0.0;
  double top_ext = cond->_top_ext;
  double bot_ext = cond->_bot_ext;

  //	extMasterConductor* m= _process->getMasterConductor(met);
  if (top_ext != 0.0 || bot_ext != 0.0) {
    xvar = NULL;
    measure->_metExtFlag = true;
  }

  for (uint dIndex = 0; dIndex < measure->_dataTable.getCnt(); dIndex++) {
    double r = measure->_dataTable.get(dIndex);  // layout
    measure->_rIndex = dIndex;

    uint wcnt;
    uint scnt;
    if (dIndex) {
      wcnt = measure->_widthTable.getCnt();
      if (!measure->_diag)
        scnt = measure->_spaceTable.getCnt();
      else
        //      scnt=measure->_spaceTable0.getCnt();
        scnt = measure->_diagSpaceTable0.getCnt();
    } else {
      wcnt = measure->_widthTable0.getCnt();
      if (!measure->_diag)
        scnt = measure->_spaceTable0.getCnt();
      else
        scnt = measure->_diagSpaceTable0.getCnt();
    }
    for (uint wIndex = 0; wIndex < wcnt; wIndex++) {
      double w;
      if (!dIndex)
        w = measure->_widthTable0.get(wIndex);
      else
        w = measure->_widthTable.get(wIndex);  // layout
      measure->_wIndex = wIndex;

      for (uint sIndex = 0; sIndex < scnt; sIndex++) {
        double s;
        if (!measure->_diag) {
          if (!dIndex)
            s = measure->_spaceTable0.get(sIndex);
          else
            s = measure->_spaceTable.get(sIndex);  // layout
          if (sIndex == scnt - 1 && wIndex == wcnt - 1 && !measure->_overUnder)
            measure->_plate = true;
          else
            measure->_plate = false;
        } else
          s = measure->_diagSpaceTable0.get(sIndex);

        double top_width = w + 2 * top_ext;
        double top_widthR = w + 2 * top_ext;
        double bot_width = w + 2 * bot_ext;
        double thickness = t;
        double bot_widthR = w + 2 * bot_ext;
        double thicknessR = t;

        if (r == 0.0) {
          //					top_width= w;
          //					top_widthR= w;
          if (xvar != NULL) {
            double a = xvar->getP(w);
            if (a != 0.0)
              ro = a;
          }
          res = measureResistance(
              measure, ro, top_widthR, bot_widthR, thicknessR);
        } else if (xvar != NULL && !_maxMinFlag) {
          uint ss;
          if (measure->_diag)
            ss = 5;
          else {
            if (sIndex < scnt - 1)
              ss = sIndex;
            else
              ss = scnt - 2;
          }
          /*
                                                  top_width=
             xvar->getTopWidth(wIndex, sIndex); top_widthR=
             xvar->getTopWidthR(wIndex, sIndex);
          */
          top_width = xvar->getTopWidth(wIndex, ss);
          top_widthR = xvar->getTopWidthR(wIndex, ss);

          bot_width = xvar->getBottomWidth(w, dIndex - 1);
          bot_width = top_width - bot_width;

          thickness = xvar->getThickness(w, dIndex - 1);

          bot_widthR = xvar->getBottomWidthR(w, dIndex - 1);
          bot_widthR = top_widthR - bot_widthR;

          thicknessR = xvar->getThicknessR(w, dIndex - 1);
          double a = xvar->getP(w);
          if (a != 0.0)
            ro = a;
          res = measureResistance(
              measure, ro, top_widthR, bot_widthR, thicknessR);
        } else if (_maxMinFlag && r == 1.0) {
          top_width = w - 2 * cond->_min_cw_del;
          thickness = t - cond->_min_ct_del;
          bot_width = top_width - 2 * thickness * cond->_min_ca;
          if (bot_width > w)
            bot_width = w;
          res = measureResistance(
              measure, ro, top_widthR, bot_widthR, thicknessR);
        } else if (_maxMinFlag && r == 2.0) {
          top_width = w + 2 * cond->_max_cw_del;
          thickness = t + cond->_max_ct_del;
          bot_width = top_width - 2 * thickness * cond->_max_ca;
          if (bot_width < w)
            bot_width = w;
          res = measureResistance(
              measure, ro, top_widthR, bot_widthR, thicknessR);
        } else if (measure->_thickVarFlag) {
          thickness *= 1 + r;
          thicknessR *= 1 + r;
        } else {
          continue;
        }

        measure->setTargetParams(w, s, r, t, h);
        measurePatternVar(measure,
                          top_width,
                          bot_width,
                          thickness,
                          measure->_wireCnt,
                          NULL,
                          res * 0.5);

        //				measure->setTargetParams(w, s, 0.0, t,
        // h); 				measurePatternVar(measure, w, w, t,
        // measure->_wireCnt, "2");

        // fprintf(stdout, "%s\n", _commentLine);
        cnt++;
      }
    }
  }

  return cnt;
}

uint extRCModel::linesOver(uint wireCnt,
                           uint widthCnt,
                           uint spaceCnt,
                           uint dCnt,
                           uint metLevel)
{
  sprintf(_patternName, "Over%d", wireCnt);

  openCapLogFile();
  uint cnt = 0;

  extMeasure measure(NULL);
  measure._wireCnt = wireCnt;
  measure._3dFlag = true;
  measure._len = _len;
  measure._simVersion = _simVersion;

  for (uint met = 1; met < _layerCnt; met++) {
    if (metLevel > 0 && met != metLevel)
      continue;

    measure._met = met;
    computeTables(&measure, wireCnt, widthCnt, spaceCnt, dCnt);

    allocOverTable(&measure);

    for (uint underMet = 0; underMet < met; underMet++) {
      int metDist = met - underMet;
      if (metDist > _maxLevelDist && underMet > 0)
        continue;

      measure.setMets(met, underMet, -1);
      measure._capTable = _capOver;

      uint cnt1 = measureWithVar(&measure);

      logger_->info(RCX,
                    236,
                    "Finished {} measurements for pattern M{}_over_M{}",
                    cnt1,
                    met,
                    underMet);

      cnt += cnt1;
    }
  }
  logger_->info(
      RCX, 237, "Finished {} measurements for pattern MET_OVER_MET", cnt);

  closeCapLogFile();
  return cnt;
}
uint extRCModel::linesDiagUnder(uint wireCnt,
                                uint widthCnt,
                                uint spaceCnt,
                                uint dCnt,
                                uint metLevel)
{
  _diag = true;
  sprintf(_patternName, "UnderDiag%d", wireCnt);
  openCapLogFile();
  uint cnt = 0;

  extMeasure measure(NULL);
  measure._wireCnt = wireCnt;
  measure._3dFlag = true;
  measure._len = _len;
  measure._simVersion = _simVersion;

  measure._diag = true;
  measure._diagModel = _diagModel;

  for (uint met = 1; met < _layerCnt - 1; met++) {
    if (metLevel > 0 && met != metLevel)
      continue;

    measure._met = met;
    measure._overMet = met;
    computeTables(&measure, wireCnt, widthCnt, spaceCnt, dCnt);

    allocDiagUnderTable(&measure);

    for (uint overMet = met + 1; overMet < met + 5 && overMet < _layerCnt;
         overMet++) {  // the max overMet need to be the same as in functions
                       // writeRulesDiagUnder readRulesDiagUnder

      if (overMet - met > _maxLevelDist)
        continue;

      measure.setMets(met, 0, overMet);
      uint cnt1 = 0;
      if (_diagModel == 2) {
        getDiagTables(&measure, widthCnt, spaceCnt);
        setDiagUnderTables(&measure);
        measure._capTable = _capDiagUnder;
        cnt1 = measureDiagWithVar(&measure);
      } else if (_diagModel == 1) {
        measure._capTable = _capDiagUnder;
        cnt1 = measureWithVar(&measure);
      }

      logger_->info(RCX,
                    227,
                    "Finished {} measurements for pattern M{}_diagUnder_M{}",
                    cnt1,
                    met,
                    overMet);

      cnt += cnt1;
    }
  }
  logger_->info(
      RCX, 247, "Finished {} measurements for pattern MET_DIAGUNDER_MET", cnt);

  closeCapLogFile();
  return cnt;
}

uint extRCModel::measureDiagWithVar(extMeasure* measure)
{
  uint cnt = 0;
  int met = measure->_met;
  extVariation* xvar = _process->getVariation(measure->_met);

  extConductor* cond = _process->getConductor(met);
  double t = cond->_thickness;
  double h = cond->_height;
  double ro = cond->_p;
  double res = 0.0;
  double top_ext = cond->_top_ext;
  double bot_ext = cond->_bot_ext;

  //        extMasterConductor* m= _process->getMasterConductor(met);
  if (top_ext != 0.0 || bot_ext != 0.0) {
    xvar = NULL;
    measure->_metExtFlag = true;
  }

  for (uint dIndex = 0; dIndex < measure->_dataTable.getCnt(); dIndex++) {
    double r = measure->_dataTable.get(dIndex);  // layout
    measure->_rIndex = dIndex;

    uint wcnt;
    uint scnt;
    uint dwcnt;
    uint dscnt;
    if (dIndex) {
      wcnt = measure->_widthTable.getCnt();
      scnt = measure->_spaceTable.getCnt();
      dwcnt = measure->_diagWidthTable0.getCnt();
      dscnt = measure->_diagSpaceTable0.getCnt();
    } else {
      wcnt = measure->_widthTable0.getCnt();
      scnt = measure->_spaceTable0.getCnt();
      dwcnt = measure->_diagWidthTable0.getCnt();
      dscnt = measure->_diagSpaceTable0.getCnt();
    }
    if (_diagModel == 2)
      scnt /= 2;

    for (uint wIndex = 0; wIndex < wcnt; wIndex++) {
      double w;
      if (!dIndex)
        w = measure->_widthTable0.get(wIndex);
      else
        w = measure->_widthTable.get(wIndex);  // layout
      measure->_wIndex = wIndex;
      for (uint dwIndex = 0; dwIndex < dwcnt; dwIndex++) {
        double dw;
        dw = measure->_diagWidthTable0.get(dwIndex);
        measure->_dwIndex = dwIndex;
        for (uint dsIndex = 0; dsIndex < dscnt; dsIndex++) {
          double ds;
          ds = measure->_diagSpaceTable0.get(dsIndex);
          measure->_dsIndex = dsIndex;
          for (uint sIndex = 0; sIndex < scnt; sIndex++) {
            double s;
            if (!dIndex)
              s = measure->_spaceTable0.get(sIndex);
            else
              s = measure->_spaceTable.get(sIndex);
            double top_width = w + 2 * top_ext;
            double top_widthR = w + 2 * top_ext;
            double bot_width = w + 2 * bot_ext;
            double thickness = t;
            double bot_widthR = w + 2 * bot_ext;
            double thicknessR = t;

            if (r == 0.0) {
              //                                        		top_width=
              //                                        w;
              //                                        top_widthR= w;
              if (xvar != NULL) {
                double a = xvar->getP(w);
                if (a != 0.0)
                  ro = a;
              }
              res = measureResistance(
                  measure, ro, top_widthR, bot_widthR, thicknessR);
            } else if (xvar != NULL && !_maxMinFlag) {
              uint ss;
              //							if
              //(sIndex < scnt-1)
              if (sIndex < scnt - 2)
                ss = sIndex;
              else
                //								ss
                //= scnt-2;
                ss = scnt - 3;
              top_width = xvar->getTopWidth(wIndex, ss);
              top_widthR = xvar->getTopWidthR(wIndex, ss);
              bot_width = xvar->getBottomWidth(w, dIndex - 1);
              bot_width = top_width - bot_width;
              thickness = xvar->getThickness(w, dIndex - 1);
              bot_widthR = xvar->getBottomWidthR(w, dIndex - 1);
              bot_widthR = top_widthR - bot_widthR;
              thicknessR = xvar->getThicknessR(w, dIndex - 1);
              double a = xvar->getP(w);
              if (a != 0.0)
                ro = a;
              res = measureResistance(
                  measure, ro, top_widthR, bot_widthR, thicknessR);
            } else if (_maxMinFlag && r == 1.0) {
              top_width = w - 2 * cond->_min_cw_del;
              thickness = t - cond->_min_ct_del;
              bot_width = top_width - 2 * thickness * cond->_min_ca;
              if (bot_width > w)
                bot_width = w;
              res = measureResistance(
                  measure, ro, top_widthR, bot_widthR, thicknessR);
            } else if (_maxMinFlag && r == 2.0) {
              top_width = w + 2 * cond->_max_cw_del;
              thickness = t + cond->_max_ct_del;
              bot_width = top_width - 2 * thickness * cond->_max_ca;
              if (bot_width < w)
                bot_width = w;
              res = measureResistance(
                  measure, ro, top_widthR, bot_widthR, thicknessR);
            } else if (measure->_thickVarFlag) {
              thickness *= 1 + r;
              thicknessR *= 1 + r;
            } else {
              continue;
            }
            measure->setTargetParams(w, s, r, t, h, dw, ds);
            measurePatternVar(measure,
                              top_width,
                              bot_width,
                              thickness,
                              measure->_wireCnt,
                              NULL,
                              res * 0.5);

            cnt++;
          }
        }
      }
    }
  }
  return cnt;
}
uint extRCModel::linesUnder(uint wireCnt,
                            uint widthCnt,
                            uint spaceCnt,
                            uint dCnt,
                            uint metLevel)
{
  sprintf(_patternName, "Under%d", wireCnt);
  openCapLogFile();
  uint cnt = 0;

  extMeasure measure(NULL);
  measure._wireCnt = wireCnt;
  measure._3dFlag = true;
  measure._len = _len;
  measure._simVersion = _simVersion;

  for (uint met = 1; met < _layerCnt; met++) {
    if (metLevel > 0 && met != metLevel)
      continue;

    measure._met = met;
    computeTables(&measure, wireCnt, widthCnt, spaceCnt, dCnt);

    allocUnderTable(&measure);

    for (uint overMet = met + 1; overMet < _layerCnt; overMet++) {
      int metDist = overMet - met;
      if (metDist > _maxLevelDist)
        continue;

      measure.setMets(met, 0, overMet);
      measure._capTable = _capUnder;

      uint cnt1 = measureWithVar(&measure);

      logger_->info(RCX,
                    248,
                    "Finished {} measurements for pattern M{}_under_M{}",
                    cnt1,
                    met,
                    overMet);

      cnt += cnt1;
    }
  }
  logger_->info(
      RCX, 410, "Finished {} measurements for pattern MET_UNDER_MET", cnt);

  closeCapLogFile();
  return cnt;
}

void extRCModel::setOptions(const char* topDir,
                            const char* pattern,
                            bool keepFile,
                            uint metLevel)
{
  _logFP = openFile("./", "rulesGen", ".log", "w");
  strcpy(_topDir, topDir);
  strcpy(_patternName, pattern);

  _writeFiles = true;
  _readSolver = true;
  _runSolver = true;

  if (metLevel)
    _metLevel = metLevel;
#ifdef _WIN32
  _runSolver = false;
#endif
}
uint extRCModel::linesOverUnder(uint wireCnt,
                                uint widthCnt,
                                uint spaceCnt,
                                uint dCnt,
                                uint metLevel)
{
  sprintf(_patternName, "OverUnder%d", wireCnt);
  openCapLogFile();
  uint cnt = 0;

  extMeasure measure(NULL);
  measure._wireCnt = wireCnt;
  measure._3dFlag = true;
  measure._len = _len;
  measure._simVersion = _simVersion;

  for (uint met = 1; met < _layerCnt - 1; met++) {
    if (metLevel > 0 && met != metLevel)
      continue;
    measure._met = met;
    computeTables(&measure, wireCnt, widthCnt, spaceCnt, dCnt);

    allocOverUnderTable(&measure);

    for (uint underMet = 1; underMet < met; underMet++) {
      int metDist = met - underMet;
      if (metDist > _maxLevelDist)
        continue;
      for (uint overMet = met + 1; overMet < _layerCnt; overMet++) {
        int metDist = overMet - met;
        if (metDist > _maxLevelDist)
          continue;
        measure.setMets(met, underMet, overMet);
        measure._capTable = _capUnder;

        uint cnt1 = measureWithVar(&measure);

        logger_->info(
            RCX,
            233,
            "Finished {} measurements for pattern M{}_over_M{}_under_M{}",
            cnt1,
            met,
            underMet,
            overMet);

        cnt += cnt1;
      }
    }
  }
  logger_->info(
      RCX, 234, "Finished {} measurements for pattern MET_UNDER_MET", cnt);

  closeCapLogFile();
  return cnt;
}
uint extMain::rulesGen(const char* name,
                       const char* topDir,
                       const char* rulesFile,
                       int pattern,
                       bool keepFile,
                       int wLen,
                       int version,
                       bool win)
{
  extRCModel* m = _modelTable->get(0);

  m->setOptions(topDir, name, keepFile);
  m->_winDirFlat = win;
  m->_len = wLen;
  m->_simVersion = version;
  m->_maxLevelDist = 999;

  if ((pattern > 0) && (pattern <= 9))
    m->linesOver(pattern, 20, 20, 20);
  else if ((pattern > 10) && (pattern <= 19))
    m->linesUnder(pattern - 10, 20, 20, 20);
  else if ((pattern > 20) && (pattern <= 29))
    m->linesOverUnder(pattern - 20, 20, 20, 20);
  else if ((pattern > 30) && (pattern <= 39)) {
    m->setDiagModel(1);
    m->linesDiagUnder(pattern - 30, 20, 20, 20);
  } else if ((pattern > 40) && (pattern <= 49)) {
    m->setDiagModel(2);
    m->linesDiagUnder(pattern - 40, 20, 20, 20);
  } else if (pattern > 100) {
    uint wireCnt = pattern % 10;
    uint widthCnt = 10;
    uint spaceCnt = 10;
    uint dCnt = 20;
    if (m->_simVersion > 0) {
      widthCnt = 1;
      spaceCnt = 10;
      dCnt = 1;
      m->setDiagModel(0);
      m->_maxLevelDist = 4;
      if (m->_simVersion >= 2)
        m->_maxLevelDist = 4;
    }
    uint cnt1 = m->linesOver(wireCnt, widthCnt, spaceCnt, dCnt);
    cnt1 += m->linesUnder(wireCnt, widthCnt, spaceCnt, dCnt);
    cnt1 += m->linesOverUnder(wireCnt, widthCnt, spaceCnt, dCnt);
    if (pattern > 200)
      m->setDiagModel(2);
    else
      m->setDiagModel(1);
    cnt1 += m->linesDiagUnder(wireCnt, widthCnt, spaceCnt, dCnt);

    logger_->info(RCX, 232, "Finished {} patterns", cnt1);
  }
  m->closeFiles();
  if (!(pattern > 100))
    m->writeRules((char*) rulesFile, false);
  return 0;
}
void extRCModel::writeRuleWires(FILE* fp, extMeasure* measure, uint wireCnt)
{
  extMasterConductor* m = _process->getMasterConductor(measure->_met);
  double minWidth = _process->getConductor(measure->_met)->_min_width;
  double minSpace = _process->getConductor(measure->_met)->_min_spacing;
  double top_ext = _process->getConductor(measure->_met)->_top_ext;
  double pitch;
  if (measure->_diag && _diagModel == 1) {
    //		pitch= measure->_topWidth + 2*minSpace;
    if (measure->_metExtFlag)
      pitch = measure->_topWidth - 2 * top_ext + minSpace;
    else
      pitch = measure->_topWidth + minSpace;
  } else {
    if (measure->_metExtFlag)
      pitch = measure->_topWidth - 2 * top_ext + measure->_seff;
    else
      pitch = measure->_topWidth + measure->_seff;
  }
  double min_pitch = minWidth + minSpace;

  uint n = wireCnt / 2;  // ASSUME odd number of wires, 2 will also work
  double orig = 0.0;
  double x;
  uint ii;

  if (!measure->_plate) {
    // assume origin = (0,0)
    x = -min_pitch * (n - 1) - pitch - 0.5 * measure->_topWidth - top_ext
        + orig;
    for (ii = 0; ii < n - 1; ii++) {
      m->writeRaphaelConformalPoly(fp, minWidth, x, _process);
      m->writeRaphaelPoly(fp, ii + 1, minWidth, x, 0.0, _process);
      x += min_pitch;
    }
    x += 0.5 * measure->_topWidth;
    m->writeRaphaelConformalPoly(fp, 0.0, x, _process);
    m->writeRaphaelPoly(fp, n, x, 0.0);

    m->writeRaphaelConformalPoly(fp, 0.0, orig, _process);
    m->writeRaphaelPoly(fp, n + 1, orig, 1.0);
    x = orig + pitch;
    m->writeRaphaelConformalPoly(fp, 0.0, x, _process);
    m->writeRaphaelPoly(fp, n + 2, x, 0.0);

    x += 0.5 * measure->_topWidth - top_ext + minSpace;
    for (uint jj = n + 2; jj < wireCnt; jj++) {
      m->writeRaphaelConformalPoly(fp, minWidth, x, _process);
      m->writeRaphaelPoly(fp, jj + 1, minWidth, x, 0.0, _process);
      x += min_pitch;
    }
  } else {
    m->writeRaphaelConformalPoly(fp, 100, -50, _process);
    m->writeRaphaelPoly(fp, n + 1, 100, -50, 1.0, _process);
  }

  if (!measure->_diag)
    return;
  int met;
  if (measure->_overMet > 0)
    met = measure->_overMet;
  else if (measure->_underMet > 0)
    met = measure->_underMet;
  else
    return;

  m = _process->getMasterConductor(met);
  minWidth = _process->getConductor(met)->_min_width;
  minSpace = _process->getConductor(met)->_min_spacing;
  min_pitch = minWidth + minSpace;
  pitch = measure->_w2_m + measure->_s2_m;

  if (_diagModel == 2) {
    if (measure->_seff != 99) {
      x = orig - measure->_s2_m - min_pitch * n - 0.5 * measure->_w2_m;
      for (ii = 0; ii < n; ii++) {
        m->writeRaphaelConformalPoly(fp, minWidth, x, _process);
        m->writeRaphaelPoly(fp, ii + 1, minWidth, x, 0.0, _process);

        x += min_pitch;
      }
      m->writeRaphaelConformalPoly(fp, measure->_w2_m, x, _process);
      m->writeRaphaelPoly(fp, ii + 1, measure->_w2_m, x, 0.0, _process);
    } else {
      x = orig - measure->_s2_m - 0.5 * measure->_w2_m;
      m->writeRaphaelConformalPoly(fp, measure->_w2_m, x, _process);
      m->writeRaphaelPoly(fp, n + 1, measure->_w2_m, x, 0.0, _process);
    }
  }
  if (_diagModel == 1) {
    x = orig - measure->_seff - min_pitch * n - 0.5 * minWidth;
    for (ii = 0; ii < n + 1; ii++) {
      m->writeRaphaelConformalPoly(fp, minWidth, x, _process);
      m->writeRaphaelPoly(fp, ii + 1, minWidth, x, 0.0, _process);
      x += min_pitch;
    }
  }
}
double extRCModel::writeWirePatterns(FILE* fp,
                                     extMeasure* measure,
                                     uint wireCnt,
                                     double height_offset,
                                     double& len,
                                     double& max_x)
{
  extMasterConductor* m = _process->getMasterConductor(measure->_met);
  extMasterConductor* mOver = NULL;
  if (_diagModel > 0)
    mOver = _process->getMasterConductor(measure->_overMet);

  double targetWidth = measure->_topWidth;
  double targetPitch = measure->_topWidth + measure->_seff;
  double minWidth = _process->getConductor(measure->_met)->_min_width;
  double minSpace = _process->getConductor(measure->_met)->_min_spacing;
  double min_pitch = minWidth + minSpace;

  len = measure->_len * minWidth;  // HEIGHT param
  if (len <= 0)
    len = 10 * minWidth;

  // len *= 0.001;

  // Assumption -- odd wireCnt, >1
  if (wireCnt == 0)
    return 0.0;
  if (wireCnt % 2 > 0)  // should be odd
    wireCnt--;

  int n = wireCnt / 2;
  double orig = 0.0;
  double x = orig;

  uint cnt = 1;

  double xd[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  double X0 = x - targetPitch;  // next neighbor spacing
  // double W0= targetWidth; // next neighbor width
  xd[1] = X0 - min_pitch;
  double min_x = xd[1];
  for (int ii = 2; ii < n; ii++) {
    xd[ii] = xd[ii - 1] - min_pitch;  // next over neighbor spacing
    if (min_x > xd[ii])
      min_x = xd[ii];
  }
  for (int ii = n - 1; ii > 0; ii--) {
    m->writeWire3D(fp, cnt++, xd[ii], minWidth, len, height_offset, 0.0);
    max_x = xd[ii] + minWidth;
  }
  if (n > 1)
    m->writeWire3D(fp, cnt++, X0, targetWidth, len, height_offset, 0.0);

  if (min_x > x)
    min_x = x;

  m->writeWire3D(
      fp, cnt++, x, targetWidth, len, height_offset, 1.0);  // Wire on focues
  double center_diag_x = x;
  max_x = x + targetWidth;

  // ----- DKF 09162024 Single Wire NOT COVERED for now -- needs a flag
  /*
  if (n == 1) {
    if (_diagModel > 0) {
      double minWidthDiag=
  _process->getConductor(measure->_overMet)->_min_width; double minSpaceDiag=
  _process->getConductor(measure->_overMet)->_min_spacing;

      fprintf(fp, "\n");
      mOver->writeWire3D(fp, cnt, x, minWidthDiag, len, height_offset, 0.0);
    }
    return min_x;
  }
  */

  double xu[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  xu[0] = x + targetPitch;  // next neighbor spacing
  m->writeWire3D(fp, cnt++, xu[0], targetWidth, len, height_offset, 0.0);
  xu[1] += xu[0] + targetWidth + minSpace;
  for (int ii = 2; ii < n; ii++) {
    xu[ii] = xu[ii - 1] + min_pitch;  // context: next over neighbor spacing
  }
  for (int ii = 1; ii < n; ii++) {
    m->writeWire3D(fp, cnt++, xu[ii], minWidth, len, height_offset, 0.0);
    max_x = xu[ii] + minWidth;
  }
  if (_diagModel > 0) {
    center_diag_x += measure->_s2_m;
    double minWidthDiag = _process->getConductor(measure->_overMet)->_min_width;
    double minSpaceDiag
        = _process->getConductor(measure->_overMet)->_min_spacing;

    fprintf(fp, "\n");
    mOver->writeWire3D(
        fp, cnt++, center_diag_x, minWidthDiag, len, height_offset, 0.0);
    mOver->writeWire3D(fp,
                       cnt++,
                       center_diag_x + minWidthDiag + minSpaceDiag,
                       minWidthDiag,
                       len,
                       height_offset,
                       0.0);
  }
  return min_x;
}
double extRCModel::writeWirePatterns_w3(FILE* fp,
                                        extMeasure* measure,
                                        uint wireCnt,
                                        double height_offset,
                                        double& len,
                                        double& max_x)
{
  extMasterConductor* m = _process->getMasterConductor(measure->_met);
  extMasterConductor* mOver = NULL;
  if (_diagModel > 0)
    mOver = _process->getMasterConductor(measure->_overMet);

  double targetWidth = measure->_topWidth;
  double targetPitch = measure->_topWidth + measure->_seff;
  double minWidth = _process->getConductor(measure->_met)->_min_width;
  // DELETE double minSpace =
  // _process->getConductor(measure->_met)->_min_spacing; DELETE double
  // min_pitch = minWidth + minSpace;

  len = measure->_len * minWidth;  // HEIGHT param
  if (len <= 0)
    len = 10 * minWidth;

  // len *= 0.001;

  // Assumption -- odd wireCnt, >1
  if (wireCnt == 0)
    return 0.0;
  if (wireCnt % 2 > 0)  // should be odd
    wireCnt--;

  // int n = wireCnt / 2;
  double orig = 0.0;
  double x = orig;
  double X0 = x - targetPitch;  // next neighbor
  double X1 = x + targetPitch;
  uint cnt = 1;
  m->writeWire3D(
      fp, cnt++, X0, targetWidth, len, height_offset, 0);  // Wire on focues
  m->writeWire3D(
      fp, cnt++, x, targetWidth, len, height_offset, 1.0);  // Wire on focues
  m->writeWire3D(
      fp, cnt++, X1, targetWidth, len, height_offset, 0);  // Wire on focues
  double min_x = X0;
  double center_diag_x = x;
  max_x = X1 + minWidth;

  if (_diagModel > 0) {
    center_diag_x += measure->_s2_m;
    double minWidthDiag = _process->getConductor(measure->_overMet)->_min_width;
    double minSpaceDiag
        = _process->getConductor(measure->_overMet)->_min_spacing;

    fprintf(fp, "\n");
    mOver->writeWire3D(
        fp, cnt++, center_diag_x, minWidthDiag, len, height_offset, 0.0);
    mOver->writeWire3D(fp,
                       cnt++,
                       center_diag_x + minWidthDiag + minSpaceDiag,
                       minWidthDiag,
                       len,
                       height_offset,
                       0.0);
  }
  return min_x;
}

/* see version 2 above -- 5/27/23
void extRCModel::writeRuleWires_3D(FILE* fp, extMeasure* measure,
                                   uint wireCnt) {
  extMasterConductor* m = _process->getMasterConductor(measure->_met);
  double pitch = measure->_topWidth + measure->_seff;
  double minWidth = _process->getConductor(measure->_met)->_min_width;
  double minSpace = _process->getConductor(measure->_met)->_min_spacing;
  double min_pitch = minWidth + minSpace;

  uint n = wireCnt / 2;  // ASSUME odd number of wires, 2 will also work
  double orig = 0.0;

  // assume origin = (0,0)
  double x = -min_pitch * (n - 1) - pitch - 0.5 * measure->_topWidth + orig;
  for (uint ii = 0; ii < n - 1; ii++) {
    m->writeRaphaelPoly3D(fp, ii + 1, minWidth, measure->_len * 0.001, 0.0);
    x += min_pitch;
  }
  x += 0.5 * measure->_topWidth;
  m->writeRaphaelPoly(fp, n, x, measure->_len * 0.001, 0.0);

  m->writeRaphaelPoly3D(fp, n + 1, orig, measure->_len * 0.001, 1.0);
  x = orig + pitch;
  m->writeRaphaelPoly3D(fp, n + 2, measure->_len * 0.001, x, 0.0);

  x += 0.5 * measure->_topWidth + minSpace;
  for (uint jj = n + 2; jj < wireCnt; jj++) {
    m->writeRaphaelPoly3D(fp, jj + 1, minWidth, measure->_len * 0.001, x, 0.0);
    x += min_pitch;
  }
}
*/
void extRCModel::writeWires2_3D(FILE* fp, extMeasure* measure, uint wireCnt)
{
  extMasterConductor* m = _process->getMasterConductor(measure->_met);
  double pitch = measure->_topWidth + measure->_seff;
  double min_pitch = 0.001 * (measure->_minWidth + measure->_minSpace);

  uint n = wireCnt / 2;  // ASSUME odd number of wires, 2 will also work
  double orig = 0.0;

  // assume origin = (0,0)
  double x = -min_pitch * (n - 1) - pitch - 0.5 * measure->_topWidth + orig;
  for (uint ii = 0; ii < n - 1; ii++) {
    m->writeRaphaelPoly3D(
        fp, ii + 1, 0.001 * measure->_minWidth, measure->_len * 0.001, 0.0);
    x += min_pitch;
  }
  x += 0.5 * measure->_topWidth;
  m->writeRaphaelPoly(fp, n, x, measure->_len * 0.001, 0.0);

  m->writeRaphaelPoly3D(fp, n + 1, orig, measure->_len * 0.001, 1.0);
  x = 0.5 * measure->_w_m + measure->_s2_m;
  m->writeRaphaelPoly3D(
      fp, n + 2, measure->_w2_m, measure->_len * 0.001, x, 0.0);

  x = orig + 0.5 * measure->_topWidth + measure->_w2_m + measure->_s2_m
      + 0.001 * measure->_minSpace;
  for (uint jj = n + 2; jj < wireCnt; jj++) {
    m->writeRaphaelPoly3D(
        fp, jj + 1, 0.001 * measure->_minWidth, measure->_len * 0.001, x, 0.0);
    x += min_pitch;
  }
}
void extRCModel::writeWires(FILE* fp, extMeasure* measure, uint wireCnt)
{
  extMasterConductor* m = _process->getMasterConductor(measure->_met);
  double pitch = measure->_topWidth + measure->_seff;

  uint n = wireCnt / 2;  // ASSUME odd number of wires, 2 will also work

  if (_commentFlag)
    fprintf(fp, "%s\n", _commentLine);

  // assume origin = (0,0)
  double x = -pitch * n;
  m->writeRaphaelPoly(fp, 1, x, 1.0);

  for (uint ii = 1; ii < wireCnt; ii++) {
    x += pitch;
    m->writeRaphaelPoly(fp, ii + 1, x, 0.0);
  }

  fprintf(fp, "\nOPTIONS SET_GRID=10000;\n\n");
  fprintf(fp, "POTENTIAL\n");
  /*
          fprintf(fp, "CAPACITANCE ");
          for (uint kk= 1; kk<=wireCnt; kk++)
                  m->writeBoxName(fp, kk);
          fprintf(fp, " \n");
  */
}
bool extRCModel::measurePatternVar_3D(extMeasure* m,
                                      double top_width,
                                      double bot_width,
                                      double thickness,
                                      uint wireCnt,
                                      char* wiresNameSuffix,
                                      double res)
{
  m->setEffParams(top_width, bot_width, thickness);
  double thicknessChange
      = _process->adjustMasterLayersForHeight(m->_met, thickness);

  // _process->getMasterConductor(m->_met)->reset(m->_heff, top_width,
  // bot_width, thickness);
  _process->getMasterConductor(m->_met)->resetWidth(top_width, bot_width);

  mkFileNames(m, wiresNameSuffix);

  printCommentLine('#', m);
  fprintf(_logFP, "%s\n", _commentLine);
  fprintf(_logFP, "%c %g thicknessChange\n", '$', thicknessChange);
  fflush(_logFP);

  FILE* wfp = mkPatternFile();

  if (wfp == NULL)
    return false;  // should be an exception!! and return!

  double maxHeight
      = _process->adjustMasterDielectricsForHeight(m->_met, thicknessChange);
  maxHeight *= 1.2;

  double len = top_width * _len;
  if (len <= 0)
    len = top_width * 10 * 0.001;
  //                        double W = (m->_ur[m->_dir] -
  // m->_ll[m->_dir])*10;
  double W = 40;

  bool apply_height_offset = _simVersion > 1;

  double height_low = 0;
  double height_ceiling = 0;
  if (_diagModel > 0) {
    height_low = _process->writeProcessAndGroundPlanes(wfp,
                                                       "GND",
                                                       0,
                                                       0,
                                                       -30.0,
                                                       60.0,
                                                       len,
                                                       maxHeight,
                                                       W,
                                                       apply_height_offset,
                                                       height_ceiling);
  } else {
    height_low = _process->writeProcessAndGroundPlanes(wfp,
                                                       "GND",
                                                       m->_underMet,
                                                       m->_overMet,
                                                       -30.0,
                                                       60.0,
                                                       len,
                                                       maxHeight,
                                                       W,
                                                       apply_height_offset,
                                                       height_ceiling);
  }
  if (_simVersion < 2)
    height_low = 0;

  if (_commentFlag)
    fprintf(wfp, "%s\n", _commentLine);

  double len1;
  double X1;
  double X0 = 0;
  if (wireCnt == 5)
    X0 = writeWirePatterns(wfp, m, wireCnt, height_low, len1, X1);
  else
    X0 = writeWirePatterns_w3(wfp, m, wireCnt, height_low, len1, X1);

  fprintf(wfp,
          "\nWINDOW_BBOX  LL %6.3f %6.3f UR %6.3f %6.3f LENGTH %6.3f\n",
          X0,
          0.0,
          X1,
          height_ceiling,
          len1);
  double DX = X1 - X0;
  // fprintf(wfp, "\nSIM_WIN_EXT  LL %6.3f %6.3f UR %6.3f %6.3f LENGTH %6.3f
  // %6.3f\n", -DX/2 , 0.0, DX/2, 0.0, -len1/2, len1/2);
  fprintf(wfp,
          "\nSIM_WIN_EXT  LL %6.3f %6.3f UR %6.3f %6.3f LENGTH %6.3f %6.3f\n",
          -DX,
          0.0,
          DX,
          0.0,
          0.0,
          len1);

  fprintf(_filesFP, "%s/wires\n", _wireDirName);

  fclose(wfp);
  return true;
}
}  // namespace rcx
