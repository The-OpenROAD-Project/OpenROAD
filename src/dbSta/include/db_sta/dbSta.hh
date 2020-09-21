/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2019, OpenROAD
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

#include "opendb/db.h"
#include "sta/Sta.hh"
#include "openroad/OpenRoad.hh"

namespace sta {

class dbNetwork;

using odb::dbDatabase;
using odb::dbLib;
using odb::dbNet;
using odb::dbBlock;
using odb::dbTech;
using odb::dbLib;

class dbSta : public Sta, public ord::OpenRoad::Observer
{
public:
  dbSta();
  void init(Tcl_Interp *tcl_interp,
	    dbDatabase *db);

  dbDatabase *db() { return db_; }
  virtual void makeComponents() override;
  dbNetwork *getDbNetwork() { return db_network_; }

  Slack netSlack(const dbNet *net,
		 const MinMax *min_max);

  // From ord::OpenRoad::Observer
  virtual void postReadLef(odb::dbTech* tech, odb::dbLib* library) override;
  virtual void postReadDef(odb::dbBlock* block) override;
  virtual void postReadDb(odb::dbDatabase* db) override;

  // Find clock nets connected by combinational gates from the clock roots. 
  void findClkNets(// Return value
		   std::set<dbNet*> &clk_nets);
  void findClkNets(const Clock *clk,
		   // Return value
		   std::set<dbNet*> &clk_nets);

  using Sta::netSlack;
  using Sta::isClock;

protected:
  virtual void makeNetwork() override;
  virtual void makeSdcNetwork() override;

  dbDatabase *db_;
  dbNetwork *db_network_;
};

dbSta *
makeBlockSta(dbBlock *block);

} // namespace
