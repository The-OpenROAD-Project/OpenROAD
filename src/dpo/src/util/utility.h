// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#pragma once

#include <boost/random/mersenne_twister.hpp>

namespace dpl {
class Edge;
}

namespace dpo {
using Placer_RNG = boost::mt19937;

class Network;
using dpl::Edge;

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

  static double hpwl(const Network* nw);
  static double hpwl(const Network* nw, double& hpwlx, double& hpwly);
  static double hpwl(const Edge* ed);
  static double hpwl(const Edge*, double& hpwlx, double& hpwly);
};

}  // namespace dpo
