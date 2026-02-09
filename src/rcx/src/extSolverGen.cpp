// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#include "rcx/extSolverGen.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <vector>

#include "parse.h"
#include "rcx/extModelGen.h"
#include "rcx/extprocess.h"
#include "utl/Logger.h"

namespace rcx {

uint32_t extSolverGen::genSolverPatterns(const char* process_name,
                                         int version,
                                         int wire_cnt,
                                         int len,
                                         int over_dist,
                                         int under_dist,
                                         const char* w_list,
                                         const char* s_list)
{
  if (!setWidthSpaceMultipliers(w_list, s_list)) {
    logger_->warn(utl::RCX,
                  235,
                  "Width {} and Space {} multiplier lists are NOT correct\n",
                  w_list,
                  s_list);
  }

  initLogFiles(process_name);
  _len = len;
  _simVersion = version;
  _maxUnderDist = under_dist;
  _maxOverDist = over_dist;
  _wireCnt = wire_cnt;
  _layerCnt = getConductorCnt();

  _3dFlag = true;
  _diag = false;

  setDiagModel(0);  // to initialize _diagModel;

  uint32_t cnt1 = linesOver();
  cnt1 += linesUnder();
  cnt1 += linesOverUnder();
  setDiagModel(2);
  cnt1 += linesDiagUnder();

  logger_->info(utl::RCX, 246, "Finished {} patterns", cnt1);

  closeFiles();
  return 0;
}
void extSolverGen::init()
{
  _wireDirName = new char[2048];
  _topDir = new char[1024];
  _patternName = new char[1024];
  _parser = new Parser(logger_);
  _solverFileName = new char[1024];
  _wireFileName = new char[1024];
  _capLogFP = nullptr;
  _logFP = nullptr;

  _readCapLog = false;
  _commentFlag = false;
  _layerCnt = 0;
}
/// @brief Loops for widths and spacings of target met
/// @param diagMet:  if >0 there is an added loop of spacings of diagMet
/// @return number of patterns completed
uint32_t extSolverGen::widthsSpacingsLoop(uint32_t diagMet)
{
  uint32_t cnt = 0;

  extConductor* cond = getConductor(_met);
  double t = cond->_thickness;
  double h = cond->_height;
  double ro = cond->_p;
  // double res = 0.0;
  double top_ext = cond->_top_ext;
  double bot_ext = cond->_bot_ext;

  double min_width = getConductor(_met)->_min_width;
  double min_spacing = getConductor(_met)->_min_spacing;

  double diag_min_width = 0.0;
  double diag_min_spacing = 0.0;
  uint32_t diagSpaceCnt = 3;
  float diagSpaceMultipliers[3] = {0, 1.0, 2.0};
  _diag = diagMet > 0;
  if (_diag > 0) {
    diag_min_width = getConductor(diagMet)->_min_width;
    diag_min_spacing = getConductor(diagMet)->_min_spacing;
  }
  _metExtFlag = false;
  if (top_ext != 0.0 || bot_ext != 0.0) {
    _metExtFlag = true;
  }
  for (float mult_w : _widthMultTable) {
    float w = mult_w * min_width;

    double top_width = w + 2 * top_ext;
    // double top_widthR = w + 2 * top_ext;
    double bot_width = w + 2 * bot_ext;
    double thickness = t;
    // double bot_widthR = w + 2 * bot_ext;
    // double thicknessR = t;

    if (diagMet == 0) {
      for (uint32_t jj = 0; jj < _spaceMultTable.size(); jj++) {
        float mult_s = _spaceMultTable[jj];
        float s = mult_s * min_spacing;

        setTargetParams(w, s, 0.0, t, h);
        measurePatternVar_3D(
            _met, top_width, bot_width, thickness, _wireCnt, nullptr);

        cnt++;
        if (jj == 0 && _overMet <= 0 && _underMet == 0) {
          calcResistance(ro, w, s, _len * w, top_width, bot_width, thickness);
        }
        if (_wireCnt == 1) {
          break;
        }
      }
    } else {
      uint32_t scnt = 3;  // First 3 spacings from the spacing table
      for (uint32_t kk = 0; kk < diagSpaceCnt; kk++) {
        float mult_s = diagSpaceMultipliers[kk];
        float diag_s = mult_s * (diag_min_spacing + diag_min_width);

        for (uint32_t jj = 0; jj < scnt; jj++) {
          float mult_s = _spaceMultTable[jj];
          float s = mult_s * min_spacing;

          setTargetParams(w, s, 0.0, t, h, diag_min_width, diag_s);
          measurePatternVar_3D(
              _met, top_width, bot_width, thickness, _wireCnt, nullptr);
          cnt++;
          if (_wireCnt == 1) {
            break;
          }
        }
        if (_wireCnt == 1) {
          break;
        }
      }
    }
    return cnt;
  }
  return cnt;
}
double extSolverGen::calcResistance(double ro,
                                    double w,
                                    double s,
                                    double len,
                                    double top_widthR,
                                    double bot_widthR,
                                    double thicknessR)
{
  double r_unit = ro / (thicknessR * (top_widthR + bot_widthR) / 2);
  double tot_res = len * r_unit;
  fprintf(_resFP,
          "Metal %d RESOVER 0 Under 0  Dist 0.0 Width %.4f LEN %.4f  CC 0.0  "
          "FR 0.0  TC 0.0  CC2 0.0 RES %.6f %s/wire_0\n",
          _met,
          w,
          len,
          tot_res,
          _wireDirName);
  return tot_res;
}
uint32_t extSolverGen::linesOver(uint32_t metLevel)
{
  sprintf(_patternName, "Over%d", _wireCnt);
  uint32_t cnt = 0;

  for (uint32_t met = 1; met < _layerCnt; met++) {
    if (metLevel > 0 && met != metLevel) {
      continue;
    }

    _met = met;
    for (uint32_t underMet = 0; underMet < met; underMet++) {
      if (underMet > 0 && met - underMet > _maxUnderDist && underMet > 0) {
        continue;
      }

      setMets(met, underMet, -1);
      uint32_t cnt1 = widthsSpacingsLoop();
      cnt += cnt1;

      logger_->info(utl::RCX,
                    251,
                    "Finished {} measurements for pattern M{}_over_M{}",
                    cnt1,
                    met,
                    underMet);
    }
  }
  logger_->info(
      utl::RCX, 250, "Finished {} measurements for pattern MET_OVER_MET", cnt);
  return cnt;
}
uint32_t extSolverGen::linesUnder(uint32_t metLevel)
{
  sprintf(_patternName, "Under%d", _wireCnt);
  uint32_t cnt = 0;

  for (uint32_t met = 1; met < _layerCnt; met++) {
    if (metLevel > 0 && met != metLevel) {
      continue;
    }

    for (uint32_t overMet = met + 1; overMet < _layerCnt; overMet++) {
      if (overMet - met > _maxOverDist) {
        continue;
      }

      setMets(met, 0, overMet);
      uint32_t cnt1 = widthsSpacingsLoop();
      cnt += cnt1;

      logger_->info(utl::RCX,
                    238,
                    "Finished {} measurements for pattern M{}_under_M{}",
                    cnt1,
                    met,
                    overMet);
    }
  }
  logger_->info(
      utl::RCX, 241, "Finished {} measurements for pattern MET_UNDER_MET", cnt);
  return cnt;
}
uint32_t extSolverGen::linesDiagUnder(uint32_t metLevel)
{
  _diag = true;
  sprintf(_patternName, "UnderDiag%d", _wireCnt);

  uint32_t cnt = 0;
  for (uint32_t met = 1; met < _layerCnt; met++) {
    if (metLevel > 0 && met != metLevel) {
      continue;
    }

    for (uint32_t overMet = met + 1; overMet < _layerCnt; overMet++) {
      if (overMet - met > _maxOverDist) {
        continue;
      }

      setMets(met, 0, overMet);
      uint32_t cnt1 = widthsSpacingsLoop(overMet);
      cnt += cnt1;

      logger_->info(utl::RCX,
                    244,
                    "Finished {} measurements for pattern M{}_diagUnder_M{}",
                    cnt1,
                    met,
                    overMet);
    }
  }
  logger_->info(utl::RCX,
                245,
                "Finished {} measurements for pattern MET_DIAGUNDER_MET",
                cnt);
  return cnt;
}
uint32_t extSolverGen::linesOverUnder(uint32_t metLevel)
{
  sprintf(_patternName, "OverUnder%d", _wireCnt);
  uint32_t cnt = 0;

  for (uint32_t met = 1; met < _layerCnt - 1; met++) {
    if (metLevel > 0 && met != metLevel) {
      continue;
    }

    for (uint32_t underMet = 1; underMet < met; underMet++) {
      if (met - underMet > _maxUnderDist) {
        continue;
      }
      for (uint32_t overMet = met + 1; overMet < _layerCnt; overMet++) {
        if (overMet - met > _maxOverDist) {
          continue;
        }

        setMets(met, underMet, overMet);

        uint32_t cnt1 = widthsSpacingsLoop();
        cnt += cnt1;

        logger_->info(
            utl::RCX,
            242,
            "Finished {} measurements for pattern M{}_over_M{}_under_M{}",
            cnt1,
            met,
            underMet,
            overMet);
      }
    }
  }
  logger_->info(utl::RCX,
                243,
                "Finished {} measurements for pattern MET_OVERUNDER_MET",
                cnt);
  return cnt;
}

void extSolverGen::setEffParams(double wTop, double wBot, double teff)
{
  _topWidth = wTop;
  _botWidth = wBot;
  _teff = teff;
  _heff = _h;
  if (!_metExtFlag && _s_m != 99) {
    _seff = _w_m + _s_m - wTop;
  } else {
    _seff = _s_m;
  }
}
void extSolverGen::mkFileNames(char* wiresNameSuffix)
{
  char overUnder[128];

  if ((_overMet > 0) && (_underMet > 0)) {
    sprintf(overUnder, "M%doM%duM%d", _met, _underMet, _overMet);

  } else if (_overMet > 0) {
    if (_diag) {
      sprintf(overUnder, "M%dduM%d", _met, _overMet);
    } else {
      sprintf(overUnder, "M%duM%d", _met, _overMet);
    }

  } else if (_underMet >= 0) {
    sprintf(overUnder, "M%doM%d", _met, _underMet);

  } else {
    sprintf(overUnder, "Uknown");
  }

  double w = _w_m;
  double s = _s_m;
  double w2 = _w2_m;
  double s2 = _s2_m;

  sprintf(_wireDirName,
          "%s/%s/%s/W%g_W%g/S%g_S%g_L%d",
          _topDir,
          _patternName,
          overUnder,
          w,
          w2,
          s,
          s2,
          _len);

  if (wiresNameSuffix != nullptr) {
    sprintf(_wireFileName, "%s.%s", "wires", wiresNameSuffix);
  } else {
    sprintf(_wireFileName, "%s", "wires");
  }

  fprintf(_logFP, "PATTERN %s\n\n", _wireDirName);
  fflush(_logFP);
}
bool extSolverGen::measurePatternVar_3D(int met,
                                        double top_width,
                                        double bot_width,
                                        double thickness,
                                        uint32_t wireCnt,
                                        char* wiresNameSuffix)
{
  setEffParams(top_width, bot_width, thickness);
  double thicknessChange = adjustMasterLayersForHeight(met, thickness);
  getMasterConductor(met)->resetWidth(top_width, bot_width);

  mkFileNames(wiresNameSuffix);

  printCommentLine('#');
  fprintf(_logFP, "%s\n", _commentLine);
  fprintf(_logFP, "%c %g thicknessChange\n", '$', thicknessChange);
  fflush(_logFP);

  FILE* wfp = mkPatternFile();

  if (wfp == nullptr) {
    return false;  // should be an exception!! and return!
  }

  double maxHeight = adjustMasterDielectricsForHeight(met, thicknessChange);
  maxHeight *= 1.2;

  double len = top_width * _len;
  if (len <= 0) {
    len = top_width * 10 * 0.001;
  }
  //                        double W = (m->_ur[m->_dir] -
  // m->_ll[m->_dir])*10;
  double W = 40;

  bool apply_height_offset = _simVersion > 1;

  double height_low = 0;
  double height_ceiling = 0;
  if (_diagModel > 0) {
    height_low = writeProcessAndGroundPlanes(wfp,
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
    height_low = writeProcessAndGroundPlanes(wfp,
                                             "GND",
                                             _underMet,
                                             _overMet,
                                             -30.0,
                                             60.0,
                                             len,
                                             maxHeight,
                                             W,
                                             apply_height_offset,
                                             height_ceiling);
  }
  if (_simVersion < 2) {
    height_low = 0;
  }

  if (_commentFlag) {
    fprintf(wfp, "%s\n", _commentLine);
  }

  double len1;
  double X1;
  double X0 = 0;
  if (wireCnt >= 5) {
    X0 = writeWirePatterns(wfp, height_low, len1, X1);
  } else {
    X0 = writeWirePatterns_w3(wfp, height_low, len1, X1);
  }

  fprintf(wfp,
          "\nWINDOW_BBOX  LL %6.3f %6.3f UR %6.3f %6.3f LENGTH %6.3f\n",
          X0,
          0.0,
          X1,
          height_ceiling,
          len1);
  double DX = X1 - X0;
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
double extSolverGen::writeWirePatterns(FILE* fp,
                                       double height_offset,
                                       double& len,
                                       double& max_x)
{
  // assume _wireCnt>=5
  extMasterConductor* m = getMasterConductor(_met);
  extMasterConductor* mOver = nullptr;
  if (_diagModel > 0) {
    mOver = getMasterConductor(_overMet);
  }

  double targetWidth = _topWidth;
  double targetPitch = _topWidth + _seff;
  double minWidth = getConductor(_met)->_min_width;
  double minSpace = getConductor(_met)->_min_spacing;
  double min_pitch = minWidth + minSpace;

  len = _len * minWidth;  // HEIGHT param

  // Assumption -- odd wireCnt, >1
  int wireCnt = _wireCnt;
  if (wireCnt == 0) {
    return 0.0;
  }
  if (wireCnt % 2 > 0) {  // should be odd
    wireCnt--;
  }

  int n = wireCnt / 2;
  double orig = 0.0;
  double x = orig;

  uint32_t cnt = 1;

  double xd[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  double X0 = x - targetPitch;  // next neighbor spacing
  // double W0= targetWidth; // next neighbor width
  xd[1] = X0 - min_pitch;
  double min_x = xd[1];
  for (int ii = 2; ii < n; ii++) {
    xd[ii] = xd[ii - 1] - min_pitch;  // next over neighbor spacing
    min_x = std::min(min_x, xd[ii]);
  }
  for (int ii = 2; ii > 0; ii--) {
    m->writeWire3D(fp, cnt++, xd[ii], minWidth, len, height_offset, 0.0);
    max_x = xd[ii] + minWidth;
  }
  min_x = std::min(min_x, x);

  m->writeWire3D(
      fp, cnt++, x, targetWidth, len, height_offset, 1.0);  // Wire on focus
  double center_diag_x = x;
  max_x = x + targetWidth;

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
    center_diag_x += _s2_m;
    double minWidthDiag = getConductor(_overMet)->_min_width;
    double minSpaceDiag = getConductor(_overMet)->_min_spacing;

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
double extSolverGen::writeWirePatterns_w3(FILE* fp,
                                          double height_offset,
                                          double& len,
                                          double& max_x)
{
  extMasterConductor* m = getMasterConductor(_met);
  extMasterConductor* mOver = nullptr;
  if (_diagModel > 0) {
    mOver = getMasterConductor(_overMet);
  }

  double targetWidth = _topWidth;
  double targetPitch = _topWidth + _seff;
  double minWidth = getConductor(_met)->_min_width;
  // DELETE double minSpace =  getConductor(_met)->_min_spacing;
  // double min_pitch = minWidth + minSpace;

  len = _len * minWidth;  // HEIGHT param
  if (len <= 0) {
    len = 10 * minWidth;
  }

  // len *= 0.001;

  // Assumption -- odd wireCnt, >1
  int wireCnt = _wireCnt;
  if (wireCnt == 0) {
    return 0.0;
  }
  if (wireCnt % 2 > 0) {  // should be odd
    wireCnt--;
  }

  // DELETE int n = wireCnt / 2;
  double orig = 0.0;
  double x = orig;
  double X0 = x - targetPitch;  // prev neighbor
  double X1 = x + targetPitch;  // next neighbor
  uint32_t cnt = 1;
  if (_wireCnt == 3) {
    m->writeWire3D(fp, cnt++, X0, targetWidth, len, height_offset, 0);
    m->writeWire3D(
        fp, cnt++, x, targetWidth, len, height_offset, 1.0);  // Wire on focues
    m->writeWire3D(fp, cnt++, X1, targetWidth, len, height_offset, 0);
  } else {
    m->writeWire3D(
        fp, cnt++, x, targetWidth, len, height_offset, 1.0);  // Wire on focues
    if (_wireCnt > 1) {
      m->writeWire3D(fp, cnt++, X1, targetWidth, len, height_offset, 0);
    }
  }
  double min_x = X0;
  double center_diag_x = orig;
  max_x = X1 + minWidth;

  if (_diagModel > 0) {
    center_diag_x += _s2_m;
    double minWidthDiag = getConductor(_overMet)->_min_width;
    double minSpaceDiag = getConductor(_overMet)->_min_spacing;

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
bool extSolverGen::getMultipliers(const char* input, std::vector<double>& table)
{
  std::vector<std::string> result;
  std::istringstream stream(input);  // Wrap input in a string stream
  std::string word;

  // Extract words from the stream and push them into the vector
  while (stream >> word) {
    result.push_back(word);
    double v = atof(word.c_str());
    table.push_back(v);
  }
  return !table.empty();
}
bool extSolverGen::setWidthSpaceMultipliers(const char* w_list,
                                            const char* s_list)
{
  return getMultipliers(w_list, _widthMultTable)
         && getMultipliers(s_list, _spaceMultTable);
}
void extSolverGen::setTargetParams(double w,
                                   double s,
                                   double r,
                                   double t,
                                   double h,
                                   double w2,
                                   double s2)
{
  _w_m = w;
  _s_m = s;
  _w_nm = lround(1000 * w);
  _s_nm = lround(1000 * s);
  _r = r;
  _t = t;
  _h = h;
  if (w2 > 0.0) {
    _w2_m = w2;
    _w2_nm = lround(1000 * w2);
  } else {
    _w2_m = _w_m;
    _w2_nm = _w_nm;
  }
  if (s2 > 0.0 || (s2 == 0.0 && _diag)) {
    _s2_m = s2;
    _s2_nm = lround(1000 * s2);
  } else {
    _s2_m = _s_m;
    _s2_nm = _s_nm;
  }
}
void extSolverGen::setMets(int m, int u, int o)
{
  _met = m;
  _underMet = u;
  _overMet = o;
  _over = false;
  _overUnder = false;
  if ((u > 0) && (o > 0)) {
    _overUnder = true;
  } else if ((u >= 0) && (o < 0)) {
    _over = true;
  }
}
FILE* extSolverGen::openFile(const char* topDir,
                             const char* name,
                             const char* suffix,
                             const char* permissions)
{
  char filename[2048];

  filename[0] = '\0';
  if (topDir != nullptr) {
    sprintf(filename, "%s/", topDir);
  }
  strcat(filename, name);
  if (suffix != nullptr) {
    strcat(filename, suffix);
  }

  FILE* fp = fopen(filename, permissions);
  if (fp == nullptr) {
    logger_->info(utl::RCX,
                  485,
                  "Cannot open file {} with permissions {}",
                  filename,
                  permissions);
    return nullptr;
  }
  return fp;
}
FILE* extSolverGen::mkPatternFile()
{
  _parser->mkDirTree(_wireDirName, "/");

  FILE* fp = openFile(_wireDirName, _wireFileName, nullptr, "w");
  if (fp == nullptr) {
    return nullptr;
  }

  fprintf(fp, "PATTERN %s\n\n", _wireDirName);
  // if (strcmp("TYP/Under3/M6uM7/W0.42_W0.42/S0.84_S0.84", _wireDirName)==0)
  // {
  //  fprintf(stdout, "%s\n", _wireDirName);
  // }

  return fp;
}
void extSolverGen::printCommentLine(char commentChar)
{
  sprintf(_commentLine,
          "%c %s w= %g s= %g r= %g\n\n%c %s %6.3f %6.3f top_width\n%c %s %6.3f "
          "%g bot_width\n%c %s %6.3f %6.3f spacing\n%c %s %6.3f %6.3f height "
          "\n%c %s %6.3f %6.3f thicknes\n",
          commentChar,
          "Layout params",
          _w_m,
          _s_m,
          _r,
          commentChar,
          "Layout/Eff",
          _w_m,
          _topWidth,
          commentChar,
          "Layout/Eff",
          _w_m,
          _botWidth,
          commentChar,
          "Layout/Eff",
          _s_m,
          _seff,
          commentChar,
          "Layout/Eff",
          _h,
          _heff,
          commentChar,
          "Layout/Eff",
          _t,
          _teff);
  _commentFlag = true;
}
void extSolverGen::initLogFiles(const char* process_name)
{
  _logFP = openFile("./", "rulesGen", ".log", "w");
  _resFP = openFile("./", "resistance.", process_name, "w");
  _filesFP = openFile("./", "patternFiles.", process_name, "w");
  strcpy(_topDir, process_name);
  strcpy(_patternName, process_name);
  _winDirFlat = false;
}
void extSolverGen::closeFiles()
{
  fflush(_logFP);

  if (_logFP != nullptr) {
    fclose(_logFP);
  }

  fflush(_filesFP);
  if (_filesFP != nullptr) {
    fclose(_filesFP);
  }

  fflush(_resFP);
  if (_resFP != nullptr) {
    fclose(_resFP);
  }
}
}  // namespace rcx
