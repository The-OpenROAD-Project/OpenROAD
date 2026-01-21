// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include <string.h>  // NOLINT(modernize-deprecated-headers): for strdup()

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <vector>

#include "odb/db.h"
#include "odb/dbSet.h"
#include "odb/dbTypes.h"
#include "parse.h"
#include "rcx/array1.h"
#include "rcx/extRCap.h"
#include "rcx/extSpef.h"
#include "rcx/extViaModel.h"
#include "rcx/extprocess.h"
#include "rcx/util.h"
#include "utl/Logger.h"

using odb::dbBTerm;
using odb::dbCapNode;
using odb::dbCCSeg;
using odb::dbIoType;
using odb::dbNet;
using odb::dbObstruction;
using odb::dbSet;
using odb::dbTechLayer;

namespace rcx {

extMetRCTable* extRCModel::initCapTables(uint32_t layerCnt, uint32_t widthCnt)
{
  createModelTable(1, layerCnt);
  for (uint32_t kk = 0; kk < _modelCnt; kk++) {
    _dataRateTable->add(0.0);
  }

  // _modelTable[0]->allocateInitialTables(10, true, true, true);
  _modelTable[0]->allocateInitialTables(widthCnt, true, true, true);
  return _modelTable[0];
}
AthPool<extDistRC>* extMetRCTable::getRCPool()
{
  return _rcPoolPtr;
}

uint32_t extMain::GenExtRules(const char* rulesFileName)
{
  uint32_t widthCnt = 12;
  uint32_t layerCnt = _tech->getRoutingLayerCount() + 1;

  extRCModel* extRulesModel = new extRCModel(layerCnt, "TYPICAL", logger_);
  this->_modelTable->add(extRulesModel);
  extRulesModel->setDiagModel(1);
  // _modelTable->add(m);
  // extRCModel *model= _modelTable->get(0);
  extRulesModel->setOptions("./", "", false);

  extMetRCTable* rcModel = extRulesModel->initCapTables(layerCnt, widthCnt);

  AthPool<extDistRC>* rcPool = rcModel->getRCPool();
  extMeasure m(nullptr);
  m._diagModel = 1;
  uint32_t openWireNumber = 1;

  char buff[2000];
  sprintf(buff, "%s.log", rulesFileName);
  FILE* logFP = fopen(buff, "w");

  Parser* p = new Parser(logger_);
  Parser* w = new Parser(logger_);

  int prev_sep = 0;
  int prev_width = 0;
  int n = 0;

  dbSet<dbNet> nets = _block->getNets();
  dbSet<dbNet>::iterator itr;
  for (itr = nets.begin(); itr != nets.end(); ++itr) {
    dbNet* net = *itr;
    const char* netName = net->getConstName();
    // if (strcmp(netName, "O4_M2oM0_W200W200_S02400S02400_3")==0)
    // fprintf(stdout,"%s\n", netName);

    uint32_t wireCnt = 0;
    uint32_t viaCnt = 0;
    uint32_t len = 0;
    uint32_t layerCnt = 0;
    uint32_t layerTable[20];

    net->getNetStats(wireCnt, viaCnt, len, layerCnt, layerTable);
    uint32_t wcnt = p->mkWords(netName, "_");

    // Read Via patterns - dkf 12262023
    // via pattern: V2.W2.M5.M6.DX520.DY1320.C2.V56_1x2_VH_S
    if (p->getFirstChar() == 'V') {
      if (!rcModel->GetViaRes(p, w, net, logFP)) {
        break;
      }
      continue;
    }
    if (wcnt < 5) {
      continue;
    }

    // dkf 12302023 -- NOTE Original Patterns start with: O6_ U6_ OU6_ DU6_ R6_
    // example: R6_M6oM0_W440W440_S02640S03520_3

    if (rcModel->SkipPattern(p, net, logFP)) {
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

    uint32_t wireNum = p->getInt(wcnt - 1);
    // if (wireNum != targetWire / 2)
    //   continue;

    // fprintf(logFP, "%s\n", netName);
    bool diag = false;
    bool ResModel = false;

    uint32_t met = p->getInt(0, 1);
    uint32_t overMet = 0;
    uint32_t underMet = 0;

    m._overMet = -1;
    m._underMet = -1;
    m._overUnder = false;
    m._over = false;
    m._diag = false;
    m._res = false;

    char* overUnderToken = strdup(p->get(1));  // M2oM1uM3
    int wCnt = w->mkWords(overUnderToken, "ou");
    if (wCnt < 2) {
      free(overUnderToken);
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
    free(overUnderToken);
    // TODO DIAGUNDER
    m._met = met;

    if (w->mkWords(p->get(2), "W") <= 0) {
      continue;
    }

    double w1 = w->getDouble(0) / 1000;

    m._w_m = w1;
    m._w_nm = ceil(m._w_m * 1000);

    if (w->mkWords(p->get(3), "S") <= 0) {
      continue;
    }

    double s1 = w->getDouble(0) / 1000;
    double s2 = w->getDouble(1) / 1000;

    m._s_m = s1;
    m._s_nm = ceil(m._s_m * 1000);
    m._s2_m = s2;
    m._s2_nm = ceil(m._s2_m * 1000);

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
    if (ResModel) {
      R *= 2;
    }

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
    if (strstr(netName, "cntxM") != nullptr) {
      continue;
    }

    extDistRC* rc = rcPool->alloc();
    if (ResModel) {
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

    uint32_t wireNum2 = 2;
    m._open = false;
    m._over1 = false;
    if (wireNum == openWireNumber) {  // default openWireNumber=1
      if (ResModel || m._diag) {
        continue;
      }
      m._open = true;
    } else if (wireNum == wireNum2) {
      if (ResModel || m._diag) {
        continue;
      }
      m._over1 = true;
    } else if (wireNum != 3) {
      continue;
    }

    /* TODO
  if (ResModel) {
    if ((wireNum != targetWire/2) &&  wireNum!=3)
      continue;
  } else {
    if (wireNum != targetWire/2)
      continue;
  }
*/
    m._tmpRC = rc;
    rcModel->addRCw(&m);
    prev_sep = m._s_nm;
    prev_width = m._w_nm;
  }
  rcModel->mkWidthAndSpaceMappings();
  extRulesModel->writeRules((char*) rulesFileName, false);

  fclose(logFP);
  return n;
}

double extMain::getTotalCouplingCap(dbNet* net,
                                    const char* filterNet,
                                    uint32_t corner)
{
  double cap = 0.0;
  dbSet<dbCapNode> capNodes = net->getCapNodes();
  dbSet<dbCapNode>::iterator citr;

  for (citr = capNodes.begin(); citr != capNodes.end(); ++citr) {
    dbCapNode* n = *citr;
    dbSet<dbCCSeg> ccSegs = n->getCCSegs();
    dbSet<dbCCSeg>::iterator ccitr;

    for (ccitr = ccSegs.begin(); ccitr != ccSegs.end(); ++ccitr) {
      dbCCSeg* cc = *ccitr;
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

uint32_t extMain::benchVerilog(FILE* fp)
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
uint32_t extMain::benchVerilog_bterms(FILE* fp,
                                      dbIoType iotype,
                                      const char* prefix,
                                      const char* postfix,
                                      bool skip_postfix_last)
{
  int n = 0;
  dbSet<dbNet> nets = _block->getNets();
  dbSet<dbNet>::iterator itr;
  for (itr = nets.begin(); itr != nets.end(); ++itr) {
    dbNet* net = *itr;

    dbSet<dbBTerm> bterms = net->getBTerms();
    dbSet<dbBTerm>::iterator itr;
    for (itr = bterms.begin(); itr != bterms.end(); ++itr) {
      dbBTerm* bterm = *itr;
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
uint32_t extMain::benchVerilog_assign(FILE* fp)
{
  int n = 0;
  dbSet<dbNet> nets = _block->getNets();
  dbSet<dbNet>::iterator itr;
  for (itr = nets.begin(); itr != nets.end(); ++itr) {
    dbNet* net = *itr;

    const char* bterm1 = nullptr;
    const char* bterm2 = nullptr;
    dbSet<dbBTerm> bterms = net->getBTerms();
    dbSet<dbBTerm>::iterator itr;
    for (itr = bterms.begin(); itr != bterms.end(); ++itr) {
      dbBTerm* bterm = *itr;

      if (bterm->getIoType() == dbIoType::OUTPUT) {
        bterm1 = bterm->getConstName();
        break;
      }
    }
    for (itr = bterms.begin(); itr != bterms.end(); ++itr) {
      dbBTerm* bterm = *itr;

      if (bterm->getIoType() == dbIoType::INPUT) {
        bterm2 = bterm->getConstName();
        break;
      }
    }
    fprintf(fp, "  assign %s = %s ;\n", bterm1, bterm2);
  }
  return n;
}
uint32_t extRCModel::benchDB_WS(extMainOptions* opt, extMeasure* measure)
{
  auto widthTable = std::make_unique<Array1D<double>>(4);
  auto spaceTable = std::make_unique<Array1D<double>>(4);
  Array1D<double>* wTable = &opt->_widthTable;
  Array1D<double>* sTable = &opt->_spaceTable;
  Array1D<double>* gTable = &opt->_gridTable;

  uint32_t cnt = 0;
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
    for (uint32_t ii = 0; ii < gTable->getCnt(); ii++) {
      // double s = minWidth + pitch * (gTable->get(ii) - 1);
      double s = spacing * gTable->get(ii);
      spaceTable->add(s);
      double w = minWidth * gTable->get(ii);
      widthTable->add(w);
    }
  } else if (opt->_nondefault_lef_rules) {
    return 0;
    wTable->resetCnt();
    sTable->resetCnt();
    dbSet<odb::dbTechNonDefaultRule> nd_rules
        = opt->_tech->getNonDefaultRules();
    dbSet<odb::dbTechNonDefaultRule>::iterator nditr;
    odb::dbTechLayerRule* tst_rule;
    //		odb::dbTechNonDefaultRule  *wdth_rule = nullptr;

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
  } else {
    if (measure->_diag) {
      spaceTable->add(0.0);
    }
    if (!opt->_res_patterns) {
      for (uint32_t ii = 0; ii < sTable->getCnt(); ii++) {
        double s = spacing * sTable->get(ii);
        if (sTable->get(ii) == 0 && measure->_diag) {
          continue;
        }
        spaceTable->add(s);
      }
    } else {
      spaceTable->add(0);
      spaceTable->add(spacing);

      for (uint32_t ii = 1; ii < 2; ii++) {
        // double m = sTable->get(ii);
        double s = pitch * ii;
        double s1 = s - minWidth;

        if (std::fabs(spacing - s1) > DBL_EPSILON) {
          spaceTable->add(s1);
        }
        spaceTable->add(s);
      }
      for (uint32_t ii = 2; ii < 4; ii++) {
        // double m = sTable->get(ii);
        double s = pitch * ii;
        double s1 = s - minWidth;
        double s2 = s1 - minWidth / 2;

        spaceTable->add(s2);
        spaceTable->add(s1);
        spaceTable->add(s);
      }
      spaceTable->add(pitch * 4);
    }
    for (uint32_t ii = 0; ii < wTable->getCnt(); ii++) {
      double w = minWidth * wTable->get(ii);
      widthTable->add(w);
    }
  }
  bool use_symmetric_widths_spacings = false;
  if (!use_symmetric_widths_spacings) {
    for (uint32_t ii = 0; ii < widthTable->getCnt(); ii++) {
      double w = widthTable->get(ii);  // layout
      double w2 = w;
      if (!opt->_res_patterns) {
        for (uint32_t jj = 0; jj < spaceTable->getCnt(); jj++) {
          double s = spaceTable->get(jj);  // layout
          double s2 = s;

          measure->setTargetParams(w, s, 0.0, 0, 0, w2, s2);
          // measureResistance(measure, ro, top_widthR, bot_widthR, thicknessR);
          // measurePatternVar(measure, top_width, bot_width, thickness,
          // measure->_wireCnt, nullptr);
          writeBenchWires_DB(measure);

          cnt++;
        }
      } else {
        for (uint32_t jj = 0; jj < spaceTable->getCnt(); jj++) {
          double s = spaceTable->get(jj);  // layout
          for (uint32_t kk = jj; kk < spaceTable->getCnt(); kk++) {
            double s2 = spaceTable->get(kk);

            measure->setTargetParams(w, s, 0.0, 0, 0, w2, s2);
            writeBenchWires_DB_res(measure);

            cnt++;
          }
        }
      }
    }
  } else {
    /* REQUIRED Testing
            for (uint32_t ii = 0; ii < wTable->getCnt(); ii++) {
                    double w = wTable->get(ii); // layout
                    for (uint32_t iii = 0; iii < wTable->getCnt(); iii++) {
                            double w2 = wTable->get(iii);
                            for (uint32_t jj = 0; jj < sTable->getCnt(); jj++) {
                                    double s = sTable->get(jj); // layout
                                    for (uint32_t jjj = 0; jjj <
       sTable->getCnt(); jjj++) { double s2 = sTable->get(jjj);

                                            for (uint32_t kk = 0; kk <
       thTable->getCnt(); kk++) { double tt = thTable->get(kk); // layout if
       (!opt->_thListFlag) // multiplier tt *= t;

                                                    double top_width = w;
                                                    double top_widthR = w;

                                                    double bot_width = w;
                                                    double bot_widthR = w;

                                                    double thickness = tt;
                                                    double thicknessR = tt;


                                                    measure->setTargetParams(w,
       s, 0.0, t, h, w2, s2); measureResistance(measure, ro, top_widthR,
       bot_widthR, thicknessR); measurePatternVar(measure, top_width, bot_width,
       thickness, measure->_wireCnt, nullptr);

                                                    cnt++;
                                            }
                                    }
                            }
                    }
            }
            */
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
  // mkFileNames(measure, "");
  mkNet_prefix(measure, "");
  measure->_skip_delims = true;
  uint32_t grid_gap_cnt = 40;

  int gap = grid_gap_cnt * (measure->_minWidth + measure->_minSpace);
  // does NOT work measure->_ll[!measure->_dir] += gap;
  // measure->_ur[1] += gap;

  int n
      = measure->_wireCnt / 2;  // ASSUME odd number of wires, 2 will also work

  uint32_t w_layout = measure->_minWidth;
  uint32_t s_layout = measure->_minSpace;

  uint32_t WW = measure->_w_nm;
  uint32_t SS1 = measure->_s_nm;
  uint32_t WW2 = measure->_w2_nm;
  uint32_t SS2 = measure->_s2_nm;

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

  uint32_t idCnt = 1;
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

  // mkFileNames(measure, "");
  mkNet_prefix(measure, "");
  measure->_skip_delims = true;
  uint32_t grid_gap_cnt = 40;

  int gap = grid_gap_cnt * (measure->_minWidth + measure->_minSpace);
  // does NOT work measure->_ll[!measure->_dir] += gap;
  // measure->_ur[1] += gap;
  int bboxLL[2];
  bboxLL[measure->_dir] = measure->_ur[measure->_dir];
  bboxLL[!measure->_dir] = measure->_ll[!measure->_dir];

  int n
      = measure->_wireCnt / 2;  // ASSUME odd number of wires, 2 will also work

  if (measure->_s_nm == 0 && !measure->_diag) {
    n = 1;
  }

  uint32_t w_layout = measure->_minWidth;
  uint32_t s_layout = measure->_minSpace;

  measure->clean2dBoxTable(measure->_met, false);

  uint32_t idCnt = 1;
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

  uint32_t WW = measure->_w_nm;
  uint32_t SS1;
  //	if (measure->_diag)
  //		SS1= 2*measure->_minSpace;
  //	else
  SS1 = measure->_s_nm;
  uint32_t WW2 = measure->_w2_nm;
  uint32_t SS2 = measure->_s2_nm;

  uint32_t base;
  if (n > 1) {
    cnt++;
    measure->createNetSingleWire(_wireDirName, idCnt, WW, s_layout);
    idCnt++;

    cnt++;
    if (!measure->_diag) {
      measure->createNetSingleWire(_wireDirName, idCnt, WW, SS1);
      base = measure->_ll[measure->_dir] + WW / 2;
      idCnt++;
      cnt++;
      measure->createNetSingleWire(_wireDirName, idCnt, WW2, SS2);
      idCnt++;
    } else {
      uint32_t met_tmp = measure->_met;
      measure->_met = measure->_overMet;
      uint32_t ss2 = SS1;
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
    if (measure->_wireCnt == 3) {  // dkf: for option -wireCnt 3
      cnt++;
      measure->createNetSingleWire(_wireDirName, 2, w_layout, s_layout);
      idCnt++;
    }
    base = measure->_ll[measure->_dir] + WW / 2;
    cnt++;
    measure->createNetSingleWire(
        _wireDirName, 3, WW2, SS2);  // SS2 = variable spacing
    idCnt++;
    if (measure->_wireCnt == 3) {  // dkf: for option -wireCnt 3
      cnt++;
      measure->createNetSingleWire(
          _wireDirName, 4, w_layout, SS2);  // SS2 = variable spacing
      idCnt++;
    }
  }

  if (measure->_diag) {
    return cnt;

    int met;
    if (measure->_overMet > 0) {
      met = measure->_overMet;
    } else if (measure->_underMet > 0) {
      met = measure->_underMet;
    }

    double minWidth = measure->_minWidth;
    double minSpace = measure->_minSpace;
    double min_pitch = minWidth + minSpace;
    measure->clean2dBoxTable(met, false);
    int i;
    uint32_t begin
        = base - ceil(measure->_seff * 1000) + ceil(minWidth * 1000) / 2;
    for (i = 0; i < n + 1; i++) {
      measure->createDiagNetSingleWire(_wireDirName,
                                       idCnt,
                                       begin,
                                       ceil(1000 * minWidth),
                                       ceil(1000 * minSpace),
                                       measure->_dir);
      begin -= ceil(min_pitch * 1000);
      idCnt++;
    }
    measure->_ur[measure->_dir] += grid_gap_cnt * (w_layout + s_layout);
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
    // measure->createContextNets(_wireDirName, bboxLL, bboxUR,
    // measure->_underMet, pitchMult);
    measure->createContextObstruction(_wireDirName,
                                      bboxLL[0],
                                      bboxLL[1],
                                      bboxUR,
                                      measure->_underMet,
                                      pitchMult);

    measure->clean2dBoxTable(measure->_overMet, true);
    // measure->createContextNets(_wireDirName, bboxLL, bboxUR,
    // measure->_overMet, pitchMult);
    measure->createContextObstruction(_wireDirName,
                                      bboxLL[0],
                                      bboxLL[1],
                                      bboxUR,
                                      measure->_overMet,
                                      pitchMult);
  }

  measure->_ur[measure->_dir] += gap + extend_blockage_gap;

  //	double mainNetStart= X[0];
  int main_xlo, main_ylo, main_xhi, main_yhi;
  measure->getBox(measure->_met, false, main_xlo, main_ylo, main_xhi, main_yhi);
  if (measure->_underMet > 0) {
    // double h = _process->getConductor(measure->_underMet)->_height;
    // double t = _process->getConductor(measure->_underMet)->_thickness;
    /* KEEP -- REMINDER
                    dbTechLayer
       *layer=measure->_tech->findRoutingLayer(_underMet); uint32_t minWidth=
       layer->getWidth(); uint32_t minSpace= layer->getSpacing(); uint32_t
       pitch= 1000*((minWidth+minSpace)*pitchMult)/1000; uint32_t offset=
       2*(minWidth+minSpace); int start= mainNetStart+offset; contextRaphaelCnt=
       measure->writeRaphael3D(fp, measure->_underMet, true, mainNetStart, h,
       t);
    */
    // contextRaphaelCnt = measure->writeRaphael3D(fp, measure->_underMet, true,
    // low, h, t);
  }

  if (measure->_overMet > 0) {
    // double h = _process->getConductor(measure->_overMet)->_height;
    // double t = _process->getConductor(measure->_overMet)->_thickness;
    /* KEEP -- REMINDER
                    dbTechLayer
       *layer=measure->_tech->findRoutingLayer(_overMet); uint32_t minWidth=
       layer->getWidth(); uint32_t minSpace= layer->getSpacing(); uint32_t
       pitch= 1000*((minWidth+minSpace)*pitchMult)/1000; uint32_t offset=
       2*(minWidth+minSpace); int start= mainNetStart+offset; contextRaphaelCnt
       += measure->writeRaphael3D(fp, measure->_overMet, true, mainNetStart, h,
       t);
    */
    // contextRaphaelCnt += measure->writeRaphael3D(fp, measure->_overMet, true,
    // low, h, t);
  }
  // fprintf(stdout, "\nOBS %d %d %d %d %d\n", met, x, y, bboxUR[0], bboxUR[1]);
  // dbTechLayer* layer = _tech->findRoutingLayer(met);
  // dbObstruction::create(_block, layer, x, y, bboxUR[0], bboxUR[1]);
  return 1;
}
uint32_t extMeasure::getPatternExtend()
{
  int extend_blockage = (this->_minWidth + this->_minSpace);

  if (this->_overMet > 0) {
    dbTechLayer* layer
        = this->_create_net_util.getRoutingLayer()[this->_overMet];
    uint32_t ww = layer->getWidth();
    uint32_t sp = layer->getSpacing();
    if (sp == 0) {
      sp = layer->getPitch() - ww;
    }

    extend_blockage = sp;
  }
  if (this->_underMet > 0) {
    dbTechLayer* layer
        = this->_create_net_util.getRoutingLayer()[this->_underMet];
    uint32_t ww = layer->getWidth();
    uint32_t sp = layer->getSpacing();
    if (sp == 0) {
      sp = layer->getPitch() - ww;
    }

    extend_blockage = std::max<uint32_t>(extend_blockage, sp);
  }
  return extend_blockage;
}
uint32_t extMeasure::createContextObstruction(const char* dirName,
                                              int x,
                                              int y,
                                              int bboxUR[2],
                                              int met,
                                              double pitchMult)
{
  if (met <= 0) {
    return 0;
  }

  // fprintf(stdout, "\nOBS %d %d %d %d %d\n", met, x, y, bboxUR[0], bboxUR[1]);
  dbTechLayer* layer = _tech->findRoutingLayer(met);
  dbObstruction::create(_block, layer, x, y, bboxUR[0], bboxUR[1]);
  return 1;
}

uint32_t extMeasure::createContextGrid(char* dirName,
                                       const int bboxLL[2],
                                       const int bboxUR[2],
                                       int met,
                                       int s_layout)
{
  if (met <= 0) {
    return 0;
  }
  dbTechLayer* layer = this->_create_net_util.getRoutingLayer()[met];
  uint32_t ww = layer->getWidth();
  uint32_t sp = layer->getSpacing();
  if (sp == 0) {
    sp = layer->getPitch() - ww;
  }
  uint32_t half_width = sp / 2;

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
    uint32_t d = !_dir;
    ll[d] = ur[d] + sp;
    ur[d] = ll[d] + ww;
  }
  return xcnt;
}

uint32_t extMeasure::createContextGrid_dir(char* dirName,
                                           const int bboxLL[2],
                                           const int bboxUR[2],
                                           int met)
{
  if (met <= 0) {
    return 0;
  }

  dbTechLayer* layer = this->_create_net_util.getRoutingLayer()[met];
  uint32_t ww = layer->getWidth();
  uint32_t sp = layer->getSpacing();
  if (sp == 0) {
    sp = layer->getPitch() - ww;
  }
  uint32_t half_width = sp / 2;

  uint32_t dir = this->_dir;

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

  uint32_t mover = measure->_overMet;
  uint32_t munder = measure->_met;
  measure->_met = mover;

  // mkFileNames(measure, "");
  measure->_skip_delims = true;
  uint32_t grid_gap_cnt = 20;

  int gap = grid_gap_cnt * (measure->_minWidth + measure->_minSpace);
  // does NOT work measure->_ll[!measure->_dir] += gap;
  // measure->_ur[1] += gap;
  int bboxLL[2];
  bboxLL[measure->_dir] = measure->_ur[measure->_dir];
  bboxLL[!measure->_dir] = measure->_ll[!measure->_dir];

  measure->clean2dBoxTable(measure->_met, false);

  uint32_t idCnt = 1;
  if (!lines_3) {
    measure->createNetSingleWire(
        _wireDirName,
        idCnt,
        diag_width,
        diag_space,
        measure->_dir);  // 0 to force min width and spacing
  }
  idCnt++;

  uint32_t SS1;
  //	if (measure->_diag)
  //		SS1= 2*measure->_minSpace;
  //	else
  SS1 = measure->_s_nm;

  measure->createNetSingleWire(
      _wireDirName, idCnt, diag_width, diag_space, measure->_dir);
  idCnt++;

  uint32_t ss2 = SS1;
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
    if (measure->_underMet > 0 || measure->_overMet > 0) {
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
  if (measure->_underMet > 0) {
    // double h = _process->getConductor(measure->_underMet)->_height;
    // double t = _process->getConductor(measure->_underMet)->_thickness;
    /* KEEP -- REMINDER
                    dbTechLayer
       *layer=measure->_tech->findRoutingLayer(_underMet); uint32_t minWidth=
       layer->getWidth(); uint32_t minSpace= layer->getSpacing(); uint32_t
       pitch= 1000*((minWidth+minSpace)*pitchMult)/1000; uint32_t offset=
       2*(minWidth+minSpace); int start= mainNetStart+offset; contextRaphaelCnt=
       measure->writeRaphael3D(fp, measure->_underMet, true, mainNetStart, h,
       t);
    */
    // contextRaphaelCnt = measure->writeRaphael3D(fp, measure->_underMet, true,
    // low, h, t);
  }

  if (measure->_overMet > 0) {
    // double h = _process->getConductor(measure->_overMet)->_height;
    // double t = _process->getConductor(measure->_overMet)->_thickness;
    /* KEEP -- REMINDER
                    dbTechLayer
       *layer=measure->_tech->findRoutingLayer(_overMet); uint32_t minWidth=
       layer->getWidth(); uint32_t minSpace= layer->getSpacing(); uint32_t
       pitch= 1000*((minWidth+minSpace)*pitchMult)/1000; uint32_t offset=
       2*(minWidth+minSpace); int start= mainNetStart+offset; contextRaphaelCnt
       += measure->writeRaphael3D(fp, measure->_overMet, true, mainNetStart, h,
       t);
    */
    // contextRaphaelCnt += measure->writeRaphael3D(fp, measure->_overMet, true,
    // low, h, t);
  }
  // fprintf(stdout, "\nOBS %d %d %d %d %d\n", met, x, y, bboxUR[0], bboxUR[1]);
  // dbTechLayer* layer = _tech->findRoutingLayer(met);
  // dbObstruction::create(_block, layer, x, y, bboxUR[0], bboxUR[1]);
  return 1;
}

}  // namespace rcx
