// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors
//
// Routing watermark.
//
// Implements the routing watermarking scheme of
//   A. B. Kahng, S. Mantik, I. L. Markov, M. Potkonjak, P. Tucker,
//   H. Wang and G. Wolfe, "Robust IP Watermarking Methodologies for
//   Physical Design", in ISPD'98.
//
// The idea: use a keyed PRF to select a subset of signal nets (the
// watermark nets) and impose a strong upper bound on the amount of
// wrong-way (non-preferred-direction) wiring used to route them.
// Detection compares, for every net, the ratio WL_way / WL_tot; watermark
// nets are expected to rank at the low end.  Because selection is keyed,
// the marked set is unpredictable without the key.
//
// This header is the public API for the wmk module; it is SWIG-wrapped
// for Tcl.

#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "odb/db.h"
#include "utl/Logger.h"

namespace sta {
class dbSta;
}

namespace dpl {
class Opendp;
}

namespace est {
class EstimateParasitics;
}

namespace wmk {

// Knobs for the placement watermark.  The defaults are the values the scheme
// was characterised with; they trade capacity against how much timing and
// wirelength slack the marks are allowed to consume.
struct PlacementOptions
{
  int grid_nx = 8;
  int grid_ny = 8;
  double pair_dist_um = 1.0;
  int pairs_per_tile = 4;
  // Cells with less slack than this are left alone, so the watermark does not
  // land on the paths that decide the clock period.
  double slack_threshold_ns = 0.20;
  // A swap that changes half-perimeter wirelength by more than this is
  // reverted, keeping the mark invisible in wirelength.  Database units differ
  // between platforms, so this is 0.05 um on NanGate45 and 1.0 um on ASAP7;
  // it is inherited from the reference implementation rather than chosen per
  // platform, and is worth setting deliberately on a new one.
  int hpwl_eps_dbu = 100;
  // How far legalization may move a cell afterwards, in microns.
  int max_disp_um = 5;
  // Below this many pairs the design is searched again with the gates widened,
  // because a design that yields only a handful of bits cannot prove much.
  int min_pairs_total = 64;
  // After legalization, a pair whose cells lost more slack than this is put
  // back.  A watermark that costs timing is not worth the evidence.
  double guard_degrade_ns = 0.02;
  bool post_guard = true;
};

// One committed placement mark: the pair, and which order the key called for.
struct PlacementClaim
{
  std::string a_name;
  std::string b_name;
  int target_bit = 0;
  bool already_satisfied = false;
};

// Knobs for the clock tree watermark.
struct CtsOptions
{
  // Upper bound on how many buffer pairs to mark.  Each pair carries one bit.
  int num_pairs = 32;
  // Two buffers are only paired if their centres are within this distance, so
  // a moved sink stays local and the skew cost stays small.
  double sibling_dist_um = 20.0;
  // How much worse a moved sink may leave the clock's worst skew.  Not zero:
  // a sink move almost always costs a little skew, and demanding none at all
  // turns the stage off wherever the clock is tight.  Measured on a routed
  // ASAP7 aes, whose period is 380 ps and whose skew is 7.8 ps, a zero margin
  // rejects 14 of 26 pairs and leaves the extraction rate at 0.46; 20 ps
  // accepts all 26 and costs about 1 ps of skew.
  double skew_margin_ns = 0.020;
  // Fraction of a leaf buffer's liberty slew and capacitance limits that must
  // remain unused after a sink has moved onto it.  The limits come from the
  // library rather than from a number fixed here, so the check follows
  // whatever technology the design is built in.
  double slew_headroom_frac = 0.20;
  double cap_headroom_frac = 0.20;
};

// One committed clock tree mark.
struct CtsClaim
{
  std::string pair_key;
  std::string target_lcb;
  std::string other_lcb;
  int target_bit = 0;
  int final_bit = 0;
};

// Outcome of the routing watermark test.  Unlike placement and CTS this is a
// population statistic, not a count of claims: the marked nets should carry
// less wrong-way metal than the rest.
struct RoutingStat
{
  int eligible = 0;
  int marked = 0;
  double q_marked = 0.0;
  double q_rest = 0.0;
  // Difference in mean wrong-way fraction.  More negative is stronger.
  //
  // The sign alone is not evidence: on a design carrying no watermark it is a
  // coin flip, so the decision is made on p_r, not on this.
  double t_r = 0.0;
  // Randomization p-value: the fraction of uniformly drawn marked sets of the
  // same size whose T_R is at least as negative.  Floored at 1/(B+1).
  double p_r = 1.0;
  // log10 of an upper bound on the same tail, computed in closed form.  It is
  // exact when the marked nets carry no wrong-way metal at all, which is the
  // case where the sampling floor above is far too coarse to be useful.  0.0
  // means the bound is vacuous.
  double log10_tail = 0.0;
  int zero_wrongway_nets = 0;
  // True when no eligible net anywhere carries wrong-way metal.  The carrier
  // does not exist on such a technology, so the stage cannot be tested at all
  // -- which is a different answer from testing it and finding nothing.
  bool carrier_absent = false;
};

// Outcome of checking one stage's claims against a design.  Ownership is
// decided by the extraction rate against a threshold rather than by an exact
// match, because routing and filling disturb a few marked objects.
struct VerifyResult
{
  int checked = 0;
  int held = 0;

  double rate() const
  {
    return checked > 0 ? static_cast<double>(held) / checked : 0.0;
  }
};

class Watermark
{
 public:
  Watermark(odb::dbDatabase* db,
            sta::dbSta* sta,
            dpl::Opendp* opendp,
            est::EstimateParasitics* estimate_parasitics,
            utl::Logger* logger);

