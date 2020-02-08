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
#ifndef OPENROAD_OPENPHYSYN_LIBRARY_BUILD

#include <OpenPhySyn/Sta/DatabaseSta.hpp>
#include <OpenPhySyn/Sta/DatabaseStaNetwork.hpp>
#include <Sta/DatabaseSdcNetwork.hpp>
#include <tcl.h>
#include "Machine.hh"
#include "StaMain.hh"
#include "opendb/db.h"

namespace sta
{

extern "C"
{
    extern int Sta_Init(Tcl_Interp* interp);
}

extern const char* tcl_inits[];

DatabaseSta::DatabaseSta() : Sta(), db_(nullptr)
{
}
DatabaseSta::DatabaseSta(dbDatabase* db) : Sta(), db_(db)
{
}

void
DatabaseSta::init(Tcl_Interp* tcl_interp, dbDatabase* db)
{
    initSta();
    Sta::setSta(this);
    db_ = db;
    makeComponents();
    setTclInterp(tcl_interp);
    // Define swig TCL commands.
    Sta_Init(tcl_interp);
    // Eval encoded sta TCL sources.
    evalTclInit(tcl_interp, tcl_inits);
    // Import exported commands from sta namespace to global namespace.
    Tcl_Eval(tcl_interp, "sta::define_sta_cmds");
    Tcl_Eval(tcl_interp, "namespace import sta::*");
}

// Wrapper to init network db.
void
DatabaseSta::makeComponents()
{
    Sta::makeComponents();
    db_network_->setDb(db_);
}

void
DatabaseSta::makeNetwork()
{
    db_network_ = new class DatabaseStaNetwork();
    network_    = db_network_;
}

void
DatabaseSta::makeSdcNetwork()
{
    sdc_network_ = new DatabaseSdcNetwork(network_);
}

void
DatabaseSta::readLefAfter(dbLib* lib)
{
    db_network_->readLefAfter(lib);
}

void
DatabaseSta::readDefAfter()
{
    db_network_->readDefAfter();
}

void
DatabaseSta::readDbAfter()
{
    db_network_->readDbAfter();
}

// Wrapper to sync db/liberty libraries.
LibertyLibrary*
DatabaseSta::readLiberty(const char* filename, Corner* corner,
                         const MinMaxAll* min_max, bool infer_latches)

{
    LibertyLibrary* lib =
        Sta::readLiberty(filename, corner, min_max, infer_latches);
    db_network_->readLibertyAfter(lib);
    return lib;
}

Slack
DatabaseSta::netSlack(const dbNet* db_net, const MinMax* min_max)
{
    const Net* net = db_network_->dbToSta(db_net);
    return netSlack(net, min_max);
}

} // namespace sta
#endif