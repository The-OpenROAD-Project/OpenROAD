/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2020, OpenROAD
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

#include "opendp/Opendp.h"

#include <unordered_set>
#include <cstdlib>

#include "utility/Logger.h"

#include "opendb/dbTypes.h"

namespace dpl {

using utl::DPL;

using std::sort;
using std::unordered_set;

using odb::dbITerm;
using odb::dbOrientType;

static dbOrientType
orientMirrorY(dbOrientType orient);

NetBox::NetBox(dbNet *n) :
  net(n)
{
}

int64_t
NetBox::hpwl()
{
  return box.xMax() - box.xMin() + box.yMax() - box.yMin();
}

////////////////////////////////////////////////////////////////

void
Opendp::optimizeMirroring()
{
  NetBoxes net_boxes;
  findNetBoxes(net_boxes);
  // Sort net boxes by net hpwl.
  sort(net_boxes.begin(), net_boxes.end(),
       [] (NetBox &net_box1, NetBox &net_box2) -> bool {
         return net_box1.hpwl() > net_box2.hpwl();
       });

  vector<dbInst*> mirror_candidates;
  findMirrorCandidates(net_boxes, mirror_candidates);

  int64_t hpwl_before = hpwl();
  int mirror_count = mirrorCandidates(mirror_candidates);

  if (mirror_count > 0) {
    logger_->info(DPL, 20, "Mirrored {} instances", mirror_count);
    double hpwl_after = hpwl();
    logger_->info(DPL, 21, "HPWL before          {:8.1f} u", dbuToMicrons(hpwl_before));
    logger_->info(DPL, 22, "HPWL after           {:8.1f} u", dbuToMicrons(hpwl_after));
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
      net_box.box = getBox(net);
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
              //printf("candidate %s\n", inst->getConstName());
            }
          }
        }
      }
    }
  }
}

int
Opendp::mirrorCandidates(vector<dbInst*> &mirror_candidates)
{
  int mirror_count = 0;
  for (dbInst *inst : mirror_candidates) {
    // Use hpwl of all nets connected to the instance terms
    // before/after to determine incremental change to total hpwl.
    int64_t hpwl_before = hpwl(inst);
    dbOrientType orient = inst->getOrient();
    dbOrientType orient_my = orientMirrorY(orient);
    inst->setLocationOrient(orient_my);
    int64_t hpwl_after = hpwl(inst);
    if (hpwl_after > hpwl_before)
      // Undo mirroring if hpwl is worse.
      inst->setLocationOrient(orient);
    else {
      //printf("mirror %s\n", inst->getConstName());
      mirror_count++;
    }
  }
  return mirror_count;
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

}  // namespace
