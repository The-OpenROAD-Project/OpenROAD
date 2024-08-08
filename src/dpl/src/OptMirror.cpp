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

#include "dpl/OptMirror.h"

#include <cstdlib>
#include <unordered_set>

#include "dpl/Opendp.h"
#include "odb/dbTypes.h"
#include "odb/util.h"
#include "utl/Logger.h"

namespace dpl {

using utl::DPL;

using std::sort;
using std::unordered_set;

using odb::dbITerm;
using odb::dbOrientType;

static dbOrientType orientMirrorY(const dbOrientType& orient);

NetBox::NetBox(dbNet* net, Rect box, bool ignore)
    : net_(net), box_(box), ignore_(ignore)
{
}

int64_t NetBox::hpwl()
{
  return box_.dy() + box_.dx();
}

void NetBox::saveBox()
{
  box_saved_ = box_;
}

void NetBox::restoreBox()
{
  box_ = box_saved_;
}

////////////////////////////////////////////////////////////////

OptimizeMirroring::OptimizeMirroring(Logger* logger, odb::dbDatabase* db)
    : logger_(logger), db_(db), block_(db_->getChip()->getBlock())
{
}

void OptimizeMirroring::run()
{
  findNetBoxes();

  NetBoxes sorted_boxes;
  for (auto& net_box : net_box_map_) {
    sorted_boxes.push_back(&net_box.second);
  }

  // Sort net boxes by net hpwl.
  sort(sorted_boxes.begin(),
       sorted_boxes.end(),
       [](NetBox* net_box1, NetBox* net_box2) -> bool {
         return net_box1->hpwl() > net_box2->hpwl();
       });

  vector<dbInst*> mirror_candidates = findMirrorCandidates(sorted_boxes);
  odb::WireLengthEvaluator eval(block_);
  int64_t hpwl_before = eval.hpwl();
  int mirror_count = mirrorCandidates(mirror_candidates);

  if (mirror_count > 0) {
    logger_->info(DPL, 20, "Mirrored {} instances", mirror_count);
    double hpwl_after = eval.hpwl();
    logger_->info(DPL,
                  21,
                  "HPWL before          {:8.1f} u",
                  block_->dbuToMicrons(hpwl_before));
    logger_->info(DPL,
                  22,
                  "HPWL after           {:8.1f} u",
                  block_->dbuToMicrons(hpwl_after));
    double hpwl_delta = (hpwl_before != 0)
                            ? (hpwl_after - hpwl_before) / hpwl_before * 100
                            : 0.0;
    logger_->info(DPL, 23, "HPWL delta           {:8.1f} %", hpwl_delta);
  }
}

void OptimizeMirroring::findNetBoxes()
{
  net_box_map_.clear();
  auto nets = block_->getNets();
  for (dbNet* net : nets) {
    bool ignore = net->getSigType().isSupply()
                  || net->isSpecial()
                  // Reducing HPWL on large nets (like clocks) is irrelevant
                  // to mirroring criterra.
                  // Note that getITerms().size() iteractes through the iterms
                  // so it has to be checked once here instead of where it is
                  // needed.
                  || net->getITerms().size() > mirror_max_iterm_count_;
    if (ignore) {
      debugPrint(
          logger_, DPL, "opt_mirror", 2, "ignore {}", net->getConstName());
    }
    net_box_map_[net] = NetBox(net, net->getTermBBox(), ignore);
  }
}

vector<dbInst*> OptimizeMirroring::findMirrorCandidates(NetBoxes& net_boxes)
{
  vector<dbInst*> mirror_candidates;
  unordered_set<dbInst*> existing;
  // Find inst terms on the boundary of the net boxes.
  for (NetBox* net_box : net_boxes) {
    if (!net_box->ignore_) {
      dbNet* net = net_box->net_;
      Rect& box = net_box->box_;
      for (dbITerm* iterm : net->getITerms()) {
        dbInst* inst = iterm->getInst();
        int x, y;
        if (inst->isCore() && !inst->isFixed() && iterm->getAvgXY(&x, &y)
            && (x == box.xMin() || x == box.xMax() || y == box.yMin()
                || y == box.yMax())) {
          dbInst* inst = iterm->getInst();
          if (existing.find(inst) == existing.end()) {
            mirror_candidates.push_back(inst);
            existing.insert(inst);
            debugPrint(logger_,
                       DPL,
                       "opt_mirror",
                       1,
                       "candidate {}",
                       inst->getConstName());
          }
        }
      }
    }
  }
  return mirror_candidates;
}

int OptimizeMirroring::mirrorCandidates(vector<dbInst*>& mirror_candidates)
{
  int mirror_count = 0;
  for (dbInst* inst : mirror_candidates) {
    // Use hpwl of all nets connected to the instance terms
    // before/after to determine incremental change to total hpwl.
    int64_t hpwl_before = hpwl(inst);
    saveNetBoxes(inst);
    dbOrientType orient = inst->getOrient();
    dbOrientType orient_my = orientMirrorY(orient);
    inst->setLocationOrient(orient_my);
    updateNetBoxes(inst);
    int64_t hpwl_after = hpwl(inst);
    if (hpwl_after > hpwl_before) {
      // Undo mirroring if hpwl is worse.
      inst->setLocationOrient(orient);
      restoreNetBoxes(inst);
    } else {
      debugPrint(
          logger_, DPL, "opt_mirror", 1, "mirror {}", inst->getConstName());
      mirror_count++;
    }
  }
  return mirror_count;
}

// apply mirror about Y axis to orient
static dbOrientType orientMirrorY(const dbOrientType& orient)
{
  switch (orient.getValue()) {
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

int64_t OptimizeMirroring::hpwl(dbInst* inst)
{
  int64_t inst_hpwl = 0;
  for (dbITerm* iterm : inst->getITerms()) {
    dbNet* net = iterm->getNet();
    if (net) {
      NetBox& net_box = net_box_map_[net];
      if (!net_box.ignore_) {
        inst_hpwl += net_box.hpwl();
      }
    }
  }
  return inst_hpwl;
}

void OptimizeMirroring::updateNetBoxes(dbInst* inst)
{
  for (dbITerm* iterm : inst->getITerms()) {
    dbNet* net = iterm->getNet();
    if (net) {
      NetBox& net_box = net_box_map_[net];
      if (!net_box.ignore_) {
        net_box_map_[net].box_ = net->getTermBBox();
      }
    }
  }
}

void OptimizeMirroring::saveNetBoxes(dbInst* inst)
{
  for (dbITerm* iterm : inst->getITerms()) {
    dbNet* net = iterm->getNet();
    if (net) {
      net_box_map_[net].saveBox();
    }
  }
}

void OptimizeMirroring::restoreNetBoxes(dbInst* inst)
{
  for (dbITerm* iterm : inst->getITerms()) {
    dbNet* net = iterm->getNet();
    if (net) {
      net_box_map_[net].restoreBox();
    }
  }
}

}  // namespace dpl
