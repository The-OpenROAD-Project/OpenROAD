// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#include "db/obj/frNet.h"

#include "db/obj/frBTerm.h"
namespace drt {

bool frNet::hasBTermsAboveTopLayer() const
{
  for (const auto term : getBTerms()) {
    if (term->isAboveTopLayer()) {
      return true;
    }
  }
  return false;
}
}  // namespace drt
