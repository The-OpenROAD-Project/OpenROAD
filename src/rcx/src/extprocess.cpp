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

#include "rcx/extprocess.h"

#include "utl/Logger.h"

namespace rcx {

using utl::RCX;

extConductor::extConductor(Logger* logger)
{
  strcpy(_name, "");
  _height = 0;
  _distance = 0;
  _thickness = 0;
  _min_width = 0;
  _min_spacing = 0;
  _origin_x = 0;
  _bottom_left_x = 0;
  _bottom_right_x = 0;
  _top_left_x = 0;
  _top_right_x = 0;
  _var_table_index = 0;
  _p = 0.0;
  _min_cw_del = 0;
  _max_cw_del = 0;
  _min_ct_del = 0;
  _max_ct_del = 0;
  _min_ca = 0;
  _max_ca = 0;
  _top_ext = 0;
  _bot_ext = 0;
  logger_ = logger;
}

extDielectric::extDielectric(Logger* logger)
{
  strcpy(_name, "");
  strcpy(_non_conformal_metal, "");
  _epsilon = 0;
  _height = 0;
  _distance = 0;
  _thickness = 0;
  _left_thickness = 0;
  _right_thickness = 0;
  _top_thickness = 0;
  _bottom_thickness = 0;
  _conformal = false;
  _trench = false;
  _bottom_ext = 0;
  _slope = 0;
  _met = 0;
  _nextMet = 0;
  logger_ = logger;
}

void extMasterConductor::writeRaphaelDielPoly(FILE* fp,
                                              double X,
                                              double width,
                                              extDielectric* diel)
{
  fprintf(fp, "POLY NAME= %s; ", diel->_name);

  fprintf(fp, " COORD= ");

  writeRaphaelPointXY(fp, X + _loLeft[0], _loLeft[2]);
  writeRaphaelPointXY(fp, X + _loRight[0], _loLeft[2]);
  writeRaphaelPointXY(fp, X + _hiRight[0], _hiRight[2]);
  writeRaphaelPointXY(fp, X + _hiLeft[0], _hiLeft[2]);

  fprintf(fp, " DIEL=%g ;\n", diel->_epsilon);
}

void extMasterConductor::printDielBox(FILE* fp,
                                      double X,
                                      double width,
                                      extDielectric* diel)
{
  // non conformal

  double thickness = _hiLeft[2] - _loLeft[2];
  if (thickness == 0.0) {
    return;
  }

  _loRight[0] = _loLeft[0] + width;
  _hiRight[0] = _hiLeft[0] + width;

  writeRaphaelDielPoly(fp, X, width, diel);
}

void extProcess::writeParam(FILE* fp, const char* name, double val)
{
  fprintf(fp, "param %s=%g;\n", name, val);
}

void extProcess::writeWindow(FILE* fp,
                             const char* param_width_name,
                             double y1,
                             const char* param_thickness_name)
{
  fprintf(fp, "\n$ Simulation window\n");
  fprintf(fp,
          "WINDOW X1=-0.5*%s; Y1=%g; X2=0.5*%s; Y2=%s; DIEL=1.0;\n",
          param_width_name,
          y1,
          param_width_name,
          param_thickness_name);
}

void extProcess::writeGround(FILE* fp,
                             int met,
                             const char* name,
                             double width,
                             double x1,
                             double volt,
                             bool diag)
{
  if (met < 0) {
    return;
  }
  if (!met && diag) {
    return;
  }

  double y1 = -1.0;
  double th = 1.0;
  extMasterConductor* m;
  if (met > 0 && !diag) {
    m = getMasterConductor(met);
    y1 = m->_loLeft[2];
    th = m->_hiLeft[2] - y1;
    m->writeRaphaelConformalGround(fp, x1, width, this);
    fprintf(fp, "POLY NAME= M%d__%s; ", met, name);
  } else {
    fprintf(fp, "POLY NAME= M0__%s; ", name);
  }

  fprintf(fp, " COORD= ");

  writeRaphaelPointXY(fp, x1, y1);
  writeRaphaelPointXY(fp, x1 + width, y1);
  writeRaphaelPointXY(fp, x1 + width, y1 + th);
  writeRaphaelPointXY(fp, x1, y1 + th);

  fprintf(fp, " VOLT=%g ;\n", volt);

  fprintf(fp, "OPTIONS SET_GRID=10000;\n\n");
}

void extProcess::writeGround(FILE* fp,
                             int met,
                             const char* name,
                             const char* param_width_name,
                             double x1,
                             double volt)
{
  if (met < 0) {
    return;
  }

  double y1 = -0.5;
  double th = 1.0;
  if (met > 0) {
    extMasterConductor* m = getMasterConductor(met);
    y1 = m->_loLeft[2];
    th = m->_hiLeft[2] - y1;
  }
  fprintf(fp,
          "BOX NAME=M%d__%s CX=%g; CY=%g; W=%s; H=%g; VOLT=%g;\n",
          met,
          name,
          x1,
          y1,
          param_width_name,
          th,
          volt);

  fprintf(fp, "OPTIONS SET_GRID=10000;\n\n");
}

void extProcess::writeFullProcess(FILE* fp, double X, double width)
{
  for (uint jj = 1; jj < _masterDielectricTable->getCnt(); jj++) {
    extMasterConductor* m = _masterDielectricTable->get(jj);
    extDielectric* diel = _dielTable->get(m->_condId);

    m->printDielBox(fp, X, width, diel);
  }
}

void extProcess::writeProcessAndGround(FILE* wfp,
                                       const char* gndName,
                                       int underMet,
                                       int overMet,
                                       double X,
                                       double width,
                                       double thickness,
                                       bool diag)
{
  const char* widthName = "window_width";
  const char* thicknessName = "window_thichness";

  writeParam(wfp, widthName, width);
  writeParam(wfp, thicknessName, thickness);

  writeFullProcess(wfp, X, width);

  double y1 = 0.0;
  writeWindow(wfp, widthName, y1, thicknessName);

  fprintf(wfp, "\n$ Ground Plane(s)\n");

  writeGround(wfp, underMet, gndName, width, -0.5 * width, 0.0, diag);
  writeGround(wfp, overMet, gndName, width, -0.5 * width, 0.0, diag);
}

extMasterConductor::extMasterConductor(uint condId,
                                       extConductor* cond,
                                       double prevHeight,
                                       Logger* logger)
{
  _condId = condId;
  logger_ = logger;
  // X coordinates

  double min_width = cond->_min_width;
  if (cond->_bottom_right_x - cond->_bottom_left_x > 0) {
    _loLeft[0] = cond->_bottom_left_x;
    _loRight[0] = cond->_bottom_right_x;
  } else {
    if (cond->_bot_ext == 0.0) {
      _loLeft[0] = 0;
    } else {
      _loLeft[0] = -cond->_bot_ext;
    }
    if (!(min_width > 0)) {
      logger_->warn(RCX,
                    158,
                    "Can't determine Bottom Width for Conductor <{}>",
                    cond->_name);
      exit(0);
    }
    if (cond->_bot_ext == 0.0) {
      _loRight[0] = min_width;
    } else {
      _loRight[0] = min_width + cond->_bot_ext;
    }
  }
  _hiLeft[0] = cond->_top_left_x;
  _hiRight[0] = cond->_top_right_x;

  if (cond->_top_right_x - cond->_top_left_x > 0) {
    _hiLeft[0] = cond->_top_left_x;
    _hiRight[0] = cond->_top_right_x;
  } else {
    if (cond->_top_ext == 0.0) {
      _hiLeft[0] = 0;
    } else {
      _hiLeft[0] = -cond->_top_ext;
    }
    if (!(min_width > 0)) {
      logger_->warn(RCX,
                    152,
                    "Can't determine Top Width for Conductor <{}>",
                    cond->_name);
      exit(0);
    }
    if (cond->_top_ext == 0.0) {
      _hiRight[0] = min_width;
    } else {
      _hiRight[0] = min_width + cond->_top_ext;
    }
  }

  // Y coordinates
  _loLeft[1] = 0;
  _loRight[1] = 0;
  _hiLeft[1] = 0;
  _hiRight[1] = 0;

  // Z coordinates
  double height = cond->_height;
  if (height <= 0) {
    height += prevHeight + cond->_distance;
    cond->_height = height;
  }

  _loLeft[2] = height;
  _loRight[2] = height;

  double thickness = cond->_thickness;
  if (!(thickness > 0)) {
    logger_->warn(
        RCX, 153, "Can't determine thickness for Conductor <{}>", cond->_name);
    exit(0);
  }
  _hiLeft[2] = height + thickness;
  _hiRight[2] = height + thickness;

  _dy = 0;
  _e = 0.0;
  for (uint i = 0; i < 3; i++) {
    _conformalId[i] = 0;
  }
}

void extMasterConductor::resetThicknessHeight(double height, double thickness)
{
  _hiLeft[2] = height + thickness;
  _hiRight[2] = height + thickness;
  _loLeft[2] = _hiLeft[2] - thickness;
  _loRight[2] = _loLeft[2];
}

void extMasterConductor::resetWidth(double top_width, double bottom_width)
{
  _loLeft[0] = -bottom_width / 2;
  _loRight[0] = bottom_width / 2;

  _hiLeft[0] = -top_width / 2;
  _hiRight[0] = top_width / 2;
}

void extMasterConductor::reset(double height,
                               double top_width,
                               double bottom_width,
                               double thickness)
{
  _loLeft[0] = -bottom_width / 2;
  _loRight[0] = bottom_width / 2;

  _hiLeft[0] = -top_width / 2;
  _hiRight[0] = top_width / 2;

  // Y coordinates
  _loLeft[1] = 0;
  _loRight[1] = 0;
  _hiLeft[1] = 0;
  _hiRight[1] = 0;

  // Z coordinates
  // assume target height and thickness were set

  _hiLeft[2] = height + thickness;
  _hiRight[2] = height + thickness;
  _loLeft[2] = _hiLeft[2] - thickness;
  _loRight[2] = _loLeft[2];
}

void extMasterConductor::writeRaphaelPointXY(FILE* fp, double X, double Y)
{
  fprintf(fp, "  %6.3f,%6.3f ; ", X, Y);
}

void extProcess::writeRaphaelPointXY(FILE* fp, double X, double Y)
{
  fprintf(fp, "  %6.3f,%6.3f ; ", X, Y);
}

void extMasterConductor::writeRaphaelPoly(FILE* fp,
                                          uint wireNum,
                                          double X,
                                          double volt)
{
  fprintf(fp, "POLY NAME=");
  writeBoxName(fp, wireNum);

  fprintf(fp, " COORD= ");

  writeRaphaelPointXY(fp, X + _loLeft[0], _loLeft[2]);
  writeRaphaelPointXY(fp, X + _loRight[0], _loLeft[2]);
  writeRaphaelPointXY(fp, X + _hiRight[0], _hiRight[2]);
  writeRaphaelPointXY(fp, X + _hiLeft[0], _hiLeft[2]);

  fprintf(fp, " VOLT=%g\n", volt);
}

void extMasterConductor::writeRaphaelConformalGround(FILE* fp,
                                                     double X,
                                                     double width,
                                                     extProcess* p)
{
  double height[3];
  double thickness[3];
  double e[3];
  bool trench = false;
  uint cnt = 0;
  uint start = 0;
  double h = _loLeft[2];
  for (uint i = 0; i < 3; i++) {
    uint j = 2 - i;
    if (!_conformalId[j]) {
      continue;
    }
    if (!start) {
      start = i;
    }
    extDielectric* d = p->getDielectric(_conformalId[j]);
    // assuming conformal and trench will not show up at the same time. Also
    // height for the trench layer is negative.
    trench = d->_trench;
    height[i] = d->_height;
    thickness[i] = d->_thickness;
    e[i] = d->_epsilon;
    h += d->_thickness;
    cnt++;
  }
  if (!cnt) {
    return;
  }
  if (trench) {
    for (uint j = start; j < start + cnt; j++) {
      fprintf(fp, "POLY NAME=");
      fprintf(fp, "M%d_Trench%d;", _condId, 2 - j);

      writeRaphaelPointXY(fp, X, _hiRight[2] + height[j]);
      writeRaphaelPointXY(fp, X + width, _hiRight[2] + height[j]);
      writeRaphaelPointXY(fp, X + width, _hiRight[2]);
      writeRaphaelPointXY(fp, X, _hiRight[2]);
      fprintf(fp, " DIEL=%g\n", e[j]);
    }
    return;
  }
  for (uint j = start; j < start + cnt; j++) {
    fprintf(fp, "POLY NAME=");
    fprintf(fp, "M%d_Conformal%d;", _condId, 2 - j);

    h -= thickness[j];
    writeRaphaelPointXY(fp, X, h);
    writeRaphaelPointXY(fp, X + width, h);
    writeRaphaelPointXY(fp, X + width, h + height[j]);
    writeRaphaelPointXY(fp, X, h + height[j]);
    fprintf(fp, " DIEL=%g\n", e[j]);
  }
}

double extMasterConductor::writeRaphaelPoly(FILE* fp,
                                            uint wireNum,
                                            double width,
                                            double X,
                                            double volt,
                                            extProcess* p)
{
  fprintf(fp, "POLY NAME=");
  writeBoxName(fp, wireNum);

  fprintf(fp, " COORD= ");

  extConductor* cond = p->getConductor(_condId);
  double top_ext = cond->_top_ext;
  double bot_ext = cond->_bot_ext;

  writeRaphaelPointXY(fp, X - bot_ext, _loLeft[2]);
  writeRaphaelPointXY(fp, X + width + bot_ext, _loLeft[2]);
  writeRaphaelPointXY(fp, X + width + top_ext, _hiRight[2]);
  writeRaphaelPointXY(fp, X - top_ext, _hiLeft[2]);

  fprintf(fp, " VOLT=%g\n", volt);
  return X + width;
}

void extMasterConductor::writeBoxName(FILE* fp, uint wireNum)
{
  fprintf(fp, "M%d_w%d;", _condId, wireNum);
}

extMasterConductor::extMasterConductor(uint dielId,
                                       extDielectric* diel,
                                       double xlo,
                                       double dx1,
                                       double xhi,
                                       double dx2,
                                       double h,
                                       double th,
                                       Logger* logger)
{
  _condId = dielId;
  logger_ = logger;

  // X coordinates
  _loLeft[0] = xlo;
  _loRight[0] = xlo + dx1;
  _hiLeft[0] = xhi;
  _hiRight[0] = xhi + dx2;

  // Y coordinates
  _loLeft[1] = 0;
  _loRight[1] = 0;
  _hiLeft[1] = 0;
  _hiRight[1] = 0;

  // Z coordinates

  _loLeft[2] = h;
  _loRight[2] = h;

  if (!(th > 0)) {
    logger_->warn(
        RCX, 154, "Can't determine thickness for Diel <{}>", diel->_name);
  }
  _hiLeft[2] = h + th;
  _hiRight[2] = h + th;

  _dy = 0;
  _e = diel->_epsilon;
  for (uint i = 0; i < 3; i++) {
    _conformalId[i] = 0;
  }
}

FILE* extProcess::openFile(const char* filename, const char* permissions)
{
  FILE* fp = fopen(filename, permissions);
  if (fp == nullptr) {
    logger_->error(RCX,
                   159,
                   "Can't open file {} with permissions <{}>",
                   filename,
                   permissions);
  }
  return fp;
}

double extProcess::adjustMasterLayersForHeight(uint met, double thickness)
{
  double condThickness = _condTable->get(met)->_thickness;
  double dth = (thickness - condThickness) / condThickness;

  double h = 0.0;
  for (uint ii = 1; ii < _masterConductorTable->getCnt(); ii++) {
    extConductor* cond = _condTable->get(ii);
    extMasterConductor* m = _masterConductorTable->get(ii);

    if (_thickVarFlag) {
      h += cond->_distance * (1 + dth);
    } else {
      h += cond->_distance;
    }

    double th = cond->_thickness;
    if (_thickVarFlag) {
      th *= (1 + dth);
    } else if (ii == met) {
      th = thickness;
    }

    m->resetThicknessHeight(h, th);
    h += th;
  }
  return dth;
}

double extProcess::adjustMasterDielectricsForHeight(uint met, double dth)
{
  double h = 0.0;
  for (uint ii = 1; ii < _masterDielectricTable->getCnt(); ii++) {
    extDielectric* diel = _dielTable->get(ii);
    extMasterConductor* m = _masterDielectricTable->get(ii);

    double th = diel->_thickness;
    if (_thickVarFlag) {
      th *= (1 + dth);
    } else if (diel->_met == (int) met) {
      th *= (1 + dth);
    }

    m->resetThicknessHeight(h, th);
    h += th;
  }
  return h;
}

extConductor* extProcess::getConductor(uint ii)
{
  return _condTable->get(ii);
}

extMasterConductor* extProcess::getMasterConductor(uint ii)
{
  return _masterConductorTable->get(ii);
}

extDielectric* extProcess::getDielectric(uint ii)
{
  return _dielTable->get(ii);
}

Ath__array1D<double>* extProcess::getWidthTable(uint met)
{
  double min_width = getConductor(met)->_min_width;

  const double wTable[8] = {1.0, 1.5, 2.0, 2.5, 3, 4, 5, 10};

  Ath__array1D<double>* A = new Ath__array1D<double>(11);
  for (uint ii = 0; ii < 8; ii++) {
    double w = wTable[ii] * min_width;
    A->add(w);
  }

  return A;
}

Ath__array1D<double>* extProcess::getSpaceTable(uint met)
{
  double min_spacing = getConductor(met)->_min_spacing;

  const double sTable[8] = {1.0, 1.5, 2.0, 2.5, 3, 4, 5, 10};

  Ath__array1D<double>* A = new Ath__array1D<double>(8);
  for (uint ii = 0; ii < 8; ii++) {
    double s = sTable[ii] * min_spacing;
    A->add(s);
  }

  return A;
}

Ath__array1D<double>* extProcess::getDiagSpaceTable(uint met)
{
  double min_spacing = getConductor(met)->_min_spacing;
  double min_width = getConductor(met)->_min_width;
  double p = min_spacing + min_width;

  const double sTable[7] = {0, 0.2, 0.5, 0.7, 1.0, 2.0, 3};

  Ath__array1D<double>* A = new Ath__array1D<double>(8);
  for (uint ii = 0; ii < 7; ii++) {
    double s = sTable[ii] * p;
    A->add(s);
  }

  return A;
}

Ath__array1D<double>* extProcess::getDataRateTable(uint met)
{
  if (_dataRateTable) {
    return _dataRateTable;
  }
  Ath__array1D<double>* A = new Ath__array1D<double>(8);
  A->add(0.0);
  return A;
}

bool extProcess::getMaxMinFlag()
{
  return _maxMinFlag;
}

bool extProcess::getThickVarFlag()
{
  return _thickVarFlag;
}

extMasterConductor* extProcess::getMasterConductor(uint met,
                                                   uint wIndex,
                                                   uint sIndex,
                                                   double& w,
                                                   double& s)
{
  const double wTable[7] = {1.0, 1.5, 2.0, 2.5, 3, 3.5, 4};
  const double sTable[14]
      = {1.0, 1.2, 1.5, 1.7, 2.0, 2.25, 2.5, 2.75, 3, 3.5, 4, 4.5, 5, 6};

  extMasterConductor* m = getMasterConductor(met);

  w = wTable[wIndex] * getConductor(met)->_min_width;
  s = sTable[sIndex] * getConductor(met)->_min_spacing;

  return m;
}

extVariation* extProcess::getVariation(uint met)
{
  extConductor* m = getConductor(met);

  extVariation* v = nullptr;
  if (m->_var_table_index > 0) {
    v = _varTable->get(m->_var_table_index);
  }

  return v;
}

double extVariation::interpolate(double w,
                                 Ath__array1D<double>* X,
                                 Ath__array1D<double>* Y)
{
  if (X->getCnt() < 2) {
    return w;
  }

  int jj = X->findNextBiggestIndex(w);

  if (jj >= (int) X->getCnt() - 1) {
    jj = X->getCnt() - 2;
  } else if (jj < 0) {
    jj = 0;
  }

  double w1 = X->get(jj);
  double w2 = X->get(jj + 1);

  double v1 = Y->get(jj);
  double v2 = Y->get(jj + 1);

  double slope = (v2 - v1) / (w2 - w1);

  double retVal = v2 - slope * (w2 - w);

  return retVal;
}

double extVariation::getThickness(double w, uint dIndex)
{
  return interpolate(w, _thicknessC->_width, _thicknessC->_vTable[dIndex]);
}

double extVariation::getThicknessR(double w, uint dIndex)
{
  return interpolate(w, _thicknessR->_width, _thicknessR->_vTable[dIndex]);
}

double extVariation::getBottomWidth(double w, uint dIndex)
{
  return interpolate(w, _loWidthC->_width, _loWidthC->_vTable[dIndex]);
}

double extVariation::getBottomWidthR(double w, uint dIndex)
{
  return interpolate(w, _loWidthR->_width, _loWidthR->_vTable[dIndex]);
}

double extVariation::getTopWidth(uint ii, uint jj)
{
  return _hiWidthC->getVal(ii, jj);
}

double extVariation::getTopWidthR(uint ii, uint jj)
{
  return _hiWidthR->getVal(ii, jj);
}

Ath__array1D<double>* extVariation::getWidthTable()
{
  return _hiWidthC->_width;
}

Ath__array1D<double>* extVariation::getSpaceTable()
{
  return _hiWidthC->_space;
}

Ath__array1D<double>* extVariation::getDataRateTable()
{
  return _loWidthC->_density;
}

Ath__array1D<double>* extVariation::getPTable()
{
  if (_p == nullptr) {
    return nullptr;
  }
  return _p->_p;
}

double extVariation::getP(double w)
{
  if (_p == nullptr) {
    return 0;
  }
  return interpolate(w, _p->_width, _p->_p);
}

Ath__array1D<double>* extVarTable::readDoubleArray(Ath__parser* parser,
                                                   const char* keyword)
{
  if ((keyword != nullptr) && (strcmp(keyword, parser->get(0)) != 0)) {
    return nullptr;
  }

  if (parser->getWordCnt() < 1) {
    return nullptr;
  }

  Ath__array1D<double>* A = new Ath__array1D<double>(parser->getWordCnt());
  uint start = 0;
  if (keyword != nullptr) {
    start = 1;
  }
  parser->getDoubleArray(A, start);
  return A;
}

void extVarTable::printOneLine(FILE* fp,
                               Ath__array1D<double>* A,
                               const char* header,
                               const char* trail)
{
  if (A == nullptr) {
    return;
  }

  if (header != nullptr) {
    fprintf(fp, "%s ", header);
  }

  for (uint ii = 0; ii < A->getCnt(); ii++) {
    fprintf(fp, "%g ", A->get(ii));
  }

  fprintf(fp, "%s", trail);
}

void extVarTable::printTable(FILE* fp, const char* valKey)
{
  printOneLine(fp, _width, "Width", "\n");

  if (_space != nullptr) {
    printOneLine(fp, _space, "Spacing", "\n");
  } else if (_density != nullptr) {
    printOneLine(fp, _space, "Deff", "\n");
  } else {
    printOneLine(fp, _p, "P", "\n");
    return;
  }

  fprintf(fp, "%s\n", valKey);

  for (uint ii = 0; ii < _rowCnt; ii++) {
    printOneLine(fp, _vTable[ii], nullptr, "\n");
  }
}

void extVariation::printVariation(FILE* fp, uint n)
{
  fprintf(fp, "VAR_TABLE %d {\n", n);

  _hiWidthC->printTable(fp, "hi_cWidth_eff");
  _loWidthC->printTable(fp, "lo_cWidth_delta");
  _thicknessC->printTable(fp, "c_thickness_eff");

  _hiWidthR->printTable(fp, "hi_rWidth_eff");
  _loWidthR->printTable(fp, "lo_rWidth_delta");
  _thicknessR->printTable(fp, "r_thickness_eff");
  if (_p != nullptr) {
    _p->printTable(fp, "P");
  }

  fprintf(fp, "}\n");
}

extProcess::extProcess(uint condCnt, uint dielCnt, Logger* logger)
{
  logger_ = logger;
  _condTable = new Ath__array1D<extConductor*>(condCnt);
  _condTable->add(nullptr);
  _maxMinFlag = false;
  _dielTable = new Ath__array1D<extDielectric*>(dielCnt);
  _dielTable->add(nullptr);
  _masterConductorTable = new Ath__array1D<extMasterConductor*>(condCnt);
  _masterConductorTable->add(nullptr);
  _masterDielectricTable = new Ath__array1D<extMasterConductor*>(dielCnt);
  _masterDielectricTable->add(nullptr);

  _varTable = new Ath__array1D<extVariation*>(condCnt);
  _varTable->add(nullptr);
  _dataRateTable = nullptr;
  _thickVarFlag = false;
}

extVarTable::extVarTable(uint rowCnt)
{
  _rowCnt = rowCnt;
  _vTable = new Ath__array1D<double>*[rowCnt];
  _density = nullptr;
  _space = nullptr;
  _width = nullptr;
  _p = nullptr;
}

extVarTable::~extVarTable()
{
  for (uint ii = 0; ii < _rowCnt; ii++) {
    if (_vTable[ii] != nullptr) {
      delete _vTable[ii];
    }
  }
  delete[] _vTable;

  delete _density;
  delete _space;
  delete _width;
  delete _p;
  _p = nullptr;
  _density = nullptr;
  _space = nullptr;
  _width = nullptr;
}

}  // namespace rcx
