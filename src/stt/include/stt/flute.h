// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include <memory>
#include <vector>

#include "boost/multi_array.hpp"
#include "stt/SteinerTreeBuilder.h"

#pragma once

namespace stt::flt {

class Flute
{
 public:
  Flute() = default;
  ~Flute() { deleteLUT(); }

  Tree flute(const std::vector<int>& x, const std::vector<int>& y, int acc);
  int wirelength(const Tree& t);
  void plottree(const Tree& t);
  Tree flutes(const std::vector<int>& xs,
              const std::vector<int>& ys,
              const std::vector<int>& s,
              int acc);
  int flute_wl(int d,
               const std::vector<int>& x,
               const std::vector<int>& y,
               int acc);

 private:
  struct Csoln;

  using LutType = boost::multi_array<std::shared_ptr<Csoln[]>, 2>*;
  using NumSoln = int**;

  void readLUT();
  void makeLUT(LutType& lut, NumSoln& numsoln);
  void initLUT(int to_d, LutType lut, NumSoln numsoln);
  void ensureLUT(int d);
  void deleteLUT();
  void deleteLUT(LutType& lut, NumSoln& numsoln);

  Tree d_merge_tree(const Tree& t1, const Tree& t2) const;
  Tree h_merge_tree(const Tree& t1, const Tree& t2, const std::vector<int>& s);
  Tree v_merge_tree(const Tree& t1, const Tree& t2) const;
  void local_refinement(int deg, Tree* tp, int p);

  int flutes_wl_low_degree(int d,
                           const std::vector<int>& xs,
                           const std::vector<int>& ys,
                           const std::vector<int>& s);
  int flutes_wl_medium_degree(int d,
                              const std::vector<int>& xs,
                              const std::vector<int>& ys,
                              const std::vector<int>& s,
                              int acc);
  Tree flutes_low_degree(int d,
                         const std::vector<int>& xs,
                         const std::vector<int>& ys,
                         const std::vector<int>& s);
  Tree flutes_medium_degree(int d,
                            const std::vector<int>& xs,
                            const std::vector<int>& ys,
                            const std::vector<int>& s,
                            int acc);

  int flutes_wl_all_degree(int d,
                           const std::vector<int>& xs,
                           const std::vector<int>& ys,
                           const std::vector<int>& s,
                           int acc);

  Tree flutes_all_degree(int d,
                         const std::vector<int>& xs,
                         const std::vector<int>& ys,
                         const std::vector<int>& s,
                         int acc);

  // Dynamically allocate LUTs.
  LutType lut_ = nullptr;
  NumSoln numsoln_ = nullptr;
  int lut_valid_d_ = 0;

  // LUTs are initialized to this order at startup.
  static constexpr int kLutInitialDegree = 8;
  static constexpr int kNumGroup[10]
      = {0, 0, 0, 0, 6, 30, 180, 1260, 10080, 90720};
};

}  // namespace stt::flt
