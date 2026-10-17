// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "odb/wOrder.h"

#include "odb/db.h"
#include "tmg_conn.h"

namespace odb {

void orderWires(utl::Logger* logger, dbBlock* block)
{
  tmg_conn conn(logger);

  for (dbNet* net : block->getNets()) {
    if (net->getSigType().isSupply()) {
      continue;
    }

    dbWire* wire = net->getWire();

    if (wire && !wire->isOrdered()) {
      conn.setNet(net);
      conn.analyzeNet();
    }
  }
}

}  // namespace odb
