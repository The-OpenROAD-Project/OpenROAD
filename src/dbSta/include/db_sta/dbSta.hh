/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2019, The Regents of the University of California
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

#include "odb/db.h"
#include "odb/dbBlockCallBackObj.h"
#include "sta/Sta.hh"
#include "ord/OpenRoad.hh"

namespace gui {
class Gui;
}

namespace ord {
class OpenRoad;
}

namespace utl {
class Logger;
}

namespace sta {

class dbSta;
class dbNetwork;
class dbStaReport;
class dbStaCbk;
class PathRenderer;

using ord::OpenRoad;
using utl::Logger;

using odb::dbDatabase;
using odb::dbLib;
using odb::dbNet;
using odb::dbInst;
using odb::dbITerm;
using odb::dbBTerm;
using odb::dbBlock;
using odb::dbTech;
using odb::dbLib;
using odb::dbMaster;
using odb::dbBlockCallBackObj;

class dbSta : public Sta, public ord::OpenRoad::Observer
{
public:
  dbSta();
  virtual ~dbSta();
  void init(Tcl_Interp *tcl_interp,
	    dbDatabase *db,
            gui::Gui *gui,
            Logger *logger);

  dbDatabase *db() { return db_; }
  virtual void makeComponents() override;
  dbNetwork *getDbNetwork() { return db_network_; }
  dbStaReport *getDbReport() { return db_report_; }

  Slack netSlack(const dbNet *net,
		 const MinMax *min_max);

  // From ord::OpenRoad::Observer
  virtual void postReadLef(odb::dbTech* tech, odb::dbLib* library) override;
  virtual void postReadDef(odb::dbBlock* block) override;
  virtual void postReadDb(odb::dbDatabase* db) override;

  // Find clock nets connected by combinational gates from the clock roots. 
  std::set<dbNet*> findClkNets();
  std::set<dbNet*> findClkNets(const Clock *clk);

  virtual void deleteInstance(Instance *inst) override;
  virtual void deleteNet(Net *net) override;
  virtual void connectPin(Instance *inst,
			  Port *port,
			  Net *net) override;
  virtual void connectPin(Instance *inst,
			  LibertyPort *port,
			  Net *net) override;
  virtual void disconnectPin(Pin *pin) override;
  // Highlight path in the gui.
  void highlight(PathRef *path);

  using Sta::netSlack;
  using Sta::replaceCell;

protected:
  virtual void makeReport() override;
  virtual void makeNetwork() override;
  virtual void makeSdcNetwork() override;

  virtual void replaceCell(Instance *inst,
                           Cell *to_cell,
                           LibertyCell *to_lib_cell) override;

  dbDatabase *db_;
  gui::Gui *gui_;
  Logger *logger_;

  dbNetwork *db_network_;
  dbStaReport *db_report_;
  dbStaCbk *db_cbk_;
  PathRenderer *path_renderer_;
};

// Make a stand-alone (scratchpad) sta for block.
dbSta *
makeBlockSta(OpenRoad *openroad,
             dbBlock *block);

} // namespace
