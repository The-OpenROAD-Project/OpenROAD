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

#pragma once

// Description:
// - An objective function to help with computation of change in wirelength
//   if doing some sort of moves (e.g., single, swap, sets, etc.).

////////////////////////////////////////////////////////////////////////////////
// Includes.
////////////////////////////////////////////////////////////////////////////////
#include <deque>
#include <vector>
#include "architecture.h"
#include "detailed_objective.h"
#include "network.h"
#include "router.h"

namespace dpo {

////////////////////////////////////////////////////////////////////////////////
// Defines.
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Forward declarations.
////////////////////////////////////////////////////////////////////////////////
class DetailedOrient;
class DetailedMgr;

////////////////////////////////////////////////////////////////////////////////
// Classes.
////////////////////////////////////////////////////////////////////////////////

class DetailedHPWL : public DetailedObjective {
  // For WL objective.
 public:
  DetailedHPWL(Architecture* arch, Network* network, RoutingParams* rt);
  virtual ~DetailedHPWL();

  void init();
  double curr();
  double delta(int n, std::vector<Node*>& nodes, 
               std::vector<int>& curLeft, std::vector<int>& curBottom, 
               std::vector<unsigned>& curOri,
               std::vector<int>& newLeft, std::vector<int>& newBottom,
               std::vector<unsigned>& newOri);

  void getCandidates(std::vector<Node*>& candidates);

  // Other.
  void init(DetailedMgr* mgrPtr, DetailedOrient* orientPtr);
  double delta(Node* ndi, double new_x, double new_y);
  double delta(Node* ndi, Node* ndj);
  double delta(Node* ndi, double target_xi, double target_yi, Node* ndj,
               double target_xj, double target_yj);

  ////////////////////////////////////////////////////////////////////////////////

 protected:
  Architecture* m_arch;
  Network* m_network;
  RoutingParams* m_rt;

  DetailedMgr* m_mgrPtr;
  DetailedOrient* m_orientPtr;

  // Other.
  int m_skipNetsLargerThanThis;
  int m_traversal;
  std::vector<int> m_edgeMask;
};

}  // namespace dpo
