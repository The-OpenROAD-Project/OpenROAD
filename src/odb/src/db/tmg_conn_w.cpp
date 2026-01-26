// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include <cstdio>
#include <cstdlib>
#include <unordered_set>

#include "odb/db.h"
#include "odb/dbShape.h"
#include "odb/dbWireCodec.h"
#include "tmg_conn.h"
#include "utl/Logger.h"

namespace odb {

void tmg_conn::checkConnOrdered()
{
  std::unordered_set<dbITerm*> iterms;
  std::unordered_set<dbBTerm*> bterms;
  connected_ = true;
  dbWirePathItr pitr;
  pitr.begin(net_->getWire());
  dbWirePath path;
  bool first = true;
  while (pitr.getNextPath(path)) {
    if (!path.is_branch) {
      if (!path.iterm && !path.bterm) {
        connected_ = false;
      }
      if (first) {
        first = false;
        if (path.iterm) {
          iterms.insert(path.iterm);
        }
        if (path.bterm) {
          bterms.insert(path.bterm);
        }
      } else {
        if (path.iterm) {
          if (iterms.find(path.iterm) == iterms.end()) {
            connected_ = false;
            iterms.insert(path.iterm);
          }
        }
        if (path.bterm) {
          if (bterms.find(path.bterm) == bterms.end()) {
            connected_ = false;
            bterms.insert(path.bterm);
          }
        }
      }
    }
    dbWirePathShape pathShape;
    while (pitr.getNextShape(pathShape)) {
      if (pathShape.iterm) {
        iterms.insert(pathShape.iterm);
      } else if (pathShape.bterm) {
        bterms.insert(pathShape.bterm);
      }
    }
  }
  if (!connected_) {
    net_->setDisconnected(true);
    logger_->info(utl::ODB, 15, "disconnected net {}", net_->getName());
  }
}

}  // namespace odb
