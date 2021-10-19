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

#include "wOrder.h"

#include <stdio.h>
#include <stdlib.h>

#include "db.h"
#include "dbLogger.h"
#include "dbMap.h"
#include "dbShape.h"
#include "dbWireCodec.h"
#include "tmg_conn.h"

namespace odb {

//
// not including tmg_db.h, only need tmg_findNet
//

static tmg_conn* _conn = NULL;

void orderWires(dbBlock* block,
                bool force,
                int cutLength,
                int maxLength,
                bool quiet)
{
  bool no_patch = true;
  if (_conn == NULL)
    _conn = new tmg_conn();
  _conn->resetSplitCnt();
  bool verbose = false;
  bool no_convert = false;
  dbSet<dbNet> nets = block->getNets();
  dbSet<dbNet>::iterator net_itr;
  dbNet* net;
  for (net_itr = nets.begin(); net_itr != nets.end(); ++net_itr) {
    net = *net_itr;
    if (net->getSigType() == dbSigType::POWER
        || net->getSigType() == dbSigType::GROUND)
      continue;
    if (!force && net->isWireOrdered())
      continue;
    _conn->analyzeNet(
        net, force, verbose, quiet, no_convert, cutLength, maxLength, no_patch);
  }
  if (_conn->_swireNetCnt)
    notice(0, "Set dont_touch on %d swire nets.\n", _conn->_swireNetCnt);
  int splitcnt = _conn->getSplitCnt();
  if (splitcnt != 0)
    notice(0, "Split top of %d T shapes.\n", splitcnt);
}

void orderWires(dbBlock* block,
                const char* net_name_or_id,
                bool force,
                bool verbose,
                bool quiet,
                int cutLength,
                int maxLength)
{
  bool no_patch = true;
  if (_conn == NULL)
    _conn = new tmg_conn();
  _conn->set_gv(verbose);
  if (!net_name_or_id || !net_name_or_id[0]) {
    orderWires(block, force, cutLength, maxLength, quiet);
    return;
  }
  bool no_convert = false;
  // dbNet *net = tmg_findNet(block,net_name_or_id);
  dbNet* net = block->findNet(net_name_or_id);
  if (net == NULL) {
    notice(0, "net not found\n");
    return;
  }
  if (net->getSigType() == dbSigType::POWER
      || net->getSigType() == dbSigType::GROUND) {
    notice(0, "skipping power net\n");
    return;
  }
  _conn->resetSplitCnt();
  _conn->analyzeNet(
      net, force, verbose, false, no_convert, cutLength, maxLength, no_patch);
  int splitcnt = _conn->getSplitCnt();
  if (splitcnt != 0)
    notice(0, "Split top of %d T shapes.\n", splitcnt);
}

void orderWires(dbNet* net, bool force, bool verbose)
{
  if (_conn == NULL)
    _conn = new tmg_conn();
  bool no_convert = false;
  if (net->getSigType() == dbSigType::POWER
      || net->getSigType() == dbSigType::GROUND) {
    notice(0, "skipping power net\n");
    return;
  }
  _conn->resetSplitCnt();
  _conn->analyzeNet(net, force, verbose, false, no_convert);
  int splitcnt = _conn->getSplitCnt();
  if (splitcnt != 0)
    notice(0, "Split top of %d T shapes.\n", splitcnt);
}

/////////////////////////////////////////////////////////////////

typedef struct tmg_wire_link
{
  tmg_wire_link* next;
  dbWire* wire;
} tmg_wire_link;

class tmg_wire_link_pool
{
 private:
  int blkSize;
  int blkN;
  int curblk;
  tmg_wire_link* curp;
  tmg_wire_link* endp;
  tmg_wire_link** V;

 public:
  tmg_wire_link_pool();
  ~tmg_wire_link_pool();
  void init();
  tmg_wire_link* get();
};

tmg_wire_link_pool::tmg_wire_link_pool()
{
  V = (tmg_wire_link**) malloc(4096 * sizeof(tmg_wire_link*));
  blkSize = 4096;
  V[0] = (tmg_wire_link*) malloc(blkSize * sizeof(tmg_wire_link));
  blkN = 1;
  curblk = 0;
  curp = V[0];
  endp = V[0] + blkSize;
}

tmg_wire_link_pool::~tmg_wire_link_pool()
{
  int j;
  for (j = 0; j < blkN; j++)
    free(V[j]);
  free(V);
}

void tmg_wire_link_pool::init()
{
  curblk = 0;
  curp = V[0];
  endp = V[0] + blkSize;
}

tmg_wire_link* tmg_wire_link_pool::get()
{
  tmg_wire_link* x = curp++;
  if (curp == endp) {
    curblk++;
    if (curblk == blkN) {
      if (blkN == 4096) {
        notice(0, "overflow in tmg_wire_link_pool!\n");
        return NULL;
      }
      V[blkN++] = (tmg_wire_link*) malloc(blkSize * sizeof(tmg_wire_link));
    }
    curp = V[curblk];
    endp = V[curblk] + blkSize;
  }
  x->next = NULL;
  return x;
}

}  // namespace odb
