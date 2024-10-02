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
#include <regex>

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
#include "sta/Liberty.hh"
#include "sta/PathExpanded.hh"
#include "sta/PathRef.hh"
#include "sta/PatternMatch.hh"
#include "sta/ReportTcl.hh"
#include "sta/Sdc.hh"
#include "sta/Search.hh"
#include "sta/StaMain.hh"
#include "utl/Logger.h"

////////////////////////////////////////////////////////////////

namespace ord {

using sta::dbSta;

dbSta* makeDbSta()
{
  return new dbSta;
}
}  // namespace ord

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

////////////////////////////////////////////////////////////////

void dbStaState::init(dbSta* sta)
{
  sta_ = sta;
  copyState(sta);
  sta->registerStaState(this);
}

dbStaState::~dbStaState()
{
  if (sta_) {
    sta_->unregisterStaState(this);
  }
}

////////////////////////////////////////////////////////////////

dbSta::~dbSta() = default;

void dbSta::initVars(Tcl_Interp* tcl_interp,
                     odb::dbDatabase* db,
                     utl::Logger* logger)
{
  db_ = db;
  logger_ = logger;
  makeComponents();
  if (tcl_interp) {
    setTclInterp(tcl_interp);
  }
  db_report_->setLogger(logger);
  db_network_->init(db, logger);
  db_cbk_ = std::make_unique<dbStaCbk>(this, logger);
  buffer_use_analyser_ = std::make_unique<BufferUseAnalyser>();
}

void dbSta::updateComponentsState()
{
  Sta::updateComponentsState();
  for (auto& state : sta_states_) {
    state->copyState(this);
  }
}

void dbSta::registerStaState(dbStaState* state)
{
  sta_states_.insert(state);
}

void dbSta::unregisterStaState(dbStaState* state)
{
  sta_states_.erase(state);
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

std::string dbSta::getInstanceTypeText(InstType type)
{
  switch (type) {
    case BLOCK:
      return "Macro";
    case PAD:
      return "Pad";
    case PAD_INPUT:
      return "Input pad";
    case PAD_OUTPUT:
      return "Output pad";
    case PAD_INOUT:
      return "Input/output pad";
    case PAD_POWER:
      return "Power pad";
    case PAD_SPACER:
      return "Pad spacer";
    case PAD_AREAIO:
      return "Area IO";
    case ENDCAP:
      return "Endcap cell";
    case FILL:
      return "Fill cell";
    case TAPCELL:
      return "Tap cell";
    case BUMP:
      return "Bump";
    case COVER:
      return "Cover";
    case ANTENNA:
      return "Antenna cell";
    case TIE:
      return "Tie cell";
    case LEF_OTHER:
      return "Other";
    case STD_CELL:
      return "Standard cell";
    case STD_BUF:
      return "Buffer";
    case STD_BUF_CLK_TREE:
      return "Clock buffer";
    case STD_BUF_TIMING_REPAIR:
      return "Timing Repair Buffer";
    case STD_INV:
      return "Inverter";
    case STD_INV_CLK_TREE:
      return "Clock inverter";
    case STD_INV_TIMING_REPAIR:
      return "Timing Repair inverter";
    case STD_CLOCK_GATE:
      return "Clock gate cell";
    case STD_LEVEL_SHIFT:
      return "Level shifter cell";
    case STD_SEQUENTIAL:
      return "Sequential cell";
    case STD_PHYSICAL:
      return "Generic Physical";
    case STD_COMBINATIONAL:
      return "Multi-Input combinational cell";
    case STD_OTHER:
      return "Other";
  }

  return "Unknown";
}

dbSta::InstType dbSta::getInstanceType(odb::dbInst* inst)
{
  odb::dbMaster* master = inst->getMaster();
  const auto master_type = master->getType();
  const auto source_type = inst->getSourceType();
  if (master->isBlock()) {
    return BLOCK;
  }
  if (master->isPad()) {
    if (master_type == odb::dbMasterType::PAD_INPUT) {
      return PAD_INPUT;
    }
    if (master_type == odb::dbMasterType::PAD_OUTPUT) {
      return PAD_OUTPUT;
    }
    if (master_type == odb::dbMasterType::PAD_INOUT) {
      return PAD_INOUT;
    }
    if (master_type == odb::dbMasterType::PAD_POWER) {
      return PAD_POWER;
    }
    if (master_type == odb::dbMasterType::PAD_SPACER) {
      return PAD_SPACER;
    }
    if (master_type == odb::dbMasterType::PAD_AREAIO) {
      return PAD_AREAIO;
    }
    return PAD;
  }
  if (master->isEndCap()) {
    return ENDCAP;
  }
  if (master->isFiller()) {
    return FILL;
  }
  if (master_type == odb::dbMasterType::CORE_WELLTAP) {
    return TAPCELL;
  }
  if (master->isCover()) {
    if (master_type == odb::dbMasterType::COVER_BUMP) {
      return BUMP;
    }
    return COVER;
  }
  if (master_type == odb::dbMasterType::CORE_ANTENNACELL) {
    return ANTENNA;
  }
  if (master_type == odb::dbMasterType::CORE_TIEHIGH
      || master_type == odb::dbMasterType::CORE_TIELOW) {
    return TIE;
  }
  if (source_type == odb::dbSourceType::DIST) {
    return LEF_OTHER;
  }

  sta::dbNetwork* network = getDbNetwork();
  sta::Cell* cell = network->dbToSta(master);
  if (cell == nullptr) {
    return LEF_OTHER;
  }
  sta::LibertyCell* lib_cell = network->libertyCell(cell);
  if (lib_cell == nullptr) {
    if (master->isCore()) {
      return STD_CELL;
    }
    // default to use overall instance setting if there is no liberty cell and
    // it's not a core cell.
    return STD_OTHER;
  }

  const bool is_inverter = lib_cell->isInverter();
  if (is_inverter || lib_cell->isBuffer()) {
    if (source_type == odb::dbSourceType::TIMING) {
      for (auto* iterm : inst->getITerms()) {
        // look through iterms and check for clock nets
        auto* net = iterm->getNet();
        if (net == nullptr) {
          continue;
        }
        if (net->getSigType() == odb::dbSigType::CLOCK) {
          return is_inverter ? STD_INV_CLK_TREE : STD_BUF_CLK_TREE;
        }
      }
      return is_inverter ? STD_INV_TIMING_REPAIR : STD_BUF_TIMING_REPAIR;
    }
    return is_inverter ? STD_INV : STD_BUF;
  }
  if (lib_cell->isClockGate()) {
    return STD_CLOCK_GATE;
  }
  if (lib_cell->isLevelShifter()) {
    return STD_LEVEL_SHIFT;
  }
  if (lib_cell->hasSequentials()) {
    return STD_SEQUENTIAL;
  }
  if (lib_cell->portCount() == 0) {
    return STD_PHYSICAL;  // generic physical
  }
  // not anything else, so combinational
  return STD_COMBINATIONAL;
}

std::map<dbSta::InstType, dbSta::TypeStats> dbSta::countInstancesByType()
{
  auto insts = db_->getChip()->getBlock()->getInsts();
  std::map<InstType, TypeStats> inst_type_stats;

  for (auto inst : insts) {
    InstType type = getInstanceType(inst);
    auto& stats = inst_type_stats[type];
    stats.count++;
    auto master = inst->getMaster();
    stats.area += master->getArea();
  }
  return inst_type_stats;
}

std::string toLowerCase(std::string str)
{
  std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) {
    return std::tolower(c);
  });
  return str;
}

