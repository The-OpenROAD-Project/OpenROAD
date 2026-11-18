// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cstdint>
#include <cstdio>

#include "rcx/array1.h"
#include "utl/Logger.h"

namespace rcx {

class extProcess;
class Parser;

class extConductor
{
  extConductor(utl::Logger* logger);

  void printConductor(FILE* fp, Parser* parse);
  bool readConductor(Parser* parser);
  bool setDoubleVal(Parser* parser, const char* key, int n, double& val);
  bool setIntVal(Parser* parser, const char* key, int n, int& val);

  void printString(FILE* fp,
                   const char* sep,
                   const char* key,
                   char* v,
                   bool pos = false);
  void printInt(FILE* fp,
                const char* sep,
                const char* key,
                int v,
                bool pos = false);
  void printDouble(FILE* fp,
                   const char* sep,
                   const char* key,
                   double v,
                   bool pos = false);

  char _name[128];
  double _height;
  double _distance;
  double _thickness;
  double _min_width;
  double _min_spacing;
  double _origin_x;
  double _bottom_left_x;
  double _bottom_right_x;
  double _top_left_x;
  double _top_right_x;
  double _top_ext;
  double _bot_ext;

  double _p;
  int _var_table_index;

  double _min_cw_del;
  double _max_cw_del;
  double _min_ct_del;
  double _max_ct_del;
  double _min_ca;
  double _max_ca;
  utl::Logger* logger_;

  friend class extSolverGen;
  friend class extRCModel;
  friend class extProcess;
  friend class extMasterConductor;
};

class extDielectric
{
  extDielectric(utl::Logger* logger);
  bool readDielectric(Parser* parser);
  void printDielectric(FILE* fp, Parser* parse);
  void printDielectric(FILE* fp, float planeWidth, float planeThickness);
  void printDielectric3D(FILE* fp,
                         float blockWidth,
                         float blockThickness,
                         float blockLength);

  void printString(FILE* fp,
                   const char* sep,
                   const char* key,
                   char* v,
                   bool pos = false);
  void printInt(FILE* fp,
                const char* sep,
                const char* key,
                int v,
                bool pos = false);
  void printDouble(FILE* fp,
                   const char* sep,
                   const char* key,
                   double v,
                   bool pos = false);

  bool setDoubleVal(Parser* parser, const char* key, int n, double& val);
  bool setIntVal(Parser* parser, const char* key, int n, int& val);
  char _name[128];
  char _non_conformal_metal[128];
  bool _conformal;
  double _epsilon;
  double _height;
  double _distance;
  double _thickness;
  double _left_thickness;
  double _right_thickness;
  double _top_thickness;
  double _bottom_thickness;
  double _bottom_ext;
  double _slope;
  double _trench;

  int _met;
  int _nextMet;

  utl::Logger* logger_;

  friend class extProcess;
  friend class extMasterConductor;
};

class extMasterConductor
{
 public:
  void writeWire3D(FILE* fp,
                   uint32_t wireNum,
                   double X,
                   double width,
                   double length,
                   double height_offset,
                   double volt);
  void writePointXY(FILE* fp,
                    const char* suffix,
                    double X,
                    double Y,
                    const char* postfix = "");
  void writeWireName(FILE* fp, uint32_t wireNum);
  void writeDiel(FILE* fp,
                 const char* name,
                 double epsilon,
                 double height_offset);
  void printDielHeights(FILE* fp, extDielectric* diel);
  double writeGround(FILE* fp, int met, int wire_num, double th);

  void reset(double height,
             double top_width,
             double bottom_width,
             double thickness);
  void resetThicknessHeight(double height, double thickness);
  void resetWidth(double top_width, double bottom_width);

