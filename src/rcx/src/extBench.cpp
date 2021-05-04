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

#include "rcx/extRCap.h"
#include "rcx/extSpef.h"
#include "rcx/extprocess.h"

#ifdef _WIN32
#include "direct.h"
#endif

#include <map>
#include <vector>

#include "utl/Logger.h"

namespace rcx {

using utl::RCX;

using odb::dbBlock;
using odb::dbBox;
using odb::dbChip;
using odb::dbNet;
using odb::dbRSeg;
using odb::dbSet;
using odb::dbShape;
using odb::dbTechLayer;
using odb::dbTechLayerRule;
using odb::dbTechNonDefaultRule;
using odb::dbWire;
using odb::dbWireShapeItr;
using odb::ISdb;
using odb::Rect;
using odb::ZPtr;

extMainOptions::extMainOptions()
{
  _met = -1;
  _underMet = -1;
  _overMet = -1;

  _varFlag = false;
  _3dFlag = false;
}
uint extRCModel::benchWithVar_density(extMainOptions* opt, extMeasure* measure)
{
  if (opt->_db_only)
    return benchDB_WS(opt, measure);
  if (opt->_listsFlag)
    return benchWithVar_lists(opt, measure);

  uint cnt = 0;
  int met = measure->_met;
  extVariation* xvar = _process->getVariation(measure->_met);

  extConductor* cond = _process->getConductor(met);
  double t = cond->_thickness;
  double h = cond->_height;
  double ro = cond->_p;

  //	extMasterConductor* m= _process->getMasterConductor(met);

  for (uint ii = 0; ii < opt->_widthTable.getCnt(); ii++) {
    double w = 0.001 * measure->_minWidth * opt->_widthTable.get(ii);  // layout
    measure->_wIndex = measure->_widthTable.findNextBiggestIndex(w);   // layout
    //		double ww= measure->_widthTable.get(measure->_wIndex);

    for (uint jj = 0; jj < opt->_spaceTable.getCnt(); jj++) {
      double s
          = 0.001 * measure->_minSpace * opt->_spaceTable.get(jj);  // layout
      measure->_sIndex = measure->_widthTable.findNextBiggestIndex(
          s);  // layout
               //			double ss=
      // measure->_spaceTable.get(measure->_sIndex); // layout

      for (uint kk = 0; kk < opt->_densityTable.getCnt(); kk++) {
        double r = opt->_densityTable.get(kk);  // layout
        measure->_rIndex = measure->_dataTable.findNextBiggestIndex(
            r);  // layout
                 //				double rr=
        // measure->_dataTable.get(measure->_rIndex); // layout

        double top_width = w;
        double top_widthR = w;
        double bot_width = w;
        double thickness = t;
        double bot_widthR = w;
        double thicknessR = t;

        if (r <= 0.0) {
          top_width = w;
          top_widthR = w;
        } else if (xvar != NULL) {
          top_width = xvar->getTopWidth(measure->_wIndex, measure->_sIndex);
          top_widthR = xvar->getTopWidthR(measure->_wIndex, measure->_sIndex);

          bot_width = xvar->getBottomWidth(w, measure->_rIndex);
          bot_width = top_width - bot_width;

          thickness = xvar->getThickness(w, measure->_rIndex);

          bot_widthR = xvar->getBottomWidthR(w, measure->_rIndex);
          bot_widthR = top_widthR - bot_widthR;

          thicknessR = xvar->getThicknessR(w, measure->_rIndex);
        } else {
          continue;
        }

        measure->setTargetParams(w, s, r, t, h);
        measureResistance(measure, ro, top_widthR, bot_widthR, thicknessR);
        measurePatternVar(
            measure, top_width, bot_width, thickness, measure->_wireCnt, NULL);

        cnt++;
      }
    }
  }
  return cnt;
}
uint extRCModel::benchWithVar_lists(extMainOptions* opt, extMeasure* measure)
{
  Ath__array1D<double>* wTable = &opt->_widthTable;
  Ath__array1D<double>* sTable = &opt->_spaceTable;
  Ath__array1D<double>* thTable = &opt->_thicknessTable;
  Ath__array1D<double>* gTable = &opt->_gridTable;

  uint cnt = 0;
  int met = measure->_met;
  //	extVariation *xvar= _process->getVariation(measure->_met);

  extConductor* cond = _process->getConductor(met);
  double t = cond->_thickness;
  double h = cond->_height;
  double ro = cond->_p;

  dbTechLayer* layer = opt->_tech->findRoutingLayer(met);

  double minWidth = 0.001 * layer->getWidth();
  ;
  double pitch = 0.001 * layer->getPitch();
  //	double spacing= pitch-minWidth;

  if (opt->_default_lef_rules) {  // minWidth, minSpacing, minThickness, pitch
                                  // multiplied by grid_list
    wTable->resetCnt();
    sTable->resetCnt();
    wTable->add(minWidth);

    for (uint ii = 0; ii < gTable->getCnt(); ii++) {
      double s = minWidth + pitch * (gTable->get(ii) - 1);
      sTable->add(s);
    }
  } else if (opt->_nondefault_lef_rules) {
    wTable->resetCnt();
    sTable->resetCnt();
    dbSet<dbTechNonDefaultRule> nd_rules = opt->_tech->getNonDefaultRules();
    dbSet<dbTechNonDefaultRule>::iterator nditr;
    dbTechLayerRule* tst_rule;
    //		dbTechNonDefaultRule  *wdth_rule = NULL;

    for (nditr = nd_rules.begin(); nditr != nd_rules.end(); ++nditr) {
      tst_rule = (*nditr)->getLayerRule(layer);
      if (tst_rule == NULL)
        continue;

      double w = tst_rule->getWidth();
      double s = tst_rule->getSpacing();
      wTable->add(w);
      sTable->add(s);
    }
  }

  //	extMasterConductor* m= _process->getMasterConductor(met);

  for (uint ii = 0; ii < wTable->getCnt(); ii++) {
    double w = wTable->get(ii);  // layout
    for (uint iii = 0; iii < wTable->getCnt(); iii++) {
      double w2 = wTable->get(iii);
      for (uint jj = 0; jj < sTable->getCnt(); jj++) {
        double s = sTable->get(jj);  // layout
        for (uint jjj = 0; jjj < sTable->getCnt(); jjj++) {
          double s2 = sTable->get(jjj);

          for (uint kk = 0; kk < thTable->getCnt(); kk++) {
            double tt = thTable->get(kk);  // layout
            if (!opt->_thListFlag)         // multiplier
              tt *= t;

            double top_width = w;
            double top_widthR = w;

            double bot_width = w;
            double bot_widthR = w;

            double thickness = tt;
            double thicknessR = tt;

            measure->setTargetParams(w, s, 0.0, t, h, w2, s2);
            measureResistance(measure, ro, top_widthR, bot_widthR, thicknessR);
            measurePatternVar(measure,
                              top_width,
                              bot_width,
                              thickness,
                              measure->_wireCnt,
                              NULL);

            cnt++;
          }
        }
      }
    }
  }
  return cnt;
}
uint extRCModel::linesOverBench(extMainOptions* opt)
{
  if (opt->_met == 0)
    return 0;

  extMeasure measure;
  measure.updateForBench(opt, _extMain);
  measure._diag = false;

  sprintf(_patternName, "O%d", opt->_wireCnt + 1);

  // openCapLogFile();
  uint cnt = 0;

  for (int met = 1; met <= (int) _layerCnt; met++) {
    if (met > opt->_met_cnt)
      continue;
    if ((opt->_met > 0) && (opt->_met != met))
      continue;

    measure._met = met;

    if (!opt->_db_only)
      computeTables(&measure, opt->_wireCnt + 1, 1000, 1000, 1000);

    uint patternSep = measure.initWS_box(opt, 20);

    for (int underMet = 0; underMet < met; underMet++) {
      if ((opt->_underMet > 0) && (opt->_underMet != underMet))
        continue;

      if (met - underMet > (int) opt->_underDist)
        continue;

      measure.setMets(met, underMet, -1);

      uint cnt1 = benchWithVar_density(opt, &measure);

      cnt += cnt1;
      measure._ur[measure._dir] += patternSep;

      if (opt->_underMet == 0 && !opt->_gen_def_patterns)
        break;
    }
    opt->_ur[0] = MAX(opt->_ur[0], measure._ur[0]);
    opt->_ur[1] = MAX(opt->_ur[1], measure._ur[1]);
  }

  logger_->info(
      RCX, 55, "Finished {} bench measurements for pattern MET_OVER_MET", cnt);

  // closeCapLogFile();
  return cnt;
}

uint extRCModel::linesUnderBench(extMainOptions* opt)
{
  if (opt->_overMet == 0)
    return 0;

  extMeasure measure;
  measure.updateForBench(opt, _extMain);
  measure._diag = false;

  uint patternSep = 1000;

  sprintf(_patternName, "U%d", opt->_wireCnt + 1);
  // openCapLogFile();
  uint cnt = 0;

  for (int met = 1; met < (int) _layerCnt; met++) {
    if (met > opt->_met_cnt)
      continue;
    if ((opt->_met > 0) && (opt->_met != met))
      continue;

    measure._met = met;

    if (!opt->_db_only)
      computeTables(&measure, opt->_wireCnt + 1, 1000, 1000, 1000);

    patternSep = measure.initWS_box(opt, 20);

    for (int overMet = met + 1; overMet <= (int) _layerCnt; overMet++) {
      if (overMet > opt->_met_cnt)
        continue;

      if ((opt->_overMet > 0) && (opt->_overMet != overMet))
        continue;

      if (overMet - met > (int) opt->_overDist)
        continue;

      measure.setMets(met, 0, overMet);

      uint cnt1 = benchWithVar_density(opt, &measure);

      cnt += cnt1;
      measure._ur[measure._dir] += patternSep;
    }
    opt->_ur[0] = MAX(opt->_ur[0], measure._ur[0]);
    opt->_ur[1] = MAX(opt->_ur[1], measure._ur[1]);
  }
  logger_->info(
      RCX, 57, "Finished {} bench measurements for pattern MET_UNDER_MET", cnt);

  // closeCapLogFile();
  return cnt;
}
uint extRCModel::linesDiagUnderBench(extMainOptions* opt)
{
  if (opt->_overMet == 0)
    return 0;

  extMeasure measure;
  measure.updateForBench(opt, _extMain);
  measure._diag = true;

  uint patternSep = 1000;

  sprintf(_patternName, "DU%d", opt->_wireCnt + 1);
  // openCapLogFile();
  uint cnt = 0;

  for (int met = 1; met < (int) _layerCnt; met++) {
    if (met > opt->_met_cnt)
      continue;

    if ((opt->_met > 0) && (opt->_met != met))
      continue;

    measure._met = met;
    if (!opt->_db_only)
      computeTables(&measure, opt->_wireCnt + 1, 1000, 1000, 1000);

    patternSep = measure.initWS_box(opt, 20);

    for (int overMet = met + 1; overMet < met + 5 && overMet <= (int) _layerCnt;
         overMet++) {
      if (overMet > opt->_met_cnt)
        continue;

      if ((opt->_overMet > 0) && (opt->_overMet != overMet))
        continue;

      if (overMet - met > (int) opt->_overDist)
        continue;

      measure.setMets(met, 0, overMet);

      uint cnt1 = benchWithVar_density(opt, &measure);

      cnt += cnt1;
      measure._ur[measure._dir] += patternSep;
    }
    opt->_ur[0] = MAX(opt->_ur[0], measure._ur[0]);
    opt->_ur[1] = MAX(opt->_ur[1], measure._ur[1]);
  }

  logger_->info(RCX,
                58,
                "Finished {} bench measurements for pattern MET_DIAGUNDER_MET",
                cnt);

  // closeCapLogFile();
  return cnt;
}
uint extRCModel::linesOverUnderBench(extMainOptions* opt)
{
  if (opt->_overMet == 0)
    return 0;

  extMeasure measure;
  measure.updateForBench(opt, _extMain);
  measure._diag = false;

  uint patternSep = 1000;

  sprintf(_patternName, "OU%d", opt->_wireCnt + 1);
  // openCapLogFile();
  uint cnt = 0;

  for (int met = 1; met <= (int) _layerCnt - 1; met++) {
    if (met > opt->_met_cnt)
      continue;
    if ((opt->_met > 0) && (opt->_met != met))
      continue;

    measure._met = met;

    if (!opt->_db_only)
      computeTables(&measure, opt->_wireCnt + 1, 1000, 1000, 1000);

    patternSep = measure.initWS_box(opt, 20);

    for (int underMet = 1; underMet < met; underMet++) {
      if (met - underMet > (int) opt->_underDist)
        continue;
      if ((opt->_underMet > 0) && ((int) opt->_underMet != underMet))
        continue;

      for (uint overMet = met + 1; overMet <= _layerCnt; overMet++) {
        if (overMet > opt->_met_cnt)
          continue;
        if (overMet - met > opt->_overDist)
          continue;
        if ((opt->_overMet > 0) && (opt->_overMet != (int) overMet))
          continue;

        patternSep = measure.initWS_box(opt, 20);
        measure.setMets(met, underMet, overMet);

        uint cnt1 = benchWithVar_density(opt, &measure);

        cnt += cnt1;

        opt->_ur[0] = MAX(opt->_ur[0], measure._ur[0]);
        opt->_ur[1] = MAX(opt->_ur[1], measure._ur[1]);
      }
    }
  }

  logger_->info(
      RCX, 56, "Finished {} measurements for pattern MET_UNDER_MET", cnt);

  // closeCapLogFile();
  return cnt;
}
uint extMain::benchWires(extMainOptions* opt)
{
  _tech = _db->getTech();
  if (opt->_db_only) {
    uint layerCnt = _tech->getRoutingLayerCount();
    extRCModel* m = new extRCModel(layerCnt, "processName", logger_);
    _modelTable->add(m);

    // m->setProcess(p);
    m->setDataRateTable(1);
  }
  extRCModel* m = _modelTable->get(0);

  m->setOptions(opt->_topDir,
                opt->_name,
                opt->_write_to_solver,
                opt->_read_from_solver,
                opt->_run_solver);

  opt->_tech = _tech;

  if (_block == NULL) {
    dbChip* chip = dbChip::create(_db);
    assert(chip);
    _block = dbBlock::create(chip, opt->_name, '/');
    assert(_block);
    _prevControl = _block->getExtControl();
    _block->setBusDelimeters('[', ']');
    _block->setDefUnits(1000);
    m->setExtMain(this);
    setupMapping(0);
    _noModelRC = true;
    _cornerCnt = 1;
    _extDbCnt = 1;
    _block->setCornerCount(_cornerCnt);
    //	getResCapTable(true);
    opt->_ll[0] = 0;
    opt->_ll[1] = 0;
    opt->_ur[0] = 0;
    opt->_ur[1] = 0;
    m->setLayerCnt(_tech->getRoutingLayerCount());
  } else {
    dbBox* bb = _block->getBBox();

    opt->_ll[0] = bb->xMax();
    opt->_ll[1] = 0;
    opt->_ur[0] = bb->xMax();
    opt->_ur[1] = 0;
  }
  opt->_block = _block;

  if (opt->_gen_def_patterns) {
    m->linesOverBench(opt);
    m->linesOverUnderBench(opt);
    m->linesUnderBench(opt);
    m->linesDiagUnderBench(opt);
  } else {
    if (opt->_over)
      m->linesOverBench(opt);
    else if (opt->_overUnder)
      m->linesOverUnderBench(opt);
    else {
      if (opt->_diag)
        m->linesDiagUnderBench(opt);
      else
        m->linesUnderBench(opt);
    }
  }

  /*
  if (opt->_over)
    m->linesOverBench(opt);

  if ((!opt->_over) && (opt->_overMet != 0) && (opt->_underMet == 0)) {
    if (opt->_diag)
      m->linesDiagUnderBench(opt);
    else
      m->linesUnderBench(opt);
  }

  if ((!opt->_over) && (opt->_overMet != 0) && (opt->_underMet != 0))
    m->linesOverUnderBench(opt);
  */

  dbBox* bb = _block->getBBox();
  Rect r(bb->xMin(), bb->yMin(), bb->xMax(), bb->yMax());
  _block->setDieArea(r);
  _extracted = true;
  updatePrevControl();

  return 0;
}
uint extMain::runSolver(extMainOptions* opt, uint netId, int shapeId)
{
  extRCModel* m = new extRCModel("TYPICAL", logger_);
  m->setExtMain(this);
  m->setOptions(opt->_topDir, "nets", false, false, true);
  uint shapeCnt = m->runWiresSolver(netId, shapeId);
  return shapeCnt;
}
uint extMain::benchNets(extMainOptions* opt,
                        uint netId,
                        uint trackCnt,
                        ZPtr<ISdb> netSdb)
{
  if (_block == NULL) {
    return 0;
  }
  extRCModel* m = _modelTable->get(0);
  if (m->getProcess() == NULL)
    m = _modelTable->get(1);

  m->setExtMain(this);

  m->setOptions(opt->_topDir,
                "nets",
                opt->_write_to_solver,
                opt->_read_from_solver,
                opt->_run_solver);

  opt->_tech = _tech;
  opt->_block = _block;

  uint boxCnt = m->netWiresBench(opt, this, netId, netSdb);

  //_spef->writeNet(dbNet::getNet(_block, netId), 0.0, 0);

  return boxCnt;
}
bool extRCModel::measureNetPattern(extMeasure* m,
                                   uint shapeId,
                                   Ath__array1D<ext2dBox*>* boxArray)
{
  strcpy(_wireFileName, "net_wires");
  fprintf(_logFP, "pattern Dir %s\n\n", _wireDirName);
  fflush(_logFP);

  sprintf(_commentLine, "%s", "");
  /*
          printCommentLine('$', m);
          fprintf(_logFP, "%s\n", _commentLine);
          fflush(_logFP);
  */
  if (_writeFiles)
    makePatternNet3D(m, boxArray);

  if ((_runSolver) && (!_readCapLog))
    runSolver("rc3 -n -x");

  if (_readSolver) {
    _readCapLog = false;
    uint lineCnt = readCapacitanceBench3D(_readCapLog, m, true);

    if (lineCnt <= 0) {
      _readCapLog = false;
      if (_runSolver)
        runSolver("rc3 -n -x");

      lineCnt = readCapacitanceBench3D(_readCapLog, m, true);
    }
    if (lineCnt > 0)
      getNetCapMatrixValues3D(lineCnt, shapeId, m);
  }
  return true;
}
uint extRCModel::writePatternGeoms(extMeasure* m,
                                   Ath__array1D<ext2dBox*>* boxArray)
{
  FILE* fp = openFile(_wireDirName, "2d_geoms", NULL, "w");

  fprintf(fp,
          "BBOX: (%d, %d)  (%d, %d)\n\n",
          m->_ll[0],
          m->_ll[1],
          m->_ur[0],
          m->_ur[1]);
  fprintf(fp,
          "BBOX: (%g, %g)  (%g, %g)\n\n",
          0.001 * m->_ll[0],
          0.001 * m->_ll[1],
          0.001 * m->_ur[0],
          0.001 * m->_ur[1]);

  for (uint ii = 0; ii < boxArray->getCnt(); ii++) {
    ext2dBox* bb = boxArray->get(ii);
    uint met = bb->_met;

    double h = _process->getConductor(met)->_height;
    double t = _process->getConductor(met)->_thickness;
    bb->printGeoms3D(fp, h, t, m->_ll);
  }
  fclose(fp);
  return boxArray->getCnt();
}
bool extRCModel::makePatternNet3D(extMeasure* measure,
                                  Ath__array1D<ext2dBox*>* boxArray)
{
  FILE* wfp = mkPatternFile();

  if (wfp == NULL)
    return false;  // should be an exception!! and return!

  double maxHeight
      = _process->adjustMasterDielectricsForHeight(measure->_met, 0.0);
  maxHeight *= 1.2;

  double W = 40;
  _process->writeProcessAndGround3D(
      wfp, "GND", -1, -1, -30.0, 60.0, 15, maxHeight, W, false);

  if (_commentFlag)
    fprintf(wfp, "%s\n", _commentLine);

  double h = _process->getConductor(measure->_met)->_height;
  double th = _process->getConductor(measure->_met)->_thickness;

  ext2dBox* bb1 = boxArray->get(0);
  uint mainDir = bb1->_dir;
  if (mainDir == 0) {
    int x = measure->_ll[0];
    measure->_ll[0] = measure->_ll[1];
    measure->_ll[1] = x;

    x = measure->_ur[0];
    measure->_ur[0] = measure->_ur[1];
    measure->_ur[1] = x;

    measure->_dir = !measure->_dir;

    bb1->rotate();
  }

  measure->writeBoxRaphael3D(wfp, bb1, measure->_ll, measure->_ur, h, th, 1.0);

  for (uint ii = 1; ii < boxArray->getCnt(); ii++) {
    ext2dBox* bb = boxArray->get(ii);
    if (mainDir == 0) {
      bb->rotate();
    }
    uint met = bb->_met;

    double h = _process->getConductor(met)->_height;
    double t = _process->getConductor(met)->_thickness;
    //		int low= 0;

    measure->writeBoxRaphael3D(wfp, bb, measure->_ll, measure->_ur, h, t, 0.0);
  }
  uint cn = boxArray->getCnt();

  if (cn < 50)
    fprintf(wfp, "\nOPTIONS SET_GRID=1000000;\n\n");
  else if (cn < 100)
    fprintf(wfp, "\nOPTIONS SET_GRID=2000000;\n\n");
  else if (cn < 150)
    fprintf(wfp, "\nOPTIONS SET_GRID=3000000;\n\n");
  else
    fprintf(wfp, "\nOPTIONS SET_GRID=5000000;\n\n");
  fprintf(wfp, "POTENTIAL\n");

  fclose(wfp);

  writePatternGeoms(measure, boxArray);

  measure->clean2dBoxTable(measure->_met, false);
  for (uint jj = 1; jj < _layerCnt; jj++) {
    measure->clean2dBoxTable(jj, true);
  }

  return true;
}
uint extMeasure::getRSeg(dbNet* net, uint shapeId)
{
  dbWire* w = net->getWire();

  int rsegId = 0;
  if (w->getProperty(shapeId, rsegId) && rsegId != 0)
    return rsegId;
  else
    return 0;
}
bool ext2dBox::matchCoords(int* ll, int* ur)
{
  if ((ur[0] < _ll[0]) || (ll[0] > _ur[0]) || (ur[1] < _ll[1])
      || (ll[1] > _ur[1]))
    return false;
  /*
          for (uint ii= 0; ii<2; ii++) {
                  if ((_ur[ii]!=ur[ii]) || (_ll[ii]!=ll[ii]))
                          return false;
          }
  */
  return true;
}

uint extRCModel::runWiresSolver(uint netId, int shapeId)
{
  sprintf(_wireDirName, "%s/%d/%d", _topDir, netId, shapeId);
  strcpy(_wireFileName, "net_wires");
  runSolver("rc3 -n -x");
  return 0;
}
uint extRCModel::netWiresBench(extMainOptions* opt,
                               extMain* xMain,
                               uint netId,
                               ZPtr<ISdb> netSearch)
{
  Ath__array1D<ext2dBox*> boxTable;

  extMeasure measure;
  measure._block = opt->_block;
  measure._extMain = xMain;
  measure._netId = netId;

  sprintf(_patternName, "%d", netId);

  // openCapLogFile();
  char filedir[2048];
  sprintf(filedir, "%s/%d", _topDir, netId);
  FILE* shapefp = openFile(filedir, "shapeId", NULL, "w");
  //	uint cnt= 0;

  dbNet* net = dbNet::getNet(opt->_block, netId);
  dbWire* wire = net->getWire();
  if (wire == NULL)
    return 0;

  int LL[2] = {0, 0};

  measure._mapTable[0] = 0;
  uint trackNum = 5;

  dbWireShapeItr shapes;
  dbShape s;
  for (shapes.begin(wire); shapes.next(s);) {
    boxTable.resetCnt();
    uint boxCnt = 1;

    if (s.isVia())
      continue;

    dbTechLayer* layer = s.getTechLayer();
    uint level = layer->getRoutingLevel();

    uint minWidth = layer->getWidth();
    uint minSpace = layer->getSpacing();
    uint pitch = minWidth + minSpace;
    uint offset = trackNum * pitch;

    int ll[2] = {s.xMin(), s.yMin()};
    int ur[2] = {s.xMax(), s.yMax()};

    uint dir = (ur[1] - ll[1]) < (ur[0] - ll[0]) ? 0 : 1;
    //		uint dir= (ur[1]-ll[1])<(ur[0]-ll[0]) ? 1 : 0;
    uint not_dir = !dir;

    // mapTable
    ext2dBox* bb = measure.addNew2dBox(NULL, ll, ur, level, dir, boxCnt, false);
    boxTable.add(bb);

    measure._len = bb->length();
    measure._met = level;
    measure._netId = netId;

    int shapeId = shapes.getShapeId();
    fprintf(shapefp, "%d\n", shapeId);
    uint rseg1 = measure.getRSeg(net, shapeId);
    measure._mapTable[boxCnt] = rseg1;
    bb->_map = rseg1;

    boxCnt++;

    measure._ll[dir] = ll[dir];
    measure._ur[dir] = ur[dir];

    measure._ll[not_dir] = ll[not_dir] - offset;
    measure._ur[not_dir] = ur[not_dir] + offset;

    sprintf(_wireDirName, "%s/%d/%d", _topDir, netId, shapeId);
    char cmd[2048];
    sprintf(cmd, "%s %s", "mkdir", _wireDirName);
    system(cmd);

    FILE* fp = openFile(_wireDirName, "db_geoms", NULL, "w");
    fprintf(fp,
            "BBOX: (%g, %g)  (%g, %g)\n\n",
            0.001 * measure._ll[0],
            0.001 * measure._ll[1],
            0.001 * measure._ur[0],
            0.001 * measure._ur[1]);
    bb->printGeoms3D(fp, 0, 0, LL);

    netSearch->searchWireIds(measure._ll[0],
                             measure._ll[1],
                             measure._ur[0],
                             measure._ur[1],
                             false,
                             NULL);

    int x1, y1, x2, y2;
    uint cntxLevel, cntxNetId, sId;
    while (netSearch->getNextBox(x1, y1, x2, y2, cntxLevel, cntxNetId, sId)) {
      if (cntxLevel >= _layerCnt)
        continue;

      int bb_ll[2] = {x1, y1};
      int bb_ur[2] = {x2, y2};

      if ((cntxNetId == netId) && (level == cntxLevel)
          && bb->matchCoords(bb_ll, bb_ur))
        continue;

      uint d = (x2 - x1) > (y2 - y1) ? 0 : 1;
      //			uint d= (x2-x1)>(y2-y1) ? 1 : 0;
      uint nd = !d;

      if (bb_ll[d] < measure._ll[d])
        bb_ll[d] = measure._ll[d];

      if (bb_ur[d] > measure._ur[d])
        bb_ur[d] = measure._ur[d];

      if (bb_ur[nd] > measure._ur[nd])
        //				measure._ur[nd]= bb_ur[nd];
        bb_ur[nd] = measure._ur[nd];

      if (bb_ll[nd] < measure._ll[nd])
        //				measure._ll[nd]= bb_ll[nd];
        bb_ll[nd] = measure._ll[nd];
      if (bb_ll[0] >= bb_ur[0] || bb_ll[1] >= bb_ur[1])
        continue;

      uint rsegId1 = 0;
      if (sId > 0) {
        dbNet* cntxNet = dbNet::getNet(opt->_block, cntxNetId);
        rsegId1 = measure.getRSeg(cntxNet, sId);
      }
      measure._mapTable[boxCnt] = rsegId1;

      ext2dBox* bb_cntxt;
      if (sId > 0)
        bb_cntxt = measure.addNew2dBox(
            NULL, bb_ll, bb_ur, cntxLevel, d, boxCnt, true);
      else
        bb_cntxt
            = measure.addNew2dBox(NULL, bb_ll, bb_ur, cntxLevel, d, 0, true);

      bb_cntxt->_map = rsegId1;

      boxTable.add(bb_cntxt);
      bb_cntxt->printGeoms3D(fp, 0.0, 0.0, LL);

      boxCnt++;
    }
    fclose(fp);

    logger_->info(RCX,
                  0,
                  "Finished {} boxes for net ({},{}) at coords ({},{}) ({},{})",
                  boxCnt - 1,
                  netId,
                  shapeId,
                  s.xMin(),
                  s.yMin(),
                  s.xMax(),
                  s.yMax(),
                  boxCnt);

    if (boxCnt == 0)
      return 0;

    if (!measureNetPattern(&measure, shapeId, &boxTable))
      return 0;
  }
  //	logger_->info(RCX, 0, "Finished pattern for net id %d at coors ({},{})
  //({},{}), {} boxes", 		cnt, s.xMin(), s.yMin(), s.xMax(),
  // s.yMax(), boxCnt);

  // closeCapLogFile();
  fclose(shapefp);

  return 0;
}
uint extRCModel::getNetCapMatrixValues3D(uint nodeCnt,
                                         uint shapeId,
                                         extMeasure* m)
{
  dbRSeg* rseg1 = dbRSeg::getRSeg(m->_block, m->_mapTable[1]);
  uint Id = rseg1->getNet()->getId();

  //	double gndCap= m->_capMatrix[1][0];
  m->_capMatrix[1][0] = 0.0;
  double frCap = m->_capMatrix[1][1];
  m->_capMatrix[1][1] = 0.0;

  double CC = 0.0;
  double sameNetC = 0.0;

  for (uint n = 2; n < nodeCnt + 1; n++) {
    double cc1 = m->_capMatrix[1][n];
    m->_capMatrix[1][n] = 0.0;

    if (cc1 < m->_extMain->getLoCoupling())
      continue;
    /*
    double diff_per= 100*cc1/totCC;
    if (diff_per < 1.0) // small coupling
            continue;
    */

    //		if (m->_mapTable[n]>0) {
    uint capId = m->_idTable[n];
    uint rsegId = m->_mapTable[capId];
    if (rsegId > 0) {
      //			dbRSeg* rsegN=
      // dbRSeg::getRSeg(m->_block, m->_mapTable[n]);
      dbRSeg* rsegN = dbRSeg::getRSeg(m->_block, rsegId);

      if (Id == rsegN->getNet()->getId()) {
        sameNetC += cc1;
      } else {
        m->_extMain->updateCCCap(rseg1, rsegN, cc1);
        CC += cc1;
      }
      // logger_->info(RCX, 0, "\tccCap for netIds %d(%d), {}({}) %e",
      //	m->_idTable[n], n, m->_idTable[n+1], n+1, cc1);
    } else {
    }
  }
  frCap -= (CC + sameNetC);
  rseg1->setCapacitance(frCap);
  fprintf(_capLogFP,
          "3DCAP(FF): net_%d_node_%d_rseg_%d  tot= %g  cc= %g  gnd= %g\n",
          m->_netId,
          shapeId,
          m->_mapTable[1],
          frCap + CC,
          CC,
          frCap);

  // logger_->info(RCX, 0, "\tfrCap from CC for netId {}({}) %e",
  // m->_idTable[n], n, ccFr); m->printStats(_capLogFP);
  fprintf(_capLogFP, "\n\nEND\n\n");

  return 0;
}

}  // namespace rcx
