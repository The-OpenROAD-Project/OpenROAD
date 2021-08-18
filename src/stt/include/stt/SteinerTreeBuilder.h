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

#include <map>
#include <string>
#include <utility>
#include <vector>
#include "utl/Logger.h"

namespace ord {
class OpenRoad;
}

namespace odb {
class dbDatabase;
class dbNet;
}

namespace gui {
class Gui;
}

namespace stt {

using utl::Logger;

typedef int DTYPE;

struct Branch {
  DTYPE x, y;  // starting point of the branch
  int n;       // index of neighbor
};

struct Tree {
  int deg;         // degree
  DTYPE length;    // total wirelength
  std::vector<Branch> branch;  // array of tree branches

  void printTree(utl::Logger* logger);
  int branchCount() const { return deg * 2 - 2; }
};

class SteinerTreeBuilder
{
 public:
  SteinerTreeBuilder();
  ~SteinerTreeBuilder() = default;

  void init(odb::dbDatabase* db, Logger* logger);

  Tree makeSteinerTree(std::vector<int>& x,
                       std::vector<int>& y,
                       int drvr_index,
                       float alpha);
  Tree makeSteinerTree(std::vector<int>& x,
                       std::vector<int>& y,
                       int drvr_index);
  Tree makeSteinerTree(odb::dbNet* net,
                       std::vector<int>& x,
                       std::vector<int>& y,
                       int drvr_index);
  // API only for FastRoute, that requires the use of flutes in its
  // internal flute implementation
  Tree makeSteinerTree(const std::vector<int>& x,
                       const std::vector<int>& y,
                       const std::vector<int>& s,
                       int acc);
  float getAlpha() const { return alpha_; }
  void setAlpha(float alpha);
  float getAlpha(const odb::dbNet* net) const;
  void setNetAlpha(const odb::dbNet* net, float alpha);
  void setMinFanoutAlpha(int min_fanout, float alpha);
  void setMinHPWLAlpha(int min_hpwl, float alpha);

 private:
  int computeHPWL(odb::dbNet* net);
  bool checkTree(const Tree& tree) const;

  const int flute_accuracy = 3;
  float alpha_;
  std::map<const odb::dbNet*, float> net_alpha_map_;
  std::pair<int, float> min_fanout_alpha_;
  std::pair<int, float> min_hpwl_alpha_;

  Logger* logger_;
  odb::dbDatabase* db_;
};

// Used by regressions.
void
reportSteinerTree(Tree &tree,
                  int drvr_x,
                  int drvr_y,
                  Logger *logger);

void
highlightSteinerTree(Tree &tree,
                     gui::Gui *gui);

}  // namespace stt
