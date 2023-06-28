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

#include "db.h"
#include "dbMap.h"
#include "dbShape.h"
#include "dbWireCodec.h"
#include "tmg_conn.h"
#include "utl/Logger.h"

namespace odb {

void tmg_conn::checkConnOrdered()
{
  dbITerm* drv_iterm = nullptr;
  dbBTerm* drv_bterm = nullptr;
  dbITerm* itermV[1024];
  dbBTerm* btermV[1024];
  int itermN = 0;
  int btermN = 0;
  int j;
  _connected = true;
  dbWire* wire = _net->getWire();
  dbWirePathItr pitr;
  dbWirePath path;
  dbWirePathShape pathShape;
  pitr.begin(wire);
  int first = 1;
  while (pitr.getNextPath(path)) {
    if (!path.is_branch) {
      if (!path.iterm && !path.bterm) {
        _connected = false;
      }
      if (first) {
        first = 0;
        if (path.iterm) {
          drv_iterm = path.iterm;
          itermV[itermN++] = drv_iterm;
        }
        if (path.bterm) {
          drv_bterm = path.bterm;
          btermV[btermN++] = drv_bterm;
        }
      } else {
        if (path.iterm) {
          for (j = 0; j < itermN; j++)
            if (itermV[j] == path.iterm)
              break;
          if (j == itermN) {
            _connected = false;
            itermV[itermN++] = path.iterm;
          }
        }
        if (path.bterm) {
          for (j = 0; j < btermN; j++)
            if (btermV[j] == path.bterm)
              break;
          if (j == btermN) {
            _connected = false;
            btermV[btermN++] = path.bterm;
          }
        }
      }
    }
    while (pitr.getNextShape(pathShape)) {
      if (pathShape.iterm) {
        for (j = 0; j < itermN; j++)
          if (itermV[j] == pathShape.iterm)
            break;
        if (j == itermN) {
          itermV[itermN++] = pathShape.iterm;
        }
      } else if (pathShape.bterm) {
        for (j = 0; j < btermN; j++)
          if (btermV[j] == pathShape.bterm)
            break;
        if (j == btermN)
          btermV[btermN++] = pathShape.bterm;
      }
    }
  }
  if (!_connected) {
    _net->setDisconnected(true);
    logger_->info(utl::ODB, 15, "disconnected net {}", _net->getName());
  }
}

}  // namespace odb
