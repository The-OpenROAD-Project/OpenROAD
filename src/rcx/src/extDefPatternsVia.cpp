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

#include "rcx/extRCap.h"
#include "rcx/extRulesPattern.h"
#include "rcx/extSpef.h"
#include "rcx/extprocess.h"

#ifdef _WIN32
#include "direct.h"
#endif

#include <map>
#include <vector>

#include "utl/Logger.h"

namespace rcx {

using utl::RCX;

using namespace odb;

using rcx::extRulesPat;

uint extRulesPat::setLayerInfoVia(dbTechLayer* layer,
                                  uint met,
                                  bool startPatternGroup)
{
  _layer = layer;
  _met = met;
  _minSpace = getMinWidthSpacing(layer, _minWidth);

  _len = _option_len * _minWidth / 1000;

  _dir = 1;
  _short_dir = _dir;
  _long_dir = !_dir;

  // UpdateOrigin_start(_met);
  _sepGridCnt = 10;
  _patternSep = _sepGridCnt * (_minWidth + _minSpace);
  if (!startPatternGroup)
    _origin[1] += _ur_last[1] + _patternSep;  // Grow vertically
  else
    _origin[1] += _ur_last[1];

  _origin[0] = _init_origin[0];

  PrintOrigin(stdout, _origin, met, "Pattern Start");

  _ll[1] = _ur_last[1] + _patternSep;
  _ll[0] = 0;

  _ur[_short_dir] = _ll[_short_dir];
  _ur[_long_dir]
      = _ll[_long_dir] + _len * _minWidth / 1000;  // _len is in nm per ext.ti

  for (uint ii = 0; ii < _spaceCnt; ii++) {
    _target_width[ii] = _minWidth * _sMult[ii] / 1000;
    _target_spacing[ii] = _minSpace * _sMult[ii] / 1000;
  }
  return _patternSep;
}

uint extRulesPat::GetViaCutCount(dbTechVia* tvia)
{
  dbSet<dbBox> boxes = tvia->getBoxes();
  dbSet<dbBox>::iterator bitr;
  uint cnt = 0;
  for (bitr = boxes.begin(); bitr != boxes.end(); ++bitr) {
    dbBox* box = *bitr;
    dbTechLayer* layer1 = box->getTechLayer();
    if (layer1->getType() == dbTechLayerType::CUT)
      cnt++;
  }
  return cnt;
}
double extRulesPat::GetViaArea(dbTechVia* via)
{
  dbBox* bbox = via->getBBox();
  uint vdx = bbox->getDX();
  uint vdy = bbox->getDY();

  double area = 0.001 * vdx * 0.001 * vdy;
  return area;
}

uint extRulesPat::CreatePatternVia(dbTechVia* via,
                                   uint widthIndex,
                                   uint spaceIndex,
                                   uint wcnt)
{
  const char* viaName = via->getConstName();
  uint cutCnt = GetViaCutCount(via);
  // double area= GetViaArea(via);
  dbBox* bbox = via->getBBox();
  uint vdx = bbox->getDX();
  uint vdy = bbox->getDY();

  uint w = _target_width[widthIndex];
  uint s = _target_spacing[spaceIndex];
  char buf[100];
  // dkf 04032024 sprintf(buf, "M%d.M%d.DX%d.DY%d.C%d.%s", _underMet, _met, vdx,
  // vdy, cutCnt, viaName);
  sprintf(buf,
          "M%d-M%d-DX%d-DY%d-C%d-%s",
          _underMet,
          _met,
          vdx,
          vdy,
          cutCnt,
          viaName);

  Init(s);

  uint ii = 1;
  _LL[ii][_long_dir] = 0;
  _UR[ii][_long_dir] = _len;
  _LL[ii][_short_dir] = _UR[ii - 1][_short_dir] + s;
  _UR[ii][_short_dir] = _LL[ii][_short_dir] + w;

  // dkf 04032024 sprintf(_patName[ii], "V2.%s.W%d", buf, ii);
  sprintf(_patName[ii], "V2-%s-W%d", buf, ii);

  ii++;
  _LL[ii][_long_dir] = 0;
  _UR[ii][_long_dir] = _len;
  _LL[ii][_short_dir] = _UR[ii - 1][_short_dir] + s;
  _UR[ii][_short_dir] = _LL[ii][_short_dir] + w;

  sprintf(_patName[ii], "V2-%s-W%d", buf, ii);

  _lineCnt = ii + 1;

  // WriteDB(_dir, _met, _layer);
  WriteDBWireVia(1, _dir, via);
  WriteDB(2, _dir, _met, _layer, _def_fp);

  Print(stdout);

  return ii;  // cnt of main pattern;
}
void extRulesPat::WriteDBWireVia(uint jj, uint dir, dbTechVia* via)
{
  WriteWire(_def_fp, _LL[jj], _UR[jj], _patName[jj]);
  // uint d= dir>0 ? 0 :1;
  int width = _UR[jj][dir] - _LL[jj][dir];
  int ll[2] = {_LL[jj][0] + _origin[0], _LL[jj][1] + _origin[1]};
  int ur[2] = {_UR[jj][0] + _origin[0], _UR[jj][1] + _origin[1]};

  WriteWire(_def_fp, ll, ur, _patName[jj]);

  createNetSingleWireAndVia(_patName[jj], ll, ur, width, dir == 0, via);
}
uint extRCModel::ViaRulePat(extMainOptions* opt,
                            int len,
                            int origin[2],
                            int UR[2],
                            bool res,
                            bool diag,
                            uint overDist)
{
  bool dbg = false;
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
  dbSet<dbTechVia> vias = opt->_tech->getVias();
  dbSet<dbTechVia>::iterator vitr;

  bool startPatterns = true;
  for (vitr = vias.begin(); vitr != vias.end(); ++vitr) {
    dbTechVia* via = *vitr;

    if (via->getNonDefaultRule() != NULL)
      continue;
    if (via->getViaGenerateRule() != NULL)
      continue;

    const char* viaName = via->getConstName();
    cnt++;

    dbTechLayer* top_layer = via->getTopLayer();
    if (top_layer == NULL)
      continue;
    dbTechLayer* bot_layer = via->getBottomLayer();
    if (bot_layer == NULL)
      continue;
    uint met = top_layer->getRoutingLevel();
    int underMet = bot_layer->getRoutingLevel();
    if (dbg) {
      fprintf(stdout,
              "\n%s: L%d %s - L%d %s\n",
              viaName,
              underMet,
              bot_layer->getConstName(),
              met,
              top_layer->getConstName());
    }

    p->setLayerInfoVia(top_layer, met, startPatterns);

    if (startPatterns)
      startPatterns = false;

    p->setMets(underMet, bot_layer, -1, NULL);

    cnt += p->CreatePatternVia(via, 0, 0, 1);

    p->UpdateBBox();
  }
  logger_->info(RCX, 56, "Finished {} bench measurements for pattern VIA", cnt);
  origin[1] = 0;  // reset for next family of patterns
  origin[0] = p->GetOrigin_end(p->_ur_last);

  opt->_ur[1] = opt->_ur[1];  // required for the DIE_AREA in DEF

  return cnt;
}
dbNet* extRulesPat::createNetSingleWireAndVia(const char* netName,
                                              int ll[2],
                                              int ur[2],
                                              uint width,
                                              bool vertical,
                                              dbTechVia* via)
{
  dbTechLayer* bot_layer = via->getBottomLayer();
  dbTechLayer* top_layer = via->getTopLayer();

  dbNet* net = dbNet::create(_block, netName);
  if (net == NULL) {
    fprintf(stdout, "Cannot create net %s, duplicate\n", netName);
    return NULL;
  }
  net->setSigType(dbSigType::SIGNAL);
  dbBTerm* hiBTerm = extRulesPat::createBterm(
      false, net, ll, ur, "_HI", bot_layer, width, vertical, true /*input*/);
  dbBTerm* loBTerm = extRulesPat::createBterm(
      true, net, ll, ur, "_LO", top_layer, width, vertical, false /*input*/);
  if ((loBTerm == NULL) || (hiBTerm == NULL)) {
    dbNet::destroy(net);
    fprintf(stdout,
            "Cannot create net %s, because failed to create bterms\n",
            netName);
    return NULL;
  }
  dbWireEncoder encoder;
  encoder.begin(dbWire::create(net));

  encoder.newPath(top_layer, dbWireType::ROUTED);
  encoder.addPoint(ll[0], ll[1], 0);
  encoder.addBTerm(loBTerm);

  if (vertical)
    encoder.addPoint(ll[0], ur[1], 0);
  else
    encoder.addPoint(ur[0], ll[1], 0);

  encoder.addTechVia(via);
  encoder.addBTerm(hiBTerm);
  encoder.end();

  return net;
}

}  // namespace rcx
