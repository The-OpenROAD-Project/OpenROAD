////////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2018, Iowa State University All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its contributors
// may be used to endorse or promote products derived from this software
// without specific prior written permission.
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
////////////////////////////////////////////////////////////////////////////////

#include <vector>

#include "SteinerTreeBuilder.h"

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

#if FLUTE_D <= 7
#define MGROUP 5040 / 4  // Max. # of groups, 7! = 5040
#define MPOWV 15         // Max. # of POWVs per group
#elif FLUTE_D == 8
#define MGROUP 40320 / 4  // Max. # of groups, 8! = 40320
#define MPOWV 33          // Max. # of POWVs per group
#elif FLUTE_D == 9
#define MGROUP 362880 / 4  // Max. # of groups, 9! = 362880
#define MPOWV 79           // Max. # of POWVs per group
#endif

class Flute
{
 public:
  struct csoln
  {
    unsigned char parent;
    unsigned char seg[11];  // Add: 0..i, Sub: j..10; seg[i+1]=seg[j-1]=0
    unsigned char rowcol[FLUTE_D - 2];  // row = rowcol[]/16, col = rowcol[]%16,
    unsigned char neighbor[2 * FLUTE_D - 2];
  };
  using LUT_TYPE = struct csoln***;
  using NUMSOLN_TYPE = int**;

 public:
  Flute(){

  };
  ~Flute() { deleteLUT(); }

  // User-Callable Functions
  // Delete LUT tables for exit so they are not leaked.
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
  Tree flute(const std::vector<int>& x, const std::vector<int>& y, int acc);
  int wirelength(Tree t);
  void plottree(Tree t);
  void write_svg(Tree t, const char* filename);

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
  NUMSOLN_TYPE numsoln;
  // LUTs are initialized to this order at startup.
  const int lut_initial_d = 8;
  int lut_valid_d = 0;

  const int numgrp[10] = {0, 0, 0, 0, 6, 30, 180, 1260, 10080, 90720};
};

}  // namespace flt

}  // namespace stt
