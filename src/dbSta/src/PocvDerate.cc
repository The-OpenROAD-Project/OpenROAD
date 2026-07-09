// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "db_sta/PocvDerate.hh"

#include <cmath>

#include "sta/Delay.hh"
#include "sta/Graph.hh"
#include "sta/MinMax.hh"
#include "sta/Network.hh"
#include "sta/Path.hh"
#include "sta/PathEnd.hh"
#include "sta/PathExpanded.hh"
#include "sta/Search.hh"
#include "sta/Sta.hh"
#include "sta/TimingArc.hh"
#include "sta/TimingRole.hh"

namespace sta {

PocvPathResult pocvAdjustPathEnd(Sta* sta,
                                 PathEnd* path_end,
                                 const Search::PocvSigma& sigma)
{
  PocvPathResult result;
  const Path* end_path = path_end->path();
  result.endpoint = sta->network()->pathName(end_path->pin(sta));
  result.flat_slack = delayAsFloat(path_end->slack(sta));
  result.pocv_slack = result.flat_slack;
  result.n_sigma = sigma.n_sigma;

  Graph* graph = sta->graph();
  PathExpanded expanded(end_path, sta);
  const size_t size = expanded.size();
  // path(0) is the path root (startpoint side); path(size-1) is the endpoint.

  // Walk combinational data-path stages, accumulating both the linear sum of
  // raw stage delays and the sum of their squares (for the quadrature sigma).
  int logic_depth = 0;
  double sum_delay = 0.0;     // sum_i d_i        -> linear (correlated) sigma
  double sum_delay_sq = 0.0;  // sum_i d_i^2      -> RSS (independent) sigma
  for (size_t i = 1; i < size; i++) {
    const Path* p = expanded.path(i);
    const Path* prev = p->prevPath();
    const TimingArc* arc = p->prevArc(sta);
    const Edge* edge = p->prevEdge(sta);
    if (arc == nullptr || edge == nullptr || prev == nullptr) {
      continue;
    }
    // Only the combinational logic stages carry per-stage gate variation; clock
    // / reg / latch / check arcs keep their flat treatment (matches AOCV
    // slice).
    if (arc->role() != TimingRole::combinational()) {
      continue;
    }
    const DcalcAPIndex dcalc_ap = p->dcalcAnalysisPtIndex(sta);
    // Raw (un-derated) arc delay: the nominal stage delay d_i.
    const double d = delayAsFloat(graph->arcDelay(edge, arc, dcalc_ap));
    if (d <= 0.0) {
      continue;  // negative/zero arcs contribute no meaningful variation
    }
    logic_depth++;
    sum_delay += d;
    sum_delay_sq += d * d;
  }
  result.logic_depth = logic_depth;

  // Per-stage fractional sigma k. Linear (fully-correlated) path sigma is the
  // straight sum; quadrature (independent) path sigma is the root-sum-square.
  const double k = sigma.per_stage;
  result.flat_sigma = static_cast<float>(k * sum_delay);
  result.rss_sigma = static_cast<float>(k * std::sqrt(sum_delay_sq));

  // Inactive (default) => POCV slack == flat slack exactly (baseline-safe).
  if (!sigma.active() || logic_depth == 0) {
    return result;
  }

  // The flat grading effectively charged N_sigma * sigma_lin of pessimism to
  // the data path. Parametric POCV charges only N_sigma * sigma_rss. Since
  // sigma_rss <= sigma_lin, recovering the difference improves setup slack:
  //   pocv_slack = flat_slack + N_sigma * (sigma_lin - sigma_rss)
  // For min/hold paths the early variation subtracts from arrival, so reduced
  // pessimism likewise relaxes the (required - arrival) hold check in the same
  // slack-improving direction; the magnitude is identical, hence one formula.
  const double recovered
      = sigma.n_sigma * (result.flat_sigma - result.rss_sigma);
  result.pocv_slack = static_cast<float>(result.flat_slack + recovered);
  return result;
}

}  // namespace sta
