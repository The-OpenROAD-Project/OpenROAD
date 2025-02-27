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

#include "parse.h"
#include "rcx/extRCap.h"
#include "rcx/extSpef.h"
#include "rcx/extprocess.h"
#include "utl/Logger.h"

namespace rcx {

using utl::RCX;

using namespace odb;

// dkf 12272023
extViaModel* extMetRCTable::addViaModel(char* name,
                                        double R,
                                        uint cCnt,
                                        uint dx,
                                        uint dy,
                                        uint top,
                                        uint bot)
{
  int n1;
  if (_viaModelHash.get(name, n1)) {
    if (n1 < 0)
      return NULL;
    extViaModel* v = _viaModel.get(n1);
    return v;
  }

  extViaModel* v = new extViaModel(name, R, cCnt, dx, dy, top, bot);
  int n = _viaModel.add(v);
  _viaModelHash.add(name, n);

  return v;
}
extViaModel* extMetRCTable::getViaModel(char* name)
{
  int n1;
  if (_viaModelHash.get(name, n1)) {
    if (n1 < 0)
      return NULL;
    extViaModel* v = _viaModel.get(n1);
    return v;
  }
  return NULL;
}
void extViaModel::printViaRule(FILE* fp)
{
  fprintf(fp,
          "M%d M%d C%d dx%d dy%d  %g %s\n",
          _botMet,
          _topMet,
          _cutCnt,
          _dx,
          _dy,
          _res,
          _viaName);
}

void extViaModel::writeViaRule(FILE* fp)
{
  fprintf(fp,
          "%g %s cuts %d  TopMetal %d BotMetal %d dx %d dy %d\n",
          _res,
          _viaName,
          _cutCnt,
          _topMet,
          _botMet,
          _dx,
          _dy);
}
void extMetRCTable::printViaModels()
{
  for (uint ii = 0; ii < _viaModel.getCnt(); ii++)
    _viaModel.get(ii)->printViaRule(stdout);
}
void extMetRCTable::writeViaRes(FILE* fp)
{
  fprintf(fp, "\nVIARES %d Default LEF Vias:\n", _viaModel.getCnt());
  for (uint ii = 0; ii < _viaModel.getCnt(); ii++)
    _viaModel.get(ii)->writeViaRule(fp);

  fprintf(fp, "END VIARES\n\n");
}
bool extMetRCTable::GetViaRes(Ath__parser* p,
                              Ath__parser* w,
                              dbNet* net,
                              FILE* logFP)
{
  // via pattern: V2.W2.M5.M6.DX520.DY1320.C2.V56_1x2_VH_S

  const char* netName = net->getConstName();
  int vCnt = w->mkWords(netName, ".-");
  // if (vCnt<=1)
  //  vCnt = w->mkWords(netName, "-");

  int viaWireNum = w->getInt(vCnt - 1, 1);
  int bot_met = w->getInt(1, 1);
  int top_met = w->getInt(2, 1);
  int dx = w->getInt(3, 2);
  int dy = w->getInt(4, 2);
  int cutCnt = w->getInt(5, 1);

  const char* viaName = w->get(vCnt - 2);
  double via_res = net->getTotalResistance();

  if (viaWireNum == 1) {
    fprintf(logFP, "WV %g  %s %s\n", via_res, viaName, netName);
    addViaModel((char*) viaName, via_res, cutCnt, dx, dy, top_met, bot_met);
    return true;
  }
  extViaModel* viaModel = getViaModel((char*) viaName);
  if (viaModel == NULL) {
    fprintf(
        stderr, "not defined viaModel: %s -- netName %s \n", viaName, netName);
    return false;
  }
  double via_res_1 = viaModel->_res;
  double vres = via_res_1 - via_res;

  viaModel->_res = vres;

  fprintf(logFP,
          "V  %g WV %g W %g  %s   %s\n",
          vres,
          via_res_1,
          via_res,
          viaName,
          netName);
  return true;
}
bool extMetRCTable::ReadRules(Ath__parser* p)
{
  // 10.1442 V12_VV cuts 1  TopMetal 2 BotMetal 1 dx 200 dy 320
  while (p->parseNextLine() > 0) {
    if (p->isKeyword(0, "END") && p->isKeyword(1, "VIARES"))
      break;

    char* viaName = p->get(1);
    double via_res = p->getDouble(0);
    int bot_met = p->getInt(7);
    int top_met = p->getInt(5);
    int dx = p->getInt(9);
    int dy = p->getInt(11);
    int cutCnt = p->getInt(3);

    addViaModel((char*) viaName, via_res, cutCnt, dx, dy, top_met, bot_met);
  }
  return true;
}
uint extMetRCTable::SetDefaultTechViaRes(dbTech* tech, bool dbg)
{
  uint cnt = 0;
  dbSet<dbTechVia> vias = tech->getVias();
  dbSet<dbTechVia>::iterator vitr;

  for (vitr = vias.begin(); vitr != vias.end(); ++vitr) {
    dbTechVia* via = *vitr;

    if (via->getNonDefaultRule() != NULL)
      continue;
    if (via->getViaGenerateRule() != NULL)
      continue;

    const char* viaName = via->getConstName();
    cnt++;

    extViaModel* viaModel = getViaModel((char*) viaName);
    if (viaModel == NULL)
      continue;

    via->setResistance(viaModel->_res);
    if (dbg)
      viaModel->printViaRule(stdout);
  }
  return cnt;
}
bool extMetRCTable::SkipPattern(Ath__parser* p, dbNet* net, FILE* logFP)
{
  // This will go away completely when new rules become default
  // This is to track compatibility between original Def Patterns and latest
  // dkf 12302023 -- NOTE Original Patterns start with:
  //                 O6_ U6_ OU6_ DU6_ R6_

  // const char *netName = net->getConstName();
  uint targetWire = 0;

  if (p->getFirstChar() == 'D') {
    if (p->get(0)[1] == 'O')
      return true;
    targetWire = p->getInt(0, 2);
    if (targetWire < 3)
      return true;
    else
      return false;
  }
  if (p->getFirstChar() == 'U' || p->getFirstChar() == 'R') {
    targetWire = p->getInt(0, 1);
  } else {
    targetWire = p->getInt(0, 1);
    if (targetWire == 0)
      targetWire = p->getInt(0, 2);
  }
  /*
  else if (p->getFirstChar() == 'D')
  {
      targetWire = p->getInt(0, 1);
  }
  else if (p->getFirstChar() == 'R')
  {
      targetWire = p->getInt(0, 1);
  } */
  if (targetWire < 5)
    return true;
  //  if (targetWire < 6)
  //    return true;
  return false;
}
}  // namespace rcx
