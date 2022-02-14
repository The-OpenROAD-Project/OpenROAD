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

#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include <boost/format.hpp>
#include <boost/tokenizer.hpp>
#include <cmath>
#include <iostream>
#include <stack>
#include <utility>
#include "utl/Logger.h"
#include "plotgnu.h"
#include "utility.h"

// Detailed management of segments.
#include "detailed_manager.h"
#include "detailed_segment.h"
// Detailed placement algorithms.
#include "detailed.h"
#include "detailed_global.h"
#include "detailed_mis.h"
#include "detailed_random.h"
#include "detailed_reorder.h"
#include "detailed_vertical.h"
#include "detailed_orient.h"

using utl::DPO;

namespace dpo {

////////////////////////////////////////////////////////////////////////////////
// Defines.
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Detailed::improve:
////////////////////////////////////////////////////////////////////////////////
bool Detailed::improve(DetailedMgr& mgr)
// bool Detailed::improve( Architecture* arch, Network* network, RoutingParams*
// rt )
{
  m_mgr = &mgr;

  m_arch = mgr.getArchitecture();
  m_network = mgr.getNetwork();
  m_rt = mgr.getRoutingParams();  // Can be NULL.



  // Parse the script string and run each command.
  boost::char_separator<char> separators(" \r\t\n", ";");
  boost::tokenizer<boost::char_separator<char> > tokens(m_params.m_script,
                                                        separators);
  std::vector<std::string> args;
  for (boost::tokenizer<boost::char_separator<char> >::iterator it =
           tokens.begin();
       it != tokens.end(); it++) {
    std::string temp = *it;
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
    DetailedOrient orienter(m_arch, m_network, m_rt);
    orienter.run(m_mgr, "orient -f");
  }

  // Different checks which are useful for debugging.
  mgr.checkRegionAssignment();  
  mgr.checkRowAlignment();   
  mgr.checkSiteAlignment();  
  mgr.checkOverlapInSegments(); 
  mgr.checkEdgeSpacingInSegments(); 

  return true;
}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
void Detailed::doDetailedCommand(std::vector<std::string>& args) {
  if (args.size() == 0) {
    return;
  }

  // Removed some checks here.  Just check after.

  // The first argument is always the command.  XXX: Not implemented, but
  // include some samples...

  // Print something about what command will run.
  std::string command = "";
  if (strcmp(args[0].c_str(), "mis") == 0) {
    command = "independent set matching";
  } else if (strcmp(args[0].c_str(), "gs") == 0) {
    command = "global swaps";
  } else if (strcmp(args[0].c_str(), "vs") == 0) {
    command = "vertical swaps";
  } else if (strcmp(args[0].c_str(), "ro") == 0) {
    command = "reordering";
  } else if (strcmp(args[0].c_str(), "default") == 0) {
    command = "random improvement";
  } else {
    //command = "unknown command";
    return;
  }
  m_mgr->getLogger()->info(DPO, 303, "Running algorithm for {:s}.", command);

  // Comment out some algos I haven't confirmed as working.
  if (strcmp(args[0].c_str(), "mis") == 0) {
    DetailedMis mis(m_arch, m_network, m_rt);
    mis.run(m_mgr, args);
  } else if (strcmp(args[0].c_str(), "gs") == 0) {
    DetailedGlobalSwap gs(m_arch, m_network, m_rt);
    gs.run(m_mgr, args);
  } else if (strcmp(args[0].c_str(), "vs") == 0) {
    DetailedVerticalSwap vs(m_arch, m_network, m_rt);
    vs.run(m_mgr, args);
  //} else if (strcmp(args[0].c_str(), "interleave") == 0) {
  //  DetailedInterleave interleave(m_arch, m_network, m_rt);
  //  interleave.run(m_mgr, args);
  } else if (strcmp(args[0].c_str(), "ro") == 0) {
    DetailedReorderer ro(m_arch, m_network, m_rt);
    ro.run(m_mgr, args);
  } else if (strcmp(args[0].c_str(), "orient") == 0) {
    DetailedOrient orienter(m_arch, m_network, m_rt);
    orienter.run(m_mgr, args);
  } else if (strcmp(args[0].c_str(), "default") == 0) {
    DetailedRandom random(m_arch, m_network, m_rt);
    random.run(m_mgr, args);
  } else {
    return;
  }

  // Different checks which are useful for debugging.
  //m_mgr->checkRegionAssignment();  
  //m_mgr->checkRowAlignment();   
  //m_mgr->checkSiteAlignment();  
  //m_mgr->checkOverlapInSegments(); 
  //m_mgr->checkEdgeSpacingInSegments(); 
}

}  // namespace dpo
