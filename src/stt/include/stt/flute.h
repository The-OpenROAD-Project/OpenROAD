// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include <vector>

#include "SteinerTreeBuilder.h"
#include "boost/multi_array.hpp"

#pragma once

namespace stt {

namespace flt {

using stt::Branch;
using stt::Tree;

/*****************************/
/*  User-Defined Parameters  */
/*****************************/
#define FLUTE_ROUTING 1  // 1 to construct routing, 0 to estimate WL only
#define FLUTE_LOCAL_REFINEMENT 1  // Suggestion: Set to 1 if ACCURACY >= 5
#define FLUTE_REMOVE_DUPLICATE_PIN \
  0  // Remove dup. pin for flute_wl() & flute()

#define FLUTE_POWVFILE "POWV9.dat"  // LUT for POWV (Wirelength Vector)
#define FLUTE_POSTFILE "POST9.dat"  // LUT for POST (Steiner Tree)
#define FLUTE_D 9  // LUT is used for d <= FLUTE_D, FLUTE_D <= 9

class Flute
{
 public:
  struct csoln;

 private:
  using LUT_TYPE = boost::multi_array<std::shared_ptr<struct csoln[]>, 2>*;
  using NUMSOLN_TYPE = int**;

 public:
  Flute() = default;
  ~Flute() { deleteLUT(); }

  // User-Callable Functions
  // Delete LUT tables for exit so they are not leaked.

  Tree flute(const std::vector<int>& x, const std::vector<int>& y, int acc);
  int wirelength(Tree t);
  void plottree(Tree t);
  inline Tree flutes(const std::vector<int>& xs,
                     const std::vector<int>& ys,
                     const std::vector<int>& s,
                     int acc)
  {
    int d = xs.size();
    if (FLUTE_REMOVE_DUPLICATE_PIN == 1) {
      return flutes_RDP(d, xs, ys, s, acc);
    }
    return flutes_ALLD(d, xs, ys, s, acc);
  }

 private:
  void readLUT();
  void makeLUT(LUT_TYPE& LUT, NUMSOLN_TYPE& numsoln);
  void initLUT(int to_d, LUT_TYPE LUT, NUMSOLN_TYPE numsoln);
  void ensureLUT(int d);
  void deleteLUT();
  void deleteLUT(LUT_TYPE& LUT, NUMSOLN_TYPE& numsoln);

  int flute_wl(int d,
               const std::vector<int>& x,
               const std::vector<int>& y,
               int acc);

  Tree dmergetree(Tree t1, Tree t2);
  Tree hmergetree(Tree t1, Tree t2, const std::vector<int>& s);
  Tree vmergetree(Tree t1, Tree t2);
  void local_refinement(int deg, Tree* tp, int p);

  // Other useful functions
  int flutes_wl_LD(int d,
                   const std::vector<int>& xs,
                   const std::vector<int>& ys,
                   const std::vector<int>& s);
  int flutes_wl_MD(int d,
                   const std::vector<int>& xs,
                   const std::vector<int>& ys,
                   const std::vector<int>& s,
                   int acc);
  int flutes_wl_RDP(int d,
                    std::vector<int> xs,
                    std::vector<int> ys,
                    std::vector<int> s,
                    int acc);
  Tree flutes_LD(int d,
                 const std::vector<int>& xs,
                 const std::vector<int>& ys,
                 const std::vector<int>& s);
  Tree flutes_MD(int d,
                 const std::vector<int>& xs,
                 const std::vector<int>& ys,
                 const std::vector<int>& s,
                 int acc);
  Tree flutes_RDP(int d,
                  std::vector<int> xs,
                  std::vector<int> ys,
                  std::vector<int> s,
                  int acc);

  inline int flutes_wl_LMD(int d,
                           const std::vector<int>& xs,
                           const std::vector<int>& ys,
                           const std::vector<int>& s,
                           int acc)
  {
    if (d <= FLUTE_D) {
      return flutes_wl_LD(d, xs, ys, s);
    }
    return flutes_wl_MD(d, xs, ys, s, acc);
  }

  inline int flutes_wl_ALLD(int d,
                            const std::vector<int>& xs,
                            const std::vector<int>& ys,
                            const std::vector<int>& s,
                            int acc)
  {
    return flutes_wl_LMD(d, xs, ys, s, acc);
  }

  inline int flutes_wl(int d,
                       const std::vector<int>& xs,
                       const std::vector<int>& ys,
                       const std::vector<int>& s,
                       int acc)
  {
    if (FLUTE_REMOVE_DUPLICATE_PIN == 1) {
      return flutes_wl_RDP(d, xs, ys, s, acc);
    }
    return flutes_wl_ALLD(d, xs, ys, s, acc);
  }

  inline Tree flutes_ALLD(int d,
                          const std::vector<int>& xs,
                          const std::vector<int>& ys,
                          const std::vector<int>& s,
                          int acc)
  {
    if (d <= FLUTE_D) {
      return flutes_LD(d, xs, ys, s);
    }
    return flutes_MD(d, xs, ys, s, acc);
  }

  inline Tree flutes_LMD(int d,
                         const std::vector<int>& xs,
                         const std::vector<int>& ys,
                         const std::vector<int>& s,
                         int acc)
  {
    if (d <= FLUTE_D) {
      return flutes_LD(d, xs, ys, s);
    }
    return flutes_MD(d, xs, ys, s, acc);
  }

 private:
  // Dynamically allocate LUTs.
  LUT_TYPE LUT = nullptr;
  NUMSOLN_TYPE numsoln = nullptr;
  // LUTs are initialized to this order at startup.
  const int lut_initial_d = 8;
  int lut_valid_d = 0;

  const int numgrp[10] = {0, 0, 0, 0, 6, 30, 180, 1260, 10080, 90720};
};

}  // namespace flt

}  // namespace stt
