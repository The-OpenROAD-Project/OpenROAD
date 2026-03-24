// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#include "rcx/extModelGen.h"

#include <string.h>  // NOLINT(modernize-deprecated-headers): for strdup()

#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <list>
#include <string>
#include <vector>

#include "odb/dbSet.h"
#include "parse.h"
#include "rcx/extRCap.h"
#include "rcx/util.h"
#include "utl/Logger.h"

namespace rcx {

uint32_t extRCModel::GenExtModel(std::list<std::string>& corner_list,
                                 const char* out_file,
                                 const char* comment,
                                 const char* version,
                                 int pattern)
{
  extModelGen* g = (extModelGen*) this;
  bool binary = false;
  uint32_t corner_cnt = corner_list.size();
  FILE* outFP
      = g->InitWriteRules(out_file, corner_list, comment, false, corner_cnt);
  for (uint32_t ii = 0; ii < corner_cnt; ii++) {
    extMetRCTable* corner_model = getMetRCTable(ii);

    corner_model->mkWidthAndSpaceMappings();

    g->writeRules(outFP, binary, ii, ii);
  }
  fclose(outFP);
  return 0;
}

uint32_t extMain::GenExtModel(std::list<std::string> spef_file_list,
                              std::list<std::string> corner_list,
                              const char* out_file,
                              const char* comment,
                              const char* version,
                              int pattern)
{
  std::vector<std::string> corner_name;

  std::list<std::string>::iterator it1;
  for (it1 = corner_list.begin(); it1 != corner_list.end(); ++it1) {
    const std::string& str = *it1;
    corner_name.push_back(str);
  }
  uint32_t widthCnt = 12;
  uint32_t layerCnt = _tech->getRoutingLayerCount() + 1;
  FILE* outFP = nullptr;

  bool binary = false;
  uint32_t fileCnt = spef_file_list.size();
  uint32_t cnt = 0;
  std::list<std::string>::iterator it;
  for (it = spef_file_list.begin(); it != spef_file_list.end(); ++it) {
    const std::string& str = *it;
    const char* filename = str.c_str();
    readSPEF((char*) filename,
             nullptr,
             /*force*/ true,
             false,
             nullptr,
             false,
             false,
             false,
             -0.5,
             0.0,
             1.0,
             false,
             false,
             nullptr,
             false,
             -1,
             0.0,
             0.0,
             nullptr,
             nullptr,
             nullptr,
             nullptr,
             nullptr,
             -1,
             0,
             false,
             false,
             /*testParsing*/ false,
             false,
             false /*diff*/,
             false /*calibrate*/,
             0);

    extModelGen* extRulesModel = new extModelGen(layerCnt, "TYPICAL", logger_);
    extRulesModel->setExtMain(this);

    uint32_t diagOption = 1;
    char* out = (char*) corner_name[cnt].c_str();
    // if (cnt == 1)
    //    out = "2.model";
    extRulesModel->ReadRCDB(_block, widthCnt, diagOption, out);
    if (outFP == nullptr) {
      outFP = extRulesModel->InitWriteRules(
          out_file, corner_list, comment, binary, fileCnt);
    }

    extRulesModel->writeRules(outFP, binary, cnt);  // always first model
    cnt++;
  }
  fclose(outFP);
  return 0;
}
void extModelGen::writeRules(FILE* fp, bool binary, uint32_t mIndex, int corner)
{
  uint32_t m = 0;
  bool writeRes = true;

  extMetRCTable* rcTable0 = getMetRCTable(0);  // orig call

  extMetRCTable* rcTable;
  if (corner >= 0) {
    rcTable = getMetRCTable(corner);
  } else {
    rcTable = getMetRCTable(m);
  }

  uint32_t layerCnt = getLayerCnt();
  uint32_t diagModel = getDiagModel();

  fprintf(fp, "\nDensityModel %d\n", mIndex);

  for (uint32_t ii = 1; ii < layerCnt; ii++) {
    if (writeRes) {
      writeRulesPattern(0,
                        ii,
                        m,
                        rcTable->_resOver[ii],
                        rcTable->_resOver[0],
                        "RESOVER",
                        fp,
                        binary);
    }
    writeRulesPattern(0,
                      ii,
                      m,
                      rcTable->_capOver[ii],
                      rcTable->_capOver[0],
                      "OVER",
                      fp,
                      binary);
    writeRulesPattern(0,
                      ii,
                      m,
                      rcTable->_capOver_open[ii][0],
                      rcTable->_capOver_open[0][0],
                      "OVER0",
                      fp,
                      binary);
    writeRulesPattern(0,
                      ii,
                      m,
                      rcTable->_capOver_open[ii][1],
                      rcTable->_capOver_open[0][1],
                      "OVER1",
                      fp,
                      binary);
    writeRulesPattern(1,
                      ii,
                      m,
                      rcTable->_capUnder[ii],
                      rcTable->_capUnder[0],
                      "UNDER",
                      fp,
                      binary);
    writeRulesPattern(1,
                      ii,
                      m,
                      rcTable->_capUnder_open[ii][0],
                      rcTable->_capUnder_open[0][0],
                      "UNDER0",
                      fp,
                      binary);
    writeRulesPattern(1,
                      ii,
                      m,
                      rcTable->_capUnder_open[ii][1],
                      rcTable->_capUnder_open[0][1],
                      "UNDER1",
                      fp,
                      binary);

    if (rcTable->_capDiagUnder[ii] != nullptr) {
      if (diagModel == 1) {
        rcTable->_capDiagUnder[ii]->writeRulesDiagUnder(fp, binary);
      }
      if (diagModel == 2) {
        rcTable->_capDiagUnder[ii]->writeRulesDiagUnder2(fp, binary);
      }
    } else if ((m > 0) && (rcTable0->_capDiagUnder[ii] != nullptr)) {
      if (diagModel == 1) {
        rcTable0->_capDiagUnder[ii]->writeRulesDiagUnder(fp, binary);
      }
      if (diagModel == 2) {
        rcTable0->_capDiagUnder[ii]->writeRulesDiagUnder2(fp, binary);
      }
    } else if (m == 0) {
      // TODO logger_->info( RCX, 220, "Cannot write <DIAGUNDER> rules for
      // <DensityModel> {} and layer {}", m, ii);
      fprintf(
          stdout,
          "Cannot write <DIAGUNDER> rules for <DensityModel> %d and layer %d",
          m,
          ii);
    }
    if ((ii > 1) && (ii < layerCnt - 1)) {
      writeRulesPattern(2,
                        ii,
                        m,
                        rcTable->_capOverUnder[ii],
                        rcTable->_capOverUnder[0],
                        "OVERUNDER",
                        fp,
                        binary);
      writeRulesPattern(2,
                        ii,
                        m,
                        rcTable->_capOverUnder_open[ii][0],
                        rcTable->_capOverUnder_open[0][0],
                        "OVERUNDER0",
                        fp,
                        binary);
      writeRulesPattern(2,
                        ii,
                        m,
                        rcTable->_capOverUnder_open[ii][1],
                        rcTable->_capOverUnder_open[0][1],
                        "OVERUNDER1",
                        fp,
                        binary);
    }
  }
  rcTable->writeViaRes(fp);
  fprintf(fp, "END DensityModel %d\n", mIndex);
}
uint32_t extRCModel::writeRulesPattern(uint32_t ou,
                                       uint32_t layer,
                                       int modelIndex,
                                       extDistWidthRCTable* table_m,
                                       extDistWidthRCTable* table_0,
                                       const char* patternKeyword,
                                       FILE* fp,
                                       bool binary)
{
  uint32_t cnt = 0;
  extDistWidthRCTable* table = nullptr;
  if (table_m != nullptr) {
    table = table_m;
  } else if ((modelIndex > 0) && (table_0 != nullptr)) {
    table = table_0;
  } else if (modelIndex == 0) {
    fprintf(stdout,
            "Cannot write %s rules for <DensityModel> %d and layer %d\n",
            patternKeyword,
            modelIndex,
            layer);
    // TODO logger_->info( RCX, 218, "Cannot write {} rules for <DensityModel>
    // {} and layer {}", patternKeyword, modelIndex, layer);
    return 0;
  }
  if (ou == 0) {
    cnt = table->writeRulesOver(fp, patternKeyword, binary);
  } else if (ou == 1) {
    cnt = table->writeRulesUnder(fp, patternKeyword, binary);
  } else if (ou == 2) {
    cnt = table->writeRulesOverUnder(fp, patternKeyword, binary);
  }

  return cnt;
}

uint32_t extDistWidthRCTable::writeRulesUnder(FILE* fp,
                                              const char* keyword,
                                              bool bin)
{
  uint32_t cnt = 0;
  fprintf(fp, "\nMetal %d %s\n", _met, keyword);

  writeWidthTable(fp, bin);
  uint32_t widthCnt = _widthTable->getCnt();

  for (uint32_t ii = _met + 1; ii < _layerCnt; ii++) {
    fprintf(fp, "\nMetal %d %s %d\n", _met, keyword, ii);

    uint32_t metIndex = getMetIndexUnder(ii);

    for (uint32_t jj = 0; jj < widthCnt; jj++) {
      cnt += _rcDistTable[metIndex][jj]->writeRules(
          fp, 0.001 * _widthTable->get(jj), false, bin);
    }
  }
  return cnt;
}
uint32_t extDistWidthRCTable::writeRulesDiagUnder2(FILE* fp, bool bin)
{
  uint32_t cnt = 0;
  fprintf(fp, "\nMetal %d DIAGUNDER\n", _met);

  writeWidthTable(fp, bin);
  uint32_t widthCnt = _widthTable->getCnt();
  writeDiagTablesCnt(fp, _met + 1, bin);

  for (uint32_t ii = _met + 1; ii < _met + 5 && ii < _layerCnt; ii++) {
    fprintf(fp, "\nMetal %d DIAGUNDER %d\n", _met, ii);
    writeDiagWidthTable(fp, ii, bin);
    uint32_t diagWidthCnt = _diagWidthTable[ii]->getCnt();
    writeDiagDistTable(fp, ii, bin);
    uint32_t diagDistCnt = _diagDistTable[ii]->getCnt();

    uint32_t metIndex = getMetIndexUnder(ii);

    for (uint32_t jj = 0; jj < widthCnt; jj++) {
      for (uint32_t kk = 0; kk < diagWidthCnt; kk++) {
        for (uint32_t ll = 0; ll < diagDistCnt; ll++) {
          cnt += _rcDiagDistTable[metIndex][jj][kk][ll]->writeDiagRules(
              fp,
              0.001 * _widthTable->get(jj),
              0.001 * _diagWidthTable[ii]->get(kk),
              0.001 * _diagDistTable[ii]->get(ll),
              false,
              bin);
        }
      }
    }
  }
  return cnt;
}
uint32_t extDistWidthRCTable::writeRulesDiagUnder(FILE* fp, bool bin)
{
  uint32_t cnt = 0;
  fprintf(fp, "\nMetal %d DIAGUNDER\n", _met);

  writeWidthTable(fp, bin);
  uint32_t widthCnt = _widthTable->getCnt();

  for (uint32_t ii = _met + 1; ii < _layerCnt; ii++) {
    fprintf(fp, "\nMetal %d DIAGUNDER %d\n", _met, ii);

    uint32_t metIndex = getMetIndexUnder(ii);

    for (uint32_t jj = 0; jj < widthCnt; jj++) {
      cnt += _rcDistTable[metIndex][jj]->writeRules(
          fp, 0.001 * _widthTable->get(jj), false, bin);
    }
  }
  return cnt;
}
int extRCModel::getMetIndexOverUnder(int met,
                                     int mUnder,
                                     int mOver,
                                     int layerCnt,
                                     int maxCnt)
{
  int n = layerCnt - met - 1;
  n *= mUnder - 1;
  n += mOver - met - 1;

  if ((n < 0) || (n >= maxCnt)) {
    return -1;
  }

  return n;
}
uint32_t extDistWidthRCTable::writeRulesOverUnder(FILE* fp,
                                                  const char* keyword,
                                                  bool bin)
{
  uint32_t cnt = 0;
  fprintf(fp, "\nMetal %d %s\n", _met, keyword);

  writeWidthTable(fp, bin);
  uint32_t widthCnt = _widthTable->getCnt();

  for (uint32_t mUnder = 1; mUnder < _met; mUnder++) {
    for (uint32_t mOver = _met + 1; mOver < _layerCnt; mOver++) {
      fprintf(fp, "\nMetal %d OVER %d UNDER %d\n", _met, mUnder, mOver);

      int metIndex = extRCModel::getMetIndexOverUnder(
          _met, mUnder, mOver, _layerCnt, _metCnt);
      assert(metIndex >= 0);

      for (uint32_t jj = 0; jj < widthCnt; jj++) {
        cnt += _rcDistTable[metIndex][jj]->writeRules(
            fp, 0.001 * _widthTable->get(jj), false, bin);
      }
    }
  }
  return cnt;
}
uint32_t extDistWidthRCTable::writeRulesOver(FILE* fp,
                                             const char* keyword,
                                             bool bin)
{
  uint32_t cnt = 0;
  // fprintf(fp, "\nMetal %d OVER\n", _met);
  fprintf(fp, "\nMetal %d %s\n", _met, keyword);

  writeWidthTable(fp, bin);
  uint32_t widthCnt = _widthTable->getCnt();

  for (uint32_t ii = 0; ii < _met; ii++) {
    fprintf(fp, "\nMetal %d %s %d\n", _met, keyword, ii);
    // fprintf(fp, "\nMetal %d OVER %d\n", _met, ii);

    for (uint32_t jj = 0; jj < widthCnt; jj++) {
      cnt += _rcDistTable[ii][jj]->writeRules(
          fp, 0.001 * _widthTable->get(jj), false, bin);
    }
  }
  return cnt;
}
uint32_t extDistWidthRCTable::writeRulesOver_res(FILE* fp, bool bin)
{
  uint32_t cnt = 0;
  fprintf(fp, "\nMetal %d RESOVER\n", _met);

  writeWidthTable(fp, bin);
  uint32_t widthCnt = _widthTable->getCnt();

  for (uint32_t ii = 0; ii < _met; ii++) {
    fprintf(fp, "\nMetal %d RESOVER %d\n", _met, ii);

    for (uint32_t jj = 0; jj < widthCnt; jj++) {
      cnt += _rcDistTable[ii][jj]->writeRules(
          fp, 0.001 * _widthTable->get(jj), false, bin);
    }
  }
  return cnt;
}
FILE* extModelGen::InitWriteRules(const char* name,
                                  std::list<std::string> corner_list,
                                  const char* comment,
                                  bool binary,
                                  uint32_t modelCnt)
{
  uint32_t diagModel = getDiagModel();
  int layerCnt = getLayerCnt();
  bool diag = getDiagFlag();

  //	FILE *fp= openFile("./", name, nullptr, "w");
  FILE* fp = fopen(name, "w");

  fprintf(fp, "Extraction Rules for rcx\n\n");
  fprintf(fp, "Version 1.2\n\n");
  if (diag || diagModel > 0) {
    if (diagModel == 1) {
      fprintf(fp, "DIAGMODEL ON\n\n");
    } else if (diagModel == 2) {
      fprintf(fp, "DIAGMODEL TRUE\n\n");
    }
  }
  fprintf(fp, "LayerCount %d\n", layerCnt - 1);

  fprintf(fp, "\nDensityRate %d ", modelCnt);
  for (uint32_t kk = 0; kk < modelCnt; kk++) {
    fprintf(fp, " %d", kk);
  }
  fprintf(fp, "\n");

  fprintf(fp, "\nCorners %ld : ", corner_list.size());
  std::list<std::string>::iterator it;
  for (it = corner_list.begin(); it != corner_list.end(); ++it) {
    const std::string& str = *it;
    fprintf(fp, " %s", str.c_str());
  }
  fprintf(fp, "\n");

  if (comment != nullptr && strlen(comment) > 0) {
    fprintf(fp, "\nCOMMENT : %s \n\n", comment);
  }

  return fp;
}
uint32_t extModelGen::ReadRCDB(odb::dbBlock* block,
                               uint32_t widthCnt,
                               uint32_t diagOption,
                               char* logFilePrefix)
{
  extMain* extMain = get_extMain();
  // ORIG: setDiagModel(1);
  setDiagModel(diagOption);
  //  setOptions("./", "", false, true, false, false);

  uint32_t layerCnt = getLayerCnt();
  extMetRCTable* rcModel = initCapTables(layerCnt, widthCnt);

  AthPool<extDistRC>* rcPool = rcModel->getRCPool();
  extMeasure m(nullptr);
  m._diagModel = 1;
  uint32_t openWireNumber = 1;

  char buff[2000];
  sprintf(buff, "%s.log", logFilePrefix);
  FILE* logFP = fopen(buff, "w");

  sprintf(buff, "%s.debug.log", logFilePrefix);
  FILE* dbg_logFP = fopen(buff, "w");

  Parser* p = new Parser(logger_);
  Parser* w = new Parser(logger_);

  int prev_sep = 0;
  int prev_width = 0;
  odb::dbSet<odb::dbNet> nets = block->getNets();
  odb::dbSet<odb::dbNet>::iterator itr;
  for (itr = nets.begin(); itr != nets.end(); ++itr) {
    odb::dbNet* net = *itr;
    const char* netName = net->getConstName();

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
    // example:

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

    double wLen = extMain->GetDBcoords2(len) * 1.0;
    double totCC = net->getTotalCouplingCap();
    double totGnd = net->getTotalCapacitance();
    double res = net->getTotalResistance();

    double contextCoupling = extMain->getTotalCouplingCap(net, "cntxM", 0);
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
    if (ResModel) {
      fprintf(dbg_logFP,
              "M%2d OVER %2d UNDER %2d W %.3f S1 %.3f S2 %.3f R %g LEN %g %g  "
              "%s --- ",
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
          dbg_logFP,
          "M%2d OVER %2d UNDER %2d W %.3f S %.3f CC %.6f GND %.6f TC %.6f x "
          "%.6f R %g LEN %g  %s --- ",
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
    rc->writeRC(dbg_logFP, false);

    m._tmpRC = rc;
    rcModel->addRCw(&m);
    prev_sep = m._s_nm;
    prev_width = m._w_nm;
  }
  rcModel->mkWidthAndSpaceMappings();

  fclose(logFP);
  fclose(dbg_logFP);
  return 0;
}

std::list<std::string> extModelGen::GetCornerNames(const char* filename,
                                                   double& version,
                                                   utl::Logger* logger)
{
  bool dbg = false;
  std::list<std::string> corner_list;
  Parser parser(logger);
  // parser.setDbg(1);
  parser.addSeparator("\r");
  parser.openFile((char*) filename);

  while (parser.parseNextLine() > 0) {
    if (parser.isKeyword(0, "Version")) {
      version = parser.getDouble(1);
      if (dbg) {
        fprintf(stdout, "Version %g\n", version);
      }
      continue;
    }
    if (parser.isKeyword(0, "Corners")) {
      if (dbg) {
        parser.printWords(stdout);
      }

      int cnt = parser.getInt(1);
      for (int ii = 0; ii < cnt; ii++) {
        std::string str(parser.get(ii + 3));
        corner_list.push_back(str);
        if (dbg) {
          fprintf(stdout, "corner %d %s\n", ii, str.c_str());
        }
      }
      break;
    }
  }
  return corner_list;
}

/*
AFILE extModelGen::InitWriteRules_old(AFILE *fp, bool binary, uint32_t
modelIndex)
{
//	FILE *fp= openFile("./", name, nullptr, "w");
#ifndef _WIN32
        //AFILE *fp= ATH__fopen(name, "w", AF_ENCRYPT);
        AFILE *fp= ATH__fopen(name, "w");
#else
        FILE *fp= fopen(name, "w");
#endif

        uint32_t cnt= 0;
        ATH__fprintf(fp, "Extraction Rules for Athena Design Systems
Tools\n\n"); if (_diag) { if (_diagModel==1) ATH__fprintf(fp, "DIAGMODEL
ON\n\n"); else if (_diagModel==2) ATH__fprintf(fp, "DIAGMODEL TRUE\n\n");
        }

        ATH__fprintf(fp, "LayerCount %d\n", _layerCnt-1);
        ATH__fprintf(fp, "DensityRate %d ", _modelCnt);
        for (uint32_t kk= 0; kk<_modelCnt; kk++)
                ATH__fprintf(fp, " %g", _dataRateTable->get(kk));
        ATH__fprintf(fp, "\n");
    return fp;
}
*/
}  // namespace rcx
