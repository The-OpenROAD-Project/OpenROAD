/////////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2019, University of California, San Diego.
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

#pragma once

#include "CtsOptions.h"
#include "TreeBuilder.h"
#include "Clock.h"
#include "Util.h"

#include <cmath>
#include <limits>
#include <map>

namespace utl {
class Logger;
} // namespace utl

namespace cts {

using utl::Logger;

/////////////////////////////////////////////////////////////////////////
// Class: LevelBalancer
// Purpose: Balance buffer levels accross nets of same clock
// Nets driven by drivers other than clock source itself are driven by
// clock gates (CGC). Each of these nets is build independently by CTS
// Since the clock is same, large skew would be introduced between sinks
// of different clock nets.
//
// INPUT to Level Balancer
//
//                |----|>----[]  Level = 1
//                |----|>----[]
//                |----|>----[]
//   [root]-------|                   |---|>----[]   Level = 3
//                |----|>----D--------|
//                          (CGC)     |---|>-----[]
//                            
// OUTPUT of Level Balancer
//
//                |----|>-|>|>---[]  Level = 3
//                |----|>-|>|>---[]
//                |----|>-|>|>---[]
//   [root]-------|                   |---|>----[] Level = 3
//                |----|>----D--------|
//                          (CGC)     |---|>-----[]
//
//


typedef std::map<odb::dbInst*, std::pair<unsigned, TreeBuilder*>> CellLevelMap;
class LevelBalancer
{
 public:
  LevelBalancer(TreeBuilder* root, CtsOptions* options, Logger* logger):
                _root(root), _options(options), _logger(logger) { }

  void run();
  void addBufferLevels(TreeBuilder* builder, std::vector<ClockInst*> cluster,
          Clock::SubNet* driverNet, unsigned bufLevels, const std::string nameSuffix);
  void fixTreeLevels(TreeBuilder* builder, unsigned parentDepth, unsigned maxTreeDepth);
  unsigned computeMaxTreeDepth(TreeBuilder* parent);

 private:
  TreeBuilder* _root;
  CtsOptions* _options;
  Logger* _logger;
  CellLevelMap cgcLevelMap_;
};
}