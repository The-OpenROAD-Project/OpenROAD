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

// dbSta, OpenSTA on OpenDB

// Network edit function flow
// tcl edit cmd -> dbNetwork -> db -> dbStaCbk -> Sta
// Edits from tcl, Sta api and db edits are all supported.

#include "db_sta/dbSta.hh"

#include <tcl.h>

#include <algorithm>  // min
#include <mutex>

#include "AbstractPathRenderer.h"
#include "AbstractPowerDensityDataSource.h"
#include "dbSdcNetwork.hh"
#include "db_sta/MakeDbSta.hh"
#include "db_sta/dbNetwork.hh"
#include "odb/db.h"
#include "ord/OpenRoad.hh"
#include "sta/Bfs.hh"
#include "sta/Clock.hh"
#include "sta/EquivCells.hh"
#include "sta/Graph.hh"
#include "sta/PathExpanded.hh"
#include "sta/PathRef.hh"
#include "sta/ReportTcl.hh"
#include "sta/Sdc.hh"
#include "sta/Search.hh"
#include "sta/StaMain.hh"
#include "utl/Logger.h"

////////////////////////////////////////////////////////////////

namespace sta {

using utl::Logger;
using utl::STA;

class dbStaReport : public sta::ReportTcl
{
 public:
  explicit dbStaReport(bool gui_is_on) : gui_is_on_(gui_is_on) {}

  void setLogger(Logger* logger);
  void warn(int id, const char* fmt, ...) override
      __attribute__((format(printf, 3, 4)));
  void fileWarn(int id,
                const char* filename,
                int line,
                const char* fmt,
                ...) override __attribute__((format(printf, 5, 6)));
  void vfileWarn(int id,
                 const char* filename,
                 int line,
                 const char* fmt,
                 va_list args) override;

  void error(int id, const char* fmt, ...) override
      __attribute__((format(printf, 3, 4)));
  void fileError(int id,
                 const char* filename,
                 int line,
                 const char* fmt,
                 ...) override __attribute__((format(printf, 5, 6)));
  void vfileError(int id,
                  const char* filename,
                  int line,
                  const char* fmt,
                  va_list args) override;

  void critical(int id, const char* fmt, ...) override
      __attribute__((format(printf, 3, 4)));
  size_t printString(const char* buffer, size_t length) override;

 protected:
  void printLine(const char* line, size_t length) override;

  Logger* logger_ = nullptr;

 private:
  // text buffer for tcl puts output when in GUI mode.
  std::string tcl_buffer_;
  bool gui_is_on_;
};

class dbStaCbk : public dbBlockCallBackObj
{
 public:
  dbStaCbk(dbSta* sta, Logger* logger);
  void setNetwork(dbNetwork* network);
  void inDbInstCreate(dbInst* inst) override;
  void inDbInstDestroy(dbInst* inst) override;
  void inDbInstSwapMasterBefore(dbInst* inst, dbMaster* master) override;
  void inDbInstSwapMasterAfter(dbInst* inst) override;
  void inDbNetDestroy(dbNet* net) override;
  void inDbITermPostConnect(dbITerm* iterm) override;
  void inDbITermPreDisconnect(dbITerm* iterm) override;
  void inDbITermDestroy(dbITerm* iterm) override;
  void inDbBTermPostConnect(dbBTerm* bterm) override;
  void inDbBTermPreDisconnect(dbBTerm* bterm) override;
  void inDbBTermCreate(dbBTerm*) override;
  void inDbBTermDestroy(dbBTerm* bterm) override;
  void inDbBTermSetIoType(dbBTerm* bterm, const dbIoType& io_type) override;

