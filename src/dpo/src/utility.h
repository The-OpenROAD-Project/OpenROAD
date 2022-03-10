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

////////////////////////////////////////////////////////////////////////////////
// File: utility.h
////////////////////////////////////////////////////////////////////////////////

#pragma once

////////////////////////////////////////////////////////////////////////////////
// Includes.
////////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <iostream>
#include <vector>
#include <sys/time.h>
#include <unistd.h>
#include <boost/random/mersenne_twister.hpp>


////////////////////////////////////////////////////////////////////////////////
// Forward declarations.
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Defines.
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Typedefs.
////////////////////////////////////////////////////////////////////////////////
typedef boost::mt19937 Placer_RNG;

namespace dpo {
class Network;
class Node;
class Edge;
class Pin;
class Architecture;

////////////////////////////////////////////////////////////////////////////////
// Classes.
////////////////////////////////////////////////////////////////////////////////
class Utility {
 public:
  template <class RandomAccessIter>
  static void random_shuffle(RandomAccessIter first,
                             RandomAccessIter last, Placer_RNG* rng) {
    // This function implements the random_shuffle code from the STL, but
    // uses our Boost-based random number generator to get much better
    // random permutations.

    if (first == last) {
      return;
    }
    for (RandomAccessIter i = first + 1; i != last; ++i) {
      long randnum = ((*rng)()) % (i - first + 1);
      std::iter_swap(i, first + randnum);
    }
  }

  struct compare_blockages {
    bool operator()(std::pair<double, double> i1,
                    std::pair<double, double> i2) const {
      if (i1.first == i2.first) {
        return i1.second < i2.second;
      }
      return i1.first < i2.first;
    }
  };

 public:
  static double disp_l1(Network* nw, double& tot, double& max, double& avg);

  static double hpwl(Network* nw);
  static double hpwl(Network* nw, double& hpwlx, double& hpwly);
  static double hpwl(Network* nw, Edge* ed);
  static double hpwl(Network* nw, Edge*, double& hpwlx, double& hpwly);

  static unsigned count_num_ones(unsigned x) {
    x -= ((x >> 1) & 0x55555555);
    x = (((x >> 2) & 0x33333333) + (x & 0x33333333));
    x = (((x >> 4) + x) & 0x0f0f0f0f);
    x += (x >> 8);
    x += (x >> 16);
    return (x & 0x0000003f);
  }

  static void get_segments_from_blockages(
      Network* network, Architecture* arch,
      std::vector<std::vector<std::pair<double, double> > >& blockages,
      std::vector<std::vector<std::pair<double, double> > >& intervals);

  static double compute_overlap(double xmin1, double xmax1, double ymin1,
                                double ymax1, double xmin2, double xmax2,
                                double ymin2, double ymax2);
};

}  // namespace dpo
