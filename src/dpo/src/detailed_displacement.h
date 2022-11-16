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
////////////////////////////////////////////////////////////////////////////////
#include <vector>

#include "detailed_objective.h"

namespace dpo {

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
class DetailedOrient;
class DetailedMgr;

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
class DetailedDisplacement : public DetailedObjective
{
  // For WL objective.
 public:
  DetailedDisplacement(Architecture* arch);

  void init();
  double curr() override;
  double delta(int n,
               std::vector<Node*>& nodes,
               std::vector<int>& curLeft,
               std::vector<int>& curBottom,
               std::vector<unsigned>& curOri,
               std::vector<int>& newLeft,
               std::vector<int>& newBottom,
               std::vector<unsigned>& newOri) override;
  void getCandidates(std::vector<Node*>& candidates);

  // Other.
  void init(DetailedMgr* mgrPtr, DetailedOrient* orientPtr);
  double delta(Node* ndi, double new_x, double new_y);
  double delta(Node* ndi, Node* ndj);
  double delta(Node* ndi,
               double target_xi,
               double target_yi,
               Node* ndj,
               double target_xj,
               double target_yj);

 private:
  Architecture* arch_;

  DetailedMgr* mgrPtr_;
  DetailedOrient* orientPtr_;

  // Other.
  double singleRowHeight_;
  std::vector<double> tot_;
  std::vector<double> del_;
  std::vector<int> count_;
  int nSets_;
};

}  // namespace dpo
