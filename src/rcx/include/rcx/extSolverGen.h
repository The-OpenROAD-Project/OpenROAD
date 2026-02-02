// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#pragma once

#include <cstdint>
#include <cstdio>
#include <vector>

#include "rcx/extprocess.h"
#include "utl/Logger.h"

namespace rcx {

class extProcess;

class extSolverGen : public extProcess
{
 public:
  void init();
  extSolverGen(uint32_t layerCnt, uint32_t dielCnt, utl::Logger* logger)
      : extProcess(layerCnt, dielCnt, logger)
  {
    init();
  }
  bool setWidthSpaceMultipliers(const char* w_list, const char* s_list);
  static bool getMultipliers(const char* input, std::vector<double>& table);
  double calcResistance(double ro,
                        double w,
                        double s,
                        double len,
                        double top_widthR,
                        double bot_widthR,
                        double thicknessR);
  uint32_t linesOver(uint32_t metLevel = 0);
  uint32_t linesUnder(uint32_t metLevel = 0);
  uint32_t linesDiagUnder(uint32_t metLevel = 0);
  uint32_t linesOverUnder(uint32_t metLevel = 0);
  uint32_t widthsSpacingsLoop(uint32_t diagMet = 0);
  void setTargetParams(double w,
                       double s,
                       double r,
                       double t,
                       double h,
                       double w2 = 0.0,
                       double s2 = 0.0);
  void setMets(int m, int u, int o);
  double writeWirePatterns(FILE* fp,
                           double height_offset,
                           double& len,
                           double& max_x);
  double writeWirePatterns_w3(FILE* fp,
                              double height_offset,
                              double& len,
                              double& max_x);
  FILE* mkPatternFile();
  FILE* openFile(const char* topDir,
                 const char* name,
                 const char* suffix,
                 const char* permissions);
  void printCommentLine(char commentChar);
  void initLogFiles(const char* process_name);
  void closeFiles();
  /*
  extModelGen(uint32_t layerCnt, const char *name, utl::Logger *logger) :
  extRCModel(layerCnt, name, logger) {} uint32_t ReadRCDB(dbBlock *block,
  uint32_t widthCnt, uint32_t diagOption, char *logFile); void writeRules(FILE
  *fp, bool binary, uint32_t m, int corner=-1); FILE *InitWriteRules(const char
  *name, std::list<std::string> corner_list, const char *comment, bool binary,
  uint32_t modelCnt); static std::list<std::string> GetCornerNames(const char
  *filename, double &version) ;

  // ----------------------------------- DKF 09212024
  --------------------------------------- uint32_t GenExtModel(const char
  *out_file, const char *comment, const char *version, int pattern);
  //
  --------------------------------------------------------------------------------------------
*/

  // DKF 9/25/2024 -- 3d pattern generation

  void setDiagModel(uint32_t i) { _diagModel = i; }

  std::vector<double> _widthMultTable;
  std::vector<double> _spaceMultTable;

  std::vector<double> _widthTable;
  std::vector<double> _spaceTable;
  std::vector<double> _diagSpaceTable;
  std::vector<double> _diagWidthTable;

  bool _winDirFlat;
  int _len;
  int _simVersion;
  int _maxLevelDist;
  FILE* _filesFP;
  int _maxUnderDist;
  int _maxOverDist;
  bool _3dFlag;
  bool _diag;
  bool _metExtFlag;

  double _topWidth;
  double _botWidth;
  double _teff;
  double _heff;
  double _seff;
  void setEffParams(double wTop, double wBot, double teff);

  double _w_m;
  int _w_nm;
  double _s_m;
  int _s_nm;
  double _w2_m;
  int _w2_nm;
  double _s2_m;
  int _s2_nm;

  double _r;
  double _t;
  double _h;

  uint32_t _layerCnt;
  int _met;
  int _underMet;
  int _overMet;
  uint32_t _wireCnt;
  bool _over;
  bool _overUnder;
  int _diagModel;

  char* _wireDirName;
  char* _wireFileName;

  char* _topDir;
  char* _patternName;
  Parser* _parser;
  char* _solverFileName;

  FILE* _capLogFP;
  FILE* _logFP;
  FILE* _resFP;

  FILE* _dbg_logFP;

  bool _writeFiles;

  bool _readCapLog;
  char _commentLine[10000];
  bool _commentFlag;

  bool measurePatternVar_3D(int met,
                            double top_width,
                            double bot_width,
                            double thickness,
                            uint32_t wireCnt,
                            char* wiresNameSuffix);

  void mkFileNames(char* wiresNameSuffix);
  uint32_t genSolverPatterns(const char* process_name,
                             int version,
                             int wire_cnt,
                             int len,
                             int over_dist,
                             int under_dist,
                             const char* w_list,
                             const char* s_list);
  static int getLastCharInt(const char* name);
};

}  // namespace rcx
