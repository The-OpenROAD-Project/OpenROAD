// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "tmg_conn.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <tuple>

#include "odb/db.h"
#include "odb/dbSet.h"
#include "odb/dbShape.h"
#include "odb/dbWireCodec.h"
#include "odb/isotropy.h"
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
  rcV_.reserve(1024);
  termV_.reserve(1024);
  tstackV_.reserve(1024);
  csVV_.reserve(1024);
  csNV_.reserve(1024);
  shortV_.reserve(1024);
  need_short_wire_id_ = false;
  first_for_clear_ = nullptr;
}

tmg_conn::~tmg_conn() = default;

int tmg_conn::ptDist(const int fr, const int to) const
{
  return abs(ptV_[fr].x - ptV_[to].x) + abs(ptV_[fr].y - ptV_[to].y);
}

tmg_rcpt* tmg_conn::allocPt(int x, int y, dbTechLayer* layer)
{
  return &ptV_.emplace_back(x, y, layer);
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
  } else if (ptV_[from_idx].x != ptV_[to_idx].x) {
    is_vertical = false;
    width = s.yMax() - s.yMin();
  } else if (ptV_[from_idx].y != ptV_[to_idx].y) {
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
  rcV_.push_back(x);
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
  const int width = rcV_[k].width;
  tmg_rc x(from_idx,
           to_idx,
           {{xmin, ymin, xmax, ymax},
            s.getTechLayer(),
            s.getTechVia(),
            s.getVia(),
            s.getRule()},
           rcV_[k].is_vertical,
           width,
           width / 2);
  rcV_.push_back(x);
}

tmg_rc* tmg_conn::addRcPatch(const int from_idx, const int to_idx)
{
  dbTechLayer* layer = ptV_[from_idx].layer;
  if (!layer || layer != ptV_[to_idx].layer
      || (ptV_[from_idx].x != ptV_[to_idx].x
          && ptV_[from_idx].y != ptV_[to_idx].y)) {
    return nullptr;
  }
  const bool is_vertical = (ptV_[from_idx].y != ptV_[to_idx].y);
  int xlo, ylo, xhi, yhi;
  if (is_vertical) {
    xlo = ptV_[from_idx].x;
    xhi = xlo;
    std::tie(ylo, yhi) = std::minmax(ptV_[from_idx].y, ptV_[to_idx].y);
  } else {
    ylo = ptV_[from_idx].y;
    yhi = ylo;
    std::tie(xlo, xhi) = std::minmax(ptV_[from_idx].x, ptV_[to_idx].x);
  }
  const int width = layer->getWidth();  // trouble for nondefault
  const int hw = width / 2;
  tmg_rc x(from_idx,
           to_idx,
           {{xlo - hw, ylo - hw, xhi + hw, yhi + hw}, layer, nullptr, nullptr},
           is_vertical,
           width,
           hw);
  rcV_.push_back(x);
  return &rcV_.back();
}

void tmg_conn::addITerm(dbITerm* iterm)
{
  csVV_.emplace_back();
  csNV_.emplace_back();

  tmg_rcterm& x = termV_.emplace_back(iterm);
  x.pt = nullptr;
  x.first_pt = nullptr;
}

void tmg_conn::addBTerm(dbBTerm* bterm)
{
  csVV_.emplace_back();
  csNV_.emplace_back();

  tmg_rcterm& x = termV_.emplace_back(bterm);
  x.pt = nullptr;
  x.first_pt = nullptr;
}

void tmg_conn::addShort(const int i0, const int i1)
{
  shortV_.emplace_back(i0, i1);
  if (ptV_[i0].fre) {
    ptV_[i0].fre = false;
  } else {
    ptV_[i0].jct = true;
  }
  if (ptV_[i1].fre) {
    ptV_[i1].fre = false;
  } else {
    ptV_[i1].jct = true;
  }
}

void tmg_conn::loadNet(dbNet* net)
{
  net_ = net;
  rcV_.clear();
  ptV_.clear();
  termV_.clear();
  csVV_.clear();
  csNV_.clear();
  shortV_.clear();
  first_for_clear_ = nullptr;

  for (dbITerm* iterm : net->getITerms()) {
    addITerm(iterm);
  }

  for (dbBTerm* bterm : net->getBTerms()) {
    addBTerm(bterm);
  }
}

void tmg_conn::loadSWire(dbNet* net)
{
  hasSWire_ = false;
  dbSet<dbSWire> swires = net->getSWires();
  if (swires.empty()) {
    return;
  }

  hasSWire_ = true;
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
        if (rect.getDir() == horizontal) {
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

      if (ptV_.empty() || layer1 != ptV_.back().layer || x1 != ptV_.back().x
          || y1 != ptV_.back().y) {
        allocPt(x1, y1, layer1);
      }

      allocPt(x2, y2, layer2);
      addRc(shape, ptV_.size() - 2, ptV_.size() - 1);
    }
  }
}

