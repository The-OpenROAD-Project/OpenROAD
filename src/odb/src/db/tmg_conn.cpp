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

#include "tmg_conn.h"

#include <cstdio>
#include <cstdlib>

#include "db.h"
#include "dbShape.h"
#include "dbWireCodec.h"
#include "utl/Logger.h"

namespace odb {

using utl::ODB;

tmg_rcpt::tmg_rcpt()
    : _x(0),
      _y(0),
      _layer(nullptr),
      _tindex(-1),
      _next_for_term(nullptr),
      _t_alt(nullptr),
      _next_for_clear(nullptr),
      _sring(nullptr),
      _dbwire_id(-1),
      _fre(false),
      _jct(false),
      _pinpt(false),
      _c2pinpt(false)
{
}

static void tmg_getDriveTerm(dbNet* net, dbITerm** iterm, dbBTerm** bterm)
{
  *iterm = nullptr;
  *bterm = nullptr;
  dbSet<dbITerm> iterms = net->getITerms();
  dbSet<dbITerm>::iterator iterm_itr;
  dbITerm* it;
  dbITerm* it_inout = nullptr;
  for (iterm_itr = iterms.begin(); iterm_itr != iterms.end(); ++iterm_itr) {
    it = *iterm_itr;
    if (it->getIoType() == dbIoType::OUTPUT) {
      *iterm = it;
      return;
    }
    if (it->getIoType() == dbIoType::INOUT && !it_inout)
      it_inout = it;
  }
  dbSet<dbBTerm> bterms = net->getBTerms();
  dbSet<dbBTerm>::iterator bterm_itr;
  dbBTerm* bt;
  dbBTerm* bt_inout = nullptr;
  for (bterm_itr = bterms.begin(); bterm_itr != bterms.end(); ++bterm_itr) {
    bt = *bterm_itr;
    if (bt->getIoType() == dbIoType::INPUT) {
      *bterm = bt;
      return;
    }
    if (bt->getIoType() == dbIoType::INOUT && !bt_inout)
      bt_inout = bt;
  }
  if (bt_inout) {
    *bterm = bt_inout;
    return;
  }
  if (it_inout) {
    *iterm = it_inout;
    return;
  }
  if (bterms.begin() != bterms.end()) {
    *bterm = *bterms.begin();
    return;
  }
  if (iterms.begin() != iterms.end()) {
    *iterm = *iterms.begin();
    return;
  }
}

tmg_conn::tmg_conn(utl::Logger* logger) : logger_(logger)
{
  _rcV.reserve(1024);
  _termNmax = 1024;
  _termV = (tmg_rcterm*) malloc(_termNmax * sizeof(tmg_rcterm));
  _tstackV = (tmg_rcterm**) malloc(_termNmax * sizeof(tmg_rcterm*));
  _csVV.resize(_termNmax);
  _csNV = (int*) malloc(_termNmax * sizeof(int));
  _shortNmax = 1024;
  _shortV = (tmg_rcshort*) malloc(_shortNmax * sizeof(tmg_rcshort));
  _search = nullptr;
  _graph = nullptr;
  _max_length = 0;
  _cut_length = 0;
  _cut_end_extMin = 1;
  _need_short_wire_id = 0;
  _first_for_clear = nullptr;
  _preserveSWire = false;
  _swireNetCnt = 0;
}

int tmg_conn::ptDist(int fr, int to)
{
  return abs(_ptV[fr]._x - _ptV[to]._x) + abs(_ptV[fr]._y - _ptV[to]._y);
}

tmg_rcpt* tmg_conn::allocPt()
{
  _ptV.emplace_back();
  return &_ptV.back();
}

void tmg_conn::addRc(dbShape& s, int ifr, int ito)
{
  tmg_rc x;
  x._ifr = ifr;
  x._ito = ito;
  x._shape._rect.reset(s.xMin(), s.yMin(), s.xMax(), s.yMax());
  x._shape._layer = s.getTechLayer();
  x._shape._tech_via = s.getTechVia();
  x._shape._block_via = s.getVia();
  x._shape._rule = nullptr;
  if (x._shape._tech_via || x._shape._block_via) {
    x._vert = 0;
    x._width = 0;
  } else if (_ptV[ifr]._x != _ptV[ito]._x) {
    x._vert = 0;
    x._width = s.yMax() - s.yMin();
  } else if (_ptV[ifr]._y != _ptV[ito]._y) {
    x._vert = 1;
    x._width = s.xMax() - s.xMin();
  } else if (s.xMax() - s.xMin() == s.yMax() - s.yMin()) {
    x._vert = 0;
    x._width = s.xMax() - s.xMin();
  } else {
    x._vert = 0;
    x._width = 0;
  }
  x._default_ext = x._width / 2;
  _rcV.push_back(x);
}

void tmg_conn::addRc(int k,
                     tmg_rc_sh& s,
                     int ifr,
                     int ito,
                     int xmin,
                     int ymin,
                     int xmax,
                     int ymax)
{
  tmg_rc x;
  x._ifr = ifr;
  x._ito = ito;
  x._shape._rect.reset(xmin, ymin, xmax, ymax);
  x._shape._layer = s._layer;
  x._shape._tech_via = s._tech_via;
  x._shape._block_via = s._block_via;
  x._shape._rule = s._rule;
  x._vert = _rcV[k]._vert;
  x._width = _rcV[k]._width;
  x._default_ext = x._width / 2;
  _rcV.push_back(x);
}

tmg_rc* tmg_conn::addRcPatch(int ifr, int ito)
{
  dbTechLayer* layer = _ptV[ifr]._layer;
  if (!layer || layer != _ptV[ito]._layer
      || (_ptV[ifr]._x != _ptV[ito]._x && _ptV[ifr]._y != _ptV[ito]._y)) {
    return nullptr;
  }
  tmg_rc x;
  x._ifr = ifr;
  x._ito = ito;
  x._shape._layer = layer;
  x._shape._tech_via = nullptr;
  x._shape._block_via = nullptr;
  x._width = layer->getWidth();  // trouble for nondefault
  x._vert = (_ptV[ifr]._y != _ptV[ito]._y);
  int xlo, ylo, xhi, yhi;
  if (x._vert) {
    xlo = _ptV[ifr]._x;
    xhi = xlo;
    if (_ptV[ifr]._y < _ptV[ito]._y) {
      ylo = _ptV[ifr]._y;
      yhi = _ptV[ito]._y;
    } else {
      ylo = _ptV[ito]._y;
      yhi = _ptV[ifr]._y;
    }
  } else {
    ylo = _ptV[ifr]._y;
    yhi = ylo;
    if (_ptV[ifr]._x < _ptV[ito]._x) {
      xlo = _ptV[ifr]._x;
      xhi = _ptV[ito]._x;
    } else {
      xlo = _ptV[ito]._x;
      xhi = _ptV[ifr]._x;
    }
  }
  int hw = x._width / 2;
  x._default_ext = hw;
  x._shape._rect.reset(xlo - hw, ylo - hw, xhi + hw, yhi + hw);
  _rcV.push_back(x);
  return &_rcV.back();
}
void tmg_conn::addITerm(dbITerm* iterm)
{
  if (_termN == _termNmax) {
    _termNmax *= 2;
    _termV = (tmg_rcterm*) realloc(_termV, _termNmax * sizeof(tmg_rcterm));
    _tstackV
        = (tmg_rcterm**) realloc(_tstackV, _termNmax * sizeof(tmg_rcterm*));
    _csVV.resize(_termNmax);
    _csNV = (int*) realloc(_csNV, _termNmax * sizeof(int));
  }
  tmg_rcterm* x = _termV + _termN++;
  x->_iterm = iterm;
  x->_bterm = nullptr;
  x->_pt = nullptr;
  x->_first_pt = nullptr;
}

void tmg_conn::addBTerm(dbBTerm* bterm)
{
  if (_termN == _termNmax) {
    _termNmax *= 2;
    _termV = (tmg_rcterm*) realloc(_termV, _termNmax * sizeof(tmg_rcterm));
    _tstackV
        = (tmg_rcterm**) realloc(_tstackV, _termNmax * sizeof(tmg_rcterm*));
    _csVV.resize(_termNmax);
    _csNV = (int*) realloc(_csNV, _termNmax * sizeof(int));
  }
  tmg_rcterm* x = _termV + _termN++;
  x->_iterm = nullptr;
  x->_bterm = bterm;
  x->_pt = nullptr;
  x->_first_pt = nullptr;
}

void tmg_conn::addShort(int i0, int i1)
{
  if (_shortN == _shortNmax) {
    _shortNmax *= 2;
    _shortV = (tmg_rcshort*) realloc(_shortV, _shortNmax * sizeof(tmg_rcshort));
  }
  tmg_rcshort* x = _shortV + _shortN++;
  x->_i0 = i0;
  x->_i1 = i1;
  x->_skip = false;
  if (_ptV[i0]._fre)
    _ptV[i0]._fre = 0;
  else
    _ptV[i0]._jct = 1;
  if (_ptV[i1]._fre)
    _ptV[i1]._fre = 0;
  else
    _ptV[i1]._jct = 1;
}

void tmg_conn::loadNet(dbNet* net)
{
  _net = net;
  _rcV.clear();
  _ptV.clear();
  _termN = 0;
  _shortN = 0;
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
  if (swires.size() == 0)
    return;

  _hasSWire = true;
  dbShape shape;
  int x1, y1, x2, y2;
  dbTechLayer* layer1 = nullptr;
  dbTechLayer* layer2 = nullptr;
  tmg_rcpt* pt;
  for (dbSWire* sw : swires) {
    for (dbSBox* sbox : sw->getWires()) {
      Rect rect = sbox->getBox();
      if (sbox->isVia()) {
        x1 = (rect.xMin() + rect.xMax()) / 2;
        x2 = x1;
        y1 = (rect.yMin() + rect.yMax()) / 2;
        y2 = y1;
        dbTechVia* tech_via = sbox->getTechVia();
        dbVia* via = sbox->getBlockVia();
        if (tech_via) {
          layer1 = tech_via->getTopLayer();
          layer2 = tech_via->getBottomLayer();
          shape.setVia(tech_via, rect);
        } else if (via) {
          layer1 = via->getTopLayer();
          layer2 = via->getBottomLayer();
          shape.setVia(via, rect);
        }
      } else {
        if (rect.xMax() - rect.xMin() > rect.yMax() - rect.yMin()) {
          y1 = (rect.yMin() + rect.yMax()) / 2;
          y2 = y1;
          x1 = rect.xMin() + (rect.yMax() - y1);
          x2 = rect.xMax() - (rect.yMax() - y1);
        } else {
          x1 = (rect.xMin() + rect.xMax()) / 2;
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
        pt = allocPt();
        pt->_x = x1;
        pt->_y = y1;
        pt->_layer = layer1;
      }

      pt = allocPt();
      pt->_x = x2;
      pt->_y = y2;
      pt->_layer = layer2;
      addRc(shape, _ptV.size() - 2, _ptV.size() - 1);
      _rcV.back()._shape._rule = nullptr;
    }
  }
}

void tmg_conn::loadWire(dbWire* wire)
{
  dbWirePathItr pitr;
  dbWirePath path;
  dbWirePathShape pathShape;
  _ptV.clear();
  pitr.begin(wire);
  while (pitr.getNextPath(path)) {
    if (_ptV.empty() || path.layer != _ptV.back()._layer
        || path.point.getX() != _ptV.back()._x
        || path.point.getY() != _ptV.back()._y) {
      auto pt = allocPt();
      pt->_x = path.point.getX();
      pt->_y = path.point.getY();
      pt->_layer = path.layer;
    }
    while (pitr.getNextShape(pathShape)) {
      auto pt = allocPt();
      pt->_x = pathShape.point.getX();
      pt->_y = pathShape.point.getY();
      pt->_layer = pathShape.layer;
      addRc(pathShape.shape, _ptV.size() - 2, _ptV.size() - 1);
      _rcV.back()._shape._rule = path.rule;
    }
  }

  loadSWire(wire->getNet());
}

void tmg_conn::splitBySj(int j,
                         tmg_rc_sh* sj,
                         int rt,
                         int sjxMin,
                         int sjyMin,
                         int sjxMax,
                         int sjyMax)
{
  int k, klast, nxmin, nymin, nxmax, nymax, endTo;
  tmg_rcpt* pt;
  int isVia = sj->isVia() ? 1 : 0;
  _search->searchStart(rt, sjxMin, sjyMin, sjxMax, sjyMax, isVia);
  klast = -1;
  while (_search->searchNext(&k))
    if (k != klast && k != j) {
      if (_rcV[j]._ito == _rcV[k]._ifr || _rcV[j]._ifr == _rcV[k]._ito)
        continue;
      if (!sj->isVia() && _rcV[j]._vert == _rcV[k]._vert)
        continue;
      tmg_rc_sh* sk = &(_rcV[k]._shape);
      if (sk->isVia())
        continue;
      dbTechLayer* tlayer = _ptV[_rcV[k]._ifr]._layer;
      nxmin = sk->xMin();
      nxmax = sk->xMax();
      nymin = sk->yMin();
      nymax = sk->yMax();
      int x;
      int y;
      if (_rcV[k]._vert) {
        if (sjyMin - sk->yMin() < _rcV[k]._width)
          continue;
        if (sk->yMax() - sjyMax < _rcV[k]._width)
          continue;
        _vertSplitCnt++;
        ;
        if (_ptV[_rcV[k]._ifr]._y > _ptV[_rcV[k]._ito]._y) {
          _rcV[k]._shape.setYmin(_ptV[_rcV[j]._ifr]._y - _rcV[k]._width / 2);
          nymax = _ptV[_rcV[j]._ifr]._y + _rcV[k]._width / 2;
        } else {
          _rcV[k]._shape.setYmax(_ptV[_rcV[j]._ifr]._y + _rcV[k]._width / 2);
          nymin = _ptV[_rcV[j]._ifr]._y - _rcV[k]._width / 2;
        }
        x = _ptV[_rcV[k]._ifr]._x;
        y = _ptV[_rcV[j]._ifr]._y;
      } else {
        if (sjxMin - sk->xMin() < _rcV[k]._width)
          continue;
        if (sk->xMax() - sjxMax < _rcV[k]._width)
          continue;
        _horzSplitCnt++;
        if (_ptV[_rcV[k]._ifr]._x > _ptV[_rcV[k]._ito]._x) {
          _rcV[k]._shape.setXmin(_ptV[_rcV[j]._ifr]._x - _rcV[k]._width / 2);
          nxmax = _ptV[_rcV[j]._ifr]._x + _rcV[k]._width / 2;
        } else {
          _rcV[k]._shape.setXmax(_ptV[_rcV[j]._ifr]._x + _rcV[k]._width / 2);
          nxmin = _ptV[_rcV[j]._ifr]._x - _rcV[k]._width / 2;
        }
        x = _ptV[_rcV[j]._ifr]._x;
        y = _ptV[_rcV[k]._ifr]._y;
      }
      klast = k;
      pt = allocPt();
      pt->_x = x;
      pt->_y = y;
      pt->_layer = tlayer;
      pt->_tindex = -1;
      pt->_t_alt = nullptr;
      pt->_next_for_term = nullptr;
      pt->_pinpt = 0;
      pt->_c2pinpt = 0;
      pt->_next_for_clear = nullptr;
      pt->_sring = nullptr;
      endTo = _rcV[k]._ito;
      _rcV[k]._ito = _ptV.size() - 1;
      // create new tmg_rc
      addRc(k,
            _rcV[k]._shape,
            _ptV.size() - 1,
            endTo,
            nxmin,
            nymin,
            nxmax,
            nymax);
      _search->addShape(rt, nxmin, nymin, nxmax, nymax, 0, _rcV.size() - 1);
    }
}

void tmg_conn::splitTtop()
{
  // split top of T shapes (for GALET created def file)
  dbTechLayer *layb, *layt;
  int rt, rt_b, rt_t;
  int via_x, via_y;
  dbSet<dbBox> boxes;

  for (unsigned long j = 0; j < _rcV.size(); j++) {
    tmg_rc_sh* sj = &(_rcV[j]._shape);
    if (sj->isVia()) {
      via_x = _ptV[_rcV[j]._ifr]._x;
      via_y = _ptV[_rcV[j]._ifr]._y;
      layb = nullptr;
      layt = nullptr;
      dbTechVia* tv = sj->getTechVia();
      if (tv) {
        layb = tv->getBottomLayer();
        layt = tv->getTopLayer();
        boxes = tv->getBoxes();
      } else {
        dbVia* vv = sj->getVia();
        layb = vv->getBottomLayer();
        layt = vv->getTopLayer();
        boxes = vv->getBoxes();
      }
      rt_b = layb->getRoutingLevel();
      rt_t = layt->getRoutingLevel();
      dbSet<dbBox>::iterator bitr;
      dbBox* b;
      for (bitr = boxes.begin(); bitr != boxes.end(); ++bitr) {
        b = *bitr;
        if (b->getTechLayer() == layb) {
          splitBySj(j,
                    sj,
                    rt_b,
                    via_x + b->xMin(),
                    via_y + b->yMin(),
                    via_x + b->xMax(),
                    via_y + b->yMax());
        } else if (b->getTechLayer() == layt) {
          splitBySj(j,
                    sj,
                    rt_t,
                    via_x + b->xMin(),
                    via_y + b->yMin(),
                    via_x + b->xMax(),
                    via_y + b->yMax());
        }
      }
    } else {
      rt = sj->getTechLayer()->getRoutingLevel();
      splitBySj(j, sj, rt, sj->xMin(), sj->yMin(), sj->xMax(), sj->yMax());
    }
  }
}

void tmg_conn::setSring()
{
  int ii;
  for (ii = 0; ii < _shortN; ii++)
    if (!_shortV[ii]._skip) {
      tmg_rcpt* pfr = &_ptV[_shortV[ii]._i0];
      tmg_rcpt* pto = &_ptV[_shortV[ii]._i1];
      tmg_rcpt* x;
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
        x = pfr->_sring;
        while (x->_sring != pfr && x != pto)
          x = x->_sring;
        if (x == pto)
          continue;
        x->_sring = pto;
        x = pto;
        while (x->_sring != pto)
          x = x->_sring;
        x->_sring = pfr;
      }
    }
}

void tmg_conn::detachTilePins()
{
  int j, k, rtlb, rtli;
  int x1, y1, x2, y2;
  tmg_rcterm* tx;
  dbBTerm* bterm;
  dbShape pin;
  dbTechVia* tv;
  _slicedTilePinCnt = 0;
  bool sliceDone;
  for (j = 0; j < _termN; j++) {
    tx = _termV + j;
    if (tx->_iterm)
      continue;
    bterm = tx->_bterm;
    if (!bterm->getFirstPin(pin) || pin.isVia())
      continue;
    Rect rectb = pin.getBox();
    rtlb = pin.getTechLayer()->getRoutingLevel();
    sliceDone = false;
    for (k = 0; !sliceDone && k < _termN; k++) {
      tx = _termV + k;
      if (tx->_bterm)
        continue;
      dbMTerm* mterm = tx->_iterm->getMTerm();
      int px, py;
      tx->_iterm->getInst()->getOrigin(px, py);
      Point origin = Point(px, py);
      dbOrientType orient = tx->_iterm->getInst()->getOrient();
      dbTransform transform(orient, origin);
      dbSet<dbMPin> mpins = mterm->getMPins();
      dbSet<dbMPin>::iterator mpin_itr;
      for (mpin_itr = mpins.begin(); !sliceDone && mpin_itr != mpins.end();
           mpin_itr++) {
        dbMPin* mpin = *mpin_itr;
        dbSet<dbBox> boxes = mpin->getGeometry();
        dbSet<dbBox>::iterator box_itr;
        for (box_itr = boxes.begin(); !sliceDone && box_itr != boxes.end();
             box_itr++) {
          dbBox* box = *box_itr;
          Rect recti = box->getBox();
          transform.apply(recti);
          if (box->isVia()) {
            tv = box->getTechVia();
            rtli = tv->getTopLayer()->getRoutingLevel();
            if (rtli <= 1)
              continue;
            if (rtli != rtlb) {
              rtli = tv->getBottomLayer()->getRoutingLevel();
              if (rtli == 0 || rtli != rtlb)
                continue;
            }
          } else {
            rtli = box->getTechLayer()->getRoutingLevel();
            if (rtli != rtlb)
              continue;
          }
          if (recti.contains(rectb))
            logger_->error(
                ODB, 420, "tmg_conn::detachTilePins: tilepin inside iterm.");

          if (!recti.overlaps(rectb))
            continue;
          x1 = rectb.xMin();
          y1 = rectb.yMin();
          x2 = rectb.xMax();
          y2 = rectb.yMax();
          if (x2 > recti.xMax() && x1 > recti.xMin())
            x1 = recti.xMax();
          else if (x1 < recti.xMin() && x2 < recti.xMax())
            x2 = recti.xMin();
          else if (y2 > recti.yMax() && y1 > recti.yMin())
            y1 = recti.yMax();
          else if (y1 < recti.yMin() && y2 < recti.yMax())
            y2 = recti.yMin();
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
  int ii;
  for (ii = 0; ii < _slicedTilePinCnt; ii++) {
    if (_slicedTileBTerm[ii] == bterm) {
      rect.reset(_stbtx1[ii], _stbty1[ii], _stbtx2[ii], _stbty2[ii]);
      return;
    }
  }
  rect = pin.getBox();
  return;
}

void tmg_conn::findConnections()
{
  if (_ptV.empty())
    return;
  if (!_search)
    _search = new tmg_conn_search();
  _search->clear();
  int k, klast, rt;
  dbTechVia* tv;
  dbTechLayer *layb, *layt;
  int rt_b, rt_t;
  int via_x, via_y;
  dbSet<dbBox> boxes;

  for (int j = 0; j < _ptV.size(); j++) {
    _ptV[j]._fre = 1;
    _ptV[j]._jct = 0;
    _ptV[j]._pinpt = 0;
    _ptV[j]._c2pinpt = 0;
    _ptV[j]._next_for_clear = nullptr;
    _ptV[j]._sring = nullptr;
  }
  _first_for_clear = nullptr;
  for (unsigned long j = 0; j < _rcV.size() - 1; j++)
    if (_rcV[j]._ito == _rcV[j + 1]._ifr)
      _ptV[_rcV[j]._ito]._fre = 0;

  // put wires in search
  for (unsigned long j = 0; j < _rcV.size(); j++) {
    tmg_rc_sh* s = &(_rcV[j]._shape);
    if (s->isVia()) {
      via_x = _ptV[_rcV[j]._ifr]._x;
      via_y = _ptV[_rcV[j]._ifr]._y;

      layb = nullptr;
      layt = nullptr;
      tv = s->getTechVia();
      if (tv) {
        layb = tv->getBottomLayer();
        layt = tv->getTopLayer();
        boxes = tv->getBoxes();
      } else {
        dbVia* vv = s->getVia();
        layb = vv->getBottomLayer();
        layt = vv->getTopLayer();
        boxes = vv->getBoxes();
      }
      rt_b = layb->getRoutingLevel();
      rt_t = layt->getRoutingLevel();
      dbSet<dbBox>::iterator bitr;
      dbSet<dbBox>::iterator bend = boxes.end();
      dbBox* b;
      for (bitr = boxes.begin(); bitr != bend; ++bitr) {
        b = *bitr;
        if (b->getTechLayer() == layb) {
          _search->addShape(rt_b,
                            via_x + b->xMin(),
                            via_y + b->yMin(),
                            via_x + b->xMax(),
                            via_y + b->yMax(),
                            1,
                            j);
        } else if (b->getTechLayer() == layt) {
          _search->addShape(rt_t,
                            via_x + b->xMin(),
                            via_y + b->yMin(),
                            via_x + b->xMax(),
                            via_y + b->yMax(),
                            1,
                            j);
        }
      }

    } else {
      rt = s->getTechLayer()->getRoutingLevel();
      _search->addShape(rt, s->xMin(), s->yMin(), s->xMax(), s->yMax(), 0, j);
    }
  }

  if (_rcV.size() < 10000)
    splitTtop();

  // find self-intersections of wires
  for (int j = 0; j < (int) _rcV.size() - 1; j++) {
    tmg_rc_sh* s = &(_rcV[j]._shape);
    int conn_next = (_rcV[j]._ito == _rcV[j + 1]._ifr);
    if (s->isVia()) {
      via_x = _ptV[_rcV[j]._ifr]._x;
      via_y = _ptV[_rcV[j]._ifr]._y;

      layb = nullptr;
      layt = nullptr;
      dbTechVia* tv = s->getTechVia();
      if (tv) {
        layb = tv->getBottomLayer();
        layt = tv->getTopLayer();
        boxes = tv->getBoxes();
      } else {
        dbVia* vv = s->getVia();
        layb = vv->getBottomLayer();
        layt = vv->getTopLayer();
        boxes = vv->getBoxes();
      }
      rt_b = layb->getRoutingLevel();
      rt_t = layt->getRoutingLevel();
      dbSet<dbBox>::iterator bitr;
      dbSet<dbBox>::iterator bend = boxes.end();
      dbBox* b;
      for (bitr = boxes.begin(); bitr != bend; ++bitr) {
        b = *bitr;
        if (b->getTechLayer() == layb) {
          _search->searchStart(rt_b,
                               via_x + b->xMin(),
                               via_y + b->yMin(),
                               via_x + b->xMax(),
                               via_y + b->yMax(),
                               1);
        } else if (b->getTechLayer() == layt) {
          _search->searchStart(rt_t,
                               via_x + b->xMin(),
                               via_y + b->yMin(),
                               via_x + b->xMax(),
                               via_y + b->yMax(),
                               1);
        } else {
          continue;  // cut layer
        }
        klast = -1;
        while (_search->searchNext(&k))
          if (k != klast && k > j) {
            if (k == j + 1 && conn_next)
              continue;
            klast = k;
            connectShapes(j, k);
          }
      }

    } else {
      rt = s->getTechLayer()->getRoutingLevel();
      _search->searchStart(rt, s->xMin(), s->yMin(), s->xMax(), s->yMax(), 0);
      klast = -1;
      while (_search->searchNext(&k))
        if (k != klast && k > j) {
          if (k == j + 1 && conn_next)
            continue;
          klast = k;
          connectShapes(j, k);
        }
    }
  }

  removeWireLoops();

  // detach tilPins from iterms
  detachTilePins();

  // connect pins
  for (int j = 0; j < _termN; j++) {
    _csV = &_csVV[j];
    _csN = 0;
    tmg_rcterm* x = _termV + j;
    if (x->_iterm) {
      dbMTerm* mterm = x->_iterm->getMTerm();
      int px, py;
      x->_iterm->getInst()->getOrigin(px, py);
      Point origin = Point(px, py);
      dbOrientType orient = x->_iterm->getInst()->getOrient();
      dbTransform transform(orient, origin);
      dbSet<dbMPin> mpins = mterm->getMPins();
      dbSet<dbMPin>::iterator mpin_itr;
      for (mpin_itr = mpins.begin(); mpin_itr != mpins.end(); mpin_itr++) {
        dbMPin* mpin = *mpin_itr;
        dbSet<dbBox> boxes = mpin->getGeometry();
        dbSet<dbBox>::iterator box_itr;
        int ipass;
        for (ipass = 0; ipass < 2; ipass++)
          for (box_itr = boxes.begin(); box_itr != boxes.end(); box_itr++) {
            dbBox* box = *box_itr;
            if (ipass == 1 && box->isVia()) {
              tv = box->getTechVia();
              rt_t = tv->getTopLayer()->getRoutingLevel();
              if (rt_t <= 1)
                continue;  // skipping V01
              Rect rect = box->getBox();
              transform.apply(rect);
              _search->searchStart(
                  rt_t, rect.xMin(), rect.yMin(), rect.xMax(), rect.yMax(), 2);
              klast = -1;
              while (_search->searchNext(&k))
                if (k != klast) {
                  klast = k;
                  int ii;
                  for (ii = 0; ii < _csN; ii++)
                    if (k == (*_csV)[ii].k)
                      break;
                  if (ii < _csN)
                    continue;
                  if (_csN == 32)
                    break;
                  (*_csV)[_csN].k = k;
                  (*_csV)[_csN].rect = rect;
                  (*_csV)[_csN].rtlev = rt_t;
                  _csN++;
                }
              rt_b = tv->getBottomLayer()->getRoutingLevel();
              if (rt_b == 0)
                continue;
              _search->searchStart(
                  rt_b, rect.xMin(), rect.yMin(), rect.xMax(), rect.yMax(), 2);
              klast = -1;
              while (_search->searchNext(&k))
                if (k != klast) {
                  klast = k;
                  int ii;
                  for (ii = 0; ii < _csN; ii++)
                    if (k == (*_csV)[ii].k)
                      break;
                  if (ii < _csN)
                    continue;
                  if (_csN == 32)
                    break;
                  (*_csV)[_csN].k = k;
                  (*_csV)[_csN].rect = rect;
                  (*_csV)[_csN].rtlev = rt_b;
                  _csN++;
                }
            } else if (ipass == 0 && !box->isVia()) {
              rt = box->getTechLayer()->getRoutingLevel();
              Rect rect = box->getBox();
              transform.apply(rect);
              _search->searchStart(
                  rt, rect.xMin(), rect.yMin(), rect.xMax(), rect.yMax(), 2);
              klast = -1;
              while (_search->searchNext(&k))
                if (k != klast) {
                  klast = k;
                  int ii;
                  for (ii = 0; ii < _csN; ii++)
                    if (k == (*_csV)[ii].k)
                      break;
                  if (ii < _csN && _csN >= 8)
                    continue;
                  if (_csN == 32)
                    break;
                  (*_csV)[_csN].k = k;
                  (*_csV)[_csN].rect = rect;
                  (*_csV)[_csN].rtlev = rt;
                  _csN++;
                }
            }
          }
      }  // mpins
    } else {
      // bterm
      dbShape pin;
      if (x->_bterm->getFirstPin(pin)) {  // TWG: added bpins
        if (pin.isVia()) {
          // TODO
        } else {
          rt = pin.getTechLayer()->getRoutingLevel();
          Rect rect;
          getBTermSearchBox(x->_bterm, pin, rect);
          _search->searchStart(
              rt, rect.xMin(), rect.yMin(), rect.xMax(), rect.yMax(), 2);
          klast = -1;
          while (_search->searchNext(&k))
            if (k != klast) {
              klast = k;
              int ii;
              for (ii = 0; ii < _csN; ii++)
                if (k == (*_csV)[ii].k)
                  break;
              if (ii < _csN)
                continue;
              if (_csN == 32)
                break;
              (*_csV)[_csN].k = k;
              (*_csV)[_csN].rect = rect;
              (*_csV)[_csN].rtlev = rt;
              _csN++;
            }
        }
      }
    }
    _csNV[j] = _csN;
  }

  for (int j = 0; j < _ptV.size(); j++) {
    tmg_rcpt* pc = &_ptV[j];
    pc->_pinpt = 0;
    pc->_c2pinpt = 0;
    pc->_next_for_clear = nullptr;
    pc->_sring = nullptr;
  }
  setSring();

  for (int j = 0; j < _termN; j++) {
    connectTerm(j, false);
  }
  bool ok = checkConnected();
  if (!ok) {
    for (int j = 0; j < _termN; j++) {
      connectTerm(j, true);
    }
  }

  // make terms of shorted points consistent
  int it;
  for (it = 0; it < 5; it++) {
    int cnt = 0;
    for (int j = 0; j < _shortN; j++)
      if (!_shortV[j]._skip) {
        int i0 = _shortV[j]._i0;
        int i1 = _shortV[j]._i1;
        if (_ptV[i0]._tindex < 0 && _ptV[i1]._tindex >= 0) {
          _ptV[i0]._tindex = _ptV[i1]._tindex;
          cnt++;
        }
        if (_ptV[i1]._tindex < 0 && _ptV[i0]._tindex >= 0) {
          _ptV[i1]._tindex = _ptV[i0]._tindex;
          cnt++;
        }
      }
    if (!cnt)
      break;
  }
}

void tmg_conn::connectShapes(int j, int k)
{
  tmg_rc_sh* sa = &(_rcV[j]._shape);
  tmg_rc_sh* sb = &(_rcV[k]._shape);
  int afr = _rcV[j]._ifr;
  int ato = _rcV[j]._ito;
  int bfr = _rcV[k]._ifr;
  int bto = _rcV[k]._ito;
  int xlo, ylo, xhi, yhi, xc, yc;
  xlo = sa->xMin();
  if (sb->xMin() > xlo)
    xlo = sb->xMin();
  ylo = sa->yMin();
  if (sb->yMin() > ylo)
    ylo = sb->yMin();
  xhi = sa->xMax();
  if (sb->xMax() < xhi)
    xhi = sb->xMax();
  yhi = sa->yMax();
  if (sb->yMax() < yhi)
    yhi = sb->yMax();
  xc = (xlo + xhi) / 2;
  yc = (ylo + yhi) / 2;
  int choose_afr = 0;
  int choose_bfr = 0;
  if (sa->isVia() && sb->isVia()) {
    if (_ptV[afr]._layer == _ptV[bfr]._layer) {
      choose_afr = 1;
      choose_bfr = 1;
    } else if (_ptV[afr]._layer == _ptV[bto]._layer) {
      choose_afr = 1;
      choose_bfr = 0;
    } else if (_ptV[ato]._layer == _ptV[bfr]._layer) {
      choose_afr = 0;
      choose_bfr = 1;
    } else if (_ptV[ato]._layer == _ptV[bto]._layer) {
      choose_afr = 0;
      choose_bfr = 0;
    }
  } else if (sa->isVia()) {
    choose_afr = (_ptV[afr]._layer == _ptV[bfr]._layer);
    xc = _ptV[afr]._x;
    yc = _ptV[afr]._y;  // same for afr and ato
    int dbfr = abs(_ptV[bfr]._x - xc) + abs(_ptV[bfr]._y - yc);
    int dbto = abs(_ptV[bto]._x - xc) + abs(_ptV[bto]._y - yc);
    choose_bfr = (dbfr < dbto);
  } else if (sb->isVia()) {
    choose_bfr = (_ptV[afr]._layer == _ptV[bfr]._layer);
    xc = _ptV[bfr]._x;
    yc = _ptV[bfr]._y;
    int dafr = abs(_ptV[afr]._x - xc) + abs(_ptV[afr]._y - yc);
    int dato = abs(_ptV[ato]._x - xc) + abs(_ptV[ato]._y - yc);
    choose_afr = (dafr < dato);
  } else {
    // get distances to the center of intersection region, (xc,yc)
    int dafr = abs(_ptV[afr]._x - xc) + abs(_ptV[afr]._y - yc);
    int dato = abs(_ptV[ato]._x - xc) + abs(_ptV[ato]._y - yc);
    choose_afr = (dafr < dato);
    int dbfr = abs(_ptV[bfr]._x - xc) + abs(_ptV[bfr]._y - yc);
    int dbto = abs(_ptV[bto]._x - xc) + abs(_ptV[bto]._y - yc);
    choose_bfr = (dbfr < dbto);
  }
  int i0, i1;
  i0 = (choose_afr ? afr : ato);
  i1 = (choose_bfr ? bfr : bto);
  if (i1 < i0) {
    int itmp = i0;
    i0 = i1;
    i1 = itmp;
  }
  addShort(i0, i1);
  _ptV[i0]._fre = 0;
  _ptV[i1]._fre = 0;
}

static void addPointToTerm(tmg_rcpt* pt, tmg_rcterm* x)
{
  tmg_rcpt* tpt = x->_pt;
  tmg_rcpt* ptpt = nullptr;
  while (tpt && (pt->_x > tpt->_x || (pt->_x == tpt->_x && pt->_y > tpt->_y))) {
    ptpt = tpt;
    tpt = tpt->_next_for_term;
  }
  if (ptpt)
    ptpt->_next_for_term = pt;
  else
    x->_pt = pt;
  pt->_next_for_term = tpt;
}

static void removePointFromTerm(tmg_rcpt* pt, tmg_rcterm* x)
{
  if (x->_pt == pt) {
    x->_pt = pt->_next_for_term;
    pt->_next_for_term = nullptr;
    return;
  }
  tmg_rcpt *ptpt = nullptr, *tpt;
  for (tpt = x->_pt; tpt; tpt = tpt->_next_for_term) {
    if (tpt == pt)
      break;
    ptpt = tpt;
  }
  if (!tpt)
    return;  // error, not found
  ptpt->_next_for_term = tpt->_next_for_term;
  pt->_next_for_term = nullptr;
}

void tmg_conn::connectTerm(int j, bool soft)
{
  _csV = &_csVV[j];
  _csN = _csNV[j];
  if (!_csN)
    return;
  tmg_rcterm* x = _termV + j;
  int ii;
  tmg_rcpt* pc;
  for (pc = _first_for_clear; pc; pc = pc->_next_for_clear) {
    pc->_pinpt = 0;
    pc->_c2pinpt = 0;
  }
  _first_for_clear = nullptr;

  for (ii = 0; ii < _csN; ii++) {
    int k = (*_csV)[ii].k;
    tmg_rcpt* pfr = &_ptV[_rcV[k]._ifr];
    tmg_rcpt* pto = &_ptV[_rcV[k]._ito];
    Point afr(pfr->_x, pfr->_y);
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
      pfr->_pinpt = 1;
      pfr->_c2pinpt = 1;
      pto->_c2pinpt = 1;
    }
    Point ato(pto->_x, pto->_y);
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
      pto->_pinpt = 1;
      pto->_c2pinpt = 1;
      pfr->_c2pinpt = 1;
    }
  }

#if 1
  {
    tmg_rcpt *pc, *x;
    for (pc = _first_for_clear; pc; pc = pc->_next_for_clear)
      if (pc->_sring) {
        int c2pinpt = pc->_c2pinpt;
        for (x = pc->_sring; x != pc; x = x->_sring)
          if (x->_c2pinpt)
            c2pinpt = 1;
        if (c2pinpt) {
          for (x = pc->_sring; x != pc; x = x->_sring) {
            if (!(x->_pinpt || x->_c2pinpt)) {
              x->_next_for_clear = _first_for_clear;
              _first_for_clear = x;
            }
            x->_c2pinpt = 1;
          }
        }
      }
  }
#else
  for (ii = 0; ii < _shortN; ii++)
    if (!_shortV[ii]._skip) {
      tmg_rcpt* pfr = _ptV + _shortV[ii]._i0;
      tmg_rcpt* pto = _ptV + _shortV[ii]._i1;
      if (pfr->_c2pinpt) {
        if (!(pto->_pinpt || pto->_c2pinpt)) {
          pto->_next_for_clear = _first_for_clear;
          _first_for_clear = pto;
        }
        pto->_c2pinpt = 1;
      }
      if (pto->_c2pinpt) {
        if (!(pfr->_pinpt || pfr->_c2pinpt)) {
          pfr->_next_for_clear = _first_for_clear;
          _first_for_clear = pfr;
        }
        pfr->_c2pinpt = 1;
      }
    }
#endif

  for (ii = 0; ii < _csN; ii++) {
    int k = (*_csV)[ii].k;
    tmg_rcpt* pfr = &_ptV[_rcV[k]._ifr];
    tmg_rcpt* pto = &_ptV[_rcV[k]._ito];
    if (pfr->_c2pinpt) {
      if (!(pto->_pinpt || pto->_c2pinpt)) {
        pto->_next_for_clear = _first_for_clear;
        _first_for_clear = pto;
      }
      pto->_c2pinpt = 1;
    }
    if (pto->_c2pinpt) {
      if (!(pfr->_pinpt || pfr->_c2pinpt)) {
        pfr->_next_for_clear = _first_for_clear;
        _first_for_clear = pfr;
      }
      pfr->_c2pinpt = 1;
    }
  }

  for (ii = 0; ii < _csN; ii++) {
    int k = (*_csV)[ii].k;
    int bfr = _rcV[k]._ifr;
    int bto = _rcV[k]._ito;
    tmg_rcpt *pt, *pother;
    bool cfr = _ptV[bfr]._pinpt;
    bool cto = _ptV[bto]._pinpt;
    if (soft && !cfr && !cto) {
      if (!(_ptV[bfr]._c2pinpt || _ptV[bto]._c2pinpt))
        connectTermSoft(j, (*_csV)[ii].rtlev, (*_csV)[ii].rect, (*_csV)[ii].k);
      continue;
    }
    if (cfr && !cto) {
      pt = &_ptV[bfr];
      pother = &_ptV[bto];
      if (pt->_tindex == j)
        continue;
      if (pt->_tindex >= 0 && pt->_t_alt && pt->_t_alt->_tindex < 0) {
        int oldt = pt->_tindex;
        removePointFromTerm(pt, _termV + oldt);
        pt->_t_alt->_tindex = oldt;
        addPointToTerm(pt->_t_alt, _termV + oldt);
        pt->_tindex = -1;
      }
      if (pt->_tindex >= 0 && pt->_tindex == pother->_tindex) {
        // override old connection if it is on the other
        removePointFromTerm(pt, _termV + pt->_tindex);
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
      pt = &_ptV[bto];
      pother = &_ptV[bfr];
      if (pt->_tindex == j)
        continue;
      if (pt->_tindex >= 0 && pt->_t_alt && pt->_t_alt->_tindex < 0) {
        int oldt = pt->_tindex;
        removePointFromTerm(pt, _termV + oldt);
        pt->_t_alt->_tindex = oldt;
        addPointToTerm(pt->_t_alt, _termV + oldt);
        pt->_tindex = -1;
      }
      if (pt->_tindex >= 0 && pt->_tindex == pother->_tindex) {
        // override old connection if it is on the other
        removePointFromTerm(pt, _termV + pt->_tindex);
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
      if (_ptV[bfr]._tindex == j || _ptV[bto]._tindex == j)
        continue;
      if (_ptV[bfr]._tindex >= 0 && _ptV[bto]._tindex < 0) {
        pt = &_ptV[bto];
        pt->_tindex = j;
        addPointToTerm(pt, x);
        continue;
      }
      pt = &_ptV[bfr];
      pother = &_ptV[bto];
      if (pt->_tindex >= 0 && pt->_t_alt && pt->_t_alt->_tindex < 0) {
        int oldt = pt->_tindex;
        removePointFromTerm(pt, _termV + oldt);
        pt->_t_alt->_tindex = oldt;
        addPointToTerm(pt->_t_alt, _termV + oldt);
        pt->_tindex = -1;
      }
      if (pt->_tindex >= 0 && pt->_tindex == pother->_tindex) {
        // override old connection if it is on the other
        removePointFromTerm(pt, _termV + pt->_tindex);
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
  for (pc = _first_for_clear; pc; pc = pc->_next_for_clear) {
    pc->_pinpt = 0;
    pc->_c2pinpt = 0;
  }
  _first_for_clear = nullptr;
}

void tmg_conn::connectTermSoft(int j, int rt, Rect& rect, int k)
{
  tmg_rcterm* x = _termV + j;
  tmg_rc_sh* sb = &(_rcV[k]._shape);
  int bfr = _rcV[k]._ifr;
  int bto = _rcV[k]._ito;
  int xlo, ylo, xhi, yhi, xc, yc;
  xlo = rect.xMin();
  if (sb->xMin() > xlo)
    xlo = sb->xMin();
  ylo = rect.yMin();
  if (sb->yMin() > ylo)
    ylo = sb->yMin();
  xhi = rect.xMax();
  if (sb->xMax() < xhi)
    xhi = sb->xMax();
  yhi = rect.yMax();
  if (sb->yMax() < yhi)
    yhi = sb->yMax();
  xc = (xlo + xhi) / 2;
  yc = (ylo + yhi) / 2;
  int choose_bfr = 0;
  bool has_alt = true;
  if (sb->isVia()) {
    choose_bfr = (rt == _ptV[bfr]._layer->getRoutingLevel());
    has_alt = false;
  } else {
    int dbfr = abs(_ptV[bfr]._x - xc) + abs(_ptV[bfr]._y - yc);
    int dbto = abs(_ptV[bto]._x - xc) + abs(_ptV[bto]._y - yc);
    choose_bfr = (dbfr < dbto);
    if (abs(dbfr - dbto) > 5000)
      has_alt = false;
  }
  tmg_rcpt* pt = &_ptV[choose_bfr ? bfr : bto];
  tmg_rcpt* pother = &_ptV[choose_bfr ? bto : bfr];
  if (pt->_tindex == j) {
    return;
  }

  // This was needed in a case (ahb3) where a square patch
  // of M1 was used to connect pins A and B of an instance.
  // The original input def looked like:
  // NEW M1 ( 2090900 1406000 ) ( * 1406000 ) NEW M1 ...
  // In this case we get two _ptV[] points, that have identical
  // x,y,layer, and we connect one iterm to each.
  if (pt->_tindex >= 0 && _ptV[bfr]._x == _ptV[bto]._x
      && _ptV[bfr]._y == _ptV[bto]._y) {
    // if wire shape k is an isolated square,
    // then connect to other point if available
    if (pother->_tindex == j)
      return;  // already connected
    if (pother->_tindex < 0) {
      pt = pother;
      has_alt = false;
    }
  }

  // override old connection if it is on the other
  if (pt->_tindex >= 0 && pother->_tindex == pt->_tindex) {
    removePointFromTerm(pt, _termV + pt->_tindex);
    pt->_tindex = -1;
  }

  if (pt->_tindex >= 0 && pother->_tindex < 0 && pt->_layer == pother->_layer) {
    int dist = abs(pt->_x - pother->_x) + abs(pt->_y - pother->_y);
    if (dist < 5000) {  // increased from 500, wfs 120105
      pt = pother;
      has_alt = false;
    }
  }

  if (pt->_tindex >= 0) {
    return;  // skip soft if conflicts with hard
  }
  pt->_tindex = j;
  addPointToTerm(pt, x);
  pt->_fre = 0;
  if (has_alt)
    pt->_t_alt = pother;
}

int tmg_conn::getStartNode()
{
  // find a driver iterm
  // or any bterm
  // or any iterm
  // or default to the first point
  dbITerm* it_drv;
  dbBTerm* bt_drv;
  tmg_getDriveTerm(_net, &it_drv, &bt_drv);
  tmg_rcterm* x;
  int j;
  for (j = 0; j < _termN; j++) {
    x = _termV + j;
    if (x->_iterm == it_drv && x->_bterm == bt_drv) {
      if (!x->_pt)
        return 0;
      return (x->_pt - &_ptV[0]);
    }
  }
  return 0;
}

void tmg_conn::analyzeNet(dbNet* net)
{
  _max_length = 0;
  _cut_length = 0;
  if (net->isWireOrdered()) {
    _net = net;
    checkConnOrdered();
  } else {
    loadNet(net);
    if (net->getWire())
      loadWire(net->getWire());
    if (_ptV.empty()) {
      // ignoring this net
      net->setDisconnected(false);
      net->setWireOrdered(false);
      return;
    }
    findConnections();
    bool noConvert = false;
    if (_hasSWire) {
      if (_preserveSWire) {
        net->setDoNotTouch(true);
        noConvert = true;
        _swireNetCnt++;
      } else
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
  int j;
  tmg_rcterm *x, *xstart;
  for (j = 0; j < _termN; j++) {
    x = _termV + j;
    if (x->_pt == nullptr)
      return false;
  }
  if (_termN == 0)
    return true;
  int jstart = getStartNode();
  x = nullptr;
  xstart = nullptr;
  if (_ptV[jstart]._tindex >= 0) {
    x = _termV + _ptV[jstart]._tindex;
    xstart = x;
  }
  int jfr, jto, k;
  bool is_short, is_loop;
  int tstack0 = 0;
  int tstackN = 0;
  tmg_rcterm** tstackV = _tstackV;
  if (x)
    tstackV[tstackN++] = x;
  dfsClear();
  if (!dfsStart(jstart)) {
    return false;
  }
  while (1) {
    // do a physically-connected subtree
    while (dfsNext(&jfr, &jto, &k, &is_short, &is_loop)) {
      x = nullptr;
      if (_ptV[jto]._tindex >= 0) {
        x = _termV + _ptV[jto]._tindex;
        if (x == xstart && !is_short) {
          // removing multi-connection at driver
          removePointFromTerm(&_ptV[jto], _termV + _ptV[jto]._tindex);
          _ptV[jto]._tindex = -1;
          _ptV[jto]._t_alt = nullptr;
        } else if (x->_pt && x->_pt->_next_for_term) {
          // add potential short-from points to stack
          tstackV[tstackN++] = x;
        }
      }
      if (1) {
        // the part of addToWire needed in no_convert case
        if (_ptV[jfr]._tindex >= 0) {
          x = _termV + _ptV[jfr]._tindex;
          if (x->_first_pt == nullptr)
            x->_first_pt = &_ptV[jfr];
        }
        if (_ptV[jto]._tindex >= 0) {
          x = _termV + _ptV[jto]._tindex;
          if (x->_first_pt == nullptr)
            x->_first_pt = &_ptV[jto];
        }
      }
    }
    // finished physically-connected subtree,
    // find an unvisited short-from point
    tmg_rcpt* pt = nullptr;
    while (tstack0 < tstackN && !pt) {
      x = tstackV[tstack0++];
      for (pt = x->_pt; pt; pt = pt->_next_for_term)
        if (!isVisited(pt - &_ptV[0]))
          break;
    }
    if (pt) {
      tstack0--;
    }
    if (!pt) {
      break;
    }
    jstart = pt - &_ptV[0];
    if (!dfsStart(jstart)) {
      return false;
    }
  }
  bool con = true;
  for (j = 0; j < _termN; j++) {
    x = _termV + j;
    if (!x->_first_pt)
      con = false;
    x->_first_pt = nullptr;  // cleanup
  }
  return con;  // all terms connected, may be floating pieces of wire
}

void tmg_conn::treeReorder(bool no_convert)
{
  _connected = true;
  _need_short_wire_id = 0;
  if (_ptV.empty())
    return;
  _newWire = nullptr;
  _last_id = -1;
  int j;
  if (!no_convert) {
    _newWire = _net->getWire();
    if (!_newWire)
      _newWire = dbWire::create(_net);
    _encoder.begin(_newWire);
    for (j = 0; j < _ptV.size(); j++)
      _ptV[j]._dbwire_id = -1;
  }
  tmg_rcterm *x, *xstart;
  for (j = 0; j < _termN; j++) {
    x = _termV + j;
    x->_first_pt = nullptr;
    if (x->_pt == nullptr) {
      _connected = false;
    }
  }

  if (_termN == 0)
    return;

  _net_rule = _net->getNonDefaultRule();
  _path_rule = _net_rule;

  int jstart = getStartNode();
  x = nullptr;
  xstart = nullptr;
  if (_ptV[jstart]._tindex >= 0) {
    x = _termV + _ptV[jstart]._tindex;
    xstart = x;
  }
  int jfr, jto, k;
  bool is_short, is_loop;
  int tstack0 = 0;
  int tstackN = 0;
  tmg_rcterm** tstackV = _tstackV;
  if (x)
    tstackV[tstackN++] = x;
  dfsClear();
  if (!dfsStart(jstart)) {
    logger_->warn(ODB, 395, "cannot order {}", _net->getConstName());
    return;
  }
  int last_term_index = 0;
  while (1) {
    // do a physically-connected subtree
    while (dfsNext(&jfr, &jto, &k, &is_short, &is_loop)) {
      x = nullptr;
      if (_ptV[jto]._tindex >= 0) {
        x = _termV + _ptV[jto]._tindex;
        if (x == xstart && !is_short) {
          // removing multi-connection at driver
          removePointFromTerm(&_ptV[jto], _termV + _ptV[jto]._tindex);
          _ptV[jto]._tindex = -1;
          _ptV[jto]._t_alt = nullptr;
        } else if (x->_pt && x->_pt->_next_for_term) {
          // add potential short-from points to stack
          tstackV[tstackN++] = x;
        }
      }
      if (!no_convert) {
        addToWire(jfr, jto, k, is_short, is_loop);
      } else {
        // the part of addToWire needed in no_convert case
        if (_ptV[jfr]._tindex >= 0) {
          x = _termV + _ptV[jfr]._tindex;
          if (x->_first_pt == nullptr)
            x->_first_pt = &_ptV[jfr];
        }
        if (_ptV[jto]._tindex >= 0) {
          x = _termV + _ptV[jto]._tindex;
          if (x->_first_pt == nullptr)
            x->_first_pt = &_ptV[jto];
        }
      }
    }
    // finished physically-connected subtree,
    // find an unvisited short-from point
    tmg_rcpt* pt = nullptr;
    while (tstack0 < tstackN && !pt) {
      x = tstackV[tstack0++];
      for (pt = x->_pt; pt; pt = pt->_next_for_term)
        if (!isVisited(pt - &_ptV[0]))
          break;
    }
    if (pt) {
      tstack0--;
      _last_id = x->_first_pt ? x->_first_pt->_dbwire_id : -1;
    }
    if (!pt) {
      for (j = last_term_index; j < _termN; j++) {
        x = _termV + j;
        if (x->_pt && !isVisited(x->_pt - &_ptV[0]))
          break;
      }
      last_term_index = j;
      if (j < _termN) {
        // disconnected, start new path from another term
        _connected = false;
        _last_id = -1;
        tstackV[tstackN++] = x;
        pt = x->_pt;
      } else {
        jstart = getDisconnectedStart();
        if (jstart < 0)
          break;  // normal exit, no more subtrees
        pt = &_ptV[jstart];
        _last_id = -1;
      }
    }
    jstart = pt - &_ptV[0];
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

int tmg_conn::getExtension(int ipt, tmg_rc* rc)
{
  int ext = rc->_default_ext;
  tmg_rcpt* p = &_ptV[ipt];
  tmg_rcpt* pto;
  if (ipt == rc->_ifr)
    pto = &_ptV[rc->_ito];
  else if (ipt == rc->_ito)
    pto = &_ptV[rc->_ifr];
  else {
    logger_->error(ODB, 16, "problem in getExtension()");
  }
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

int tmg_conn::addPoint(int ipt, tmg_rc* rc)
{
  int wire_id, ext;
  tmg_rcpt* p = &_ptV[ipt];
  ext = getExtension(ipt, rc);
  if (ext == rc->_default_ext) {
    wire_id = _encoder.addPoint(p->_x, p->_y);
  } else {
    wire_id = _encoder.addPoint(p->_x, p->_y, ext);
  }
  return wire_id;
}

int tmg_conn::addPoint(int ifr, int ipt, tmg_rc* rc)
{
  int wire_id, ext;
  tmg_rcpt* p = &_ptV[ipt];
  ext = getExtension(ipt, rc);
  if (_max_length) {
    int fx = _ptV[ifr]._x;
    int fy = _ptV[ifr]._y;
    int delt = ext * 2 + _max_length;
    if (fx < p->_x) {
      while (p->_x - fx > delt) {
        fx += _max_length;
        _encoder.addPoint(fx, p->_y);
      }
    } else if (fx > p->_x) {
      while (p->_x + delt < fx) {
        fx -= _max_length;
        _encoder.addPoint(fx, p->_y);
      }
    } else if (fy < p->_y) {
      while (p->_y - fy > delt) {
        fy += _max_length;
        _encoder.addPoint(p->_x, fy);
      }
    } else if (fy > p->_y) {
      while (p->_y + delt < fy) {
        fy -= _max_length;
        _encoder.addPoint(p->_x, fy);
      }
    }
  }
  if (_cut_length) {
    int fx = _ptV[ifr]._x;
    int fy = _ptV[ifr]._y;
    int tx = _ptV[ipt]._x;
    int ty = _ptV[ipt]._y;
    int delt = ext * _cut_end_extMin;
    int dx = 0;
    int dy = 0;
    int sx, sy;
    bool onseg;
    if (fx < tx) {
      dx = _cut_length;
      sx = fx < 0 ? (fx / _cut_length) * _cut_length
                  : (fx / _cut_length + 1) * _cut_length;
    } else if (fx > tx) {
      dx = -_cut_length;
      sx = fx <= 0 ? (fx / _cut_length - 1) * _cut_length
                   : (fx / _cut_length) * _cut_length;
    } else if (fy < ty) {
      dy = _cut_length;
      sy = fy < 0 ? (fy / _cut_length) * _cut_length
                  : (fy / _cut_length + 1) * _cut_length;
    } else if (fy > ty) {
      dy = -_cut_length;
      sy = fy <= 0 ? (fy / _cut_length - 1) * _cut_length
                   : (fy / _cut_length) * _cut_length;
    }
    if (dx) {
      if (abs(sx - fx) < delt)
        sx += dx;
      onseg = (fx < sx && sx < tx) || (fx > sx && sx > tx);
      while (onseg && abs(sx - tx) >= delt) {
        _encoder.addPoint(sx, ty);
        sx += dx;
        onseg = (fx < sx && sx < tx) || (fx > sx && sx > tx);
      }
    }
    if (dy) {
      if (abs(sy - fy) < delt)
        sy += dy;
      onseg = (fy < sy && sy < ty) || (fy > sy && sy > ty);
      while (onseg && abs(sy - ty) >= delt) {
        _encoder.addPoint(tx, sy);
        sy += dy;
        onseg = (fy < sy && sy < ty) || (fy > sy && sy > ty);
      }
    }
  }
  if (ext == rc->_default_ext) {
    wire_id = _encoder.addPoint(p->_x, p->_y);
  } else {
    wire_id = _encoder.addPoint(p->_x, p->_y, ext);
  }
  return wire_id;
}

int tmg_conn::addPointIfExt(int ipt, tmg_rc* rc)
{
  // for first wire after a via, need to add a point
  // only if the extension is not the default ext
  int wire_id = 0, ext;
  tmg_rcpt* p = &_ptV[ipt];
  ext = getExtension(ipt, rc);
  if (ext != rc->_default_ext) {
    wire_id = _encoder.addPoint(p->_x, p->_y, ext);
  }
  return wire_id;
}

void tmg_conn::addToWire(int fr, int to, int k, bool is_short, bool is_loop)
{
  tmg_rc* rc = (k >= 0) ? &_rcV[k] : nullptr;
  tmg_rcterm* x;
  int xfr, yfr, xto, yto;
  xfr = _ptV[fr]._x;
  yfr = _ptV[fr]._y;
  xto = _ptV[to]._x;
  yto = _ptV[to]._y;

  if (!_newWire)
    return;

  if (is_short) {
    // wfs 4-19-05
    if (xfr != xto || yfr != yto) {
      _ptV[to]._dbwire_id = -1;
      _last_id = _ptV[fr]._dbwire_id;
      return;
    }
    if (_ptV[fr]._dbwire_id < 0) {
      ++_need_short_wire_id;
      return;
    }
    _ptV[to]._dbwire_id = _ptV[fr]._dbwire_id;
    return;
  }
  if (k < 0)
    logger_->error(
        ODB, 393, "tmg_conn::addToWire: value of k is negative: {}", k);

  int ext;
  int fr_id = _ptV[fr]._dbwire_id;
  dbTechLayerRule* lyr_rule = nullptr;
  if (rc->_shape._rule)
    lyr_rule = rc->_shape._rule->getLayerRule(_ptV[fr]._layer);
  if (fr_id < 0) {
    _path_rule = rc->_shape._rule;
    _firstSegmentAfterVia = 0;
    if (_last_id >= 0) {
      // term feedthru
      if (_path_rule)
        _encoder.newPathShort(
            _last_id, _ptV[fr]._layer, dbWireType::ROUTED, lyr_rule);
      else
        _encoder.newPathShort(_last_id, _ptV[fr]._layer, dbWireType::ROUTED);
    } else {
      if (_path_rule)
        _encoder.newPath(_ptV[fr]._layer, dbWireType::ROUTED, lyr_rule);
      else
        _encoder.newPath(_ptV[fr]._layer, dbWireType::ROUTED);
    }
    if (!rc->_shape.isVia()) {
      fr_id = addPoint(fr, rc);
    } else {
      fr_id = _encoder.addPoint(xfr, yfr);
    }
    _ptV[fr]._dbwire_id = fr_id;
    if (_ptV[fr]._tindex >= 0) {
      x = _termV + _ptV[fr]._tindex;
      if (x->_first_pt == nullptr)
        x->_first_pt = &_ptV[fr];
      if (x->_iterm)
        _encoder.addITerm(x->_iterm);
      else
        _encoder.addBTerm(x->_bterm);
    }
  } else if (fr_id != _last_id) {
    _path_rule = rc->_shape._rule;
    if (rc->_shape.isVia()) {
      if (_path_rule)
        _encoder.newPath(fr_id, lyr_rule);
      else
        _encoder.newPath(fr_id);
    } else {
      _firstSegmentAfterVia = 0;
      ext = getExtension(fr, rc);
      if (ext != rc->_default_ext) {
        if (_path_rule)
          _encoder.newPathExt(fr_id, ext, lyr_rule);
        else
          _encoder.newPathExt(fr_id, ext);
      } else {
        if (_path_rule)
          _encoder.newPath(fr_id, lyr_rule);
        else
          _encoder.newPath(fr_id);
      }
    }
    if (_ptV[fr]._tindex >= 0) {
      x = _termV + _ptV[fr]._tindex;
      if (x->_first_pt == nullptr)
        x->_first_pt = &_ptV[fr];
      if (x->_iterm)
        _encoder.addITerm(x->_iterm);
      else
        _encoder.addBTerm(x->_bterm);
    }
  } else if (_path_rule != rc->_shape._rule) {
    // make a branch, for taper

    _path_rule = rc->_shape._rule;
    if (rc->_shape.isVia()) {
      if (_path_rule)
        _encoder.newPath(fr_id, lyr_rule);
      else
        _encoder.newPath(fr_id);
    } else {
      _firstSegmentAfterVia = 0;
      ext = getExtension(fr, rc);
      if (ext != rc->_default_ext) {
        if (_path_rule)
          _encoder.newPathExt(fr_id, ext, lyr_rule);
        else
          _encoder.newPathExt(fr_id, ext);
      } else {
        if (_path_rule)
          _encoder.newPath(fr_id, lyr_rule);
        else
          _encoder.newPath(fr_id);
      }
    }
    if (_ptV[fr]._tindex >= 0) {
      x = _termV + _ptV[fr]._tindex;
      if (x->_first_pt == nullptr)
        x->_first_pt = &_ptV[fr];
      if (x->_iterm)
        _encoder.addITerm(x->_iterm);
      else
        _encoder.addBTerm(x->_bterm);
    }

    // end taper branch
  }

  if (_need_short_wire_id) {
    copyWireIdToVisitedShorts(fr);
    _need_short_wire_id = 0;
  }

  int to_id = -1;
  if (!rc->_shape.isVia()) {
    if (_firstSegmentAfterVia) {
      _firstSegmentAfterVia = 0;
      addPointIfExt(fr, rc);
    }
    to_id = addPoint(fr, to, rc);
  } else if (rc->_shape.getTechVia())
    to_id = _encoder.addTechVia(rc->_shape.getTechVia());
  else if (rc->_shape.getVia())
    to_id = _encoder.addVia(rc->_shape.getVia());
  else
    logger_->error(ODB, 18, "error in addToWire");

  if (_ptV[to]._tindex >= 0 && _ptV[to]._tindex != _ptV[fr]._tindex
      && _ptV[to]._t_alt && _ptV[to]._t_alt->_tindex < 0
      && !isVisited(_ptV[to]._t_alt - &_ptV[0])) {
    // move an ambiguous connection to the later point
    // this is for receiver; we should not get here for driver
    tmg_rcpt* pother = _ptV[to]._t_alt;
    pother->_tindex = _ptV[to]._tindex;
    _ptV[to]._tindex = -1;
  }

  if (_ptV[to]._tindex >= 0) {
    x = _termV + _ptV[to]._tindex;
    if (x->_first_pt == nullptr)
      x->_first_pt = &_ptV[to];
    if (x->_iterm)
      _encoder.addITerm(x->_iterm);
    else
      _encoder.addBTerm(x->_bterm);
  }

  _ptV[to]._dbwire_id = to_id;
  _last_id = to_id;

  _firstSegmentAfterVia = rc->_shape.isVia();
}

}  // namespace odb
