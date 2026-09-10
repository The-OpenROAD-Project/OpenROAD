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
  wire_sections_.reserve(1024);
  terminals_.reserve(1024);
  tstackV_.reserve(1024);
  csVV_.reserve(1024);
  csNV_.reserve(1024);
  shorts_.reserve(1024);
  need_short_wire_id_ = false;
  first_for_clear_ = nullptr;
}

tmg_conn::~tmg_conn() = default;

int tmg_conn::distance(const int fr, const int to) const
{
  return abs(wire_points_[fr].x - wire_points_[to].x)
         + abs(wire_points_[fr].y - wire_points_[to].y);
}

WirePoint* tmg_conn::addWirePoint(int x, int y, dbTechLayer* layer)
{
  return &wire_points_.emplace_back(x, y, layer);
}

void tmg_conn::addWireSection(const dbShape& s,
                              const int from_idx,
                              const int to_idx,
                              dbTechNonDefaultRule* rule)
{
  bool is_vertical;
  int width;
  if (s.getTechVia() || s.getVia()) {
    is_vertical = false;
    width = 0;
  } else if (wire_points_[from_idx].x != wire_points_[to_idx].x) {
    is_vertical = false;
    width = s.yMax() - s.yMin();
  } else if (wire_points_[from_idx].y != wire_points_[to_idx].y) {
    is_vertical = true;
    width = s.xMax() - s.xMin();
  } else if (s.xMax() - s.xMin() == s.yMax() - s.yMin()) {
    is_vertical = false;
    width = s.xMax() - s.xMin();
  } else {
    is_vertical = false;
    width = 0;
  }
  WireSection x(from_idx,
                to_idx,
                {{s.xMin(), s.yMin(), s.xMax(), s.yMax()},
                 s.getTechLayer(),
                 s.getTechVia(),
                 s.getVia(),
                 rule},
                is_vertical,
                width,
                width / 2);
  wire_sections_.push_back(x);
}

void tmg_conn::addWireSection(const int k,
                              const tmg_rc_sh& s,
                              const int from_idx,
                              const int to_idx,
                              const int xmin,
                              const int ymin,
                              const int xmax,
                              const int ymax)
{
  const int width = wire_sections_[k].width;
  WireSection x(from_idx,
                to_idx,
                {{xmin, ymin, xmax, ymax},
                 s.getTechLayer(),
                 s.getTechVia(),
                 s.getVia(),
                 s.getRule()},
                wire_sections_[k].is_vertical,
                width,
                width / 2);
  wire_sections_.push_back(x);
}

void tmg_conn::addITerm(dbITerm* iterm)
{
  csVV_.emplace_back();
  csNV_.emplace_back();

  Terminal& x = terminals_.emplace_back(iterm);
  x.pt = nullptr;
  x.first_pt = nullptr;
}

void tmg_conn::addBTerm(dbBTerm* bterm)
{
  csVV_.emplace_back();
  csNV_.emplace_back();

  Terminal& x = terminals_.emplace_back(bterm);
  x.pt = nullptr;
  x.first_pt = nullptr;
}

void tmg_conn::addShort(const int i0, const int i1)
{
  shorts_.emplace_back(i0, i1);
  if (wire_points_[i0].fre) {
    wire_points_[i0].fre = false;
  } else {
    wire_points_[i0].jct = true;
  }
  if (wire_points_[i1].fre) {
    wire_points_[i1].fre = false;
  } else {
    wire_points_[i1].jct = true;
  }
}

void tmg_conn::loadNet(dbNet* net)
{
  net_ = net;
  wire_sections_.clear();
  wire_points_.clear();
  terminals_.clear();
  csVV_.clear();
  csNV_.clear();
  shorts_.clear();
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

      if (wire_points_.empty() || layer1 != wire_points_.back().layer
          || x1 != wire_points_.back().x || y1 != wire_points_.back().y) {
        addWirePoint(x1, y1, layer1);
      }

      addWirePoint(x2, y2, layer2);
      addWireSection(shape, wire_points_.size() - 2, wire_points_.size() - 1);
    }
  }
}

