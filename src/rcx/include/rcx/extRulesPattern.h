// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#pragma once

#include <cstdint>
#include <cstdio>

#include "odb/db.h"
#include "rcx/dbUtil.h"
#include "rcx/extRCap.h"

namespace utl {
class Logger;
}

namespace rcx {

class extRulesPat
{
 public:
  bool _dbg;

  uint32_t _met;
  int _underMet;
  int _overMet;

  bool _over;
  bool _under;
  bool _diag;
  bool _res;
  uint32_t _dir;
  uint32_t _short_dir;
  uint32_t _long_dir;
  uint32_t _option_len;
  uint32_t _len;

  uint32_t _lineCnt;

  uint32_t _sepGridCnt;

  char _name_prefix[20];
  char _name[500];
  uint32_t _minWidth;
  uint32_t _minSpace;
  uint32_t _pitch;

  uint32_t _spaceCnt;
  uint32_t _widthCnt;
  const uint32_t _sMult[10]
      = {1000, 1250, 1500, 2000, 2500, 3000, 4000, 5000, 8000, 10000};

  const int _dMult[14] = {-1000,
                          -500,
                          0,
                          500,
                          1000,
                          1250,
                          1500,
                          2000,
                          2500,
                          3000,
                          4000,
                          5000,
                          8000,
                          10000};
  uint32_t _diagSpaceCnt;
  int _target_diag_spacing[20];

  uint32_t _targetSpaceCount;
  uint32_t _targetWidthCount;

  uint32_t _target_width[20];
  uint32_t _target_spacing[20];

  uint32_t _under_minWidthCntx;
  uint32_t _over_minWidthCntx;
  uint32_t _under_minSpaceCntx;
  uint32_t _over_minSpaceCntx;

  int _init_origin[2];
  int _origin[2];

  uint32_t _patternSep;
  int _ll[2];
  int _ur[2];
  int _ll_last[2];
  int _ur_last[2];

  int _ll_1[2000][2];
  int _ur_1[2000][2];
  char _patName[2000][2000];

  FILE* _def_fp;
  odb::dbBlock* _block;
  odb::dbTech* _tech;
  extMain* _extMain;

  odb::dbTechLayer* _under_layer;
  odb::dbTechLayer* _over_layer;
  odb::dbTechLayer* _layer;
  odb::dbTechLayer* _diag_layer;
  dbCreateNetUtil* _create_net_util;

  int _dbunit;

 public:
  extRulesPat(const char* pat,
              bool over,
              bool under,
              bool diag,
              bool res,
              uint32_t len,
              const int org[2],
              const int LL[2],
              const int UR[2],
              odb::dbBlock* block,
              extMain* xt,
              odb::dbTech* tech);
  void PrintOrigin(FILE* fp, const int ll[2], uint32_t met, const char* msg);
  void UpdateOrigin_start(uint32_t met);
  void UpdateOrigin_wires(int ll[2], int ur[2]);
  int GetOrigin_end(int ur[2]);
  uint32_t setLayerInfo(odb::dbTechLayer* layer, uint32_t met);
  uint32_t getMinWidthSpacing(odb::dbTechLayer* layer, uint32_t& w);
  void setMets(int underMet,
               odb::dbTechLayer* under_layer,
               int overMet,
               odb::dbTechLayer* over_layer);
  void UpdateBBox();
  void Init(int s);
  void SetInitName1(uint32_t n);
  void SetInitName(uint32_t n,
                   uint32_t w1,
                   uint32_t w2,
                   uint32_t s1,
                   uint32_t s2,
                   int ds = 0);
  void AddName(uint32_t jj,
               uint32_t wireIndex,
               const char* wire = "",
               int met = -1);
  void AddName1(uint32_t jj,
                uint32_t w1,
                uint32_t w2,
                uint32_t s1,
                uint32_t s2,
                uint32_t wireIndex,
                const char* wire = "",
                int met = -1);
  uint32_t CreatePatterns();
  uint32_t CreatePatterns_res();
  uint32_t CreatePatterns_diag();
  uint32_t CreatePattern2s_diag(uint32_t widthIndex,
                                uint32_t spaceIndex1,
                                uint32_t spaceIndex2,
                                uint32_t wcnt,
                                uint32_t spaceDiagIndex,
                                uint32_t spaceDiagIndex2,
                                uint32_t dcnt);
  uint32_t CreatePattern1();
  uint32_t CreatePattern2(uint32_t wcnt);
  uint32_t CreatePattern(uint32_t widthIndex,
                         uint32_t spaceIndex,
                         uint32_t wcnt);
  uint32_t CreatePattern2s(uint32_t widthIndex,
                           uint32_t spaceIndex1,
                           uint32_t spaceIndex2,
                           uint32_t wcnt);

  uint32_t CreateContext(uint32_t met,
                         int ll[2],
                         int ur[2],
                         uint32_t w,
                         uint32_t s,
                         uint32_t cntxWidth,
                         uint32_t cntxSpace,
                         odb::dbTechLayer* layer);

  void Print(FILE* fp, uint32_t jj);
  void Print(FILE* fp);
  void PrintBbox(FILE* fp, const int LL[2], const int UR[2]);
  void WriteDB(uint32_t dir, uint32_t met, odb::dbTechLayer* layer);
  void WriteDB(uint32_t jj,
               uint32_t dir,
               uint32_t met,
               odb::dbTechLayer* layer,
               FILE* fp);
  void WriteWire(FILE* fp, const int ll[2], const int ur[2], char* name);

  odb::dbBTerm* createBterm1(bool lo,
                             odb::dbNet* net,
                             const int ll[2],
                             const int ur[2],
                             const char* postFix,
                             odb::dbTechLayer* layer,
                             uint32_t width,
                             bool horizontal,
                             bool io);
  odb::dbNet* createNetSingleWire(const char* netName,
                                  int ll[2],
                                  int ur[2],
                                  uint32_t width,
                                  bool vertical,
                                  uint32_t met,
                                  odb::dbTechLayer* layer);
  // dkf 12/19/2023
  uint32_t setLayerInfoVia(odb::dbTechLayer* layer,
                           uint32_t met,
                           bool start = false);
  uint32_t CreatePatternVia(odb::dbTechVia* via,
                            uint32_t widthIndex,
                            uint32_t spaceIndex,
                            uint32_t wcnt);
  odb::dbNet* createNetSingleWireAndVia(const char* netName,
                                        int ll[2],
                                        int ur[2],
                                        uint32_t width,
                                        bool vertical,
                                        odb::dbTechVia* via);
  void WriteDBWireVia(uint32_t jj, uint32_t dir, odb::dbTechVia* via);
  // dkf 12/20/2023
  odb::dbBTerm* createBterm(bool lo,
                            odb::dbNet* net,
                            int ll[2],
                            int ur[2],
                            const char* postFix,
                            odb::dbTechLayer* layer,
                            uint32_t width,
                            bool horizontal,
                            bool io);
  // dkf 12/26/2023
  uint32_t GetViaCutCount(odb::dbTechVia* tvia);
  double GetViaArea(odb::dbTechVia* tvia);
};
}  // namespace rcx
