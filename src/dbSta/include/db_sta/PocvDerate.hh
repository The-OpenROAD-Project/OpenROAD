// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Parametric statistical on-chip-variation (POCV / LVF) derate, first slice.
//
// This is an OpenROAD-side (dbSta) feature that re-uses public OpenSTA APIs to
// recompute a STATISTICAL (root-sum-square / quadrature) setup slack on
// already-found critical paths. It does NOT modify the OpenSTA forward search:
// parametric-POCV variation combines in quadrature, which does not compose with
// the additive arrival sum the search propagates, so a per-arc propagation hook
// (like AOCV) is not the right place for it. See POCV_INVESTIGATION.md for the
// rationale, the quadrature-vs-linear math, and the documented scope.

#pragma once

#include <string>

#include "sta/Search.hh"  // OpenROAD-fork: POCV -- Search::PocvSigma state holder

namespace sta {

class Sta;
class PathEnd;

// One row of the POCV slack report for a single path end.
struct PocvPathResult
{
  std::string endpoint;
  int logic_depth = 0;      // number of combinational data-path stages
  float flat_slack = 0.0f;  // slack as graded by flat OCV (baseline)
  float pocv_slack = 0.0f;  // statistical (RSS) slack
  float flat_sigma = 0.0f;  // linear (fully-correlated) path sigma: k*sum(d_i)
  float rss_sigma = 0.0f;   // quadrature path sigma:        k*sqrt(sum(d_i^2))
  float n_sigma = 0.0f;     // sign-off sigma multiple used
};

// Recompute the parametric-POCV statistical setup slack for one path end.
//
// For each combinational data-path stage i with raw (un-derated) delay d_i and
// per-stage fractional sigma k, the path variation combines in QUADRATURE:
//     sigma_rss = k * sqrt( sum_i d_i^2 )
// versus the fully-correlated linear sigma a flat derate implies:
//     sigma_lin = k * sum_i d_i
// The reported POCV slack replaces the linear pessimism with N_sigma*sigma_rss:
//     pocv_slack = flat_slack + N_sigma*(sigma_lin - sigma_rss)
// Because sigma_rss <= sigma_lin (equal only for a single stage), POCV slack is
// >= flat slack, and the gap GROWS with depth as sqrt(N) vs N. When the sigma
// state is inactive (default), pocv_slack == flat_slack exactly.
PocvPathResult pocvAdjustPathEnd(Sta* sta,
                                 PathEnd* path_end,
                                 const Search::PocvSigma& sigma);

}  // namespace sta
