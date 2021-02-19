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

#include "timingBase.h"

#include <algorithm>
#include <cmath>

#include "nesterovBase.h"
#include "placerBase.h"
#include "rsz/Resizer.hh"
#include "sta/Fuzzy.hh"
#include "utility/Logger.h"

namespace gpl {

using utl::GPL;

// TimingBase
TimingBase::TimingBase() 
  : rs_(nullptr),
    nb_(nullptr),
    log_(nullptr),
    net_weight_max_(4.0)
{
}

TimingBase::TimingBase(
    std::shared_ptr<NesterovBase> nb, 
    rsz::Resizer* rs,
    utl::Logger* log)
  : TimingBase() {
    rs_ = rs;
    nb_ = nb;
    log_ = log;

    addTimingUpdateIter(79);
    addTimingUpdateIter(64);
    addTimingUpdateIter(49);
    addTimingUpdateIter(29);
    addTimingUpdateIter(21);
    addTimingUpdateIter(15);

    initTimingIterChk();
  }

void
TimingBase::initTimingIterChk() {
  timingIterChk_.clear();
  timingIterChk_.resize(timingUpdateIter_.size(), false);
}

void
TimingBase::addTimingUpdateIter(int overflow) {
  std::vector<int>::iterator it 
    = std::find(timingUpdateIter_.begin(), 
        timingUpdateIter_.end(), 
        overflow);

  // only push overflow when the overflow is not in vector.
  if( it == timingUpdateIter_.end() ) {
    timingUpdateIter_.push_back(overflow);
  }

  // do sort in reverse order
  std::sort(timingUpdateIter_.begin(), 
      timingUpdateIter_.end(),
      std::greater <int> ());
}

void
TimingBase::deleteTimingUpdateIter(int overflow) {
  std::vector<int>::iterator it 
    = std::find(timingUpdateIter_.begin(), 
        timingUpdateIter_.end(), 
        overflow);
  // only erase overflow when the overflow is in vector.
  if( it != timingUpdateIter_.end() ) {
    timingUpdateIter_.erase(it);
  }
}

void
TimingBase::clearTimingUpdateIter() {
  timingUpdateIter_.clear();
}

size_t
TimingBase::getTimingUpdateIterSize() const {
  return timingUpdateIter_.size();
}

bool
TimingBase::isTimingUpdateIter(float overflow) {
  int intOverflow = std::round(overflow * 100);
  // exception case handling
  if ( timingUpdateIter_.empty()
       || intOverflow > timingUpdateIter_[0] ) { 
    return false;
  } 

  bool needTdRun = false;
  for(int i=0; i<timingUpdateIter_.size(); i++) {
    if( timingUpdateIter_[i] > intOverflow ) {
      if( !timingIterChk_[i] ) {
        timingIterChk_[i] = true;
        needTdRun = true; 
      }
      continue;
    }
    return needTdRun;
  }
  return needTdRun;
}

bool
TimingBase::updateGNetWeights(float overflow) {
  rs_->findResizeSlacks();

  // get Top 10% worst resize nets
  sta::NetSeq &worst_slack_nets = rs_->resizeWorstSlackNets();

  if( worst_slack_nets.empty()) {
    log_->warn(GPL, 114, "No net slacks found. Timing-driven mode disabled.");
    return false;
  }
  
  // min/max slack for worst 10% nets
  sta::Slack slack_min = rs_->resizeNetSlack(worst_slack_nets[0]);
  sta::Slack slack_max = rs_->resizeNetSlack(worst_slack_nets[worst_slack_nets.size()-1]);

  log_->info(GPL, 100, "worst slack {:.3g}", slack_min);

  if (sta::fuzzyInf(slack_min)) {
    log_->warn(GPL, 102, "No slacks found. Timing-driven mode disabled.");
    return false;
  }

  int weighted_net_count = 0;
  for(auto& gNet : nb_->gNets()) {
    // default weight
    gNet->setTimingWeight(1.0);
    if (gNet->gPins().size() > 1) {
      float net_slack = rs_->resizeNetSlack(gNet->net()->dbNet());
      if (net_slack < slack_max) {
        // weight(min_slack) = net_weight_max_
        // weight(max_slack) = 1
        float weight = 1 + (net_weight_max_ - 1)
          * (slack_max - net_slack) / (slack_max - slack_min);
        gNet->setTimingWeight(weight);
        weighted_net_count++;
      }
    }
  }

  log_->info(GPL, 103, "Weighted {} nets.", weighted_net_count);
  return true;
}

}
