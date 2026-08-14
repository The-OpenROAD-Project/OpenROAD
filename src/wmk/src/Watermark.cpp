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
#include <random>
#include <string>
#include <vector>

#include "HmacSha256.h"
#include "odb/db.h"
#include "odb/dbWireCodec.h"
#include "utl/Logger.h"

namespace wmk {

using odb::dbBlock;
using odb::dbBoolProperty;
using odb::dbNet;
using odb::dbSigType;
using odb::dbTechLayer;
using odb::dbTechLayerDir;
using odb::dbWire;
using odb::dbWireDecoder;

namespace {

// Deterministically build a 32-bit PRNG seed from a message.  We avoid
// an MD5 dependency: the scheme in Kahng et al. uses MD5/RSA/RC4 but
// any cryptographically strong PRNG will do for signature selection.
// For the MVP we use std::seed_seq over the message bytes, which gives
// reproducible selection for a given (message, net-count, fraction).
std::mt19937 makePrng(const std::string& message)
{
  std::vector<std::uint32_t> seed_bytes;
  seed_bytes.reserve(message.size() + 1);
  for (unsigned char c : message) {
    seed_bytes.push_back(static_cast<std::uint32_t>(c));
  }
  // Length terminator so that messages of the form "foo" and "foo\0..."
  // still differ.
  seed_bytes.push_back(static_cast<std::uint32_t>(message.size()));
  std::seed_seq seq(seed_bytes.begin(), seed_bytes.end());
  return std::mt19937(seq);
}

// A signal net we are allowed to watermark.  Matches the paper: we
// skip special / supply / clock nets.
bool isRoutableSignal(dbNet* net)
{
  if (net->isSpecial()) {
    return false;
  }
  const dbSigType sig = net->getSigType();
  if (sig.isSupply()) {
    return false;
  }
  if (sig == dbSigType::CLOCK) {
    return false;
  }
  return true;
}

// Sum the preferred- and non-preferred-direction wirelengths of the
// given routed net, in DBU.  Vias contribute nothing (zero-length in
// the plane); PATH/POINT/POINT_EXT give rectilinear segments on the
// current layer.
void measureNetWirelength(dbNet* net, std::int64_t& wl_tot,
                          std::int64_t& wl_way)
{
  wl_tot = 0;
  wl_way = 0;
  dbWire* wire = net->getWire();
  if (wire == nullptr) {
    return;
  }
  dbWireDecoder dec;
  dec.begin(wire);
  int prev_x = 0;
  int prev_y = 0;
  bool have_prev = false;
  dbTechLayer* layer = nullptr;
  for (;;) {
    dbWireDecoder::OpCode op = dec.next();
    if (op == dbWireDecoder::END_DECODE) {
      break;
    }
    switch (op) {
      case dbWireDecoder::PATH:
      case dbWireDecoder::JUNCTION:
      case dbWireDecoder::SHORT:
      case dbWireDecoder::VWIRE:
        layer = dec.getLayer();
        have_prev = false;  // fresh path: next POINT is the anchor
        break;
      case dbWireDecoder::POINT:
      case dbWireDecoder::POINT_EXT: {
        int x = 0;
        int y = 0;
        if (op == dbWireDecoder::POINT) {
          dec.getPoint(x, y);
        } else {
          int ext = 0;
          dec.getPoint(x, y, ext);
        }
        if (have_prev && layer != nullptr) {
          const int dx = std::abs(x - prev_x);
          const int dy = std::abs(y - prev_y);
          const std::int64_t seg = dx + dy;  // rectilinear
          if (seg > 0) {
            const dbTechLayerDir dir = layer->getDirection();
            bool seg_is_way = false;
            if (dir == dbTechLayerDir::HORIZONTAL) {
              // Preferred axis is X; any Y movement is wrong-way.
              seg_is_way = (dy > 0);
            } else if (dir == dbTechLayerDir::VERTICAL) {
              seg_is_way = (dx > 0);
            } else {
              // NONE -> no preferred axis; count as preferred.
              seg_is_way = false;
            }
            wl_tot += seg;
            if (seg_is_way) {
              wl_way += seg;
            }
          }
        }
        prev_x = x;
        prev_y = y;
        have_prev = true;
        break;
      }
      case dbWireDecoder::VIA:
      case dbWireDecoder::TECH_VIA:
        // Via changes the layer but occupies the same (x,y); do NOT
        // advance the "previous point" because the next POINT on the
        // new layer starts a fresh segment anchor.
        layer = dec.getLayer();
        have_prev = true;  // keep anchor at current (prev_x, prev_y)
        break;
      default:
        // RECT, ITERM, BTERM, RULE -- no wirelength contribution.
        break;
    }
  }
}

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
  const double max_lt
      = *std::max_element(log_terms.begin(), log_terms.end());
  double acc = 0.0;
  for (double lt : log_terms) {
    acc += std::exp(lt - max_lt);
  }
  return std::exp(max_lt) * acc;
}

}  // namespace

