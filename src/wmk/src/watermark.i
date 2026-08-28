// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

%{
#include <string>
#include "ord/OpenRoad.hh"
#include "wmk/Watermark.h"
%}

%include "../../Exception.i"
%include <std_string.i>

%{
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include "HmacSha256.h"
#include "Keys.h"
%}

%inline %{

// Draw a fresh secret key.  Returned as hex rather than logged: the module
// never writes a secret key to the log, and never keeps one after the command
// returns.
const char*
random_hex_cmd(int n_bytes)
{
  static std::string result;
  std::vector<std::uint8_t> raw;
  if (n_bytes <= 0 || !wmk::randomBytes(static_cast<std::size_t>(n_bytes), raw)) {
    result.clear();
    return result.c_str();
  }
  result = wmk::toHex(raw.data(), raw.size());
  return result.c_str();
}

// K_s = HMAC-SHA256(K, ID(D_0), nu, "stage=" || s).  Empty on bad input.
const char*
derive_stage_key_cmd(const char* master_hex,
                     const char* design_id,
                     const char* nonce_hex,
                     const char* stage)
{
  static std::string result;
  result.clear();
  std::array<std::uint8_t, 32> master;
  std::vector<std::uint8_t> nonce;
  if (!wmk::parse_hex_key32(std::string(master_hex), master)
      || !wmk::fromHex(std::string(nonce_hex), nonce)
      || !wmk::isWatermarkStage(std::string(stage))) {
    return result.c_str();
  }
  const std::array<std::uint8_t, 32> k = wmk::deriveStageKey(
      master, std::string(design_id), nonce, std::string(stage));
  result = wmk::toHex(k.data(), k.size());
  return result.c_str();
}

int
set_routing_watermark_cmd(const char* key_hex, double fraction)
{
  std::array<std::uint8_t, 32> key;
  if (!wmk::parse_hex_key32(std::string(key_hex), key)) {
    auto* w = ord::OpenRoad::openRoad()->getWatermark();
    (void) w;
    return -1;
  }
  auto* w = ord::OpenRoad::openRoad()->getWatermark();
  return w->selectNetsKeyed(key, fraction);
}

double
report_routing_watermark_cmd(double p)
{
  auto* w = ord::OpenRoad::openRoad()->getWatermark();
  return w->reportWatermark(p);
}

int
clear_routing_watermark_cmd()
{
  auto* w = ord::OpenRoad::openRoad()->getWatermark();
  return w->clearWatermark();
}

int
place_watermark_cmd(const char* key_hex,
                    const char* claims_file,
                    int grid_nx,
                    int grid_ny,
                    double pair_dist_um,
                    int pairs_per_tile,
                    double slack_threshold_ns,
                    double hpwl_eps_um,
                    int max_disp_um,
                    int min_pairs_total,
                    double guard_degrade_ns)
{
  std::array<std::uint8_t, 32> key;
  if (!wmk::parse_hex_key32(std::string(key_hex), key)) {
    return -1;
  }
  wmk::PlacementOptions opts;
  opts.grid_nx = grid_nx;
  opts.grid_ny = grid_ny;
  opts.pair_dist_um = pair_dist_um;
  opts.pairs_per_tile = pairs_per_tile;
  opts.slack_threshold_ns = slack_threshold_ns;
  opts.hpwl_eps_um = hpwl_eps_um;
  opts.max_disp_um = max_disp_um;
  opts.min_pairs_total = min_pairs_total;
  opts.guard_degrade_ns = guard_degrade_ns;
  opts.post_guard = guard_degrade_ns > 0.0;
  auto* w = ord::OpenRoad::openRoad()->getWatermark();
  return w->placementWatermark(key, opts, std::string(claims_file));
}

int
cts_watermark_cmd(const char* key_hex,
                  const char* claims_file,
                  int num_pairs,
                  double sibling_dist_um,
                  double skew_margin_ns,
                  double slew_headroom_frac,
                  double cap_headroom_frac)
{
  std::array<std::uint8_t, 32> key;
  if (!wmk::parse_hex_key32(std::string(key_hex), key)) {
    return -1;
  }
  wmk::CtsOptions opts;
  opts.num_pairs = num_pairs;
  opts.sibling_dist_um = sibling_dist_um;
  opts.skew_margin_ns = skew_margin_ns;
  opts.slew_headroom_frac = slew_headroom_frac;
  opts.cap_headroom_frac = cap_headroom_frac;
  auto* w = ord::OpenRoad::openRoad()->getWatermark();
  return w->ctsWatermark(key, opts, std::string(claims_file));
}

// Routing verification returns the p-value the ownership decision is made on:
// the sampled one, or the closed-form tail when that is tighter.  T_R and both
// components are reported by the module.  Returns -1 on a bad key.
double
verify_routing_watermark_cmd(const char* key_hex,
                             double fraction,
                             int permutations)
{
  std::array<std::uint8_t, 32> key;
  if (!wmk::parse_hex_key32(std::string(key_hex), key)) {
    return -1.0;
  }
  auto* w = ord::OpenRoad::openRoad()->getWatermark();
  const wmk::RoutingStat s = w->verifyRouting(key, fraction, permutations);
  if (s.carrier_absent) {
    return -2.0;
  }
  return std::min(s.p_r, std::pow(10.0, s.log10_tail));
}

// Verification returns the extraction rate; the caller compares it against the
// ownership threshold.  A stage with no checkable claims returns -1 so that it
// is distinguishable from a stage where every claim failed.
double
verify_placement_watermark_cmd(const char* claims_file)
{
  auto* w = ord::OpenRoad::openRoad()->getWatermark();
  const wmk::VerifyResult r = w->verifyPlacement(std::string(claims_file));
  return r.checked > 0 ? r.rate() : -1.0;
}

double
verify_cts_watermark_cmd(const char* claims_file)
{
  auto* w = ord::OpenRoad::openRoad()->getWatermark();
  const wmk::VerifyResult r = w->verifyCts(std::string(claims_file));
  return r.checked > 0 ? r.rate() : -1.0;
}

%}  // inline
