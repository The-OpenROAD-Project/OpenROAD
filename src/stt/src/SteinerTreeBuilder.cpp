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

namespace stt{

SteinerTreeBuilder::SteinerTreeBuilder() :
  alpha_(0.3)
{
}

void SteinerTreeBuilder::init(ord::OpenRoad* openroad)
{
  db_ = openroad->getDb();
  logger_ = openroad->getLogger();
}

Tree SteinerTreeBuilder::findFluteTree(int pin_count,
                                       int x[],
                                       int y[],
                                       int accuracy)
{
  return flt::flute(pin_count, x, y, accuracy);
}

Tree SteinerTreeBuilder::findSteinerTree(odb::dbNet* net,
                                         std::vector<int> x,
                                         std::vector<int> y,
                                         int flute_accuracy,
                                         int drvr_index)
{
  Tree tree;
  float net_alpha = net_alpha_map_.find(net) != net_alpha_map_.end() ?
                    net_alpha_map_[net] : alpha_;

  if (net_alpha > 0.0) {
    tree = pdr::primDijkstra(x, y, drvr_index, net_alpha, logger_);
  } else {
    int pin_count = x.size();
    int x_arr[pin_count];
    int y_arr[pin_count];

    std::copy(x.begin(), x.end(), x_arr);
    std::copy(y.begin(), y.end(), x_arr);

    tree = flt::flute(pin_count, x_arr, y_arr, flute_accuracy);
  }

  return tree;
}

void SteinerTreeBuilder::freeTree(Tree tree)
{
  flt::free_tree(tree);
}

}