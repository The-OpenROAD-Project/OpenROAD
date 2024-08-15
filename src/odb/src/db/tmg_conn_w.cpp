///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2019, Nefelus Inc
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

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
  _connected = true;
  dbWire* wire = _net->getWire();
  dbWirePathItr pitr;
  dbWirePath path;
  pitr.begin(wire);
  bool first = true;
  while (pitr.getNextPath(path)) {
    if (!path.is_branch) {
      if (!path.iterm && !path.bterm) {
        _connected = false;
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
            _connected = false;
            iterms.insert(path.iterm);
          }
        }
        if (path.bterm) {
          if (bterms.find(path.bterm) == bterms.end()) {
            _connected = false;
            bterms.insert(path.bterm);
          }
        }
      }
    }
    dbWirePathShape pathShape;
    while (pitr.getNextShape(pathShape)) {
      if (pathShape.iterm) {
        if (iterms.find(pathShape.iterm) == iterms.end()) {
          iterms.insert(pathShape.iterm);
        }
      } else if (pathShape.bterm) {
        if (bterms.find(pathShape.bterm) == bterms.end()) {
          bterms.insert(pathShape.bterm);
        }
      }
    }
  }
  if (!_connected) {
    _net->setDisconnected(true);
    logger_->info(utl::ODB, 15, "disconnected net {}", _net->getName());
  }
}

}  // namespace odb
