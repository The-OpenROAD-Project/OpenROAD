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

#include "dbPrintControl.h"

#include "db.h"
#include "dbBlock.h"
#include "dbDatabase.h"
#include "dbInst.h"
#include "dbNet.h"

namespace odb {

dbPrintControl::dbPrintControl()
{
  _groupCnt   = (uint) dbPrintControl::EXTRACT;
  _targetNet  = (Ath__array1D<dbNet*>**) calloc(sizeof(Ath__array1D<dbNet*>*),
                                               _groupCnt);
  _targetInst = (Ath__array1D<dbInst*>**) calloc(sizeof(Ath__array1D<dbInst*>*),
                                                 _groupCnt);
  for (uint ii = 0; ii < _groupCnt; ii++) {
    _targetNet[ii]  = new Ath__array1D<dbNet*>;
    _targetInst[ii] = new Ath__array1D<dbInst*>;
  }
  _printCnt = 1;
}

dbPrintControl::~dbPrintControl()
{
  for (uint ii = 0; ii < _groupCnt; ii++) {
    delete _targetNet[ii];
    delete _targetInst[ii];
  }
  free(_targetNet);
  free(_targetInst);
}

void dbPrintControl::setPrintControl(dbBlock*    block,
                                     const char* name,
                                     const char* netn,
                                     const char* instn)
{
  Type ptype;
  if (strcmp(name, "EXTTREE") == 0)
    ptype = EXTTREE;
  if (strcmp(name, "EXTRACT") == 0)
    ptype = EXTRACT;
  std::vector<dbNet*> pnets;
  block->findSomeNet(netn, pnets);
  Ath__array1D<dbNet*>* tnet = _targetNet[(uint) ptype];
  tnet->resetCnt();
  for (uint ii = 0; ii < pnets.size(); ii++)
    tnet->add(pnets[ii]);
  std::vector<dbInst*> pinsts;
  block->findSomeInst(instn, pinsts);
  Ath__array1D<dbInst*>* tinst = _targetInst[(uint) ptype];
  tinst->resetCnt();
  for (uint jj = 0; jj < pinsts.size(); jj++)
    tinst->add(pinsts[jj]);
}

void dbBlock::setPrintControl(const char* name,
                              const char* netn,
                              const char* instn)
{
  dbPrintControl* pcntl = ((_dbBlock*) this)->_printControl;
  pcntl->setPrintControl(this, name, netn, instn);
}

uint dbPrintControl::getPrintCnt(dbPrintControl::Type ptype, dbNet* net)
{
  Ath__array1D<dbNet*>* tnet = _targetNet[(uint) ptype];
  for (uint ii = 0; ii < tnet->getCnt(); ii++) {
    if (tnet->get(ii) == net)
      return _printCnt++;
  }
  return 0;
}

uint dbBlock::getPrintCnt(dbPrintControl::Type ptype, dbNet* net)
{
  dbPrintControl* pcntl = ((_dbBlock*) this)->_printControl;
  uint            count = pcntl->getPrintCnt(ptype, net);
  return count;
}

uint dbNet::getPrintCnt(dbPrintControl::Type type)
{
  dbBlock* block = (dbBlock*) getImpl()->getOwner();
  uint     pcnt  = block->getPrintCnt(type, this);
  return pcnt;
}

}  // namespace odb
