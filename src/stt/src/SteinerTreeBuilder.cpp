/////////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2019, The Regents of the University of California
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
//
///////////////////////////////////////////////////////////////////////////////

#include "stt/SteinerTreeBuilder.h"

#include <map>
#include <vector>

#include "ord/OpenRoad.hh"
#include "opendb/db.h"

namespace stt{

SteinerTreeBuilder::SteinerTreeBuilder() :
  alpha_(0.3),
  min_fanout_alpha_({std::numeric_limits<int>::max(), -1}),
  min_hpwl_alpha_({std::numeric_limits<int>::max(), -1}),
  logger_(nullptr),
  db_(nullptr)
{
}

void SteinerTreeBuilder::init(odb::dbDatabase* db, Logger* logger)
{
  db_ = db;
  logger_ = logger;
}

Tree SteinerTreeBuilder::makeSteinerTree(std::vector<int>& x,
                                         std::vector<int>& y,
                                         int drvr_index)
{
  Tree tree = makeTree(x, y, drvr_index, alpha_);

  return tree;
}

Tree SteinerTreeBuilder::makeSteinerTree(odb::dbNet* net,
                                         std::vector<int>& x,
                                         std::vector<int>& y,
                                         int drvr_index)
{
  float net_alpha = alpha_;

  if (net->getTermCount()-1 >= min_fanout_alpha_.first) {
    net_alpha = min_fanout_alpha_.second;
  }

  if (computeHPWL(net) >= min_hpwl_alpha_.first) {
    net_alpha = min_hpwl_alpha_.second;
  }

  net_alpha = net_alpha_map_.find(net) != net_alpha_map_.end() ?
              net_alpha_map_[net] : net_alpha;

  Tree tree = makeTree(x, y, drvr_index, net_alpha);

  return tree;
}

Tree SteinerTreeBuilder::makeSteinerTree(const std::vector<int>& x,
                                         const std::vector<int>& y,
                                         const std::vector<int>& s,
                                         int accuracy)
{
  Tree tree = flt::flutes(x, y, s, accuracy);
  return tree;
}

float SteinerTreeBuilder::getAlpha(const odb::dbNet* net) const
{
  float net_alpha = net_alpha_map_.find(net) != net_alpha_map_.end() ?
                    net_alpha_map_.at(net) : alpha_;
  return net_alpha;
}

Tree SteinerTreeBuilder::makeTree(std::vector<int>& x,
                                  std::vector<int>& y,
                                  int drvr_index,
                                  float alpha)
{
  Tree tree;

  if (alpha > 0.0) {
    tree = pdr::primDijkstra(x, y, drvr_index, alpha, logger_);
  } else {
    tree = flt::flute(x, y, flute_accuracy);
  }

  return tree;
}

int SteinerTreeBuilder::computeHPWL(odb::dbNet* net)
{
  int min_x = std::numeric_limits<int>::max();
  int min_y = std::numeric_limits<int>::max();
  int max_x = std::numeric_limits<int>::min();
  int max_y = std::numeric_limits<int>::min();

  for (odb::dbITerm* iterm : net->getITerms()) {
    if (iterm->getInst()->getPlacementStatus() != odb::dbPlacementStatus::NONE ||
        iterm->getInst()->getPlacementStatus() != odb::dbPlacementStatus::UNPLACED) {
      int x, y;
      iterm->getAvgXY(&x, &y);
      min_x = std::min(min_x, x);
      max_x = std::max(max_x, x);
      min_y = std::min(min_y, y);
      max_y = std::max(max_y, y);
    } else {
      logger_->error(utl::STT, 4, "Net {} is connected to unplaced instance {}.",
                     net->getName(),
                     iterm->getInst()->getName());
    }
  }

  for (odb::dbBTerm* bterm : net->getBTerms()) {
    if (bterm->getFirstPinPlacementStatus() != odb::dbPlacementStatus::NONE ||
        bterm->getFirstPinPlacementStatus() != odb::dbPlacementStatus::UNPLACED) {
      int x, y;
      bterm->getFirstPinLocation(x, y);
      min_x = std::min(min_x, x);
      max_x = std::max(max_x, x);
      min_y = std::min(min_y, y);
      max_y = std::max(max_y, y);
    } else {
      logger_->error(utl::STT, 5, "Net {} is connected to unplaced pin {}.",
                     net->getName(),
                     bterm->getName());
    }
  }

  int hpwl = (max_x - min_x) + (max_y - min_y);

  return hpwl;
}

}