void tmg_conn::loadWire(dbWire* wire)
{
  ptV_.clear();
  dbWirePathItr pitr;
  dbWirePath path;
  pitr.begin(wire);
  while (pitr.getNextPath(path)) {
    if (ptV_.empty() || path.layer != ptV_.back().layer
        || path.point.getX() != ptV_.back().x
        || path.point.getY() != ptV_.back().y) {
      allocPt(path.point.getX(), path.point.getY(), path.layer);
    }
    dbWirePathShape pathShape;
    while (pitr.getNextShape(pathShape)) {
      allocPt(pathShape.point.getX(), pathShape.point.getY(), pathShape.layer);
      addRc(pathShape.shape, ptV_.size() - 2, ptV_.size() - 1, path.rule);
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
  tmg_rc_sh* sj = &(rcV_[j].shape);
  const int isVia = sj->isVia() ? 1 : 0;
  search_->searchStart(rt, {sjxMin, sjyMin, sjxMax, sjyMax}, isVia);
  int klast = -1;
  int k;
  while (search_->searchNext(&k)) {
    if (k == klast || k == j) {
      continue;
    }
    if (rcV_[j].to_idx == rcV_[k].from_idx
        || rcV_[j].from_idx == rcV_[k].to_idx) {
      continue;
    }
    sj = &(rcV_[j].shape);
    if (!sj->isVia() && rcV_[j].is_vertical == rcV_[k].is_vertical) {
      continue;
    }
    const tmg_rc_sh* sk = &(rcV_[k].shape);
    if (sk->isVia()) {
      continue;
    }
    dbTechLayer* tlayer = ptV_[rcV_[k].from_idx].layer;
    int nxmin = sk->xMin();
    int nxmax = sk->xMax();
    int nymin = sk->yMin();
    int nymax = sk->yMax();
    int x;
    int y;
    if (rcV_[k].is_vertical) {
      if (sjyMin - sk->yMin() < rcV_[k].width) {
        continue;
      }
      if (sk->yMax() - sjyMax < rcV_[k].width) {
        continue;
      }
      if (ptV_[rcV_[k].from_idx].y > ptV_[rcV_[k].to_idx].y) {
        rcV_[k].shape.setYmin(ptV_[rcV_[j].from_idx].y - (rcV_[k].width / 2));
        nymax = ptV_[rcV_[j].from_idx].y + rcV_[k].width / 2;
      } else {
        rcV_[k].shape.setYmax(ptV_[rcV_[j].from_idx].y + (rcV_[k].width / 2));
        nymin = ptV_[rcV_[j].from_idx].y - rcV_[k].width / 2;
      }
      x = ptV_[rcV_[k].from_idx].x;
      y = ptV_[rcV_[j].from_idx].y;
    } else {
      if (sjxMin - sk->xMin() < rcV_[k].width) {
        continue;
      }
      if (sk->xMax() - sjxMax < rcV_[k].width) {
        continue;
      }
      if (ptV_[rcV_[k].from_idx].x > ptV_[rcV_[k].to_idx].x) {
        rcV_[k].shape.setXmin(ptV_[rcV_[j].from_idx].x - (rcV_[k].width / 2));
        nxmax = ptV_[rcV_[j].from_idx].x + rcV_[k].width / 2;
      } else {
        rcV_[k].shape.setXmax(ptV_[rcV_[j].from_idx].x + (rcV_[k].width / 2));
        nxmin = ptV_[rcV_[j].from_idx].x - rcV_[k].width / 2;
      }
      x = ptV_[rcV_[j].from_idx].x;
      y = ptV_[rcV_[k].from_idx].y;
    }
    klast = k;
    tmg_rcpt* pt = allocPt(x, y, tlayer);
    pt->tindex = -1;
    pt->t_alt = nullptr;
    pt->next_for_term = nullptr;
    pt->pinpt = false;
    pt->c2pinpt = false;
    pt->next_for_clear = nullptr;
    pt->sring = nullptr;
    const int endTo = rcV_[k].to_idx;
    rcV_[k].to_idx = ptV_.size() - 1;
    // create new tmg_rc
    addRc(k, rcV_[k].shape, ptV_.size() - 1, endTo, nxmin, nymin, nxmax, nymax);
    search_->addShape(rt, {nxmin, nymin, nxmax, nymax}, 0, rcV_.size() - 1);
  }
}

// split top of T shapes
void tmg_conn::splitTtop()
{
  for (size_t j = 0; j < rcV_.size(); j++) {
    tmg_rc_sh* sj = &(rcV_[j].shape);
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
      const int via_x = ptV_[rcV_[j].from_idx].x;
      const int via_y = ptV_[rcV_[j].from_idx].y;
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
  for (const tmg_rcshort& rcshort : shortV_) {
    if (rcshort.skip) {
      continue;
    }
    tmg_rcpt* pfr = &ptV_[rcshort.i0];
    tmg_rcpt* pto = &ptV_[rcshort.i1];
    if (pfr == pto) {
      continue;
    }
    if (pfr->sring && !pto->sring) {
      pto->sring = pfr->sring;
      pfr->sring = pto;
    } else if (pto->sring && !pfr->sring) {
      pfr->sring = pto->sring;
      pto->sring = pfr;
    } else if (!pfr->sring && !pto->sring) {
      pfr->sring = pto;
      pto->sring = pfr;
    } else {
      tmg_rcpt* x = pfr->sring;
      while (x->sring != pfr && x != pto) {
        x = x->sring;
      }
      if (x == pto) {
        continue;
      }
      x->sring = pto;
      x = pto;
      while (x->sring != pto) {
        x = x->sring;
      }
      x->sring = pfr;
    }
  }
}

void tmg_conn::detachTilePins()
{
  slicedTilePinCnt_ = 0;
  for (const tmg_rcterm& term : termV_) {
    if (term.iterm) {
      continue;
    }
    dbBTerm* bterm = term.bterm;
    dbShape pin;
    if (!bterm->getFirstPin(pin) || pin.isVia()) {
      continue;
    }
    const Rect rectb = pin.getBox();
    const int rtlb = pin.getTechLayer()->getRoutingLevel();
    bool sliceDone = false;
    for (int k = 0; !sliceDone && k < termV_.size(); k++) {
      tmg_rcterm* tx = &termV_[k];
      if (tx->bterm) {
        continue;
      }
      dbMTerm* mterm = tx->iterm->getMTerm();
      const dbTransform transform = tx->iterm->getInst()->getTransform();
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
          stbtx1_[slicedTilePinCnt_] = x1 + 1;
          stbty1_[slicedTilePinCnt_] = y1 + 1;
          stbtx2_[slicedTilePinCnt_] = x2 - 1;
          stbty2_[slicedTilePinCnt_] = y2 - 1;
          slicedTileBTerm_[slicedTilePinCnt_++] = bterm;
          sliceDone = true;
        }
      }
    }
  }
}

void tmg_conn::getBTermSearchBox(dbBTerm* bterm, dbShape& pin, Rect& rect)
{
  for (int ii = 0; ii < slicedTilePinCnt_; ii++) {
    if (slicedTileBTerm_[ii] == bterm) {
      rect.reset(stbtx1_[ii], stbty1_[ii], stbtx2_[ii], stbty2_[ii]);
      return;
    }
  }
  rect = pin.getBox();
}

void tmg_conn::findConnections()
{
  if (ptV_.empty()) {
    return;
  }
  if (!search_) {
    search_ = std::make_unique<tmg_conn_search>();
  }
  search_->clear();

  for (auto& pt : ptV_) {
    pt.fre = true;
    pt.jct = false;
    pt.pinpt = false;
    pt.c2pinpt = false;
    pt.next_for_clear = nullptr;
    pt.sring = nullptr;
  }
  first_for_clear_ = nullptr;
  for (size_t j = 0; j < rcV_.size() - 1; j++) {
    if (rcV_[j].to_idx == rcV_[j + 1].from_idx) {
      ptV_[rcV_[j].to_idx].fre = false;
    }
  }

  // put wires in search
  for (size_t j = 0; j < rcV_.size(); j++) {
    tmg_rc_sh* s = &(rcV_[j].shape);
    if (s->isVia()) {
      const int via_x = ptV_[rcV_[j].from_idx].x;
      const int via_y = ptV_[rcV_[j].from_idx].y;

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
          search_->addShape(rt_b,
                            {via_x + b->xMin(),
                             via_y + b->yMin(),
                             via_x + b->xMax(),
                             via_y + b->yMax()},
                            1,
                            j);
        } else if (b->getTechLayer() == layt) {
          search_->addShape(rt_t,
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
      search_->addShape(rt, s->rect(), 0, j);
    }
  }

  if (rcV_.size() < 10000) {
    splitTtop();
  }

  // find self-intersections of wires
  for (int j = 0; j < (int) rcV_.size() - 1; j++) {
    const tmg_rc_sh* s = &(rcV_[j].shape);
    const int conn_next = (rcV_[j].to_idx == rcV_[j + 1].from_idx);
    if (s->isVia()) {
      const int via_x = ptV_[rcV_[j].from_idx].x;
      const int via_y = ptV_[rcV_[j].from_idx].y;

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
          search_->searchStart(rt_b,
                               {via_x + b->xMin(),
                                via_y + b->yMin(),
                                via_x + b->xMax(),
                                via_y + b->yMax()},
                               1);
        } else if (b->getTechLayer() == layt) {
          search_->searchStart(rt_t,
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
        while (search_->searchNext(&k)) {
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
      search_->searchStart(rt, s->rect(), 0);
      int klast = -1;
      int k;
      while (search_->searchNext(&k)) {
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
  for (int j = 0; j < termV_.size(); j++) {
    csV_ = &csVV_[j];
    csN_ = 0;
    tmg_rcterm* x = &termV_[j];
    if (x->iterm) {
      dbMTerm* mterm = x->iterm->getMTerm();
      const dbTransform transform = x->iterm->getInst()->getTransform();
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
              search_->searchStart(rt_t, rect, 2);
              int klast = -1;
              int k;
              while (search_->searchNext(&k)) {
                if (k != klast) {
                  klast = k;
                  int ii;
                  for (ii = 0; ii < csN_; ii++) {
                    if (k == (*csV_)[ii].k) {
                      break;
                    }
                  }
                  if (ii < csN_) {
                    continue;
                  }
                  if (csN_ == 32) {
                    break;
                  }
                  (*csV_)[csN_].k = k;
                  (*csV_)[csN_].rect = rect;
                  (*csV_)[csN_].rtlev = rt_t;
                  csN_++;
                }
              }
              const int rt_b = tv->getBottomLayer()->getRoutingLevel();
              if (rt_b == 0) {
                continue;
              }
              search_->searchStart(rt_b, rect, 2);
              klast = -1;
              while (search_->searchNext(&k)) {
                if (k != klast) {
                  klast = k;
                  int ii;
                  for (ii = 0; ii < csN_; ii++) {
                    if (k == (*csV_)[ii].k) {
                      break;
                    }
                  }
                  if (ii < csN_) {
                    continue;
                  }
                  if (csN_ == 32) {
                    break;
                  }
                  (*csV_)[csN_].k = k;
                  (*csV_)[csN_].rect = rect;
                  (*csV_)[csN_].rtlev = rt_b;
                  csN_++;
                }
              }
            } else if (ipass == 0 && !box->isVia()) {
              const int rt = box->getTechLayer()->getRoutingLevel();
              Rect rect = box->getBox();
              transform.apply(rect);
              search_->searchStart(rt, rect, 2);
              int klast = -1;
              int k;
              while (search_->searchNext(&k)) {
                if (k != klast) {
                  klast = k;
                  int ii;
                  for (ii = 0; ii < csN_; ii++) {
                    if (k == (*csV_)[ii].k) {
                      break;
                    }
                  }
                  if (ii < csN_ && csN_ >= 8) {
                    continue;
                  }
                  if (csN_ == 32) {
                    break;
                  }
                  (*csV_)[csN_].k = k;
                  (*csV_)[csN_].rect = rect;
                  (*csV_)[csN_].rtlev = rt;
                  csN_++;
                }
              }
            }
          }
        }
      }  // mpins
    } else {
      // bterm
      dbShape pin;
      if (x->bterm->getFirstPin(pin)) {
        if (pin.isVia()) {
          // TODO
        } else {
          const int rt = pin.getTechLayer()->getRoutingLevel();
          Rect rect;
          getBTermSearchBox(x->bterm, pin, rect);
          search_->searchStart(rt, rect, 2);
          int klast = -1;
          int k;
          while (search_->searchNext(&k)) {
            if (k != klast) {
              klast = k;
              int ii;
              for (ii = 0; ii < csN_; ii++) {
                if (k == (*csV_)[ii].k) {
                  break;
                }
              }
              if (ii < csN_) {
                continue;
              }
              if (csN_ == 32) {
                break;
              }
              (*csV_)[csN_].k = k;
              (*csV_)[csN_].rect = rect;
              (*csV_)[csN_].rtlev = rt;
              csN_++;
            }
          }
        }
      }
    }
    csNV_[j] = csN_;
  }

  for (auto& pc : ptV_) {
    pc.pinpt = false;
    pc.c2pinpt = false;
    pc.next_for_clear = nullptr;
    pc.sring = nullptr;
  }
  setSring();

  for (int j = 0; j < termV_.size(); j++) {
    connectTerm(j, false);
  }
  const bool ok = checkConnected();
  if (!ok) {
    for (int j = 0; j < termV_.size(); j++) {
      connectTerm(j, true);
    }
  }

  // make terms of shorted points consistent
  for (int it = 0; it < 5; it++) {
    int cnt = 0;
    for (const tmg_rcshort& rcshort : shortV_) {
      if (rcshort.skip) {
        continue;
      }
      const int i0 = rcshort.i0;
      const int i1 = rcshort.i1;
      if (ptV_[i0].tindex < 0 && ptV_[i1].tindex >= 0) {
        ptV_[i0].tindex = ptV_[i1].tindex;
        cnt++;
      }
      if (ptV_[i1].tindex < 0 && ptV_[i0].tindex >= 0) {
        ptV_[i1].tindex = ptV_[i0].tindex;
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
  const tmg_rc_sh* sa = &(rcV_[j].shape);
  const tmg_rc_sh* sb = &(rcV_[k].shape);
  const int afr = rcV_[j].from_idx;
  const int ato = rcV_[j].to_idx;
  const int bfr = rcV_[k].from_idx;
  const int bto = rcV_[k].to_idx;
  const int xlo = std::max(sa->xMin(), sb->xMin());
  const int ylo = std::max(sa->yMin(), sb->yMin());
  const int xhi = std::min(sa->xMax(), sb->xMax());
  const int yhi = std::min(sa->yMax(), sb->yMax());
  int xc = (xlo + xhi) / 2;
  int yc = (ylo + yhi) / 2;
  bool choose_afr = false;
  bool choose_bfr = false;
  if (sa->isVia() && sb->isVia()) {
    if (ptV_[afr].layer == ptV_[bfr].layer) {
      choose_afr = true;
      choose_bfr = true;
    } else if (ptV_[afr].layer == ptV_[bto].layer) {
      choose_afr = true;
      choose_bfr = false;
    } else if (ptV_[ato].layer == ptV_[bfr].layer) {
      choose_afr = false;
      choose_bfr = true;
    } else if (ptV_[ato].layer == ptV_[bto].layer) {
      choose_afr = false;
      choose_bfr = false;
    }
  } else if (sa->isVia()) {
    choose_afr = (ptV_[afr].layer == ptV_[bfr].layer);
    xc = ptV_[afr].x;
    yc = ptV_[afr].y;  // same for afr and ato
    const int dbfr = abs(ptV_[bfr].x - xc) + abs(ptV_[bfr].y - yc);
    const int dbto = abs(ptV_[bto].x - xc) + abs(ptV_[bto].y - yc);
    choose_bfr = (dbfr < dbto);
  } else if (sb->isVia()) {
    choose_bfr = (ptV_[afr].layer == ptV_[bfr].layer);
    xc = ptV_[bfr].x;
    yc = ptV_[bfr].y;
    const int dafr = abs(ptV_[afr].x - xc) + abs(ptV_[afr].y - yc);
    const int dato = abs(ptV_[ato].x - xc) + abs(ptV_[ato].y - yc);
    choose_afr = (dafr < dato);
  } else {
    // get distances to the center of intersection region, (xc,yc)
    const int dafr = abs(ptV_[afr].x - xc) + abs(ptV_[afr].y - yc);
    const int dato = abs(ptV_[ato].x - xc) + abs(ptV_[ato].y - yc);
    choose_afr = (dafr < dato);
    const int dbfr = abs(ptV_[bfr].x - xc) + abs(ptV_[bfr].y - yc);
    const int dbto = abs(ptV_[bto].x - xc) + abs(ptV_[bto].y - yc);
    choose_bfr = (dbfr < dbto);
  }
  int i0 = (choose_afr ? afr : ato);
  int i1 = (choose_bfr ? bfr : bto);
  if (i1 < i0) {
    std::swap(i0, i1);
  }
  addShort(i0, i1);
  ptV_[i0].fre = false;
  ptV_[i1].fre = false;
}

static void addPointToTerm(tmg_rcpt* pt, tmg_rcterm* x)
{
  tmg_rcpt* tpt = x->pt;
  tmg_rcpt* ptpt = nullptr;
  while (tpt && (pt->x > tpt->x || (pt->x == tpt->x && pt->y > tpt->y))) {
    ptpt = tpt;
    tpt = tpt->next_for_term;
  }
  if (ptpt) {
    ptpt->next_for_term = pt;
  } else {
    x->pt = pt;
  }
  pt->next_for_term = tpt;
}

static void removePointFromTerm(tmg_rcpt* pt, tmg_rcterm* x)
{
  if (x->pt == pt) {
    x->pt = pt->next_for_term;
    pt->next_for_term = nullptr;
    return;
  }
  tmg_rcpt* ptpt = nullptr;
  tmg_rcpt* tpt;
  for (tpt = x->pt; tpt; tpt = tpt->next_for_term) {
    if (tpt == pt) {
      break;
    }
    ptpt = tpt;
  }
  if (!tpt) {
    return;  // error, not found
  }
  ptpt->next_for_term = tpt->next_for_term;
  pt->next_for_term = nullptr;
}

void tmg_conn::connectTerm(const int j, const bool soft)
{
  csV_ = &csVV_[j];
  csN_ = csNV_[j];
  if (!csN_) {
    return;
  }
  for (tmg_rcpt* pc = first_for_clear_; pc; pc = pc->next_for_clear) {
    pc->pinpt = false;
    pc->c2pinpt = false;
  }
  first_for_clear_ = nullptr;

  for (int ii = 0; ii < csN_; ii++) {
    const int k = (*csV_)[ii].k;
    tmg_rcpt* pfr = &ptV_[rcV_[k].from_idx];
    tmg_rcpt* pto = &ptV_[rcV_[k].to_idx];
    const Point afr(pfr->x, pfr->y);
    if ((*csV_)[ii].rtlev == pfr->layer->getRoutingLevel()
        && (*csV_)[ii].rect.intersects(afr)) {
      if (!(pfr->pinpt || pfr->c2pinpt)) {
        pfr->next_for_clear = first_for_clear_;
        first_for_clear_ = pfr;
      }
      if (!(pto->pinpt || pto->c2pinpt)) {
        pto->next_for_clear = first_for_clear_;
        first_for_clear_ = pto;
      }
      pfr->pinpt = true;
      pfr->c2pinpt = true;
      pto->c2pinpt = true;
    }
    const Point ato(pto->x, pto->y);
    if ((*csV_)[ii].rtlev == pto->layer->getRoutingLevel()
        && (*csV_)[ii].rect.intersects(ato)) {
      if (!(pfr->pinpt || pfr->c2pinpt)) {
        pfr->next_for_clear = first_for_clear_;
        first_for_clear_ = pfr;
      }
      if (!(pto->pinpt || pto->c2pinpt)) {
        pto->next_for_clear = first_for_clear_;
        first_for_clear_ = pto;
      }
      pto->pinpt = true;
      pto->c2pinpt = true;
      pfr->c2pinpt = true;
    }
  }

  for (tmg_rcpt* pc = first_for_clear_; pc; pc = pc->next_for_clear) {
    if (pc->sring) {
      int c2pinpt = pc->c2pinpt;
      for (tmg_rcpt* x = pc->sring; x != pc; x = x->sring) {
        if (x->c2pinpt) {
          c2pinpt = 1;
        }
      }
      if (c2pinpt) {
        for (tmg_rcpt* x = pc->sring; x != pc; x = x->sring) {
          if (!(x->pinpt || x->c2pinpt)) {
            x->next_for_clear = first_for_clear_;
            first_for_clear_ = x;
          }
          x->c2pinpt = true;
        }
      }
    }
  }

  for (int ii = 0; ii < csN_; ii++) {
    const int k = (*csV_)[ii].k;
    tmg_rcpt* pfr = &ptV_[rcV_[k].from_idx];
    tmg_rcpt* pto = &ptV_[rcV_[k].to_idx];
    if (pfr->c2pinpt) {
      if (!(pto->pinpt || pto->c2pinpt)) {
        pto->next_for_clear = first_for_clear_;
        first_for_clear_ = pto;
      }
      pto->c2pinpt = true;
    }
    if (pto->c2pinpt) {
      if (!(pfr->pinpt || pfr->c2pinpt)) {
        pfr->next_for_clear = first_for_clear_;
        first_for_clear_ = pfr;
      }
      pfr->c2pinpt = true;
    }
  }

  tmg_rcterm* x = &termV_[j];
  for (int ii = 0; ii < csN_; ii++) {
    const int k = (*csV_)[ii].k;
    const int bfr = rcV_[k].from_idx;
    const int bto = rcV_[k].to_idx;
    const bool cfr = ptV_[bfr].pinpt;
    const bool cto = ptV_[bto].pinpt;
    if (soft && !cfr && !cto) {
      if (!(ptV_[bfr].c2pinpt || ptV_[bto].c2pinpt)) {
        connectTermSoft(j, (*csV_)[ii].rtlev, (*csV_)[ii].rect, (*csV_)[ii].k);
      }
      continue;
    }
    if (cfr && !cto) {
      tmg_rcpt* pt = &ptV_[bfr];
      const tmg_rcpt* pother = &ptV_[bto];
      if (pt->tindex == j) {
        continue;
      }
      if (pt->tindex >= 0 && pt->t_alt && pt->t_alt->tindex < 0) {
        const int oldt = pt->tindex;
        removePointFromTerm(pt, &termV_[oldt]);
        pt->t_alt->tindex = oldt;
        addPointToTerm(pt->t_alt, &termV_[oldt]);
        pt->tindex = -1;
      }
      if (pt->tindex >= 0 && pt->tindex == pother->tindex) {
        // override old connection if it is on the other
        removePointFromTerm(pt, &termV_[pt->tindex]);
        pt->tindex = -1;
      }
      if (pt->tindex >= 0) {
        logger_->error(ODB,
                       390,
                       "order_wires failed: net {}, shorts to another term at "
                       "wire point ({} {})",
                       net_->getName(),
                       pt->x,
                       pt->y);
      }
      pt->tindex = j;
      addPointToTerm(pt, x);

    } else if (cto && !cfr) {
      tmg_rcpt* pt = &ptV_[bto];
      const tmg_rcpt* pother = &ptV_[bfr];
      if (pt->tindex == j) {
        continue;
      }
      if (pt->tindex >= 0 && pt->t_alt && pt->t_alt->tindex < 0) {
        const int oldt = pt->tindex;
        removePointFromTerm(pt, &termV_[oldt]);
        pt->t_alt->tindex = oldt;
        addPointToTerm(pt->t_alt, &termV_[oldt]);
        pt->tindex = -1;
      }
      if (pt->tindex >= 0 && pt->tindex == pother->tindex) {
        // override old connection if it is on the other
        removePointFromTerm(pt, &termV_[pt->tindex]);
        pt->tindex = -1;
      }
      if (pt->tindex >= 0) {
        logger_->error(ODB,
                       391,
                       "order_wires failed: net {}, shorts to another term at "
                       "wire point ({} {})",
                       net_->getName(),
                       pt->x,
                       pt->y);
      }
      pt->tindex = j;
      addPointToTerm(pt, x);

    } else if (cfr && cto) {
      if (ptV_[bfr].tindex == j || ptV_[bto].tindex == j) {
        continue;
      }
      if (ptV_[bfr].tindex >= 0 && ptV_[bto].tindex < 0) {
        tmg_rcpt* pt = &ptV_[bto];
        pt->tindex = j;
        addPointToTerm(pt, x);
        continue;
      }
      tmg_rcpt* pt = &ptV_[bfr];
      tmg_rcpt* pother = &ptV_[bto];
      if (pt->tindex >= 0 && pt->t_alt && pt->t_alt->tindex < 0) {
        const int oldt = pt->tindex;
        removePointFromTerm(pt, &termV_[oldt]);
        pt->t_alt->tindex = oldt;
        addPointToTerm(pt->t_alt, &termV_[oldt]);
        pt->tindex = -1;
      }
      if (pt->tindex >= 0 && pt->tindex == pother->tindex) {
        // override old connection if it is on the other
        removePointFromTerm(pt, &termV_[pt->tindex]);
        pt->tindex = -1;
      }
      if (pt->tindex >= 0) {
        logger_->error(ODB,
                       392,
                       "order_wires failed: net {}, shorts to another term at "
                       "wire point ({} {})",
                       net_->getName(),
                       pt->x,
                       pt->y);
      }
      pt->tindex = j;
      addPointToTerm(pt, x);
      pt->t_alt = pother;
    }
  }
  for (tmg_rcpt* pc = first_for_clear_; pc; pc = pc->next_for_clear) {
    pc->pinpt = false;
    pc->c2pinpt = false;
  }
  first_for_clear_ = nullptr;
}

void tmg_conn::connectTermSoft(const int j,
                               const int rt,
                               Rect& rect,
                               const int k)
{
  const tmg_rc_sh* sb = &(rcV_[k].shape);
  const int bfr = rcV_[k].from_idx;
  const int bto = rcV_[k].to_idx;
  const int xlo = std::max(rect.xMin(), sb->xMin());
  const int ylo = std::max(rect.yMin(), sb->yMin());
  const int xhi = std::min(rect.xMax(), sb->xMax());
  const int yhi = std::min(rect.yMax(), sb->yMax());
  const int xc = (xlo + xhi) / 2;
  const int yc = (ylo + yhi) / 2;
  bool choose_bfr = false;
  bool has_alt = true;
  if (sb->isVia()) {
    choose_bfr = (rt == ptV_[bfr].layer->getRoutingLevel());
    has_alt = false;
  } else {
    const int dbfr = abs(ptV_[bfr].x - xc) + abs(ptV_[bfr].y - yc);
    const int dbto = abs(ptV_[bto].x - xc) + abs(ptV_[bto].y - yc);
    choose_bfr = (dbfr < dbto);
    if (abs(dbfr - dbto) > 5000) {
      has_alt = false;
    }
  }
  tmg_rcpt* pt = &ptV_[choose_bfr ? bfr : bto];
  tmg_rcpt* pother = &ptV_[choose_bfr ? bto : bfr];
  if (pt->tindex == j) {
    return;
  }

  // This was needed in a case where a square patch
  // of M1 was used to connect pins A and B of an instance.
  // The original input def looked like:
  // NEW M1 ( 2090900 1406000 ) ( * 1406000 ) NEW M1 ...
  // In this case we get two _ptV[] points, that have identical
  // x,y,layer, and we connect one iterm to each.
  if (pt->tindex >= 0 && ptV_[bfr].x == ptV_[bto].x
      && ptV_[bfr].y == ptV_[bto].y) {
    // if wire shape k is an isolated square,
    // then connect to other point if available
    if (pother->tindex == j) {
      return;  // already connected
    }
    if (pother->tindex < 0) {
      pt = pother;
      has_alt = false;
    }
  }

  // override old connection if it is on the other
  if (pt->tindex >= 0 && pother->tindex == pt->tindex) {
    removePointFromTerm(pt, &termV_[pt->tindex]);
    pt->tindex = -1;
  }

  if (pt->tindex >= 0 && pother->tindex < 0 && pt->layer == pother->layer) {
    const int dist = abs(pt->x - pother->x) + abs(pt->y - pother->y);
    if (dist < 5000) {
      pt = pother;
      has_alt = false;
    }
  }

  if (pt->tindex >= 0) {
    return;  // skip soft if conflicts with hard
  }
  pt->tindex = j;
  tmg_rcterm* x = &termV_[j];
  addPointToTerm(pt, x);
  pt->fre = false;
  if (has_alt) {
    pt->t_alt = pother;
  }
}

// find a driver iterm, or any bterm, or any iterm, or default to the first
// point
int tmg_conn::getStartNode()
{
  dbITerm* it_drv;
  dbBTerm* bt_drv;
  tmg_getDriveTerm(net_, &it_drv, &bt_drv);
  for (const tmg_rcterm& x : termV_) {
    if (x.iterm == it_drv && x.bterm == bt_drv) {
      if (!x.pt) {
        return 0;
      }
      return (x.pt - ptV_.data());
    }
  }
  return 0;
}

void tmg_conn::analyzeNet(dbNet* net)
{
  if (net->isWireOrdered()) {
    net_ = net;
    checkConnOrdered();
  } else {
    loadNet(net);
    if (net->getWire()) {
      loadWire(net->getWire());
    }
    if (ptV_.empty()) {
      // ignoring this net
      net->setDisconnected(false);
      net->setWireOrdered(false);
      return;
    }
    findConnections();
    bool noConvert = false;
    if (hasSWire_) {
      net->destroySWires();
    }
    relocateShorts();
    treeReorder(noConvert);
  }
  net->setDisconnected(!connected_);
  net->setWireOrdered(true);
}

bool tmg_conn::checkConnected()
{
  for (const tmg_rcterm& x : termV_) {
    if (x.pt == nullptr) {
      return false;
    }
  }
  if (termV_.empty()) {
    return true;
  }
  tstackV_.clear();
  int jstart = getStartNode();
  tmg_rcterm* xstart = nullptr;
  if (ptV_[jstart].tindex >= 0) {
    tmg_rcterm* x = &termV_[ptV_[jstart].tindex];
    xstart = x;
    tstackV_.push_back(x);
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
      if (ptV_[jto].tindex >= 0) {
        tmg_rcterm* x = &termV_[ptV_[jto].tindex];
        if (x == xstart && !is_short) {
          // removing multi-connection at driver
          removePointFromTerm(&ptV_[jto], &termV_[ptV_[jto].tindex]);
          ptV_[jto].tindex = -1;
          ptV_[jto].t_alt = nullptr;
        } else if (x->pt && x->pt->next_for_term) {
          // add potential short-from points to stack
          tstackV_.push_back(x);
        }
      }
      // the part of addToWire needed in no_convert case
      if (ptV_[jfr].tindex >= 0) {
        tmg_rcterm* x = &termV_[ptV_[jfr].tindex];
        if (x->first_pt == nullptr) {
          x->first_pt = &ptV_[jfr];
        }
      }
      if (ptV_[jto].tindex >= 0) {
        tmg_rcterm* x = &termV_[ptV_[jto].tindex];
        if (x->first_pt == nullptr) {
          x->first_pt = &ptV_[jto];
        }
      }
    }
    // finished physically-connected subtree,
    // find an unvisited short-from point
    tmg_rcpt* pt = nullptr;
    while (tstack0 < tstackV_.size() && !pt) {
      tmg_rcterm* x = tstackV_[tstack0++];
      for (pt = x->pt; pt; pt = pt->next_for_term) {
        if (!isVisited(pt - ptV_.data())) {
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
    jstart = pt - ptV_.data();
    if (!dfsStart(jstart)) {
      return false;
    }
  }
  bool con = true;
  for (tmg_rcterm& x : termV_) {
    if (!x.first_pt) {
      con = false;
    }
    x.first_pt = nullptr;  // cleanup
  }
  return con;  // all terms connected, may be floating pieces of wire
}

void tmg_conn::treeReorder(const bool no_convert)
{
  connected_ = true;
  need_short_wire_id_ = false;
  if (ptV_.empty()) {
    return;
  }
  newWire_ = nullptr;
  last_id_ = -1;
  if (!no_convert) {
    newWire_ = net_->getWire();
    if (!newWire_) {
      newWire_ = dbWire::create(net_);
    }
    encoder_.begin(newWire_);
    for (tmg_rcpt& pt : ptV_) {
      pt.dbwire_id = -1;
    }
  }
  for (tmg_rcterm& x : termV_) {
    x.first_pt = nullptr;
    if (x.pt == nullptr) {
      connected_ = false;
    }
  }

  if (termV_.empty()) {
    return;
  }

  net_rule_ = net_->getNonDefaultRule();
  path_rule_ = net_rule_;

  int tstack0 = 0;
  tstackV_.clear();
  int jstart = getStartNode();
  tmg_rcterm* xstart = nullptr;
  if (ptV_[jstart].tindex >= 0) {
    tmg_rcterm* x = &termV_[ptV_[jstart].tindex];
    xstart = x;
    tstackV_.push_back(x);
  }
  dfsClear();
  if (!dfsStart(jstart)) {
    logger_->warn(ODB, 395, "cannot order {}", net_->getConstName());
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
      if (ptV_[jto].tindex >= 0) {
        x = &termV_[ptV_[jto].tindex];
        if (x == xstart && !is_short) {
          // removing multi-connection at driver
          removePointFromTerm(&ptV_[jto], &termV_[ptV_[jto].tindex]);
          ptV_[jto].tindex = -1;
          ptV_[jto].t_alt = nullptr;
        } else if (x->pt && x->pt->next_for_term) {
          // add potential short-from points to stack
          tstackV_.push_back(x);
        }
      }
      if (!no_convert) {
        addToWire(jfr, jto, k, is_short, is_loop);
      } else {
        // the part of addToWire needed in no_convert case
        if (ptV_[jfr].tindex >= 0) {
          x = &termV_[ptV_[jfr].tindex];
          if (x->first_pt == nullptr) {
            x->first_pt = &ptV_[jfr];
          }
        }
        if (ptV_[jto].tindex >= 0) {
          x = &termV_[ptV_[jto].tindex];
          if (x->first_pt == nullptr) {
            x->first_pt = &ptV_[jto];
          }
        }
      }
    }
    // finished physically-connected subtree,
    // find an unvisited short-from point
    tmg_rcpt* pt = nullptr;
    while (tstack0 < tstackV_.size() && !pt) {
      x = tstackV_[tstack0++];
      for (pt = x->pt; pt; pt = pt->next_for_term) {
        if (!isVisited(pt - ptV_.data())) {
          break;
        }
      }
    }
    if (pt) {
      tstack0--;
      last_id_ = x->first_pt ? x->first_pt->dbwire_id : -1;
    }
    if (!pt) {
      int j;
      for (j = last_term_index; j < termV_.size(); j++) {
        x = &termV_[j];
        if (x->pt && !isVisited(x->pt - ptV_.data())) {
          break;
        }
      }
      last_term_index = j;
      if (j < termV_.size()) {
        // disconnected, start new path from another term
        connected_ = false;
        last_id_ = -1;
        tstackV_.push_back(x);
        pt = x->pt;
      } else {
        jstart = getDisconnectedStart();
        if (jstart < 0) {
          break;  // normal exit, no more subtrees
        }
        pt = &ptV_[jstart];
        last_id_ = -1;
      }
    }
    jstart = pt - ptV_.data();
    if (!dfsStart(jstart)) {
      logger_->warn(ODB, 396, "cannot order {}", net_->getConstName());
      return;
    }
  }

  checkVisited();
  if (!no_convert) {
    encoder_.end();
  }
}

int tmg_conn::getExtension(const int ipt, const tmg_rc* rc)
{
  const tmg_rcpt* p = &ptV_[ipt];
  tmg_rcpt* pto;
  if (ipt == rc->from_idx) {
    pto = &ptV_[rc->to_idx];
  } else if (ipt == rc->to_idx) {
    pto = &ptV_[rc->from_idx];
  } else {
    logger_->error(ODB, 16, "problem in getExtension()");
  }
  int ext = rc->default_ext;
  if (p->x < pto->x) {
    ext = p->x - rc->shape.xMin();
  } else if (p->x > pto->x) {
    ext = rc->shape.xMax() - p->x;
  } else if (p->y < pto->y) {
    ext = p->y - rc->shape.yMin();
  } else if (p->y > pto->y) {
    ext = rc->shape.yMax() - p->y;
  }
  return ext;
}

int tmg_conn::addPoint(const int ipt, const tmg_rc* rc)
{
  int wire_id;
  const tmg_rcpt* p = &ptV_[ipt];
  const int ext = getExtension(ipt, rc);
  if (ext == rc->default_ext) {
    wire_id = encoder_.addPoint(p->x, p->y);
  } else {
    wire_id = encoder_.addPoint(p->x, p->y, ext);
  }
  return wire_id;
}

int tmg_conn::addPoint(const int from_idx, const int ipt, const tmg_rc* rc)
{
  int wire_id;
  const tmg_rcpt* p = &ptV_[ipt];
  const int ext = getExtension(ipt, rc);
  if (ext == rc->default_ext) {
    wire_id = encoder_.addPoint(p->x, p->y);
  } else {
    wire_id = encoder_.addPoint(p->x, p->y, ext);
  }
  return wire_id;
}

int tmg_conn::addPointIfExt(const int ipt, const tmg_rc* rc)
{
  // for first wire after a via, need to add a point
  // only if the extension is not the default ext
  int wire_id = 0;
  const tmg_rcpt* p = &ptV_[ipt];
  const int ext = getExtension(ipt, rc);
  if (ext != rc->default_ext) {
    wire_id = encoder_.addPoint(p->x, p->y, ext);
  }
  return wire_id;
}

void tmg_conn::addToWire(const int fr,
                         const int to,
                         const int k,
                         const bool is_short,
                         const bool is_loop)
{
  if (!newWire_) {
    return;
  }

  const int xfr = ptV_[fr].x;
  const int yfr = ptV_[fr].y;
  const int xto = ptV_[to].x;
  const int yto = ptV_[to].y;

  if (is_short) {
    if (xfr != xto || yfr != yto) {
      ptV_[to].dbwire_id = -1;
      last_id_ = ptV_[fr].dbwire_id;
      return;
    }
    if (ptV_[fr].dbwire_id < 0) {
      need_short_wire_id_ = true;
      return;
    }
    ptV_[to].dbwire_id = ptV_[fr].dbwire_id;
    return;
  }
  if (k < 0) {
    logger_->error(
        ODB, 393, "tmg_conn::addToWire: value of k is negative: {}", k);
  }

  tmg_rc* rc = (k >= 0) ? &rcV_[k] : nullptr;
  int fr_id = ptV_[fr].dbwire_id;
  dbTechLayerRule* lyr_rule = nullptr;
  if (rc->shape.getRule()) {
    lyr_rule = rc->shape.getRule()->getLayerRule(ptV_[fr].layer);
  }
  if (fr_id < 0) {
    path_rule_ = rc->shape.getRule();
    firstSegmentAfterVia_ = 0;
    if (last_id_ >= 0) {
      // term feedthru
      if (path_rule_) {
        encoder_.newPathShort(
            last_id_, ptV_[fr].layer, dbWireType::ROUTED, lyr_rule);
      } else {
        encoder_.newPathShort(last_id_, ptV_[fr].layer, dbWireType::ROUTED);
      }
    } else {
      if (path_rule_) {
        encoder_.newPath(ptV_[fr].layer, dbWireType::ROUTED, lyr_rule);
      } else {
        encoder_.newPath(ptV_[fr].layer, dbWireType::ROUTED);
      }
    }
    if (!rc->shape.isVia()) {
      fr_id = addPoint(fr, rc);
    } else {
      fr_id = encoder_.addPoint(xfr, yfr);
    }
    ptV_[fr].dbwire_id = fr_id;
    if (ptV_[fr].tindex >= 0) {
      tmg_rcterm* x = &termV_[ptV_[fr].tindex];
      if (x->first_pt == nullptr) {
        x->first_pt = &ptV_[fr];
      }
      if (x->iterm) {
        encoder_.addITerm(x->iterm);
      } else {
        encoder_.addBTerm(x->bterm);
      }
    }
  } else if (fr_id != last_id_) {
    path_rule_ = rc->shape.getRule();
    if (rc->shape.isVia()) {
      if (path_rule_) {
        encoder_.newPath(fr_id, lyr_rule);
      } else {
        encoder_.newPath(fr_id);
      }
    } else {
      firstSegmentAfterVia_ = 0;
      const int ext = getExtension(fr, rc);
      if (ext != rc->default_ext) {
        if (path_rule_) {
          encoder_.newPathExt(fr_id, ext, lyr_rule);
        } else {
          encoder_.newPathExt(fr_id, ext);
        }
      } else {
        if (path_rule_) {
          encoder_.newPath(fr_id, lyr_rule);
        } else {
          encoder_.newPath(fr_id);
        }
      }
    }
    if (ptV_[fr].tindex >= 0) {
      tmg_rcterm* x = &termV_[ptV_[fr].tindex];
      if (x->first_pt == nullptr) {
        x->first_pt = &ptV_[fr];
      }
      if (x->iterm) {
        encoder_.addITerm(x->iterm);
      } else {
        encoder_.addBTerm(x->bterm);
      }
    }
  } else if (path_rule_ != rc->shape.getRule()) {
    // make a branch, for taper

    path_rule_ = rc->shape.getRule();
    if (rc->shape.isVia()) {
      if (path_rule_) {
        encoder_.newPath(fr_id, lyr_rule);
      } else {
        encoder_.newPath(fr_id);
      }
    } else {
      firstSegmentAfterVia_ = 0;
      const int ext = getExtension(fr, rc);
      if (ext != rc->default_ext) {
        if (path_rule_) {
          encoder_.newPathExt(fr_id, ext, lyr_rule);
        } else {
          encoder_.newPathExt(fr_id, ext);
        }
      } else {
        if (path_rule_) {
          encoder_.newPath(fr_id, lyr_rule);
        } else {
          encoder_.newPath(fr_id);
        }
      }
    }
    if (ptV_[fr].tindex >= 0) {
      tmg_rcterm* x = &termV_[ptV_[fr].tindex];
      if (x->first_pt == nullptr) {
        x->first_pt = &ptV_[fr];
      }
      if (x->iterm) {
        encoder_.addITerm(x->iterm);
      } else {
        encoder_.addBTerm(x->bterm);
      }
    }

    // end taper branch
  }

  if (need_short_wire_id_) {
    copyWireIdToVisitedShorts(fr);
    need_short_wire_id_ = false;
  }

  int to_id = -1;
  if (!rc->shape.isVia()) {
    if (firstSegmentAfterVia_) {
      firstSegmentAfterVia_ = 0;
      addPointIfExt(fr, rc);
    }
    to_id = addPoint(fr, to, rc);
  } else if (rc->shape.getTechVia()) {
    to_id = encoder_.addTechVia(rc->shape.getTechVia());
  } else if (rc->shape.getVia()) {
    to_id = encoder_.addVia(rc->shape.getVia());
  } else {
    logger_->error(ODB, 18, "error in addToWire");
  }

  if (ptV_[to].tindex >= 0 && ptV_[to].tindex != ptV_[fr].tindex
      && ptV_[to].t_alt && ptV_[to].t_alt->tindex < 0
      && !isVisited(ptV_[to].t_alt - ptV_.data())) {
    // move an ambiguous connection to the later point
    // this is for receiver; we should not get here for driver
    tmg_rcpt* pother = ptV_[to].t_alt;
    pother->tindex = ptV_[to].tindex;
    ptV_[to].tindex = -1;
  }

  if (ptV_[to].tindex >= 0) {
    tmg_rcterm* x = &termV_[ptV_[to].tindex];
    if (x->first_pt == nullptr) {
      x->first_pt = &ptV_[to];
    }
    if (x->iterm) {
      encoder_.addITerm(x->iterm);
    } else {
      encoder_.addBTerm(x->bterm);
    }
  }

  ptV_[to].dbwire_id = to_id;
  last_id_ = to_id;

  firstSegmentAfterVia_ = rc->shape.isVia();
}

}  // namespace odb