void dbSta::report_cell_usage(const bool verbose)
{
  auto instances_types = countInstancesByType();
  auto block = db_->getChip()->getBlock();
  auto insts = block->getInsts();
  const int total_usage = insts.size();
  int64_t total_area = 0;
  const double area_to_microns = std::pow(block->getDbUnitsPerMicron(), 2);

  const char* header_format = "{:37} {:>7} {:>10}";
  const char* format = "  {:35} {:>7} {:>10.2f}";
  logger_->report(header_format, "Cell type report:", "Count", "Area");
  for (auto [type, stats] : instances_types) {
    std::string type_name = getInstanceTypeText(type);
    logger_->report(
        format, type_name, stats.count, stats.area / area_to_microns);
    total_area += stats.area;

    std::regex regexp(" |/|-");
    logger_->metric("design__instance__count__class:"
                        + toLowerCase(regex_replace(type_name, regexp, "_")),
                    stats.count);
  }
  logger_->report(format, "Total", total_usage, total_area / area_to_microns);

  if (verbose) {
    logger_->report("\nCell instance report:");
    std::map<dbMaster*, TypeStats> usage_count;
    for (auto inst : insts) {
      auto master = inst->getMaster();
      auto& stats = usage_count[master];
      stats.count++;
      stats.area += master->getArea();
    }
    for (auto [master, stats] : usage_count) {
      logger_->report(
          format, master->getName(), stats.count, stats.area / area_to_microns);
    }
  }
}

BufferUse dbSta::getBufferUse(sta::LibertyCell* buffer)
{
  return buffer_use_analyser_->getBufferUse(buffer);
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

////////////////////////////////////////////////////////////////

BufferUseAnalyser::BufferUseAnalyser()
{
  clkbuf_pattern_
      = std::make_unique<sta::PatternMatch>(".*CLKBUF.*",
                                            /* is_regexp */ true,
                                            /* nocase */ true,
                                            /* Tcl_interp* */ nullptr);
}

BufferUse BufferUseAnalyser::getBufferUse(sta::LibertyCell* buffer)
{
  // is_clock_cell is a custom lib attribute that may not exist,
  // so we also use the name pattern to help
  if (buffer->isClockCell() || clkbuf_pattern_->match(buffer->name())) {
    return CLOCK;
  }

  return DATA;
}

}  // namespace sta
