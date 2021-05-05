///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2019, Nefelus Inc
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

#include "darr.h"
#include "db.h"
#include "rcx/extRCap.h"
#include "rcx/extSpef.h"
#include "rcx/exttree.h"

namespace rcx {

void extMain::resetMinMaxRC(uint ii, uint jj)
{
  _minCapTable[ii][jj] = 0;
  _maxCapTable[ii][jj] = 0;
  _minResTable[ii][jj] = 0;
  _maxResTable[ii][jj] = 0;
}
void extMain::setMinRC(uint ii, uint jj, extDistRC* rc)
{
  _minCapTable[ii][jj] = 2 * rc->getTotalCap();
  _minResTable[ii][jj] = 2 * rc->getRes();
}
void extMain::setMaxRC(uint ii, uint jj, extDistRC* rc)
{
  _maxCapTable[ii][jj] = 2 * rc->getTotalCap();
  _maxResTable[ii][jj] = 2 * rc->getRes();
}
extDistRC* extRCModel::getMinRC(int met, int width)
{
  if (met >= _layerCnt)
    return NULL;

  extMeasure m;
  m._met = met;
  m._underMet = 0;
  m._overMet = 0;
  m._width = width;

  return getOverFringeRC(&m);
}
extDistRC* extRCModel::getMaxRC(int met, int width, int dist)
{
  if (met >= _layerCnt)
    return NULL;

  extMeasure m;
  m._met = met;
  m._width = width;
  m._dist = dist;

  m._underMet = met - 1;
  m._overMet = met + 1;

  extDistRC* rc = NULL;
  if (met == _layerCnt - 1) {  // over
    m._overMet = 0;
    rc = getOverFringeRC(&m);
  } else if (met == 1) {  // over
    m._overMet = 2;
    rc = getUnderRC(&m);
  } else {
    rc = getOverUnderRC(&m);
  }
  return rc;
}
void extDistRC::debugRC(const char* debugWord,
                        const char* from,
                        int width,
                        int level)
{
  char tmp[32];
  sprintf(tmp, " ");
  if (level > 0)
    sprintf(tmp, "%d", level);
  if (width > 0)
    sprintf(tmp, "%s %d", tmp, width);

  // debug(debugWord, "C", "%s: %s, tC %g  CC %g F %g D %g   R %g  Sep %d\n",
  //		from, tmp, _coupling+_fringe+_diag, _coupling,  _fringe, _diag,
  //_res, _sep);
}
uint extMain::calcMinMaxRC()
{
  uint cornerCnt = _modelTable->getCnt();
  if (cornerCnt == 0)
    cornerCnt = 1;

  _currentModel = getRCmodel(0);

  odb::dbSet<odb::dbTechLayer> layers = _tech->getLayers();
  odb::dbSet<odb::dbTechLayer>::iterator itr;

  uint cnt = 0;
  for (itr = layers.begin(); itr != layers.end(); ++itr) {
    odb::dbTechLayer* layer = *itr;
    odb::dbTechLayerType type = layer->getType();

    if (type.getValue() != odb::dbTechLayerType::ROUTING)
      continue;

    int met = layer->getRoutingLevel();
    int width = layer->getWidth();
    int dist = layer->getSpacing();
    if (dist == 0)
      dist = layer->getPitch() - layer->getWidth();

    for (uint jj = 0; jj < _modelMap.getCnt(); jj++) {
      uint modelIndex = _modelMap.get(jj);
      resetMinMaxRC(met, jj);

      extDistRC* rcMin = _currentModel->getMinRC(met, width);
      extDistRC* rcMax = _currentModel->getMaxRC(met, width, dist);

      setMinRC(met, jj, rcMin);
      setMaxRC(met, jj, rcMax);

      rcMin->debugRC("EXT_STATS", "MinRC", width, met);
      rcMax->debugRC("EXT_STATS", "MaxRC", width, met);
    }
    cnt++;
  }
  return cnt;
}
uint extMain::getExtStats(odb::dbNet* net,
                          uint corner,
                          int& wlen,
                          double& min_cap,
                          double& max_cap,
                          double& min_res,
                          double& max_res,
                          double& via_res,
                          uint& via_cnt)
{
  min_cap = 0;
  max_cap = 0;
  min_res = 0;
  max_res = 0;
  via_cnt = 0;
  via_res = 0;
  wlen = 0;
  uint cnt = 0;
  sprintf(_tmpLenStats, "");

  odb::dbWire* wire = net->getWire();
  if (wire == NULL)
    return 0;

  odb::dbWireShapeItr shapes;
  odb::dbShape s;
  for (shapes.begin(wire); shapes.next(s);) {
    //		uint level= 0;

    int shapeId = shapes.getShapeId();
    if (s.isVia()) {
      // if (!_skip_via_wires)
      //    continue;
      via_cnt++;

      odb::dbTechVia* tvia = s.getTechVia();
      if (tvia != NULL) {
        double res = tvia->getResistance();
        via_res += res;
      } else {
        odb::dbVia* bvia = s.getVia();
        if (bvia != NULL) {
          double res = getViaResistance_b(bvia, net);
          via_res += res;
        }
      }
      continue;
    }
    cnt++;
    uint met = s.getTechLayer()->getRoutingLevel();
    int width = s.getTechLayer()->getWidth();

    odb::Rect r;
    s.getBox(r);
    int dx = r.xMax() - r.xMin();
    int dy = r.yMax() - r.yMin();

    int len = 0;
    if (width == dx)
      len = dy;
    else if (width == dy)
      len = dx;
    else {
      len = dx;
      if (dy > dx)
        len = dy;
    }
    sprintf(_tmpLenStats, "%s,M%d:%d", _tmpLenStats, met, len);
    wlen += len;

    min_res += len * _minResTable[met][corner];
    max_res += len * _maxResTable[met][corner];

    min_cap += len * _minCapTable[met][corner];
    max_cap += len * _maxCapTable[met][corner];
  }
  return cnt;
}

}  // namespace rcx
