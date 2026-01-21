// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "rcx/extprocess.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "parse.h"
#include "rcx/array1.h"
#include "utl/Logger.h"

namespace rcx {

using utl::RCX;

void printInt(FILE* fp, const char* sep, const char* key, int v, bool pos)
{
  if (pos && !(v > 0)) {
    return;
  }

  fprintf(fp, "%s%s %d\n", sep, key, v);
}
void printDouble(FILE* fp, const char* sep, const char* key, double v, bool pos)
{
  if (pos && !(v > 0)) {
    return;
  }

  fprintf(fp, "%s%s %g\n", sep, key, v);
}
void printString(FILE* fp, const char* sep, const char* key, char* v, bool pos)
{
  if (pos && !((v == nullptr) || (strcmp("", v) == 0))) {
    return;
  }

  fprintf(fp, "%s%s %s\n", sep, key, v);
}

extConductor::extConductor(utl::Logger* logger)
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
void extConductor::printString(FILE* fp,
                               const char* sep,
                               const char* key,
                               char* v,
                               bool pos)
{
  if (pos && !((v == nullptr) || (strcmp("", v) == 0))) {
    return;
  }

  fprintf(fp, "%s%s %s\n", sep, key, v);
}
void extConductor::printInt(FILE* fp,
                            const char* sep,
                            const char* key,
                            int v,
                            bool pos)
{
  if (pos && !(v > 0)) {
    return;
  }

  fprintf(fp, "%s%s %d\n", sep, key, v);
}
void extConductor::printDouble(FILE* fp,
                               const char* sep,
                               const char* key,
                               double v,
                               bool pos)
{
  if (pos && !(v > 0)) {
    return;
  }

  fprintf(fp, "%s%s %g\n", sep, key, v);
}
void extConductor::printConductor(FILE* fp, Parser* parse)
{
  fprintf(fp, "CONDUCTOR {\n");

  printString(fp, "\t", "name", _name, false);
  printDouble(fp, "\t", "height", _height, true);
  printDouble(fp, "\t", "thickness", _thickness, true);

  printDouble(fp, "\t", "min_width", _min_width, true);
  printDouble(fp, "\t", "min_spacing", _min_spacing, true);
  printDouble(fp, "\t", "origin_x", _origin_x, true);
  printDouble(fp, "\t", "bottom_left_x", _bottom_left_x, true);
  printDouble(fp, "\t", "bottom_right_x", _bottom_right_x, true);
  printDouble(fp, "\t", "top_left_x", _top_left_x, true);
  printDouble(fp, "\t", "top_right_x", _top_right_x, true);
  printDouble(fp, "\t", "top_extension", _top_ext, false);
  printDouble(fp, "\t", "bottom_extension", _bot_ext, false);
  printInt(fp, "\t", "var_table", _var_table_index, true);
  printDouble(fp, "\t", "resistivity", _p, true);

  fprintf(fp, "}\n");
}
bool extConductor::setIntVal(Parser* parser, const char* key, int n, int& val)
{
  if (strcmp(key, parser->get(0)) == 0) {
    val = parser->getInt(n);
    return true;
  }
  return false;
}
bool extDielectric::setIntVal(Parser* parser, const char* key, int n, int& val)
{
  if (strcmp(key, parser->get(0)) == 0) {
    val = parser->getInt(n);
    return true;
  }
  return false;
}
bool extConductor::setDoubleVal(Parser* parser,
                                const char* key,
                                int n,
                                double& val)
{
  if (strcmp(key, parser->get(0)) == 0) {
    val = parser->getDouble(n);
    return true;
  }
  return false;
}
bool extDielectric::setDoubleVal(Parser* parser,
                                 const char* key,
                                 int n,
                                 double& val)
{
  if (strcmp(key, parser->get(0)) == 0) {
    val = parser->getDouble(n);
    return true;
  }
  return false;
}
/*
bool extConductor::readConductor(Parser* parser) {
  if (setDoubleVal(parser, "distance", 1, _distance))
    return true;
  else if (setDoubleVal(parser, "height", 1, _height))
    return true;
  else if (setDoubleVal(parser, "thickness", 1, _thickness))
    return true;
  else if (setDoubleVal(parser, "min_width", 1, _min_width))
    return true;
  else if (setDoubleVal(parser, "min_spacing", 1, _min_spacing))
    return true;
  else if (setDoubleVal(parser, "origin_x", 1, _origin_x))
    return true;
  else if (setDoubleVal(parser, "bottom_left_x", 1, _bottom_left_x))
    return true;
  else if (setDoubleVal(parser, "bottom_right_x", 1, _bottom_right_x))
    return true;
  else if (setDoubleVal(parser, "top_left_x", 1, _top_left_x))
    return true;
  else if (setDoubleVal(parser, "top_right_x", 1, _top_right_x))
    return true;
  else if (setIntVal(parser, "var_table", 1, _var_table_index))
    return true;
  else if (setDoubleVal(parser, "resistivity", 1, _p))
    return true;
  else if (setDoubleVal(parser, "min_cw_del", 1, _min_cw_del))
    return true;
  else if (setDoubleVal(parser, "max_cw_del", 1, _max_cw_del))
    return true;
  else if (setDoubleVal(parser, "min_ct_del", 1, _min_ct_del))
    return true;
  else if (setDoubleVal(parser, "max_ct_del", 1, _max_ct_del))
    return true;
  else if (setDoubleVal(parser, "min_ca", 1, _min_ca))
    return true;
  else if (setDoubleVal(parser, "max_ca", 1, _max_ca))
    return true;
  else if (setDoubleVal(parser, "top_extension", 1, _top_ext))
    return true;
  else if (setDoubleVal(parser, "bottom_extension", 1, _bot_ext))
    return true;

  return false;
}
*/
bool extConductor::readConductor(Parser* parser)
{
  char* keyword = parser->get(0);
  if (strcmp("distance", keyword) == 0) {
    _distance = parser->getDouble(1);
    return true;
  }
  if (strcmp("height", keyword) == 0) {
    _height = parser->getDouble(1);
    return true;
  }
  if (strcmp("thickness", keyword) == 0) {
    _thickness = parser->getDouble(1);
    return true;
  }
  if (strcmp("min_width", keyword) == 0) {
    _min_width = parser->getDouble(1);
    return true;
  }
  if (strcmp("min_spacing", keyword) == 0) {
    _min_spacing = parser->getDouble(1);
    return true;
  }
  if (strcmp("origin_x", keyword) == 0) {
    _origin_x = parser->getDouble(1);
    return true;
  }
  if (strcmp("bottom_left_x", keyword) == 0) {
    _bottom_left_x = parser->getDouble(1);
    return true;
  }
  if (strcmp("bottom_right_x", keyword) == 0) {
    _bottom_right_x = parser->getDouble(1);
    return true;
  }
  if (strcmp("top_left_x", keyword) == 0) {
    _top_left_x = parser->getDouble(1);
    return true;
  }
  if (strcmp("top_right_x", keyword) == 0) {
    _top_right_x = parser->getDouble(1);
    return true;
  }
  if (strcmp("var_table", keyword) == 0) {
    _var_table_index = parser->getInt(1);
    return true;
  }
  if (strcmp("resistivity", keyword) == 0) {
    _p = parser->getDouble(1);
    return true;
  }
  if (strcmp("min_cw_del", keyword) == 0) {
    _min_cw_del = parser->getDouble(1);
    return true;
  }
  if (strcmp("max_cw_del", keyword) == 0) {
    _max_cw_del = parser->getDouble(1);
    return true;
  }
  if (strcmp("min_ct_del", keyword) == 0) {
    _min_ct_del = parser->getDouble(1);
    return true;
  }
  if (strcmp("max_ct_del", keyword) == 0) {
    _max_ct_del = parser->getDouble(1);
    return true;
  }
  if (strcmp("min_ca", keyword) == 0) {
    _min_ca = parser->getDouble(1);
    return true;
  }
  if (strcmp("max_ca", keyword) == 0) {
    _max_ca = parser->getDouble(1);
    return true;
  }
  if (strcmp("top_extension", keyword) == 0) {
    _top_ext = parser->getDouble(1);
    return true;
  }
  if (strcmp("bottom_extension", keyword) == 0) {
    _bot_ext = parser->getDouble(1);
    return true;
  }

  return false;
}

extDielectric::extDielectric(utl::Logger* logger)
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
void extDielectric::printString(FILE* fp,
                                const char* sep,
                                const char* key,
                                char* v,
                                bool pos)
{
  if (pos && !((v == nullptr) || (strcmp("", v) == 0))) {
    return;
  }

  fprintf(fp, "%s%s %s\n", sep, key, v);
}
void extDielectric::printInt(FILE* fp,
                             const char* sep,
                             const char* key,
                             int v,
                             bool pos)
{
  if (pos && !(v > 0)) {
    return;
  }

  fprintf(fp, "%s%s %d\n", sep, key, v);
}
void extDielectric::printDouble(FILE* fp,
                                const char* sep,
                                const char* key,
                                double v,
                                bool pos)
{
  if (pos && !(v > 0)) {
    return;
  }

  fprintf(fp, "%s%s %g\n", sep, key, v);
}

void extDielectric::printDielectric(FILE* fp, Parser* parse)
{
  fprintf(fp, "DIELECTRIC {\n");

  printString(fp, "\t", "name", _name, false);
  printDouble(fp, "\t", "epsilon", _epsilon, false);
  if (!_conformal && !_trench) {
    printString(fp, "\t", "non_conformal_metal", _non_conformal_metal, true);
  }

  printDouble(fp, "\t", "height", _height, true);
  printDouble(fp, "\t", "thickness", _thickness, true);

  printDouble(fp, "\t", "left_thickness", _left_thickness, true);
  printDouble(fp, "\t", "right_thickness", _right_thickness, true);
  printDouble(fp, "\t", "top_thickness", _top_thickness, true);
  printDouble(fp, "\t", "bottom_thickness", _bottom_thickness, true);
  printDouble(fp, "\t", "bottom_ext", _bottom_ext, true);
  printDouble(fp, "\t", "slope", _slope, false);
  printInt(fp, "\t", "met", _met, true);
  printInt(fp, "\t", "next_met", _nextMet, true);

  fprintf(fp, "}\n");
}
void extDielectric::printDielectric(FILE* fp,
                                    float planeWidth,
                                    float planeThickness)
{
  fprintf(fp,
          "BOX NAME %-15s; CX=0; CY=%g; W=%g; H= %g; DIEL= %g;\n",
          _name,
          _height,
          planeWidth,
          planeThickness,
          _epsilon);
}
void extDielectric::printDielectric3D(FILE* fp,
                                      float blockWidth,
                                      float blockThickness,
                                      float blockLength)
{
  fprintf(fp,
          "BLOCK NAME %-15s; V1=0,%g,%g; LENGTH=%g; WIDTH=%g; HEIGHT=%g; "
          "DIEL=%g;\n",
          _name,
          _height - (blockThickness * 0.5),
          blockLength * 0.5,
          blockThickness,
          blockWidth,
          blockLength,
          _epsilon);
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
void extMasterConductor::writeRaphaelDielPoly3D(FILE* fp,
                                                double X,
                                                double width,
                                                double length,
                                                extDielectric* diel)
{
  fprintf(fp, "POLY3D NAME= %s; ", diel->_name);

  fprintf(fp, " COORD= ");

  writeRaphaelPointXY(fp, X + _loLeft[0], _loLeft[2]);
  writeRaphaelPointXY(fp, X + _loRight[0], _loLeft[2]);
  writeRaphaelPointXY(fp, X + _hiRight[0], _hiRight[2]);
  writeRaphaelPointXY(fp, X + _hiLeft[0], _hiLeft[2]);

  fprintf(fp, "V1=0,0,0; HEIGHT=%g;", length);
  fprintf(fp, " DIEL=%g ;\n", diel->_epsilon);
}
void extMasterConductor::printDielBox(FILE* fp,
                                      double X,
                                      double width,
                                      extDielectric* diel,
                                      const char* width_name)
{
  // non conformal

  double thickness = _hiLeft[2] - _loLeft[2];
  if (thickness == 0.0) {
    return;
  }
  if (width_name != nullptr) {
    double height = _loLeft[2];

    fprintf(fp,
            "BOX NAME=%-15s; CX=0; CY=%6.3f; W=%s; H= %.3f; DIEL= %g;\n",
            diel->_name,
            height,
            width_name,
            thickness,
            diel->_epsilon);

    /*		fprintf(stdout, "BOX NAME=%-15s; CX=0; CY=%6.3f; W=%s; H= %.3f;
       DIEL= %g;\n", diel->_name, height, width_name, thickness,
       diel->_epsilon);
    */
    logger_->info(
        RCX,
        156,
        "BOX NAME={:-15s}; CX=0; CY={:6.3f}; W={}; H= {:.3f}; DIEL= {};",
        diel->_name,
        height,
        width_name,
        thickness,
        diel->_epsilon);
  } else {
    _loRight[0] = _loLeft[0] + width;
    _hiRight[0] = _hiLeft[0] + width;

    writeRaphaelDielPoly(fp, X, width, diel);

    // writeRaphaelDielPoly(stdout, X, width, diel);
  }
}
void extMasterConductor::printDielBox3D(FILE* fp,
                                        double X,
                                        double width,
                                        double length,
                                        extDielectric* diel,
                                        const char* width_name)
{
  // non conformal

  if (width_name != nullptr) {
    //                double height= _loLeft[2];
    double thickness = _hiLeft[2] - _loLeft[2];

    fprintf(fp,
            "BLOCK NAME=%-15s; V1=0,0,%6.3f; WIDTH=%s; LENGTH= %.3f; "
            "HEIGHT=%6.3f; DIEL= %g;\n",
            diel->_name,
            0.5 * length,
            width_name,
            thickness,
            length,
            diel->_epsilon);
    /*        	fprintf(stdout, "BLOCK NAME=%-15s; V1=0,0,%6.3f; WIDTH=%s;
       LENGTH= %.3f; HEIGHT=%6.3f; DIEL= %g;\n", diel->_name, 0.5*length,
       width_name, thickness, length, diel->_epsilon);
    */
    logger_->info(
        RCX,
        157,
        "BLOCK NAME={:-15s}; V1=0, 0,{:6.3f}; WIDTH={}; LENGTH= {.3f}; "
        "HEIGHT={:6.3f}; DIEL= {:g};",
        diel->_name,
        0.5 * length,
        width_name,
        thickness,
        length,
        diel->_epsilon);
  } else {
    _loRight[0] = _loLeft[0] + width;
    _hiRight[0] = _hiLeft[0] + width;

    writeRaphaelDielPoly3D(fp, X, width, length, diel);
  }
}
void extMasterConductor::printDielHeights(FILE* fp, extDielectric* diel)
{
}
void extProcess::writeProcess(FILE* fp,
                              char* gndName,
                              float planeWidth,
                              float planeThickness)
{
  fprintf(fp,
          "WINDOW X1=%g; Y1=-1.0; X2=%g; Y2=%g; DIEL=1.0;\n",
          -planeWidth * 0.5,
          planeWidth * 0.5,
          planeThickness);

  for (uint32_t jj = 1; jj < _dielTable->getCnt(); jj++) {
    extDielectric* diel = _dielTable->get(jj);
    //		if (diel->_conformal)
    diel->printDielectric(fp, planeWidth, planeThickness);
  }
  fprintf(fp,
          "BOX NAME=%s CX=0; CY=-0.5; W=%g; H=1.0; VOLT=0.0;\n",
          gndName,
          planeWidth);

  fprintf(fp, "OPTIONS SET_GRID=10000;\n");
}
void extProcess::writeProcess3D(FILE* fp,
                                char* gndName,
                                float blockWidth,
                                float blockThickness,
                                float blockLength)
{
  fprintf(fp,
          "WINDOW3D V1=%g,0,0; V2=%g,%g,%g; DIEL=1.0;\n",
          -blockWidth * 0.5,
          blockWidth * 0.5,
          blockThickness,
          blockLength);

  for (uint32_t jj = 1; jj < _dielTable->getCnt(); jj++) {
    extDielectric* diel = _dielTable->get(jj);
    //                if (diel->_conformal)
    diel->printDielectric3D(fp, blockWidth, blockThickness, blockLength);
  }
  fprintf(
      fp,
      "BLOCK NAME=%s; V1=0,-1,%g; LENGTH=1.0; WIDTH=%g, HEIGHT=%g; VOLT=0.0;\n",
      gndName,
      blockLength * 0.5,
      blockWidth,
      blockLength);

  fprintf(fp, "OPTIONS SET_GRID=1000000;\n");
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
void extProcess::writeWindow3D(FILE* fp,
                               const char* param_width_name,
                               double y1,
                               const char* param_thickness_name,
                               const char* param_length_name)
{
  fprintf(fp, "\n$ Simulation window\n");
  fprintf(fp,
          "WINDOW3D V1=-0.5*%s,%g,0; V2=0.5*%s,%s,%s; DIEL=1.0;\n",
          param_width_name,
          y1,
          param_width_name,
          param_thickness_name,
          param_length_name);
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
void extProcess::writeGround3D(FILE* fp,
                               int met,
                               const char* name,
                               double width,
                               double length,
                               double x1,
                               double volt,
                               bool diag)
{
  if (met < 0) {
    return;
  }

  double y1 = -1.0;
  double th = 1.0;
  if (!met || diag) {
    if (diag) {
      fprintf(fp, "POLY3D NAME= M0__w0; ");
    } else {
      fprintf(fp, "POLY3D NAME= M%d__w0; ", met);
    }
    fprintf(fp, " COORD= ");
    writeRaphaelPointXY(fp, x1, y1);
    writeRaphaelPointXY(fp, x1 + width, y1);
    writeRaphaelPointXY(fp, x1 + width, y1 + th);
    writeRaphaelPointXY(fp, x1, y1 + th);
    fprintf(fp, " V1=0,0,0; HEIGHT=%g;", length);
    fprintf(fp, " VOLT=%g ;\n", volt);
    fprintf(fp, "OPTIONS SET_GRID=1000000;\n\n");
    return;
  }

  extMasterConductor* m = getMasterConductor(met);
  y1 = m->_loLeft[2];
  th = m->_hiLeft[2] - y1;
  const double w = getConductor(met)->_min_width;
  const double s = getConductor(met)->_min_spacing;
  const double p = w + s;

  double l = 2 * p;
  const double end = length - 2 * p;
  while (l <= end) {
    fprintf(fp, "POLY3D NAME= M%d__w0; ", met);
    fprintf(fp, " COORD= ");
    writeRaphaelPointXY(fp, x1, y1);
    writeRaphaelPointXY(fp, x1 + width, y1);
    writeRaphaelPointXY(fp, x1 + width, y1 + th);
    writeRaphaelPointXY(fp, x1, y1 + th);

    fprintf(fp, " V1=0,0,%g; HEIGHT=%g;", l, w);
    fprintf(fp, " VOLT=%g ;\n", volt);
    l += p;
  }

  fprintf(fp, "OPTIONS SET_GRID=1000000;\n\n");
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
void extProcess::writeFullProcess(FILE* fp,
                                  double X,
                                  double width,
                                  char* width_name)
{
  for (uint32_t jj = 1; jj < _masterDielectricTable->getCnt(); jj++) {
    extMasterConductor* m = _masterDielectricTable->get(jj);
    extDielectric* diel = _dielTable->get(m->_condId);

    // if (diel->_conformal)
    m->printDielBox(fp, X, width, diel, width_name);
  }
}
void extProcess::writeFullProcess3D(FILE* fp,
                                    double X,
                                    double width,
                                    double length,
                                    char* width_name)
{
  for (uint32_t jj = 1; jj < _masterDielectricTable->getCnt(); jj++) {
    extMasterConductor* m = _masterDielectricTable->get(jj);
    extDielectric* diel = _dielTable->get(m->_condId);

    // if (diel->_conformal)
    m->printDielBox3D(fp, X, width, length, diel, width_name);
  }
}
double extProcess::writeProcessHeights(FILE* fp,
                                       double X,
                                       double width,
                                       double length,
                                       char* width_name,
                                       double height_base,
                                       double height_ceiling)
{
  double height = 0;
  for (uint32_t jj = 1; jj < _masterDielectricTable->getCnt(); jj++) {
    extMasterConductor* m = _masterDielectricTable->get(jj);
    extDielectric* diel = _dielTable->get(m->_condId);

    double h1 = diel->_height;
    height += h1;
    if (height >= height_ceiling) {
      break;
    }

    height += diel->_thickness;
    if (height <= height_base) {
      continue;
    }

    // if (diel->_conformal)
    m->writeDiel(fp, diel->_name, diel->_epsilon, height_base);
  }
  return height;
}

void extProcess::writeFullProcess(FILE* fp,
                                  char* gndName,
                                  double planeWidth,
                                  double planeThickness)
{
  fprintf(fp, "param plane_width=%g;\n", planeWidth);

  fprintf(fp,
          "WINDOW X1=%g; Y1=-1.0; X2=%g; Y2=%g; DIEL=1.0;\n",
          -planeWidth * 0.5,
          planeWidth * 0.5,
          planeThickness);

  for (uint32_t jj = 1; jj < _masterDielectricTable->getCnt(); jj++) {
    extMasterConductor* m = _masterDielectricTable->get(jj);
    extDielectric* diel = _dielTable->get(m->_condId);

    // if (diel->_conformal)
    m->printDielBox(fp, 0.0, planeWidth, diel, "plane_width");
  }
  fprintf(fp,
          "\nBOX NAME=%s CX=0; CY=-0.5; W=%g; H=1.0; VOLT=0.0;\n",
          gndName,
          planeWidth);

  fprintf(fp, "OPTIONS SET_GRID=10000;\n\n");
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

  // CX,CY bug : writeFullProcess(wfp, X, width, widthName);
  writeFullProcess(wfp, X, width, nullptr);

  double y1 = 0.0;
  writeWindow(wfp, widthName, y1, thicknessName);

  fprintf(wfp, "\n$ Ground Plane(s)\n");
  //	writeGround(wfp, underMet, gndName, widthName, 0.0, 0.0);
  //	writeGround(wfp, overMet, gndName, widthName, 0.0, 0.0);

  writeGround(wfp, underMet, gndName, width, -0.5 * width, 0.0, diag);
  writeGround(wfp, overMet, gndName, width, -0.5 * width, 0.0, diag);
}
void extProcess::writeProcessAndGround3D(FILE* wfp,
                                         const char* gndName,
                                         int underMet,
                                         int overMet,
                                         double X,
                                         double width,
                                         double length,
                                         double thickness,
                                         double W,
                                         bool diag)
{
  double wid, x;
  if (width > W) {
    wid = width;
    x = X;
  } else {
    wid = W + 10.0;
    x = -wid * 0.5;
  }
  const char* widthName = "window_width";
  const char* thicknessName = "window_thichness";
  const char* lengthName = "window_length";

  writeParam(wfp, widthName, wid);
  writeParam(wfp, thicknessName, thickness);
  writeParam(wfp, lengthName, length);

  // CX,CY bug : writeFullProcess(wfp, X, width, widthName);
  writeFullProcess3D(wfp, x, width, length, nullptr);

  double y1 = 0.0;
  writeWindow3D(wfp, widthName, y1, thicknessName, lengthName);

  fprintf(wfp, "\n$ Ground Plane(s)\n");

  if (underMet <= 0) {
    writeGround3D(wfp, 0, gndName, wid, 0, -0.5 * wid, 0.0);
  }

  double w, s, offset;

  if (underMet > 0) {
    w = getConductor(underMet)->_min_width;
    s = getConductor(underMet)->_min_spacing;
    offset = w + s;
    wid = W + 2 * offset;
    writeGround3D(wfp, underMet, gndName, wid, length, -0.5 * wid, 0.0);
  }
  if (overMet > 0) {
    w = getConductor(overMet)->_min_width;
    s = getConductor(overMet)->_min_spacing;
    offset = w + s;
    wid = W + 2 * offset;
    writeGround3D(wfp, overMet, gndName, wid, length, -0.5 * wid, 0.0);
  }
}
double extMasterConductor::writeGround(FILE* fp,
                                       int met,
                                       int wire_num,
                                       double th)
{
  fprintf(fp, "GROUND_PLANE %d M%d_w%d", met, met, wire_num);

  writePointXY(fp, "HEIGHT", _loRight[2], _hiRight[2]);

  fprintf(fp, "   THICKNESS %6.3f\n", th);
  return _hiRight[2];
}
double extProcess::writeProcessAndGroundPlanes(FILE* wfp,
                                               const char* gndName,
                                               int underMet,
                                               int overMet,
                                               double X,
                                               double width,
                                               double length,
                                               double thickness,
                                               double W,
                                               bool apply_height_offset,
                                               double& height_ceiling,
                                               bool diag)
{
  double height_base = 0.0;
  height_ceiling = 100000;
  if (underMet > 0) {
    // DELETE double w = getConductor(underMet)->_min_width;
    // DELETE double s = getConductor(underMet)->_min_spacing;
    double th = getConductor(underMet)->_thickness;

    extMasterConductor* a = getMasterConductor(underMet);

    if (apply_height_offset) {
      double h1 = getConductor(underMet)->_height;
      double th1 = getConductor(underMet)->_thickness;
      height_base = h1 + th1;
      fprintf(wfp,
              "GROUND_PLANE %d M%d_w%d HEIGHT %6.3f %6.3f  THICKNESS %6.3f\n",
              underMet,
              underMet,
              underMet,
              -th1,
              0.0,
              th1);
    } else {
      a->writeGround(wfp, underMet, 0, th);
    }
  } else {
    if (apply_height_offset) {
      fprintf(wfp, "GROUND_PLANE 0 M0_w0  HEIGHT -0.1 0.0  THICKNESS 0.1\n");
    } else {
      fprintf(wfp, "GROUND_PLANE 0 M0_w0  HEIGHT 0.0 0.0  THICKNESS 0.0\n");
    }
  }
  if (overMet > 0) {
    // DELETE double w = getConductor(overMet)->_min_width;
    // DELETE double s = getConductor(overMet)->_min_spacing;
    double th = getConductor(overMet)->_thickness;
    double h2 = getConductor(overMet)->_height;

    extMasterConductor* a = getMasterConductor(overMet);
    if (apply_height_offset) {
      height_ceiling = getConductor(overMet)->_height;

      fprintf(wfp,
              "GROUND_PLANE %d M%d_w%d HEIGHT %6.3f %6.3f  THICKNESS %6.3f\n",
              overMet,
              overMet,
              overMet,
              h2 - height_base,
              h2 + th - height_base,
              th);
    } else {
      a->writeGround(wfp, overMet, 0, th);
    }
  }
  fprintf(wfp, "\n# Process Stack -- Dielectric Heights\n");
  double height = writeProcessHeights(
      wfp, 0, width, length, nullptr, height_base, height_ceiling);
  if (overMet <= 0) {
    height_ceiling = height;
  }

  if (!apply_height_offset) {
    height_ceiling = height;
  }

  return height_base;
}

bool extDielectric::readDielectric(Parser* parser)
{
  if (strcmp("non_conformal_metal", parser->get(0)) == 0) {
    strcpy(_non_conformal_metal, parser->get(1));
    _conformal = false;
    _trench = false;
    return true;
  }
  if (strcmp("conformal", parser->get(0)) == 0) {
    _conformal = true;
    return true;
  }
  if (strcmp("trench", parser->get(0)) == 0) {
    _trench = true;
    return true;
  }
  if (setDoubleVal(parser, "epsilon", 1, _epsilon)) {
    return true;
  }
  if (setDoubleVal(parser, "distance", 1, _distance)) {
    return true;
  }
  if (setIntVal(parser, "met", 1, _met)) {
    return true;
  }
  if (setIntVal(parser, "next_met", 1, _nextMet)) {
    return true;
  }
  if (setDoubleVal(parser, "height", 1, _height)) {
    return true;
  }
  if (setDoubleVal(parser, "thickness", 1, _thickness)) {
    return true;
  }
  if (setDoubleVal(parser, "bottom_thickness", 1, _bottom_thickness)) {
    return true;
  }
  if (setDoubleVal(parser, "top_thickness", 1, _top_thickness)) {
    return true;
  }
  if (setDoubleVal(parser, "left_thickness", 1, _left_thickness)) {
    return true;
  }
  if (setDoubleVal(parser, "right_thickness", 1, _right_thickness)) {
    return true;
  }
  if (setDoubleVal(parser, "bottom_ext", 1, _bottom_ext)) {
    return true;
  }
  if (setDoubleVal(parser, "slope", 1, _slope)) {
    return true;
  }

  return false;
}
extMasterConductor::extMasterConductor(uint32_t condId,
                                       extConductor* cond,
                                       double prevHeight,
                                       utl::Logger* logger)
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
      /*			fprintf(stdout, "Cannot determine Bottom Width
         for Conductor <%s>\n", cond->_name);
      */
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
      /*			fprintf(stdout, "Cannot determine Top Width for
         Conductor <%s>\n", cond->_name);
      */
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
    /*		fprintf(stdout, "Cannot determine thickness for Conductor
       <%s>\n", cond->_name);
    */
    logger_->warn(
        RCX, 153, "Can't determine thickness for Conductor <{}>", cond->_name);
    exit(0);
  }
  _hiLeft[2] = height + thickness;
  _hiRight[2] = height + thickness;

  _dy = 0;
  _e = 0.0;
  for (unsigned int& id : _conformalId) {
    id = 0;
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

  //_hiLeft[2]= height + thickness;
  //_hiRight[2]= height + thickness;

  //_loLeft[2]= height;
  //_loRight[2]= height;
}
double extMasterConductor::writeRaphaelBox(FILE* fp,
                                           uint32_t wireNum,
                                           double width,
                                           double X,
                                           double volt)
{
  return writeRaphaelPoly(fp, wireNum, width, X, volt);

  fprintf(fp, "BOX NAME=");
  writeBoxName(fp, wireNum);

  fprintf(fp,
          " CX=%g; CY=%g; W=%g; H=%g; VOLT=%g\n",
          X,
          _loLeft[2],
          width,
          _hiLeft[2] - _loLeft[2],
          volt);

  return X + width;
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
                                          uint32_t wireNum,
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
void extMasterConductor::writeRaphaelConformalPoly(FILE* fp,
                                                   double width,
                                                   double X,
                                                   extProcess* p)
{
  double height[3] = {0, 0, 0};
  double thickness[3] = {0, 0, 0};
  double bottom_ext[3] = {0, 0, 0};
  double slope[3] = {0, 0, 0};
  double e[3] = {0, 0, 0};
  bool trench = false;
  uint32_t cnt = 0;
  uint32_t start = -1;
  double h = _loLeft[2];
  for (uint32_t i = 0; i < 3; i++) {
    uint32_t j = 2 - i;
    if (!_conformalId[j]) {
      continue;
    }
    if (start == -1) {
      start = i;
    }
    extDielectric* d = p->getDielectric(_conformalId[j]);
    // assuming conformal and trench will not show up at the same time. Also
    // height for the trench layer is negative.
    trench = d->_trench;
    height[i] = d->_height;
    thickness[i] = d->_thickness;
    bottom_ext[i] = d->_bottom_ext;
    slope[i] = d->_slope;
    e[i] = d->_epsilon;
    h += d->_thickness;
    cnt++;
  }
  if (!cnt) {
    return;
  }
  if (trench) {
    for (uint32_t j = start; j < start + cnt; j++) {
      fprintf(fp, "POLY NAME=");
      fprintf(fp, "M%d_Trench%d;", _condId, 2 - j);
      if (width == 0.0) {
        writeRaphaelPointXY(
            fp, X + _loLeft[0] - bottom_ext[j], _hiRight[2] + height[j]);
        writeRaphaelPointXY(
            fp, X + _loRight[0] + bottom_ext[j], _hiRight[2] + height[j]);
        writeRaphaelPointXY(fp, X + _hiRight[0] + bottom_ext[j], _hiRight[2]);
        writeRaphaelPointXY(fp, X + _hiLeft[0] - bottom_ext[j], _hiRight[2]);
        fprintf(fp, " DIEL=%g\n", e[j]);
      } else {
        writeRaphaelPointXY(fp, X - bottom_ext[j], _hiRight[2] + height[j]);
        writeRaphaelPointXY(
            fp, X + width + bottom_ext[j], _hiRight[2] + height[j]);
        writeRaphaelPointXY(fp, X + width + bottom_ext[j], _hiRight[2]);
        writeRaphaelPointXY(fp, X - bottom_ext[j], _hiRight[2]);
        fprintf(fp, " DIEL=%g\n", e[j]);
      }
    }
    return;
  }
  double dx;
  for (uint32_t j = start; j < start + cnt; j++) {
    fprintf(fp, "POLY NAME=");
    fprintf(fp, "M%d_Conformal%d;", _condId, 2 - j);
    if (width == 0.0) {
      if (slope[j]) {
        dx = (height[j] - thickness[j]) / slope[j];
      } else {
        dx = bottom_ext[j];
      }
      writeRaphaelPointXY(fp, X + _loLeft[0] - bottom_ext[j], h);
      writeRaphaelPointXY(fp, X + _loRight[0] + bottom_ext[j], h);
      h -= thickness[j];
      writeRaphaelPointXY(
          fp, X + _hiRight[0] + bottom_ext[j] - dx, h + height[j]);
      writeRaphaelPointXY(
          fp, X + _hiLeft[0] - bottom_ext[j] + dx, h + height[j]);
      fprintf(fp, " DIEL=%g\n", e[j]);
    } else {
      if (slope[j]) {
        dx = (height[j] - thickness[j]) / slope[j];
      } else {
        dx = bottom_ext[j];
      }
      writeRaphaelPointXY(fp, X - bottom_ext[j], h);
      writeRaphaelPointXY(fp, X + width + bottom_ext[j], h);
      h -= thickness[j];
      writeRaphaelPointXY(fp, X + width + bottom_ext[j] - dx, h + height[j]);
      writeRaphaelPointXY(fp, X - bottom_ext[j] + dx, h + height[j]);
      fprintf(fp, " DIEL=%g\n", e[j]);
    }
  }
}
void extMasterConductor::writeRaphaelConformalGround(FILE* fp,
                                                     double X,
                                                     double width,
                                                     extProcess* p)
{
  double height[3] = {0, 0, 0};
  double thickness[3] = {0, 0, 0};
  double e[3] = {0, 0, 0};
  bool trench = false;
  uint32_t cnt = 0;
  uint32_t start = -1;
  double h = _loLeft[2];
  for (uint32_t i = 0; i < 3; i++) {
    uint32_t j = 2 - i;
    if (!_conformalId[j]) {
      continue;
    }
    if (start == -1) {
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
    for (uint32_t j = start; j < start + cnt; j++) {
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
  for (uint32_t j = start; j < start + cnt; j++) {
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
void extMasterConductor::writeRaphaelPoly3D_w(FILE* fp,
                                              uint32_t wireNum,
                                              double X,
                                              double width,
                                              double length,
                                              double volt)
{
  // writeWire3D(fp, wireNum, X, width, length, volt);

  fprintf(fp, "POLY3D NAME=");
  writeBoxName(fp, wireNum);

  fprintf(fp, " COORD= ");

  double X2 = X + width;
  writeRaphaelPointXY(fp, X, _loLeft[2]);
  writeRaphaelPointXY(fp, X2, _loRight[2]);
  writeRaphaelPointXY(fp, X2, _hiRight[2]);
  writeRaphaelPointXY(fp, X, _hiLeft[2]);

  if (false) {
    fprintf(fp, "\nPOLY3D NAME=");
    writeBoxName(fp, wireNum);

    fprintf(fp, " COORD= ");
    writeRaphaelPointXY(fp, X + _loLeft[0], _loLeft[2]);
    writeRaphaelPointXY(fp, X + _loRight[0], _loRight[2]);
    writeRaphaelPointXY(fp, X + _hiRight[0], _hiRight[2]);
    writeRaphaelPointXY(fp, X + _hiLeft[0], _hiLeft[2]);
  }

  fprintf(fp, "V1=0,0,0; HEIGHT=%g;", length);

  fprintf(fp, " VOLT=%g\n", volt);
  if (false) {
    printf("X= %g\n", X);
    printf("%g  %g\n", _loLeft[0], _loLeft[2]);
    printf("%g  %g\n", _loRight[0], _loRight[2]);
    printf("%g  %g\n", _hiRight[0], _hiRight[2]);
    printf("%g  %g\n", _hiLeft[0], _hiLeft[2]);

    writeRaphaelPointXY(stdout, X + _loLeft[0], _loLeft[2]);
    writeRaphaelPointXY(stdout, X + _loRight[0], _loRight[2]);
    writeRaphaelPointXY(stdout, X + _hiRight[0], _hiRight[2]);
    writeRaphaelPointXY(stdout, X + _hiLeft[0], _hiLeft[2]);
  }
}
void extMasterConductor::writePointXY(FILE* fp,
                                      const char* suffix,
                                      double X,
                                      double Y,
                                      const char* postfix)
{
  fprintf(fp, " %s %6.3f %6.3f%s", suffix, X, Y, postfix);
}
void extMasterConductor::writeWire3D(FILE* fp,
                                     uint32_t wireNum,
                                     double X,
                                     double width,
                                     double length,
                                     double height_offset,
                                     double volt)
{
  writeWireName(fp, wireNum);

  double X2 = X + width;
  writePointXY(fp, "LL", X, _loLeft[2] - height_offset);
  writePointXY(fp, "LR", X2, _loRight[2] - height_offset);
  writePointXY(fp, "UR", X2, _hiRight[2] - height_offset);
  writePointXY(fp, "UL", X, _hiLeft[2] - height_offset);

  fprintf(fp, "   LENGTH %6.3f", length);

  fprintf(fp, "   VOLTAGE %g\n", volt);
}
void extMasterConductor::writeDiel(FILE* fp,
                                   const char* name,
                                   double epsilon,
                                   double height_offset)
{
  fprintf(fp, "DIELECTRIC %s ", name);

  writePointXY(
      fp, "HEIGHT", _loRight[2] - height_offset, _hiRight[2] - height_offset);

  fprintf(fp, "   EPSILON %6.3f\n", epsilon);
}
void extMasterConductor::writeRaphaelPoly3D(FILE* fp,
                                            uint32_t wireNum,
                                            double X,
                                            double length,
                                            double volt)
{
  fprintf(fp, "POLY3D NAME=");
  writeBoxName(fp, wireNum);

  fprintf(fp, " COORD= ");

  writeRaphaelPointXY(fp, X + _loLeft[0], _loLeft[2]);
  writeRaphaelPointXY(fp, X + _loRight[0], _loRight[2]);
  writeRaphaelPointXY(fp, X + _hiRight[0], _hiRight[2]);
  writeRaphaelPointXY(fp, X + _hiLeft[0], _hiLeft[2]);

  fprintf(fp, "V1=0,0,0; HEIGHT=%g;", length);

  fprintf(fp, " VOLT=%g\n", volt);
  if (false) {
    printf("X= %g\n", X);
    printf("%g  %g\n", _loLeft[0], _loLeft[2]);
    printf("%g  %g\n", _loRight[0], _loRight[2]);
    printf("%g  %g\n", _hiRight[0], _hiRight[2]);
    printf("%g  %g\n", _hiLeft[0], _hiLeft[2]);

    writeRaphaelPointXY(stdout, X + _loLeft[0], _loLeft[2]);
    writeRaphaelPointXY(stdout, X + _loRight[0], _loRight[2]);
    writeRaphaelPointXY(stdout, X + _hiRight[0], _hiRight[2]);
    writeRaphaelPointXY(stdout, X + _hiLeft[0], _hiLeft[2]);
  }
}
double extMasterConductor::writeRaphaelPoly(FILE* fp,
                                            uint32_t wireNum,
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
double extMasterConductor::writeRaphaelPoly3D(FILE* fp,
                                              uint32_t wireNum,
                                              double width,
                                              double length,
                                              double X,
                                              double volt)
{
  fprintf(fp, "POLY3D NAME=");
  writeBoxName(fp, wireNum);

  fprintf(fp, " COORD= ");

  writeRaphaelPointXY(fp, X, _loLeft[2]);
  writeRaphaelPointXY(fp, X + width, _loLeft[2]);
  writeRaphaelPointXY(fp, X + width, _hiRight[2]);
  writeRaphaelPointXY(fp, X, _hiLeft[2]);

  fprintf(fp, " V1=0,0,0; HEIGHT=%g;", length);
  fprintf(fp, " VOLT=%g\n", volt);
  return X + width;
}
void extMasterConductor::writeBoxName(FILE* fp, uint32_t wireNum)
{
  fprintf(fp, "M%d_w%d;", _condId, wireNum);
}
void extMasterConductor::writeWireName(FILE* fp, uint32_t wireNum)
{
  fprintf(fp, "WIRE %d M%d_w%d", wireNum, _condId, wireNum);
}
extMasterConductor::extMasterConductor(uint32_t dielId,
                                       extDielectric* diel,
                                       double xlo,
                                       double dx1,
                                       double xhi,
                                       double dx2,
                                       double h,
                                       double th,
                                       utl::Logger* logger)
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
    /*		fprintf(stdout, "Cannot determine thickness for Diel <%s>\n",
                            diel->_name);
    */
    logger_->warn(
        RCX, 154, "Can't determine thickness for Diel <{}>", diel->_name);
    //		exit(0);
  }
  _hiLeft[2] = h + th;
  _hiRight[2] = h + th;

  _dy = 0;
  _e = diel->_epsilon;
  for (unsigned int& id : _conformalId) {
    id = 0;
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
    // exit(0);
  }
  return fp;
}
double extProcess::adjustMasterLayersForHeight(uint32_t met, double thickness)
{
  double condThickness = _condTable->get(met)->_thickness;
  double dth = (thickness - condThickness) / condThickness;

  double h = 0.0;
  for (uint32_t ii = 1; ii < _masterConductorTable->getCnt(); ii++) {
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
double extProcess::adjustMasterDielectricsForHeight(uint32_t met, double dth)
{
  double h = 0.0;
  for (uint32_t ii = 1; ii < _masterDielectricTable->getCnt(); ii++) {
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
void extProcess::createMasterLayers()
{
  double upperCondHeight = 0;
  for (uint32_t ii = 1; ii < _condTable->getCnt(); ii++) {
    extConductor* cond = _condTable->get(ii);
    extMasterConductor* m
        = new extMasterConductor(ii, cond, upperCondHeight, logger_);
    _masterConductorTable->add(m);
    upperCondHeight = cond->_height + cond->_thickness;
  }
  double h = 0.0;
  for (uint32_t jj = 1; jj < _dielTable->getCnt(); jj++) {
    extDielectric* diel = _dielTable->get(jj);
    if ((diel->_conformal || diel->_trench) && diel->_met) {
      extMasterConductor* mm = _masterConductorTable->get(diel->_met);
      for (unsigned int& id : mm->_conformalId) {
        if (!id) {
          id = jj;
          break;
        }
      }
    }

    //		if (diel->_conformal) {
    extMasterConductor* m = new extMasterConductor(
        jj, diel, 0, 0, 0, 0, h, diel->_thickness, logger_);
    h += diel->_thickness;

    _masterDielectricTable->add(m);
    /*		}
                    else {
                            extMasterConductor *m=
                                    new extMasterConductor(jj, diel, 0, 0, 0, 0,
       0, 0);

                            _masterDielectricTable->add(m);
                    }
    */
  }
}
extConductor* extProcess::getConductor(uint32_t ii)
{
  return _condTable->get(ii);
}
extMasterConductor* extProcess::getMasterConductor(uint32_t ii)
{
  return _masterConductorTable->get(ii);
}
extDielectric* extProcess::getDielectric(uint32_t ii)
{
  return _dielTable->get(ii);
}
Array1D<double>* extProcess::getWidthTable(uint32_t met)
{
  double min_width = getConductor(met)->_min_width;

  //	const double wTable[7]= {1.0, 1.5, 2.0, 2.5, 3, 3.5, 4};
  // const double wTable[3]= {1.0, 1.5, 2.0 };
  const double wTable[8] = {1.0, 1.5, 2.0, 2.5, 3, 4, 5, 10};

  //	Array1D<double>* A= new Array1D<double>(8);
  Array1D<double>* A = new Array1D<double>(11);
  //	for (uint32_t ii= 0; ii<7; ii++) {
  // for (uint32_t ii= 0; ii<3; ii++) {
  for (double wi : wTable) {
    double w = wi * min_width;
    A->add(w);
  }

  return A;
}
Array1D<double>* extProcess::getSpaceTable(uint32_t met)
{
  double min_spacing = getConductor(met)->_min_spacing;

  //	const double sTable[18]= {1.0, 1.2, 1.5, 1.7, 2.0, 2.25, 2.5, 2.75,
  // 3, 3.5, 4, 4.5, 5, 6, 7, 8, 9, 10};
  // DKF 0702727 const double sTable[8] = {1.0, 1.5, 2.0, 2.5, 3, 4, 5, 10};

  const double sTable[5] = {1.0, 1.5, 2.0, 3, 5};
  // const double sTable[3]= {1.0, 1.5, 2.0};

  Array1D<double>* A = new Array1D<double>(5);
  for (double si : sTable) {
    double s = si * min_spacing;
    A->add(s);
  }

  return A;
}
Array1D<double>* extProcess::getDiagSpaceTable(uint32_t met)
{
  double min_spacing = getConductor(met)->_min_spacing;
  double min_width = getConductor(met)->_min_width;
  double p = min_spacing + min_width;

  // const double sTable[7] = {0, 0.2, 0.5, 0.7, 1.0, 2.0, 3};
  // const double sTable[5] = {0, 0.5, 1.0, 2.0, 3};
  const double sTable[3] = {0, 1.0, 2.0};

  Array1D<double>* A = new Array1D<double>(8);
  // for (uint32_t ii = 0; ii < 7; ii++) {
  // for (uint32_t ii = 0; ii < 5; ii++) {
  for (double si : sTable) {
    double s = si * p;
    A->add(s);
  }

  return A;
}
Array1D<double>* extProcess::getDataRateTable(uint32_t met)
{
  if (_dataRateTable) {
    return _dataRateTable;
  }
  Array1D<double>* A = new Array1D<double>(8);
  A->add(0.0);
  return A;
}
void extProcess::readDataRateTable(Parser* parser, const char* keyword)
{
  if ((keyword != nullptr) && (strcmp(keyword, parser->get(0)) != 0)) {
    return;
  }

  if (parser->getWordCnt() < 1) {
    return;
  }
  Array1D<double>* A = new Array1D<double>(parser->getWordCnt() + 1);
  A->add(0.0);
  parser->getDoubleArray(A, 1);
  _dataRateTable = A;
  _thickVarFlag = true;
}
bool extProcess::getMaxMinFlag()
{
  return _maxMinFlag;
}
bool extProcess::getThickVarFlag()
{
  return _thickVarFlag;
}
extMasterConductor* extProcess::getMasterConductor(uint32_t met,
                                                   uint32_t wIndex,
                                                   uint32_t sIndex,
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
extVariation* extProcess::getVariation(uint32_t met)
{
  extConductor* m = getConductor(met);

  extVariation* v = nullptr;
  if (m->_var_table_index > 0) {
    v = _varTable->get(m->_var_table_index);
  }

  return v;
}
double extVariation::interpolate(double w,
                                 Array1D<double>* X,
                                 Array1D<double>* Y)
{
  if (X->getCnt() < 2) {
    return w;
  }

  int jj = X->findNextBiggestIndex(w);

  if (jj >= X->getCnt() - 1) {
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
double extVariation::getThickness(double w, uint32_t dIndex)
{
  return interpolate(w, _thicknessC->_width, _thicknessC->_vTable[dIndex]);
}
double extVariation::getThicknessR(double w, uint32_t dIndex)
{
  return interpolate(w, _thicknessR->_width, _thicknessR->_vTable[dIndex]);
}
double extVariation::getBottomWidth(double w, uint32_t dIndex)
{
  return interpolate(w, _loWidthC->_width, _loWidthC->_vTable[dIndex]);
}
double extVariation::getBottomWidthR(double w, uint32_t dIndex)
{
  return interpolate(w, _loWidthR->_width, _loWidthR->_vTable[dIndex]);
}
double extVariation::getTopWidth(uint32_t ii, uint32_t jj)
{
  return _hiWidthC->getVal(ii, jj);
}
double extVariation::getTopWidthR(uint32_t ii, uint32_t jj)
{
  return _hiWidthR->getVal(ii, jj);
}
Array1D<double>* extVariation::getWidthTable()
{
  return _hiWidthC->_width;
}
Array1D<double>* extVariation::getSpaceTable()
{
  return _hiWidthC->_space;
}
Array1D<double>* extVariation::getDataRateTable()
{
  return _loWidthC->_density;
}
Array1D<double>* extVariation::getPTable()
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
void extProcess::writeProcess(const char* filename)
{
  FILE* fp = openFile(filename, "w");
  Parser parse(logger_);

  for (uint32_t kk = 1; kk < _varTable->getCnt(); kk++) {
    _varTable->get(kk)->printVariation(fp, kk);
  }

  for (uint32_t ii = 1; ii < _condTable->getCnt(); ii++) {
    extConductor* cond = _condTable->get(ii);
    cond->printConductor(fp, &parse);
  }
  for (uint32_t jj = 1; jj < _dielTable->getCnt(); jj++) {
    extDielectric* diel = _dielTable->get(jj);
    diel->printDielectric(fp, &parse);
  }
  fclose(fp);
}
Array1D<double>* extVarTable::readDoubleArray(Parser* parser,
                                              const char* keyword)
{
  if ((keyword != nullptr) && (strcmp(keyword, parser->get(0)) != 0)) {
    return nullptr;
  }

  if (parser->getWordCnt() < 1) {
    return nullptr;
  }

  Array1D<double>* A = new Array1D<double>(parser->getWordCnt());
  uint32_t start = 0;
  if (keyword != nullptr) {
    start = 1;
  }
  parser->getDoubleArray(A, start);
  return A;
}
void extVarTable::printOneLine(FILE* fp,
                               Array1D<double>* A,
                               const char* header,
                               const char* trail)
{
  if (A == nullptr) {
    return;
  }

  if (header != nullptr) {
    fprintf(fp, "%s ", header);
  }

  for (uint32_t ii = 0; ii < A->getCnt(); ii++) {
    fprintf(fp, "%g ", A->get(ii));
  }

  fprintf(fp, "%s", trail);
}
int extVarTable::readWidthSpacing2D(Parser* parser,
                                    const char* keyword1,
                                    const char* keyword2,
                                    const char* keyword3,
                                    const char* key4)
{
  uint32_t debug = 0;

  if (strcmp("}", parser->get(0)) == 0) {
    return -1;
  }
  _width = readDoubleArray(parser, keyword1);

  if (debug > 0) {
    printOneLine(stdout, _width, keyword1, "\n");
  }

  if (!(parser->parseNextLine() > 0)) {
    return -1;
  }

  if (strcmp("Spacing", keyword2) == 0) {
    _space = readDoubleArray(parser, keyword2);
  }
  if (strcmp("Deff", keyword2) == 0) {
    _density = readDoubleArray(parser, keyword2);
    for (uint32_t jj = 0; jj < _density->getCnt(); jj++) {
      _vTable[jj] = new Array1D<double>(_width->getCnt());
    }
    if (debug > 0) {
      printOneLine(stdout, _space, keyword2, "\n");
      printOneLine(stdout, _density, keyword2, "\n");
    }
  }
  if (strcmp("P", keyword2) == 0) {
    _p = readDoubleArray(parser, keyword2);
    _rowCnt = 1;
    if (debug > 0) {
      printOneLine(stdout, _p, keyword2, "\n");
    }
  }

  if (!(parser->parseNextLine() > 0)) {
    return -1;
  }
  if (strcmp("}", keyword3) == 0) {
    return 1;
  }
  if (strcmp(keyword3, parser->get(0)) != 0) {
    return -1;
  }

  uint32_t ii = 0;
  while (parser->parseNextLine() > 0) {
    if (strcmp(key4, parser->get(0)) == 0) {
      break;
    }
    if (strcmp("}", parser->get(0)) == 0) {
      break;
    }
    if (_space != nullptr) {
      _vTable[ii++] = readDoubleArray(parser, nullptr);
      if (debug > 0) {
        printOneLine(stdout, _vTable[ii - 1], nullptr, "\n");
      }
    } else {
      ii++;
      for (int jj = 0; jj < parser->getWordCnt(); jj++) {
        _vTable[jj]->add(parser->getDouble(jj));
      }
    }
  }
  if (debug > 0) {
    if (_density != nullptr) {
      for (uint32_t jj = 0; jj < _density->getCnt(); jj++) {
        printOneLine(stdout, _vTable[jj], nullptr, "\n");
      }
    }
  }

  if (_space != nullptr) {
    _rowCnt = ii;
  } else {
    _rowCnt = _density->getCnt();
  }

  return ii;
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

  for (uint32_t ii = 0; ii < _rowCnt; ii++) {
    printOneLine(fp, _vTable[ii], nullptr, "\n");
  }
}
void extVariation::printVariation(FILE* fp, uint32_t n)
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
extVarTable* extVariation::readVarTable(Parser* parser,
                                        const char* key1,
                                        const char* key2,
                                        const char* key3,
                                        const char* endKey)
{
  extVarTable* V = new extVarTable(20);  // TODO
  if (V->readWidthSpacing2D(parser, key1, key2, key3, endKey) < 1) {
    //		fprintf(stdout, "Cannot read VarTable section: <%s>", key3);
    logger_->warn(RCX, 155, "Can't read VarTable section: <{}>", key3);
    delete V;
    return nullptr;
  }

  return V;
}
int extVariation::readVariation(Parser* parser)
{
  _hiWidthC
      = readVarTable(parser, "Width", "Spacing", "hi_cWidth_eff", "Width");
  _loWidthC = readVarTable(parser, "Width", "Deff", "lo_cWidth_delta", "Width");
  _thicknessC
      = readVarTable(parser, "Width", "Deff", "c_thickness_eff", "Width");

  _hiWidthR
      = readVarTable(parser, "Width", "Spacing", "hi_rWidth_eff", "Width");
  _loWidthR = readVarTable(parser, "Width", "Deff", "lo_rWidth_delta", "Width");
  _thicknessR
      = readVarTable(parser, "Width", "Deff", "r_thickness_eff", "Width");
  if (strcmp("}", parser->get(0)) == 0) {
    return 0;
  }
  _p = readVarTable(parser, "Width", "P", "}", "");

  return 0;
}
uint32_t extProcess::readProcess(const char* name, char* filename)
{
  // uint32_t debug = 0;
  // create process object

  // read process numbers
  Parser parser(logger_);
  parser.addSeparator("\r");
  parser.openFile(filename);

  while (parser.parseNextLine() > 0) {
    if (strcmp("PROCESS", parser.get(0)) == 0) {
      while (parser.parseNextLine() > 0) {
        if (strcmp("}", parser.get(0)) == 0) {
          break;
        }
      }
    } else if (strcmp("THICKNESS_VARIATION", parser.get(0)) == 0) {
      readDataRateTable(&parser, "THICKNESS_VARIATION");
    } else if (strcmp("VAR_TABLE", parser.get(0)) == 0) {
      // extParser *xp= (extParser *) &parser;
      // xp->setDbg(debug);

      // parser.setDbg(debug);
      parser.getInt(1);

      extVariation* extVar = new extVariation();
      extVar->setLogger(logger_);

      while (parser.parseNextLine() > 0) {
        if (strcmp("}", parser.get(0)) == 0) {
          break;
        }
        extVar->readVariation(&parser);

        // extParser *xp= (extParser *) &parser;
        // xp->setDbg(0);

        // parser.setDbg(0);

        if (strcmp("}", parser.get(0)) == 0) {
          break;
        }
      }
      _varTable->add(extVar);
    } else if (strcmp("CONDUCTOR", parser.get(0)) == 0) {
      extConductor* cond = new extConductor(logger_);
      strcpy(cond->_name, parser.get(1));

      while (parser.parseNextLine() > 0) {
        if (strcmp("}", parser.get(0)) == 0) {
          break;
        }

        cond->readConductor(&parser);
      }
      _condTable->add(cond);
    } else if (strcmp("DIELECTRIC", parser.get(0)) == 0) {
      extDielectric* diel = new extDielectric(logger_);
      if (parser.getWordCnt() > 2) {
        strcpy(diel->_name, parser.get(1));
      }

      while (parser.parseNextLine() > 0) {
        if (strcmp("}", parser.get(0)) == 0) {
          break;
        }

        diel->readDielectric(&parser);
      }
      _dielTable->add(diel);
    } else if (strcmp("SETMAXMINFLAG", parser.get(0)) == 0) {
      _maxMinFlag = true;
    }
  }
  createMasterLayers();

  // create dielectric "planes"

  //	uint32_t layerCnt= 8;
  //	extRCModel *m= new extRCModel(layerCnt, (char *) name);
  //	_modelTable->add(m);

  return 0;
}
extProcess::extProcess(uint32_t condCnt, uint32_t dielCnt, utl::Logger* logger)
{
  logger_ = logger;
  _condTable = new Array1D<extConductor*>(condCnt);
  _condTable->add(nullptr);
  _maxMinFlag = false;
  _dielTable = new Array1D<extDielectric*>(dielCnt);
  _dielTable->add(nullptr);
  _masterConductorTable = new Array1D<extMasterConductor*>(condCnt);
  _masterConductorTable->add(nullptr);
  _masterDielectricTable = new Array1D<extMasterConductor*>(dielCnt);
  _masterDielectricTable->add(nullptr);

  _varTable = new Array1D<extVariation*>(condCnt);
  _varTable->add(nullptr);
  _dataRateTable = nullptr;
  _thickVarFlag = false;
}
extVarTable::extVarTable(uint32_t rowCnt)
{
  _rowCnt = rowCnt;
  _vTable = new Array1D<double>*[rowCnt];
  for (int i = 0; i < rowCnt; ++i) {
    _vTable[i] = nullptr;
  }
  _density = nullptr;
  _space = nullptr;
  _width = nullptr;
  _p = nullptr;
}
extVarTable::~extVarTable()
{
  for (uint32_t ii = 0; ii < _rowCnt; ii++) {
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
