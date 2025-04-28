// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#pragma once

#include <boost/random/mersenne_twister.hpp>

namespace dpl {
}

namespace dpl {
using Placer_RNG = boost::mt19937;

class Network;
class Edge;

class Utility
{
 public:
  template <class RandomAccessIter>
  static void random_shuffle(RandomAccessIter first,
                             RandomAccessIter last,
                             Placer_RNG* rng)
  {
    // This function implements the random_shuffle code from the STL, but
    // uses our Boost-based random number generator to get much better
    // random permutations.

    if (first == last) {
      return;
    }
    for (RandomAccessIter i = first + 1; i != last; ++i) {
      auto randnum = ((*rng)()) % (i - first + 1);
      std::iter_swap(i, first + randnum);
    }
  }

  static double disp_l1(Network* nw, double& tot, double& max, double& avg);

  static uint64_t hpwl(const Network* nw);
  static uint64_t hpwl(const Network* nw, uint64_t& hpwlx, uint64_t& hpwly);
  static uint64_t hpwl(const Edge* ed);
  static uint64_t hpwl(const Edge*, uint64_t& hpwlx, uint64_t& hpwly);
};

}  // namespace dpl
