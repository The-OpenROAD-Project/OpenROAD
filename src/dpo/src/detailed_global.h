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

#include <deque>
#include <vector>
#include "architecture.h"
#include "detailed_generator.h"
#include "network.h"
#include "rectangle.h"
#include "router.h"

namespace dpo {

class DetailedSeg;
class DetailedMgr;

// CLASSES ===================================================================
class DetailedGlobalSwap : public DetailedGenerator {
 public:
  DetailedGlobalSwap(Architecture* arch, Network* network, RoutingParams* rt);
  DetailedGlobalSwap();
  virtual ~DetailedGlobalSwap();

  // Interfaces for scripting.
  void run(DetailedMgr* mgrPtr, std::string command);
  void run(DetailedMgr* mgrPtr, std::vector<std::string>& args);

  // Interface for move generation.
  virtual bool generate(DetailedMgr* mgr, std::vector<Node*>& candiates);
  virtual void stats();
  virtual void init(DetailedMgr* mgr);

 protected:

  void globalSwap();  // tries to avoid overlap.
  bool calculateEdgeBB(Edge* ed, Node* nd, Rectangle& bbox);
  bool getRange(Node*, Rectangle&);
  double delta(Node* ndi, double new_x, double new_y);
  double delta(Node* ndi, Node* ndj);

  bool generate(Node* ndi);

  // Standard stuff.
  DetailedMgr* m_mgr;
  Architecture* m_arch;
  Network* m_network;
  RoutingParams* m_rt;

  // Other.
  int m_skipNetsLargerThanThis;
  std::vector<int> m_edgeMask;
  int m_traversal;

  std::vector<double> m_xpts;
  std::vector<double> m_ypts;

  // For use as a move generator.
  int m_attempts;
  int m_moves;
  int m_swaps;
};

}  // namespace dpo
