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

#include "rcx/extRCap.h"
#include "utl/Logger.h"

namespace rcx {

using utl::RCX;

uint extMain::allocateOverUnderMaps(uint layerCnt)
{
  uint cnt = layerCnt + 1;
  _overUnderPlaneLayerMap = new uint*[layerCnt + 1];
  uint ii;
  for (ii = 1; ii < layerCnt + 1; ii++) {
    _overUnderPlaneLayerMap[ii] = new uint[layerCnt + 1];
    for (uint jj = 1; jj < layerCnt + 1; jj++) {
      _overUnderPlaneLayerMap[ii][jj] = 0;
    }
  }
  for (ii = 2; ii < layerCnt + 1; ii += 2) {
    uint mUnder = ii - 1;

    uint mOver = ii + 1;
    if (mOver > layerCnt)
      break;

    _overUnderPlaneLayerMap[mUnder][mOver] = cnt;
    cnt++;
  }
  for (ii = 3; ii < layerCnt + 1; ii += 2) {
    uint mUnder = ii - 1;

    uint mOver = ii + 1;
    if (mOver > layerCnt)
      break;

    _overUnderPlaneLayerMap[mUnder][mOver] = cnt;
    cnt++;
  }
  return cnt;
}
uint extMain::initPlanesNew(uint planeCnt, odb::Rect* bb)
{
  _geomSeq->setSlices(planeCnt);

  // odb::Rect rectTable[15];
  // for (uint k= 0; k<15; k++)
  //	rectTable[k].reset( INT_MAX, INT_MAX, INT_MIN, INT_MIN );
  // getBboxPerLayer(rectTable);

  odb::Rect maxRect;
  _block->getDieArea(maxRect);

  if (bb != NULL) {
    maxRect = *bb;
    logger_->info(RCX,
                  183,
                  "Init planes area: {} {}  {} {}",
                  maxRect.xMin(),
                  maxRect.yMin(),
                  maxRect.xMax(),
                  maxRect.yMax());
  }

  odb::dbSet<odb::dbTechLayer> layers = _tech->getLayers();
  odb::dbSet<odb::dbTechLayer>::iterator litr;
  odb::dbTechLayer* layer;
  for (litr = layers.begin(); litr != layers.end(); ++litr) {
    layer = *litr;
    if (layer->getType() != odb::dbTechLayerType::ROUTING)
      continue;

    uint level = layer->getRoutingLevel();
    uint pp = layer->getPitch();
    uint minWidth = layer->getWidth();

    odb::Rect r = maxRect;
    //_geomSeq->configureSlice(level, pp, pp, r.xMin(), r.yMin(), r.xMax(),
    // r.yMax());

    // odb::Rect r= rectTable[level];

    if (layer->getDirection() == odb::dbTechLayerDir::HORIZONTAL)
      _geomSeq->configureSlice(
          level, minWidth, pp, r.xMin(), r.yMin(), r.xMax(), r.yMax());
    else
      _geomSeq->configureSlice(
          level, pp, minWidth, r.xMin(), r.yMin(), r.xMax(), r.yMax());
  }
  return planeCnt + 1;
}
uint extMain::initPlanes(uint dir,
                         uint layerCnt,
                         uint* pitchTable,
                         uint* widthTable,
                         int* ll,
                         int* ur)
{
  if (_geomSeq != NULL)
    delete _geomSeq;
  _geomSeq = new odb::gs;

  _geomSeq->setSlices(layerCnt);

  for (uint ii = 1; ii < layerCnt; ii++) {
    if (dir > 0)  // horizontal
      _geomSeq->configureSlice(
          ii, widthTable[ii], pitchTable[ii], ll[0], ll[1], ur[0], ur[1]);
    else {
      if (!_rotatedGs)
        _geomSeq->configureSlice(
            ii, pitchTable[ii], widthTable[ii], ll[0], ll[1], ur[0], ur[1]);
      else
        _geomSeq->configureSlice(
            ii, widthTable[ii], pitchTable[ii], ll[1], ll[0], ur[1], ur[0]);
    }
  }
  return layerCnt;
}
uint extMain::initPlanes(uint layerCnt, odb::Rect* bb)
{
  // logger_->info(RCX, 0, "Initializing Extraction search DB ... ");

  if (_geomSeq)
    delete _geomSeq;
  _geomSeq = new odb::gs();

  if (layerCnt == 0)
    layerCnt = getExtLayerCnt(_tech);

  //	uint planeCnt= layerCnt+1;

  _overUnderPlaneLayerMap = NULL;

  // if (! _diagFlow)
  // 	planeCnt= allocateOverUnderMaps(layerCnt);

#ifdef GS_OLD
  return initPlanesOld(planeCnt);
#else
  return initPlanesNew(layerCnt, bb);
#endif
}
uint extMain::makeIntersectPlanes(uint layerCnt)
{
  if (_geomSeq == NULL)
    return 0;
  if (_overUnderPlaneLayerMap == NULL)
    return 0;

  if (layerCnt == 0) {
    layerCnt = getExtLayerCnt(_tech);
  }

  uint cnt = 0;
  uint ii;
  for (ii = 2; ii < layerCnt + 1; ii += 2) {
    uint mUnder = ii - 1;

    uint mOver = ii + 1;
    if (mOver > layerCnt)
      break;

    uint planeIndex = _overUnderPlaneLayerMap[mUnder][mOver];
    _geomSeq->intersect_rows(mUnder, mOver, planeIndex);
    cnt++;
  }
  for (ii = 3; ii < layerCnt + 1; ii += 2) {
    uint mUnder = ii - 1;

    uint mOver = ii + 1;
    if (mOver > layerCnt)
      break;

    uint planeIndex = _overUnderPlaneLayerMap[mUnder][mOver];
    _geomSeq->intersect_rows(mUnder, mOver, planeIndex);
    cnt++;
  }
  return cnt;
}

void extMain::deletePlanes(uint layerCnt)
{
  if (_overUnderPlaneLayerMap == NULL)
    return;

  for (uint ii = 1; ii < layerCnt + 1; ii++) {
    delete[] _overUnderPlaneLayerMap[ii];
  }
  delete[] _overUnderPlaneLayerMap;
}
uint extMain::addShapeOnGs(odb::dbShape* s, bool swap_coords)
{
  int level = s->getTechLayer()->getRoutingLevel();

  if (!swap_coords)  // horizontal
    return _geomSeq->box(s->xMin(), s->yMin(), s->xMax(), s->yMax(), level);
  else
    return _geomSeq->box(s->yMin(), s->xMin(), s->yMax(), s->xMax(), level);
}
uint extMain::addSBoxOnGs(odb::dbSBox* s, bool swap_coords)
{
  int level = s->getTechLayer()->getRoutingLevel();

  if (!swap_coords)  // horizontal
    return _geomSeq->box(s->xMin(), s->yMin(), s->xMax(), s->yMax(), level);
  else
    return _geomSeq->box(s->yMin(), s->xMin(), s->yMax(), s->xMax(), level);
}

uint extMain::addPowerGs(int dir, int* ll, int* ur)
{
  bool rotatedGs = getRotatedFlag();
  bool swap_coords = !dir;

  uint cnt = 0;
  odb::dbSet<odb::dbNet> bnets = _block->getNets();
  odb::dbSet<odb::dbNet>::iterator net_itr;
  odb::dbNet* net;
  for (net_itr = bnets.begin(); net_itr != bnets.end(); ++net_itr) {
    net = *net_itr;

    odb::dbSigType type = net->getSigType();
    if (type != odb::dbSigType::POWER && type != odb::dbSigType::GROUND)
      continue;
    odb::dbSet<odb::dbSWire> swires = net->getSWires();
    odb::dbSet<odb::dbSWire>::iterator itr;
    for (itr = swires.begin(); itr != swires.end(); ++itr) {
      odb::dbSWire* swire = *itr;
      odb::dbSet<odb::dbSBox> sboxes = swire->getWires();
      odb::dbSet<odb::dbSBox>::iterator box_itr;
      for (box_itr = sboxes.begin(); box_itr != sboxes.end(); ++box_itr) {
        odb::dbSBox* s = *box_itr;
        if (s->isVia())
          continue;

        if (ll == NULL) {
          if (dir >= 0) {
            odb::Rect r;
            s->getBox(r);
            if (matchDir(dir, r))
              continue;
          }
          int n = 0;
          if (!rotatedGs)
            n = _geomSeq->box(s->xMin(),
                              s->yMin(),
                              s->xMax(),
                              s->yMax(),
                              s->getTechLayer()->getRoutingLevel());
          else
            n = addSBoxOnGs(s, swap_coords);

          if (n == 0)
            cnt++;
          continue;
        }
        odb::Rect r;
        s->getBox(r);

        int bb[2] = {r.xMin(), r.yMin()};

        if (bb[dir] > ur[dir])
          continue;

        if (bb[dir] < ll[dir])
          continue;

        _geomSeq->box(r.xMin(),
                      r.yMin(),
                      r.xMax(),
                      r.yMax(),
                      s->getTechLayer()->getRoutingLevel());
        cnt++;
      }
    }
  }
  return cnt;
}
void extMain::getBboxPerLayer(odb::Rect* rectTable)
{
  odb::dbSet<odb::dbNet> bnets = _block->getNets();
  odb::dbSet<odb::dbNet>::iterator net_itr;
  odb::dbWirePathItr pitr;
  odb::dbWire* wire;
  odb::dbWirePath path;
  odb::dbWirePathShape pshape;
  odb::dbShape dshape;
  odb::dbNet* net;
  for (net_itr = bnets.begin(); net_itr != bnets.end(); ++net_itr) {
    net = *net_itr;

    odb::dbSigType type = net->getSigType();
    if (type != odb::dbSigType::POWER
        && type != odb::dbSigType::GROUND) {  // signal

      wire = net->getWire();
      if (!wire)
        continue;

      for (pitr.begin(wire); pitr.getNextPath(path);) {
        while (pitr.getNextShape(pshape)) {
          dshape = pshape.shape;
          if (dshape.isVia())
            continue;

          uint level = dshape.getTechLayer()->getRoutingLevel();

          odb::Rect r;
          dshape.getBox(r);

          rectTable[level].merge(r);
        }
      }
      continue;
    }

    odb::dbSet<odb::dbSWire> swires = net->getSWires();
    odb::dbSet<odb::dbSWire>::iterator itr;
    for (itr = swires.begin(); itr != swires.end(); ++itr) {
      odb::dbSWire* swire = *itr;
      odb::dbSet<odb::dbSBox> sboxes = swire->getWires();
      odb::dbSet<odb::dbSBox>::iterator box_itr;
      for (box_itr = sboxes.begin(); box_itr != sboxes.end(); ++box_itr) {
        odb::dbSBox* s = *box_itr;
        if (s->isVia())
          continue;

        uint level = s->getTechLayer()->getRoutingLevel();

        odb::Rect r;
        s->getBox(r);

        rectTable[level].merge(r);
      }
    }
  }
}
uint extMain::addSignalGs(int dir, int* ll, int* ur)
{
  bool rotatedGs = getRotatedFlag();
  bool swap_coords = !dir;

  uint cnt = 0;
  odb::dbSet<odb::dbNet> bnets = _block->getNets();
  odb::dbSet<odb::dbNet>::iterator net_itr;
  odb::dbNet* net;
  odb::dbWirePathItr pitr;
  odb::dbWire* wire;
  odb::dbWirePath path;
  odb::dbWirePathShape pshape;
  odb::dbShape dshape;
  for (net_itr = bnets.begin(); net_itr != bnets.end(); ++net_itr) {
    net = *net_itr;
    odb::dbSigType type = net->getSigType();
    if (type == odb::dbSigType::POWER || type == odb::dbSigType::GROUND)
      continue;
    wire = net->getWire();
    if (!wire)
      continue;
    for (pitr.begin(wire); pitr.getNextPath(path);) {
      while (pitr.getNextShape(pshape)) {
        dshape = pshape.shape;
        if (dshape.isVia())
          continue;
        //				uint netId= net->getId();

        if (ll == NULL) {
          if (dir >= 0) {
            odb::Rect r;
            dshape.getBox(r);
            if (matchDir(dir, r))
              continue;
          }
          int n = 0;
          if (!rotatedGs)
            n = _geomSeq->box(dshape.xMin(),
                              dshape.yMin(),
                              dshape.xMax(),
                              dshape.yMax(),
                              dshape.getTechLayer()->getRoutingLevel());
          else
            n = addShapeOnGs(&dshape, swap_coords);

          if (n == 0)
            cnt++;
          continue;
        }
        odb::Rect r;
        dshape.getBox(r);

        int bb[2] = {r.xMin(), r.yMin()};

        if (bb[dir] >= ur[dir])
          continue;

        if (bb[dir] <= ll[dir])
          continue;

        _geomSeq->box(r.xMin(),
                      r.yMin(),
                      r.xMax(),
                      r.yMax(),
                      dshape.getTechLayer()->getRoutingLevel());
        cnt++;
      }
    }
  }
  return cnt;
}
uint extMain::addObsShapesOnPlanes(odb::dbInst* inst,
                                   bool rotatedFlag,
                                   bool swap_coords)
{
  uint cnt = 0;

  odb::dbInstShapeItr obs_shapes;

  odb::dbShape s;
  for (obs_shapes.begin(inst, odb::dbInstShapeItr::OBSTRUCTIONS);
       obs_shapes.next(s);) {
    if (s.isVia())
      continue;

    uint level = s.getTechLayer()->getRoutingLevel();

    uint n = 0;
    if (!rotatedFlag)
      n = _geomSeq->box(s.xMin(), s.yMin(), s.xMax(), s.yMax(), level);
    else
      n = addShapeOnGs(&s, swap_coords);
  }
  return cnt;
}
uint extMain::addItermShapesOnPlanes(odb::dbInst* inst,
                                     bool rotatedFlag,
                                     bool swap_coords)
{
  uint cnt = 0;
  odb::dbSet<odb::dbITerm> iterms = inst->getITerms();
  odb::dbSet<odb::dbITerm>::iterator iterm_itr;

  for (iterm_itr = iterms.begin(); iterm_itr != iterms.end(); ++iterm_itr) {
    odb::dbITerm* iterm = *iterm_itr;

    odb::dbShape s;
    odb::dbITermShapeItr term_shapes;
    for (term_shapes.begin(iterm); term_shapes.next(s);) {
      if (s.isVia())
        continue;

      uint level = s.getTechLayer()->getRoutingLevel();

      uint n = 0;
      if (!rotatedFlag)
        n = _geomSeq->box(s.xMin(), s.yMin(), s.xMax(), s.yMax(), level);
      else
        n = addShapeOnGs(&s, swap_coords);

      if (n == 0)
        cnt++;
    }
  }
  return cnt;
}
uint extMain::addInstShapesOnPlanes(uint dir, int* ll, int* ur)
{
  bool rotated = getRotatedFlag();

  uint cnt = 0;
  odb::dbSet<odb::dbInst> insts = _block->getInsts();
  odb::dbSet<odb::dbInst>::iterator inst_itr;
  for (inst_itr = insts.begin(); inst_itr != insts.end(); ++inst_itr) {
    odb::dbInst* inst = *inst_itr;

    if (ll == NULL) {
      cnt += addItermShapesOnPlanes(inst, rotated, !dir);
      cnt += addObsShapesOnPlanes(inst, rotated, !dir);
      continue;
    }
  }
  return cnt;
}

}  // namespace rcx
