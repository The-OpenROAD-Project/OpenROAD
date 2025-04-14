// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include <algorithm>

#include "odb/db.h"
#include "rcx/extRCap.h"
#include "rcx/extSpef.h"

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
  if (rc) {
    _minCapTable[ii][jj] = 2 * rc->getTotalCap();
    _minResTable[ii][jj] = 2 * rc->getRes();
  } else {
    _minCapTable[ii][jj] = 0;
    _minResTable[ii][jj] = 0;
  }
}

void extMain::setMaxRC(uint ii, uint jj, extDistRC* rc)
{
  if (rc) {
    _maxCapTable[ii][jj] = 2 * rc->getTotalCap();
    _maxResTable[ii][jj] = 2 * rc->getRes();
  } else {
    _maxCapTable[ii][jj] = 0;
    _maxResTable[ii][jj] = 0;
  }
}

extDistRC* extRCModel::getMinRC(int met, int width)
{
  if (met >= _layerCnt) {
    return nullptr;
  }
  extDistRC* rc
      = _modelTable[_tmpDataRate]->_capOver[met]->getFringeRC(0, width);
  return rc;
}

extDistRC* extRCModel::getMaxRC(int met, int width, int dist)
{
  if (met >= _layerCnt) {
    return nullptr;
  }
  extDistRC* rc = nullptr;
  if (met == _layerCnt - 1) {  // over
    rc = _modelTable[_tmpDataRate]->_capOver[met]->getFringeRC(0, width);
  } else if (met == 1) {  // over
    rc = getUnderRC(met, 2, width, dist);
  } else {
    rc = getOverUnderRC(met, met - 1, met + 1, width, dist);
  }
  return rc;
}

uint extMain::calcMinMaxRC()
{
  _currentModel = getRCmodel(0);

  uint cnt = 0;

  for (odb::dbTechLayer* layer : _tech->getLayers()) {
    if (layer->getRoutingLevel() == 0) {
      continue;
    }

    const int met = layer->getRoutingLevel();
    const int width = layer->getWidth();
    int dist = layer->getSpacing();
    if (dist == 0) {
      dist = layer->getPitch() - layer->getWidth();
    }

    for (uint jj = 0; jj < _modelMap.getCnt(); jj++) {
      resetMinMaxRC(met, jj);

      extDistRC* rcMin = _currentModel->getMinRC(met, width);
      extDistRC* rcMax = _currentModel->getMaxRC(met, width, dist);

      setMinRC(met, jj, rcMin);
      setMaxRC(met, jj, rcMax);
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
  _tmpLenStats.clear();

  odb::dbWire* wire = net->getWire();
  if (wire == nullptr) {
    return 0;
  }

  odb::dbWireShapeItr shapes;
  odb::dbShape s;
  for (shapes.begin(wire); shapes.next(s);) {
    if (s.isVia()) {
      via_cnt++;

      odb::dbTechVia* tvia = s.getTechVia();
      if (tvia != nullptr) {
        double res = tvia->getResistance();
        via_res += res;
      } else {
        odb::dbVia* bvia = s.getVia();
        if (bvia != nullptr) {
          double res = getViaResistance_b(bvia, net);
          via_res += res;
        }
      }
      continue;
    }
    cnt++;
    uint met = s.getTechLayer()->getRoutingLevel();
    int width = s.getTechLayer()->getWidth();

    odb::Rect r = s.getBox();
    int dx = r.xMax() - r.xMin();
    int dy = r.yMax() - r.yMin();

    int len = 0;
    if (width == dx) {
      len = dy;
    } else if (width == dy) {
      len = dx;
    } else {
      len = std::max(dx, dy);
    }
    char buf[64];
    sprintf(buf, ",M%d:%d", met, len);
    _tmpLenStats += buf;
    wlen += len;

    min_res += len * _minResTable[met][corner];
    max_res += len * _maxResTable[met][corner];

    min_cap += len * _minCapTable[met][corner];
    max_cap += len * _maxCapTable[met][corner];
  }
  return cnt;
}

}  // namespace rcx
