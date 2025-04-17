// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#include "rcx/extRCap.h"
#include "rcx/extRulesPattern.h"
#include "rcx/extSpef.h"
#include "rcx/extprocess.h"

#ifdef _WIN32
#include "direct.h"
#endif

#include <algorithm>
#include <map>
#include <vector>

#include "utl/Logger.h"

namespace rcx {

using namespace odb;
using utl::RCX;

extRulesPat::extRulesPat(const char* pat,
                         bool over,
                         bool under,
                         bool diag,
                         bool res,
                         uint len,
                         int org[2],
                         int LL[2],
                         int UR[2],
                         dbBlock* block,
                         extMain* xt,
                         dbTech* tech)
{
  _dbg = false;
  _over = over;
  _under = under;
  _diag = diag;
  _res = res;
  _ur_last[0] = UR[0];
  _ur_last[1] = UR[1];
  _ll_last[0] = LL[0];
  _ll_last[1] = LL[1];
  _option_len = len;

  // _sepGridCnt = 20;
  _sepGridCnt = 15;
  _spaceCnt = 10;
  _widthCnt = 1;

  _diagSpaceCnt = 5;
  _init_origin[0] = org[0];
  _init_origin[1] = org[1];

  _block = block;
  _tech = tech;
  _extMain = xt;
  _dbunit = _block->getDbUnitsPerMicron();

  if (_res)
    strcpy(_name_prefix, "R");
  else if (_over) {
    if (_under)
      strcpy(_name_prefix, "OU");
    else if (_diag)
      strcpy(_name_prefix, "DO");
    else
      strcpy(_name_prefix, "O");
  } else if (_under) {
    if (_diag)
      strcpy(_name_prefix, "DU");
    else
      strcpy(_name_prefix, "U");
  }
  PrintOrigin(stdout, _init_origin, 0, "Pattern Initial");
  _def_fp = stdout;

  _create_net_util = new dbCreateNetUtil(xt->getLogger());

  // _create_net_util.setBlock(_block, false);
}
void extRulesPat::PrintOrigin(FILE* fp, int ll[2], uint met, const char* msg)
{
  if (!_dbg)
    return;

  float units = 0.001;
  fprintf(stdout,
          "Origin %s M%d %10.2f %10.2f  %s\n",
          _name_prefix,
          met,
          ll[0] * units,
          ll[1] * units,
          msg);
}
void extRulesPat::UpdateOrigin_start(uint met)
{
  _patternSep = _sepGridCnt * (_minWidth + _minSpace);
  if (met > 1)
    _origin[0] += _ur_last[0] + _patternSep;  // Grow horizontally for each met
  else
    _origin[0] = _init_origin[0];

  _origin[1] = _init_origin[1];

  PrintOrigin(stdout, _origin, met, "Pattern Start");
}
void extRulesPat::UpdateOrigin_wires(int ll[2], int ur[2])
{
  // called at  the end of each individual wire pattern
  _origin[1] += ur[1] + _patternSep;
  if (_ur_last[0] < ur[0])
    _ur_last[0] = ur[0];  // next big pattern will start

  PrintBbox(stdout, ll, ur);
}
int extRulesPat::GetOrigin_end(int ur[2])
{
  // called at  the end of each individual wire pattern
  _origin[0] += _patternSep;  // already update at last wire Set

  PrintOrigin(stdout, _origin, _met, "Pattern End");
  return _origin[0];
}
void extRulesPat::SetInitName1(uint n)
{
  if (_diag) {
    sprintf(_name, "%s%d_M%duuM%d", _name_prefix, n, _met, _overMet);
  } else if (_res) {
    sprintf(_name, "%s%d_M%doM%d", _name_prefix, n, _met, _underMet);
  } else if (_over && _under) {
    sprintf(
        _name, "%s%d_M%doM%duM%d", _name_prefix, n, _met, _underMet, _overMet);
  } else if (_over) {
    sprintf(_name, "%s%d_M%doM%d", _name_prefix, n, _met, _underMet);
  } else {
    sprintf(_name, "%s%d_M%duM%d", _name_prefix, n, _met, _overMet);
  }
}
void extRulesPat::SetInitName(uint n,
                              uint w1,
                              uint w2,
                              uint s1,
                              uint s2,
                              int ds1)
{
  char name[100];
  if (_diag) {
    if (_under)
      sprintf(name, "%s%d_M%duuM%d", _name_prefix, n, _met, _overMet);
    else
      sprintf(name, "%s%d_M%dooM%d", _name_prefix, n, _met, _underMet);
  } else if (_res) {
    sprintf(name, "%s%d_M%doM%d", _name_prefix, n, _met, _underMet);
  } else if (_over && _under) {
    sprintf(
        name, "%s%d_M%doM%duM%d", _name_prefix, n, _met, _underMet, _overMet);
  } else if (_over) {
    sprintf(name, "%s%d_M%doM%d", _name_prefix, n, _met, _underMet);
  } else {
    sprintf(name, "%s%d_M%duM%d", _name_prefix, n, _met, _overMet);
  }
  if (!_diag)
    sprintf(_name, "%s_W%dW%d_S%05dS%05d", name, w1, w2, s1, s2);
  else if (_under)
    sprintf(_name,
            "%s_W%dW%d_S%05dS%05d_W%05d_S%05d",
            name,
            w1,
            w2,
            s1,
            s2,
            _over_minWidthCntx,
            ds1);
  else
    sprintf(_name,
            "%s_W%dW%d_S%05dS%05d_W%05d_S%05d",
            name,
            w1,
            w2,
            s1,
            s2,
            _under_minWidthCntx,
            ds1);
}
void extRulesPat::AddName(uint jj, uint wireIndex, const char* wire, int met)
{
  if (strcmp(wire, "") == 0)
    sprintf(_patName[jj], "%s_%s%d", _name, wire, wireIndex);
  else
    sprintf(_patName[jj], "%s_%s_M%d_%d", _name, wire, met, wireIndex);
}
void extRulesPat::AddName1(uint jj,
                           uint w1,
                           uint w2,
                           uint s1,
                           uint s2,
                           uint wireIndex,
                           const char* wire,
                           int met)
{
  if (strcmp(wire, "cntx") == 0)
    sprintf(_patName[jj],
            "%s_W%dW%d_S%05dS%05d_%s_%d_w%d",
            _name,
            w1,
            w2,
            s1,
            s2,
            wire,
            met,
            wireIndex);
  else
    sprintf(_patName[jj],
            "%s_W%dW%d_S%05dS%05d_%s%d",
            _name,
            w1,
            w2,
            s1,
            s2,
            wire,
            wireIndex);
}
uint extRulesPat::getMinWidthSpacing(dbTechLayer* layer, uint& w)
{
  uint minWidth = layer->getWidth();
  w = minWidth;
  uint p = layer->getPitch();
  int minSpace = layer->getSpacing();
  int s = p - minWidth;
  if (s > minSpace)
    s = minSpace;
  return s;
}
uint extRulesPat::setLayerInfo(dbTechLayer* layer, uint met)
{
  _layer = layer;
  _met = met;
  _minSpace = getMinWidthSpacing(layer, _minWidth);

  _len = _option_len * _minWidth / 1000;

  _dir = layer->getDirection() == dbTechLayerDir::HORIZONTAL ? 1 : 0;
  _short_dir = _dir;
  _long_dir = !_dir;

  UpdateOrigin_start(_met);

  _ll[0] = _ur_last[0] + _patternSep;
  _ll[1] = 0;

  _ur[_short_dir] = _ll[_short_dir];
  _ur[_long_dir]
      = _ll[_long_dir] + _len * _minWidth / 1000;  // _len is in nm per ext.ti

  for (uint ii = 0; ii < _spaceCnt; ii++) {
    _target_width[ii] = _minWidth * _sMult[ii] / 1000;
    _target_spacing[ii] = _minSpace * _sMult[ii] / 1000;
  }
  for (uint ii = 0; ii < _diagSpaceCnt; ii++) {
    _target_diag_spacing[ii] = (int) _minSpace * _dMult[ii] / 1000;
  }
  return _patternSep;
}
void extRulesPat::setMets(int underMet,
                          dbTechLayer* under_layer,
                          int overMet,
                          dbTechLayer* over_layer)
{
  _under_layer = under_layer;
  _underMet = underMet;
  if (under_layer != nullptr)
    _under_minSpaceCntx = getMinWidthSpacing(under_layer, _under_minWidthCntx);

  _over_layer = over_layer;
  _overMet = overMet;
  if (over_layer != nullptr)
    _over_minSpaceCntx = getMinWidthSpacing(over_layer, _over_minWidthCntx);
}
void extRulesPat::UpdateBBox()
{
  //  _ur_last[_long_dir] += _patternSep;
  // _origin[_long_dir] += _patternSep;
}
void extRulesPat::Init(int s)
{
  for (uint ii = 0; ii < 2; ii++) {
    _LL[0][ii] = 0;
    _UR[0][ii] = 0;
  }
  _short_dir = _dir;
  _long_dir = !_dir;

  _UR[0][_short_dir] = -s;
  _LL[0][_long_dir] = 0;
  _UR[0][_long_dir] = _len;

  _lineCnt = 0;
  PrintOrigin(stdout, _origin, _met, _name);
}
uint extRulesPat::CreatePatterns()
{
  uint cnt = 0;
  cnt += CreatePattern1();
  cnt += CreatePattern2(2);
  cnt += CreatePattern2(3);
  cnt += CreatePattern2(5);

  return cnt;
}
uint extRulesPat::CreatePatterns_res()
{
  uint cnt = 0;
  cnt += CreatePattern1();
  cnt += CreatePattern2(2);

  for (uint ii = 0; ii < _spaceCnt - 1; ii++) {
    for (uint jj = ii; jj < _spaceCnt; jj++) {
      cnt += CreatePattern2s(0, ii, jj, 3);
    }
  }
  for (uint ii = 0; ii < _spaceCnt - 1; ii++) {
    for (uint jj = ii; jj < _spaceCnt; jj++) {
      cnt += CreatePattern2s(0, ii, jj, 5);
    }
  }
  return cnt;
}
uint extRulesPat::CreatePatterns_diag()
{
  uint cnt = 0;

  for (uint ii = 0; ii < _diagSpaceCnt; ii++) {
    cnt += CreatePattern2s_diag(0, 0, 0, 1, ii, 0, 1);
  }

  uint maxSpaceCnt = 5;
  for (uint ii = 0; ii < _diagSpaceCnt; ii++) {
    for (uint jj = 0; jj < maxSpaceCnt; jj++) {
      cnt += CreatePattern2s_diag(0, jj, 0, 2, ii, 0, 1);
    }
  }
  for (uint ii = 0; ii < _diagSpaceCnt; ii++) {
    for (uint jj = 0; jj < maxSpaceCnt - 1; jj++) {
      for (uint kk = jj; kk < maxSpaceCnt; kk++) {
        cnt += CreatePattern2s_diag(0, jj, kk, 3, ii, 0, 1);
      }
    }
  }

  return cnt;
}
uint extRulesPat::CreatePattern(uint widthIndex, uint spaceIndex, uint wcnt)
{
  if (wcnt > 5)
    return 0;
  bool w5 = wcnt == 5;
  uint w = _target_width[widthIndex];
  uint s = _target_spacing[spaceIndex];
  if (wcnt == 1)
    s = 0;
  uint cntx_s = _target_spacing[0];
  uint cntx_w = _target_width[0];

  if (w5)
    Init(cntx_s);
  else
    Init(s);

  SetInitName(wcnt, w, w, s, s);
  // multiple widths
  uint ii = 1;
  for (uint jj = 0; jj < wcnt; jj++) {
    _LL[ii][_long_dir] = 0;
    _UR[ii][_long_dir] = _len;

    uint sp = s;
    uint ww = w;
    if (w5 && (jj <= 1 || jj == 4)) {
      sp = cntx_s;
      ww = cntx_w;
    }
    _LL[ii][_short_dir] = _UR[ii - 1][_short_dir] + sp;
    _UR[ii][_short_dir] = _LL[ii][_short_dir] + ww;
    // AddName(ii, w, w, s, s, ii);
    AddName(ii, ii);
    ii++;
  }
  int ll[2] = {_LL[1][0], _LL[1][1]};
  int ur[2] = {_UR[ii - 1][0], _UR[ii - 1][1]};
  _lineCnt = ii;

  WriteDB(_dir, _met, _layer);

  Print(stdout);

  if (_underMet > 0)
    CreateContext(_underMet,
                  ll,
                  ur,
                  w,
                  s,
                  _under_minWidthCntx,
                  _under_minSpaceCntx,
                  _under_layer);

  if (_overMet > 0)
    CreateContext(_overMet,
                  ll,
                  ur,
                  w,
                  s,
                  _over_minWidthCntx,
                  _over_minSpaceCntx,
                  _over_layer);

  UpdateOrigin_wires(ll, ur);

  return ii;  // cnt of main pattern;
}
uint extRulesPat::CreateContext(uint met,
                                int ll[2],
                                int ur[2],
                                uint w,
                                uint s,
                                uint cntxWidth,
                                uint cntxSpace,
                                dbTechLayer* cntx_layer)
{
  // s is NOT used
  int long_lo = ll[_short_dir] - w;  // reverse from main layer
  int long_hi = ur[_short_dir] + w;
  int start_xy = ll[_long_dir] + w;
  int limit_xy = ur[_long_dir] - w;

  int next_xy_low = start_xy;
  int next_xy_hi = next_xy_low + cntxSpace;
  uint jj = 1;
  while (next_xy_hi <= limit_xy) {
    _LL[jj][_long_dir] = next_xy_low;
    _UR[jj][_long_dir] = next_xy_hi;

    _LL[jj][_short_dir] = long_lo;
    _UR[jj][_short_dir] = long_hi;

    // AddName(jj, w, w, s, s, jj, "cntx", met);
    AddName(jj, jj, "cntx", met);
    jj++;

    next_xy_low = next_xy_hi + cntxSpace;
    next_xy_hi = next_xy_low + cntxWidth;
  }
  _lineCnt = jj;
  Print(stdout);
  for (uint k = 0; k < 2; k++) {
    if (ur[k] < _UR[jj - 1][k])
      ur[k] = _UR[jj - 1][k];

    if (ll[k] > _LL[jj - 1][k])
      ll[k] = _LL[jj - 1][k];
  }
  WriteDB(_long_dir, met, cntx_layer);

  return _lineCnt;
}
uint extRulesPat::CreatePattern2s(uint widthIndex,
                                  uint spaceIndex1,
                                  uint spaceIndex2,
                                  uint wcnt)
{
  if (wcnt > 5 || wcnt < 3)
    return 0;
  bool wcnt5 = wcnt == 5;
  uint w = _target_width[widthIndex];
  uint s1 = _target_spacing[spaceIndex1];
  uint s2 = _target_spacing[spaceIndex2];

  uint cntx_s = _target_spacing[0];
  uint cntx_w = _target_width[0];

  if (wcnt5)
    Init(cntx_s);
  else
    Init(s1);

  uint sp5[5] = {cntx_s, cntx_s, s1, s2, cntx_s};
  uint sp3[3] = {s1, s1, s2};

  uint w5[5] = {cntx_w, w, w, w, cntx_w};
  uint w3[3] = {w, w, w};

  // SetInitName(wcnt);
  SetInitName(wcnt, w, w, s1, s2);

  // multiple widths
  uint ii = 1;
  for (uint jj = 0; jj < wcnt; jj++) {
    _LL[ii][_long_dir] = 0;
    _UR[ii][_long_dir] = _len;

    uint sp = wcnt5 ? sp5[jj] : sp3[jj];
    uint ww = wcnt5 ? w5[jj] : w3[jj];

    _LL[ii][_short_dir] = _UR[ii - 1][_short_dir] + sp;
    _UR[ii][_short_dir] = _LL[ii][_short_dir] + ww;
    // AddName(ii, w, w, s1, s2, ii);
    AddName(ii, ii);
    ii++;
  }
  int ll[2] = {_LL[1][0], _LL[1][1]};
  int ur[2] = {_UR[ii - 1][0], _UR[ii - 1][1]};
  _lineCnt = ii;

  WriteDB(_dir, _met, _layer);

  Print(stdout);

  PrintBbox(stdout, ll, ur);
  UpdateOrigin_wires(ll, ur);

  if (_underMet > 0)
    CreateContext(_underMet,
                  ll,
                  ur,
                  w,
                  s1,
                  _under_minWidthCntx,
                  _under_minSpaceCntx,
                  _under_layer);

  if (_overMet > 0)
    CreateContext(_overMet,
                  ll,
                  ur,
                  w,
                  s1,
                  _over_minWidthCntx,
                  _over_minSpaceCntx,
                  _over_layer);

  return ii;  // cnt of main pattern;
}
uint extRulesPat::CreatePattern2s_diag(uint widthIndex,
                                       uint spaceIndex1,
                                       uint spaceIndex2,
                                       uint wcnt,
                                       uint spaceDiagIndex,
                                       uint spaceDiagIndex1,
                                       uint dcnt)
{
  if (wcnt > 3)
    return 0;
  bool wcnt5 = wcnt == 5;
  uint w = _target_width[widthIndex];
  uint s1 = _target_spacing[spaceIndex1];
  uint s2 = _target_spacing[spaceIndex2];

  int ds = _target_diag_spacing[spaceDiagIndex];

  uint cntx_s = _target_spacing[0];
  uint cntx_w = _target_width[0];

  if (wcnt5)
    Init(cntx_s);
  else
    Init(s1);

  uint sp5[5] = {cntx_s, cntx_s, s1, s2, cntx_s};
  uint sp3[3] = {s1, s1, s2};

  uint w5[5] = {cntx_w, w, w, w, cntx_w};
  uint w3[3] = {w, w, w};

  // SetInitName(wcnt);
  SetInitName(wcnt, w, w, s1, s2, ds);

  // multiple widths
  uint ii = 1;
  for (uint jj = 0; jj < wcnt; jj++) {
    _LL[ii][_long_dir] = 0;
    _UR[ii][_long_dir] = _len;

    uint sp = wcnt5 ? sp5[jj] : sp3[jj];
    uint ww = wcnt5 ? w5[jj] : w3[jj];

    _LL[ii][_short_dir] = _UR[ii - 1][_short_dir] + sp;
    _UR[ii][_short_dir] = _LL[ii][_short_dir] + ww;
    // AddName(ii, w, w, s1, s2, ii);
    AddName(ii, ii);
    ii++;
  }
  int ll[2] = {_LL[1][0], _LL[1][1]};
  int ur[2] = {_UR[ii - 1][0], _UR[ii - 1][1]};
  _lineCnt = ii;

  WriteDB(_dir, _met, _layer);

  Print(stdout);

  UpdateOrigin_wires(ll, ur);

  PrintBbox(stdout, ll, ur);

  int fisrt_hi = _UR[1][_short_dir];

  uint jj = 1;
  _LL[jj][_long_dir] = 0;
  _UR[jj][_long_dir] = _len;

  _LL[jj][_short_dir] = fisrt_hi + ds;
  if (_under) {
    _UR[jj][_short_dir] = _LL[jj][_short_dir] + _over_minWidthCntx;
    AddName(jj, jj, "diag", _overMet);
  } else {
    _UR[jj][_short_dir] = _LL[jj][_short_dir] + _under_minWidthCntx;
    AddName(jj, jj, "diag", _underMet);
  }
  jj++;

  _lineCnt = jj;

  if (_under)
    WriteDB(_dir, _overMet, _over_layer);
  else
    WriteDB(_dir, _underMet, _under_layer);

  Print(stdout);

  /* TODO
  uint uCnt=0;
  if (_underMet>0)
    uCnt= CreateContext(_underMet, ll, ur, w, s1, _under_minWidthCntx,
  _under_minSpaceCntx);

  uint oCnt=0;
  if (_overMet>0)
    oCnt= CreateContext(_overMet, ll, ur, w, s1, _over_minWidthCntx,
  _over_minSpaceCntx);
  */
  return ii;  // cnt of main pattern;
}

uint extRulesPat::CreatePattern1()
{
  uint cnt = 0;
  for (uint jj = 0; jj < _spaceCnt; jj++) {
    cnt += CreatePattern(jj, 0, 1);
  }
  return cnt;
}
uint extRulesPat::CreatePattern2(uint wct)
{
  uint cnt = 0;
  for (uint jj = 0; jj < _spaceCnt; jj++) {
    cnt += CreatePattern(0, jj, wct);
    /*
    if (!_res)
      cnt += CreatePattern(0, jj, wct);
    else if (wct<=2)
      cnt += CreatePattern(jj, 0, wct);
    */
  }
  return cnt;
}
void extRulesPat::Print(FILE* fp, uint jj)
{
  if (!_dbg)
    return;
  float units = 0.001;
  fprintf(fp,
          "%10.2f %10.2f  %10.2f %10.2f   %s\n",
          _LL[jj][0] * units,
          _UR[jj][0] * units,
          _LL[jj][1] * units,
          _UR[jj][1] * units,
          _patName[jj]);
}
void extRulesPat::PrintBbox(FILE* fp, int LL[2], int UR[2])
{
  if (!_dbg)
    return;
  float units = 0.001;
  fprintf(fp,
          "\n%10.2f %10.2f  %10.2f %10.2f   BBox\n",
          LL[0] * units,
          LL[1] * units,
          UR[0] * units,
          UR[1] * units);
}
void extRulesPat::Print(FILE* fp)
{
  if (!_dbg)
    return;
  fprintf(stdout, "\n%s\n", _name);
  for (uint jj = 1; jj < _lineCnt; jj++)
    Print(fp, jj);
}
void extRulesPat::WriteDB(uint dir, uint met, dbTechLayer* layer)
{
  // fprintf(stdout, "DB: %s\n", _name);
  for (uint jj = 1; jj < _lineCnt; jj++)
    WriteDB(jj, dir, met, layer, _def_fp);
}
void extRulesPat::WriteDB(uint jj,
                          uint dir,
                          uint met,
                          dbTechLayer* layer,
                          FILE* fp)
{
  WriteWire(_def_fp, _LL[jj], _UR[jj], _patName[jj]);
  // uint d= dir>0 ? 0 :1;
  int width = _UR[jj][dir] - _LL[jj][dir];
  int ll[2] = {_LL[jj][0] + _origin[0], _LL[jj][1] + _origin[1]};
  int ur[2] = {_UR[jj][0] + _origin[0], _UR[jj][1] + _origin[1]};

  WriteWire(_def_fp, ll, ur, _patName[jj]);

  createNetSingleWire(_patName[jj], ll, ur, width, dir == 0, met, layer);
}
void extRulesPat::WriteWire(FILE* fp, int ll[2], int ur[2], char* name)
{
  if (!_dbg)
    return;
  float units = 0.001;
  fprintf(fp,
          "%10.2f %10.2f  %10.2f %10.2f   %s\n",
          ll[0] * units,
          ur[0] * units,
          ll[1] * units,
          ur[1] * units,
          name);
}
uint extMain::DefWires(extMainOptions* opt)
{
  _tech = _db->getTech();
  uint layerCnt = _tech->getRoutingLayerCount();

  extRCModel* m = new extRCModel(layerCnt, "processName", logger_);
  _modelTable->add(m);
  m->setDataRateTable(1);
  m = _modelTable->get(0);

  m->setOptions(opt->_topDir, opt->_name);

  opt->_tech = _tech;

  dbChip* chip = dbChip::create(_db);
  assert(chip);
  _block = dbBlock::create(chip, opt->_name, nullptr, '/');
  assert(_block);

  _block->setBusDelimiters('[', ']');
  _block->setDefUnits(1000);
  m->setExtMain(this);

  m->setLayerCnt(_tech->getRoutingLayerCount());
  opt->_block = _block;

  int LL[2] = {0, 0};
  int UR[2] = {0, 0};

  uint cnt = 0;
  cnt += m->OverRulePat(
      opt, opt->_len, LL, UR, false, false, opt->_overDist);  // over
  cnt += m->OverRulePat(
      opt, opt->_len, LL, UR, true, false, opt->_overDist);  // res
  cnt += m->UnderRulePat(opt, opt->_len, LL, UR, false, opt->_overDist);

  cnt += m->OverUnderRulePat(opt, opt->_len, LL, UR);
  cnt += m->UnderRulePat(opt, opt->_len, LL, UR, true, opt->_overDist);  // diag
  cnt += m->OverRulePat(
      opt, opt->_len, LL, UR, false, true, opt->_overDist);  // diag

  cnt += m->ViaRulePat(
      opt, opt->_len, LL, UR, false, false, opt->_overDist);  // over

  dbBox* bb = _block->getBBox();
  Rect r(bb->xMin(), bb->yMin(), bb->xMax(), bb->yMax());
  _block->setDieArea(r);

  return 0;
}

uint extRCModel::OverRulePat(extMainOptions* opt,
                             int len,
                             int origin[2],
                             int UR[2],
                             bool res,
                             bool diag,
                             uint overDist)
{
  if (opt->_met == 0)
    return 0;

  extRulesPat* p = new extRulesPat("",
                                   true,
                                   false,
                                   diag,
                                   res,
                                   len,
                                   origin,
                                   origin,
                                   UR,
                                   opt->_block,
                                   _extMain,
                                   opt->_tech);

  uint cnt = 0;
  for (int met = 1; met <= (int) _layerCnt; met++) {
    if (met > opt->_met_cnt)
      continue;
    if ((opt->_met > 0) && (opt->_met != met))
      continue;

    dbTechLayer* layer = opt->_tech->findRoutingLayer(met);
    if (layer == nullptr)
      continue;

    p->setLayerInfo(layer, met);

    for (int underMet = 0; underMet < met; underMet++) {
      if (diag && underMet == 0)
        continue;
      if ((opt->_underMet > 0) && (opt->_underMet != underMet))
        continue;

      if (met - underMet > (int) opt->_underDist)
        continue;

      dbTechLayer* under_layer
          = underMet > 0 ? opt->_tech->findRoutingLayer(underMet) : nullptr;
      p->setMets(underMet, under_layer, -1, nullptr);

      if (diag)
        cnt += p->CreatePatterns_diag();
      else if (!res)
        cnt += p->CreatePatterns();
      else
        cnt += p->CreatePatterns_res();

      p->UpdateBBox();

      if (underMet == 0 && p->_res)
        break;
    }
  }
  logger_->info(
      RCX, 59, "Finished {} bench measurements for pattern MET_OVER_MET", cnt);
  origin[1] = 0;  // reset for next family of patterns
  origin[0] = p->GetOrigin_end(p->_ur_last);
  return cnt;
}
uint extRCModel::UnderRulePat(extMainOptions* opt,
                              int len,
                              int origin[2],
                              int UR[2],
                              bool diag,
                              uint overDist)
{
  if (opt->_overMet == 0)
    return 0;

  extRulesPat* p = new extRulesPat("",
                                   false,
                                   true,
                                   diag,
                                   false,
                                   len,
                                   origin,
                                   origin,
                                   UR,
                                   opt->_block,
                                   _extMain,
                                   opt->_tech);

  uint cnt = 0;
  for (int met = 1; met <= (int) _layerCnt - 1; met++) {
    if (met > opt->_met_cnt)
      continue;
    if ((opt->_met > 0) && (opt->_met != met))
      continue;

    dbTechLayer* layer = opt->_tech->findRoutingLayer(met);
    if (layer == nullptr)
      continue;

    p->setLayerInfo(layer, met);

    for (uint overMet = met + 1; overMet <= _layerCnt; overMet++) {
      if (overMet > opt->_met_cnt)
        continue;
      if (overMet - met > overDist)
        continue;
      if ((opt->_overMet > 0) && (opt->_overMet != (int) overMet))
        continue;

      dbTechLayer* over_layer = opt->_tech->findRoutingLayer(overMet);
      if (over_layer == nullptr)
        continue;

      p->setMets(-1, nullptr, overMet, over_layer);

      if (!diag)
        cnt += p->CreatePatterns();
      else
        cnt += p->CreatePatterns_diag();

      p->UpdateBBox();

      UR[0] = std::max(UR[0], p->_ur[0]);
      UR[1] = std::max(UR[1], p->_ur[1]);
    }
  }
  logger_->info(
      RCX, 9, "Finished {} measurements for pattern MET_UNDER_MET", cnt);
  origin[1] = 0;  // reset for next family of patterns
  origin[0] = p->GetOrigin_end(p->_ur_last);
  return cnt;
}
uint extRCModel::DiagUnderRulePat(extMainOptions* opt,
                                  int len,
                                  int LL[2],
                                  int UR[2])
{
  // NOT USED
  if (opt->_overMet == 0)
    return 0;

  extRulesPat* p = new extRulesPat("",
                                   false,
                                   false,
                                   true,
                                   false,
                                   len,
                                   LL,
                                   LL,
                                   UR,
                                   opt->_block,
                                   _extMain,
                                   opt->_tech);

  uint cnt = 0;
  for (int met = 1; met <= (int) _layerCnt - 1; met++) {
    if (met > opt->_met_cnt)
      continue;
    if ((opt->_met > 0) && (opt->_met != met))
      continue;

    dbTechLayer* layer = opt->_tech->findRoutingLayer(met);
    if (layer == nullptr)
      continue;

    p->setLayerInfo(layer, met);

    for (uint overMet = met + 1; overMet <= _layerCnt; overMet++) {
      if (overMet > opt->_met_cnt)
        continue;
      if (overMet - met > opt->_overDist)
        continue;
      if ((opt->_overMet > 0) && (opt->_overMet != (int) overMet))
        continue;

      dbTechLayer* over_layer = opt->_tech->findRoutingLayer(overMet);
      if (over_layer == nullptr)
        continue;

      p->setMets(-1, nullptr, overMet, over_layer);

      cnt += p->CreatePatterns_diag();
      p->UpdateBBox();

      UR[0] = std::max(UR[0], p->_ur[0]);
      UR[1] = std::max(UR[1], p->_ur[1]);
    }
  }
  logger_->info(
      RCX, 10, "Finished {} measurements for pattern MET_UNDER_MET", cnt);
  return cnt;
}
uint extRCModel::OverUnderRulePat(extMainOptions* opt,
                                  int len,
                                  int origin[2],
                                  int UR[2])
{
  if (opt->_overMet == 0)
    return 0;

  extRulesPat* p = new extRulesPat("",
                                   true,
                                   true,
                                   false,
                                   false,
                                   len,
                                   origin,
                                   origin,
                                   UR,
                                   opt->_block,
                                   _extMain,
                                   opt->_tech);

  uint cnt = 0;
  for (int met = 1; met <= (int) _layerCnt - 1; met++) {
    if (met > opt->_met_cnt)
      continue;
    if ((opt->_met > 0) && (opt->_met != met))
      continue;

    dbTechLayer* layer = opt->_tech->findRoutingLayer(met);
    if (layer == nullptr)
      continue;

    p->setLayerInfo(layer, met);

    for (int underMet = 1; underMet < met; underMet++) {
      if (met - underMet > (int) opt->_underDist)
        continue;
      if ((opt->_underMet > 0) && ((int) opt->_underMet != underMet))
        continue;

      dbTechLayer* under_layer = opt->_tech->findRoutingLayer(underMet);
      if (under_layer == nullptr)
        continue;

      for (uint overMet = met + 1; overMet <= _layerCnt; overMet++) {
        if (overMet > opt->_met_cnt)
          continue;
        if (overMet - met > opt->_overDist)
          continue;
        if ((opt->_overMet > 0) && (opt->_overMet != (int) overMet))
          continue;

        dbTechLayer* over_layer = opt->_tech->findRoutingLayer(overMet);
        if (over_layer == nullptr)
          continue;

        p->setMets(underMet, under_layer, overMet, over_layer);

        cnt += p->CreatePatterns();
        p->UpdateBBox();

        UR[0] = std::max(UR[0], p->_ur[0]);
        UR[1] = std::max(UR[1], p->_ur[1]);
      }
    }
  }
  logger_->info(
      RCX, 11, "Finished {} measurements for pattern MET_UNDER_MET", cnt);
  origin[1] = 0;  // reset for next family of patterns
  origin[0] = p->GetOrigin_end(p->_ur_last);
  return cnt;
}
dbBTerm* extRulesPat::createBterm(bool lo,
                                  dbNet* net,
                                  int ll[2],
                                  int ur[2],
                                  const char* postFix,
                                  dbTechLayer* layer,
                                  uint width,
                                  bool vertical,
                                  bool io)
{
  std::string term_str(net->getConstName());
  term_str = term_str + postFix;

  dbBTerm* bterm = dbBTerm::create(net, term_str.c_str());

  dbBPin* bpin = dbBPin::create(bterm);
  bpin->setPlacementStatus(dbPlacementStatus::PLACED);

  int hwidth = width / 2;
  if (lo) {
    if (!vertical)
      dbBox::create(bpin,
                    layer,
                    ll[0],
                    ll[1] - hwidth,
                    ll[0] + width,
                    ll[1] + hwidth);  // TESTED OK
    else
      dbBox::create(bpin,
                    layer,
                    ll[0] - hwidth,
                    ll[1],
                    ll[0] + hwidth,
                    ll[1] + width);  // TO_TEST
  } else {
    if (!vertical)
      dbBox::create(bpin,
                    layer,
                    ur[0] - width,
                    ur[1] - width - hwidth,
                    ur[0],
                    ur[1] - hwidth);  // TESTED OK
    else
      dbBox::create(bpin,
                    layer,
                    ur[0] - width,
                    ur[1] - hwidth,
                    ur[0],
                    ur[1] + hwidth);  // TO TEST
  }
  bterm->setSigType(dbSigType::SIGNAL);
  if (io)
    bterm->setIoType(dbIoType::INPUT);
  else
    bterm->setIoType(dbIoType::OUTPUT);

  return bterm;
}

dbBTerm* extRulesPat::createBterm1(bool lo,
                                   dbNet* net,
                                   int ll[2],
                                   int ur[2],
                                   const char* postFix,
                                   dbTechLayer* layer,
                                   uint width,
                                   bool vertical,
                                   bool io)
{
  std::string term_str("N");
  term_str = term_str + std::to_string(net->getId()) + postFix;
  dbBTerm* bterm = dbBTerm::create(net, term_str.c_str());

  dbBPin* bpin = dbBPin::create(bterm);
  bpin->setPlacementStatus(dbPlacementStatus::PLACED);

  uint hwidth = width / 2;

  int x1 = ll[0];
  int x2 = ur[0];
  int y1 = ll[1] - hwidth;
  int y2 = ll[1] + hwidth;
  if (vertical) {
    if (!lo) {
      y1 = ur[1] - hwidth;
      y2 = ur[1] + hwidth;
    }
  } else {
    y1 = ll[1];
    y2 = ur[1];
    x1 = ll[0] - hwidth;
    x2 = ll[0] + hwidth;
    if (!lo) {
      x1 = ur[0] - hwidth;
      x2 = ur[0] + hwidth;
    }
  }
  dbBox::create(bpin, layer, x1, y1, x2, y2);

  bterm->setSigType(dbSigType::SIGNAL);
  if (io)
    bterm->setIoType(dbIoType::INPUT);
  else
    bterm->setIoType(dbIoType::OUTPUT);

  return bterm;
}

dbNet* extRulesPat::createNetSingleWire(const char* netName,
                                        int ll[2],
                                        int ur[2],
                                        uint width,
                                        bool vertical,
                                        uint met,
                                        dbTechLayer* layer)
{
  dbNet* net = dbNet::create(_block, netName);
  if (net == nullptr) {
    fprintf(stdout, "Cannot create net %s, duplicate\n", netName);
    return nullptr;
  }
  net->setSigType(dbSigType::SIGNAL);
  dbBTerm* loBTerm = extRulesPat::createBterm(
      false, net, ll, ur, "_lo", layer, width, vertical, true /*input*/);
  dbBTerm* hiBTerm = extRulesPat::createBterm(
      true, net, ll, ur, "_hi", layer, width, vertical, false /*input*/);
  if ((loBTerm == nullptr) || (hiBTerm == nullptr)) {
    dbNet::destroy(net);
    fprintf(stdout,
            "Cannot create net %s, because failed to create bterms\n",
            netName);
    return nullptr;
  }

  dbWireEncoder encoder;
  encoder.begin(dbWire::create(net));

  encoder.newPath(layer, dbWireType::ROUTED);

  encoder.addPoint(ll[0], ll[1], 0);
  encoder.addBTerm(loBTerm);

  if (vertical) {
    encoder.addPoint(ll[0], ur[1], 0);
  } else
    encoder.addPoint(ur[0], ll[1], 0);

  encoder.addBTerm(hiBTerm);
  encoder.end();

  return net;
}

/*
dbTechLayerRule* extRulesPat::GetRule(int routingLayer, int width)
{
  dbTechLayerRule*& rule = _rules[routingLayer][width];

  if (rule != nullptr)
    return rule;

  // Create a non-default-rule for this width
  dbTechNonDefaultRule* nd_rule;
  char rule_name[64];

  while (_ruleNameHint >= 0) {
    snprintf(rule_name, 64, "ADS_ND_%d", _ruleNameHint++);
    nd_rule = dbTechNonDefaultRule::create(_tech, rule_name);

    if (nd_rule)
      break;
  }

  if (nd_rule == nullptr)
    return nullptr;
  fprintf(stdout, " NewRule %s,layer=%d width=%d\n", rule_name, routingLayer,
width); fflush(stdout);
  // rule->getImpl()->getLogger()->info(utl::ODB,
  //                                    273,
  //                                    "Create ND RULE {} for layer/width
{},{}",
  //                                    rule_name,
  //                                    routingLayer,
  //                                    width);

  int i;
  for (i = 1; i <= _tech->getRoutingLayerCount(); i++) {
    dbTechLayer* layer = _routingLayers[i];

    if (layer != nullptr) {
      dbTechLayerRule* lr = dbTechLayerRule::create(nd_rule, layer);
      lr->setWidth(width);
      lr->setSpacing(layer->getSpacing());

      dbTechLayerRule*& r = _rules[i][width];
      if (r == nullptr)
        r = lr;
    }
  }

  assert(rule != nullptr);
  return rule;
}
*/
}  // namespace rcx
