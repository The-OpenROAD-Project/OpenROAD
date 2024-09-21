/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2024, Precision Innonvations, Inc.
// All rights reserved.
//
// BSD 3-Clause License
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
//
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include "db_sta/dbNetwork.hh"
#include "odb/db.h"
#include "odb/dbBlockCallBackObj.h"

namespace rsz {

using odb::dbBlockCallBackObj;
using odb::dbInst;
using odb::dbITerm;
using odb::dbNet;
using sta::dbNetwork;
using sta::Network;

class Resizer;

class OdbCallBack : public dbBlockCallBackObj
{
 public:
  OdbCallBack(Resizer* resizer, Network* network, dbNetwork* db_network);

  void inDbInstCreate(dbInst* inst) override;
  void inDbNetCreate(dbNet* net) override;
  void inDbNetDestroy(dbNet* net) override;
  void inDbITermPostConnect(dbITerm* iterm) override;
  void inDbITermPostDisconnect(dbITerm* iterm, dbNet* net) override;
  void inDbInstSwapMasterAfter(dbInst* inst) override;

 private:
  Resizer* resizer_;
  Network* network_;
  dbNetwork* db_network_;
};

}  // namespace rsz
