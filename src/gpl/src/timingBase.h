///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2018-2020, The Regents of the University of California
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
// ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <cstddef>
#include <memory>
#include <vector>

#include "rsz/Resizer.hh"

namespace rsz {
class Resizer;
}

namespace utl {
class Logger;
}

namespace gpl {

class NesterovBaseCommon;
class GNet;

class TimingBase
{
 public:
  TimingBase();
  TimingBase(std::shared_ptr<NesterovBaseCommon> nbc,
             rsz::Resizer* rs,
             utl::Logger* log);

  // check whether overflow reached the timingOverflow
  bool isTimingNetWeightOverflow(float overflow);
  void addTimingNetWeightOverflow(int overflow);
  void setTimingNetWeightOverflows(std::vector<int>& overflows);
  void deleteTimingNetWeightOverflow(int overflow);
  void clearTimingNetWeightOverflow();
  size_t getTimingNetWeightOverflowSize() const;

  void setTimingNetWeightMax(float max);

  int repairDesignBufferCount() { return rs_->repairDesignBufferCount(); }

  // updateNetWeight.
  // True: successfully reweighted gnets
  // False: no slacks found
  bool executeTimingDriven(bool run_journal_restore);

 private:
  rsz::Resizer* rs_ = nullptr;
  utl::Logger* log_ = nullptr;
  std::shared_ptr<NesterovBaseCommon> nbc_;

  std::vector<int> timingNetWeightOverflow_;
  std::vector<int> timingOverflowChk_;
  float net_weight_max_ = 5;
  void initTimingOverflowChk();
};

}  // namespace gpl
