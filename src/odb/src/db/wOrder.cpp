// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "odb/wOrder.h"

#include "odb/db.h"
#include "tmg_conn.h"

namespace odb {

void orderWires(utl::Logger* logger, dbBlock* block)
{
  tmg_conn conn(logger);

  for (auto net : block->getNets()) {
    if (net->getSigType().isSupply() || net->isWireOrdered()) {
      continue;
    }
    conn.analyzeNet(net);
  }
}

}  // namespace odb
