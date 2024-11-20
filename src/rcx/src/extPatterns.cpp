
///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2024, IC BENCH, Dimitris Fotakis
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

#include <map>
#include <vector>

#include "rcx/extPattern.h"
#include "rcx/extRCap.h"
#include "rcx/extSpef.h"
#include "rcx/extprocess.h"
#include "utl/Logger.h"

namespace rcx {

using utl::RCX;
using namespace odb;

uint extMain::benchPatternsGen(const PatternOptions& opt1)
{
  PatternOptions opt = opt1;
  _tech = _db->getTech();
  if (_tech == NULL) {
    fprintf(stderr, "NO tech! exit\n");
    return 0;
  }
  opt.met_cnt = _tech->getRoutingLayerCount();

  printf("benchPatternsGen= layerCnt %d %s\n", opt.met_cnt, opt.name);

  dbChip* chip = dbChip::create(_db);
  assert(chip);
  _block = dbBlock::create(chip, opt.name, NULL, '/');
  assert(_block);
  // _prevControl = _block->getExtControl();
  _block->setBusDelimeters('[', ']');
  _block->setDefUnits(1000);
  // m->setExtMain(this);
  setupMapping(0);
  _noModelRC = true;
  _cornerCnt = 1;
  _extDbCnt = 1;
  _block->setCornerCount(_cornerCnt);
  // opt->_ll[0] = 0;
  //  opt->_ll[1] = 0;
  //  opt->_ur[0] = 0;
  //  opt->_ur[1] = 0;
  // m->setLayerCnt(_tech->getRoutingLayerCount());

  // opt->_block = _block;

  dbCreateNetUtil* db_net_util = new dbCreateNetUtil(logger_);
  db_net_util->setBlock(_block);

  int origin[2] = {0, 0};  // TODO - input

  bool all = !opt.over && !opt.under && !opt.over_under;

  uint cnt = 0;
  if (opt.over || all)
    cnt += overPatterns(opt, origin, db_net_util);
  if (opt.under || all)
    cnt += UnderPatterns(opt, origin, db_net_util);
  if (opt.over_under || all)
    cnt += OverUnderPatterns(opt, origin, db_net_util);

  fprintf(stdout, "\n       %d Total Patterns\n", cnt);

  dbBox* bb = _block->getBBox();
  Rect r(bb->xMin(), bb->yMin(), bb->xMax(), bb->yMax());
  _block->setDieArea(r);
  _extracted = true;
  // updatePrevControl();

  return 0;
}

uint extMain::overPatterns(const PatternOptions& opt,
                           int origin[2],
                           dbCreateNetUtil* db_net_util)
{
  int wireCnt = 5;

  int start[2] = {origin[0], origin[1]};
  uint cnt = 0;
  for (int met = 1; met <= opt.met_cnt; met++) {
    if ((opt.met > 0) && (opt.met != met))  // TODO change to list
      continue;

    FILE* fp = extPattern::OpenLog(met, "o");

    int pattern_separation = 0;
    int max_ur[2] = {0, 0};
    for (int under = 0; under < met; under++) {
      if (under > 0 && met - under > opt.under_dist)
        continue;

      extPattern p(
          wireCnt, -1, -1, met, under, under - 1, opt, fp, start, db_net_util);
      cnt += p.CreatePattern_under(start, max_ur);
      pattern_separation = p._pattern_separation;
    }
    start[0] = max_ur[0] + pattern_separation;
    start[1] = origin[1];

    fclose(fp);
  }
  extPattern::PrintStats(cnt, origin, start, "Total Over");
  return cnt;
}
uint extMain::UnderPatterns(const PatternOptions& opt,
                            int origin[2],
                            dbCreateNetUtil* db_net_util)
{
  int wireCnt = 5;
  int start[2] = {origin[0], origin[1]};

  uint cnt = 0;
  for (int met = 1; met < opt.met_cnt; met++) {
    if ((opt.met > 0) && (opt.met != met))  // change to list
      continue;

    FILE* fp = extPattern::OpenLog(met, "u");

    int pattern_separation = 0;
    int max_ur[2] = {0, 0};
    for (int over = met + 1; over <= opt.met_cnt; over++) {
      if (over < opt.met_cnt && over - met > opt.over_dist)
        continue;

      int over2 = over + 1;
      if (over2 > opt.met_cnt)
        over2 = -1;
      extPattern p(
          wireCnt, over2, over, met, -1, -1, opt, fp, start, db_net_util);
      cnt += p.CreatePattern_over(start, max_ur);
      pattern_separation = p._pattern_separation;
    }
    start[0] = max_ur[0] + pattern_separation;
    start[1] = origin[1];
  }
  extPattern::PrintStats(cnt, origin, start, "Total Under");
  return cnt;
}
uint extMain::OverUnderPatterns(const PatternOptions& opt,
                                int origin[2],
                                dbCreateNetUtil* db_net_util)
{
  int wireCnt = 5;
  int start[2] = {origin[0], origin[1]};

  uint cnt = 0;
  for (int met = 1; met < opt.met_cnt; met++) {
    if ((opt.met > 0) && (opt.met != met))  // change to list
      continue;

    FILE* fp = extPattern::OpenLog(met, "ou");

    int pattern_separation = 0;
    int max_ur[2] = {0, 0};
    for (int over = met + 1; over <= opt.met_cnt; over++) {
      if (over - met > opt.over_dist)
        continue;

      for (int under = 1; under < met; under++) {
        if (met - under > opt.under_dist)
          continue;

        int over2 = over + 1;
        if (over2 > opt.met_cnt)
          over2 = -1;
        extPattern p(wireCnt,
                     over2,
                     over,
                     met,
                     under,
                     under - 1,
                     opt,
                     fp,
                     start,
                     db_net_util);
        cnt += p.CreatePattern(start, max_ur, db_net_util);
        pattern_separation = p._pattern_separation;
      }
      start[0] = max_ur[0] + pattern_separation;
      start[1] = origin[1];
    }
    fclose(fp);
  }
  extPattern::PrintStats(cnt, origin, start, "Total OverUnder");
  return cnt;
}
void extPattern::PrintStats(uint cnt,
                            int origin[2],
                            int start[2],
                            const char* msg)
{
  fprintf(stdout,
          "\n---------------- %s ---------- %d patterns origin= %d %d start %d "
          "%d \n\n",
          msg,
          cnt,
          origin[0],
          origin[1],
          start[0],
          start[1]);
}
extPattern::extPattern(int cnt,
                       int over1,
                       int over,
                       int m,
                       int under,
                       int under1,
                       const PatternOptions& opt1,
                       FILE* fp,
                       int org[2],
                       dbCreateNetUtil* net_util)
{
  nameHash = new AthHash<int>(10000000, 0);  // TODO: check for memory free

  patternLog = fp;
  opt = opt1;
  wireCnt = cnt;
  met = m;
  over_met = over;
  under_met = under;
  over_met2 = over1;
  under_met2 = under1;

  char patternName_buff[100];
  if (under > 0 && over > 0) {
    sprintf(patternName_buff, "OU%d_M%doM%duM%d", wireCnt, met, under, over);
  } else if (under >= 0) {
    sprintf(patternName_buff, "O%d_M%doM%d", wireCnt, met, under);
  } else {
    sprintf(patternName_buff, "U%d_M%duM%d", wireCnt, met, over);
  }
  if (over > 0 && over1 > 0)
    sprintf(patternName, "%s__uM%d", patternName_buff, over1);
  if (under > 0 && under1 > 0)
    sprintf(patternName, "%s__oM%d", patternName_buff, under1);

  mWidth = getMultipliers(opt.width);
  mSpacing = getMultipliers(opt.spacing);
  lWidth = getMultipliers(opt.couple_width);
  rWidth = getMultipliers(opt.couple_width);
  csWidth = getMultipliers(opt.far_width);
  csSpacing = getMultipliers(opt.far_spacing);

  overSpacing = getMultipliers(opt.over_spacing);
  over2Spacing = getMultipliers(opt.over2_spacing);
  underSpacing = getMultipliers(opt.under_spacing);
  under2Spacing = getMultipliers(opt.under2_spacing);
  overWidth = getMultipliers(opt.over_width);
  over2Width = getMultipliers(opt.over2_width);
  underWidth = getMultipliers(opt.under_width);
  under2Width = getMultipliers(opt.under2_width);
  underOffset = getMultipliers(opt.offset_under);
  overOffset = getMultipliers(opt.offset_over);

  origin[0] = org[0];
  origin[1] = org[1];
  _origin[0] = (int) org[0];
  _origin[1] = (int) org[1];

  db_net_util = NULL;
  init(net_util);
}
float extPattern::init(dbCreateNetUtil* net_util)
{
  units = 1000;
  float uu = 0.001;
  db_net_util = net_util;
  dbTechLayer* layer = db_net_util->getRoutingLayer()[met];
  // const char *name= layer->getConstName();
  _minWidth = layer->getWidth();
  // if (met==6)
  //     fprintf(stdout, "M6 getWidth= %d  getMinWidth=%d \n",
  //     layer->getWidth(), layer->getMinWidth());

  // _minWidth = layer->getMinWidth() ;
  minWidth = _minWidth * uu;
  int pitch = layer->getPitch();
  int minSpace = layer->getSpacing();
  int s = pitch - _minWidth;
  if (s > minSpace)
    s = minSpace;

  _minSpacing = s;
  minSpacing = s * uu;

  // TODO: debug why not working dir = layer->getDirection() ==
  // dbTechLayerDir::HORIZONTAL ? 1 : 0;
  dir = layer->getDirection() == 2 ? 1 : 0;

  fprintf(stdout,
          "%s %g %g    %d  %d \n",
          patternName,
          0.001 * _origin[0],
          0.001 * _origin[1],
          _origin[0],
          _origin[1]);
  strcpy(lastName, "");

  _cnt = 0;
  _totEnumerationCnt = 0;
  return minSpacing;
}
uint extPattern::CreatePattern(int org[2],
                               int MAX_UR[2],
                               dbCreateNetUtil* net_util)
{
  for (uint i = 0; i < mWidth.size(); i++)  // main wire
  {
    float mw = mWidth[i];                          // width multiplier;
    for (uint jj = 0; jj < mSpacing.size(); jj++)  // left wire
    {
      float msL = mSpacing[jj];
      for (uint k = 0; k < mSpacing.size(); k++)  // right wire
      {
        float msR = mSpacing[k];
        for (uint ll = 0; ll < lWidth.size(); ll++)  // left wire
        {
          float mwL = lWidth[ll];
          for (uint rr = 0; rr < rWidth.size(); rr++)  // right wire
          {
            float mwR = rWidth[rr];
            for (uint cl = 0; cl < csWidth.size(); cl++)  // left wire
            {
              float cwl = csWidth[cl];
              for (uint cr = 0; cr < csWidth.size(); cr++)  // right wire
              {
                float cwr = csWidth[cr];
                for (uint sl = 0; sl < csSpacing.size(); sl++)  // left wire
                {
                  float csl = csSpacing[sl];
                  for (uint sr = 0; sr < csSpacing.size(); sr++)  // right wire
                  {
                    float csr = csSpacing[sr];
                    sprintf(
                        targetMetName,
                        "__M%d_w%g_sl%g_sr%g_L_w%g_R_w%g_LL_w%g_s%g_RR_w%g_s%g",
                        met,
                        mw,
                        msL,
                        msR,
                        mwL,
                        mwR,
                        cwl,
                        csl,
                        cwr,
                        csr);

                    for (uint i1 = 0; i1 < overWidth.size(); i1++) {
                      float ow = overWidth[i1];
                      for (uint i2 = 0; i2 < overSpacing.size(); i2++) {
                        float os = overSpacing[i2];
                        for (uint i3 = 0; i3 < over2Width.size(); i3++) {
                          float ow2 = over2Width[i3];
                          for (uint i4 = 0; i4 < over2Spacing.size(); i4++) {
                            float os2 = over2Spacing[i4];
                            for (uint j5 = 0; j5 < overOffset.size(); j5++) {
                              float offsetOver = overOffset[j5];
                              for (uint j1 = 0; j1 < underWidth.size(); j1++) {
                                float uw = underWidth[j1];
                                for (uint j2 = 0; j2 < underSpacing.size();
                                     j2++) {
                                  float us = underSpacing[j2];
                                  for (uint j3 = 0; j3 < under2Width.size();
                                       j3++) {
                                    float uw2 = under2Width[j3];
                                    for (uint j4 = 0; j4 < under2Spacing.size();
                                         j4++) {
                                      float us2 = under2Spacing[j4];

                                      for (uint j6 = 0; j6 < underOffset.size();
                                           j6++) {
                                        float offsetUnder = underOffset[j6];

                                        sprintf(contextName, "%s", "");
                                        createContextName_under(
                                            uw, us, offsetUnder, uw2, us2);
                                        createContextName_over(
                                            ow, os, offsetOver, ow2, os2);

                                        if (SetPatternName())
                                          continue;

                                        extWirePattern* mainp = MainPattern(
                                            mw,
                                            msL,
                                            msR,
                                            mwL,
                                            mwR,
                                            cwl,
                                            csl,
                                            cwr,
                                            csr);  // this->origin does NOT
                                                   // change
                                        if (mainp == NULL)
                                          continue;

                                        contextPatterns_under(mainp,
                                                              uw,
                                                              us,
                                                              offsetUnder,
                                                              uw2,
                                                              us2);
                                        contextPatterns_over(mainp,
                                                             ow,
                                                             os,
                                                             offsetOver,
                                                             ow2,
                                                             os2);
                                        PatternEnd(mainp, max_ur, 10);
                                      }
                                    }
                                  }
                                }
                              }
                            }
                          }
                        }
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  set_max(MAX_UR);
  org[!dir] = MAX_UR[!dir] + _pattern_separation;
  fprintf(stdout,
          "%s         cnt=%d (out of %d enumerated)\n",
          patternName,
          _cnt,
          _totEnumerationCnt);
  return _cnt;
}
uint extPattern::CreatePattern_over(int org[2], int MAX_UR[2])
{
  for (uint i = 0; i < mWidth.size(); i++)  // main wire
  {
    float mw = mWidth[i];                          // width multiplier;
    for (uint jj = 0; jj < mSpacing.size(); jj++)  // left wire
    {
      float msL = mSpacing[jj];
      for (uint k = 0; k < mSpacing.size(); k++)  // right wire
      {
        float msR = mSpacing[k];
        for (uint ll = 0; ll < lWidth.size(); ll++)  // left wire
        {
          float mwL = lWidth[ll];
          for (uint rr = 0; rr < rWidth.size(); rr++)  // right wire
          {
            float mwR = rWidth[rr];
            for (uint cl = 0; cl < csWidth.size(); cl++)  // left wire
            {
              float cwl = csWidth[cl];
              for (uint cr = 0; cr < csWidth.size(); cr++)  // right wire
              {
                float cwr = csWidth[cr];
                for (uint sl = 0; sl < csSpacing.size(); sl++)  // left wire
                {
                  float csl = csSpacing[sl];
                  for (uint sr = 0; sr < csSpacing.size(); sr++)  // right wire
                  {
                    float csr = csSpacing[sr];
                    sprintf(
                        targetMetName,
                        "__M%d_w%g_sl%g_sr%g_L_w%g_R_w%g_LL_w%g_s%g_RR_w%g_s%g",
                        met,
                        mw,
                        msL,
                        msR,
                        mwL,
                        mwR,
                        cwl,
                        csl,
                        cwr,
                        csr);

                    for (uint i1 = 0; i1 < overWidth.size(); i1++) {
                      float ow = overWidth[i1];
                      for (uint i2 = 0; i2 < overSpacing.size(); i2++) {
                        float os = overSpacing[i2];
                        if (over_met2 <= 0) {
                          sprintf(contextName, "%s", "");
                          createContextName_over(ow, os, 0, 0, 0);
                          if (SetPatternName())
                            continue;
                          extWirePattern* mainp = MainPattern(
                              mw, msL, msR, mwL, mwR, cwl, csl, cwr, csr);
                          if (mainp == NULL)
                            continue;
                          contextPatterns_over(mainp, ow, os, 0, 0, 0);
                          PatternEnd(mainp, max_ur, 10);
                        }
                        if (over_met2 > 0) {
                          for (uint i3 = 0; i3 < over2Width.size(); i3++) {
                            float ow2 = over2Width[i3];
                            for (uint i4 = 0; i4 < over2Spacing.size(); i4++) {
                              float os2 = over2Spacing[i4];
                              for (uint j5 = 0; j5 < overOffset.size(); j5++) {
                                float offsetOver = overOffset[j5];

                                sprintf(contextName, "%s", "");
                                createContextName_over(
                                    ow, os, offsetOver, ow2, os2);
                                if (SetPatternName())
                                  continue;
                                extWirePattern* mainp = MainPattern(
                                    mw, msL, msR, mwL, mwR, cwl, csl, cwr, csr);
                                if (mainp == NULL)
                                  continue;
                                contextPatterns_over(
                                    mainp, ow, os, offsetOver, ow2, os2);
                                PatternEnd(mainp, max_ur, 10);
                              }
                            }
                          }
                        }
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  set_max(MAX_UR);
  org[!dir] = MAX_UR[!dir] + _pattern_separation;
  fprintf(stdout,
          "%s         cnt=%d (out of %d enumerated)\n",
          patternName,
          _cnt,
          _totEnumerationCnt);
  return _cnt;
}
uint extPattern::CreatePattern_under(int org[2], int MAX_UR[2])
{
  for (uint i = 0; i < mWidth.size(); i++)  // main wire
  {
    float mw = mWidth[i];                          // width multiplier;
    for (uint jj = 0; jj < mSpacing.size(); jj++)  // left wire
    {
      float msL = mSpacing[jj];
      for (uint k = 0; k < mSpacing.size(); k++)  // right wire
      {
        float msR = mSpacing[k];
        for (uint ll = 0; ll < lWidth.size(); ll++)  // left wire
        {
          float mwL = lWidth[ll];
          for (uint rr = 0; rr < rWidth.size(); rr++)  // right wire
          {
            float mwR = rWidth[rr];
            for (uint cl = 0; cl < csWidth.size(); cl++)  // left wire
            {
              float cwl = csWidth[cl];
              for (uint cr = 0; cr < csWidth.size(); cr++)  // right wire
              {
                float cwr = csWidth[cr];
                for (uint sl = 0; sl < csSpacing.size(); sl++)  // left wire
                {
                  float csl = csSpacing[sl];
                  for (uint sr = 0; sr < csSpacing.size(); sr++)  // right wire
                  {
                    float csr = csSpacing[sr];
                    sprintf(
                        targetMetName,
                        "__M%d_w%g_sl%g_sr%g_L_w%g_R_w%g_LL_w%g_s%g_RR_w%g_s%g",
                        met,
                        mw,
                        msL,
                        msR,
                        mwL,
                        mwR,
                        cwl,
                        csl,
                        cwr,
                        csr);

                    for (uint j1 = 0; j1 < underWidth.size(); j1++) {
                      float uw = underWidth[j1];
                      for (uint j2 = 0; j2 < underSpacing.size(); j2++) {
                        float us = underSpacing[j2];
                        if (under_met2 <= 0) {
                          sprintf(contextName, "%s", "");
                          createContextName_under(uw, us, 0, 0, 0);
                          if (SetPatternName())
                            continue;
                          extWirePattern* mainp = MainPattern(
                              mw, msL, msR, mwL, mwR, cwl, csl, cwr, csr);
                          if (mainp == NULL)
                            continue;
                          contextPatterns_under(mainp, uw, us, 0, 0, 0);
                          PatternEnd(mainp, max_ur, 10);
                        }
                        if (under_met2 > 0) {
                          for (uint j3 = 0; j3 < under2Width.size(); j3++) {
                            float uw2 = under2Width[j3];
                            for (uint j4 = 0; j4 < under2Spacing.size(); j4++) {
                              float us2 = under2Spacing[j4];
                              for (uint j6 = 0; j6 < underOffset.size(); j6++) {
                                float offsetUnder = underOffset[j6];

                                sprintf(contextName, "%s", "");
                                createContextName_under(
                                    uw, us, offsetUnder, uw2, us2);
                                if (SetPatternName())
                                  continue;
                                extWirePattern* mainp = MainPattern(
                                    mw, msL, msR, mwL, mwR, cwl, csl, cwr, csr);
                                if (mainp == NULL)
                                  continue;
                                contextPatterns_under(
                                    mainp, uw, us, offsetUnder, uw2, us2);
                                PatternEnd(mainp, max_ur, 10);
                              }
                            }
                          }
                        }
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  set_max(MAX_UR);
  org[!dir] = MAX_UR[!dir] + _pattern_separation;
  fprintf(stdout,
          "%s         cnt=%d (out of %d enumerated)\n",
          patternName,
          _cnt,
          _totEnumerationCnt);
  return _cnt;
}
void extPattern::PatternEnd(extWirePattern* mainp,
                            int max_ur[2],
                            uint spacingMult)
{
  _pattern_separation = _minSpacing * spacingMult;
  _origin[dir] = max_ur[dir] + _pattern_separation;
  delete mainp;
  _cnt++;
}
bool extPattern::createContextName_under(float uw,
                                         float us,
                                         float offsetUnder,
                                         float uw2,
                                         float us2)
{
  if (under_met > 0) {
    if (uw > 0 && us > 0)
      sprintf(contextName, "__M%d_w%g_s%g", under_met, uw, us);
    if (under_met2 > 0 && (uw2 > 0 && us2 > 0))
      sprintf(
          contextName, "__f%g_M%d_w%g_s%g", offsetUnder, under_met2, uw2, us2);
  }
  return false;
}
bool extPattern::createContextName_over(float ow,
                                        float os,
                                        float offsetOver,
                                        float ow2,
                                        float os2)
{
  if (over_met > 0) {
    if (ow > 0 && os > 0)
      sprintf(contextName, "__M%d_w%g_s%g", over_met, ow, os);
    if ((over_met2 > 0) && (ow2 > 0 && os2 > 0))
      sprintf(
          contextName, "__f%g_M%d_w%g_s%g", offsetOver, over_met2, ow2, os2);
  }
  return false;
}
uint extPattern::contextPatterns_under(extWirePattern* mainp,
                                       float uw,
                                       float us,
                                       float offsetUnder,
                                       float uw2,
                                       float us2)
{
  if (under_met > 0) {
    if (uw > 0 && us > 0)
      ContextPattern(mainp, !dir, under_met, uw, us, offsetUnder);
    if ((under_met2 > 0) && (uw2 > 0 && us2 > 0))
      ContextPatternParallel(mainp, dir, under_met2, uw2, us2, offsetUnder);
  }
  return 0;
}
uint extPattern::contextPatterns_over(extWirePattern* mainp,
                                      float ow,
                                      float os,
                                      float offsetOver,
                                      float ow2,
                                      float os2)
{
  if (over_met > 0) {
    if (ow > 0 && os > 0)
      ContextPattern(mainp, !dir, over_met, ow, os, offsetOver);
    if ((over_met2 > 0) && (ow2 > 0 && os2 > 0))
      ContextPatternParallel(mainp, dir, over_met2, ow2, os2, offsetOver);
  }
  return 0;
}
/*
uint extPattern::CreatePattern_save(int org[2], float MAX_UR[2], dbCreateNetUtil
*net_util )
{
  nameHash= new odb::AthHash<int>(10000000, 0) ; // TODO: check for memory free

  origin[0]= org[0];
  origin[1]= org[1];

  init(net_util);

  fprintf(stdout, "%s  %g  %g \n", patternName, origin[0], origin[1]);
  // return 0;
  strcpy(lastName, "");
  // char baseName[4000];
  uint cnt = 0;
  for (uint i = 0; i < mWidth.size(); i++)  // main wire
  {
    float mw = mWidth[i];                          // width multiplier;
    for (uint jj = 0; jj < mSpacing.size(); jj++)  // left wire
    {
      float msL = mSpacing[jj];
      for (uint k = 0; k < mSpacing.size(); k++)  // right wire
      {
        float msR = mSpacing[k];
        for (uint ll = 0; ll < lWidth.size(); ll++)  // left wire
        {
          float mwL = lWidth[ll];
          for (uint rr = 0; rr < rWidth.size(); rr++)  // right wire
          {
            float mwR = rWidth[rr];
            for (uint cl = 0; cl < csWidth.size(); cl++)  // left wire
            {
              float cwl = csWidth[cl];
              for (uint cr = 0; cr < csWidth.size(); cr++)  // right wire
              {
                float cwr = csWidth[cr];
                for (uint sl = 0; sl < csSpacing.size(); sl++)  // left wire
                {
                  float csl = csSpacing[sl];
                  for (uint sr = 0; sr < csSpacing.size(); sr++)  // right wire
                  {
                    float csr = csSpacing[sr];
                    sprintf(
                        targetMetName,
                        "__M%d_w%g_sl%g_sr%g_L_w%g_R_w%g_LL_w%g_s%g_RR_w%g_s%g",
                        met, mw, msL, msR, mwL, mwR, cwl, csl, cwr, csr);

                    for (uint i1 = 0; i1 < overWidth.size(); i1++) {
                      float ow = overWidth[i1];
                      for (uint i2 = 0; i2 < overSpacing.size(); i2++) {
                        float os = overSpacing[i2];
                        for (uint i3 = 0; i3 < over2Width.size(); i3++) {
                          float ow2 = overWidth[i3];
                          for (uint i4 = 0; i4 < over2Spacing.size(); i4++) {
                            float os2 = over2Spacing[i4];
                            for (uint j1 = 0; j1 < underWidth.size(); j1++) {
                              float uw = underWidth[j1];
                              for (uint j2 = 0; j2 < underSpacing.size(); j2++)
{ float us = underSpacing[j2]; for (uint j3 = 0; j3 < under2Width.size(); j3++)
{ float uw2 = under2Width[j3]; for (uint j4 = 0; j4 < under2Spacing.size();
j4++) { float us2 = under2Spacing[j4]; for (uint j5 = 0; j5 < overOffset.size();
j5++) { float offsetOver = overOffset[j5]; for (uint j6 = 0; j6 <
underOffset.size(); j6++) { float offsetUnder = underOffset[j6];

                                        sprintf(contextName, "");
                                        if (under_met > 0) {
                                          if (uw > 0 && us > 0)
                                            sprintf(contextName,
"%s__M%d_w%g_s%g", contextName, under_met, uw, us); if (under_met2>0 && (uw2 > 0
&& us2 > 0)) sprintf(contextName, "%s__f%g_M%d_w%g_s%g", contextName,
offsetUnder, under_met2, uw2, us2);
                                        }
                                        if (over_met > 0) {
                                          if (ow > 0 && os > 0)
                                            sprintf(contextName,
"%s__M%d_w%g_s%g", contextName, over_met, ow, os); if ((over_met2>0) && (ow2 > 0
&& os2 > 0)) sprintf(contextName, "%s__f%g_M%d_w%g_s%g", contextName,
offsetOver, over_met2, ow2, os2);
                                        }
                                        if (SetPatternName())
                                          continue;

                                        // this->origin does NOT change
                                        extWirePattern* mainp=
MainPattern(mw,msL,msR,mwL,mwR,cwl,csl,cwr,csr); if (mainp==NULL) continue;

                                        cnt++;

                                        if (under_met > 0) {
                                          if (uw > 0 && us > 0)
                                            ContextPattern(mainp, !dir,
under_met, uw, us, offsetUnder); if ((under_met2>0) && (uw2 > 0 && us2 > 0) )
                                            ContextPatternParallel(mainp, dir,
under_met2, uw2, us2, offsetUnder);
                                        }
                                        if (over_met > 0) {
                                          if (ow > 0 && os > 0)
                                            ContextPattern(mainp, !dir,
over_met, ow, os, offsetOver); if ((over_met2) && (ow2 > 0 && os2 > 0))
                                            ContextPatternParallel(mainp, dir,
over_met2, ow2, os2, offsetOver);
                                        }
                                        pattern_separation= minSpacing * 10;
                                        origin[dir]= max_ur[dir] +
pattern_separation;

                                        delete mainp;
                                      }
                                    }
                                  }
                                }
                              }
                            }
                          }
                        }
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  set_max(MAX_UR);
  org[!dir]= MAX_UR[!dir] + _pattern_separation;
  fprintf(stdout, "%s         cnt=%d\n", patternName, cnt);
  return cnt;
}
*/
bool extPattern::SetPatternName()
{
  _totEnumerationCnt++;

  setName();
  char* pname = strdup(currentName);

  int n;
  if (nameHash->get(pname, n)) {
    free(pname);
    return true;
  }
  nameHash->add(pname, 1);
  return false;
}
extWirePattern* extPattern::MainPattern(float mw,
                                        float msL,
                                        float msR,
                                        float mwL,
                                        float mwR,
                                        float cwl,
                                        float csl,
                                        float cwr,
                                        float csr)
{
  bool dbg_p = true;

  extWirePattern* wp
      = new extWirePattern(this, dir, _minWidth, _minSpacing, opt);

  fprintf(patternLog, "Origin %d %d\n", _origin[0], _origin[1]);
  // if (msL>8 && msR==1.5)
  //  fprintf(stdout, " msL= %g Origin %g %g %s %s\n",msL, origin[0], origin[1],
  //  patternName, contextName);

  int n = wireCnt / 2;

  int jj;
  for (uint ii = n; ii > 1; ii--) {
    jj = wp->addCoords(cwl, csl, "ML", ii);
  }
  jj = wp->addCoords(mwL, csl, "ML1", -1);
  jj = wp->addCoords(mw, msL, "MC", -1);
  wp->setCenterXY(jj);
  jj = wp->addCoords(mwR, msR, "MR1", -1);
  for (uint ii = 2; ii <= n; ii++) {
    jj = wp->addCoords(cwl, csl, "MR", ii);
  }
  if (dbg_p)
    wp->printWires(patternLog);

  // wp->AddOrigin(origin);
  wp->AddOrigin_int(_origin);
  wp->printWires(patternLog, true);

  max_ur[0] = wp->last_trans(0);
  max_ur[1] = wp->last_trans(1);

  if (!printWiresDEF(wp, met))
    return NULL;

  return wp;
}
float extPattern::GetRoundedInt(float v, float mult, int units1)
{
  float v1 = lround(v * mult * units1);
  int r1 = (int) v1;
  if (r1 % 10 > 0) {
    r1 = 10 * (r1 / 10);
  }
  float r = (float) r1 / (float) units1;
  return r;
}
int extPattern::GetRoundedInt(int v, float mult, int units1)
{
  float v1 = lround(v * mult);
  int r1 = (int) v1;
  if (r1 % 10 > 0) {
    r1 = 10 * (r1 / 10);
  }
  int r = r1;
  return r;
}
int extPattern::getRoundedInt(float v, int units1)
{
  // return nearbyint(v*units1);

  // float v1= lround(v*units1);
  float v1 = v * units1;

  int r = (int) v1;
  if (r % 10 == 1) {
    r = 10 * (r / 10);
  }
  return r;
}
bool extPattern::printWiresDEF(extWirePattern* wp, int level)
{
  for (uint ii = 1; ii < wp->cnt; ii++) {
    int ll[2];
    int ur[2];
    for (uint jj = 0; jj < 2; jj++) {
      // ll[jj]= getRoundedInt(wp->trans_ll[jj][ii], units);
      // ur[jj]= getRoundedInt(wp->trans_ur[jj][ii], units);
      ll[jj] = wp->_trans_ll[jj][ii];
      ur[jj] = wp->_trans_ur[jj][ii];
    }
    wp->gen_trans_name(ii);
    char* netName = strdup(wp->tmp_pattern_name);
    if (createNetSingleWire(netName, ll, ur, level) == 0)
      return false;
  }
  return true;
}
void extWirePattern::printWires(FILE* fp, bool trans)
{
  if (trans)
    fprintf(fp, "\n");
  for (uint ii = 1; ii < cnt; ii++) {
    if (trans)
      print_trans2(fp, ii);
    else
      print(fp, ii);
  }
}
void extWirePattern::AddOrigin(float org[2])
{
  for (uint jj = 1; jj < cnt; jj++) {
    for (uint ii = 0; ii < 2; ii++) {
      trans_ll[ii][jj] = ll[ii][jj] + org[ii];
      trans_ur[ii][jj] = ur[ii][jj] + org[ii];
    }
  }
}
void extWirePattern::AddOrigin_int(int org[2])
{
  for (uint jj = 1; jj < cnt; jj++) {
    for (uint ii = 0; ii < 2; ii++) {
      _trans_ll[ii][jj] = ll[ii][jj] + org[ii];
      _trans_ur[ii][jj] = ur[ii][jj] + org[ii];
    }
  }
}
void extWirePattern::setCenterXY(int jj)
{
  _centerWireIndex = jj;
  for (uint ii = 0; ii < 2; ii++) {
    center_ll[ii] = ll[ii][jj];
    center_ur[ii] = ur[ii][jj];
  }
}
int extWirePattern::length(uint dir)
{
  int llen = ur[dir][cnt - 1] - ll[dir][1];
  return llen;
}
void extPattern::set_max(int ur[2])
{
  for (uint ii = 0; ii < 2; ii++) {
    if (ur[ii] < max_ur[ii])
      ur[ii] = max_ur[ii];
  }
}
int extPattern::max_last(extWirePattern* wp)
{
  for (uint ii = 0; ii < 2; ii++) {
    int xy = wp->last(ii);
    if (max_ur[ii] < xy)
      max_ur[ii] = xy;
  }
  return max_ur[dir];
}
int extWirePattern::last(uint dir)
{
  int xy = ur[dir][cnt - 1];
  return xy;
}
int extWirePattern::last_trans(uint dir)
{
  int xy = _trans_ur[dir][cnt - 1];
  return xy;
}
int extWirePattern::first(uint dir)
{
  int xy = ll[dir][1];
  return xy;
}
extWirePattern* extPattern::GetWireParttern(extPattern* main,
                                            uint dir,
                                            float mw,
                                            float ms,
                                            int met1,
                                            int& w,
                                            int& s)
{
  dbTechLayer* layer = db_net_util->getRoutingLayer()[met1];
  // int minW= layer->getMinWidth();
  int minW = layer->getWidth();
  // if (met1==6)
  // fprintf(stdout, "M6 getWidth= %d  getMinWidth=%d \n", layer->getWidth(),
  // layer->getMinWidth());

  int pitch = layer->getPitch();
  int minSpace = layer->getSpacing();
  int minS = pitch - minW;
  if (minS > minSpace)
    minS = minSpace;

  extWirePattern* wp = new extWirePattern(main, dir, minW, minS, opt);
  s = extPattern::GetRoundedInt(minS, ms, this->units);
  w = extPattern::GetRoundedInt(minW, mw, this->units);

  return wp;
}

int extPattern::ContextPattern(extWirePattern* main,
                               uint dir1,
                               int met1,
                               float mw,
                               float ms,
                               float offset)
{
  // offset not used
  // dir is direction where coordinates change
  // !dir coords don't change
  fprintf(patternLog, "\n");

  int s;
  int w;
  extWirePattern* wp = GetWireParttern(this, dir1, mw, ms, met1, w, s);

  int ll[2];
  int ur[2];
  ur[dir1] = -(s - wp->_minSpacing);

  wp->ur[dir1][0] = -(s - wp->_minSpacing);  // TO DELETE, not used
  wp->ll[!dir1][0] = w;                      // TO DELETE, not Used

  // Constant direction
  wp->_len = main->length(!dir1);
  ll[!dir1] = main->first(!dir1);
  ur[!dir1] = main->last(!dir1);

  // running direction
  int minXY = main->first(dir1);
  ll[dir1] = minXY - (w + s);
  ur[dir1] = ll[dir1] + w;

  int limit = main->last(dir1);

  int jj = 1;
  uint ii = 1;
  for (;; ii++) {
    // int next_xy= wp->ll[dir1][ii-1];
    // next_xy += w+s;
    ll[dir1] += w + s;
    ur[dir1] += w + s;

    // fprintf(stdout, "ContextPattern %d %d   %d %d\n", ll[0], ur[0], ll[1],
    // ur[1]);
    if (ur[dir1] > limit)
      break;

    // jj= wp->addCoords(mw, ms, "___cntx", ii, met1);
    wp->addCoords(ll, ur, "___cntx", jj, met1);
    jj++;

    // fprintf(stdout, "ContextPattern %d %d   %d %d --- \n", wp->ll[0][jj],
    // wp->ur[0][jj], wp->ll[1][jj], wp->ur[1][jj]); wp->print(patternLog, jj);
  }
  wp->printWires(patternLog);
  // wp->AddOrigin(origin);
  wp->AddOrigin_int(_origin);
  wp->printWires(patternLog, true);

  printWiresDEF(wp, met1);

  delete wp;
  return 0;
}
int extPattern::ContextPatternParallel(extWirePattern* main,
                                       uint dir1,
                                       int met1,
                                       float mw,
                                       float ms,
                                       float mid_offset)
{
  // At least one at mid+offeset will be created

  // Offset is a fraction of width
  // dir is same direction as main

  fprintf(patternLog, "\n");

  int s;
  int w;
  extWirePattern* wp = GetWireParttern(this, dir1, mw, ms, met1, w, s);

  float offs = mid_offset * minWidth;
  int f = (int) 1000 * offs;
  int n1 = f / 10;
  int off = n1 * 10;

  int ll[2];
  int ur[2];
  uint short_dir = dir1;
  uint long_dir = !dir1;

  // Constant direction
  wp->_len = main->length(long_dir);
  ll[long_dir] = main->first(long_dir);
  ur[long_dir] = main->last(long_dir);

  int mid_ll[2];
  int mid_ur[2];
  mid_ll[long_dir] = ll[long_dir];
  mid_ur[long_dir] = ur[long_dir];

  mid_ll[short_dir] = main->ll[short_dir][main->_centerWireIndex];
  mid_ll[short_dir] += off;
  mid_ur[short_dir] = mid_ll[short_dir] + w;

  int lo_limit = main->first(short_dir);
  int hi_limit = main->last(short_dir);

  ll[short_dir] = mid_ll[short_dir];
  ur[short_dir] = mid_ur[short_dir];

  uint ii = 1;
  for (;; ii++) {
    ur[short_dir] = ll[short_dir] - s;
    ll[short_dir] = ur[short_dir] - w;

    if (ll[short_dir] < lo_limit)
      break;

    wp->addCoords_temp(ii, ll, ur);
  }
  wp->cnt = 1;
  int jj = 1;
  for (int kk = ii - 1; kk > 0; kk--) {
    ll[short_dir] = wp->_trans_ll[short_dir][kk];
    ur[short_dir] = wp->_trans_ur[short_dir][kk];
    wp->addCoords(ll, ur, "___cntx", jj++, met1);
  }
  wp->addCoords(mid_ll, mid_ur, "___cntx", jj++, met1);

  ll[short_dir] = mid_ll[short_dir];
  ur[short_dir] = mid_ur[short_dir];
  for (;; jj++) {
    ll[short_dir] = ur[short_dir] + s;
    ur[short_dir] = ll[short_dir] + w;
    if (ur[short_dir] > hi_limit)
      break;

    wp->addCoords(ll, ur, "___cntx", jj, met1);
  }
  wp->printWires(patternLog);
  wp->AddOrigin_int(_origin);
  wp->printWires(patternLog, true);

  printWiresDEF(wp, met1);

  delete wp;
  return ii;
}
/*
int extPattern::ContextPatternParallel_save(extWirePattern *main, uint dir1, int
met1, float mw, float ms, float mid_offset)
{
  // Offset is a fraction of width
  // dir is same direction as main

  fprintf(patternLog, "\n");

  int s;
  int w;
  extWirePattern *wp= GetWireParttern(this, dir1, mw, ms, met1, w, s);

  float offs= mid_offset * minWidth;
  int f= (int) 1000*offs;
  int n1= f/10;
  int off= n1 * 10;

  wp->_len= main->length(dir1);
  int hi_xy= main->last(dir1);
  int lo_xy= main->first(dir1);
  int mid_xy= main->center_ll[dir1];

  mid_xy += off;

  int xy_lo= main->center_ll[!dir1];
  int xy_hi= main->center_ur[!dir1];

  int last_xy= mid_xy;
  uint ii = 0;
  for (; ; ii++) {
    int next_xy= last_xy - (w+s);
    // next_xy += w+s;

    if (next_xy<lo_xy)
      break;
      last_xy= next_xy;
  }
  int ll[2];
  int ur[2];
  ll[!dir]= xy_lo;
  ur[!dir]= xy_hi;

  uint jj= 1;
  for (; jj<=ii; jj++) {
    ll[dir]= last_xy;
    ur[dir]= last_xy + w;

    wp->addCoords(ll, ur, "___cntx", jj, met1);
    last_xy += w + s;
  }
  ll[dir1]= last_xy;
  ur[dir1]= last_xy + w;
  wp->addCoords(ll, ur, "___cntx", jj, met1 );
  jj++;

  last_xy += w + s;
  for (; ; jj++) {
    if (last_xy>hi_xy)
      break;

    ll[dir1]= last_xy;
    ur[dir1]= last_xy + w;
    wp->addCoords(ll, ur, "___cntx", jj, met1 );
    last_xy += w + s;
  }
  wp->printWires(patternLog);

     // wp->AddOrigin(origin);
  wp->AddOrigin_int(_origin);
  wp->printWires(patternLog, true);

  int xy= max_last(wp);
  printWiresDEF(wp, met1);
}
*/

void extWirePattern::print(FILE* fp, uint jj, float units)
{
  fprintf(fp,
          "%10.2f %10.2f  %10.2f %10.2f   %s___wire_%s\n",
          ll[0][jj] * units,
          ur[0][jj] * units,
          ll[1][jj] * units,
          ur[1][jj] * units,
          pattern->currentName,
          name[jj]);
}
void extWirePattern::gen_trans_name(uint jj)
{
  sprintf(tmp_pattern_name, "%s___%s", pattern->currentName, name[jj]);
}
void extWirePattern::print_trans(FILE* fp, uint jj)
{
  gen_trans_name(jj);
  fprintf(fp,
          "%10.2f %10.2f  %10.2f %10.2f   %s\n",
          trans_ll[0][jj],
          trans_ur[0][jj],
          trans_ll[1][jj],
          trans_ur[1][jj],
          tmp_pattern_name);
}
void extWirePattern::print_trans2(FILE* fp, uint jj, float units)
{
  gen_trans_name(jj);
  fprintf(fp,
          "%10.2f %10.2f  %10.2f %10.2f   %s\n",
          _trans_ll[0][jj] * units,
          _trans_ur[0][jj] * units,
          _trans_ll[1][jj] * units,
          _trans_ur[1][jj] * units,
          tmp_pattern_name);
}
void extPattern::setName()
{
  sprintf(currentName, "%s%s%s", patternName, targetMetName, contextName);
}
extWirePattern::extWirePattern(extPattern* p,
                               uint d,
                               float w,
                               float s,
                               const PatternOptions& opt)
{
  pattern = p;
  _minWidth = w;
  _minSpacing = s;
  _len = _minWidth * opt.len;  // TODO
  dir = d;

  ll[d][0] = 0;
  ur[d][0] = -_minSpacing;
  ll[!d][0] = 0;
  ur[!d][0] = 0;

  cnt = 1;
}
uint extWirePattern::addCoords(float mw,
                               float ms,
                               const char* buff,
                               int ii,
                               int met)
{
  uint d = dir;
  uint jj = cnt;
  if (jj >= 1000)
    return jj;

  // float s = extPattern::GetRoundedInt(minSpacing, ms, pattern->units);
  // float w = extPattern::GetRoundedInt(minWidth, mw, pattern->units);
  int s = extPattern::GetRoundedInt(_minSpacing, ms, pattern->units);
  int w = extPattern::GetRoundedInt(_minWidth, mw, pattern->units);
  ll[d][jj] = ur[d][jj - 1] + s;
  ur[d][jj] = ll[d][jj] + w;

  ll[!d][jj] = ll[!d][jj - 1];
  ur[!d][jj] = ll[!d][jj] + _len;

  if (ii > 0) {
    if (met < 0)
      sprintf(name[jj], "%s%d", buff, ii);
    else
      sprintf(name[jj], "%sM%d_%d", buff, met, ii);
  } else
    sprintf(name[jj], "%s", buff);

  cnt++;
  return jj;
}
uint extWirePattern::addCoords(int LL[2],
                               int UR[2],
                               const char* buff,
                               int jj,
                               int met)
{
  for (uint ii = 0; ii < 2; ii++) {
    ll[ii][jj] = LL[ii];
    ur[ii][jj] = UR[ii];
  }
  sprintf(name[jj], "%sM%d_%d", buff, met, jj);
  cnt++;
  return jj;
}
void extWirePattern::addCoords_temp(int jj, int LL[2], int UR[2])
{
  for (uint ii = 0; ii < 2; ii++) {
    _trans_ll[ii][jj] = LL[ii];
    _trans_ur[ii][jj] = UR[ii];
  }
}

uint extPattern::createNetSingleWire(char* netName,
                                     int ll[2],
                                     int ur[2],
                                     int level)
{
  dbNet* net = db_net_util->createNetSingleWire(
      netName, ll[0], ll[1], ur[0], ur[1], level);
  if (net == NULL)
    return 0;

  bool rename_all_bterms = true;
  if (rename_all_bterms) {
    RenameAllBterms(net);
  } else {
    dbBTerm* in1 = net->get1stBTerm();
    if (in1 != NULL) {
      in1->rename(net->getConstName());
    }
    RenameBterm1stInput(net);
  }
  uint netId = net->getId();
  return netId;
}
bool extPattern::RenameBterm1stInput(dbNet* net)
{
  const char* bterm1 = NULL;
  dbSet<dbBTerm> bterms = net->getBTerms();
  for (dbBTerm* bterm : bterms) {
    bterm1 = bterm->getConstName();
    if (bterm->getIoType() != dbIoType::INPUT)  // TODO create a flag
      continue;

    // printf("BTERM %s %s\n", bterm1, bterm->getIoType().getString());

    char buff[32];
    // sprintf(buff, "B%d_BL", net->getId());
    sprintf(buff, "B%d", bterm->getId());

    if (bterm->rename(buff)) {
      bterm1 = bterm->getConstName();
      // printf("BTERM %s %s\n", bterm1, bterm->getIoType().getString());
      return true;
    }
    fprintf(stdout, "Cannot rename %s to %s\n", bterm1, buff);
    return false;
  }

  return false;
}
bool extPattern::RenameAllBterms(dbNet* net)
{
  const char* bterm1 = NULL;
  dbSet<dbBTerm> bterms = net->getBTerms();
  for (dbBTerm* bterm : bterms) {
    bterm1 = bterm->getConstName();
    const char* io = "_in";
    if (bterm->getIoType() != dbIoType::INPUT)
      io = "_out";

    // printf("BTERM %s %s\n", bterm1, bterm->getIoType().getString());

    char buff[32];
    sprintf(buff, "B%d%s", net->getId(), io);
    if (bterm->rename(buff)) {
      bterm1 = bterm->getConstName();
    } else {
      fprintf(stdout, "Cannot rename %s to %s\n", bterm1, buff);
    }
  }
  return true;
}

std::vector<float> extPattern::getMultipliers(const char* s)
{
  std::vector<float> table;
  char del = ',';
  char* tmp = strdup(s);
  uint jj = 0;
  uint ii = 0;
  if (s[0] == del)
    ii = 1;
  for (; s[ii] != '\0'; ii++) {
    if (s[ii] == ' ')
      continue;

    if (s[ii] != del) {
      tmp[jj++] = s[ii];
      continue;
    }
    if (s[ii] == del) {
      tmp[jj++] = '\0';
      float v = atof(tmp);
      table.push_back(v);

      jj = 0;
    }
  }
  if (jj > 0) {
    tmp[jj] = '\0';
    float v = atof(tmp);
    table.push_back(v);
  }
  return table;
}
FILE* extPattern::OpenLog(int met, const char* postfix)
{
  char patternLog[20];
  sprintf(patternLog, "m%d%s", met, postfix);
  FILE* fp = fopen(patternLog, "w");
  if (fp == NULL) {
    fprintf(stderr, "Cannot open file %s for w\n", patternLog);
    exit(0);
  }
  return fp;
}

}  // namespace rcx
