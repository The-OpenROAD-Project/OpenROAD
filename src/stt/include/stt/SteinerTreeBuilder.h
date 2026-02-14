// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <map>
#include <memory>
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
}  // namespace odb

namespace gui {
class Gui;
}

namespace stt {

namespace flt {
class Flute;
}

struct Branch
{
  int x, y;  // starting point of the branch
  int n;     // index of neighbor
};

struct Tree
{
  int deg;                     // degree
  int length;                  // total wirelength
  std::vector<Branch> branch;  // array of tree branches

  void printTree(utl::Logger* logger) const;
  int branchCount() const { return branch.size(); }
};

class SteinerTreeBuilder
{
 public:
  SteinerTreeBuilder(odb::dbDatabase* db, utl::Logger* logger);
  ~SteinerTreeBuilder();

  Tree makeSteinerTree(const std::vector<int>& x,
                       const std::vector<int>& y,
                       int drvr_index,
                       float alpha);
  Tree makeSteinerTree(const std::vector<int>& x,
                       const std::vector<int>& y,
                       int drvr_index);
  Tree makeSteinerTree(odb::dbNet* net,
                       const std::vector<int>& x,
                       const std::vector<int>& y,
                       int drvr_index);
  // API only for FastRoute, that requires the use of flutes in its
  // internal flute implementation
  Tree makeSteinerTree(const std::vector<int>& x,
                       const std::vector<int>& y,
                       const std::vector<int>& s,
                       int acc);

  bool checkTree(const Tree& tree) const;
  float getAlpha() const { return alpha_; }
  void setAlpha(float alpha);
  float getAlpha(const odb::dbNet* net) const;
  void setNetAlpha(const odb::dbNet* net, float alpha);
  void setMinFanoutAlpha(int min_fanout, float alpha);
  void setMinHPWLAlpha(int min_hpwl, float alpha);

  Tree flute(const std::vector<int>& x, const std::vector<int>& y, int acc);
  int wirelength(const Tree& t);
  void plottree(const Tree& t);
  Tree flutes(const std::vector<int>& xs,
              const std::vector<int>& ys,
              const std::vector<int>& s,
              int acc);

 private:
  int computeHPWL(odb::dbNet* net);

  static constexpr int flute_accuracy = 3;
  float alpha_;
  std::map<const odb::dbNet*, float> net_alpha_map_;
  std::pair<int, float> min_fanout_alpha_;
  std::pair<int, float> min_hpwl_alpha_;

  utl::Logger* logger_;
  odb::dbDatabase* db_;
  std::unique_ptr<flt::Flute> flute_;
};

// Used by regressions.
void reportSteinerTree(const Tree& tree,
                       int drvr_x,
                       int drvr_y,
                       utl::Logger* logger);
void reportSteinerTree(const stt::Tree& tree, utl::Logger* logger);

}  // namespace stt
