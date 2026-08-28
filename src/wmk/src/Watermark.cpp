// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors
//
// Routing watermark implementation. See header for background.

#include "wmk/Watermark.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <string>
#include <utility>
#include <vector>

#include "HmacSha256.h"
#include "Wirelength.h"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "est/EstimateParasitics.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "sta/Clock.hh"
#include "sta/Delay.hh"
#include "sta/Liberty.hh"
#include "sta/MinMax.hh"
#include "sta/Scene.hh"
#include "sta/Transition.hh"
#include "utl/Logger.h"

namespace wmk {

using odb::dbBlock;
using odb::dbBoolProperty;
using odb::dbNet;

namespace {

// log(C(n, k)) = lgamma(n+1) - lgamma(k+1) - lgamma(n-k+1)
double logChoose(int n, int k)
{
  return std::lgamma(static_cast<double>(n) + 1.0)
         - std::lgamma(static_cast<double>(k) + 1.0)
         - std::lgamma(static_cast<double>(n - k) + 1.0);
}

// Pc = sum_{i=0..x} C(X, i) * p^(X-i) * (1-p)^i
// Computed in log-space then summed via log-sum-exp to avoid underflow
// for the small Pc values reported in the paper.
double binomialTail(int X, int x, double p)
{
  if (X <= 0) {
    return 1.0;
  }
  if (x < 0) {
    return 0.0;
  }
  if (x >= X) {
    return 1.0;
  }
  const double log_p = std::log(p);
  const double log_q = std::log(1.0 - p);
  // Collect log-terms, then log-sum-exp.
  std::vector<double> log_terms;
  log_terms.reserve(x + 1);
  for (int i = 0; i <= x; ++i) {
    const double lt = logChoose(X, i) + (X - i) * log_p + i * log_q;
    log_terms.push_back(lt);
  }
  const double max_lt = *std::ranges::max_element(log_terms);
  double acc = 0.0;
  for (double lt : log_terms) {
    acc += std::exp(lt - max_lt);
  }
  return std::exp(max_lt) * acc;
}

}  // namespace

Watermark::Watermark(odb::dbDatabase* db,
                     sta::dbSta* sta,
                     dpl::Opendp* opendp,
                     est::EstimateParasitics* estimate_parasitics,
                     utl::Logger* logger)
    : db_(db),
      sta_(sta),
      opendp_(opendp),
      estimate_parasitics_(estimate_parasitics),
      logger_(logger)
{
}

bool Watermark::clockSkewAvailable() const
{
  if (sta_ == nullptr) {
    return false;
  }
  sta::dbNetwork* network = sta_->getDbNetwork();
  return network != nullptr && network->defaultLibertyLibrary() != nullptr;
}

float Watermark::worstClockSkew() const
{
  if (sta_ == nullptr) {
    return 0.0f;
  }
  sta::dbNetwork* network = sta_->getDbNetwork();
  if (network == nullptr || network->defaultLibertyLibrary() == nullptr) {
    return 0.0f;
  }
  const float skew = sta_->findWorstClkSkew(
      sta::MinMax::max(), /* include_internal_latency */ false);
  return std::isfinite(skew) ? skew : 0.0f;
}

bool Watermark::driverHeadroomOk(odb::dbInst* inst,
                                 double slew_frac,
                                 double cap_frac) const
{
  if (!clockSkewAvailable() || inst == nullptr) {
    return true;
  }
  sta::dbNetwork* network = sta_->getDbNetwork();

  odb::dbITerm* out = nullptr;
  for (odb::dbITerm* iterm : inst->getITerms()) {
    if (iterm->getIoType() == odb::dbIoType::OUTPUT) {
      if (out != nullptr) {
        return true;  // more than one output: not a buffer we understand
      }
      out = iterm;
    }
  }
  if (out == nullptr || out->getNet() == nullptr) {
    return true;
  }
  sta::Pin* pin = network->dbToSta(out);
  if (pin == nullptr) {
    return true;
  }

  // The limits come from the library (and any constraint overriding it), so
  // what counts as too slow or too loaded is the technology's answer, not a
  // number chosen here.  A limit the library does not state cannot be
  // violated, so an absent one is not an objection.
  const sta::MinMax* max = sta::MinMax::max();
  const sta::RiseFall* rf = nullptr;
  const sta::Scene* scene = nullptr;

  sta_->checkSlewsPreamble();
  sta::Slew slew = 0.0f;
  float slew_limit = 0.0f;
  float slew_slack = 0.0f;
  sta_->checkSlew(
      pin, sta_->scenes(), max, true, slew, slew_limit, slew_slack, rf, scene);
  if (slew_limit > 0.0f
      && (slew_limit - slew) / slew_limit < static_cast<float>(slew_frac)) {
    return false;
  }

  sta_->checkCapacitancesPreamble(sta_->scenes());
  float cap = 0.0f;
  float cap_limit = 0.0f;
  float cap_slack = 0.0f;
  sta_->checkCapacitance(
      pin, sta_->scenes(), max, cap, cap_limit, cap_slack, rf, scene);
  return cap_limit <= 0.0f
         || cap <= static_cast<float>(1.0 - cap_frac) * cap_limit;
}

std::vector<int> Watermark::clockIndicesAt(odb::dbInst* inst) const
{
  std::vector<int> clocks;
  if (sta_ == nullptr || inst == nullptr) {
    return clocks;
  }
  sta::dbNetwork* network = sta_->getDbNetwork();
  if (network == nullptr || network->defaultLibertyLibrary() == nullptr) {
    return clocks;
  }

  odb::dbITerm* driver = nullptr;
  for (odb::dbITerm* iterm : inst->getITerms()) {
    if (iterm->getIoType() == odb::dbIoType::OUTPUT) {
      if (driver != nullptr) {
        return clocks;  // more than one output: not a buffer we understand
      }
      driver = iterm;
    }
  }
  if (driver == nullptr) {
    return clocks;
  }
  sta::Pin* pin = network->dbToSta(driver);
  if (pin == nullptr) {
    return clocks;
  }

  for (const sta::Clock* clock : sta_->clocks(pin, sta_->cmdMode())) {
    clocks.push_back(clock->index());
  }
  std::ranges::sort(clocks);
  return clocks;
}

void Watermark::reestimateNetParasitics(odb::dbNet* a, odb::dbNet* b) const
{
  if (estimate_parasitics_ == nullptr || sta_ == nullptr) {
    return;
  }
  sta::dbNetwork* network = sta_->getDbNetwork();
  if (network == nullptr || network->defaultLibertyLibrary() == nullptr) {
    return;
  }
  for (odb::dbNet* net : {a, b}) {
    if (net == nullptr) {
      continue;
    }
    if (sta::Net* sta_net = network->dbToSta(net)) {
      estimate_parasitics_->estimateWireParasitic(sta_net);
    }
  }
}

float Watermark::worstSlack(odb::dbInst* inst) const
{
  if (sta_ == nullptr) {
    return std::numeric_limits<float>::max();
  }
  sta::dbNetwork* network = sta_->getDbNetwork();
  // Without liberty there is no timing to consult.  Report unbounded slack so
  // the caller's screening is simply inactive, rather than failing the whole
  // run: a design read straight from a db has no libraries attached.
  if (network == nullptr || network->defaultLibertyLibrary() == nullptr) {
    return std::numeric_limits<float>::max();
  }
  float worst = std::numeric_limits<float>::max();
  for (odb::dbITerm* iterm : inst->getITerms()) {
    if (iterm->getNet() == nullptr) {
      continue;
    }
    sta::Pin* pin = network->dbToSta(iterm);
    if (pin == nullptr) {
      continue;
    }
    const float slack = sta_->slack(
        pin, sta::RiseFallBoth::riseFall(), sta_->scenes(), sta::MinMax::max());
    worst = std::min(worst, slack);
  }
  return worst;
}

int Watermark::clearWatermark()
{
  dbBlock* block = db_->getChip() ? db_->getChip()->getBlock() : nullptr;
  if (block == nullptr) {
    logger_->error(utl::WMK, 1, "No block loaded; run after read_def.");
    return 0;
  }
  int cleared = 0;
  for (dbNet* net : block->getNets()) {
    if (auto* p = dbBoolProperty::find(net, "watermark")) {
      odb::dbProperty::destroy(p);
      ++cleared;
    }
  }
  return cleared;
}

int Watermark::selectNetsKeyed(const std::array<std::uint8_t, 32>& key,
                               double fraction)
{
  if (fraction <= 0.0 || fraction > 1.0) {
    logger_->error(
        utl::WMK, 12, "fraction must be in (0, 1]; got {:.3f}.", fraction);
    return 0;
  }
  dbBlock* block = db_->getChip() ? db_->getChip()->getBlock() : nullptr;
  if (block == nullptr) {
    logger_->error(utl::WMK, 13, "No block loaded; run after read_def.");
    return 0;
  }

  clearWatermark();

  // Threshold = floor(fraction * 2^32).  Selecting iff u32 < threshold yields
  // the same Bernoulli rule as the paper's u_R(n) < fraction.
  const std::uint64_t threshold
      = static_cast<std::uint64_t>(std::llround(fraction * 4294967296.0));

  int n_candidates = 0;
  int n_pick = 0;
  // Iterate netlist in name-sorted order so that the selected set is
  // independent of pointer order.  HMAC of the same name gives the same u
  // regardless of iteration order, so this is mainly for log determinism.
  std::vector<dbNet*> nets;
  nets.reserve(block->getNets().size());
  for (dbNet* net : block->getNets()) {
    if (isRoutableSignalNet(net)) {
      nets.push_back(net);
    }
  }
  std::ranges::sort(
      nets, [](dbNet* a, dbNet* b) { return a->getName() < b->getName(); });

  for (dbNet* net : nets) {
    ++n_candidates;
    const std::string& name = net->getName();
    // msg = "net" || 0x00 || name  (matches the Python verifier)
    std::vector<std::uint8_t> msg;
    msg.reserve(4 + name.size());
    msg.push_back('n');
    msg.push_back('e');
    msg.push_back('t');
    msg.push_back('\0');
    for (char c : name) {
      msg.push_back(static_cast<std::uint8_t>(c));
    }
    std::uint8_t mac[32];
    hmac_sha256(key.data(), key.size(), msg.data(), msg.size(), mac);
    const std::uint64_t u32 = static_cast<std::uint64_t>(mac[0])
                              | (static_cast<std::uint64_t>(mac[1]) << 8)
                              | (static_cast<std::uint64_t>(mac[2]) << 16)
                              | (static_cast<std::uint64_t>(mac[3]) << 24);
    if (u32 < threshold) {
      dbBoolProperty::create(net, "watermark", true);
      ++n_pick;
    }
  }

  logger_->info(utl::WMK,
                14,
                "Keyed routing watermark: tagged {} / {} signal nets "
                "(fraction={:.4f}, key=HMAC-SHA256, 32 bytes).",
                n_pick,
                n_candidates,
                fraction);
  return n_pick;
}

double Watermark::reportWatermark(double p)
{
  if (p <= 0.0 || p >= 1.0) {
    logger_->error(utl::WMK, 6, "p must be in (0, 1); got {:.3f}.", p);
    return 1.0;
  }
  dbBlock* block = db_->getChip() ? db_->getChip()->getBlock() : nullptr;
  if (block == nullptr) {
    logger_->error(utl::WMK, 7, "No block loaded.");
    return 1.0;
  }

  struct Row
  {
    dbNet* net;
    double ratio;
    bool is_watermark;
  };

  std::vector<Row> rows;
  rows.reserve(block->getNets().size());
  std::int64_t total_wl = 0;
  std::int64_t total_way = 0;
  int diagonal_segments = 0;
  for (dbNet* net : block->getNets()) {
    if (!isRoutableSignalNet(net)) {
      continue;
    }
    std::int64_t l_ww = 0;
    std::int64_t l_tot = 0;
    canonicalWirelength(net, l_ww, l_tot, diagonal_segments);
    if (l_tot <= 0) {
      continue;  // unrouted or empty
    }
    total_wl += l_tot;
    total_way += l_ww;
    const double ratio = static_cast<double>(l_ww) / static_cast<double>(l_tot);
    const bool is_wm = (dbBoolProperty::find(net, "watermark") != nullptr);
    rows.push_back({.net = net, .ratio = ratio, .is_watermark = is_wm});
  }

  if (rows.empty()) {
    logger_->warn(
        utl::WMK, 8, "No routed signal nets found; run after detailed_route.");
    return 1.0;
  }

  // Rank all nets by ascending wrong-way ratio.  Ties broken by net
  // name for determinism.
  std::ranges::sort(rows, [](const Row& a, const Row& b) {
    if (a.ratio != b.ratio) {
      return a.ratio < b.ratio;
    }
    return a.net->getName() < b.net->getName();
  });

  // Cutoff rank = p-quantile (0-based exclusive index).  A net ranked
  // strictly below cutoff passes.
  const int cutoff
      = static_cast<int>(std::floor(p * static_cast<double>(rows.size())));

  int X = 0;       // number of watermark nets
  int passed = 0;  // watermark nets below cutoff
  for (int rank = 0; std::cmp_less(rank, rows.size()); ++rank) {
    const Row& r = rows[rank];
    if (r.is_watermark) {
      ++X;
      if (rank < cutoff) {
        ++passed;
      }
    }
  }
  const int x_failed = X - passed;
  const double Pc = binomialTail(X, x_failed, p);

  logger_->info(utl::WMK,
                9,
                "Routing watermark report: X={} watermark nets, "
                "{} passed, {} failed (p={:.3f}, cutoff rank={} / {}).",
                X,
                passed,
                x_failed,
                p,
                cutoff,
                static_cast<int>(rows.size()));
  logger_->info(utl::WMK,
                10,
                "Total signal wirelength = {} DBU, wrong-way = {} DBU "
                "({:.3f}%).",
                total_wl,
                total_way,
                total_wl > 0 ? 100.0 * static_cast<double>(total_way)
                                   / static_cast<double>(total_wl)
                             : 0.0);
  logger_->info(utl::WMK, 11, "Signature strength Pc = {:.3e}.", Pc);
  return Pc;
}

}  // namespace wmk
