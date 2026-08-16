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

namespace wmk {

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
  Watermark(odb::dbDatabase* db, utl::Logger* logger);

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

 private:
  odb::dbDatabase* db_ = nullptr;
  utl::Logger* logger_ = nullptr;
};

}  // namespace wmk
