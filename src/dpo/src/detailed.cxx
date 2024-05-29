///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2021, Andrew Kennings
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

#include <algorithm>
#include <boost/format.hpp>
#include <boost/tokenizer.hpp>
#include <cmath>
#include <iostream>
#include <stack>
#include <utility>

#include "utility.h"
#include "utl/Logger.h"

// Detailed management of segments.
#include "detailed_manager.h"
#include "detailed_segment.h"
// Detailed placement algorithms.
#include "detailed.h"
#include "detailed_global.h"
#include "detailed_mis.h"
#include "detailed_orient.h"
#include "detailed_random.h"
#include "detailed_reorder.h"
#include "detailed_vertical.h"

using utl::DPO;

namespace dpo {

////////////////////////////////////////////////////////////////////////////////
// Defines.
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Detailed::improve:
////////////////////////////////////////////////////////////////////////////////
bool Detailed::improve(DetailedMgr& mgr)
{
  mgr_ = &mgr;

  arch_ = mgr.getArchitecture();
  network_ = mgr.getNetwork();
  rt_ = mgr.getRoutingParams();  // Can be NULL.

  // Parse the script string and run each command.
  boost::char_separator<char> separators(" \r\t\n", ";");
  boost::tokenizer<boost::char_separator<char>> tokens(params_.script_,
                                                       separators);
  std::vector<std::string> args;
  for (auto temp : tokens) {
    if (temp.back() == ';') {
      while (!temp.empty() && temp.back() == ';') {
        temp.resize(temp.size() - 1);
      }
      if (!temp.empty()) {
        args.push_back(temp);
      }
      // Command ended by a semi-colon.
      doDetailedCommand(args);
      args.clear();
    } else {
      args.push_back(temp);
    }
  }
  // Last command; possible if no ending semi-colon.
  doDetailedCommand(args);

  // Note: If cell orientation was not the last script
  // command run, then we should/need to perform
  // orientation to ensure the cells are properly
  // oriented for their respective row assignments.
  // We do not need to do flipping though.
  {
    DetailedOrient orienter(arch_, network_);
    orienter.run(mgr_, "orient -f");
  }

  // Different checks which are useful for debugging.
  mgr.checkRegionAssignment();
  mgr.checkRowAlignment();
  mgr.checkSiteAlignment();
  mgr.checkOverlapInSegments();
  mgr.checkEdgeSpacingInSegments();

  if (mgr.getDisallowOneSiteGaps()) {
    std::vector<std::vector<int>> oneSiteViolations;
    int temp_move_limit = mgr.getMoveLimit();
    mgr.setMoveLimit(10000);
    mgr.getOneSiteGapViolationsPerSegment(oneSiteViolations, true);
    for (int i = 0; i < oneSiteViolations.size(); i++) {
      if (!oneSiteViolations[i].empty()) {
        std::string violating_node_ids = "[";
        for (int nodeId : oneSiteViolations[i]) {
          violating_node_ids += std::to_string(nodeId)
                                + ",]"[nodeId == oneSiteViolations[i].back()];
        }
        mgr_->getLogger()->warn(
            DPO,
            323,
            "One site gap violation in segment {:d} nodes: {}",
            i,
            violating_node_ids);
      }
    }
    mgr.setMoveLimit(temp_move_limit);
  }

  return true;
}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
void Detailed::doDetailedCommand(std::vector<std::string>& args)
{
  if (args.empty()) {
    return;
  }

  // The first argument is always the command.

  auto logger = mgr_->getLogger();

  // Print something about what command will run.
  std::string command;
  if (strcmp(args[0].c_str(), "mis") == 0) {
    command = "independent set matching";
  } else if (strcmp(args[0].c_str(), "gs") == 0) {
    command = "global swaps";
  } else if (strcmp(args[0].c_str(), "vs") == 0) {
    command = "vertical swaps";
  } else if (strcmp(args[0].c_str(), "ro") == 0) {
    command = "reordering";
  } else if (strcmp(args[0].c_str(), "orient") == 0) {
    command = "orienting";
  } else if (strcmp(args[0].c_str(), "default") == 0) {
    command = "random improvement";
  } else if (strcmp(args[0].c_str(), "disallow_one_site_gaps") == 0) {
    command = "disallow_one_site_gaps";
  } else {
    logger->error(DPO, 1, "Unknown algorithm {:s}.", args[0]);
  }
  logger->info(DPO, 303, "Running algorithm for {:s}.", command);

  if (strcmp(args[0].c_str(), "mis") == 0) {
    DetailedMis mis(arch_, network_, rt_);
    mis.run(mgr_, args);
  } else if (strcmp(args[0].c_str(), "gs") == 0) {
    DetailedGlobalSwap gs(arch_, network_, rt_);
    gs.run(mgr_, args);
  } else if (strcmp(args[0].c_str(), "vs") == 0) {
    DetailedVerticalSwap vs(arch_, network_, rt_);
    vs.run(mgr_, args);
  } else if (strcmp(args[0].c_str(), "ro") == 0) {
    DetailedReorderer ro(arch_, network_);
    ro.run(mgr_, args);
  } else if (strcmp(args[0].c_str(), "orient") == 0) {
    DetailedOrient orienter(arch_, network_);
    orienter.run(mgr_, args);
  } else if (strcmp(args[0].c_str(), "default") == 0) {
    DetailedRandom random(arch_, network_);
    random.run(mgr_, args);
  } else {
    return;
  }

  // Different checks which are useful for debugging.
  // mgr_->checkRegionAssignment();
  // mgr_->checkRowAlignment();
  // mgr_->checkSiteAlignment();
  // mgr_->checkOverlapInSegments();
  // mgr_->checkEdgeSpacingInSegments();
}

}  // namespace dpo
