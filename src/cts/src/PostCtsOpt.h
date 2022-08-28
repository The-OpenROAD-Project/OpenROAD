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

#pragma once

#include <unordered_map>

#include "Clock.h"
#include "CtsOptions.h"
#include "HTreeBuilder.h"

namespace utl {
class Logger;
}  // namespace utl

namespace cts {

using utl::Logger;

class PostCtsOpt
{
 public:
  PostCtsOpt(TreeBuilder* builder,
             CtsOptions* options,
             TechChar* techChar,
             Logger* logger);

  void run();

 private:
  void initSourceSinkDists();
  void computeNetSourceSinkDists(const Clock::SubNet& subNet);
  void fixSourceSinkDists();
  void fixNetSourceSinkDists(Clock::SubNet& subNet);
  void fixLongWire(Clock::SubNet& net, ClockInst* driver, ClockInst* sink);
  void createSubClockNet(Clock::SubNet& net,
                         ClockInst* driver,
                         ClockInst* sink);
  Point<int> computeBufferLocation(ClockInst* driver, ClockInst* sink) const;

  Clock* clock_;
  CtsOptions* options_;
  TechChar* techChar_;
  Logger* logger_;
  HTreeBuilder* builder_;
  unsigned numViolatingSinks_ = 0;
  unsigned numInsertedBuffers_ = 0;
  double avgSourceSinkDist_ = 0.0;
  double bufDistRatio_ = 0.0;
  int bufIndex = 1;
  std::unordered_map<std::string, int> sinkDistMap_;
};

}  // namespace cts
