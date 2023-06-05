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

#include <memory>

#include "odb/db.h"
#include "odb/dbBlockCallBackObj.h"
#include "ord/OpenRoadObserver.hh"
#include "sta/Sta.hh"

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
class AbstractPathRenderer;
class AbstractPowerDensityDataSource;

using utl::Logger;

using odb::dbBlock;
using odb::dbBlockCallBackObj;
using odb::dbBTerm;
using odb::dbDatabase;
using odb::dbInst;
using odb::dbITerm;
using odb::dbLib;
using odb::dbMaster;
using odb::dbNet;
using odb::dbTech;

class dbSta : public Sta, public ord::OpenRoadObserver
{
 public:
  ~dbSta() override;

  void initVars(Tcl_Interp* tcl_interp,
                odb::dbDatabase* db,
                utl::Logger* logger);

  void setPathRenderer(std::unique_ptr<AbstractPathRenderer> path_renderer);
  void setPowerDensityDataSource(std::unique_ptr<AbstractPowerDensityDataSource>
                                     power_density_data_source);

  // Creates a dbSta instance for the given dbBlock using the same context as
  // this dbSta instance (e.g. TCL interpreter, units, etc.)
  std::unique_ptr<dbSta> makeBlockSta(odb::dbBlock* block);

  dbDatabase* db() { return db_; }
  dbNetwork* getDbNetwork() { return db_network_; }
  dbStaReport* getDbReport() { return db_report_; }

  Slack netSlack(const dbNet* net, const MinMax* min_max);

  // From ord::OpenRoad::Observer
  void postReadLef(odb::dbTech* tech, odb::dbLib* library) override;
  void postReadDef(odb::dbBlock* block) override;
  void postReadDb(odb::dbDatabase* db) override;

  // Find clock nets connected by combinational gates from the clock roots.
  std::set<dbNet*> findClkNets();
  std::set<dbNet*> findClkNets(const Clock* clk);

  void deleteInstance(Instance* inst) override;
  void deleteNet(Net* net) override;
  void connectPin(Instance* inst, Port* port, Net* net) override;
  void connectPin(Instance* inst, LibertyPort* port, Net* net) override;
  void disconnectPin(Pin* pin) override;

  // Highlight path in the gui.
  void highlight(PathRef* path);

  using Sta::netSlack;
  using Sta::replaceCell;

 private:
  void makeReport() override;
  void makeNetwork() override;
  void makeSdcNetwork() override;

  void replaceCell(Instance* inst,
                   Cell* to_cell,
                   LibertyCell* to_lib_cell) override;

  dbDatabase* db_ = nullptr;
  Logger* logger_ = nullptr;

  dbNetwork* db_network_ = nullptr;
  dbStaReport* db_report_ = nullptr;
  dbStaCbk* db_cbk_ = nullptr;

  std::unique_ptr<AbstractPathRenderer> path_renderer_;
  std::unique_ptr<AbstractPowerDensityDataSource> power_density_data_source_;
};

}  // namespace sta