 private:
  dbSta* sta_;
  dbNetwork* network_ = nullptr;
  Logger* logger_;
};

dbSta::~dbSta() = default;

void dbSta::initVars(Tcl_Interp* tcl_interp,
                     odb::dbDatabase* db,
                     utl::Logger* logger)
{
  db_ = db;
  logger_ = logger;
  makeComponents();
  setTclInterp(tcl_interp);
  db_report_->setLogger(logger);
  db_network_->init(db, logger);
  db_cbk_ = new dbStaCbk(this, logger);
}

void dbSta::setPathRenderer(std::unique_ptr<AbstractPathRenderer> path_renderer)
{
  path_renderer_ = std::move(path_renderer);
}

void dbSta::setPowerDensityDataSource(
    std::unique_ptr<AbstractPowerDensityDataSource> power_density_data_source)
{
  power_density_data_source_ = std::move(power_density_data_source);
}

std::unique_ptr<dbSta> dbSta::makeBlockSta(odb::dbBlock* block)
{
  auto clone = std::make_unique<dbSta>();
  clone->makeComponents();
  clone->initVars(tclInterp(), db_, logger_);
  clone->getDbNetwork()->setBlock(block);
  clone->getDbNetwork()->setDefaultLibertyLibrary(
      network_->defaultLibertyLibrary());
  clone->copyUnits(units());
  return clone;
}

////////////////////////////////////////////////////////////////

void dbSta::makeReport()
{
  db_report_ = new dbStaReport(/*gui_is_on=*/path_renderer_ != nullptr);
  report_ = db_report_;
}

void dbSta::makeNetwork()
{
  db_network_ = new class dbNetwork();
  network_ = db_network_;
}

void dbSta::makeSdcNetwork()
{
  sdc_network_ = new dbSdcNetwork(network_);
}

void dbSta::postReadLef(dbTech* tech, dbLib* library)
{
  if (library) {
    db_network_->readLefAfter(library);
  }
}

void dbSta::postReadDef(dbBlock* block)
{
  if (!block->getParent()) {
    db_network_->readDefAfter(block);
    db_cbk_->addOwner(block);
    db_cbk_->setNetwork(db_network_);
  }
}

void dbSta::postReadDb(dbDatabase* db)
{
  db_network_->readDbAfter(db);
  odb::dbChip* chip = db_->getChip();
  if (chip) {
    odb::dbBlock* block = chip->getBlock();
    if (block) {
      db_cbk_->addOwner(block);
      db_cbk_->setNetwork(db_network_);
    }
  }
}

Slack dbSta::netSlack(const dbNet* db_net, const MinMax* min_max)
{
  const Net* net = db_network_->dbToSta(db_net);
  return netSlack(net, min_max);
}

std::set<dbNet*> dbSta::findClkNets()
{
  ensureClkNetwork();
  std::set<dbNet*> clk_nets;
  for (Clock* clk : sdc_->clks()) {
    const PinSet* clk_pins = pins(clk);
    if (clk_pins) {
      for (const Pin* pin : *clk_pins) {
        Net* net = network_->net(pin);
        if (net) {
          clk_nets.insert(db_network_->staToDb(net));
        }
      }
    }
  }
  return clk_nets;
}

std::set<dbNet*> dbSta::findClkNets(const Clock* clk)
{
  ensureClkNetwork();
  std::set<dbNet*> clk_nets;
  const PinSet* clk_pins = pins(clk);
  if (clk_pins) {
    for (const Pin* pin : *clk_pins) {
      Net* net = network_->net(pin);
      if (net) {
        clk_nets.insert(db_network_->staToDb(net));
      }
    }
  }
  return clk_nets;
}

////////////////////////////////////////////////////////////////

// Network edit functions.
// These override the default sta functions that call sta before/after
// functions because the db calls them via callbacks.

void dbSta::deleteInstance(Instance* inst)
{
  NetworkEdit* network = networkCmdEdit();
  network->deleteInstance(inst);
}

void dbSta::replaceCell(Instance* inst, Cell* to_cell, LibertyCell* to_lib_cell)
{
  NetworkEdit* network = networkCmdEdit();
  LibertyCell* from_lib_cell = network->libertyCell(inst);
  if (sta::equivCells(from_lib_cell, to_lib_cell)) {
    replaceEquivCellBefore(inst, to_lib_cell);
    network->replaceCell(inst, to_cell);
    replaceEquivCellAfter(inst);
  } else {
    replaceCellBefore(inst, to_lib_cell);
    network->replaceCell(inst, to_cell);
    replaceCellAfter(inst);
  }
}

void dbSta::deleteNet(Net* net)
{
  NetworkEdit* network = networkCmdEdit();
  network->deleteNet(net);
}

void dbSta::connectPin(Instance* inst, Port* port, Net* net)
{
  NetworkEdit* network = networkCmdEdit();
  network->connect(inst, port, net);
}

void dbSta::connectPin(Instance* inst, LibertyPort* port, Net* net)
{
  NetworkEdit* network = networkCmdEdit();
  network->connect(inst, port, net);
}

void dbSta::disconnectPin(Pin* pin)
{
  NetworkEdit* network = networkCmdEdit();
  network->disconnectPin(pin);
}

////////////////////////////////////////////////////////////////

void dbStaReport::setLogger(Logger* logger)
{
  logger_ = logger;
}

// Line return \n is implicit.
void dbStaReport::printLine(const char* line, size_t length)
{
  if (redirect_to_string_) {
    redirectStringPrint(line, length);
    redirectStringPrint("\n", 1);
    return;
  }
  if (redirect_stream_) {
    fwrite(line, sizeof(char), length, redirect_stream_);
    fwrite("\n", sizeof(char), 1, redirect_stream_);
    return;
  }

  logger_->report("{}", line);
}

// Only used by encapsulated Tcl channels, ie puts and command prompt.
size_t dbStaReport::printString(const char* buffer, size_t length)
{
  if (redirect_to_string_) {
    redirectStringPrint(buffer, length);
    return length;
  }
  if (redirect_stream_) {
    size_t ret = fwrite(buffer, sizeof(char), length, redirect_stream_);
    return std::min(ret, length);
  }

  // prepend saved buffer
  string buf = tcl_buffer_ + string(buffer);
  tcl_buffer_.clear();  // clear buffer

  if (buffer[length - 1] != '\n') {
    // does not end with a newline, so might need to buffer the information

    auto last_newline = buf.find_last_of('\n');
    if (last_newline == string::npos) {
      // no newlines found, so add entire buf to tcl_buffer_
      tcl_buffer_ = buf;
      buf.clear();
    } else {
      // save partial line to buffer
      tcl_buffer_ = buf.substr(last_newline + 1);
      buf = buf.substr(0, last_newline + 1);
    }
  }

  if (!buf.empty()) {
    // Trim trailing \r\n.
    buf.erase(buf.find_last_not_of("\r\n") + 1);
    logger_->report("{}", buf.c_str());
  }

  // if gui enabled, keep tcl_buffer_ until a newline appears
  // otherwise proceed to print directly to console
  if (!gui_is_on_) {
    // puts without a trailing \n in the string.
    // Tcl command prompts get here.
    // puts "xyz" makes a separate call for the '\n '.
    // This seems to be the only way to get the output.
    // It will not be logged.
    printConsole(tcl_buffer_.c_str(), tcl_buffer_.length());
    tcl_buffer_.clear();
  }

  return length;
}

void dbStaReport::warn(int id, const char* fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  std::unique_lock<std::mutex> lock(buffer_lock_);
  printToBuffer(fmt, args);
  // Don't give std::format a chance to interpret the message.
  logger_->warn(STA, id, "{}", buffer_);
  va_end(args);
}

void dbStaReport::fileWarn(int id,
                           const char* filename,
                           int line,
                           const char* fmt,
                           ...)
{
  va_list args;
  va_start(args, fmt);
  std::unique_lock<std::mutex> lock(buffer_lock_);
  printToBuffer("%s line %d, ", filename, line);
  printToBufferAppend(fmt, args);
  // Don't give std::format a chance to interpret the message.
  logger_->warn(STA, id, "{}", buffer_);
  va_end(args);
}

void dbStaReport::vfileWarn(int id,
                            const char* filename,
                            int line,
                            const char* fmt,
                            va_list args)
{
  printToBuffer("%s line %d, ", filename, line);
  printToBufferAppend(fmt, args);
  // Don't give std::format a chance to interpret the message.
  logger_->warn(STA, id, "{}", buffer_);
}

void dbStaReport::error(int id, const char* fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  std::unique_lock<std::mutex> lock(buffer_lock_);
  printToBuffer(fmt, args);
  // Don't give std::format a chance to interpret the message.
  logger_->error(STA, id, buffer_);
  va_end(args);
}

void dbStaReport::fileError(int id,
                            const char* filename,
                            int line,
                            const char* fmt,
                            ...)
{
  va_list args;
  va_start(args, fmt);
  std::unique_lock<std::mutex> lock(buffer_lock_);
  printToBuffer("%s line %d, ", filename, line);
  printToBufferAppend(fmt, args);
  // Don't give std::format a chance to interpret the message.
  logger_->error(STA, id, "{}", buffer_);
  va_end(args);
}

void dbStaReport::vfileError(int id,
                             const char* filename,
                             int line,
                             const char* fmt,
                             va_list args)
{
  printToBuffer("%s line %d, ", filename, line);
  printToBufferAppend(fmt, args);
  // Don't give std::format a chance to interpret the message.
  logger_->error(STA, id, "{}", buffer_);
}

void dbStaReport::critical(int id, const char* fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  std::unique_lock<std::mutex> lock(buffer_lock_);
  printToBuffer(fmt, args);
  // Don't give std::format a chance to interpret the message.
  logger_->critical(STA, id, "{}", buffer_);
  va_end(args);
}

////////////////////////////////////////////////////////////////
//
// OpenDB callbacks to notify OpenSTA of network edits
//
////////////////////////////////////////////////////////////////

dbStaCbk::dbStaCbk(dbSta* sta, Logger* logger) : sta_(sta), logger_(logger)
{
}

void dbStaCbk::setNetwork(dbNetwork* network)
{
  network_ = network;
}

void dbStaCbk::inDbInstCreate(dbInst* inst)
{
  sta_->makeInstanceAfter(network_->dbToSta(inst));
}

void dbStaCbk::inDbInstDestroy(dbInst* inst)
{
  // This is called after the iterms have been destroyed
  // so it side-steps Sta::deleteInstanceAfter.
  sta_->deleteLeafInstanceBefore(network_->dbToSta(inst));
}

void dbStaCbk::inDbInstSwapMasterBefore(dbInst* inst, dbMaster* master)
{
  LibertyCell* to_lib_cell = network_->libertyCell(network_->dbToSta(master));
  LibertyCell* from_lib_cell = network_->libertyCell(inst);
  Instance* sta_inst = network_->dbToSta(inst);
  if (sta::equivCells(from_lib_cell, to_lib_cell)) {
    sta_->replaceEquivCellBefore(sta_inst, to_lib_cell);
  } else {
    logger_->error(STA,
                   1000,
                   "instance {} swap master {} is not equivalent",
                   inst->getConstName(),
                   master->getConstName());
  }
}

void dbStaCbk::inDbInstSwapMasterAfter(dbInst* inst)
{
  sta_->replaceEquivCellAfter(network_->dbToSta(inst));
}

void dbStaCbk::inDbNetDestroy(dbNet* db_net)
{
  Net* net = network_->dbToSta(db_net);
  sta_->deleteNetBefore(net);
  network_->deleteNetBefore(net);
}

void dbStaCbk::inDbITermPostConnect(dbITerm* iterm)
{
  Pin* pin = network_->dbToSta(iterm);
  network_->connectPinAfter(pin);
  sta_->connectPinAfter(pin);
}

void dbStaCbk::inDbITermPreDisconnect(dbITerm* iterm)
{
  Pin* pin = network_->dbToSta(iterm);
  sta_->disconnectPinBefore(pin);
  network_->disconnectPinBefore(pin);
}

void dbStaCbk::inDbITermDestroy(dbITerm* iterm)
{
  sta_->deletePinBefore(network_->dbToSta(iterm));
}

void dbStaCbk::inDbBTermPostConnect(dbBTerm* bterm)
{
  Pin* pin = network_->dbToSta(bterm);
  network_->connectPinAfter(pin);
  sta_->connectPinAfter(pin);
}

void dbStaCbk::inDbBTermPreDisconnect(dbBTerm* bterm)
{
  Pin* pin = network_->dbToSta(bterm);
  sta_->disconnectPinBefore(pin);
  network_->disconnectPinBefore(pin);
}

void dbStaCbk::inDbBTermCreate(dbBTerm* bterm)
{
  sta_->getDbNetwork()->makeTopPort(bterm);
  Pin* pin = network_->dbToSta(bterm);
  sta_->makePortPinAfter(pin);
}

void dbStaCbk::inDbBTermDestroy(dbBTerm* bterm)
{
  sta_->disconnectPin(network_->dbToSta(bterm));
  // sta::NetworkEdit does not support port removal.
}

void dbStaCbk::inDbBTermSetIoType(dbBTerm* bterm, const dbIoType& io_type)
{
  sta_->getDbNetwork()->setTopPortDirection(bterm, io_type);
}

////////////////////////////////////////////////////////////////

// Highlight path in the gui.
void dbSta::highlight(PathRef* path)
{
  path_renderer_->highlight(path);
}

}  // namespace sta
