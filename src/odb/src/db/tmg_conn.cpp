// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "tmg_conn.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>

#include "odb/db.h"
#include "odb/dbShape.h"
#include "odb/dbWireCodec.h"
#include "tmg_conn_g.h"
#include "utl/Logger.h"

namespace odb {

using utl::ODB;

static void tmg_getDriveTerm(dbNet* net, dbITerm** iterm, dbBTerm** bterm)
{
  *iterm = nullptr;
  *bterm = nullptr;
  dbSet<dbITerm> iterms = net->getITerms();
  dbITerm* it_inout = nullptr;
  for (dbITerm* it : iterms) {
    if (it->getIoType() == dbIoType::OUTPUT) {
      *iterm = it;
      return;
    }
    if (it->getIoType() == dbIoType::INOUT && !it_inout) {
      it_inout = it;
    }
  }
  dbSet<dbBTerm> bterms = net->getBTerms();
  dbBTerm* bt_inout = nullptr;
  for (dbBTerm* bt : bterms) {
    if (bt->getIoType() == dbIoType::INPUT) {
      *bterm = bt;
      return;
    }
    if (bt->getIoType() == dbIoType::INOUT && !bt_inout) {
      bt_inout = bt;
    }
  }
  if (bt_inout) {
    *bterm = bt_inout;
  } else if (it_inout) {
    *iterm = it_inout;
  } else if (!bterms.empty()) {
    *bterm = *bterms.begin();
  } else if (!iterms.empty()) {
    *iterm = *iterms.begin();
  }
}

tmg_conn::tmg_conn(utl::Logger* logger) : logger_(logger)
{
  _rcV.reserve(1024);
  _termV.reserve(1024);
  _tstackV.reserve(1024);
  _csVV.reserve(1024);
  _csNV.reserve(1024);
  _shortV.reserve(1024);
  _need_short_wire_id = false;
  _first_for_clear = nullptr;
}

tmg_conn::~tmg_conn() = default;

int tmg_conn::ptDist(const int fr, const int to) const
{
  return abs(_ptV[fr]._x - _ptV[to]._x) + abs(_ptV[fr]._y - _ptV[to]._y);
}

tmg_rcpt* tmg_conn::allocPt(int x, int y, dbTechLayer* layer)
{
  return &_ptV.emplace_back(x, y, layer);
}

void tmg_conn::addRc(const dbShape& s,
                     const int from_idx,
                     const int to_idx,
                     dbTechNonDefaultRule* rule)
{
  bool is_vertical;
  int width;
  if (s.getTechVia() || s.getVia()) {
    is_vertical = false;
    width = 0;
  } else if (_ptV[from_idx]._x != _ptV[to_idx]._x) {
    is_vertical = false;
    width = s.yMax() - s.yMin();
  } else if (_ptV[from_idx]._y != _ptV[to_idx]._y) {
    is_vertical = true;
    width = s.xMax() - s.xMin();
  } else if (s.xMax() - s.xMin() == s.yMax() - s.yMin()) {
    is_vertical = false;
    width = s.xMax() - s.xMin();
  } else {
    is_vertical = false;
    width = 0;
  }
  tmg_rc x(from_idx,
           to_idx,
           {{s.xMin(), s.yMin(), s.xMax(), s.yMax()},
            s.getTechLayer(),
            s.getTechVia(),
            s.getVia(),
            rule},
           is_vertical,
           width,
           width / 2);
  _rcV.push_back(x);
}

void tmg_conn::addRc(const int k,
                     const tmg_rc_sh& s,
                     const int from_idx,
                     const int to_idx,
                     const int xmin,
                     const int ymin,
                     const int xmax,
                     const int ymax)
{
  const int width = _rcV[k]._width;
  tmg_rc x(from_idx,
           to_idx,
           {{xmin, ymin, xmax, ymax},
            s.getTechLayer(),
            s.getTechVia(),
            s.getVia(),
            s.getRule()},
           _rcV[k]._is_vertical,
           width,
           width / 2);
  _rcV.push_back(x);
}

tmg_rc* tmg_conn::addRcPatch(const int from_idx, const int to_idx)
{
  dbTechLayer* layer = _ptV[from_idx]._layer;
  if (!layer || layer != _ptV[to_idx]._layer
      || (_ptV[from_idx]._x != _ptV[to_idx]._x
          && _ptV[from_idx]._y != _ptV[to_idx]._y)) {
    return nullptr;
  }
  const bool is_vertical = (_ptV[from_idx]._y != _ptV[to_idx]._y);
  int xlo, ylo, xhi, yhi;
  if (is_vertical) {
    xlo = _ptV[from_idx]._x;
    xhi = xlo;
    std::tie(ylo, yhi) = std::minmax(_ptV[from_idx]._y, _ptV[to_idx]._y);
  } else {
    ylo = _ptV[from_idx]._y;
    yhi = ylo;
    std::tie(xlo, xhi) = std::minmax(_ptV[from_idx]._x, _ptV[to_idx]._x);
  }
  const int width = layer->getWidth();  // trouble for nondefault
  const int hw = width / 2;
  tmg_rc x(from_idx,
           to_idx,
           {{xlo - hw, ylo - hw, xhi + hw, yhi + hw}, layer, nullptr, nullptr},
           is_vertical,
           width,
           hw);
  _rcV.push_back(x);
  return &_rcV.back();
}

void tmg_conn::addITerm(dbITerm* iterm)
{
  _csVV.emplace_back();
  _csNV.emplace_back();

  tmg_rcterm& x = _termV.emplace_back(iterm);
  x._pt = nullptr;
  x._first_pt = nullptr;
}

void tmg_conn::addBTerm(dbBTerm* bterm)
{
  _csVV.emplace_back();
  _csNV.emplace_back();

  tmg_rcterm& x = _termV.emplace_back(bterm);
  x._pt = nullptr;
  x._first_pt = nullptr;
}

void tmg_conn::addShort(const int i0, const int i1)
{
  _shortV.emplace_back(i0, i1);
  if (_ptV[i0]._fre) {
    _ptV[i0]._fre = false;
  } else {
    _ptV[i0]._jct = true;
  }
  if (_ptV[i1]._fre) {
    _ptV[i1]._fre = false;
  } else {
    _ptV[i1]._jct = true;
  }
}

void tmg_conn::loadNet(dbNet* net)
{
  _net = net;
  _rcV.clear();
  _ptV.clear();
  _termV.clear();
  _csVV.clear();
  _csNV.clear();
  _shortV.clear();
  _first_for_clear = nullptr;

  for (dbITerm* iterm : net->getITerms()) {
    addITerm(iterm);
  }

  for (dbBTerm* bterm : net->getBTerms()) {
    addBTerm(bterm);
  }
}

void tmg_conn::loadSWire(dbNet* net)
{
  _hasSWire = false;
  dbSet<dbSWire> swires = net->getSWires();
  if (swires.empty()) {
    return;
  }

  _hasSWire = true;
  for (dbSWire* sw : swires) {
    for (dbSBox* sbox : sw->getWires()) {
      const Rect rect = sbox->getBox();
      dbTechLayer* layer1 = nullptr;
      dbTechLayer* layer2 = nullptr;
      int x1, y1, x2, y2;
      dbShape shape;
      if (sbox->isVia()) {
        x1 = rect.xCenter();
        x2 = x1;
        y1 = rect.yCenter();
        y2 = y1;
        if (dbTechVia* tech_via = sbox->getTechVia()) {
          layer1 = tech_via->getTopLayer();
          layer2 = tech_via->getBottomLayer();
          shape.setVia(tech_via, rect);
        } else {
          dbVia* via = sbox->getBlockVia();
          layer1 = via->getTopLayer();
          layer2 = via->getBottomLayer();
          shape.setVia(via, rect);
        }
      } else {
        if (rect.getDir() == 1) {
          y1 = rect.yCenter();
          y2 = y1;
          x1 = rect.xMin() + (rect.yMax() - y1);
          x2 = rect.xMax() - (rect.yMax() - y1);
        } else {
          x1 = rect.xCenter();
          x2 = x1;
          y1 = rect.yMin() + (rect.xMax() - x1);
          y2 = rect.yMax() - (rect.xMax() - x1);
        }
        layer1 = sbox->getTechLayer();
        layer2 = layer1;
        shape.setSegment(layer1, rect);
      }

      if (_ptV.empty() || layer1 != _ptV.back()._layer || x1 != _ptV.back()._x
          || y1 != _ptV.back()._y) {
        allocPt(x1, y1, layer1);
      }

      allocPt(x2, y2, layer2);
      addRc(shape, _ptV.size() - 2, _ptV.size() - 1);
    }
  }
}

void tmg_conn::loadWire(dbWire* wire)
{
  _ptV.clear();
  dbWirePathItr pitr;
  dbWirePath path;
  pitr.begin(wire);
  while (pitr.getNextPath(path)) {
    if (_ptV.empty() || path.layer != _ptV.back()._layer
        || path.point.getX() != _ptV.back()._x
        || path.point.getY() != _ptV.back()._y) {
      allocPt(path.point.getX(), path.point.getY(), path.layer);
    }
    dbWirePathShape pathShape;
    while (pitr.getNextShape(pathShape)) {
      allocPt(pathShape.point.getX(), pathShape.point.getY(), pathShape.layer);
      addRc(pathShape.shape, _ptV.size() - 2, _ptV.size() - 1, path.rule);
    }
  }

  loadSWire(wire->getNet());
}

void tmg_conn::splitBySj(const int j,
                         const int rt,
                         const int sjxMin,
                         const int sjyMin,
                         const int sjxMax,
                         const int sjyMax)
{
  tmg_rc_sh* sj = &(_rcV[j]._shape);
  const int isVia = sj->isVia() ? 1 : 0;
  _search->searchStart(rt, {sjxMin, sjyMin, sjxMax, sjyMax}, isVia);
  int klast = -1;
  int k;
  while (_search->searchNext(&k)) {
    if (k == klast || k == j) {
      continue;
    }
    if (_rcV[j]._to_idx == _rcV[k]._from_idx
        || _rcV[j]._from_idx == _rcV[k]._to_idx) {
      continue;
    }
    sj = &(_rcV[j]._shape);
    if (!sj->isVia() && _rcV[j]._is_vertical == _rcV[k]._is_vertical) {
      continue;
    }
    const tmg_rc_sh* sk = &(_rcV[k]._shape);
    if (sk->isVia()) {
      continue;
    }
    dbTechLayer* tlayer = _ptV[_rcV[k]._from_idx]._layer;
    int nxmin = sk->xMin();
    int nxmax = sk->xMax();
    int nymin = sk->yMin();
    int nymax = sk->yMax();
    int x;
    int y;
    if (_rcV[k]._is_vertical) {
      if (sjyMin - sk->yMin() < _rcV[k]._width) {
        continue;
      }
      if (sk->yMax() - sjyMax < _rcV[k]._width) {
        continue;
      }
      if (_ptV[_rcV[k]._from_idx]._y > _ptV[_rcV[k]._to_idx]._y) {
        _rcV[k]._shape.setYmin(_ptV[_rcV[j]._from_idx]._y - _rcV[k]._width / 2);
        nymax = _ptV[_rcV[j]._from_idx]._y + _rcV[k]._width / 2;
      } else {
        _rcV[k]._shape.setYmax(_ptV[_rcV[j]._from_idx]._y + _rcV[k]._width / 2);
        nymin = _ptV[_rcV[j]._from_idx]._y - _rcV[k]._width / 2;
      }
      x = _ptV[_rcV[k]._from_idx]._x;
      y = _ptV[_rcV[j]._from_idx]._y;
    } else {
      if (sjxMin - sk->xMin() < _rcV[k]._width) {
        continue;
      }
      if (sk->xMax() - sjxMax < _rcV[k]._width) {
        continue;
      }
      if (_ptV[_rcV[k]._from_idx]._x > _ptV[_rcV[k]._to_idx]._x) {
        _rcV[k]._shape.setXmin(_ptV[_rcV[j]._from_idx]._x - _rcV[k]._width / 2);
        nxmax = _ptV[_rcV[j]._from_idx]._x + _rcV[k]._width / 2;
      } else {
        _rcV[k]._shape.setXmax(_ptV[_rcV[j]._from_idx]._x + _rcV[k]._width / 2);
        nxmin = _ptV[_rcV[j]._from_idx]._x - _rcV[k]._width / 2;
      }
      x = _ptV[_rcV[j]._from_idx]._x;
      y = _ptV[_rcV[k]._from_idx]._y;
    }
    klast = k;
    tmg_rcpt* pt = allocPt(x, y, tlayer);
    pt->_tindex = -1;
    pt->_t_alt = nullptr;
    pt->_next_for_term = nullptr;
    pt->_pinpt = false;
    pt->_c2pinpt = false;
    pt->_next_for_clear = nullptr;
    pt->_sring = nullptr;
    const int endTo = _rcV[k]._to_idx;
    _rcV[k]._to_idx = _ptV.size() - 1;
    // create new tmg_rc
    addRc(
        k, _rcV[k]._shape, _ptV.size() - 1, endTo, nxmin, nymin, nxmax, nymax);
    _search->addShape(rt, {nxmin, nymin, nxmax, nymax}, 0, _rcV.size() - 1);
  }
}

// split top of T shapes
void tmg_conn::splitTtop()
{
  for (size_t j = 0; j < _rcV.size(); j++) {
    tmg_rc_sh* sj = &(_rcV[j]._shape);
    if (sj->isVia()) {
      dbTechLayer* layb = nullptr;
      dbTechLayer* layt = nullptr;
      dbSet<dbBox> boxes;
      if (dbTechVia* tv = sj->getTechVia()) {
        layb = tv->getBottomLayer();
        layt = tv->getTopLayer();
        boxes = tv->getBoxes();
      } else {
        dbVia* vv = sj->getVia();
        layb = vv->getBottomLayer();
        layt = vv->getTopLayer();
        boxes = vv->getBoxes();
      }
      const int via_x = _ptV[_rcV[j]._from_idx]._x;
      const int via_y = _ptV[_rcV[j]._from_idx]._y;
      for (dbBox* b : boxes) {
        if (b->getTechLayer() == layb) {
          splitBySj(j,
                    layb->getRoutingLevel(),
                    via_x + b->xMin(),
                    via_y + b->yMin(),
                    via_x + b->xMax(),
                    via_y + b->yMax());
        } else if (b->getTechLayer() == layt) {
          splitBySj(j,
                    layt->getRoutingLevel(),
                    via_x + b->xMin(),
                    via_y + b->yMin(),
                    via_x + b->xMax(),
                    via_y + b->yMax());
        }
      }
    } else {
      const int rt = sj->getTechLayer()->getRoutingLevel();
      splitBySj(j, rt, sj->xMin(), sj->yMin(), sj->xMax(), sj->yMax());
    }
  }
}

void tmg_conn::setSring()
{
  for (const tmg_rcshort& rcshort : _shortV) {
    if (rcshort._skip) {
      continue;
    }
    tmg_rcpt* pfr = &_ptV[rcshort._i0];
    tmg_rcpt* pto = &_ptV[rcshort._i1];
    if (pfr == pto) {
      continue;
    }
    if (pfr->_sring && !pto->_sring) {
      pto->_sring = pfr->_sring;
      pfr->_sring = pto;
    } else if (pto->_sring && !pfr->_sring) {
      pfr->_sring = pto->_sring;
      pto->_sring = pfr;
    } else if (!pfr->_sring && !pto->_sring) {
      pfr->_sring = pto;
      pto->_sring = pfr;
    } else {
      tmg_rcpt* x = pfr->_sring;
      while (x->_sring != pfr && x != pto) {
        x = x->_sring;
      }
      if (x == pto) {
        continue;
      }
      x->_sring = pto;
      x = pto;
      while (x->_sring != pto) {
        x = x->_sring;
      }
      x->_sring = pfr;
    }
  }
}

void tmg_conn::detachTilePins()
{
  _slicedTilePinCnt = 0;
  for (const tmg_rcterm& term : _termV) {
    if (term._iterm) {
      continue;
    }
    dbBTerm* bterm = term._bterm;
    dbShape pin;
    if (!bterm->getFirstPin(pin) || pin.isVia()) {
      continue;
    }
    const Rect rectb = pin.getBox();
    const int rtlb = pin.getTechLayer()->getRoutingLevel();
    bool sliceDone = false;
    for (int k = 0; !sliceDone && k < _termV.size(); k++) {
      tmg_rcterm* tx = &_termV[k];
      if (tx->_bterm) {
        continue;
      }
      dbMTerm* mterm = tx->_iterm->getMTerm();
      const dbTransform transform = tx->_iterm->getInst()->getTransform();
      for (dbMPin* mpin : mterm->getMPins()) {
        for (dbBox* box : mpin->getGeometry()) {
          Rect recti = box->getBox();
          transform.apply(recti);
          if (box->isVia()) {
            dbTechVia* tv = box->getTechVia();
            int rtli = tv->getTopLayer()->getRoutingLevel();
            if (rtli <= 1) {
              continue;
            }
            if (rtli != rtlb) {
              rtli = tv->getBottomLayer()->getRoutingLevel();
              if (rtli == 0 || rtli != rtlb) {
                continue;
              }
            }
          } else {
            const int rtli = box->getTechLayer()->getRoutingLevel();
            if (rtli != rtlb) {
              continue;
            }
          }
          if (recti.contains(rectb)) {
            logger_->error(
                ODB, 420, "tmg_conn::detachTilePins: tilepin inside iterm.");
          }

          if (!recti.overlaps(rectb)) {
            continue;
          }
          int x1 = rectb.xMin();
          int y1 = rectb.yMin();
          int x2 = rectb.xMax();
          int y2 = rectb.yMax();
          if (x2 > recti.xMax() && x1 > recti.xMin()) {
            x1 = recti.xMax();
          } else if (x1 < recti.xMin() && x2 < recti.xMax()) {
            x2 = recti.xMin();
          } else if (y2 > recti.yMax() && y1 > recti.yMin()) {
            y1 = recti.yMax();
          } else if (y1 < recti.yMin() && y2 < recti.yMax()) {
            y2 = recti.yMin();
          }
          _stbtx1[_slicedTilePinCnt] = x1 + 1;
          _stbty1[_slicedTilePinCnt] = y1 + 1;
          _stbtx2[_slicedTilePinCnt] = x2 - 1;
          _stbty2[_slicedTilePinCnt] = y2 - 1;
          _slicedTileBTerm[_slicedTilePinCnt++] = bterm;
          sliceDone = true;
        }
      }
    }
  }
}

void tmg_conn::getBTermSearchBox(dbBTerm* bterm, dbShape& pin, Rect& rect)
{
  for (int ii = 0; ii < _slicedTilePinCnt; ii++) {
    if (_slicedTileBTerm[ii] == bterm) {
      rect.reset(_stbtx1[ii], _stbty1[ii], _stbtx2[ii], _stbty2[ii]);
      return;
    }
  }
  rect = pin.getBox();
}

void tmg_conn::findConnections()
{
  if (_ptV.empty()) {
    return;
  }
  if (!_search) {
    _search = std::make_unique<tmg_conn_search>();
  }
  _search->clear();

  for (auto& pt : _ptV) {
    pt._fre = true;
    pt._jct = false;
    pt._pinpt = false;
    pt._c2pinpt = false;
    pt._next_for_clear = nullptr;
    pt._sring = nullptr;
  }
  _first_for_clear = nullptr;
  for (size_t j = 0; j < _rcV.size() - 1; j++) {
    if (_rcV[j]._to_idx == _rcV[j + 1]._from_idx) {
      _ptV[_rcV[j]._to_idx]._fre = false;
    }
  }

  // put wires in search
  for (size_t j = 0; j < _rcV.size(); j++) {
    tmg_rc_sh* s = &(_rcV[j]._shape);
    if (s->isVia()) {
      const int via_x = _ptV[_rcV[j]._from_idx]._x;
      const int via_y = _ptV[_rcV[j]._from_idx]._y;

      dbTechLayer* layb = nullptr;
      dbTechLayer* layt = nullptr;
      dbSet<dbBox> boxes;
      if (dbTechVia* tv = s->getTechVia()) {
        layb = tv->getBottomLayer();
        layt = tv->getTopLayer();
        boxes = tv->getBoxes();
      } else {
        dbVia* vv = s->getVia();
        layb = vv->getBottomLayer();
        layt = vv->getTopLayer();
        boxes = vv->getBoxes();
      }
      const int rt_b = layb->getRoutingLevel();
      const int rt_t = layt->getRoutingLevel();
      for (dbBox* b : boxes) {
        if (b->getTechLayer() == layb) {
          _search->addShape(rt_b,
                            {via_x + b->xMin(),
                             via_y + b->yMin(),
                             via_x + b->xMax(),
                             via_y + b->yMax()},
                            1,
                            j);
        } else if (b->getTechLayer() == layt) {
          _search->addShape(rt_t,
                            {via_x + b->xMin(),
                             via_y + b->yMin(),
                             via_x + b->xMax(),
                             via_y + b->yMax()},
                            1,
                            j);
        }
      }

    } else {
      const int rt = s->getTechLayer()->getRoutingLevel();
      _search->addShape(rt, s->rect(), 0, j);
    }
  }

  if (_rcV.size() < 10000) {
    splitTtop();
  }

  // find self-intersections of wires
  for (int j = 0; j < (int) _rcV.size() - 1; j++) {
    const tmg_rc_sh* s = &(_rcV[j]._shape);
    const int conn_next = (_rcV[j]._to_idx == _rcV[j + 1]._from_idx);
    if (s->isVia()) {
      const int via_x = _ptV[_rcV[j]._from_idx]._x;
      const int via_y = _ptV[_rcV[j]._from_idx]._y;

      dbTechLayer* layb = nullptr;
      dbTechLayer* layt = nullptr;
      dbSet<dbBox> boxes;
      if (dbTechVia* tv = s->getTechVia()) {
        layb = tv->getBottomLayer();
        layt = tv->getTopLayer();
        boxes = tv->getBoxes();
      } else {
        dbVia* vv = s->getVia();
        layb = vv->getBottomLayer();
        layt = vv->getTopLayer();
        boxes = vv->getBoxes();
      }

      const int rt_b = layb->getRoutingLevel();
      const int rt_t = layt->getRoutingLevel();
      for (dbBox* b : boxes) {
        if (b->getTechLayer() == layb) {
          _search->searchStart(rt_b,
                               {via_x + b->xMin(),
                                via_y + b->yMin(),
                                via_x + b->xMax(),
                                via_y + b->yMax()},
                               1);
        } else if (b->getTechLayer() == layt) {
          _search->searchStart(rt_t,
                               {via_x + b->xMin(),
                                via_y + b->yMin(),
                                via_x + b->xMax(),
                                via_y + b->yMax()},
                               1);
        } else {
          continue;  // cut layer
        }
        int klast = -1;
        int k;
        while (_search->searchNext(&k)) {
          if (k != klast && k > j) {
            if (k == j + 1 && conn_next) {
              continue;
            }
            klast = k;
            connectShapes(j, k);
          }
        }
      }
    } else {
      const int rt = s->getTechLayer()->getRoutingLevel();
      _search->searchStart(rt, s->rect(), 0);
      int klast = -1;
      int k;
      while (_search->searchNext(&k)) {
        if (k != klast && k > j) {
          if (k == j + 1 && conn_next) {
            continue;
          }
          klast = k;
          connectShapes(j, k);
        }
      }
    }
  }

  removeWireLoops();

  // detach tilPins from iterms
  detachTilePins();

  // connect pins
  for (int j = 0; j < _termV.size(); j++) {
    _csV = &_csVV[j];
    _csN = 0;
    tmg_rcterm* x = &_termV[j];
    if (x->_iterm) {
      dbMTerm* mterm = x->_iterm->getMTerm();
      const dbTransform transform = x->_iterm->getInst()->getTransform();
      for (dbMPin* mpin : mterm->getMPins()) {
        dbSet<dbBox> boxes = mpin->getGeometry();
        for (int ipass = 0; ipass < 2; ipass++) {
          for (dbBox* box : boxes) {
            if (ipass == 1 && box->isVia()) {
              dbTechVia* tv = box->getTechVia();
              const int rt_t = tv->getTopLayer()->getRoutingLevel();
              if (rt_t <= 1) {
                continue;
              }
              Rect rect = box->getBox();
              transform.apply(rect);
              _search->searchStart(rt_t, rect, 2);
              int klast = -1;
              int k;
              while (_search->searchNext(&k)) {
                if (k != klast) {
                  klast = k;
                  int ii;
                  for (ii = 0; ii < _csN; ii++) {
                    if (k == (*_csV)[ii].k) {
                      break;
                    }
                  }
                  if (ii < _csN) {
                    continue;
                  }
                  if (_csN == 32) {
                    break;
                  }
                  (*_csV)[_csN].k = k;
                  (*_csV)[_csN].rect = rect;
                  (*_csV)[_csN].rtlev = rt_t;
                  _csN++;
                }
              }
              const int rt_b = tv->getBottomLayer()->getRoutingLevel();
              if (rt_b == 0) {
                continue;
              }
              _search->searchStart(rt_b, rect, 2);
              klast = -1;
              while (_search->searchNext(&k)) {
                if (k != klast) {
                  klast = k;
                  int ii;
                  for (ii = 0; ii < _csN; ii++) {
                    if (k == (*_csV)[ii].k) {
                      break;
                    }
                  }
                  if (ii < _csN) {
                    continue;
                  }
                  if (_csN == 32) {
                    break;
                  }
                  (*_csV)[_csN].k = k;
                  (*_csV)[_csN].rect = rect;
                  (*_csV)[_csN].rtlev = rt_b;
                  _csN++;
                }
              }
            } else if (ipass == 0 && !box->isVia()) {
              const int rt = box->getTechLayer()->getRoutingLevel();
              Rect rect = box->getBox();
              transform.apply(rect);
              _search->searchStart(rt, rect, 2);
              int klast = -1;
              int k;
              while (_search->searchNext(&k)) {
                if (k != klast) {
                  klast = k;
                  int ii;
                  for (ii = 0; ii < _csN; ii++) {
                    if (k == (*_csV)[ii].k) {
                      break;
                    }
                  }
                  if (ii < _csN && _csN >= 8) {
                    continue;
                  }
                  if (_csN == 32) {
                    break;
                  }
                  (*_csV)[_csN].k = k;
                  (*_csV)[_csN].rect = rect;
                  (*_csV)[_csN].rtlev = rt;
                  _csN++;
                }
              }
            }
          }
        }
      }  // mpins
    } else {
      // bterm
      dbShape pin;
      if (x->_bterm->getFirstPin(pin)) {
        if (pin.isVia()) {
          // TODO
        } else {
          const int rt = pin.getTechLayer()->getRoutingLevel();
          Rect rect;
          getBTermSearchBox(x->_bterm, pin, rect);
          _search->searchStart(rt, rect, 2);
          int klast = -1;
          int k;
          while (_search->searchNext(&k)) {
            if (k != klast) {
              klast = k;
              int ii;
              for (ii = 0; ii < _csN; ii++) {
                if (k == (*_csV)[ii].k) {
                  break;
                }
              }
              if (ii < _csN) {
                continue;
              }
              if (_csN == 32) {
                break;
              }
              (*_csV)[_csN].k = k;
              (*_csV)[_csN].rect = rect;
              (*_csV)[_csN].rtlev = rt;
              _csN++;
            }
          }
        }
      }
    }
    _csNV[j] = _csN;
  }

  for (auto& pc : _ptV) {
    pc._pinpt = false;
    pc._c2pinpt = false;
    pc._next_for_clear = nullptr;
    pc._sring = nullptr;
  }
  setSring();

  for (int j = 0; j < _termV.size(); j++) {
    connectTerm(j, false);
  }
  const bool ok = checkConnected();
  if (!ok) {
    for (int j = 0; j < _termV.size(); j++) {
      connectTerm(j, true);
    }
  }

  // make terms of shorted points consistent
  for (int it = 0; it < 5; it++) {
    int cnt = 0;
    for (const tmg_rcshort& rcshort : _shortV) {
      if (rcshort._skip) {
        continue;
      }
      const int i0 = rcshort._i0;
      const int i1 = rcshort._i1;
      if (_ptV[i0]._tindex < 0 && _ptV[i1]._tindex >= 0) {
        _ptV[i0]._tindex = _ptV[i1]._tindex;
        cnt++;
      }
      if (_ptV[i1]._tindex < 0 && _ptV[i0]._tindex >= 0) {
        _ptV[i1]._tindex = _ptV[i0]._tindex;
        cnt++;
      }
    }
    if (!cnt) {
      break;
    }
  }
}

void tmg_conn::connectShapes(const int j, const int k)
{
  const tmg_rc_sh* sa = &(_rcV[j]._shape);
  const tmg_rc_sh* sb = &(_rcV[k]._shape);
  const int afr = _rcV[j]._from_idx;
  const int ato = _rcV[j]._to_idx;
  const int bfr = _rcV[k]._from_idx;
  const int bto = _rcV[k]._to_idx;
  const int xlo = std::max(sa->xMin(), sb->xMin());
  const int ylo = std::max(sa->yMin(), sb->yMin());
  const int xhi = std::min(sa->xMax(), sb->xMax());
  const int yhi = std::min(sa->yMax(), sb->yMax());
  int xc = (xlo + xhi) / 2;
  int yc = (ylo + yhi) / 2;
  bool choose_afr = false;
  bool choose_bfr = false;
  if (sa->isVia() && sb->isVia()) {
    if (_ptV[afr]._layer == _ptV[bfr]._layer) {
      choose_afr = true;
      choose_bfr = true;
    } else if (_ptV[afr]._layer == _ptV[bto]._layer) {
      choose_afr = true;
      choose_bfr = false;
    } else if (_ptV[ato]._layer == _ptV[bfr]._layer) {
      choose_afr = false;
      choose_bfr = true;
    } else if (_ptV[ato]._layer == _ptV[bto]._layer) {
      choose_afr = false;
      choose_bfr = false;
    }
  } else if (sa->isVia()) {
    choose_afr = (_ptV[afr]._layer == _ptV[bfr]._layer);
    xc = _ptV[afr]._x;
    yc = _ptV[afr]._y;  // same for afr and ato
    const int dbfr = abs(_ptV[bfr]._x - xc) + abs(_ptV[bfr]._y - yc);
    const int dbto = abs(_ptV[bto]._x - xc) + abs(_ptV[bto]._y - yc);
    choose_bfr = (dbfr < dbto);
  } else if (sb->isVia()) {
    choose_bfr = (_ptV[afr]._layer == _ptV[bfr]._layer);
    xc = _ptV[bfr]._x;
    yc = _ptV[bfr]._y;
    const int dafr = abs(_ptV[afr]._x - xc) + abs(_ptV[afr]._y - yc);
    const int dato = abs(_ptV[ato]._x - xc) + abs(_ptV[ato]._y - yc);
    choose_afr = (dafr < dato);
  } else {
    // get distances to the center of intersection region, (xc,yc)
    const int dafr = abs(_ptV[afr]._x - xc) + abs(_ptV[afr]._y - yc);
    const int dato = abs(_ptV[ato]._x - xc) + abs(_ptV[ato]._y - yc);
    choose_afr = (dafr < dato);
    const int dbfr = abs(_ptV[bfr]._x - xc) + abs(_ptV[bfr]._y - yc);
    const int dbto = abs(_ptV[bto]._x - xc) + abs(_ptV[bto]._y - yc);
    choose_bfr = (dbfr < dbto);
  }
  int i0 = (choose_afr ? afr : ato);
  int i1 = (choose_bfr ? bfr : bto);
  if (i1 < i0) {
    std::swap(i0, i1);
  }
  addShort(i0, i1);
  _ptV[i0]._fre = false;
  _ptV[i1]._fre = false;
}

static void addPointToTerm(tmg_rcpt* pt, tmg_rcterm* x)
{
  tmg_rcpt* tpt = x->_pt;
  tmg_rcpt* ptpt = nullptr;
  while (tpt && (pt->_x > tpt->_x || (pt->_x == tpt->_x && pt->_y > tpt->_y))) {
    ptpt = tpt;
    tpt = tpt->_next_for_term;
  }
  if (ptpt) {
    ptpt->_next_for_term = pt;
  } else {
    x->_pt = pt;
  }
  pt->_next_for_term = tpt;
}

static void removePointFromTerm(tmg_rcpt* pt, tmg_rcterm* x)
{
  if (x->_pt == pt) {
    x->_pt = pt->_next_for_term;
    pt->_next_for_term = nullptr;
    return;
  }
  tmg_rcpt* ptpt = nullptr;
  tmg_rcpt* tpt;
  for (tpt = x->_pt; tpt; tpt = tpt->_next_for_term) {
    if (tpt == pt) {
      break;
    }
    ptpt = tpt;
  }
  if (!tpt) {
    return;  // error, not found
  }
  ptpt->_next_for_term = tpt->_next_for_term;
  pt->_next_for_term = nullptr;
}

void tmg_conn::connectTerm(const int j, const bool soft)
{
  _csV = &_csVV[j];
  _csN = _csNV[j];
  if (!_csN) {
    return;
  }
  for (tmg_rcpt* pc = _first_for_clear; pc; pc = pc->_next_for_clear) {
    pc->_pinpt = false;
    pc->_c2pinpt = false;
  }
  _first_for_clear = nullptr;

  for (int ii = 0; ii < _csN; ii++) {
    const int k = (*_csV)[ii].k;
    tmg_rcpt* pfr = &_ptV[_rcV[k]._from_idx];
    tmg_rcpt* pto = &_ptV[_rcV[k]._to_idx];
    const Point afr(pfr->_x, pfr->_y);
    if ((*_csV)[ii].rtlev == pfr->_layer->getRoutingLevel()
        && (*_csV)[ii].rect.intersects(afr)) {
      if (!(pfr->_pinpt || pfr->_c2pinpt)) {
        pfr->_next_for_clear = _first_for_clear;
        _first_for_clear = pfr;
      }
      if (!(pto->_pinpt || pto->_c2pinpt)) {
        pto->_next_for_clear = _first_for_clear;
        _first_for_clear = pto;
      }
      pfr->_pinpt = true;
      pfr->_c2pinpt = true;
      pto->_c2pinpt = true;
    }
    const Point ato(pto->_x, pto->_y);
    if ((*_csV)[ii].rtlev == pto->_layer->getRoutingLevel()
        && (*_csV)[ii].rect.intersects(ato)) {
      if (!(pfr->_pinpt || pfr->_c2pinpt)) {
        pfr->_next_for_clear = _first_for_clear;
        _first_for_clear = pfr;
      }
      if (!(pto->_pinpt || pto->_c2pinpt)) {
        pto->_next_for_clear = _first_for_clear;
        _first_for_clear = pto;
      }
      pto->_pinpt = true;
      pto->_c2pinpt = true;
      pfr->_c2pinpt = true;
    }
  }

  for (tmg_rcpt* pc = _first_for_clear; pc; pc = pc->_next_for_clear) {
    if (pc->_sring) {
      int c2pinpt = pc->_c2pinpt;
      for (tmg_rcpt* x = pc->_sring; x != pc; x = x->_sring) {
        if (x->_c2pinpt) {
          c2pinpt = 1;
        }
      }
      if (c2pinpt) {
        for (tmg_rcpt* x = pc->_sring; x != pc; x = x->_sring) {
          if (!(x->_pinpt || x->_c2pinpt)) {
            x->_next_for_clear = _first_for_clear;
            _first_for_clear = x;
          }
          x->_c2pinpt = true;
        }
      }
    }
  }

  for (int ii = 0; ii < _csN; ii++) {
    const int k = (*_csV)[ii].k;
    tmg_rcpt* pfr = &_ptV[_rcV[k]._from_idx];
    tmg_rcpt* pto = &_ptV[_rcV[k]._to_idx];
    if (pfr->_c2pinpt) {
      if (!(pto->_pinpt || pto->_c2pinpt)) {
        pto->_next_for_clear = _first_for_clear;
        _first_for_clear = pto;
      }
      pto->_c2pinpt = true;
    }
    if (pto->_c2pinpt) {
      if (!(pfr->_pinpt || pfr->_c2pinpt)) {
        pfr->_next_for_clear = _first_for_clear;
        _first_for_clear = pfr;
      }
      pfr->_c2pinpt = true;
    }
  }

  tmg_rcterm* x = &_termV[j];
  for (int ii = 0; ii < _csN; ii++) {
    const int k = (*_csV)[ii].k;
    const int bfr = _rcV[k]._from_idx;
    const int bto = _rcV[k]._to_idx;
    const bool cfr = _ptV[bfr]._pinpt;
    const bool cto = _ptV[bto]._pinpt;
    if (soft && !cfr && !cto) {
      if (!(_ptV[bfr]._c2pinpt || _ptV[bto]._c2pinpt)) {
        connectTermSoft(j, (*_csV)[ii].rtlev, (*_csV)[ii].rect, (*_csV)[ii].k);
      }
      continue;
    }
    if (cfr && !cto) {
      tmg_rcpt* pt = &_ptV[bfr];
      const tmg_rcpt* pother = &_ptV[bto];
      if (pt->_tindex == j) {
        continue;
      }
      if (pt->_tindex >= 0 && pt->_t_alt && pt->_t_alt->_tindex < 0) {
        const int oldt = pt->_tindex;
        removePointFromTerm(pt, &_termV[oldt]);
        pt->_t_alt->_tindex = oldt;
        addPointToTerm(pt->_t_alt, &_termV[oldt]);
        pt->_tindex = -1;
      }
      if (pt->_tindex >= 0 && pt->_tindex == pother->_tindex) {
        // override old connection if it is on the other
        removePointFromTerm(pt, &_termV[pt->_tindex]);
        pt->_tindex = -1;
      }
      if (pt->_tindex >= 0) {
        logger_->error(ODB,
                       390,
                       "order_wires failed: net {}, shorts to another term at "
                       "wire point ({} {})",
                       _net->getName(),
                       pt->_x,
                       pt->_y);
      }
      pt->_tindex = j;
      addPointToTerm(pt, x);

    } else if (cto && !cfr) {
      tmg_rcpt* pt = &_ptV[bto];
      const tmg_rcpt* pother = &_ptV[bfr];
      if (pt->_tindex == j) {
        continue;
      }
      if (pt->_tindex >= 0 && pt->_t_alt && pt->_t_alt->_tindex < 0) {
        const int oldt = pt->_tindex;
        removePointFromTerm(pt, &_termV[oldt]);
        pt->_t_alt->_tindex = oldt;
        addPointToTerm(pt->_t_alt, &_termV[oldt]);
        pt->_tindex = -1;
      }
      if (pt->_tindex >= 0 && pt->_tindex == pother->_tindex) {
        // override old connection if it is on the other
        removePointFromTerm(pt, &_termV[pt->_tindex]);
        pt->_tindex = -1;
      }
      if (pt->_tindex >= 0) {
        logger_->error(ODB,
                       391,
                       "order_wires failed: net {}, shorts to another term at "
                       "wire point ({} {})",
                       _net->getName(),
                       pt->_x,
                       pt->_y);
      }
      pt->_tindex = j;
      addPointToTerm(pt, x);

    } else if (cfr && cto) {
      if (_ptV[bfr]._tindex == j || _ptV[bto]._tindex == j) {
        continue;
      }
      if (_ptV[bfr]._tindex >= 0 && _ptV[bto]._tindex < 0) {
        tmg_rcpt* pt = &_ptV[bto];
        pt->_tindex = j;
        addPointToTerm(pt, x);
        continue;
      }
      tmg_rcpt* pt = &_ptV[bfr];
      tmg_rcpt* pother = &_ptV[bto];
      if (pt->_tindex >= 0 && pt->_t_alt && pt->_t_alt->_tindex < 0) {
        const int oldt = pt->_tindex;
        removePointFromTerm(pt, &_termV[oldt]);
        pt->_t_alt->_tindex = oldt;
        addPointToTerm(pt->_t_alt, &_termV[oldt]);
        pt->_tindex = -1;
      }
      if (pt->_tindex >= 0 && pt->_tindex == pother->_tindex) {
        // override old connection if it is on the other
        removePointFromTerm(pt, &_termV[pt->_tindex]);
        pt->_tindex = -1;
      }
      if (pt->_tindex >= 0) {
        logger_->error(ODB,
                       392,
                       "order_wires failed: net {}, shorts to another term at "
                       "wire point ({} {})",
                       _net->getName(),
                       pt->_x,
                       pt->_y);
      }
      pt->_tindex = j;
      addPointToTerm(pt, x);
      pt->_t_alt = pother;
    }
  }
  for (tmg_rcpt* pc = _first_for_clear; pc; pc = pc->_next_for_clear) {
    pc->_pinpt = false;
    pc->_c2pinpt = false;
  }
  _first_for_clear = nullptr;
}

void tmg_conn::connectTermSoft(const int j,
                               const int rt,
                               Rect& rect,
                               const int k)
{
  const tmg_rc_sh* sb = &(_rcV[k]._shape);
  const int bfr = _rcV[k]._from_idx;
  const int bto = _rcV[k]._to_idx;
  const int xlo = std::max(rect.xMin(), sb->xMin());
  const int ylo = std::max(rect.yMin(), sb->yMin());
  const int xhi = std::min(rect.xMax(), sb->xMax());
  const int yhi = std::min(rect.yMax(), sb->yMax());
  const int xc = (xlo + xhi) / 2;
  const int yc = (ylo + yhi) / 2;
  bool choose_bfr = false;
  bool has_alt = true;
  if (sb->isVia()) {
    choose_bfr = (rt == _ptV[bfr]._layer->getRoutingLevel());
    has_alt = false;
  } else {
    const int dbfr = abs(_ptV[bfr]._x - xc) + abs(_ptV[bfr]._y - yc);
    const int dbto = abs(_ptV[bto]._x - xc) + abs(_ptV[bto]._y - yc);
    choose_bfr = (dbfr < dbto);
    if (abs(dbfr - dbto) > 5000) {
      has_alt = false;
    }
  }
  tmg_rcpt* pt = &_ptV[choose_bfr ? bfr : bto];
  tmg_rcpt* pother = &_ptV[choose_bfr ? bto : bfr];
  if (pt->_tindex == j) {
    return;
  }

  // This was needed in a case where a square patch
  // of M1 was used to connect pins A and B of an instance.
  // The original input def looked like:
  // NEW M1 ( 2090900 1406000 ) ( * 1406000 ) NEW M1 ...
  // In this case we get two _ptV[] points, that have identical
  // x,y,layer, and we connect one iterm to each.
  if (pt->_tindex >= 0 && _ptV[bfr]._x == _ptV[bto]._x
      && _ptV[bfr]._y == _ptV[bto]._y) {
    // if wire shape k is an isolated square,
    // then connect to other point if available
    if (pother->_tindex == j) {
      return;  // already connected
    }
    if (pother->_tindex < 0) {
      pt = pother;
      has_alt = false;
    }
  }

  // override old connection if it is on the other
  if (pt->_tindex >= 0 && pother->_tindex == pt->_tindex) {
    removePointFromTerm(pt, &_termV[pt->_tindex]);
    pt->_tindex = -1;
  }

  if (pt->_tindex >= 0 && pother->_tindex < 0 && pt->_layer == pother->_layer) {
    const int dist = abs(pt->_x - pother->_x) + abs(pt->_y - pother->_y);
    if (dist < 5000) {
      pt = pother;
      has_alt = false;
    }
  }

  if (pt->_tindex >= 0) {
    return;  // skip soft if conflicts with hard
  }
  pt->_tindex = j;
  tmg_rcterm* x = &_termV[j];
  addPointToTerm(pt, x);
  pt->_fre = false;
  if (has_alt) {
    pt->_t_alt = pother;
  }
}

// find a driver iterm, or any bterm, or any iterm, or default to the first
// point
int tmg_conn::getStartNode()
{
  dbITerm* it_drv;
  dbBTerm* bt_drv;
  tmg_getDriveTerm(_net, &it_drv, &bt_drv);
  for (const tmg_rcterm& x : _termV) {
    if (x._iterm == it_drv && x._bterm == bt_drv) {
      if (!x._pt) {
        return 0;
      }
      return (x._pt - _ptV.data());
    }
  }
  return 0;
}

void tmg_conn::analyzeNet(dbNet* net)
{
  if (net->isWireOrdered()) {
    _net = net;
    checkConnOrdered();
  } else {
    loadNet(net);
    if (net->getWire()) {
      loadWire(net->getWire());
    }
    if (_ptV.empty()) {
      // ignoring this net
      net->setDisconnected(false);
      net->setWireOrdered(false);
      return;
    }
    findConnections();
    bool noConvert = false;
    if (_hasSWire) {
      net->destroySWires();
    }
    relocateShorts();
    treeReorder(noConvert);
  }
  net->setDisconnected(!_connected);
  net->setWireOrdered(true);
}

bool tmg_conn::checkConnected()
{
  for (const tmg_rcterm& x : _termV) {
    if (x._pt == nullptr) {
      return false;
    }
  }
  if (_termV.empty()) {
    return true;
  }
  _tstackV.clear();
  int jstart = getStartNode();
  tmg_rcterm* xstart = nullptr;
  if (_ptV[jstart]._tindex >= 0) {
    tmg_rcterm* x = &_termV[_ptV[jstart]._tindex];
    xstart = x;
    _tstackV.push_back(x);
  }
  dfsClear();
  if (!dfsStart(jstart)) {
    return false;
  }
  int tstack0 = 0;
  while (true) {
    // do a physically-connected subtree
    int jfr, jto, k;
    bool is_short, is_loop;
    while (dfsNext(&jfr, &jto, &k, &is_short, &is_loop)) {
      if (_ptV[jto]._tindex >= 0) {
        tmg_rcterm* x = &_termV[_ptV[jto]._tindex];
        if (x == xstart && !is_short) {
          // removing multi-connection at driver
          removePointFromTerm(&_ptV[jto], &_termV[_ptV[jto]._tindex]);
          _ptV[jto]._tindex = -1;
          _ptV[jto]._t_alt = nullptr;
        } else if (x->_pt && x->_pt->_next_for_term) {
          // add potential short-from points to stack
          _tstackV.push_back(x);
        }
      }
      // the part of addToWire needed in no_convert case
      if (_ptV[jfr]._tindex >= 0) {
        tmg_rcterm* x = &_termV[_ptV[jfr]._tindex];
        if (x->_first_pt == nullptr) {
          x->_first_pt = &_ptV[jfr];
        }
      }
      if (_ptV[jto]._tindex >= 0) {
        tmg_rcterm* x = &_termV[_ptV[jto]._tindex];
        if (x->_first_pt == nullptr) {
          x->_first_pt = &_ptV[jto];
        }
      }
    }
    // finished physically-connected subtree,
    // find an unvisited short-from point
    tmg_rcpt* pt = nullptr;
    while (tstack0 < _tstackV.size() && !pt) {
      tmg_rcterm* x = _tstackV[tstack0++];
      for (pt = x->_pt; pt; pt = pt->_next_for_term) {
        if (!isVisited(pt - _ptV.data())) {
          break;
        }
      }
    }
    if (pt) {
      tstack0--;
    }
    if (!pt) {
      break;
    }
    jstart = pt - _ptV.data();
    if (!dfsStart(jstart)) {
      return false;
    }
  }
  bool con = true;
  for (tmg_rcterm& x : _termV) {
    if (!x._first_pt) {
      con = false;
    }
    x._first_pt = nullptr;  // cleanup
  }
  return con;  // all terms connected, may be floating pieces of wire
}

void tmg_conn::treeReorder(const bool no_convert)
{
  _connected = true;
  _need_short_wire_id = false;
  if (_ptV.empty()) {
    return;
  }
  _newWire = nullptr;
  _last_id = -1;
  if (!no_convert) {
    _newWire = _net->getWire();
    if (!_newWire) {
      _newWire = dbWire::create(_net);
    }
    _encoder.begin(_newWire);
    for (tmg_rcpt& pt : _ptV) {
      pt._dbwire_id = -1;
    }
  }
  for (tmg_rcterm& x : _termV) {
    x._first_pt = nullptr;
    if (x._pt == nullptr) {
      _connected = false;
    }
  }

  if (_termV.empty()) {
    return;
  }

  _net_rule = _net->getNonDefaultRule();
  _path_rule = _net_rule;

  int tstack0 = 0;
  _tstackV.clear();
  int jstart = getStartNode();
  tmg_rcterm* xstart = nullptr;
  if (_ptV[jstart]._tindex >= 0) {
    tmg_rcterm* x = &_termV[_ptV[jstart]._tindex];
    xstart = x;
    _tstackV.push_back(x);
  }
  dfsClear();
  if (!dfsStart(jstart)) {
    logger_->warn(ODB, 395, "cannot order {}", _net->getConstName());
    return;
  }
  int last_term_index = 0;
  while (true) {
    // do a physically-connected subtree
    tmg_rcterm* x = nullptr;
    int jfr, jto, k;
    bool is_short, is_loop;
    while (dfsNext(&jfr, &jto, &k, &is_short, &is_loop)) {
      x = nullptr;
      if (_ptV[jto]._tindex >= 0) {
        x = &_termV[_ptV[jto]._tindex];
        if (x == xstart && !is_short) {
          // removing multi-connection at driver
          removePointFromTerm(&_ptV[jto], &_termV[_ptV[jto]._tindex]);
          _ptV[jto]._tindex = -1;
          _ptV[jto]._t_alt = nullptr;
        } else if (x->_pt && x->_pt->_next_for_term) {
          // add potential short-from points to stack
          _tstackV.push_back(x);
        }
      }
      if (!no_convert) {
        addToWire(jfr, jto, k, is_short, is_loop);
      } else {
        // the part of addToWire needed in no_convert case
        if (_ptV[jfr]._tindex >= 0) {
          x = &_termV[_ptV[jfr]._tindex];
          if (x->_first_pt == nullptr) {
            x->_first_pt = &_ptV[jfr];
          }
        }
        if (_ptV[jto]._tindex >= 0) {
          x = &_termV[_ptV[jto]._tindex];
          if (x->_first_pt == nullptr) {
            x->_first_pt = &_ptV[jto];
          }
        }
      }
    }
    // finished physically-connected subtree,
    // find an unvisited short-from point
    tmg_rcpt* pt = nullptr;
    while (tstack0 < _tstackV.size() && !pt) {
      x = _tstackV[tstack0++];
      for (pt = x->_pt; pt; pt = pt->_next_for_term) {
        if (!isVisited(pt - _ptV.data())) {
          break;
        }
      }
    }
    if (pt) {
      tstack0--;
      _last_id = x->_first_pt ? x->_first_pt->_dbwire_id : -1;
    }
    if (!pt) {
      int j;
      for (j = last_term_index; j < _termV.size(); j++) {
        x = &_termV[j];
        if (x->_pt && !isVisited(x->_pt - _ptV.data())) {
          break;
        }
      }
      last_term_index = j;
      if (j < _termV.size()) {
        // disconnected, start new path from another term
        _connected = false;
        _last_id = -1;
        _tstackV.push_back(x);
        pt = x->_pt;
      } else {
        jstart = getDisconnectedStart();
        if (jstart < 0) {
          break;  // normal exit, no more subtrees
        }
        pt = &_ptV[jstart];
        _last_id = -1;
      }
    }
    jstart = pt - _ptV.data();
    if (!dfsStart(jstart)) {
      logger_->warn(ODB, 396, "cannot order {}", _net->getConstName());
      return;
    }
  }

  checkVisited();
  if (!no_convert) {
    _encoder.end();
  }
}

int tmg_conn::getExtension(const int ipt, const tmg_rc* rc)
{
  const tmg_rcpt* p = &_ptV[ipt];
  tmg_rcpt* pto;
  if (ipt == rc->_from_idx) {
    pto = &_ptV[rc->_to_idx];
  } else if (ipt == rc->_to_idx) {
    pto = &_ptV[rc->_from_idx];
  } else {
    logger_->error(ODB, 16, "problem in getExtension()");
  }
  int ext = rc->_default_ext;
  if (p->_x < pto->_x) {
    ext = p->_x - rc->_shape.xMin();
  } else if (p->_x > pto->_x) {
    ext = rc->_shape.xMax() - p->_x;
  } else if (p->_y < pto->_y) {
    ext = p->_y - rc->_shape.yMin();
  } else if (p->_y > pto->_y) {
    ext = rc->_shape.yMax() - p->_y;
  }
  return ext;
}

int tmg_conn::addPoint(const int ipt, const tmg_rc* rc)
{
  int wire_id;
  const tmg_rcpt* p = &_ptV[ipt];
  const int ext = getExtension(ipt, rc);
  if (ext == rc->_default_ext) {
    wire_id = _encoder.addPoint(p->_x, p->_y);
  } else {
    wire_id = _encoder.addPoint(p->_x, p->_y, ext);
  }
  return wire_id;
}

int tmg_conn::addPoint(const int from_idx, const int ipt, const tmg_rc* rc)
{
  int wire_id;
  const tmg_rcpt* p = &_ptV[ipt];
  const int ext = getExtension(ipt, rc);
  if (ext == rc->_default_ext) {
    wire_id = _encoder.addPoint(p->_x, p->_y);
  } else {
    wire_id = _encoder.addPoint(p->_x, p->_y, ext);
  }
  return wire_id;
}

int tmg_conn::addPointIfExt(const int ipt, const tmg_rc* rc)
{
  // for first wire after a via, need to add a point
  // only if the extension is not the default ext
  int wire_id = 0;
  const tmg_rcpt* p = &_ptV[ipt];
  const int ext = getExtension(ipt, rc);
  if (ext != rc->_default_ext) {
    wire_id = _encoder.addPoint(p->_x, p->_y, ext);
  }
  return wire_id;
}

void tmg_conn::addToWire(const int fr,
                         const int to,
                         const int k,
                         const bool is_short,
                         const bool is_loop)
{
  if (!_newWire) {
    return;
  }

  const int xfr = _ptV[fr]._x;
  const int yfr = _ptV[fr]._y;
  const int xto = _ptV[to]._x;
  const int yto = _ptV[to]._y;

  if (is_short) {
    if (xfr != xto || yfr != yto) {
      _ptV[to]._dbwire_id = -1;
      _last_id = _ptV[fr]._dbwire_id;
      return;
    }
    if (_ptV[fr]._dbwire_id < 0) {
      _need_short_wire_id = true;
      return;
    }
    _ptV[to]._dbwire_id = _ptV[fr]._dbwire_id;
    return;
  }
  if (k < 0) {
    logger_->error(
        ODB, 393, "tmg_conn::addToWire: value of k is negative: {}", k);
  }

  tmg_rc* rc = (k >= 0) ? &_rcV[k] : nullptr;
  int fr_id = _ptV[fr]._dbwire_id;
  dbTechLayerRule* lyr_rule = nullptr;
  if (rc->_shape.getRule()) {
    lyr_rule = rc->_shape.getRule()->getLayerRule(_ptV[fr]._layer);
  }
  if (fr_id < 0) {
    _path_rule = rc->_shape.getRule();
    _firstSegmentAfterVia = 0;
    if (_last_id >= 0) {
      // term feedthru
      if (_path_rule) {
        _encoder.newPathShort(
            _last_id, _ptV[fr]._layer, dbWireType::ROUTED, lyr_rule);
      } else {
        _encoder.newPathShort(_last_id, _ptV[fr]._layer, dbWireType::ROUTED);
      }
    } else {
      if (_path_rule) {
        _encoder.newPath(_ptV[fr]._layer, dbWireType::ROUTED, lyr_rule);
      } else {
        _encoder.newPath(_ptV[fr]._layer, dbWireType::ROUTED);
      }
    }
    if (!rc->_shape.isVia()) {
      fr_id = addPoint(fr, rc);
    } else {
      fr_id = _encoder.addPoint(xfr, yfr);
    }
    _ptV[fr]._dbwire_id = fr_id;
    if (_ptV[fr]._tindex >= 0) {
      tmg_rcterm* x = &_termV[_ptV[fr]._tindex];
      if (x->_first_pt == nullptr) {
        x->_first_pt = &_ptV[fr];
      }
      if (x->_iterm) {
        _encoder.addITerm(x->_iterm);
      } else {
        _encoder.addBTerm(x->_bterm);
      }
    }
  } else if (fr_id != _last_id) {
    _path_rule = rc->_shape.getRule();
    if (rc->_shape.isVia()) {
      if (_path_rule) {
        _encoder.newPath(fr_id, lyr_rule);
      } else {
        _encoder.newPath(fr_id);
      }
    } else {
      _firstSegmentAfterVia = 0;
      const int ext = getExtension(fr, rc);
      if (ext != rc->_default_ext) {
        if (_path_rule) {
          _encoder.newPathExt(fr_id, ext, lyr_rule);
        } else {
          _encoder.newPathExt(fr_id, ext);
        }
      } else {
        if (_path_rule) {
          _encoder.newPath(fr_id, lyr_rule);
        } else {
          _encoder.newPath(fr_id);
        }
      }
    }
    if (_ptV[fr]._tindex >= 0) {
      tmg_rcterm* x = &_termV[_ptV[fr]._tindex];
      if (x->_first_pt == nullptr) {
        x->_first_pt = &_ptV[fr];
      }
      if (x->_iterm) {
        _encoder.addITerm(x->_iterm);
      } else {
        _encoder.addBTerm(x->_bterm);
      }
    }
  } else if (_path_rule != rc->_shape.getRule()) {
    // make a branch, for taper

    _path_rule = rc->_shape.getRule();
    if (rc->_shape.isVia()) {
      if (_path_rule) {
        _encoder.newPath(fr_id, lyr_rule);
      } else {
        _encoder.newPath(fr_id);
      }
    } else {
      _firstSegmentAfterVia = 0;
      const int ext = getExtension(fr, rc);
      if (ext != rc->_default_ext) {
        if (_path_rule) {
          _encoder.newPathExt(fr_id, ext, lyr_rule);
        } else {
          _encoder.newPathExt(fr_id, ext);
        }
      } else {
        if (_path_rule) {
          _encoder.newPath(fr_id, lyr_rule);
        } else {
          _encoder.newPath(fr_id);
        }
      }
    }
    if (_ptV[fr]._tindex >= 0) {
      tmg_rcterm* x = &_termV[_ptV[fr]._tindex];
      if (x->_first_pt == nullptr) {
        x->_first_pt = &_ptV[fr];
      }
      if (x->_iterm) {
        _encoder.addITerm(x->_iterm);
      } else {
        _encoder.addBTerm(x->_bterm);
      }
    }

    // end taper branch
  }

  if (_need_short_wire_id) {
    copyWireIdToVisitedShorts(fr);
    _need_short_wire_id = false;
  }

  int to_id = -1;
  if (!rc->_shape.isVia()) {
    if (_firstSegmentAfterVia) {
      _firstSegmentAfterVia = 0;
      addPointIfExt(fr, rc);
    }
    to_id = addPoint(fr, to, rc);
  } else if (rc->_shape.getTechVia()) {
    to_id = _encoder.addTechVia(rc->_shape.getTechVia());
  } else if (rc->_shape.getVia()) {
    to_id = _encoder.addVia(rc->_shape.getVia());
  } else {
    logger_->error(ODB, 18, "error in addToWire");
  }

  if (_ptV[to]._tindex >= 0 && _ptV[to]._tindex != _ptV[fr]._tindex
      && _ptV[to]._t_alt && _ptV[to]._t_alt->_tindex < 0
      && !isVisited(_ptV[to]._t_alt - _ptV.data())) {
    // move an ambiguous connection to the later point
    // this is for receiver; we should not get here for driver
    tmg_rcpt* pother = _ptV[to]._t_alt;
    pother->_tindex = _ptV[to]._tindex;
    _ptV[to]._tindex = -1;
  }

  if (_ptV[to]._tindex >= 0) {
    tmg_rcterm* x = &_termV[_ptV[to]._tindex];
    if (x->_first_pt == nullptr) {
      x->_first_pt = &_ptV[to];
    }
    if (x->_iterm) {
      _encoder.addITerm(x->_iterm);
    } else {
      _encoder.addBTerm(x->_bterm);
    }
  }

  _ptV[to]._dbwire_id = to_id;
  _last_id = to_id;

  _firstSegmentAfterVia = rc->_shape.isVia();
}

}  // namespace odb