  double writeRaphaelBox(FILE* fp,
                         uint32_t wireNum,
                         double width,
                         double X,
                         double volt);
  void writeRaphaelPoly(FILE* fp, uint32_t wireNum, double X, double volt);
  void writeRaphaelPoly3D_w(FILE* fp,
                            uint32_t wireNum,
                            double X,
                            double width,
                            double length,
                            double volt);
  void writeRaphaelPoly3D(FILE* fp,
                          uint32_t wireNum,
                          double X,
                          double length,
                          double volt);
  void writeRaphaelConformalPoly(FILE* fp,
                                 double width,
                                 double X,
                                 extProcess* p);
  void writeRaphaelConformalGround(FILE* fp,
                                   double X,
                                   double width,
                                   extProcess* p);
  double writeRaphaelPoly(FILE* fp,
                          uint32_t wireNum,
                          double width,
                          double X,
                          double volt,
                          extProcess* p = nullptr);
  double writeRaphaelPoly3D(FILE* fp,
                            uint32_t wireNum,
                            double width,
                            double length,
                            double X,
                            double volt);
  void printDielBox(FILE* fp,
                    double X,
                    double width,
                    extDielectric* diel,
                    const char* width_name);
  void printDielBox3D(FILE* fp,
                      double X,
                      double width,
                      double length,
                      extDielectric* diel,
                      const char* width_name);
  void writeRaphaelPointXY(FILE* fp, double X, double Y);
  void writeRaphaelDielPoly(FILE* fp,
                            double X,
                            double width,
                            extDielectric* diel);
  void writeRaphaelDielPoly3D(FILE* fp,
                              double X,
                              double width,
                              double length,
                              extDielectric* diel);
  void writeBoxName(FILE* fp, uint32_t wireNum);

 private:
  extMasterConductor(uint32_t condId,
                     extConductor* cond,
                     double prevHeight,
                     utl::Logger* logger);
  extMasterConductor(uint32_t dielId,
                     extDielectric* diel,
                     double xlo,
                     double dx1,
                     double xhi,
                     double dx2,
                     double h,
                     double th,
                     utl::Logger* logger);

  uint32_t _conformalId[3];
  utl::Logger* logger_;
  uint32_t _condId;
  double _loLeft[3];
  double _loRight[3];
  double _hiLeft[3];
  double _hiRight[3];
  double _dy;
  double _e;

  friend class extProcess;
};

class extVarTable
{
 public:
  extVarTable(uint32_t rowCnt);
  ~extVarTable();

  int readWidthSpacing2D(Parser* parser,
                         const char* keyword1,
                         const char* keyword2,
                         const char* keyword3,
                         const char* key);
  Array1D<double>* readDoubleArray(Parser* parser, const char* keyword);
  void printOneLine(FILE* fp,
                    Array1D<double>* A,
                    const char* header,
                    const char* trail);
  void printTable(FILE* fp, const char* valKey);
  double getVal(uint32_t ii, uint32_t jj) { return _vTable[ii]->get(jj); };

 private:
  Array1D<double>* _width;
  Array1D<double>* _space;
  Array1D<double>* _density;
  Array1D<double>* _p;
  uint32_t _rowCnt;
  Array1D<double>** _vTable;

  friend class extVariation;
};

class extVariation
{
 public:
  int readVariation(Parser* parser);
  extVarTable* readVarTable(Parser* parser,
                            const char* key1,
                            const char* key2,
                            const char* key3,
                            const char* endKey);
  void printVariation(FILE* fp, uint32_t n);
  Array1D<double>* getWidthTable();
  Array1D<double>* getSpaceTable();
  Array1D<double>* getDataRateTable();
  Array1D<double>* getPTable();
  double getTopWidth(uint32_t ii, uint32_t jj);
  double getTopWidthR(uint32_t ii, uint32_t jj);
  double getBottomWidth(double w, uint32_t dIndex);
  double getBottomWidthR(double w, uint32_t dIndex);
  double getThickness(double w, uint32_t dIndex);
  double getThicknessR(double w, uint32_t dIndex);
  double getP(double w);
  double interpolate(double w, Array1D<double>* X, Array1D<double>* Y);
  void setLogger(utl::Logger* logger) { logger_ = logger; }

 private:
  extVarTable* _hiWidthC;
  extVarTable* _loWidthC;
  extVarTable* _thicknessC;
  extVarTable* _hiWidthR;
  extVarTable* _loWidthR;
  extVarTable* _thicknessR;
  extVarTable* _p;

