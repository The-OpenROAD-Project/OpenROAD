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

#include <map>
#include <vector>

#include "rcx/extRCap.h"
#include "rcx/extSpef.h"
#include "rcx/extprocess.h"
#include "utl/Logger.h"

namespace rcx {

using utl::RCX;

using odb::dbBlock;
using odb::dbBox;
using odb::dbChip;
using odb::dbNet;
using odb::dbSet;
using odb::dbTechLayer;
using odb::dbTechLayerRule;
using odb::dbTechNonDefaultRule;
using odb::dbWire;
using odb::Rect;

extMainOptions::extMainOptions()
{
  _met = -1;
  _underMet = -1;
  _overMet = -1;

  _varFlag = false;
}

uint extRCModel::benchWithVar_density(extMainOptions* opt, extMeasure* measure)
{
  if (opt->_db_only) {
    return benchDB_WS(opt, measure);
  }
  if (opt->_listsFlag) {
    return benchWithVar_lists(opt, measure);
  }

  uint cnt = 0;
  int met = measure->_met;
  extVariation* xvar = _process->getVariation(measure->_met);

  extConductor* cond = _process->getConductor(met);
  double t = cond->_thickness;
  double h = cond->_height;
  double ro = cond->_p;

  for (uint ii = 0; ii < opt->_widthTable.getCnt(); ii++) {
    double w = 0.001 * measure->_minWidth * opt->_widthTable.get(ii);  // layout
    measure->_wIndex = measure->_widthTable.findNextBiggestIndex(w);   // layout

    for (uint jj = 0; jj < opt->_spaceTable.getCnt(); jj++) {
      double s
          = 0.001 * measure->_minSpace * opt->_spaceTable.get(jj);  // layout
      measure->_sIndex
          = measure->_widthTable.findNextBiggestIndex(s);  // layout

      for (uint kk = 0; kk < opt->_densityTable.getCnt(); kk++) {
        double r = opt->_densityTable.get(kk);  // layout
        measure->_rIndex
            = measure->_dataTable.findNextBiggestIndex(r);  // layout

        double top_width;
        double top_widthR;
        double bot_width = w;
        double thickness = t;
        double bot_widthR = w;
        double thicknessR = t;

        if (r <= 0.0) {
          top_width = w;
          top_widthR = w;
        } else if (xvar != nullptr) {
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
        measurePatternVar(measure,
                          top_width,
                          bot_width,
                          thickness,
                          measure->_wireCnt,
                          nullptr);

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

  extConductor* cond = _process->getConductor(met);
  double t = cond->_thickness;
  double h = cond->_height;
  double ro = cond->_p;

  dbTechLayer* layer = opt->_tech->findRoutingLayer(met);

  double minWidth = 0.001 * layer->getWidth();
  ;
  double pitch = 0.001 * layer->getPitch();

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

    for (nditr = nd_rules.begin(); nditr != nd_rules.end(); ++nditr) {
      tst_rule = (*nditr)->getLayerRule(layer);
      if (tst_rule == nullptr) {
        continue;
      }

      double w = tst_rule->getWidth();
      double s = tst_rule->getSpacing();
      wTable->add(w);
      sTable->add(s);
    }
  }

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
            if (!opt->_thListFlag) {       // multiplier
              tt *= t;
            }

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
                              nullptr);

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
  if (opt->_met == 0) {
    return 0;
  }

  extMeasure measure(logger_);
  measure.updateForBench(opt, _extMain);
  measure._diag = false;

  sprintf(_patternName, "O%d", opt->_wireCnt + 1);
  if (opt->_res_patterns) {
    sprintf(_patternName, "R%d", opt->_wireCnt + 1);
  }

  uint cnt = 0;

  for (int met = 1; met <= (int) _layerCnt; met++) {
    if (met > opt->_met_cnt) {
      continue;
    }
    if ((opt->_met > 0) && (opt->_met != met)) {
      continue;
    }

    measure._met = met;

    if (!opt->_db_only) {
      computeTables(&measure, opt->_wireCnt + 1, 1000, 1000, 1000);
    }

    uint patternSep = measure.initWS_box(opt, 20);

    for (int underMet = 0; underMet < met; underMet++) {
      if ((opt->_underMet > 0) && (opt->_underMet != underMet)) {
        continue;
      }

      if (met - underMet > (int) opt->_underDist) {
        continue;
      }

      measure.setMets(met, underMet, -1);

      uint cnt1 = benchWithVar_density(opt, &measure);

      cnt += cnt1;
      measure._ur[measure._dir] += patternSep;

      if (opt->_underMet == 0 && !opt->_gen_def_patterns) {
        break;
      }

      if (underMet == 0 && opt->_res_patterns) {
        break;
      }
    }
    opt->_ur[0] = std::max(opt->_ur[0], measure._ur[0]);
    opt->_ur[1] = std::max(opt->_ur[1], measure._ur[1]);
  }

  logger_->info(
      RCX, 55, "Finished {} bench measurements for pattern MET_OVER_MET", cnt);

  return cnt;
}

uint extRCModel::linesUnderBench(extMainOptions* opt)
{
  if (opt->_overMet == 0) {
    return 0;
  }

  extMeasure measure(logger_);
  measure.updateForBench(opt, _extMain);
  measure._diag = false;

  uint patternSep = 1000;

  sprintf(_patternName, "U%d", opt->_wireCnt + 1);
  uint cnt = 0;

  for (int met = 1; met < (int) _layerCnt; met++) {
    if (met > opt->_met_cnt) {
      continue;
    }
    if ((opt->_met > 0) && (opt->_met != met)) {
      continue;
    }

    measure._met = met;

    if (!opt->_db_only) {
      computeTables(&measure, opt->_wireCnt + 1, 1000, 1000, 1000);
    }

    patternSep = measure.initWS_box(opt, 20);

    for (int overMet = met + 1; overMet <= (int) _layerCnt; overMet++) {
      if (overMet > opt->_met_cnt) {
        continue;
      }

      if ((opt->_overMet > 0) && (opt->_overMet != overMet)) {
        continue;
      }

      if (overMet - met > (int) opt->_overDist) {
        continue;
      }

      measure.setMets(met, 0, overMet);

      uint cnt1 = benchWithVar_density(opt, &measure);

      cnt += cnt1;
      measure._ur[measure._dir] += patternSep;
    }
    opt->_ur[0] = std::max(opt->_ur[0], measure._ur[0]);
    opt->_ur[1] = std::max(opt->_ur[1], measure._ur[1]);
  }
  logger_->info(
      RCX, 57, "Finished {} bench measurements for pattern MET_UNDER_MET", cnt);

  return cnt;
}

uint extRCModel::linesDiagUnderBench(extMainOptions* opt)
{
  if (opt->_overMet == 0) {
    return 0;
  }

  extMeasure measure(logger_);
  measure.updateForBench(opt, _extMain);
  measure._diag = true;

  uint patternSep = 1000;

  sprintf(_patternName, "DU%d", opt->_wireCnt + 1);
  uint cnt = 0;

  for (int met = 1; met < (int) _layerCnt; met++) {
    if (met > opt->_met_cnt) {
      continue;
    }

    if ((opt->_met > 0) && (opt->_met != met)) {
      continue;
    }

    measure._met = met;
    if (!opt->_db_only) {
      computeTables(&measure, opt->_wireCnt + 1, 1000, 1000, 1000);
    }

    patternSep = measure.initWS_box(opt, 20);

    for (int overMet = met + 1; overMet < met + 5 && overMet <= (int) _layerCnt;
         overMet++) {
      if (overMet > opt->_met_cnt) {
        continue;
      }

      if ((opt->_overMet > 0) && (opt->_overMet != overMet)) {
        continue;
      }

      if (overMet - met > (int) opt->_overDist) {
        continue;
      }

      measure.setMets(met, 0, overMet);

      uint cnt1 = benchWithVar_density(opt, &measure);

      cnt += cnt1;
      measure._ur[measure._dir] += patternSep;
    }
    opt->_ur[0] = std::max(opt->_ur[0], measure._ur[0]);
    opt->_ur[1] = std::max(opt->_ur[1], measure._ur[1]);
  }

  logger_->info(RCX,
                58,
                "Finished {} bench measurements for pattern MET_DIAGUNDER_MET",
                cnt);

  return cnt;
}

uint extRCModel::linesOverUnderBench(extMainOptions* opt)
{
  if (opt->_overMet == 0) {
    return 0;
  }

  extMeasure measure(logger_);
  measure.updateForBench(opt, _extMain);
  measure._diag = false;

  sprintf(_patternName, "OU%d", opt->_wireCnt + 1);
  uint cnt = 0;

  for (int met = 1; met <= (int) _layerCnt - 1; met++) {
    if (met > opt->_met_cnt) {
      continue;
    }
    if ((opt->_met > 0) && (opt->_met != met)) {
      continue;
    }

    measure._met = met;

    if (!opt->_db_only) {
      computeTables(&measure, opt->_wireCnt + 1, 1000, 1000, 1000);
    }

    measure.initWS_box(opt, 20);

    for (int underMet = 1; underMet < met; underMet++) {
      if (met - underMet > (int) opt->_underDist) {
        continue;
      }
      if ((opt->_underMet > 0) && ((int) opt->_underMet != underMet)) {
        continue;
      }

      for (uint overMet = met + 1; overMet <= _layerCnt; overMet++) {
        if (overMet > opt->_met_cnt) {
          continue;
        }
        if (overMet - met > opt->_overDist) {
          continue;
        }
        if ((opt->_overMet > 0) && (opt->_overMet != (int) overMet)) {
          continue;
        }

        measure.initWS_box(opt, 20);
        measure.setMets(met, underMet, overMet);

        uint cnt1 = benchWithVar_density(opt, &measure);

        cnt += cnt1;

        opt->_ur[0] = std::max(opt->_ur[0], measure._ur[0]);
        opt->_ur[1] = std::max(opt->_ur[1], measure._ur[1]);
      }
    }
  }

  logger_->info(
      RCX, 7, "Finished {} measurements for pattern MET_UNDER_MET", cnt);

  return cnt;
}

uint extMain::benchWires(extMainOptions* opt)
{
  _tech = _db->getTech();
  if (opt->_db_only) {
    uint layerCnt = _tech->getRoutingLayerCount();
    extRCModel* m = new extRCModel(layerCnt, "processName", logger_);
    _modelTable->add(m);

    m->setDataRateTable(1);
  }
  extRCModel* m = _modelTable->get(0);

  m->setOptions(opt->_topDir,
                opt->_name,
                opt->_write_to_solver,
                opt->_read_from_solver,
                opt->_run_solver);

  opt->_tech = _tech;

  if (_block == nullptr) {
    dbChip* chip = dbChip::create(_db);
    assert(chip);
    _block = dbBlock::create(chip, opt->_name, _tech, '/');
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

    opt->_res_patterns = true;
    m->linesOverBench(opt);
    opt->_res_patterns = false;

    m->linesOverUnderBench(opt);
    m->linesUnderBench(opt);
    m->linesDiagUnderBench(opt);
  } else {
    if (opt->_over) {
      m->linesOverBench(opt);
    } else if (opt->_overUnder) {
      m->linesOverUnderBench(opt);
    } else {
      if (opt->_diag) {
        m->linesDiagUnderBench(opt);
      } else {
        m->linesUnderBench(opt);
      }
    }
  }

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

uint extMeasure::getRSeg(dbNet* net, uint shapeId)
{
  dbWire* w = net->getWire();

  int rsegId = 0;
  if (w->getProperty(shapeId, rsegId) && rsegId != 0) {
    return rsegId;
  }
  return 0;
}

uint extRCModel::runWiresSolver(uint netId, int shapeId)
{
  sprintf(_wireDirName, "%s/%d/%d", _topDir, netId, shapeId);
  strcpy(_wireFileName, "net_wires");
  runSolver("rc3 -n -x");
  return 0;
}

}  // namespace rcx
