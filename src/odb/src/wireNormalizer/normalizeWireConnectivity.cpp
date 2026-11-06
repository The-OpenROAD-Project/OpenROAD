// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "odb/normalizeWireConnectivity.h"

#include "odb/db.h"
#include "wireNormalizer.h"

namespace odb {

void normalizeWireConnectivity(utl::Logger* logger, dbBlock* block)
{
  WireNormalizer wire_normalizer(logger);

  for (dbNet* net : block->getNets()) {
    if (net->getSigType().isSupply()) {
      continue;
    }

    dbWire* wire = net->getWire();

    if (wire && !wire->hasNormalizedConnectivity()) {
      wire_normalizer.setNet(net);
      wire_normalizer.normalizeWireConnectivity();
    }
  }
}

}  // namespace odb
