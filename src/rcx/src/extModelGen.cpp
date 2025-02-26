///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2024, IC BENCH, Dimitris Fotakis
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
//
#include "rcx/extModelGen.h"

namespace rcx {

uint extRCModel::GenExtModel(std::list<std::string>& corner_list,
                             const char* out_file,
                             const char* comment,
                             const char* version,
                             int pattern)
{
  extModelGen* g = (extModelGen*) this;
  bool binary = false;
  uint corner_cnt = corner_list.size();
  FILE* outFP
      = g->InitWriteRules(out_file, corner_list, comment, false, corner_cnt);
  for (uint ii = 0; ii < corner_cnt; ii++) {
    extMetRCTable* corner_model = getMetRCTable(ii);

    corner_model->mkWidthAndSpaceMappings();

    g->writeRules(outFP, binary, ii, ii);
  }
  fclose(outFP);
  return 0;
}

uint extMain::GenExtModel(std::list<std::string> spef_file_list,
                          std::list<std::string> corner_list,
                          const char* out_file,
                          const char* comment,
                          const char* version,
                          int pattern)
{
  std::vector<std::string> corner_name;

  std::list<std::string>::iterator it1;
  for (it1 = corner_list.begin(); it1 != corner_list.end(); ++it1) {
    std::string str = *it1;
    corner_name.push_back(str);
  }
  uint widthCnt = 12;
  uint layerCnt = _tech->getRoutingLayerCount() + 1;
  FILE* outFP = NULL;

  bool binary = false;
  uint fileCnt = spef_file_list.size();
  uint cnt = 0;
  std::list<std::string>::iterator it;
  for (it = spef_file_list.begin(); it != spef_file_list.end(); ++it) {
    std::string str = *it;
    const char* filename = str.c_str();
    readSPEF((char*) filename,
             NULL,
             /*force*/ true,
             false,
             NULL,
             false,
             false,
             false,
             -0.5,
             0.0,
             1.0,
             false,
             false,
             NULL,
             false,
             -1,
             0.0,
             0.0,
             NULL,
             NULL,
             NULL,
             NULL,
             NULL,
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

    uint diagOption = 1;
    char* out = (char*) corner_name[cnt].c_str();
    // if (cnt == 1)
    //    out = "2.model";
    extRulesModel->ReadRCDB(_block, widthCnt, diagOption, out);
    if (outFP == NULL)
      outFP = extRulesModel->InitWriteRules(
          out_file, corner_list, comment, binary, fileCnt);

    extRulesModel->writeRules(outFP, binary, cnt);  // always first model
    cnt++;
  }
  fclose(outFP);
  return 0;
}
void extModelGen::writeRules(FILE* fp, bool binary, uint mIndex, int corner)
{
  uint m = 0;
  bool writeRes = true;

  extMetRCTable* rcTable0 = getMetRCTable(0);  // orig call

  extMetRCTable* rcTable;
  if (corner >= 0)
    rcTable = getMetRCTable(corner);
  else
    rcTable = getMetRCTable(m);

  uint layerCnt = getLayerCnt();
  uint diagModel = getDiagModel();

  fprintf(fp, "\nDensityModel %d\n", mIndex);

  uint cnt = 0;
  for (uint ii = 1; ii < layerCnt; ii++) {
    if (writeRes) {
      cnt += writeRulesPattern(0,
                               ii,
                               m,
                               rcTable->_resOver[ii],
                               rcTable->_resOver[0],
                               "RESOVER",
                               fp,
                               binary);
    }
    cnt += writeRulesPattern(0,
                             ii,
                             m,
                             rcTable->_capOver[ii],
                             rcTable->_capOver[0],
                             "OVER",
                             fp,
                             binary);
    cnt += writeRulesPattern(0,
                             ii,
                             m,
                             rcTable->_capOver_open[ii][0],
                             rcTable->_capOver_open[0][0],
                             "OVER0",
                             fp,
                             binary);
    cnt += writeRulesPattern(0,
                             ii,
                             m,
                             rcTable->_capOver_open[ii][1],
                             rcTable->_capOver_open[0][1],
                             "OVER1",
                             fp,
                             binary);
    cnt += writeRulesPattern(1,
                             ii,
                             m,
                             rcTable->_capUnder[ii],
                             rcTable->_capUnder[0],
                             "UNDER",
                             fp,
                             binary);
    cnt += writeRulesPattern(1,
                             ii,
                             m,
                             rcTable->_capUnder_open[ii][0],
                             rcTable->_capUnder_open[0][0],
                             "UNDER0",
                             fp,
                             binary);
    cnt += writeRulesPattern(1,
                             ii,
                             m,
                             rcTable->_capUnder_open[ii][1],
                             rcTable->_capUnder_open[0][1],
                             "UNDER1",
                             fp,
                             binary);

    if (rcTable->_capDiagUnder[ii] != NULL) {
      if (diagModel == 1)
        cnt += rcTable->_capDiagUnder[ii]->writeRulesDiagUnder(fp, binary);
      if (diagModel == 2)
        cnt += rcTable->_capDiagUnder[ii]->writeRulesDiagUnder2(fp, binary);
    } else if ((m > 0) && (rcTable0->_capDiagUnder[ii] != NULL)) {
      if (diagModel == 1)
        cnt += rcTable0->_capDiagUnder[ii]->writeRulesDiagUnder(fp, binary);
      if (diagModel == 2)
        cnt += rcTable0->_capDiagUnder[ii]->writeRulesDiagUnder2(fp, binary);
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
      cnt += writeRulesPattern(2,
                               ii,
                               m,
                               rcTable->_capOverUnder[ii],
                               rcTable->_capOverUnder[0],
                               "OVERUNDER",
                               fp,
                               binary);
      cnt += writeRulesPattern(2,
                               ii,
                               m,
                               rcTable->_capOverUnder_open[ii][0],
                               rcTable->_capOverUnder_open[0][0],
                               "OVERUNDER0",
                               fp,
                               binary);
      cnt += writeRulesPattern(2,
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
uint extRCModel::writeRulesPattern(uint ou,
                                   uint layer,
                                   int modelIndex,
                                   extDistWidthRCTable* table_m,
                                   extDistWidthRCTable* table_0,
                                   const char* patternKeyword,
                                   FILE* fp,
                                   bool binary)
{
  uint cnt = 0;
  extDistWidthRCTable* table = NULL;
  if (table_m != NULL)
    table = table_m;
  else if ((modelIndex > 0) && (table_0 != NULL))
    table = table_0;
  else if (modelIndex == 0) {
    fprintf(stdout,
            "Cannot write %s rules for <DensityModel> %d and layer %d\n",
            patternKeyword,
            modelIndex,
            layer);
    // TODO logger_->info( RCX, 218, "Cannot write {} rules for <DensityModel>
    // {} and layer {}", patternKeyword, modelIndex, layer);
    return 0;
  }
  if (ou == 0)
    cnt = table->writeRulesOver(fp, patternKeyword, binary);
  else if (ou == 1)
    cnt = table->writeRulesUnder(fp, patternKeyword, binary);
  else if (ou == 2)
    cnt = table->writeRulesOverUnder(fp, patternKeyword, binary);

  return cnt;
}

uint extDistWidthRCTable::writeRulesUnder(FILE* fp,
                                          const char* keyword,
                                          bool bin)
{
  uint cnt = 0;
  fprintf(fp, "\nMetal %d %s\n", _met, keyword);

  writeWidthTable(fp, bin);
  uint widthCnt = _widthTable->getCnt();

  for (uint ii = _met + 1; ii < _layerCnt; ii++) {
    fprintf(fp, "\nMetal %d %s %d\n", _met, keyword, ii);

    uint metIndex = getMetIndexUnder(ii);

    for (uint jj = 0; jj < widthCnt; jj++) {
      cnt += _rcDistTable[metIndex][jj]->writeRules(
          fp, 0.001 * _widthTable->get(jj), false, bin);
    }
  }
  return cnt;
}
uint extDistWidthRCTable::writeRulesDiagUnder2(FILE* fp, bool bin)
{
  uint cnt = 0;
  fprintf(fp, "\nMetal %d DIAGUNDER\n", _met);

  writeWidthTable(fp, bin);
  uint widthCnt = _widthTable->getCnt();
  writeDiagTablesCnt(fp, _met + 1, bin);

  for (uint ii = _met + 1; ii < _met + 5 && ii < _layerCnt; ii++) {
    fprintf(fp, "\nMetal %d DIAGUNDER %d\n", _met, ii);
    writeDiagWidthTable(fp, ii, bin);
    uint diagWidthCnt = _diagWidthTable[ii]->getCnt();
    writeDiagDistTable(fp, ii, bin);
    uint diagDistCnt = _diagDistTable[ii]->getCnt();

    uint metIndex = getMetIndexUnder(ii);

    for (uint jj = 0; jj < widthCnt; jj++) {
      for (uint kk = 0; kk < diagWidthCnt; kk++) {
        for (uint ll = 0; ll < diagDistCnt; ll++) {
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
uint extDistWidthRCTable::writeRulesDiagUnder(FILE* fp, bool bin)
{
  uint cnt = 0;
  fprintf(fp, "\nMetal %d DIAGUNDER\n", _met);

  writeWidthTable(fp, bin);
  uint widthCnt = _widthTable->getCnt();

  for (uint ii = _met + 1; ii < _layerCnt; ii++) {
    fprintf(fp, "\nMetal %d DIAGUNDER %d\n", _met, ii);

    uint metIndex = getMetIndexUnder(ii);

    for (uint jj = 0; jj < widthCnt; jj++) {
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
uint extDistWidthRCTable::writeRulesOverUnder(FILE* fp,
                                              const char* keyword,
                                              bool bin)
{
  uint cnt = 0;
  fprintf(fp, "\nMetal %d %s\n", _met, keyword);

  writeWidthTable(fp, bin);
  uint widthCnt = _widthTable->getCnt();

  for (uint mUnder = 1; mUnder < _met; mUnder++) {
    for (uint mOver = _met + 1; mOver < _layerCnt; mOver++) {
      fprintf(fp, "\nMetal %d OVER %d UNDER %d\n", _met, mUnder, mOver);

      int metIndex = extRCModel::getMetIndexOverUnder(
          _met, mUnder, mOver, _layerCnt, _metCnt);
      assert(metIndex >= 0);

      for (uint jj = 0; jj < widthCnt; jj++) {
        cnt += _rcDistTable[metIndex][jj]->writeRules(
            fp, 0.001 * _widthTable->get(jj), false, bin);
      }
    }
  }
  return cnt;
}
uint extDistWidthRCTable::writeRulesOver(FILE* fp,
                                         const char* keyword,
                                         bool bin)
{
  uint cnt = 0;
  // fprintf(fp, "\nMetal %d OVER\n", _met);
  fprintf(fp, "\nMetal %d %s\n", _met, keyword);

  writeWidthTable(fp, bin);
  uint widthCnt = _widthTable->getCnt();

  for (uint ii = 0; ii < _met; ii++) {
    fprintf(fp, "\nMetal %d %s %d\n", _met, keyword, ii);
    // fprintf(fp, "\nMetal %d OVER %d\n", _met, ii);

    for (uint jj = 0; jj < widthCnt; jj++) {
      cnt += _rcDistTable[ii][jj]->writeRules(
          fp, 0.001 * _widthTable->get(jj), false, bin);
    }
  }
  return cnt;
}
uint extDistWidthRCTable::writeRulesOver_res(FILE* fp, bool bin)
{
  uint cnt = 0;
  fprintf(fp, "\nMetal %d RESOVER\n", _met);

  writeWidthTable(fp, bin);
  uint widthCnt = _widthTable->getCnt();

  for (uint ii = 0; ii < _met; ii++) {
    fprintf(fp, "\nMetal %d RESOVER %d\n", _met, ii);

    for (uint jj = 0; jj < widthCnt; jj++) {
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
                                  uint modelCnt)
{
  uint diagModel = getDiagModel();
  int layerCnt = getLayerCnt();
  bool diag = getDiagFlag();

  //	FILE *fp= openFile("./", name, NULL, "w");
  FILE* fp = fopen(name, "w");

  fprintf(fp, "Extraction Rules for rcx\n\n");
  fprintf(fp, "Version 1.2\n\n");
  if (diag || diagModel > 0) {
    if (diagModel == 1)
      fprintf(fp, "DIAGMODEL ON\n\n");
    else if (diagModel == 2)
      fprintf(fp, "DIAGMODEL TRUE\n\n");
  }
  fprintf(fp, "LayerCount %d\n", layerCnt - 1);

  fprintf(fp, "\nDensityRate %d ", modelCnt);
  for (uint kk = 0; kk < modelCnt; kk++)
    fprintf(fp, " %d", kk);
  fprintf(fp, "\n");

  fprintf(fp, "\nCorners %ld : ", corner_list.size());
  std::list<std::string>::iterator it;
  for (it = corner_list.begin(); it != corner_list.end(); ++it) {
    std::string str = *it;
    fprintf(fp, " %s", str.c_str());
  }
  fprintf(fp, "\n");

  if (comment != NULL && strlen(comment) > 0)
    fprintf(fp, "\nCOMMENT : %s \n\n", comment);

  return fp;
}
uint extModelGen::ReadRCDB(dbBlock* block,
                           uint widthCnt,
                           uint diagOption,
                           char* logFilePrefix)
{
  extMain* extMain = get_extMain();
  // ORIG: setDiagModel(1);
  setDiagModel(diagOption);
  //  setOptions("./", "", false, true, false, false);

  uint layerCnt = getLayerCnt();
  extMetRCTable* rcModel = initCapTables(layerCnt, widthCnt);

  AthPool<extDistRC>* rcPool = rcModel->getRCPool();
  extMeasure m(NULL);
  m._diagModel = 1;
  uint openWireNumber = 1;

  char buff[2000];
  sprintf(buff, "%s.log", logFilePrefix);
  FILE* logFP = fopen(buff, "w");

  sprintf(buff, "%s.debug.log", logFilePrefix);
  FILE* dbg_logFP = fopen(buff, "w");

  Ath__parser* p = new Ath__parser(logger_);
  Ath__parser* w = new Ath__parser(logger_);

  int prev_sep = 0;
  int prev_width = 0;
  odb::dbSet<dbNet> nets = block->getNets();
  odb::dbSet<dbNet>::iterator itr;
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

    // Read Via patterns - dkf 12262023
    // via pattern: V2.W2.M5.M6.DX520.DY1320.C2.V56_1x2_VH_S
    if (p->getFirstChar() == 'V') {
      if (!rcModel->GetViaRes(p, w, net, logFP))
        break;
      continue;
    }
    if (wcnt < 5)
      continue;

    // dkf 12302023 -- NOTE Original Patterns start with: O6_ U6_ OU6_ DU6_ R6_
    // example:

    if (rcModel->SkipPattern(p, net, logFP))
      continue;

    int targetWire = 0;
    if (p->getFirstChar() == 'U') {
      targetWire = p->getInt(0, 1);
    } else {
      char* w1 = p->get(0);
      if (w1[1] == 'U')  // OU
        targetWire = p->getInt(0, 2);
      else
        targetWire = p->getInt(0, 1);
    }
    if (targetWire <= 0)
      continue;

    uint wireNum = p->getInt(wcnt - 1);
    // if (wireNum != targetWire / 2)
    //   continue;

    // fprintf(logFP, "%s\n", netName);
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

    char* overUnderToken = strdup(p->get(1));  // M2oM1uM3
    int wCnt = w->mkWords(overUnderToken, "ou");
    if (wCnt < 2)
      continue;

    if (wCnt == 3) {  // M2oM1uM3
      met = w->getInt(0, 1);
      overMet = w->getInt(1, 1);
      underMet = w->getInt(2, 1);

      m._overMet = underMet;
      m._underMet = overMet;
      m._overUnder = true;
      m._over = false;
    } else if (strstr(overUnderToken, "o") != NULL) {
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
    } else if (strstr(overUnderToken, "uu") != NULL) {
      met = w->getInt(0, 1);
      underMet = w->getInt(1, 1);
      diag = true;
      m._diag = true;
      m._overMet = underMet;
      m._underMet = -1;
      m._overUnder = false;
      m._over = false;
    } else if (strstr(overUnderToken, "u") != NULL) {
      met = w->getInt(0, 1);
      underMet = w->getInt(1, 1);

      m._overMet = underMet;
      m._underMet = -1;
      m._overUnder = false;
      m._over = false;
    }
    // TODO DIAGUNDER
    m._met = met;

    if (w->mkWords(p->get(2), "W") <= 0)
      continue;

    double w1 = w->getDouble(0) / 1000;

    m._w_m = w1;
    m._w_nm = ceil(m._w_m * 1000);

    if (w->mkWords(p->get(3), "S") <= 0)
      continue;

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
    if (ResModel)
      R *= 2;

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
    if (strstr(netName, "cntxM") != NULL)
      continue;

    extDistRC* rc = rcPool->alloc();
    if (ResModel) {
      rc->set(m._s_nm, m._s2_m, 0.0, 0.0, R);
    } else {
      if (diag)
        rc->set(m._s_nm, 0.0, cc, cc, R);
      else {
        if (m._s_nm == 0)
          m._s_nm = prev_sep + prev_width;
        rc->set(m._s_nm, cc, gnd, 0.0, R);
      }
    }
    uint wireNum2 = 2;
    m._open = false;
    m._over1 = false;
    if (wireNum == openWireNumber) {  // default openWireNumber=1
      if (ResModel || m._diag)
        continue;
      m._open = true;
    } else if (wireNum == wireNum2) {
      if (ResModel || m._diag)
        continue;
      m._over1 = true;
    } else if (wireNum != 3)
      continue;

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
                                                   Logger* logger)
{
  bool dbg = false;
  std::list<std::string> corner_list;
  Ath__parser parser(logger);
  // parser.setDbg(1);
  parser.addSeparator("\r");
  parser.openFile((char*) filename);

  while (parser.parseNextLine() > 0) {
    if (parser.isKeyword(0, "Version")) {
      version = parser.getDouble(1);
      if (dbg)
        fprintf(stdout, "Version %g\n", version);
      continue;
    }
    if (parser.isKeyword(0, "Corners")) {
      if (dbg)
        parser.printWords(stdout);

      int cnt = parser.getInt(1);
      for (int ii = 0; ii < cnt; ii++) {
        std::string str(parser.get(ii + 3));
        corner_list.push_back(str);
        if (dbg)
          fprintf(stdout, "corner %d %s\n", ii, str.c_str());
      }
      break;
    }
  }
  return corner_list;
}

/*
AFILE extModelGen::InitWriteRules_old(AFILE *fp, bool binary, uint modelIndex)
{
//	FILE *fp= openFile("./", name, NULL, "w");
#ifndef _WIN32
        //AFILE *fp= ATH__fopen(name, "w", AF_ENCRYPT);
        AFILE *fp= ATH__fopen(name, "w");
#else
        FILE *fp= fopen(name, "w");
#endif

        uint cnt= 0;
        ATH__fprintf(fp, "Extraction Rules for Athena Design Systems
Tools\n\n"); if (_diag) { if (_diagModel==1) ATH__fprintf(fp, "DIAGMODEL
ON\n\n"); else if (_diagModel==2) ATH__fprintf(fp, "DIAGMODEL TRUE\n\n");
        }

        ATH__fprintf(fp, "LayerCount %d\n", _layerCnt-1);
        ATH__fprintf(fp, "DensityRate %d ", _modelCnt);
        for (uint kk= 0; kk<_modelCnt; kk++)
                ATH__fprintf(fp, " %g", _dataRateTable->get(kk));
        ATH__fprintf(fp, "\n");
    return fp;
}
*/
}  // namespace rcx
