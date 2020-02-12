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

#ifndef DB_STA_H
#define DB_STA_H

#include "opendb/db.h"
#include "Sta.hh"

namespace sta {

class dbNetwork;

using odb::dbDatabase;
using odb::dbLib;
using odb::dbNet;
using odb::dbBlock;

class dbSta : public Sta
{
public:
  dbSta();
  void init(Tcl_Interp *tcl_interp,
	    dbDatabase *db);

  dbDatabase *db() { return db_; }
  virtual void makeComponents();
  dbNetwork *getDbNetwork() { return db_network_; }
  void readLefAfter(dbLib *lib);
  void readDefAfter();
  void readDbAfter();
  virtual LibertyLibrary *readLiberty(const char *filename,
				      Corner *corner,
				      const MinMaxAll *min_max,
				      bool infer_latches);

  Slack netSlack(const dbNet *net,
		 const MinMax *min_max);
  using Sta::netSlack;

protected:
  virtual void makeNetwork();
  virtual void makeSdcNetwork();

  dbDatabase *db_;
  dbNetwork *db_network_;
};

dbSta *
makeBlockSta(dbBlock *block);

} // namespace
#endif