  // Select the watermark nets (PDMarks paper, Eq. eq:routing_selection):
  //   u_R(n) = first 4 bytes of HMAC-SHA256(key, "net\0" || net_name)
  //                interpreted as little-endian uint32, divided by 2^32
  //   net selected iff u_R(n) < fraction
  // Each net is therefore an independent Bernoulli(fraction) draw whose
  // outcome is computationally unpredictable without the 32-byte key.
  // Returns the number of nets tagged.  Clears any existing tags first.
  int selectNetsKeyed(const std::array<std::uint8_t, 32>& key, double fraction);

  // After detailed routing, classify every signal net as watermarked
  // (successfully) or not based on the ratio r = WL_way / WL_tot.
  // A net "passes" if its rank among all signal nets (sorted by
  // ascending r) is below the p-quantile.  Prints:
  //   X  = number of watermark nets
  //   s  = number that passed (below cutoff)
  //   x  = number that failed  (X - s)
  //   Pc = signature strength (binomial tail)
  // Returns the computed Pc.
  double reportWatermark(double p = 0.4);

  // Clear all "watermark" dbBoolProperty tags on nets in the current
  // block.  Useful for iterating on signature-selection parameters
  // without re-loading the design.
  int clearWatermark();

  // Check the placement claims in ``claims_file`` against the loaded design.
  // Each claim names a pair of cells and the bit they were driven to, which is
  // which of the two sits further left.
  VerifyResult verifyPlacement(const std::string& claims_file);

  // Check the CTS claims in ``claims_file`` against the loaded design.  Each
  // claim names a leaf clock buffer and the parity its sequential fanout was
  // driven to.
  VerifyResult verifyCts(const std::string& claims_file);

  // Put a keyed subset of same-row, same-width cell pairs into a keyed
  // left-to-right order, and report the pairs committed in ``claims``.  The
  // caller is responsible for legalizing afterwards.
  int embedPlacement(const std::array<std::uint8_t, 32>& key,
                     const PlacementOptions& opts,
                     std::vector<PlacementClaim>& claims);

  // Embed the placement watermark and write its claims.  Legalizes afterwards
  // and drops any pair whose cells lost slack, so a mark never costs timing.
  // Returns the number of pairs that survived.
  int placementWatermark(const std::array<std::uint8_t, 32>& key,
                         const PlacementOptions& opts,
                         const std::string& claims_file);

  // Set the sequential fanout parity of a keyed subset of leaf clock buffers
  // and write the committed pairs.  A pair is only claimed if the move it
  // needed did not worsen clock skew.
  int ctsWatermark(const std::array<std::uint8_t, 32>& key,
                   const CtsOptions& opts,
                   const std::string& claims_file);

  // Test the routing watermark on a routed design.  The marked set is
  // recovered from the key, so no record from embed time is needed.
  // ``permutations`` is the number of null draws behind p_r; it bounds the
  // smallest reportable p-value at 1/(permutations + 1).
  RoutingStat verifyRouting(const std::array<std::uint8_t, 32>& key,
                            double fraction,
                            int permutations = 100000);

 private:
  // What one committed pair moved, so it can be put back if the design turns
  // out to have paid for it in timing.
  struct PlacementEdit
  {
    odb::dbInst* a = nullptr;
    odb::dbInst* b = nullptr;
    odb::Point a_loc;
    odb::Point b_loc;
    float a_slack = 0.0f;
    float b_slack = 0.0f;
  };

  // embedPlacement, also reporting what it moved.
  int embedPlacementEdits(const std::array<std::uint8_t, 32>& key,
                          const PlacementOptions& opts,
                          std::vector<PlacementClaim>& claims,
                          std::vector<PlacementEdit>& edits);

  // Does this driver still have the headroom the library asks for, once
  // whatever was going to be connected to it has been?  True when there is no
  // timing to consult, so an unscreened design is not blocked outright.
  bool driverHeadroomOk(odb::dbInst* inst,
                        double slew_frac,
                        double cap_frac) const;

  // Worst slack over an instance's pins, in seconds.  Returns the maximum
  // representable value when no timing has been set up, so that an unscreened
  // design does not silently reject every candidate.
  float worstSlack(odb::dbInst* inst) const;

  // Worst clock skew over the design, or 0 when there is no timing to
  // consult, so a design without liberty is simply unscreened.
  float worstClockSkew() const;

  // Is there enough timing set up for worstClockSkew to mean anything?  A zero
  // from it is otherwise indistinguishable from a design with no skew.
  bool clockSkewAvailable() const;

  // The clocks reaching an instance's output pin, as sorted clock indices.
  //
  // Two leaf buffers may only be paired when a sink can move between them
  // without changing which clock it is on.  Distance alone does not establish
  // that: two trees can run alongside each other, and a move across them
  // reconnects a flop to a different clock, which changes what the design
  // does.  Empty when there is no timing to ask, or when the instance is not
  // a single-output cell -- in either case the caller must not pair it.
  std::vector<int> clockIndicesAt(odb::dbInst* inst) const;

  // Re-estimate the parasitics of the two nets a sink was moved between.
  //
  // Rewiring changes what each net drives, and nothing recomputes the RC until
  // something asks.  Without this the skew and drive checks would read the
  // values cached before the move, and every move would look free.  Only the
  // two nets that changed are touched, not the whole design.
  void reestimateNetParasitics(odb::dbNet* a, odb::dbNet* b) const;

  odb::dbDatabase* db_ = nullptr;
  sta::dbSta* sta_ = nullptr;
  dpl::Opendp* opendp_ = nullptr;
  // Moving a cell changes only its parasitics, and those are not recomputed
  // until something asks.  Without this the post-embed timing check would
  // compare a slack against itself and never find anything.
  est::EstimateParasitics* estimate_parasitics_ = nullptr;
  utl::Logger* logger_ = nullptr;
};

}  // namespace wmk
