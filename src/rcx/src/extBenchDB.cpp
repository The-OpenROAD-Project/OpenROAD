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

#include "odb/db.h"
#include "rcx/extRCap.h"
#include "rcx/extSpef.h"
#include "rcx/extprocess.h"
#include "utl/Logger.h"

namespace rcx {

using odb::dbBTerm;
using odb::dbCapNode;
using odb::dbCCSeg;
using odb::dbIoType;
using odb::dbNet;
using odb::dbObstruction;
using odb::dbSet;
using odb::dbTechLayer;

extMetRCTable* extRCModel::initCapTables(uint layerCnt, uint widthCnt)
{
  createModelTable(1, layerCnt);
  for (uint kk = 0; kk < _modelCnt; kk++) {
    _dataRateTable->add(0.0);
  }

  _modelTable[0]->allocateInitialTables(layerCnt, widthCnt, true, true, true);
  return _modelTable[0];
}

AthPool<extDistRC>* extMetRCTable::getRCPool()
{
  return _rcPoolPtr;
}

uint extMain::genExtRules(const char* rulesFileName, int pattern)
{
  uint widthCnt = 12;
  uint layerCnt = _tech->getRoutingLayerCount() + 1;

  extRCModel* extRulesModel = new extRCModel(layerCnt, "TYPICAL", logger_);
  this->_modelTable->add(extRulesModel);
  extRulesModel->setDiagModel(1);
  extRulesModel->setOptions("./", "", false, true, false, false);

  extMetRCTable* rcModel = extRulesModel->initCapTables(layerCnt, widthCnt);

  AthPool<extDistRC>* rcPool = rcModel->getRCPool();
  extMeasure m(logger_);

  m._diagModel = 1;

  char buff[2000];
  sprintf(buff, "%s.log", rulesFileName);
  FILE* logFP = fopen(buff, "w");

  Ath__parser* p = new Ath__parser(logger_);
  Ath__parser* w = new Ath__parser(logger_);

  int prev_sep = 0;
  int prev_width = 0;
  int n = 0;

  dbSet<dbNet> nets = _block->getNets();
  dbSet<dbNet>::iterator itr;
  for (itr = nets.begin(); itr != nets.end(); ++itr) {
    dbNet* net = *itr;
    const char* netName = net->getConstName();

    uint wireCnt = 0;
    uint viaCnt = 0;
    uint len = 0;
    uint layerCnt = 0;
    uint layerTable[20];

    net->getNetStats(wireCnt, viaCnt, len, layerCnt, layerTable);

    uint wcnt = p->mkWords(netName, "_");
    if (wcnt < 5) {
      continue;
    }

    int targetWire = 0;
    if (p->getFirstChar() == 'U') {
      targetWire = p->getInt(0, 1);
    } else {
      char* w1 = p->get(0);
      if (w1[1] == 'U') {  // OU
        targetWire = p->getInt(0, 2);
      } else {
        targetWire = p->getInt(0, 1);
      }
    }
    if (targetWire <= 0) {
      continue;
    }

    uint wireNum = p->getInt(4);
    if (wireNum != targetWire / 2) {
      continue;
    }

    bool diag = false;
    bool ResModel = false;

    uint met = p->getInt(0, 1);
    uint overMet = 0;
    uint underMet = 0;

    m._overMet = -1;
    m._underMet = -1;
    m._overUnder = false;
    m._over = false;
    m._diag = false;
    m._res = false;

    char* overUnderToken = p->get(1);  // M2oM1uM3
    int wCnt = w->mkWords(overUnderToken, "ou");
    if (wCnt < 2) {
      continue;
    }

    if (wCnt == 3) {  // M2oM1uM3
      met = w->getInt(0, 1);
      overMet = w->getInt(1, 1);
      underMet = w->getInt(2, 1);

      m._overMet = underMet;
      m._underMet = overMet;
      m._overUnder = true;
      m._over = false;

    } else if (strstr(overUnderToken, "o") != nullptr) {
      met = w->getInt(0, 1);
      overMet = w->getInt(1, 1);
      if (p->getFirstChar() == 'R') {
        ResModel = true;
        m._res = ResModel;
      }
      m._overMet = -1;
      m._underMet = overMet;
      m._overUnder = false;
      m._over = true;
    } else if (strstr(overUnderToken, "uu") != nullptr) {
      met = w->getInt(0, 1);
      underMet = w->getInt(1, 1);
      diag = true;
      m._diag = true;
      m._overMet = underMet;
      m._underMet = -1;
      m._overUnder = false;
      m._over = false;
    } else if (strstr(overUnderToken, "u") != nullptr) {
      met = w->getInt(0, 1);
      underMet = w->getInt(1, 1);

      m._overMet = underMet;
      m._underMet = -1;
      m._overUnder = false;
      m._over = false;
    }
    m._met = met;

    if (w->mkWords(p->get(2), "W") <= 0) {
      continue;
    }

    double w1 = w->getDouble(0) / 1000;

    m._w_m = w1;
    m._w_nm = lround(m._w_m * 1000);

    if (w->mkWords(p->get(3), "S") <= 0) {
      continue;
    }

    double s1 = w->getDouble(0) / 1000;
    double s2 = w->getDouble(1) / 1000;

    m._s_m = s1;
    m._s_nm = lround(m._s_m * 1000);
    m._s2_m = s2;
    m._s2_nm = lround(m._s2_m * 1000);

    double wLen = GetDBcoords2(len) * 1.0;
    double totCC = net->getTotalCouplingCap();
    double totGnd = net->getTotalCapacitance();
    double res = net->getTotalResistance();

    double contextCoupling = getTotalCouplingCap(net, "cntxM", 0);
    if (contextCoupling > 0) {
      totGnd += contextCoupling;
      totCC -= contextCoupling;
    }

    double cc = totCC / wLen / 2;
    double gnd = totGnd / wLen / 2;
    double R = res / wLen / 2;

    extDistRC* rc = rcPool->alloc();

    if (ResModel) {
      R *= 2;
      rc->set(m._s_nm, m._s2_m, 0.0, 0.0, R);
    } else {
      if (diag) {
        rc->set(m._s_nm, 0.0, cc, cc, R);
      } else {
        if (m._s_nm == 0) {
          m._s_nm = prev_sep + prev_width;
        }
        rc->set(m._s_nm, cc, gnd, 0.0, R);
      }
    }
    m._tmpRC = rc;
    rcModel->addRCw(&m);
    prev_sep = m._s_nm;
    prev_width = m._w_nm;

    if (ResModel) {
      fprintf(
          logFP,
          "M%2d OVER %2d UNDER %2d W %.3f S1 %.3f S2 %.3f R %g LEN %g %g  %s\n",
          met,
          overMet,
          underMet,
          w1,
          s1,
          s2,
          res,
          wLen,
          R,
          netName);
    } else {
      fprintf(
          logFP,
          "M%2d OVER %2d UNDER %2d W %.3f S %.3f CC %.6f GND %.6f TC %.6f x "
          "%.6f R %g LEN %g  %s\n",
          met,
          overMet,
          underMet,
          w1,
          s1,
          totCC,
          totGnd,
          totCC + totGnd,
          contextCoupling,
          res,
          wLen,
          netName);
    }
  }
  rcModel->mkWidthAndSpaceMappings();
  extRulesModel->writeRules((char*) rulesFileName, false);

  fclose(logFP);
  return n;
}

double extMain::getTotalCouplingCap(dbNet* net,
                                    const char* filterNet,
                                    uint corner)
{
  double cap = 0.0;
  for (dbCapNode* n : net->getCapNodes()) {
    for (dbCCSeg* cc : n->getCCSegs()) {
      dbNet* srcNet = cc->getSourceCapNode()->getNet();
      dbNet* tgtNet = cc->getTargetCapNode()->getNet();
      if ((strstr(srcNet->getConstName(), filterNet) == nullptr)
          && (strstr(tgtNet->getConstName(), filterNet) == nullptr)) {
        continue;
      }

      cap += cc->getCapacitance(corner);
    }
  }
  return cap;
}

uint extMain::benchVerilog(FILE* fp)
{
  fprintf(fp, "module %s (\n", _block->getConstName());
  benchVerilog_bterms(fp, dbIoType::OUTPUT, "  ", ",");
  benchVerilog_bterms(fp, dbIoType::INPUT, "  ", ",", true);
  fprintf(fp, ");\n\n");
  benchVerilog_bterms(fp, dbIoType::OUTPUT, "  output ", " ;");
  fprintf(fp, "\n");
  benchVerilog_bterms(fp, dbIoType::INPUT, "  input ", " ;");
  fprintf(fp, "\n");

  benchVerilog_bterms(fp, dbIoType::OUTPUT, "  wire ", " ;");
  fprintf(fp, "\n");
  benchVerilog_bterms(fp, dbIoType::INPUT, "  wire ", " ;");
  fprintf(fp, "\n");

  benchVerilog_assign(fp);
  fprintf(fp, "endmodule\n");
  fclose(fp);
  return 0;
}

uint extMain::benchVerilog_bterms(FILE* fp,
                                  const dbIoType& iotype,
                                  const char* prefix,
                                  const char* postfix,
                                  bool skip_postfix_last)
{
  int n = 0;
  dbSet<dbNet> nets = _block->getNets();
  for (dbNet* net : nets) {
    dbSet<dbBTerm> bterms = net->getBTerms();
    for (dbBTerm* bterm : bterms) {
      const char* btermName = bterm->getConstName();

      if (iotype != bterm->getIoType()) {
        continue;
      }
      if (n == nets.size() - 1 && skip_postfix_last) {
        fprintf(fp, "%s%s\n", prefix, btermName);
      } else {
        fprintf(fp, "%s%s%s\n", prefix, btermName, postfix);
      }
      n++;
    }
  }
  return n;
}

uint extMain::benchVerilog_assign(FILE* fp)
{
  int n = 0;
  dbSet<dbNet> nets = _block->getNets();
  for (dbNet* net : nets) {
    const char* bterm1 = nullptr;
    const char* bterm2 = nullptr;
    dbSet<dbBTerm> bterms = net->getBTerms();
    for (dbBTerm* bterm : bterms) {
      if (bterm->getIoType() == dbIoType::OUTPUT) {
        bterm1 = bterm->getConstName();
        break;
      }
    }
    for (dbBTerm* bterm : bterms) {
      if (bterm->getIoType() == dbIoType::INPUT) {
        bterm2 = bterm->getConstName();
        break;
      }
    }
    fprintf(fp, "  assign %s = %s ;\n", bterm1, bterm2);
  }
  return n;
}

uint extRCModel::benchDB_WS(extMainOptions* opt, extMeasure* measure)
{
  Ath__array1D<double>* widthTable = new Ath__array1D<double>(4);
  Ath__array1D<double>* spaceTable = new Ath__array1D<double>(4);
  Ath__array1D<double>* wTable = &opt->_widthTable;
  Ath__array1D<double>* sTable = &opt->_spaceTable;
  Ath__array1D<double>* gTable = &opt->_gridTable;

  uint cnt = 0;
  int met = measure->_met;
  dbTechLayer* layer = opt->_tech->findRoutingLayer(met);

  double minWidth = 0.001 * layer->getWidth();
  double spacing = 0.001 * layer->getSpacing();
  double pitch = 0.001 * layer->getPitch();
  if (layer->getSpacing() == 0) {
    spacing = pitch - minWidth;
  }

  if (opt->_default_lef_rules) {  // minWidth, minSpacing, minThickness, pitch
                                  // multiplied by grid_list
    for (uint ii = 0; ii < gTable->getCnt(); ii++) {
      double s = spacing * gTable->get(ii);
      spaceTable->add(s);
      double w = minWidth * gTable->get(ii);
      widthTable->add(w);
    }
  } else if (opt->_nondefault_lef_rules) {
    return 0;
  } else {
    if (measure->_diag) {
      spaceTable->add(0.0);
    }
    if (!opt->_res_patterns) {
      for (uint ii = 0; ii < sTable->getCnt(); ii++) {
        double s = spacing * sTable->get(ii);
        if (sTable->get(ii) == 0 && measure->_diag) {
          continue;
        }
        spaceTable->add(s);
      }
    } else {
      spaceTable->add(0);

      for (uint ii = 1; ii < 2; ii++) {
        double s = pitch * ii;
        double s1 = s - minWidth;

        spaceTable->add(s1);
        spaceTable->add(s);
      }
      for (uint ii = 2; ii < 4; ii++) {
        double s = pitch * ii;
        double s1 = s - minWidth;
        double s2 = s1 - minWidth / 2;

        spaceTable->add(s2);
        spaceTable->add(s1);
        spaceTable->add(s);
      }
      spaceTable->add(pitch * 4);
    }
    for (uint ii = 0; ii < wTable->getCnt(); ii++) {
      double w = minWidth * wTable->get(ii);
      widthTable->add(w);
    }
  }
  bool use_symmetric_widths_spacings = false;
  if (!use_symmetric_widths_spacings) {
    for (uint ii = 0; ii < widthTable->getCnt(); ii++) {
      double w = widthTable->get(ii);  // layout
      double w2 = w;
      if (!opt->_res_patterns) {
        for (uint jj = 0; jj < spaceTable->getCnt(); jj++) {
          double s = spaceTable->get(jj);  // layout
          double s2 = s;

          measure->setTargetParams(w, s, 0.0, 0, 0, w2, s2);
          writeBenchWires_DB(measure);

          cnt++;
        }
      } else {
        for (uint jj = 0; jj < spaceTable->getCnt(); jj++) {
          double s = spaceTable->get(jj);  // layout
          for (uint kk = jj; kk < spaceTable->getCnt(); kk++) {
            double s2 = spaceTable->get(kk);

            measure->setTargetParams(w, s, 0.0, 0, 0, w2, s2);
            writeBenchWires_DB_res(measure);

            cnt++;
          }
        }
      }
    }
  }
  return cnt;
}

/**
 * writeBenchWires_DB_res routine generates the pattern
 * geometries to capture the effect of spacing wiring
 * neighbors on both sides of wire of interest on the
 * per unit resistance in the calibration flow.
 *
 *
 * @return number of wires in the pattern geometries
 */
int extRCModel::writeBenchWires_DB_res(extMeasure* measure)
{
  mkNet_prefix(measure, "");
  measure->_skip_delims = true;
  uint grid_gap_cnt = 40;

  int gap = grid_gap_cnt * (measure->_minWidth + measure->_minSpace);

  int n
      = measure->_wireCnt / 2;  // ASSUME odd number of wires, 2 will also work

  uint w_layout = measure->_minWidth;
  uint s_layout = measure->_minSpace;

  uint WW = measure->_w_nm;
  uint SS1 = measure->_s_nm;
  uint WW2 = measure->_w2_nm;
  uint SS2 = measure->_s2_nm;

  measure->clean2dBoxTable(measure->_met, false);

  if (measure->_s_nm == 0) {
    measure->createNetSingleWire(_wireDirName, 3, WW, SS1);
    if (measure->_s2_nm == 0) {
      measure->_ll[measure->_dir] += gap;
      measure->_ur[measure->_dir] += gap;

      return 1;
    }
    measure->createNetSingleWire(_wireDirName, 4, WW, SS2);
    measure->_ll[measure->_dir] += gap;
    measure->_ur[measure->_dir] += gap;

    return 2;
  }

  uint idCnt = 1;
  int ii = 0;
  if (s_layout > 0) {
    for (; ii < n - 1; ii++) {
      measure->createNetSingleWire(_wireDirName, idCnt, w_layout, s_layout);
      idCnt++;
    }
  }

  ii--;
  int cnt = 0;
  for (; ii >= 0; ii--) {
    cnt++;
  }

  if (n > 1) {
    cnt++;
    measure->createNetSingleWire(_wireDirName, idCnt, WW, s_layout);
    idCnt++;

    cnt++;
    if (!measure->_diag) {
      measure->createNetSingleWire(_wireDirName, idCnt, WW, SS1);
      idCnt++;
      cnt++;
      measure->createNetSingleWire(_wireDirName, idCnt, WW2, SS2);
      idCnt++;
    }

    for (int jj = 0; jj < n - 1; jj++) {
      cnt++;
      measure->createNetSingleWire(_wireDirName, idCnt, w_layout, s_layout);
      idCnt++;
    }
  } else {
    cnt++;
    measure->createNetSingleWire(_wireDirName, 3, w_layout, s_layout);
    idCnt++;
  }
  measure->_ll[measure->_dir] += gap;
  measure->_ur[measure->_dir] += gap;

  return cnt;
}

int extRCModel::writeBenchWires_DB(extMeasure* measure)
{
  if (measure->_diag) {
    return writeBenchWires_DB_diag(measure);
  }

  mkNet_prefix(measure, "");
  measure->_skip_delims = true;
  uint grid_gap_cnt = 40;

  int gap = grid_gap_cnt * (measure->_minWidth + measure->_minSpace);
  int bboxLL[2];
  bboxLL[measure->_dir] = measure->_ur[measure->_dir];
  bboxLL[!measure->_dir] = measure->_ll[!measure->_dir];

  // ASSUME odd number of wires, 2 will also work
  int n = measure->_wireCnt / 2;

  if (measure->_s_nm == 0 && !measure->_diag) {
    n = 1;
  }

  uint w_layout = measure->_minWidth;
  uint s_layout = measure->_minSpace;

  measure->clean2dBoxTable(measure->_met, false);

  uint idCnt = 1;
  int ii;
  for (ii = 0; ii < n - 1; ii++) {
    measure->createNetSingleWire(_wireDirName, idCnt, w_layout, s_layout);
    idCnt++;
  }

  ii--;
  int cnt = 0;
  for (; ii >= 0; ii--) {
    cnt++;
  }

  uint WW = measure->_w_nm;
  uint SS1;
  SS1 = measure->_s_nm;
  uint WW2 = measure->_w2_nm;
  uint SS2 = measure->_s2_nm;

  if (n > 1) {
    cnt++;
    measure->createNetSingleWire(_wireDirName, idCnt, WW, s_layout);
    idCnt++;

    cnt++;
    if (!measure->_diag) {
      measure->createNetSingleWire(_wireDirName, idCnt, WW, SS1);
      idCnt++;
      cnt++;
      measure->createNetSingleWire(_wireDirName, idCnt, WW2, SS2);
      idCnt++;
    } else {
      uint met_tmp = measure->_met;
      measure->_met = measure->_overMet;
      uint ss2 = SS1;
      if (measure->_s_nm == 0) {
        ss2 = measure->_s_nm;
      }

      measure->createNetSingleWire(_wireDirName, idCnt, 0, ss2);
      idCnt++;

      measure->_met = met_tmp;
      measure->createNetSingleWire(_wireDirName, idCnt, w_layout, s_layout);
      idCnt++;
    }

    for (int jj = 0; jj < n - 1; jj++) {
      cnt++;
      measure->createNetSingleWire(_wireDirName, idCnt, w_layout, s_layout);
      idCnt++;
    }
  } else {
    cnt++;
    measure->createNetSingleWire(_wireDirName, 3, w_layout, s_layout);
    idCnt++;
  }

  if (measure->_diag) {
    return cnt;
  }
  bool grid_context = true;
  bool grid_context_same_dir = false;

  int cntx_dist = -1;  // -1 means min sep

  int extend_blockage = (measure->_minWidth + measure->_minSpace);
  int extend_blockage_gap = measure->getPatternExtend();

  int bboxUR[2]
      = {measure->_ur[0] + extend_blockage, measure->_ur[1] + extend_blockage};
  bboxLL[0] -= extend_blockage;
  bboxLL[1] -= extend_blockage;

  if (grid_context && (measure->_underMet > 0 || measure->_overMet > 0)) {
    if (!grid_context_same_dir) {
      measure->createContextGrid(
          _wireDirName, bboxLL, bboxUR, measure->_underMet, cntx_dist);
      measure->createContextGrid(
          _wireDirName, bboxLL, bboxUR, measure->_overMet, cntx_dist);
    } else {
      measure->createContextGrid_dir(
          _wireDirName, bboxLL, bboxUR, measure->_underMet);
      measure->createContextGrid_dir(
          _wireDirName, bboxLL, bboxUR, measure->_overMet);
    }
  } else {
    double pitchMult = 1.0;

    measure->clean2dBoxTable(measure->_underMet, true);
    measure->createContextObstruction(_wireDirName,
                                      bboxLL[0],
                                      bboxLL[1],
                                      bboxUR,
                                      measure->_underMet,
                                      pitchMult);

    measure->clean2dBoxTable(measure->_overMet, true);
    measure->createContextObstruction(_wireDirName,
                                      bboxLL[0],
                                      bboxLL[1],
                                      bboxUR,
                                      measure->_overMet,
                                      pitchMult);
  }

  measure->_ur[measure->_dir] += gap + extend_blockage_gap;

  int main_xlo, main_ylo, main_xhi, main_yhi;
  measure->getBox(measure->_met, false, main_xlo, main_ylo, main_xhi, main_yhi);
  return 1;
}

uint extMeasure::getPatternExtend()
{
  int extend_blockage = (this->_minWidth + this->_minSpace);

  if (this->_overMet > 0) {
    dbTechLayer* layer
        = this->_create_net_util.getRoutingLayer()[this->_overMet];
    const uint ww = layer->getWidth();
    uint sp = layer->getSpacing();
    if (sp == 0) {
      sp = layer->getPitch() - ww;
    }

    extend_blockage = sp;
  }
  if (this->_underMet > 0) {
    dbTechLayer* layer
        = this->_create_net_util.getRoutingLayer()[this->_underMet];
    const uint ww = layer->getWidth();
    uint sp = layer->getSpacing();
    if (sp == 0) {
      sp = layer->getPitch() - ww;
    }

    if (extend_blockage < sp) {
      extend_blockage = sp;
    }
  }
  return extend_blockage;
}

uint extMeasure::createContextObstruction(const char* dirName,
                                          int x,
                                          int y,
                                          int bboxUR[2],
                                          int met,
                                          double pitchMult)
{
  if (met <= 0) {
    return 0;
  }

  dbTechLayer* layer = _tech->findRoutingLayer(met);
  dbObstruction::create(_block, layer, x, y, bboxUR[0], bboxUR[1]);
  return 1;
}

uint extMeasure::createContextGrid(char* dirName,
                                   const int bboxLL[2],
                                   const int bboxUR[2],
                                   int met,
                                   int s_layout)
{
  if (met <= 0) {
    return 0;
  }
  dbTechLayer* layer = this->_create_net_util.getRoutingLayer()[met];
  uint ww = layer->getWidth();
  uint sp = layer->getSpacing();
  if (sp == 0) {
    sp = layer->getPitch() - ww;
  }
  uint half_width = sp / 2;

  int ll[2] = {bboxLL[0], bboxLL[1]};

  int ur[2];
  ll[!this->_dir] = ll[!this->_dir];
  ll[this->_dir] = ll[this->_dir] - half_width;
  ur[!this->_dir] = ll[!this->_dir];
  ur[this->_dir] = bboxUR[this->_dir] + half_width;

  ur[!_dir] = ll[!_dir] + ww;

  int xcnt = 1;
  while (ll[!this->_dir] <= bboxUR[!this->_dir]) {
    this->createNetSingleWire_cntx(
        met, dirName, xcnt++, !this->_dir, ll, ur, s_layout);
    uint d = !_dir;
    ll[d] = ur[d] + sp;
    ur[d] = ll[d] + ww;
  }
  return xcnt;
}

uint extMeasure::createContextGrid_dir(char* dirName,
                                       const int bboxLL[2],
                                       const int bboxUR[2],
                                       int met)
{
  if (met <= 0) {
    return 0;
  }

  dbTechLayer* layer = this->_create_net_util.getRoutingLayer()[met];
  uint ww = layer->getWidth();
  uint sp = layer->getSpacing();
  if (sp == 0) {
    sp = layer->getPitch() - ww;
  }
  uint half_width = sp / 2;

  uint dir = this->_dir;

  int ll[2] = {bboxLL[0], bboxLL[1]};
  int ur[2];
  ur[dir] = ll[dir];
  ur[!dir] = bboxUR[!dir];

  ll[!this->_dir] = ll[!this->_dir] - half_width;
  ur[!this->_dir] = ll[!this->_dir] + half_width;

  int xcnt = 1;
  while (ur[dir] <= bboxUR[dir]) {
    this->createNetSingleWire_cntx(met, dirName, xcnt++, dir, ll, ur, 0);
  }
  return xcnt;
}

int extRCModel::writeBenchWires_DB_diag(extMeasure* measure)
{
  bool lines_3 = true;
  bool lines_2 = true;
  int diag_width = 0;
  int diag_space = 0;
  dbTechLayer* layer
      = measure->_create_net_util.getRoutingLayer()[measure->_overMet];
  diag_width = layer->getWidth();
  diag_space = layer->getSpacing();
  if (diag_space == 0) {
    diag_space = layer->getPitch() - diag_width;
  }

  mkNet_prefix(measure, "");

  uint mover = measure->_overMet;
  uint munder = measure->_met;
  measure->_met = mover;

  measure->_skip_delims = true;
  uint grid_gap_cnt = 20;

  int gap = grid_gap_cnt * (measure->_minWidth + measure->_minSpace);
  int bboxLL[2];
  bboxLL[measure->_dir] = measure->_ur[measure->_dir];
  bboxLL[!measure->_dir] = measure->_ll[!measure->_dir];

  measure->clean2dBoxTable(measure->_met, false);

  uint idCnt = 1;
  if (!lines_3) {
    measure->createNetSingleWire(
        _wireDirName,
        idCnt,
        diag_width,
        diag_space,
        measure->_dir);  // 0 to force min width and spacing
  }
  idCnt++;

  uint SS1;
  SS1 = measure->_s_nm;

  measure->createNetSingleWire(
      _wireDirName, idCnt, diag_width, diag_space, measure->_dir);
  idCnt++;

  uint ss2 = SS1;
  int SS4 = measure->_s_nm;  // wire #4
  if (measure->_s_nm == 0) {
    ss2 = 0;
    SS4 = measure->_minSpace;
  }

  measure->_met = munder;
  measure->createNetSingleWire(
      _wireDirName, idCnt, measure->_w_nm, ss2, measure->_dir);
  idCnt++;
  measure->_met = mover;

  if (!lines_2) {
    measure->createNetSingleWire(
        _wireDirName, idCnt, diag_width, SS4, measure->_dir);
    idCnt++;
  }
  if (!lines_3) {
    measure->createNetSingleWire(
        _wireDirName, idCnt, diag_width, diag_space, measure->_dir);
    idCnt++;
  }

  measure->_met = munder;
  measure->_overMet = mover;

  bool grid_context = false;

  int extend_blockage = (measure->_minWidth + measure->_minSpace);
  int bboxUR[2]
      = {measure->_ur[0] + extend_blockage, measure->_ur[1] + extend_blockage};
  bboxLL[0] -= extend_blockage;
  bboxLL[1] -= extend_blockage;

  if (grid_context) {
    if (grid_context && (measure->_underMet > 0 || measure->_overMet > 0)) {
      measure->createContextGrid(
          _wireDirName, bboxLL, bboxUR, measure->_underMet);
      measure->createContextGrid(
          _wireDirName, bboxLL, bboxUR, measure->_overMet);
    } else {
      double pitchMult = 1.0;

      measure->clean2dBoxTable(measure->_underMet, true);
      measure->createContextObstruction(_wireDirName,
                                        bboxLL[0],
                                        bboxLL[1],
                                        bboxUR,
                                        measure->_underMet,
                                        pitchMult);

      measure->clean2dBoxTable(measure->_overMet, true);
      measure->createContextObstruction(_wireDirName,
                                        bboxLL[0],
                                        bboxLL[1],
                                        bboxUR,
                                        measure->_overMet,
                                        pitchMult);
    }
  }
  measure->_ur[measure->_dir] += gap;

  int main_xlo, main_ylo, main_xhi, main_yhi;
  measure->getBox(measure->_met, false, main_xlo, main_ylo, main_xhi, main_yhi);
  return 1;
}

}  // namespace rcx