Watermark::Watermark(odb::dbDatabase* db, utl::Logger* logger)
    : db_(db), logger_(logger)
{
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

int Watermark::selectNets(const std::string& message, double fraction)
{
  if (fraction <= 0.0 || fraction > 1.0) {
    logger_->error(utl::WMK,
                   2,
                   "fraction must be in (0, 1]; got {:.3f}.",
                   fraction);
    return 0;
  }
  dbBlock* block = db_->getChip() ? db_->getChip()->getBlock() : nullptr;
  if (block == nullptr) {
    logger_->error(utl::WMK, 3, "No block loaded; run after read_def.");
    return 0;
  }

  // Clear any previously tagged nets so re-selection is idempotent.
  clearWatermark();

  // Build a name-sorted list of routable signal nets.  Sorting by name
  // makes the selection netlist-stable and floorplan-independent (see
  // footnote 8 of Kahng et al.).
  std::vector<dbNet*> candidates;
  candidates.reserve(block->getNets().size());
  for (dbNet* net : block->getNets()) {
    if (isRoutableSignal(net)) {
      candidates.push_back(net);
    }
  }
  std::sort(candidates.begin(),
            candidates.end(),
            [](dbNet* a, dbNet* b) {
              return a->getName() < b->getName();
            });

  const int n_candidates = static_cast<int>(candidates.size());
  if (n_candidates == 0) {
    logger_->warn(utl::WMK, 4, "No routable signal nets found.");
    return 0;
  }

  int n_pick = static_cast<int>(std::lround(fraction * n_candidates));
  n_pick = std::clamp(n_pick, 1, n_candidates);

  // Signature-driven Fisher-Yates partial shuffle: draw n_pick distinct
  // indices using the message-seeded PRNG.
  std::mt19937 prng = makePrng(message);
  std::vector<int> idxs(n_candidates);
  for (int i = 0; i < n_candidates; ++i) {
    idxs[i] = i;
  }
  for (int i = 0; i < n_pick; ++i) {
    std::uniform_int_distribution<int> dist(i, n_candidates - 1);
    const int j = dist(prng);
    std::swap(idxs[i], idxs[j]);
  }

  for (int i = 0; i < n_pick; ++i) {
    dbNet* net = candidates[idxs[i]];
    dbBoolProperty::create(net, "watermark", true);
  }

  logger_->info(utl::WMK,
                5,
                "Tagged {} / {} signal nets as watermark nets"
                " (fraction={:.3f}, message=\"{}\").",
                n_pick,
                n_candidates,
                fraction,
                message);
  return n_pick;
}

int Watermark::selectNetsKeyed(const std::array<std::uint8_t, 32>& key,
                               double fraction)
{
  if (fraction <= 0.0 || fraction > 1.0) {
    logger_->error(utl::WMK,
                   12,
                   "fraction must be in (0, 1]; got {:.3f}.",
                   fraction);
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
  const std::uint64_t threshold = static_cast<std::uint64_t>(
      std::llround(fraction * 4294967296.0));

  int n_candidates = 0;
  int n_pick = 0;
  // Iterate netlist in name-sorted order so that the selected set is
  // independent of pointer order.  HMAC of the same name gives the same u
  // regardless of iteration order, so this is mainly for log determinism.
  std::vector<dbNet*> nets;
  nets.reserve(block->getNets().size());
  for (dbNet* net : block->getNets()) {
    if (isRoutableSignal(net)) {
      nets.push_back(net);
    }
  }
  std::sort(nets.begin(), nets.end(), [](dbNet* a, dbNet* b) {
    return a->getName() < b->getName();
  });

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
    const std::uint64_t u32 =
        static_cast<std::uint64_t>(mac[0])
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
  for (dbNet* net : block->getNets()) {
    if (!isRoutableSignal(net)) {
      continue;
    }
    std::int64_t wl_tot = 0;
    std::int64_t wl_way = 0;
    measureNetWirelength(net, wl_tot, wl_way);
    if (wl_tot <= 0) {
      continue;  // unrouted or empty
    }
    total_wl += wl_tot;
    total_way += wl_way;
    const double ratio
        = static_cast<double>(wl_way) / static_cast<double>(wl_tot);
    const bool is_wm
        = (dbBoolProperty::find(net, "watermark") != nullptr);
    rows.push_back({net, ratio, is_wm});
  }

  if (rows.empty()) {
    logger_->warn(utl::WMK,
                  8,
                  "No routed signal nets found; run after detailed_route.");
    return 1.0;
  }

  // Rank all nets by ascending wrong-way ratio.  Ties broken by net
  // name for determinism.
  std::sort(rows.begin(),
            rows.end(),
            [](const Row& a, const Row& b) {
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
  for (int rank = 0; rank < static_cast<int>(rows.size()); ++rank) {
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
                total_wl > 0
                    ? 100.0 * static_cast<double>(total_way)
                          / static_cast<double>(total_wl)
                    : 0.0);
  logger_->info(utl::WMK, 11, "Signature strength Pc = {:.3e}.", Pc);
  return Pc;
}

}  // namespace wmk