void tmg_conn::loadWire(dbWire* wire)
{
  wire_points_.clear();
  dbWirePathItr pitr;
  dbWirePath path;
  pitr.begin(wire);
  while (pitr.getNextPath(path)) {
    if (wire_points_.empty() || path.layer != wire_points_.back().layer
        || path.point.getX() != wire_points_.back().x
        || path.point.getY() != wire_points_.back().y) {
      addWirePoint(path.point.getX(), path.point.getY(), path.layer);
    }
    dbWirePathShape pathShape;
    while (pitr.getNextShape(pathShape)) {
      addWirePoint(
          pathShape.point.getX(), pathShape.point.getY(), pathShape.layer);
      addWireSection(pathShape.shape,
                     wire_points_.size() - 2,
                     wire_points_.size() - 1,
                     path.rule);
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
  tmg_rc_sh* sj = &(wire_sections_[j].shape);
  const int isVia = sj->isVia() ? 1 : 0;
  search_->searchStart(rt, {sjxMin, sjyMin, sjxMax, sjyMax}, isVia);
  int klast = -1;
  int k;
  while (search_->searchNext(&k)) {
    if (k == klast || k == j) {
      continue;
    }
    if (wire_sections_[j].to_idx == wire_sections_[k].from_idx
        || wire_sections_[j].from_idx == wire_sections_[k].to_idx) {
      continue;
    }
    sj = &(wire_sections_[j].shape);
    if (!sj->isVia()
        && wire_sections_[j].is_vertical == wire_sections_[k].is_vertical) {
      continue;
    }
    const tmg_rc_sh* sk = &(wire_sections_[k].shape);
    if (sk->isVia()) {
      continue;
    }
    dbTechLayer* tlayer = wire_points_[wire_sections_[k].from_idx].layer;
    int nxmin = sk->xMin();
    int nxmax = sk->xMax();
    int nymin = sk->yMin();
    int nymax = sk->yMax();
    int x;
    int y;
    if (wire_sections_[k].is_vertical) {
      if (sjyMin - sk->yMin() < wire_sections_[k].width) {
        continue;
      }
      if (sk->yMax() - sjyMax < wire_sections_[k].width) {
        continue;
      }
      if (wire_points_[wire_sections_[k].from_idx].y
          > wire_points_[wire_sections_[k].to_idx].y) {
        wire_sections_[k].shape.setYmin(
            wire_points_[wire_sections_[j].from_idx].y
            - (wire_sections_[k].width / 2));
        nymax = wire_points_[wire_sections_[j].from_idx].y
                + wire_sections_[k].width / 2;
      } else {
        wire_sections_[k].shape.setYmax(
            wire_points_[wire_sections_[j].from_idx].y
            + (wire_sections_[k].width / 2));
        nymin = wire_points_[wire_sections_[j].from_idx].y
                - wire_sections_[k].width / 2;
      }
      x = wire_points_[wire_sections_[k].from_idx].x;
      y = wire_points_[wire_sections_[j].from_idx].y;
    } else {
      if (sjxMin - sk->xMin() < wire_sections_[k].width) {
        continue;
      }
      if (sk->xMax() - sjxMax < wire_sections_[k].width) {
        continue;
      }
      if (wire_points_[wire_sections_[k].from_idx].x
          > wire_points_[wire_sections_[k].to_idx].x) {
        wire_sections_[k].shape.setXmin(
            wire_points_[wire_sections_[j].from_idx].x
            - (wire_sections_[k].width / 2));
        nxmax = wire_points_[wire_sections_[j].from_idx].x
                + wire_sections_[k].width / 2;
      } else {
        wire_sections_[k].shape.setXmax(
            wire_points_[wire_sections_[j].from_idx].x
            + (wire_sections_[k].width / 2));
        nxmin = wire_points_[wire_sections_[j].from_idx].x
                - wire_sections_[k].width / 2;
      }
      x = wire_points_[wire_sections_[j].from_idx].x;
      y = wire_points_[wire_sections_[k].from_idx].y;
    }
    klast = k;
    WirePoint* pt = addWirePoint(x, y, tlayer);
    pt->tindex = -1;
    pt->t_alt = nullptr;
    pt->next_for_term = nullptr;
    pt->pinpt = false;
    pt->c2pinpt = false;
    pt->next_for_clear = nullptr;
    pt->sring = nullptr;
    const int endTo = wire_sections_[k].to_idx;
    wire_sections_[k].to_idx = wire_points_.size() - 1;
    // create new WireSection
    addWireSection(k,
                   wire_sections_[k].shape,
                   wire_points_.size() - 1,
                   endTo,
                   nxmin,
                   nymin,
                   nxmax,
                   nymax);
    search_->addShape(
        rt, {nxmin, nymin, nxmax, nymax}, 0, wire_sections_.size() - 1);
  }
}

// split top of T shapes
void tmg_conn::splitTtop()
{
  for (size_t j = 0; j < wire_sections_.size(); j++) {
    tmg_rc_sh* sj = &(wire_sections_[j].shape);
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
      const int via_x = wire_points_[wire_sections_[j].from_idx].x;
      const int via_y = wire_points_[wire_sections_[j].from_idx].y;
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
  for (const Short& rcshort : shorts_) {
    if (rcshort.skip) {
      continue;
    }
    WirePoint* pfr = &wire_points_[rcshort.i0];
    WirePoint* pto = &wire_points_[rcshort.i1];
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
      WirePoint* x = pfr->sring;
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
  for (const Terminal& term : terminals_) {
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
    for (int k = 0; !sliceDone && k < terminals_.size(); k++) {
      Terminal* tx = &terminals_[k];
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
  if (wire_points_.empty()) {
    return;
  }
  if (!search_) {
    search_ = std::make_unique<tmg_conn_search>();
  }
  search_->clear();

  for (auto& pt : wire_points_) {
    pt.fre = true;
    pt.jct = false;
    pt.pinpt = false;
    pt.c2pinpt = false;
    pt.next_for_clear = nullptr;
    pt.sring = nullptr;
  }
  first_for_clear_ = nullptr;
  for (size_t j = 0; j < wire_sections_.size() - 1; j++) {
    if (wire_sections_[j].to_idx == wire_sections_[j + 1].from_idx) {
      wire_points_[wire_sections_[j].to_idx].fre = false;
    }
  }

  // put wires in search
  for (size_t j = 0; j < wire_sections_.size(); j++) {
    tmg_rc_sh* s = &(wire_sections_[j].shape);
    if (s->isVia()) {
      const int via_x = wire_points_[wire_sections_[j].from_idx].x;
      const int via_y = wire_points_[wire_sections_[j].from_idx].y;

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

  if (wire_sections_.size() < 10000) {
    splitTtop();
  }

  // find self-intersections of wires
  for (int j = 0; j < (int) wire_sections_.size() - 1; j++) {
    const tmg_rc_sh* s = &(wire_sections_[j].shape);
    const int conn_next
        = (wire_sections_[j].to_idx == wire_sections_[j + 1].from_idx);
    if (s->isVia()) {
      const int via_x = wire_points_[wire_sections_[j].from_idx].x;
      const int via_y = wire_points_[wire_sections_[j].from_idx].y;

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
  for (int j = 0; j < terminals_.size(); j++) {
    csV_ = &csVV_[j];
    csN_ = 0;
    Terminal* x = &terminals_[j];
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

  for (auto& pc : wire_points_) {
    pc.pinpt = false;
    pc.c2pinpt = false;
    pc.next_for_clear = nullptr;
    pc.sring = nullptr;
  }
  setSring();

  for (int j = 0; j < terminals_.size(); j++) {
    connectTerm(j, false);
  }
  const bool ok = checkConnected();
  if (!ok) {
    for (int j = 0; j < terminals_.size(); j++) {
      connectTerm(j, true);
    }
  }

  // make terms of shorted points consistent
  for (int it = 0; it < 5; it++) {
    int cnt = 0;
    for (const Short& rcshort : shorts_) {
      if (rcshort.skip) {
        continue;
      }
      const int i0 = rcshort.i0;
      const int i1 = rcshort.i1;
      if (wire_points_[i0].tindex < 0 && wire_points_[i1].tindex >= 0) {
        wire_points_[i0].tindex = wire_points_[i1].tindex;
        cnt++;
      }
      if (wire_points_[i1].tindex < 0 && wire_points_[i0].tindex >= 0) {
        wire_points_[i1].tindex = wire_points_[i0].tindex;
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
  const tmg_rc_sh* sa = &(wire_sections_[j].shape);
  const tmg_rc_sh* sb = &(wire_sections_[k].shape);
  const int afr = wire_sections_[j].from_idx;
  const int ato = wire_sections_[j].to_idx;
  const int bfr = wire_sections_[k].from_idx;
  const int bto = wire_sections_[k].to_idx;
  const int xlo = std::max(sa->xMin(), sb->xMin());
  const int ylo = std::max(sa->yMin(), sb->yMin());
  const int xhi = std::min(sa->xMax(), sb->xMax());
  const int yhi = std::min(sa->yMax(), sb->yMax());
  int xc = (xlo + xhi) / 2;
  int yc = (ylo + yhi) / 2;
  bool choose_afr = false;
  bool choose_bfr = false;
  if (sa->isVia() && sb->isVia()) {
    if (wire_points_[afr].layer == wire_points_[bfr].layer) {
      choose_afr = true;
      choose_bfr = true;
    } else if (wire_points_[afr].layer == wire_points_[bto].layer) {
      choose_afr = true;
      choose_bfr = false;
    } else if (wire_points_[ato].layer == wire_points_[bfr].layer) {
      choose_afr = false;
      choose_bfr = true;
    } else if (wire_points_[ato].layer == wire_points_[bto].layer) {
      choose_afr = false;
      choose_bfr = false;
    }
  } else if (sa->isVia()) {
    choose_afr = (wire_points_[afr].layer == wire_points_[bfr].layer);
    xc = wire_points_[afr].x;
    yc = wire_points_[afr].y;  // same for afr and ato
    const int dbfr
        = abs(wire_points_[bfr].x - xc) + abs(wire_points_[bfr].y - yc);
    const int dbto
        = abs(wire_points_[bto].x - xc) + abs(wire_points_[bto].y - yc);
    choose_bfr = (dbfr < dbto);
  } else if (sb->isVia()) {
    choose_bfr = (wire_points_[afr].layer == wire_points_[bfr].layer);
    xc = wire_points_[bfr].x;
    yc = wire_points_[bfr].y;
    const int dafr
        = abs(wire_points_[afr].x - xc) + abs(wire_points_[afr].y - yc);
    const int dato
        = abs(wire_points_[ato].x - xc) + abs(wire_points_[ato].y - yc);
    choose_afr = (dafr < dato);
  } else {
    // get distances to the center of intersection region, (xc,yc)
    const int dafr
        = abs(wire_points_[afr].x - xc) + abs(wire_points_[afr].y - yc);
    const int dato
        = abs(wire_points_[ato].x - xc) + abs(wire_points_[ato].y - yc);
    choose_afr = (dafr < dato);
    const int dbfr
        = abs(wire_points_[bfr].x - xc) + abs(wire_points_[bfr].y - yc);
    const int dbto
        = abs(wire_points_[bto].x - xc) + abs(wire_points_[bto].y - yc);
    choose_bfr = (dbfr < dbto);
  }
  int i0 = (choose_afr ? afr : ato);
  int i1 = (choose_bfr ? bfr : bto);
  if (i1 < i0) {
    std::swap(i0, i1);
  }
  addShort(i0, i1);
  wire_points_[i0].fre = false;
  wire_points_[i1].fre = false;
}

static void addPointToTerm(WirePoint* pt, Terminal* x)
{
  WirePoint* tpt = x->pt;
  WirePoint* ptpt = nullptr;
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

static void removePointFromTerm(WirePoint* pt, Terminal* x)
{
  if (x->pt == pt) {
    x->pt = pt->next_for_term;
    pt->next_for_term = nullptr;
    return;
  }
  WirePoint* ptpt = nullptr;
  WirePoint* tpt;
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
  for (WirePoint* pc = first_for_clear_; pc; pc = pc->next_for_clear) {
    pc->pinpt = false;
    pc->c2pinpt = false;
  }
  first_for_clear_ = nullptr;

  for (int ii = 0; ii < csN_; ii++) {
    const int k = (*csV_)[ii].k;
    WirePoint* pfr = &wire_points_[wire_sections_[k].from_idx];
    WirePoint* pto = &wire_points_[wire_sections_[k].to_idx];
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

  for (WirePoint* pc = first_for_clear_; pc; pc = pc->next_for_clear) {
    if (pc->sring) {
      int c2pinpt = pc->c2pinpt;
      for (WirePoint* x = pc->sring; x != pc; x = x->sring) {
        if (x->c2pinpt) {
          c2pinpt = 1;
        }
      }
      if (c2pinpt) {
        for (WirePoint* x = pc->sring; x != pc; x = x->sring) {
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
    WirePoint* pfr = &wire_points_[wire_sections_[k].from_idx];
    WirePoint* pto = &wire_points_[wire_sections_[k].to_idx];
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

  Terminal* x = &terminals_[j];
  for (int ii = 0; ii < csN_; ii++) {
    const int k = (*csV_)[ii].k;
    const int bfr = wire_sections_[k].from_idx;
    const int bto = wire_sections_[k].to_idx;
    const bool cfr = wire_points_[bfr].pinpt;
    const bool cto = wire_points_[bto].pinpt;
    if (soft && !cfr && !cto) {
      if (!(wire_points_[bfr].c2pinpt || wire_points_[bto].c2pinpt)) {
        connectTermSoft(j, (*csV_)[ii].rtlev, (*csV_)[ii].rect, (*csV_)[ii].k);
      }
      continue;
    }
    if (cfr && !cto) {
      WirePoint* pt = &wire_points_[bfr];
      const WirePoint* pother = &wire_points_[bto];
      if (pt->tindex == j) {
        continue;
      }
      if (pt->tindex >= 0 && pt->t_alt && pt->t_alt->tindex < 0) {
        const int oldt = pt->tindex;
        removePointFromTerm(pt, &terminals_[oldt]);
        pt->t_alt->tindex = oldt;
        addPointToTerm(pt->t_alt, &terminals_[oldt]);
        pt->tindex = -1;
      }
      if (pt->tindex >= 0 && pt->tindex == pother->tindex) {
        // override old connection if it is on the other
        removePointFromTerm(pt, &terminals_[pt->tindex]);
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
      WirePoint* pt = &wire_points_[bto];
      const WirePoint* pother = &wire_points_[bfr];
      if (pt->tindex == j) {
        continue;
      }
      if (pt->tindex >= 0 && pt->t_alt && pt->t_alt->tindex < 0) {
        const int oldt = pt->tindex;
        removePointFromTerm(pt, &terminals_[oldt]);
        pt->t_alt->tindex = oldt;
        addPointToTerm(pt->t_alt, &terminals_[oldt]);
        pt->tindex = -1;
      }
      if (pt->tindex >= 0 && pt->tindex == pother->tindex) {
        // override old connection if it is on the other
        removePointFromTerm(pt, &terminals_[pt->tindex]);
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
      if (wire_points_[bfr].tindex == j || wire_points_[bto].tindex == j) {
        continue;
      }
      if (wire_points_[bfr].tindex >= 0 && wire_points_[bto].tindex < 0) {
        WirePoint* pt = &wire_points_[bto];
        pt->tindex = j;
        addPointToTerm(pt, x);
        continue;
      }
      WirePoint* pt = &wire_points_[bfr];
      WirePoint* pother = &wire_points_[bto];
      if (pt->tindex >= 0 && pt->t_alt && pt->t_alt->tindex < 0) {
        const int oldt = pt->tindex;
        removePointFromTerm(pt, &terminals_[oldt]);
        pt->t_alt->tindex = oldt;
        addPointToTerm(pt->t_alt, &terminals_[oldt]);
        pt->tindex = -1;
      }
      if (pt->tindex >= 0 && pt->tindex == pother->tindex) {
        // override old connection if it is on the other
        removePointFromTerm(pt, &terminals_[pt->tindex]);
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
  for (WirePoint* pc = first_for_clear_; pc; pc = pc->next_for_clear) {
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
  const tmg_rc_sh* sb = &(wire_sections_[k].shape);
  const int bfr = wire_sections_[k].from_idx;
  const int bto = wire_sections_[k].to_idx;
  const int xlo = std::max(rect.xMin(), sb->xMin());
  const int ylo = std::max(rect.yMin(), sb->yMin());
  const int xhi = std::min(rect.xMax(), sb->xMax());
  const int yhi = std::min(rect.yMax(), sb->yMax());
  const int xc = (xlo + xhi) / 2;
  const int yc = (ylo + yhi) / 2;
  bool choose_bfr = false;
  bool has_alt = true;
  if (sb->isVia()) {
    choose_bfr = (rt == wire_points_[bfr].layer->getRoutingLevel());
    has_alt = false;
  } else {
    const int dbfr
        = abs(wire_points_[bfr].x - xc) + abs(wire_points_[bfr].y - yc);
    const int dbto
        = abs(wire_points_[bto].x - xc) + abs(wire_points_[bto].y - yc);
    choose_bfr = (dbfr < dbto);
    if (abs(dbfr - dbto) > 5000) {
      has_alt = false;
    }
  }
  WirePoint* pt = &wire_points_[choose_bfr ? bfr : bto];
  WirePoint* pother = &wire_points_[choose_bfr ? bto : bfr];
  if (pt->tindex == j) {
    return;
  }

  // This was needed in a case where a square patch
  // of M1 was used to connect pins A and B of an instance.
  // The original input def looked like:
  // NEW M1 ( 2090900 1406000 ) ( * 1406000 ) NEW M1 ...
  // In this case we get two wire_points_[] points, that have identical
  // x,y,layer, and we connect one iterm to each.
  if (pt->tindex >= 0 && wire_points_[bfr].x == wire_points_[bto].x
      && wire_points_[bfr].y == wire_points_[bto].y) {
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
    removePointFromTerm(pt, &terminals_[pt->tindex]);
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
  Terminal* x = &terminals_[j];
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
  for (const Terminal& x : terminals_) {
    if (x.iterm == it_drv && x.bterm == bt_drv) {
      if (!x.pt) {
        break;
      }

      return (x.pt - wire_points_.data());
    }
  }

  // On a 3D-IC design, the starting point of an input bump net is the bump
  // iterm even though such a net does have a bterm. The bterm has no geometry
  // and only serves to represent the logical connectivity. The following code
  // will NOT work if a die with such a net is loaded independently as a 2D
  // design. This will need to be revisited.
  if (bt_drv && bt_drv->getBPins().empty()) {
    dbChipBump* chip_bump = bt_drv->getChipBump();

    if (chip_bump) {
      dbInst* bump = chip_bump->getInst();

      for (const Terminal& rc_term : terminals_) {
        dbITerm* iterm = rc_term.iterm;

        if (iterm && (iterm->getInst() == bump) && rc_term.pt) {
          return (rc_term.pt - wire_points_.data());
        }
      }
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
    if (wire_points_.empty()) {
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
  for (const Terminal& x : terminals_) {
    if (x.pt == nullptr) {
      return false;
    }
  }
  if (terminals_.empty()) {
    return true;
  }
  tstackV_.clear();
  int jstart = getStartNode();
  Terminal* xstart = nullptr;
  if (wire_points_[jstart].tindex >= 0) {
    Terminal* x = &terminals_[wire_points_[jstart].tindex];
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
      if (wire_points_[jto].tindex >= 0) {
        Terminal* x = &terminals_[wire_points_[jto].tindex];
        if (x == xstart && !is_short) {
          // removing multi-connection at driver
          removePointFromTerm(&wire_points_[jto],
                              &terminals_[wire_points_[jto].tindex]);
          wire_points_[jto].tindex = -1;
          wire_points_[jto].t_alt = nullptr;
        } else if (x->pt && x->pt->next_for_term) {
          // add potential short-from points to stack
          tstackV_.push_back(x);
        }
      }
      // the part of addToWire needed in no_convert case
      if (wire_points_[jfr].tindex >= 0) {
        Terminal* x = &terminals_[wire_points_[jfr].tindex];
        if (x->first_pt == nullptr) {
          x->first_pt = &wire_points_[jfr];
        }
      }
      if (wire_points_[jto].tindex >= 0) {
        Terminal* x = &terminals_[wire_points_[jto].tindex];
        if (x->first_pt == nullptr) {
          x->first_pt = &wire_points_[jto];
        }
      }
    }
    // finished physically-connected subtree,
    // find an unvisited short-from point
    WirePoint* pt = nullptr;
    while (tstack0 < tstackV_.size() && !pt) {
      Terminal* x = tstackV_[tstack0++];
      for (pt = x->pt; pt; pt = pt->next_for_term) {
        if (!isVisited(pt - wire_points_.data())) {
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
    jstart = pt - wire_points_.data();
    if (!dfsStart(jstart)) {
      return false;
    }
  }
  bool con = true;
  for (Terminal& x : terminals_) {
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
  if (wire_points_.empty()) {
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
    for (WirePoint& pt : wire_points_) {
      pt.dbwire_id = -1;
    }
  }
  for (Terminal& x : terminals_) {
    x.first_pt = nullptr;
    if (x.pt == nullptr) {
      connected_ = false;
    }
  }

  if (terminals_.empty()) {
    return;
  }

  net_rule_ = net_->getNonDefaultRule();
  path_rule_ = net_rule_;

  int tstack0 = 0;
  tstackV_.clear();
  int jstart = getStartNode();
  Terminal* xstart = nullptr;
  if (wire_points_[jstart].tindex >= 0) {
    Terminal* x = &terminals_[wire_points_[jstart].tindex];
    xstart = x;
    tstackV_.push_back(x);
  }
  dfsClear();
  if (!dfsStart(jstart)) {
    logger_->error(ODB,
                   395,
                   "Could not order wires of net {}. No wire segment is "
                   "reachable from the start point ({} {}).",
                   net_->getConstName(),
                   wire_points_[jstart].x,
                   wire_points_[jstart].y);
  }
  int last_term_index = 0;
  while (true) {
    // do a physically-connected subtree
    Terminal* x = nullptr;
    int jfr, jto, k;
    bool is_short, is_loop;
    while (dfsNext(&jfr, &jto, &k, &is_short, &is_loop)) {
      x = nullptr;
      if (wire_points_[jto].tindex >= 0) {
        x = &terminals_[wire_points_[jto].tindex];
        if (x == xstart && !is_short) {
          // removing multi-connection at driver
          removePointFromTerm(&wire_points_[jto],
                              &terminals_[wire_points_[jto].tindex]);
          wire_points_[jto].tindex = -1;
          wire_points_[jto].t_alt = nullptr;
        } else if (x->pt && x->pt->next_for_term) {
          // add potential short-from points to stack
          tstackV_.push_back(x);
        }
      }
      if (!no_convert) {
        addToWire(jfr, jto, k, is_short, is_loop);
      } else {
        // the part of addToWire needed in no_convert case
        if (wire_points_[jfr].tindex >= 0) {
          x = &terminals_[wire_points_[jfr].tindex];
          if (x->first_pt == nullptr) {
            x->first_pt = &wire_points_[jfr];
          }
        }
        if (wire_points_[jto].tindex >= 0) {
          x = &terminals_[wire_points_[jto].tindex];
          if (x->first_pt == nullptr) {
            x->first_pt = &wire_points_[jto];
          }
        }
      }
    }
    // finished physically-connected subtree,
    // find an unvisited short-from point
    WirePoint* pt = nullptr;
    while (tstack0 < tstackV_.size() && !pt) {
      x = tstackV_[tstack0++];
      for (pt = x->pt; pt; pt = pt->next_for_term) {
        if (!isVisited(pt - wire_points_.data())) {
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
      for (j = last_term_index; j < terminals_.size(); j++) {
        x = &terminals_[j];
        if (x->pt && !isVisited(x->pt - wire_points_.data())) {
          break;
        }
      }
      last_term_index = j;
      if (j < terminals_.size()) {
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
        pt = &wire_points_[jstart];
        last_id_ = -1;
      }
    }
    jstart = pt - wire_points_.data();
    if (!dfsStart(jstart)) {
      logger_->error(ODB,
                     396,
                     "Could not order wires of net {}. No wire segment is "
                     "reachable from the branch start point ({} {}).",
                     net_->getConstName(),
                     wire_points_[jstart].x,
                     wire_points_[jstart].y);
    }
  }

  checkVisited();
  if (!no_convert) {
    encoder_.end();
  }
}

int tmg_conn::getExtension(const int ipt, const WireSection* wire_section)
{
  const WirePoint* p = &wire_points_[ipt];
  WirePoint* pto;
  if (ipt == wire_section->from_idx) {
    pto = &wire_points_[wire_section->to_idx];
  } else if (ipt == wire_section->to_idx) {
    pto = &wire_points_[wire_section->from_idx];
  } else {
    logger_->error(ODB, 16, "problem in getExtension()");
  }
  int ext = wire_section->default_ext;
  if (p->x < pto->x) {
    ext = p->x - wire_section->shape.xMin();
  } else if (p->x > pto->x) {
    ext = wire_section->shape.xMax() - p->x;
  } else if (p->y < pto->y) {
    ext = p->y - wire_section->shape.yMin();
  } else if (p->y > pto->y) {
    ext = wire_section->shape.yMax() - p->y;
  }
  return ext;
}

int tmg_conn::addPoint(const int ipt, const WireSection* wire_section)
{
  int wire_id;
  const WirePoint* p = &wire_points_[ipt];
  const int ext = getExtension(ipt, wire_section);
  if (ext == wire_section->default_ext) {
    wire_id = encoder_.addPoint(p->x, p->y);
  } else {
    wire_id = encoder_.addPoint(p->x, p->y, ext);
  }
  return wire_id;
}

int tmg_conn::addPoint(const int from_idx,
                       const int ipt,
                       const WireSection* wire_section)
{
  int wire_id;
  const WirePoint* p = &wire_points_[ipt];
  const int ext = getExtension(ipt, wire_section);
  if (ext == wire_section->default_ext) {
    wire_id = encoder_.addPoint(p->x, p->y);
  } else {
    wire_id = encoder_.addPoint(p->x, p->y, ext);
  }
  return wire_id;
}

int tmg_conn::addPointIfExt(const int ipt, const WireSection* wire_section)
{
  // for first wire after a via, need to add a point
  // only if the extension is not the default ext
  int wire_id = 0;
  const WirePoint* p = &wire_points_[ipt];
  const int ext = getExtension(ipt, wire_section);
  if (ext != wire_section->default_ext) {
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

  const int xfr = wire_points_[fr].x;
  const int yfr = wire_points_[fr].y;
  const int xto = wire_points_[to].x;
  const int yto = wire_points_[to].y;

  if (is_short) {
    if (xfr != xto || yfr != yto) {
      wire_points_[to].dbwire_id = -1;
      last_id_ = wire_points_[fr].dbwire_id;
      return;
    }
    if (wire_points_[fr].dbwire_id < 0) {
      need_short_wire_id_ = true;
      return;
    }
    wire_points_[to].dbwire_id = wire_points_[fr].dbwire_id;
    return;
  }
  if (k < 0) {
    logger_->error(
        ODB, 393, "tmg_conn::addToWire: value of k is negative: {}", k);
  }

  WireSection* wire_section = (k >= 0) ? &wire_sections_[k] : nullptr;
  int fr_id = wire_points_[fr].dbwire_id;
  dbTechLayerRule* lyr_rule = nullptr;
  if (wire_section->shape.getRule()) {
    lyr_rule
        = wire_section->shape.getRule()->getLayerRule(wire_points_[fr].layer);
  }
  if (fr_id < 0) {
    path_rule_ = wire_section->shape.getRule();
    firstSegmentAfterVia_ = 0;
    if (last_id_ >= 0) {
      // term feedthru
      if (path_rule_) {
        encoder_.newPathShort(
            last_id_, wire_points_[fr].layer, dbWireType::ROUTED, lyr_rule);
      } else {
        encoder_.newPathShort(
            last_id_, wire_points_[fr].layer, dbWireType::ROUTED);
      }
    } else {
      if (path_rule_) {
        encoder_.newPath(wire_points_[fr].layer, dbWireType::ROUTED, lyr_rule);
      } else {
        encoder_.newPath(wire_points_[fr].layer, dbWireType::ROUTED);
      }
    }
    if (!wire_section->shape.isVia()) {
      fr_id = addPoint(fr, wire_section);
    } else {
      fr_id = encoder_.addPoint(xfr, yfr);
    }
    wire_points_[fr].dbwire_id = fr_id;
    if (wire_points_[fr].tindex >= 0) {
      Terminal* x = &terminals_[wire_points_[fr].tindex];
      if (x->first_pt == nullptr) {
        x->first_pt = &wire_points_[fr];
      }
      if (x->iterm) {
        encoder_.addITerm(x->iterm);
      } else {
        encoder_.addBTerm(x->bterm);
      }
    }
  } else if (fr_id != last_id_) {
    path_rule_ = wire_section->shape.getRule();
    if (wire_section->shape.isVia()) {
      if (path_rule_) {
        encoder_.newPath(fr_id, lyr_rule);
      } else {
        encoder_.newPath(fr_id);
      }
    } else {
      firstSegmentAfterVia_ = 0;
      const int ext = getExtension(fr, wire_section);
      if (ext != wire_section->default_ext) {
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
    if (wire_points_[fr].tindex >= 0) {
      Terminal* x = &terminals_[wire_points_[fr].tindex];
      if (x->first_pt == nullptr) {
        x->first_pt = &wire_points_[fr];
      }
      if (x->iterm) {
        encoder_.addITerm(x->iterm);
      } else {
        encoder_.addBTerm(x->bterm);
      }
    }
  } else if (path_rule_ != wire_section->shape.getRule()) {
    // make a branch, for taper

    path_rule_ = wire_section->shape.getRule();
    if (wire_section->shape.isVia()) {
      if (path_rule_) {
        encoder_.newPath(fr_id, lyr_rule);
      } else {
        encoder_.newPath(fr_id);
      }
    } else {
      firstSegmentAfterVia_ = 0;
      const int ext = getExtension(fr, wire_section);
      if (ext != wire_section->default_ext) {
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
    if (wire_points_[fr].tindex >= 0) {
      Terminal* x = &terminals_[wire_points_[fr].tindex];
      if (x->first_pt == nullptr) {
        x->first_pt = &wire_points_[fr];
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
  if (!wire_section->shape.isVia()) {
    if (firstSegmentAfterVia_) {
      firstSegmentAfterVia_ = 0;
      addPointIfExt(fr, wire_section);
    }
    to_id = addPoint(fr, to, wire_section);
  } else if (wire_section->shape.getTechVia()) {
    to_id = encoder_.addTechVia(wire_section->shape.getTechVia());
  } else if (wire_section->shape.getVia()) {
    to_id = encoder_.addVia(wire_section->shape.getVia());
  } else {
    logger_->error(ODB, 18, "error in addToWire");
  }

  if (wire_points_[to].tindex >= 0
      && wire_points_[to].tindex != wire_points_[fr].tindex
      && wire_points_[to].t_alt && wire_points_[to].t_alt->tindex < 0
      && !isVisited(wire_points_[to].t_alt - wire_points_.data())) {
    // move an ambiguous connection to the later point
    // this is for receiver; we should not get here for driver
    WirePoint* pother = wire_points_[to].t_alt;
    pother->tindex = wire_points_[to].tindex;
    wire_points_[to].tindex = -1;
  }

  if (wire_points_[to].tindex >= 0) {
    Terminal* x = &terminals_[wire_points_[to].tindex];
    if (x->first_pt == nullptr) {
      x->first_pt = &wire_points_[to];
    }
    if (x->iterm) {
      encoder_.addITerm(x->iterm);
    } else {
      encoder_.addBTerm(x->bterm);
    }
  }

  wire_points_[to].dbwire_id = to_id;
  last_id_ = to_id;

  firstSegmentAfterVia_ = wire_section->shape.isVia();
}

}  // namespace odb
