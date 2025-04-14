// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#pragma once

#include <functional>
#include <memory>
#include <vector>

#include "dbUtil.h"
#include "ext_options.h"
#include "odb/db.h"
#include "util.h"

namespace rcx {

using namespace odb;

template <class T>
class AthArray;

class extWirePattern;
class extPattern
{
 public:
  char patternName[1000];
  char targetMetName[1000];
  char contextName[1000];
  char lastName[1000];
  char currentName[10000];
  int met;
  int over_met;
  int over_met2;
  int under_met;
  int under_met2;
  int wireCnt;

  float minWidth;
  int _minWidth;
  float minSpacing;
  int _minSpacing;
  uint dir;

  uint _cnt;
  uint _totEnumerationCnt;
  dbCreateNetUtil* db_net_util;

  std::vector<float> mWidth;
  std::vector<float> mSpacing;
  std::vector<float> lWidth;
  std::vector<float> rWidth;
  std::vector<float> csWidth;
  std::vector<float> csSpacing;

  std::vector<float> overSpacing;
  std::vector<float> over2Spacing;
  std::vector<float> underSpacing;
  std::vector<float> under2Spacing;

  std::vector<float> overWidth;
  std::vector<float> over2Width;
  std::vector<float> underWidth;
  std::vector<float> under2Width;
  std::vector<float> underOffset;
  std::vector<float> overOffset;

  PatternOptions opt;
  FILE* patternLog;
  float origin[2];
  int _origin[2];

  int max_ur[2];
  float pattern_separation;
  int _pattern_separation;
  int units;

  AthHash<int>* nameHash;

  extPattern(int wireCnt,
             int over1,
             int over,
             int met,
             int under,
             int under1,
             const PatternOptions& opt,
             FILE* fp,
             int org[2],
             dbCreateNetUtil* net_util);
  bool SetPatternName();
  std::vector<float> getMultipliers(const char* s);
  float init(dbCreateNetUtil* net_util);
  uint CreatePattern(int origin[2], int MAX_UR[2], dbCreateNetUtil* net_util);
  // uint CreatePattern(dbCreateNetUtil *net_util);

  uint CreatePattern_over(int org[2], int MAX_UR[2]);
  uint CreatePattern_under(int origin[2], int MAX_UR[2]);
  uint CreatePattern_save(float origin[2],
                          float MAX_UR[2],
                          dbCreateNetUtil* net_util);
  extWirePattern* GetWireParttern(extPattern* main,
                                  uint dir,
                                  float mw,
                                  float ms,
                                  int met1,
                                  int& w,
                                  int& s);
  bool createContextName_under(float uw,
                               float us,
                               float offsetUnder,
                               float uw2,
                               float us2);
  bool createContextName_over(float ow,
                              float os,
                              float offsetOver,
                              float ow2,
                              float os2);
  uint contextPatterns_under(extWirePattern* mainp,
                             float uw,
                             float us,
                             float offsetUnder,
                             float uw2,
                             float us2);
  uint contextPatterns_over(extWirePattern* mainp,
                            float ow,
                            float os,
                            float offsetOver,
                            float ow2,
                            float os2);
  extWirePattern* MainPattern(float mw,
                              float msL,
                              float msR,
                              float mwL,
                              float mwR,
                              float cwl,
                              float csl,
                              float cwr,
                              float csr);
  int ContextPattern(extWirePattern* main,
                     uint dir,
                     int met,
                     float mw,
                     float ms,
                     float offset);
  int ContextPatternParallel(extWirePattern* main,
                             uint dir,
                             int met,
                             float mw,
                             float ms,
                             float mid_offset);
  static FILE* OpenLog(int met, const char* postfix);

  void setName();
  int max_last(extWirePattern* wp);
  void set_max(int ur[2]);

  bool printWiresDEF(extWirePattern* wp, int level);
  int getRoundedInt(float v, int units1);
  static float GetRoundedInt(float v, float mult, int);
  static int GetRoundedInt(int v, float mult, int);
  uint createNetSingleWire(char* netName, int ll[2], int ur[2], int level);
  bool RenameBterm1stInput(dbNet* net);
  bool RenameAllBterms(dbNet* net);

  void PatternEnd(extWirePattern* mainp, int max_ur[2], uint spacingMult);
  static void PrintStats(uint cnt,
                         int origin[2],
                         int start[2],
                         const char* msg);
};
class extWirePattern
{
 public:
  extPattern* pattern;
  float minWidth;
  float minSpacing;
  float len;
  int _minWidth;
  int _minSpacing;
  int _len;
  int _centerWireIndex;
  uint dir;
  float trans_ll[2][1000];
  float trans_ur[2][1000];
  int _trans_ll[2][1000];
  int _trans_ur[2][1000];
  // float ll[2][100];
  // float ur[2][100];
  int ll[2][1000];
  int ur[2][1000];
  int center_ll[2];
  int center_ur[2];
  uint cnt;
  char name[1000][20];
  char tmp_pattern_name[20000];
  uint units;

  extWirePattern(extPattern* p,
                 uint dir,
                 float minWidth,
                 float minSpacing,
                 const PatternOptions& opt);
  uint addCoords(float mw,
                 float ms,
                 const char* buff,
                 int ii = -1,
                 int met = -1);
  uint addCoords(int LL[2], int UR[2], const char* buff, int jj, int met);
  void addCoords_temp(int jj, int LL[2], int UR[2]);
  void AddOrigin(float org[2]);
  void AddOrigin_int(int org[2]);
  void print(FILE* fp, uint jj, float units = 0.001);
  void print_trans2(FILE* fp, uint jj, float units = 0.001);
  void print_trans(FILE* fp, uint jj);
  void printWires(FILE* fp, bool trans = false);
  void setCenterXY(int jj);
  int length(uint dir);
  int last(uint dir);
  int last_trans(uint dir);
  int first(uint dir);
  void gen_trans_name(uint jj);
};
}  // namespace rcx
