// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#include <string.h>  // NOLINT(modernize-deprecated-headers): for strdup()

#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <list>
#include <map>
#include <string>
#include <vector>

#include "parse.h"
#include "rcx/array1.h"
#include "rcx/extRCap.h"
#include "rcx/extprocess.h"
#include "rcx/grids.h"
#include "rcx/util.h"
#include "utl/Logger.h"

using utl::RCX;

namespace rcx {

uint32_t extMain::readProcess(const char* name, const char* filename)
{
  extProcess* p = new extProcess(32, 32, logger_);

  p->readProcess(name, (char*) filename);
  p->writeProcess("process.out");

  uint32_t layerCnt = p->getConductorCnt();
  extRCModel* m = new extRCModel(layerCnt, (char*) name, logger_);
  _modelTable->add(m);

  m->setProcess(p);
  m->setDataRateTable(1);

  return 0;
}

void extRCModel::clear_corners()
{
  _cornerMap.clear();
  _cornerTable.clear();
}
bool extRCModel::addCorner(const std::string& w, int ii)
{
  if (_cornerMap.find(w) == _cornerMap.end()) {
    _cornerMap[w] = ii;
    return true;
  }
  return false;
}
uint32_t extRCModel::defineCorners(std::list<std::string>& corners)
{
  uint32_t cnt = 0;
  clear_corners();
  for (const auto& w : corners) {
    if (addCorner(w, cnt)) {
      cnt++;
    }
  }
  return cnt;
}
uint32_t extRCModel::getCorners(std::list<std::string>& corners)
{
  std::list<std::string> corner_list;
  for (const auto& pair : _cornerMap) {
    corners.push_back(pair.first);
  }
  return corner_list.size();
}
uint32_t extRCModel::initModel(std::list<std::string>& corners, int met_cnt)
{
  int cornerCnt = defineCorners(corners);
  createModelTable(cornerCnt, (uint32_t) (met_cnt + 1));
  for (uint32_t m = 0; m < cornerCnt; m++) {
    for (uint32_t ii = 1; ii < _layerCnt; ii++) {
      allocateTables(m, ii, 1);
    }
  }
  return cornerCnt;
}
int extSolverGen::getLastCharInt(const char* name)
{
  // Assume single digit -- only simulate 1, 2, 3, 5 wires

  char word[2];
  uint32_t len = strlen(name);
  char lastChar = name[len - 1];
  if (isdigit(lastChar)) {
    word[0] = lastChar;
    word[1] = '\0';
    int n = atoi(word);

    return n;
  }
  return -1;
}
bool extRCModel::getAllowedPatternWireNums(Parser& p,
                                           extMeasure& m,
                                           const char* fullPatternName,
                                           int input_target_wire,
                                           int& pattern_num)
{
  // parsing Under5/M6uM7/W0.42_W0.42/S8.4_S8.4/wire_5

  p.resetSeparator("_");
  int n1 = p.mkWords(fullPatternName);
  int wire_num = p.getInt(n1 - 1);

  p.resetSeparator("/");
  int n2 = p.mkWords(fullPatternName);
  const char* pattern = p.get(n2 - 5);
  pattern_num = extSolverGen::getLastCharInt(pattern);

  if (pattern_num < 0) {
    return false;  // old files; should NOT happen
  }

  if (m._res) {
    return wire_num == 0;  // for Resistance, wire is 0
  }

  // for OpenEnded patterns: Over1, Under1, etc, wire is 1
  m._open = pattern_num == 1;

  // for one side OpenEnded patterns: Over2, Under2, wire is 1
  // User should set it to 1 only when fully coupled patterns are to used for
  // One Side OpenEnded patterns
  m._over1 = (pattern_num == 2 && wire_num == 1)
             || (pattern_num > 2 && pattern_num < 6 && wire_num == 1
                 && input_target_wire == 1);  // OpenEnded on one  side

  if (m._open || m._over1) {
    return true;
  }

  // for fully coupled patterns: Over3, Under3, wire is 2
  // for fully coupled patterns: Over5, Under5, wire is 3
  if (pattern_num == 3 && wire_num == 2) {
    return true;
  }
  if (pattern_num == 5 && wire_num == 3) {
    return true;
  }

  return false;
}

uint32_t extRCModel::readRCvalues(const char* corner,
                                  const char* filename,
                                  int wire,
                                  bool over,
                                  bool under,
                                  bool over_under,
                                  bool diag)
{
  // for Resistance, wire is 0
  // for OpenEnded patterns: Over1, Under1, etc, wire is 1
  // for one side OpenEnded patterns: Over2, Under2, wire is 1
  // for fully coupled patterns: Over3, Under3, wire is 2
  // for fully coupled patterns: Over5, Under5, wire is 3
  // wire default is 0, User should set it to 1 only when fully coupled patterns
  // are to used for One Side OpenEnded patterns

  uint32_t cnt = 0;

  // TODO
  setDiagModel(1);
  // DELETE uint32_t layerCnt = getLayerCnt();

  uint32_t corner_index = 0;
  extMetRCTable* met_rc = getMetRCTable(corner_index);
  AthPool<extDistRC>* rcPool = met_rc->getRCPool();

  extMeasure m(nullptr);
  m._diagModel = 1;
  // DELETE uint32_t openWireNumber = 1;
  // DELETE int n = 0;
  // DELETE double via_res_1 = 0;
  /*
    char buff[2000];
    sprintf(buff, "%s.log", corner);
    FILE* logFP = fopen(buff, "w");
    sprintf(buff, "%s.debug.log", corner);
    FILE* dbg_logFP = fopen(buff, "w");
  */

  free(_ruleFileName);
  _ruleFileName = strdup(filename);
  Parser p(logger_);
  Parser parser(logger_);
  parser.addSeparator("\r");
  parser.openFile(filename);

  // Metal 6 Over 0 Under 7 Dist 8.4 Width LEN 10 0.42 CC 0.854180 FR 0.496770
  // TC 1.350950 CC2 0.002760 M6uM7/W0.42_W0.42/S8.4_S8.4/wire_5

  // Metal 1 Over 0 DiagUnder 2 Dist 0.14 Width LEN 10 0.14 CC 0.239133 FR
  // 0.138103 TC 0.377236 CC2 0.009553 DiagDist 0.14 DiagWidth 0.14 DiagCC
  // 0.019350 M1duM2/W0.14_W0.14/S0.14_S0.14/wire_1

  uint32_t skippedCnt = 0;
  uint32_t skippedWireCnt = 0;

  while (parser.parseNextLine() > 0) {
    cnt++;
    int n = parser.getWordCnt();
    parser.printWords(stdout);
    if (!parser.isKeyword(0, "Metal")) {
      skippedCnt++;
      continue;
    }
    parseMets(parser, m);

    const char* fullPatternName = parser.get(n - 1);
    int pattern_num;
    if (!getAllowedPatternWireNums(p, m, fullPatternName, wire, pattern_num)) {
      skippedWireCnt++;
      continue;
    }

    double totCC = parser.getDouble(13);
    if (m._open) {
      totCC = 0;
    }
    double totGnd = parser.getDouble(15);
    double contextCoupling = parser.getDouble(19);
    if (m._open && contextCoupling > 0) {
      totGnd += contextCoupling;
      totCC -= contextCoupling;
    }
    double wLen = parseWidthDistLen(parser, m);
    double cc = totCC / wLen / 2;
    double gnd = totGnd / wLen / 2;
    double res = !m._res ? 0.0 : parser.getDouble(21);
    double R = res / wLen / 2;
    if (m._res) {
      R *= 2;
    }

    // if (strstr(netName, "cntxM") != nullptr)
    //  continue;

    extDistRC* rc = rcPool->alloc();
    if (m._res) {
      rc->set(m._s_nm, m._s2_m, 0.0, 0.0, R);
    } else if (m._diag) {
      rc->set(m._s_nm, 0.0, cc, cc, R);
    } else {
      // if (m._s_nm == 0)
      //  m._s_nm = prev_sep + prev_width;
      rc->set(m._s_nm, cc, gnd, 0.0, R);
    }

    m._tmpRC = rc;
    met_rc->addRCw(&m);
    // prev_sep = m._s_nm;
    // prev_width = m._w_nm;
    // fprintf(dbg_logFP, "Metal %d Over %d Under %d Width %g -- ", m._met,
    // m._underMet, m._overMet,  m._w_nm); rc->writeRC(dbg_logFP, false);
  }
  logger_->info(RCX,
                446,
                "{} lines parsed, {} lines skipped not starting with <Metal>, "
                "{} lines skipped for wrong wire number, filename: {}\n",
                cnt,
                skippedCnt,
                skippedWireCnt,
                filename);
  return cnt;
}
bool extRCModel::parseMets(Parser& parser, extMeasure& m)
{
  m._diag = parser.isKeyword(4, "DiagUnder");
  m._res = parser.isKeyword(2, "RESOVER");

  m._met = parser.getInt(1);
  m._overMet = parser.getInt(5);
  m._underMet = parser.getInt(3);
  m._overUnder = m._overMet > 0 && m._underMet > 0;
  m._over = m._overMet == 0 && !m._overUnder;

  return m._diag;
}
double extRCModel::parseWidthDistLen(Parser& parser, extMeasure& m)
{
  double w1 = parser.getDouble(9);
  m._w_m = w1;
  m._w_nm = ceil(m._w_m * 1000);

  double s1 = parser.getDouble(7);
  if (m._open) {
    s1 = 0;
  }
  double s2 = s1;

  m._s_m = s1;
  m._s_nm = ceil(m._s_m * 1000);
  m._s2_m = s2;
  m._s2_nm = ceil(m._s2_m * 1000);

  int n1 = parser.getInt(11);

  double wLen = n1 * 1000 * w1;

  return wLen;
}
void extMetRCTable::allocOverUnderTable(uint32_t met,
                                        bool open,
                                        Array1D<double>* wTable,
                                        double dbFactor)
{
  if (met < 2) {
    return;
  }

  int n = extRCModel::getMaxMetIndexOverUnder(met, _layerCnt);
  if (!open) {
    _capOverUnder[met] = new extDistWidthRCTable(false,
                                                 met,
                                                 _layerCnt,
                                                 n + 1,
                                                 wTable,
                                                 _rcPoolPtr,
                                                 OUReverseOrder_,
                                                 dbFactor);
  } else {
    for (uint32_t ii = 0; ii < _wireCnt; ii++) {
      _capOverUnder_open[met][ii] = new extDistWidthRCTable(false,
                                                            met,
                                                            _layerCnt,
                                                            n + 1,
                                                            wTable,
                                                            _rcPoolPtr,
                                                            OUReverseOrder_,
                                                            dbFactor);
    }
  }
}
void extMetRCTable::allocOverTable(uint32_t met,
                                   Array1D<double>* wTable,
                                   double dbFactor)
{
  _capOver[met] = new extDistWidthRCTable(
      true, met, _layerCnt, met, wTable, _rcPoolPtr, OUReverseOrder_, dbFactor);
  _resOver[met] = new extDistWidthRCTable(
      true, met, _layerCnt, met, wTable, _rcPoolPtr, OUReverseOrder_, dbFactor);
  for (uint32_t ii = 0; ii < _wireCnt; ii++) {
    _capOver_open[met][ii] = new extDistWidthRCTable(true,
                                                     met,
                                                     _layerCnt,
                                                     met,
                                                     wTable,
                                                     _rcPoolPtr,
                                                     OUReverseOrder_,
                                                     dbFactor);
  }
}
void extMetRCTable::allocUnderTable(uint32_t met,
                                    bool open,
                                    Array1D<double>* wTable,
                                    double dbFactor)
{
  if (!open) {
    _capUnder[met] = new extDistWidthRCTable(false,
                                             met,
                                             _layerCnt,
                                             _layerCnt - met - 1,
                                             wTable,
                                             _rcPoolPtr,
                                             OUReverseOrder_,
                                             dbFactor);
  } else {
    for (uint32_t ii = 0; ii < _wireCnt; ii++) {
      _capUnder_open[met][ii] = new extDistWidthRCTable(false,
                                                        met,
                                                        _layerCnt,
                                                        _layerCnt - met - 1,
                                                        wTable,
                                                        _rcPoolPtr,
                                                        OUReverseOrder_,
                                                        dbFactor);
    }
  }
}
uint32_t extRCModel::allocateTables(uint32_t m,
                                    uint32_t met,
                                    uint32_t diagModel)
{
  double dbFactor = 1.0;
  uint32_t cnt = 0;
  Array1D<double>* wTable = nullptr;

  _modelTable[m]->allocOverTable(met, wTable, dbFactor);
  _modelTable[m]->allocOverUnderTable(met, false, wTable, dbFactor);
  _modelTable[m]->allocOverUnderTable(met, true, wTable, dbFactor);
  _modelTable[m]->allocUnderTable(met, false, wTable, dbFactor);
  _modelTable[m]->allocUnderTable(met, true, wTable, dbFactor);

  // if (diagModel == 2)
  //  _modelTable[m]->allocDiagUnderTable(met, wTable, diagWidthCnt,
  //  diagDistCnt, dbFactor);

  _modelTable[m]->allocDiagUnderTable(met, wTable, dbFactor);

  return cnt;
}
extDistWidthRCTable*** extMetRCTable::allocTable()
{
  extDistWidthRCTable*** table = new extDistWidthRCTable**[_layerCnt];
  if (table == nullptr) {
    fprintf(stderr, "Cannot allocate memory for object: extDistWidthRCTable\n");
    exit(0);
  }
  for (uint32_t ii = 0; ii < _layerCnt; ii++) {
    table[ii] = new extDistWidthRCTable*[_wireCnt];
    if (table[ii] == nullptr) {
      fprintf(stderr,
              "Cannot allocate memory for object: extDistWidthRCTable\n");
      exit(0);
    }
    for (uint32_t jj = 0; jj < _wireCnt; jj++) {
      table[ii][jj] = nullptr;
    }
  }
  return table;
}
void extMetRCTable::deleteTable(extDistWidthRCTable*** table)
{
  if (table == nullptr) {
    return;
  }

  for (uint32_t ii = 0; ii < _layerCnt; ii++) {
    if (table[ii] == nullptr) {
      continue;
    }

    for (uint32_t jj = 0; jj < _wireCnt; jj++) {
      if (table[ii][jj] != nullptr) {
        delete table[ii][jj];
      }
    }
    delete table[ii];
  }
  delete table;
}
}  // namespace rcx
