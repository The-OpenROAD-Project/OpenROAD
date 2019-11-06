// OpenStaDB, OpenSTA on OpenDB
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

#include "Machine.hh"
#include "dbSdcNetwork.hh"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "opendb/db.h"

namespace sta {

dbSta::dbSta(dbDatabase *db) :
  Sta(),
  db_(db)
{
}

dbNetwork *
dbSta::dbNetwork()
{
  return dynamic_cast<class dbNetwork *>(network_);
}

// Wrapper to init network db.
void
dbSta::makeComponents()
{
  Sta::makeComponents();
  dbNetwork()->setDb(db_);
}

void
dbSta::makeNetwork()
{
  network_ = new class dbNetwork();
}

void
dbSta::makeSdcNetwork()
{
  sdc_network_ = new dbSdcNetwork(network_);
}

void
dbSta::readDbAfter()
{
  dbNetwork()->readDbAfter();
}

}
