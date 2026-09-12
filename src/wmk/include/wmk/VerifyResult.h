// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors
#pragma once

namespace wmk {

// Counts remain available even when a stage has too little ownership evidence.
struct VerifyResult
{
  int checked = 0;
  int held = 0;

  double rate() const
  {
    return checked > 0 ? static_cast<double>(held) / checked : 0.0;
  }

  // P[Binomial(checked, 1/2) >= held], under independent, precommitted fair
  // target bits. This is a model-based chance probability, not authentication
  // of the claims or a probability that an ownership assertion is true.
  double pValue() const;
};

}  // namespace wmk
