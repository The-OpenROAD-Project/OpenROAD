// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors
#include "wmk/VerifyResult.h"

#include <stdexcept>

#include "boost/math/distributions/binomial.hpp"

namespace wmk {

double VerifyResult::pValue() const
{
  if (checked < 0 || held < 0 || held > checked) {
    throw std::invalid_argument(
        "claim counts must satisfy 0 <= held <= checked");
  }
  if (held == 0) {
    return 1.0;
  }
  // Evaluate the complemented CDF directly; 1 - cdf loses small upper tails.
  const boost::math::binomial_distribution<double> null(checked, 0.5);
  return boost::math::cdf(boost::math::complement(null, held - 1));
}

}  // namespace wmk