  utl::Logger* logger_;
};

class extProcess
{
  friend class extSolverGen;

 public:
  double writeProcessHeights(FILE* fp,
                             double X,
                             double width,
                             double length,
                             char* width_name,
                             double height_base,
                             double height_ceiling);
  double writeProcessAndGroundPlanes(FILE* wfp,
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
                                     bool diag = false);

  extProcess(uint32_t condCnt, uint32_t dielCnt, utl::Logger* logger);

  FILE* openFile(const char* filename, const char* permissions);
  uint32_t readProcess(const char* name, char* filename);
  void writeProcess(const char* filename);
  void createMasterLayers();
  void writeProcess(FILE* fp,
                    char* gndName,
                    float planeWidth,
                    float planeThickness);
  void writeProcess3D(FILE* fp,
                      char* gndName,
                      float blockWidth,
                      float blockThickness,
                      float blockLength);
  extConductor* getConductor(uint32_t ii);
  extMasterConductor* getMasterConductor(uint32_t ii);
  uint32_t getConductorCnt() { return _condTable->getCnt(); };
  extDielectric* getDielectric(uint32_t ii);
  extMasterConductor* getMasterConductor(uint32_t met,
                                         uint32_t wIndex,
                                         uint32_t sIndex,
                                         double& w,
                                         double& s);

  void writeFullProcess(FILE* fp,
                        char* gndName,
                        double planeWidth,
                        double planeThickness);
  void writeFullProcess(FILE* fp, double X, double width, char* width_name);
  void writeFullProcess3D(FILE* fp,
                          double X,
                          double width,
                          double length,
                          char* width_name);
  void writeRaphaelPointXY(FILE* fp, double X, double Y);
  void writeParam(FILE* fp, const char* name, double val);
  void writeWindow(FILE* fp,
                   const char* param_width_name,
                   double y1,
                   const char* param_thickness_name);
  void writeWindow3D(FILE* fp,
                     const char* param_width_name,
                     double y1,
                     const char* param_thickness_name,
                     const char* param_length_name);

  void writeGround(FILE* fp,
                   int met,
                   const char* name,
                   double width,
                   double x1,
                   double volt,
                   bool diag = false);

  void writeGround(FILE* fp,
                   int met,
                   const char* name,
                   const char* param_width_name,
                   double x1,
                   double volt);
  void writeGround3D(FILE* fp,
                     int met,
                     const char* name,
                     double width,
                     double length,
                     double x1,
                     double volt,
                     bool diag = false);
  void writeProcessAndGround(FILE* wfp,
                             const char* gndName,
                             int underMet,
                             int overMet,
                             double X,
                             double width,
                             double thichness,
                             bool diag = false);
  void writeProcessAndGround3D(FILE* wfp,
                               const char* gndName,
                               int underMet,
                               int overMet,
                               double X,
                               double width,
                               double length,
                               double thickness,
                               double W,
                               bool diag = false);

  extVariation* getVariation(uint32_t met);
  Array1D<double>* getWidthTable(uint32_t met);
  Array1D<double>* getSpaceTable(uint32_t met);
  Array1D<double>* getDiagSpaceTable(uint32_t met);
  Array1D<double>* getDataRateTable(uint32_t met);
  void readDataRateTable(Parser* parser, const char* keyword);
  double adjustMasterLayersForHeight(uint32_t met, double thickness);
  double adjustMasterDielectricsForHeight(uint32_t met, double dth);
  bool getMaxMinFlag();
  bool getThickVarFlag();

 private:
  utl::Logger* logger_;

  uint32_t _condctorCnt;
  uint32_t _dielectricCnt;
  bool _maxMinFlag;
  bool _thickVarFlag;
  Array1D<extConductor*>* _condTable;
  Array1D<extDielectric*>* _dielTable;
  Array1D<extMasterConductor*>* _masterConductorTable;
  Array1D<extMasterConductor*>* _masterDielectricTable;
  Array1D<extVariation*>* _varTable;
  Array1D<double>* _dataRateTable;
};

}  // namespace rcx
