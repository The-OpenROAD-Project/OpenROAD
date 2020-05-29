// StaDb, OpenSTA on OpenDB
// Copyright (c) 2019, Parallax Software, Inc.
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

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

  virtual LibertyLibrary *readLiberty(const char *filename,
				      Corner *corner,
				      const MinMaxAll *min_max,
				      bool infer_latches) override;

  Slack netSlack(const dbNet *net,
		 const MinMax *min_max);
  using Sta::netSlack;

  // From ord::OpenRoad::Observer
  virtual void postReadLef(odb::dbTech* tech, odb::dbLib* library) override;
  virtual void postReadDef(odb::dbBlock* block) override;
  virtual void postReadDb(odb::dbDatabase* db) override;

  // Find clock nets connected by combinational gates from the clock roots. 
  void findClkNets(std::set<dbNet*> &clk_nets);

protected:
  virtual void makeNetwork() override;
  virtual void makeSdcNetwork() override;

  dbDatabase *db_;
  dbNetwork *db_network_;
};

dbSta *
makeBlockSta(dbBlock *block);

} // namespace
