// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#pragma once

#include <cstdio>

#include "odb/db.h"
#include "rcx/dbUtil.h"
#include "rcx/extRCap.h"

namespace utl {
class Logger;
}

namespace rcx {

using utl::Logger;

class extRulesPat
{
 public:
  bool _dbg;

  uint _met;
  int _underMet;
  int _overMet;

  bool _over;
  bool _under;
  bool _diag;
  bool _res;
  uint _dir;
  uint _short_dir;
  uint _long_dir;
  uint _option_len;
  uint _len;

  uint _lineCnt;

  uint _sepGridCnt;

  char _name_prefix[20];
  char _name[500];
  uint _minWidth;
  uint _minSpace;
  uint _pitch;

  uint _spaceCnt;
  uint _widthCnt;
  uint _sMult[20]
      = {1000, 1250, 1500, 2000, 2500, 3000, 4000, 5000, 8000, 10000};
  uint _wMult[20] = {1};

  int _dMult[20] = {-1000,
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
  uint _diagSpaceCnt;
  int _target_diag_spacing[20];

  uint _targetSpaceCount;
  uint _targetWidthCount;

  uint _target_width[20];
  uint _target_spacing[20];

  uint _under_minWidthCntx;
  uint _over_minWidthCntx;
  uint _under_minSpaceCntx;
  uint _over_minSpaceCntx;

  int _init_origin[2];
  int _origin[2];

  uint _patternSep;
  int _ll[2];
  int _ur[2];
  int _ll_last[2];
  int _ur_last[2];

  int _LL[2000][2];
  int _UR[2000][2];
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
              uint len,
              int org[2],
              int LL[2],
              int UR[2],
              odb::dbBlock* block,
              extMain* xt,
              dbTech* tech);
  void PrintOrigin(FILE* fp, int ll[2], uint met, const char* msg);
  void UpdateOrigin_start(uint met);
  void UpdateOrigin_wires(int ll[2], int ur[2]);
  int GetOrigin_end(int ur[2]);
  uint setLayerInfo(odb::dbTechLayer* layer, uint met);
  uint getMinWidthSpacing(odb::dbTechLayer* layer, uint& w);
  void setMets(int underMet,
               odb::dbTechLayer* under_layer,
               int overMet,
               odb::dbTechLayer* over_layer);
  void UpdateBBox();
  void Init(int s);
  void SetInitName1(uint n);
  void SetInitName(uint n, uint w1, uint w2, uint s1, uint s2, int ds = 0);
  void AddName(uint jj, uint wireIndex, const char* wire = "", int met = -1);
  void AddName1(uint jj,
                uint w1,
                uint w2,
                uint s1,
                uint s2,
                uint wireIndex,
                const char* wire = "",
                int met = -1);
  uint CreatePatterns();
  uint CreatePatterns_res();
  uint CreatePatterns_diag();
  uint CreatePattern2s_diag(uint widthIndex,
                            uint spaceIndex1,
                            uint spaceIndex2,
                            uint wcnt,
                            uint spaceDiagIndex,
                            uint spaceDiagIndex2,
                            uint dcnt);
  uint CreatePattern1();
  uint CreatePattern2(uint wcnt);
  uint CreatePattern(uint widthIndex, uint spaceIndex, uint wcnt);
  uint CreatePattern2s(uint widthIndex,
                       uint spaceIndex1,
                       uint spaceIndex2,
                       uint wcnt);

  uint CreateContext(uint met,
                     int ll[2],
                     int ur[2],
                     uint w,
                     uint s,
                     uint cntxWidth,
                     uint cntxSpace,
                     odb::dbTechLayer* layer);

  void Print(FILE* fp, uint jj);
  void Print(FILE* fp);
  void PrintBbox(FILE* fp, int LL[2], int UR[2]);
  void WriteDB(uint dir, uint met, odb::dbTechLayer* layer);
  void WriteDB(uint jj, uint dir, uint met, odb::dbTechLayer* layer, FILE* fp);
  void WriteWire(FILE* fp, int ll[2], int ur[2], char* name);

  dbBTerm* createBterm1(bool lo,
                        odb::dbNet* net,
                        int ll[2],
                        int ur[2],
                        const char* postFix,
                        odb::dbTechLayer* layer,
                        uint width,
                        bool horizontal,
                        bool io);
  odb::dbNet* createNetSingleWire(const char* netName,
                                  int ll[2],
                                  int ur[2],
                                  uint width,
                                  bool vertical,
                                  uint met,
                                  odb::dbTechLayer* layer);
  // dkf 12/19/2023
  uint setLayerInfoVia(odb::dbTechLayer* layer, uint met, bool start = false);
  uint CreatePatternVia(odb::dbTechVia* via,
                        uint widthIndex,
                        uint spaceIndex,
                        uint wcnt);
  odb::dbNet* createNetSingleWireAndVia(const char* netName,
                                        int ll[2],
                                        int ur[2],
                                        uint width,
                                        bool vertical,
                                        odb::dbTechVia* via);
  void WriteDBWireVia(uint jj, uint dir, odb::dbTechVia* via);
  // dkf 12/20/2023
  dbBTerm* createBterm(bool lo,
                       odb::dbNet* net,
                       int ll[2],
                       int ur[2],
                       const char* postFix,
                       odb::dbTechLayer* layer,
                       uint width,
                       bool horizontal,
                       bool io);
  // dkf 12/26/2023
  uint GetViaCutCount(odb::dbTechVia* tvia);
  double GetViaArea(odb::dbTechVia* tvia);
};
}  // namespace rcx
