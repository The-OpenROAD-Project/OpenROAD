/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2020, The Regents of the University of California
// All rights reserved.
//
// BSD 3-Clause License
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
///////////////////////////////////////////////////////////////////////////////

#include <unordered_set>
#include <cstdlib>
#include <vector>

#include "dpl/Opendp.h"

#include "utl/Logger.h"

#include "odb/dbTypes.h"

namespace dpl {

using utl::DPL;

using std::sort;
using std::unordered_set;
using std::vector;
using std::map;
using std::pair;

using odb::dbITerm;
using odb::dbOrientType;
using odb::dbNet;

static dbOrientType
orientMirrorY(dbOrientType orient);

NetBox::NetBox(dbNet *n) :
  net(n)
{
}

int64_t
NetBox::hpwl()
{
  return box.dy() + box.dx();
}

////////////////////////////////////////////////////////////////

void
Opendp::optimizeMirroring()
{
  block_ = db_->getChip()->getBlock();
  NetBoxes net_boxes;
  findNetBoxes(net_boxes);
  // Sort net boxes by net hpwl.
  sort(net_boxes.begin(), net_boxes.end(),
       [] (NetBox &net_box1, NetBox &net_box2) -> bool {
         return net_box1.hpwl() > net_box2.hpwl();
       });

  vector<dbInst*> mirror_candidates;
  findMirrorCandidates(net_boxes, mirror_candidates);

  // Generate net pointer to net box mapping
  map<dbNet*, Rect> net2box;
  for (NetBox &net_box : net_boxes) {
    dbNet *net = net_box.net;
    Rect &box = net_box.box;
    net2box[net] = box;
  }


  int64_t hpwl_before = hpwl();
  int mirror_count = mirrorCandidates(mirror_candidates, net2box);

  if (mirror_count > 0) {
    logger_->info(DPL, 20, "Mirrored {} instances", mirror_count);
    double hpwl_after = hpwl();
    logger_->info(DPL, 21, "HPWL before          {:8.1f} u",
                  dbuToMicrons(hpwl_before));
    logger_->info(DPL, 22, "HPWL after           {:8.1f} u",
                  dbuToMicrons(hpwl_after));
    double hpwl_delta = (hpwl_before != 0.0)
      ? (hpwl_after - hpwl_before) / hpwl_before * 100
      : 0.0;
    logger_->info(DPL, 23, "HPWL delta           {:8.1f} %", hpwl_delta);
  }
}

void
Opendp::findNetBoxes(NetBoxes &net_boxes)
{
  auto nets = block_->getNets();
  net_boxes.reserve(nets.size());
  for (dbNet *net : nets) {
    if (!isSupply(net)
        && !net->isSpecial()) {
      NetBox net_box(net);
      net_box.box = net->getTermBBox();
      net_boxes.push_back(net_box);
    }
  }
}

void
Opendp::findMirrorCandidates(NetBoxes &net_boxes,
                             vector<dbInst*> &mirror_candidates)
{
  unordered_set<dbInst*> existing;
  // Find inst terms on the boundary of the net boxes.
  for (NetBox &net_box : net_boxes) {
    dbNet *net = net_box.net;
    Rect &box = net_box.box;
    for (dbITerm *iterm : net->getITerms()) {
      dbInst *inst = iterm->getInst();
      if (inst->isCore()
          && !inst->isFixed()) {
        int x, y;
        if (iterm->getAvgXY(&x, &y)) {
          if (x == box.xMin() || x == box.xMax()
              || y == box.yMin() || y == box.yMax()) {
            dbInst *inst = iterm->getInst();
            if (existing.find(inst) == existing.end()) {
              mirror_candidates.push_back(inst);
              existing.insert(inst);
              // printf("candidate %s\n", inst->getConstName());
            }
          }
        }
      }
    }
  }
}

int
Opendp::mirrorCandidates(vector<dbInst*> &mirror_candidates,
                          map<dbNet*, Rect> &net2box) {
  int mirror_count = 0;
  for (dbInst *inst : mirror_candidates) {
    // Use hpwl of all nets connected to the instance terms
    // before/after to determine incremental change to total hpwl.
    dbOrientType orient = inst->getOrient();
    dbOrientType orient_my = orientMirrorY(orient);
    int pt_x, pt_y;
    inst->getLocation(pt_x, pt_y);
    vector<pair<dbNet*, Rect*>> updated_box;
    int64_t delta = hpwlIncrement(inst, pt_x, true, net2box, updated_box);

    if (delta <= 0) {
      mirror_count++;
      inst->setLocationOrient(orient_my);
      for (auto item : updated_box) {
        net2box[item.first].reset(item.second->xMin(), item.second->yMin(),
                                  item.second->xMax(), item.second->yMax());
      }
    }
  }
  return mirror_count;
}

int64_t
Opendp::hpwl(dbInst *inst)
{
  int64_t inst_hpwl = 0;
  for (dbITerm *iterm : inst->getITerms()) {
    dbNet *net = iterm->getNet();
    if (net)
      inst_hpwl += hpwl(net);
  }
  return inst_hpwl;
}

int64_t
Opendp::hpwlIncrement(dbInst *inst, int pt_x, bool mirror,
                      map<dbNet*, Rect> &net2box,
                      vector<pair<dbNet*, Rect*>> &updated_box) {
  int64_t delta = 0;

  // Net to insterm mapping
  map<dbNet*, vector<dbITerm *>> net2iterm;

  for (dbITerm *iterm : inst->getITerms()) {
    if (iterm->getNet() != NULL) {
      dbNet *net = iterm->getNet();

      if (net2iterm.find(net) == net2iterm.end()) {
        net2iterm[net] = vector<dbITerm *>();
        net2iterm[net].push_back(iterm);
      } else {
        net2iterm[net].push_back(iterm);
      }
    }
  }

  for ( auto &item : net2iterm ) {
    dbNet *net = item.first;
    if (!isSupply(net)) {
      Rect *new_net_box = new Rect();
      delta += hpwlIncrementHelper(inst, item.second, net, pt_x, mirror,
                                  new_net_box, net2box[net]);
      updated_box.push_back({net, new_net_box});
    }
  }
  return delta;
}

int64_t
Opendp::hpwlIncrementHelper(dbInst *inst, vector<dbITerm *> &iterms,
                      dbNet *net, int pt_x, bool mirror, Rect *new_net_box,
                      Rect netBox) {
  int64_t net_hpwl = netBox.dx() + netBox.dy();
  Rect iterm_box;
  iterm_box.mergeInit();
  int cellPtX, cellPtY;
  inst->getLocation(cellPtX, cellPtY);
  int cellWidth = inst->getMaster()->getWidth();

  for (int i = 0; i < iterms.size(); i++) {
    dbITerm *iterm = iterms[i];
    int x, y;
    if (iterm->getAvgXY(&x, &y)) {
      Rect iterm_rect(x, y, x, y);
      iterm_box.merge(iterm_rect);
    } else {
      // This clause is sort of worthless because getAvgXY prints
      // a warning when it fails.
      dbInst *inst = iterm->getInst();
      odb::dbBox *inst_box = inst->getBBox();
      int center_x = (inst_box->xMin() + inst_box->xMax()) / 2;
      int center_y = (inst_box->yMin() + inst_box->yMax()) / 2;
      Rect iterm_rect(center_x, center_y, center_x, center_y);
      iterm_box.merge(iterm_rect);
    }
  }

  // Based on current location check the pin location in the netbox
  bool isInside = netBox.inside(iterm_box);

  // Calculate the new pin delta displacement
  int mirror_x = 0;
  if (mirror) {
    mirror_x = 2 * cellPtX + cellWidth - iterm_box.xMin() - iterm_box.xMax();
  }

  int delta_x = pt_x - cellPtX + mirror_x;

  // Considering there wont be any movement along the y axis
  iterm_box.moveDelta(delta_x, 0);
  bool isContain = netBox.contains(iterm_box);

  // Pin term is inside the net box before and after movement.
  if (isInside && isContain) {
    new_net_box->reset(netBox.xMin(), netBox.yMin(), netBox.xMax(),
                        netBox.yMax());
    return 0;
  }

  // Pin term is initially inside the net box but after
  // movement is outside the netbox
  if (isInside) {
    netBox.merge(iterm_box);
    int64_t new_hpwl = netBox.dx() + netBox.dy();
    new_net_box->reset(netBox.xMin(), netBox.yMin(), netBox.xMax(),
                        netBox.yMax());
    return new_hpwl - net_hpwl;
  }

  // Re calculate the Net box with the updated iterm location
  new_net_box->mergeInit();

  for (dbITerm *iterm_ : net->getITerms()) {
    int i = std::find(iterms.begin(), iterms.end(), iterm_) - iterms.begin();
    if (i >= iterms.size()) {
      int x, y;
      if (iterm_->getAvgXY(&x, &y)) {
        Rect iterm_rect_(x, y, x, y);
        new_net_box->merge(iterm_rect_);
      } else {
        // This clause is sort of worthless because getAvgXY prints
        // a warning when it fails.
        dbInst *inst = iterm_->getInst();
        odb::dbBox *inst_box = inst->getBBox();
        int center_x = (inst_box->xMin() + inst_box->xMax()) / 2;
        int center_y = (inst_box->yMin() + inst_box->yMax()) / 2;
        Rect inst_center(center_x, center_y, center_x, center_y);
        new_net_box->merge(inst_center);
      }
    }
  }
  new_net_box->merge(iterm_box);
  for (odb::dbBTerm *bterm : net->getBTerms()) {
    for (odb::dbBPin *bpin : bterm->getBPins()) {
      odb::dbPlacementStatus status = bpin->getPlacementStatus();
      if (status.isPlaced()) {
        Rect pin_bbox = bpin->getBBox();
        int center_x = (pin_bbox.xMin() + pin_bbox.xMax()) / 2;
        int center_y = (pin_bbox.yMin() + pin_bbox.yMax()) / 2;
        Rect pin_center(center_x, center_y, center_x, center_y);
        new_net_box->merge(pin_center);
      }
    }
  }
  int64_t new_hpwl = new_net_box->dx() + new_net_box->dy();
  return new_hpwl - net_hpwl;
}

// apply mirror about Y axis to orient
static dbOrientType
orientMirrorY(dbOrientType orient)
{
  switch (orient) {
  case dbOrientType::R0:
    return dbOrientType::MY;
  case dbOrientType::MX:
    return dbOrientType::R180;
  case dbOrientType::MY:
    return dbOrientType::R0;
  case dbOrientType::R180:
    return dbOrientType::MX;
  case dbOrientType::R90:
    return dbOrientType::MXR90;
  case dbOrientType::MXR90:
    return dbOrientType::R90;
  case dbOrientType::R270:
    return dbOrientType::MYR90;
  case dbOrientType::MYR90:
    return dbOrientType::R270;
  }
  // make lame gcc happy
  std::abort();
  return dbOrientType::R0;
}

}  // namespace dpl
