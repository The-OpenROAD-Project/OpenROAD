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
#include "dbMap.h"
#include "dbShape.h"
#include "dbWireCodec.h"
#include "tmg_conn.h"

namespace odb {

//
// not including tmg_db.h, only need tmg_findNet
//

static tmg_conn* _conn = NULL;

void orderWires(utl::Logger* logger, dbBlock* block)
{
  if (_conn == NULL) {
    _conn = new tmg_conn(logger);
  }
  _conn->resetSplitCnt();
  for (auto net : block->getNets()) {
    if (net->getSigType().isSupply() || net->isWireOrdered()) {
      continue;
    }
    _conn->analyzeNet(net);
  }
}

void orderWires(utl::Logger* logger, dbNet* net)
{
  if (_conn == NULL) {
    _conn = new tmg_conn(logger);
  }
  if (net->getSigType().isSupply()) {
    return;
  }
  _conn->resetSplitCnt();
  _conn->analyzeNet(net);
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
