// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#include "Utilities.h"

#include "odb/db.h"

namespace pad {

void Utilities::makeSpecial(odb::dbNet* net)
{
  net->setSpecial();
  odb::dbSigType sigtype = net->getSigType();
  for (auto* iterm : net->getITerms()) {
    if (iterm->getSigType().isSupply()) {
      sigtype = iterm->getSigType();
    }
  }

  for (auto* iterm : net->getITerms()) {
    iterm->setSpecial();
  }
  for (auto* bterm : net->getBTerms()) {
    bterm->setSpecial();
    bterm->setSigType(sigtype);
  }
  net->setSigType(sigtype);
}

}  // namespace pad
