// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// dbSta, OpenSTA on OpenDB

// Network edit function flow
// tcl edit cmd -> dbNetwork -> db -> dbStaCbk -> Sta
// Edits from tcl, Sta api and db edits are all supported.

#include "db_sta/dbSta.hh"

#include <algorithm>  // min
#include <cctype>
#include <cmath>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <regex>
#include <set>
#include <string>
#include <string_view>
#include <vector>

#include "boost/json.hpp"
#include "boost/json/src.hpp"
#include "dbSdcNetwork.hh"
#include "db_sta/dbNetwork.hh"
#include "odb/db.h"
#include "odb/dbBlockCallBackObj.h"
#include "odb/dbObject.h"
#include "odb/dbTypes.h"
#include "sta/ArcDelayCalc.hh"
#include "sta/Clock.hh"
#include "sta/Corner.hh"
#include "sta/Delay.hh"
#include "sta/EquivCells.hh"
#include "sta/Graph.hh"
#include "sta/Liberty.hh"
#include "sta/MinMax.hh"
#include "sta/Network.hh"
#include "sta/NetworkClass.hh"
#include "sta/Parasitics.hh"
#include "sta/ParasiticsClass.hh"
#include "sta/Path.hh"
#include "sta/PortDirection.hh"
#include "sta/ReportTcl.hh"
#include "sta/Sdc.hh"
#include "sta/Sta.hh"
#include "sta/StaMain.hh"
#include "sta/StringUtil.hh"
#include "sta/Transition.hh"
#include "sta/Units.hh"
#include "tcl.h"
#include "utl/Logger.h"
#include "utl/histogram.h"

////////////////////////////////////////////////////////////////

namespace sta {

namespace {
// Holds the usage information of a specific cell which includes (i) name of
// the cell, (ii) number of instances of the cell, and (iii) area of the cell
// in microns^2.
struct CellUsageInfo
{
  std::string name;
  int count = 0;
  double area = 0.0;
};

// Holds a snapshot of cell usage information at a given stage.
struct CellUsageSnapshot
{
  std::string stage;
  std::vector<CellUsageInfo> cells_usage_info;
};

void tag_invoke(boost::json::value_from_tag,
                boost::json::value& json_value,
                CellUsageInfo const& cell_usage_info)
{
  json_value = {{"name", cell_usage_info.name},
                {"count", cell_usage_info.count},
                {"area", cell_usage_info.area}};
}

void tag_invoke(boost::json::value_from_tag,
                boost::json::value& json_value,
                CellUsageSnapshot const& cell_usage_snapshot)
{
  json_value
      = {{"stage", cell_usage_snapshot.stage},
         {"cell_usage_info",
          boost::json::value_from(cell_usage_snapshot.cells_usage_info)}};
}
}  // namespace

using utl::Logger;
using utl::STA;

class dbStaReport : public sta::ReportTcl
{
 public:
  explicit dbStaReport() = default;

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

  // Redirect output to filename until redirectFileEnd is called.
  void redirectFileBegin(const char* filename) override;
  // Redirect append output to filename until redirectFileEnd is called.
  void redirectFileAppendBegin(const char* filename) override;
  void redirectFileEnd() override;
  // Redirect output to a string until redirectStringEnd is called.
  void redirectStringBegin() override;
  const char* redirectStringEnd() override;

 protected:
  void printLine(const char* line, size_t length) override;

  Logger* logger_ = nullptr;
};

class dbStaCbk : public odb::dbBlockCallBackObj
{
 public:
  dbStaCbk(dbSta* sta);
  void setNetwork(dbNetwork* network);
  void inDbInstCreate(odb::dbInst* inst) override;
  void inDbInstDestroy(odb::dbInst* inst) override;
  void inDbModuleCreate(odb::dbModule* module) override;
  void inDbModuleDestroy(odb::dbModule* module) override;
  void inDbInstSwapMasterBefore(odb::dbInst* inst,
                                odb::dbMaster* master) override;
  void inDbInstSwapMasterAfter(odb::dbInst* inst) override;
  void inDbNetDestroy(odb::dbNet* net) override;
  void inDbModNetDestroy(odb::dbModNet* modnet) override;
  void inDbITermPostConnect(odb::dbITerm* iterm) override;
  void inDbITermPreDisconnect(odb::dbITerm* iterm) override;
  void inDbITermDestroy(odb::dbITerm* iterm) override;
  void inDbModITermPostConnect(odb::dbModITerm* moditerm) override;
  void inDbModITermPreDisconnect(odb::dbModITerm* moditerm) override;
  void inDbModITermDestroy(odb::dbModITerm* moditerm) override;
  void inDbBTermPostConnect(odb::dbBTerm* bterm) override;
  void inDbBTermPreDisconnect(odb::dbBTerm* bterm) override;
  void inDbBTermCreate(odb::dbBTerm*) override;
  void inDbBTermDestroy(odb::dbBTerm* bterm) override;
  void inDbBTermSetIoType(odb::dbBTerm* bterm,
                          const odb::dbIoType& io_type) override;
  void inDbBTermSetSigType(odb::dbBTerm* bterm,
                           const odb::dbSigType& sig_type) override;
  void inDbModInstCreate(odb::dbModInst* modinst) override;
  void inDbModInstDestroy(odb::dbModInst* modinst) override;
  void inDbModBTermPostConnect(odb::dbModBTerm* modbterm) override;
  void inDbModBTermPreDisconnect(odb::dbModBTerm* modbterm) override;

 private:
  // for inDbInstSwapMasterBefore/inDbInstSwapMasterAfter
  bool swap_master_arcs_equiv_ = false;

  dbSta* sta_;
  dbNetwork* network_ = nullptr;
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

namespace {
std::once_flag init_sta_flag;
}

dbSta::dbSta(Tcl_Interp* tcl_interp, odb::dbDatabase* db, utl::Logger* logger)
{
  std::call_once(init_sta_flag, []() { sta::initSta(); });
  initVars(tcl_interp, db, logger);
  if (!sta::Sta::sta()) {
    sta::Sta::setSta(this);
  }
}

dbSta::~dbSta()
{
  if (sta::Sta::sta() == this) {
    sta::Sta::setSta(nullptr);
  }
}

void dbSta::initVars(Tcl_Interp* tcl_interp,
                     odb::dbDatabase* db,
                     utl::Logger* logger)
{
  db_ = db;
  db->addObserver(this);
  logger_ = logger;
  makeComponents();
  if (tcl_interp) {
    setTclInterp(tcl_interp);
  }
  db_report_->setLogger(logger);
  db_network_->init(db, logger);
  db_cbk_ = std::make_unique<dbStaCbk>(this);
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

std::unique_ptr<dbSta> dbSta::makeBlockSta(odb::dbBlock* block)
{
  auto clone = std::make_unique<dbSta>(tclInterp(), db_, logger_);
  clone->getDbNetwork()->setBlock(block);
  clone->getDbNetwork()->setDefaultLibertyLibrary(
      network_->defaultLibertyLibrary());
  clone->copyUnits(units());
  return clone;
}

////////////////////////////////////////////////////////////////

void dbSta::makeReport()
{
  db_report_ = new dbStaReport();
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

void dbSta::postReadLef(odb::dbTech* tech, odb::dbLib* library)
{
  if (library) {
    db_network_->readLefAfter(library);
  }
}

void dbSta::postReadDef(odb::dbBlock* block)
{
  // If this is the top block of the main chip:
  if (!block->getParent() && block->getChip() == block->getDb()->getChip()) {
    db_network_->readDefAfter(block);
    db_cbk_->addOwner(block);
    db_cbk_->setNetwork(db_network_);
  }
}

void dbSta::postRead3Dbx(odb::dbChip* chip)
{
  // TODO: we are not ready to do timing on chiplets yet
}

void dbSta::postReadDb(odb::dbDatabase* db)
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

Slack dbSta::netSlack(const odb::dbNet* db_net, const MinMax* min_max)
{
  const Net* net = db_network_->dbToSta(db_net);
  return netSlack(net, min_max);
}

std::set<odb::dbNet*> dbSta::findClkNets()
{
  ensureClkNetwork();
  std::set<odb::dbNet*> clk_nets;
  for (Clock* clk : sdc_->clks()) {
    const PinSet* clk_pins = pins(clk);
    if (clk_pins) {
      for (const Pin* pin : *clk_pins) {
        odb::dbNet* db_net = nullptr;
        sta::dbNetwork* db_network = getDbNetwork();
        db_net = db_network->flatNet(pin);
        if (db_net) {
          clk_nets.insert(db_net);
        }
      }
    }
  }
  return clk_nets;
}

std::set<odb::dbNet*> dbSta::findClkNets(const Clock* clk)
{
  ensureClkNetwork();
  std::set<odb::dbNet*> clk_nets;
  const PinSet* clk_pins = pins(clk);
  if (clk_pins) {
    for (const Pin* pin : *clk_pins) {
      odb::dbNet* db_net = nullptr;
      sta::dbNetwork* db_network = getDbNetwork();
      // hierarchical fix
      if (db_network->hasHierarchy()) {
        db_net = db_network_->flatNet(pin);
        if (db_net) {
          clk_nets.insert(db_net);
        }
      }
      // for backward compatibility with jpeg regression case.
      else {
        Net* net = network_->net(pin);
        if (net) {
          clk_nets.insert(db_network_->staToDb(net));
        }
      }
    }
  }
  return clk_nets;
}

std::string dbSta::getInstanceTypeText(InstType type) const
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

void dbSta::addInstanceByTypeInstance(odb::dbInst* inst,
                                      InstTypeMap& inst_type_stats)
{
  InstType type = getInstanceType(inst);
  auto& stats = inst_type_stats[type];
  stats.count++;
  auto master = inst->getMaster();
  stats.area += master->getArea();
}

void dbSta::countInstancesByType(odb::dbModule* module,
                                 InstTypeMap& inst_type_stats,
                                 std::vector<odb::dbInst*>& insts)
{
  for (auto inst : module->getLeafInsts()) {
    addInstanceByTypeInstance(inst, inst_type_stats);
    insts.push_back(inst);
  }
}

void dbSta::countPhysicalOnlyInstancesByType(InstTypeMap& inst_type_stats,
                                             std::vector<odb::dbInst*>& insts)
{
  odb::dbBlock* block = db_->getChip()->getBlock();
  for (auto inst : block->getInsts()) {
    if (!inst->isPhysicalOnly()) {
      continue;
    }

    addInstanceByTypeInstance(inst, inst_type_stats);
    insts.push_back(inst);
  }
}

static std::string toLowerCase(std::string str)
{
  std::ranges::transform(
      str, str.begin(), [](unsigned char c) { return std::tolower(c); });
  return str;
}

void dbSta::reportCellUsage(odb::dbModule* module,
                            const bool verbose,
                            const char* file_name,
                            const char* stage_name)
{
  InstTypeMap instances_types;
  std::vector<odb::dbInst*> insts;
  countInstancesByType(module, instances_types, insts);
  auto block = db_->getChip()->getBlock();
  const double area_to_microns = std::pow(block->getDbUnitsPerMicron(), 2);

  const char* header_format = "{:37} {:>7} {:>10}";
  const char* format = "  {:35} {:>7} {:>10.2f}";
  if (block->getTopModule() != module) {
    logger_->report("Cell type report for {} ({})",
                    module->getModInst()->getHierarchicalName(),
                    module->getName());
  } else {
    countPhysicalOnlyInstancesByType(instances_types, insts);
  }
  logger_->report(header_format, "Cell type report:", "Count", "Area");

  const std::regex regexp(" |/|-");
  std::string metrics_suffix;
  if (block->getTopModule() != module) {
    metrics_suffix = fmt::format("__in_module:{}", module->getName());
  }

  int total_usage = 0;
  int64_t total_area = 0;
  for (auto [type, stats] : instances_types) {
    total_usage += stats.count;
  }

  for (auto [type, stats] : instances_types) {
    const std::string type_name = getInstanceTypeText(type);
    logger_->report(
        format, type_name, stats.count, stats.area / area_to_microns);
    total_area += stats.area;

    const std::string type_class
        = toLowerCase(regex_replace(type_name, regexp, "_"));
    const std::string metric_suffix = type_class + metrics_suffix;

    logger_->metric("design__instance__count__class:" + metric_suffix,
                    stats.count);
    logger_->metric("design__instance__area__class:" + metric_suffix,
                    stats.area / area_to_microns);
  }
  logger_->metric("design__instance__count" + metrics_suffix, total_usage);
  logger_->metric("design__instance__area" + metrics_suffix,
                  total_area / area_to_microns);
  logger_->report(format, "Total", total_usage, total_area / area_to_microns);

  if (verbose) {
    logger_->report("\nCell instance report:");
    std::map<odb::dbMaster*, TypeStats> usage_count;
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

  std::string file(file_name);
  if (!file.empty()) {
    std::map<std::string, CellUsageInfo> name_to_cell_usage_info;
    for (const odb::dbInst* inst : insts) {
      const std::string& cell_name = inst->getMaster()->getName();
      auto [it, inserted] = name_to_cell_usage_info.insert(
          {cell_name,
           CellUsageInfo{
               .name = cell_name,
               .count = 1,
               .area = inst->getMaster()->getArea() / area_to_microns,
           }});
      if (!inserted) {
        it->second.count++;
      }
    }

    CellUsageSnapshot cell_usage_snapshot{
        .stage = std::string(stage_name),
        .cells_usage_info = std::vector<CellUsageInfo>()};
    cell_usage_snapshot.cells_usage_info.reserve(
        name_to_cell_usage_info.size());
    for (const auto& [cell_name, cell_usage_info] : name_to_cell_usage_info) {
      cell_usage_snapshot.cells_usage_info.push_back(cell_usage_info);
    }
    boost::json::value output = boost::json::value_from(cell_usage_snapshot);

    std::ofstream snapshot;
    snapshot.open(file);
    if (snapshot.fail()) {
      logger_->error(STA, 1001, "Could not open snapshot file {}", file_name);
    } else {
      snapshot << output.as_object();
      snapshot.close();
    }
  }
}

void dbSta::reportTimingHistogram(int num_bins, const MinMax* min_max) const
{
  utl::Histogram<float> histogram(logger_);

  sta::Unit* time_unit = sta_->units()->timeUnit();
  for (sta::Vertex* vertex : *sta_->endpoints()) {
    float slack = sta_->vertexSlack(vertex, min_max);
    if (slack != sta::INF) {  // Ignore unconstrained paths.
      histogram.addData(time_unit->staToUser(slack));
    }
  }

  histogram.generateBins(num_bins);
  histogram.report(/*precision=*/3);
}

void dbSta::reportLogicDepthHistogram(int num_bins,
                                      bool exclude_buffers,
                                      bool exclude_inverters) const
{
  utl::Histogram<int> histogram(logger_);

  sta_->worstSlack(MinMax::max());  // Update timing.
  for (sta::Vertex* vertex : *sta_->endpoints()) {
    int path_length = 0;
    Path* path = sta_->vertexWorstSlackPath(vertex, MinMax::max());
    odb::dbInst* prev_inst
        = nullptr;  // Used to count only unique OR instances.
    while (path) {
      Pin* pin = path->vertex(sta_)->pin();
      Instance* sta_inst = sta_->cmdNetwork()->instance(pin);
      odb::dbInst* inst = db_network_->staToDb(sta_inst);
      if (!network_->isTopLevelPort(pin) && inst != prev_inst) {
        prev_inst = inst;
        LibertyCell* lib_cell = db_network_->libertyCell(inst);
        if (lib_cell && (!exclude_buffers || !lib_cell->isBuffer())
            && (!exclude_inverters || !lib_cell->isInverter())) {
          path_length++;
        }
      }
      path = path->prevPath();
    }
    histogram.addData(path_length);
  }

  histogram.generateBins(num_bins);
  histogram.report();
}

int dbSta::checkSanity()
{
  ensureGraph();
  ensureLevelized();

  int pre_warn_cnt = logger_->getWarningCount();
  checkSanityNetlistConsistency();
  int post_warn_cnt = logger_->getWarningCount();

  return post_warn_cnt - pre_warn_cnt;
}

void dbSta::checkSanityDrvrVertexEdges(const odb::dbObject* term) const
{
  if (term == nullptr) {
    logger_->error(
        utl::STA, 2056, "checkSanityDrvrVertexEdges: input term is null");
    return;
  }

  sta::Pin* pin = db_network_->dbToSta(const_cast<odb::dbObject*>(term));
  if (pin == nullptr) {
    logger_->error(utl::STA,
                   2057,
                   "checkSanityDrvrVertexEdges: failed to convert dbObject to "
                   "sta::Pin for {}",
                   term->getName());
    return;
  }

  checkSanityDrvrVertexEdges(pin);
}

void dbSta::checkSanityDrvrVertexEdges(const Pin* pin) const
{
  if (pin == nullptr || db_network_->isDriver(pin) == false) {
    return;
  }

  sta::Graph* graph = this->graph();
  if (graph == nullptr) {
    return;  // Graph not yet built, skip check
  }
  sta::Vertex* drvr_vertex = graph->pinDrvrVertex(pin);

  if (drvr_vertex == nullptr) {
    logger_->warn(utl::STA,
                  2058,
                  "checkSanityDrvrVertexEdges: could not find driver vertex "
                  "for pin {}",
                  db_network_->pathName(pin));
    return;
  }

  // Store load vertices to check for consistency
  std::set<sta::Vertex*> visited_to_vertices;
  sta::VertexOutEdgeIterator edge_iter(drvr_vertex, graph);
  while (edge_iter.hasNext()) {
    sta::Edge* edge = edge_iter.next();
    sta::Vertex* to_vertex = edge->to(graph);
    visited_to_vertices.insert(to_vertex);
  }

  // Compare with ODB connectivity
  Net* sta_net = network_->net(pin);
  if (sta_net == nullptr) {
    return;
  }

  std::set<const sta::Pin*> odb_loads;
  class ODBLoadVisitor : public PinVisitor
  {
   public:
    std::set<const sta::Pin*>& loads_;
    const Pin* drvr_;
    const Network* network_;
    ODBLoadVisitor(std::set<const sta::Pin*>& loads,
                   const Pin* drvr,
                   const Network* network)
        : loads_(loads), drvr_(drvr), network_(network)
    {
    }

    void operator()(const Pin* load_pin) override
    {
      if (load_pin != nullptr && load_pin != drvr_
          && network_->isLoad(load_pin)) {
        loads_.insert(load_pin);
      }
    }
  };

  ODBLoadVisitor visitor(odb_loads, pin, network_);
  NetSet visited_nets(network_);
  sta_net = db_network_->net(pin);
  if (sta_net) {
    db_network_->visitConnectedPins(sta_net, visitor, visited_nets);
  }

  std::set<const sta::Pin*> sta_loads;
  for (sta::Vertex* to_vertex : visited_to_vertices) {
    const Pin* load_pin = to_vertex->pin();
    if (load_pin != nullptr) {
      sta_loads.insert(load_pin);
    }
  }

  // Loads in ODB must appear in STA edges.
  for (const sta::Pin* odb_load : odb_loads) {
    if (sta_loads.find(odb_load) == sta_loads.end()) {
      logger_->warn(
          utl::STA,
          2301,
          "Inconsistent load: ODB has load '{}' for driver '{}', but STA graph "
          "edge is missing.",
          db_network_->pathName(odb_load),
          db_network_->pathName(pin));
    }
  }
}

void dbSta::checkSanityNetlistConsistency() const
{
  odb::dbBlock* block = db_->getChip()->getBlock();
  if (block == nullptr) {
    return;
  }

  // Check all ITerms
  for (odb::dbITerm* iterm : block->getITerms()) {
    if (iterm->getNet() != nullptr) {
      checkSanityDrvrVertexEdges(iterm);
    }
  }

  // Check BTerms
  for (odb::dbBTerm* bterm : block->getBTerms()) {
    if (bterm->getNet() != nullptr) {
      checkSanityDrvrVertexEdges(bterm);
    }
  }

  // Check ModITerms recursively
  std::function<void(odb::dbModule*)> check_module
      = [&](odb::dbModule* module) {
          if (module == nullptr) {
            return;
          }
          for (odb::dbModInst* mod_inst : module->getModInsts()) {
            for (odb::dbModITerm* moditerm : mod_inst->getModITerms()) {
              Pin* pin = db_network_->dbToSta(moditerm);
              if (pin != nullptr && db_network_->isDriver(pin)) {
                checkSanityDrvrVertexEdges(pin);
              }
            }
            check_module(mod_inst->getMaster());
          }
        };

  check_module(block->getTopModule());
}

void dbSta::checkSanityDrvrVertexEdges() const
{
  // Iterate over all driver vertices in the graph.
  sta::VertexIterator vertex_iter(this->graph());
  while (vertex_iter.hasNext()) {
    sta::Vertex* vertex = vertex_iter.next();
    if (vertex->isDriver(this->network())) {
      checkSanityDrvrVertexEdges(db_network_->staToDb(vertex->pin()));
    }
  }
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
  // do not call `Sta::replaceCell` as sta's before/after hooks are called
  // from db callbacks
  NetworkEdit* network = networkCmdEdit();
  network->replaceCell(inst, to_cell);
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
  logger_->report("{}", line);
}

// Only used by encapsulated Tcl channels, ie puts and command prompt.
size_t dbStaReport::printString(const char* buffer, size_t length)
{
  logger_->reportLiteral(buffer);
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

void dbStaReport::redirectFileBegin(const char* filename)
{
  flush();
  logger_->redirectFileBegin(filename);
}

void dbStaReport::redirectFileAppendBegin(const char* filename)
{
  flush();
  logger_->redirectFileAppendBegin(filename);
}

void dbStaReport::redirectFileEnd()
{
  flush();
  logger_->redirectFileEnd();
}

void dbStaReport::redirectStringBegin()
{
  flush();
  logger_->redirectStringBegin();
}

const char* dbStaReport::redirectStringEnd()
{
  flush();
  const std::string string = logger_->redirectStringEnd();
  return stringPrintTmp("%s", string.c_str());
}

////////////////////////////////////////////////////////////////
//
// OpenDB callbacks to notify OpenSTA of network edits
//
////////////////////////////////////////////////////////////////

dbStaCbk::dbStaCbk(dbSta* sta) : sta_(sta)
{
}

void dbStaCbk::setNetwork(dbNetwork* network)
{
  network_ = network;
}

void dbStaCbk::inDbInstCreate(odb::dbInst* inst)
{
  sta_->makeInstanceAfter(network_->dbToSta(inst));
}

void dbStaCbk::inDbInstDestroy(odb::dbInst* inst)
{
  // This is called after the iterms have been destroyed
  // so it side-steps Sta::deleteInstanceAfter.
  sta_->deleteLeafInstanceBefore(network_->dbToSta(inst));
}

void dbStaCbk::inDbModuleCreate(odb::dbModule* module)
{
  network_->registerHierModule(network_->dbToSta(module));
}

void dbStaCbk::inDbModuleDestroy(odb::dbModule* module)
{
  network_->unregisterHierModule(network_->dbToSta(module));
}

void dbStaCbk::inDbInstSwapMasterBefore(odb::dbInst* inst,
                                        odb::dbMaster* master)
{
  LibertyCell* to_lib_cell = network_->libertyCell(network_->dbToSta(master));
  LibertyCell* from_lib_cell = network_->libertyCell(inst);
  Instance* sta_inst = network_->dbToSta(inst);

  swap_master_arcs_equiv_ = sta::equivCellsArcs(from_lib_cell, to_lib_cell);

  if (swap_master_arcs_equiv_) {
    sta_->replaceEquivCellBefore(sta_inst, to_lib_cell);
  } else {
    sta_->replaceCellBefore(sta_inst, to_lib_cell);
  }
}

void dbStaCbk::inDbInstSwapMasterAfter(odb::dbInst* inst)
{
  Instance* sta_inst = network_->dbToSta(inst);

  if (swap_master_arcs_equiv_) {
    sta_->replaceEquivCellAfter(sta_inst);
  } else {
    sta_->replaceCellAfter(sta_inst);
  }
}

void dbStaCbk::inDbNetDestroy(odb::dbNet* db_net)
{
  Net* net = network_->dbToSta(db_net);
  sta_->deleteNetBefore(net);
  network_->deleteNetBefore(net);
}

void dbStaCbk::inDbModNetDestroy(odb::dbModNet* modnet)
{
  Net* net = network_->dbToSta(modnet);
  network_->deleteNetBefore(net);
}

void dbStaCbk::inDbITermPostConnect(odb::dbITerm* iterm)
{
  Pin* pin = network_->dbToSta(iterm);
  network_->connectPinAfter(pin);
  sta_->connectPinAfter(pin);
}

void dbStaCbk::inDbITermPreDisconnect(odb::dbITerm* iterm)
{
  Pin* pin = network_->dbToSta(iterm);
  sta_->disconnectPinBefore(pin);
  network_->disconnectPinBefore(pin);
}

void dbStaCbk::inDbITermDestroy(odb::dbITerm* iterm)
{
  sta_->deletePinBefore(network_->dbToSta(iterm));
}

void dbStaCbk::inDbModITermPostConnect(odb::dbModITerm* moditerm)
{
  Pin* pin = network_->dbToSta(moditerm);
  network_->connectPinAfter(pin);
  // Connection is made by odb::dbITerm callbacks. Calling this causes problem.
  // sta_->connectPinAfter(pin);
}

void dbStaCbk::inDbModITermPreDisconnect(odb::dbModITerm* moditerm)
{
  Pin* pin = network_->dbToSta(moditerm);
  // Connection is made by odb::dbITerm callbacks. Calling this causes problem.
  // sta_->disconnectPinBefore(pin);
  network_->disconnectPinBefore(pin);
}

void dbStaCbk::inDbModITermDestroy(odb::dbModITerm* moditerm)
{
  sta_->deletePinBefore(network_->dbToSta(moditerm));
}

void dbStaCbk::inDbBTermPostConnect(odb::dbBTerm* bterm)
{
  Pin* pin = network_->dbToSta(bterm);
  network_->connectPinAfter(pin);
  sta_->connectPinAfter(pin);
}

void dbStaCbk::inDbBTermPreDisconnect(odb::dbBTerm* bterm)
{
  Pin* pin = network_->dbToSta(bterm);
  sta_->disconnectPinBefore(pin);
  network_->disconnectPinBefore(pin);
}

void dbStaCbk::inDbBTermCreate(odb::dbBTerm* bterm)
{
  sta_->getDbNetwork()->makeTopPort(bterm);
  Pin* pin = network_->dbToSta(bterm);
  sta_->makePortPinAfter(pin);
}

void dbStaCbk::inDbBTermDestroy(odb::dbBTerm* bterm)
{
  sta_->disconnectPin(network_->dbToSta(bterm));
  // sta::NetworkEdit does not support port removal.
}

void dbStaCbk::inDbBTermSetIoType(odb::dbBTerm* bterm,
                                  const odb::dbIoType& io_type)
{
  sta_->getDbNetwork()->setTopPortDirection(bterm, io_type);
}

void dbStaCbk::inDbBTermSetSigType(odb::dbBTerm* bterm,
                                   const odb::dbSigType& sig_type)
{
  // sta can't handle such changes, see OpenROAD#6025, so just reset the whole
  // thing.
  sta_->networkChanged();
  // The above is insufficient, see OpenROAD#6089, clear the vertex id as a
  // workaround.
  bterm->staSetVertexId(object_id_null);
}

void dbStaCbk::inDbModInstCreate(odb::dbModInst* modinst)
{
  sta_->makeInstanceAfter(network_->dbToSta(modinst));
}

void dbStaCbk::inDbModInstDestroy(odb::dbModInst* modinst)
{
  sta_->deleteInstanceBefore(network_->dbToSta(modinst));
}

void dbStaCbk::inDbModBTermPostConnect(odb::dbModBTerm* modbterm)
{
}

void dbStaCbk::inDbModBTermPreDisconnect(odb::dbModBTerm* modbterm)
{
}

////////////////////////////////////////////////////////////////

sta::LibertyPort* getLibertyScanEnable(const sta::LibertyCell* lib_cell)
{
  sta::LibertyCellPortIterator iter(lib_cell);
  while (iter.hasNext()) {
    sta::LibertyPort* port = iter.next();
    sta::ScanSignalType signal_type = port->scanSignalType();
    if (signal_type == sta::ScanSignalType::enable
        || signal_type == sta::ScanSignalType::enable_inverted) {
      return port;
    }
  }
  return nullptr;
}

sta::LibertyPort* getLibertyScanIn(const sta::LibertyCell* lib_cell)
{
  sta::LibertyCellPortIterator iter(lib_cell);
  while (iter.hasNext()) {
    sta::LibertyPort* port = iter.next();
    sta::ScanSignalType signal_type = port->scanSignalType();
    if (signal_type == sta::ScanSignalType::input
        || signal_type == sta::ScanSignalType::input_inverted) {
      return port;
    }
  }
  return nullptr;
}

sta::LibertyPort* getLibertyScanOut(const sta::LibertyCell* lib_cell)
{
  sta::LibertyCellPortIterator iter(lib_cell);
  while (iter.hasNext()) {
    sta::LibertyPort* port = iter.next();
    sta::ScanSignalType signal_type = port->scanSignalType();
    if (signal_type == sta::ScanSignalType::output
        || signal_type == sta::ScanSignalType::output_inverted) {
      return port;
    }
  }
  return nullptr;
}

void dbSta::dumpModInstPinSlacks(const char* mod_inst_name,
                                 const char* filename,
                                 const MinMax* min_max)
{
  std::ofstream out(filename);
  if (!out) {
    return;
  }

  Instance* sta_inst = network()->findInstance(mod_inst_name);
  if (!sta_inst) {
    out << "Instance " << mod_inst_name << " not found.\n";
    return;
  }

  std::vector<std::string> lines;

  odb::dbInst* db_inst = nullptr;
  odb::dbModInst* db_mod_inst = nullptr;
  db_network_->staToDb(sta_inst, db_inst, db_mod_inst);

  if (db_mod_inst) {
    odb::dbModule* master = db_mod_inst->getMaster();
    for (odb::dbInst* leaf : master->getInsts()) {
      for (odb::dbITerm* iterm : leaf->getITerms()) {
        if (iterm->getSigType().isSupply()) {
          continue;
        }
        Pin* pin = db_network_->dbToSta(iterm);
        float slack = pinSlack(pin, min_max);
        float arrival = 0.0;
        float required = 0.0;

        Graph* g = graph();
        Vertex* vertex = g->pinDrvrVertex(pin);
        if (!vertex) {
          vertex = g->pinLoadVertex(pin);
        }
        if (vertex) {
          float min_s = 1e30;
          for (Corner* corner : *corners()) {
            PathAnalysisPt* path_ap = corner->findPathAnalysisPt(min_max);
            if (!path_ap) {
              continue;
            }
            for (const RiseFall* rf : RiseFall::range()) {
              float s = vertexSlack(vertex, rf, path_ap);
              if (s < min_s) {
                min_s = s;
                arrival = vertexArrival(vertex, rf, path_ap);
                required = vertexRequired(vertex, rf, path_ap);
              }
            }
          }
        }

        float cap = 0.0;
        float res = 0.0;
        Net* net = network()->net(pin);
        if (net) {
          Parasitics* parasitics = this->parasitics();
          Corner* corner = corners()->findCorner(0);

          if (parasitics && corner) {
            const ParasiticAnalysisPt* ap
                = corner->findParasiticAnalysisPt(MinMax::max());
            if (ap) {
              Parasitic* p = parasitics->findParasiticNetwork(net, ap);
              if (p) {
                cap = parasitics->capacitance(p);
                ParasiticResistorSeq resistors = parasitics->resistors(p);
                for (ParasiticResistor* r : resistors) {
                  res += parasitics->value(r);
                }
              }
            }
          }
        }
        std::string pin_name = network()->name(pin);
        lines.push_back(fmt::format("{} {:e} {:e} {:e} {:e} {:e}",
                                    pin_name,
                                    slack,
                                    arrival,
                                    required,
                                    cap,
                                    res));
      }
    }
  }

  std::ranges::sort(lines);
  out << "Pin_Name Slack Arrival Required Capacitance Resistance\n";
  for (const std::string& line : lines) {
    out << line << "\n";
  }
  logger_->report("Dumped sorted pin slacks to {}", filename);
}

void dbSta::dumpModInstGraphConnections(const char* mod_inst_name,
                                        const char* filename)
{
  std::ofstream out(filename);
  if (!out) {
    logger_->warn(
        utl::STA, 203, "Could not open debug file {} for writing.", filename);
    return;
  }

  Instance* sta_inst = network()->findInstance(mod_inst_name);
  if (!sta_inst) {
    out << "Instance " << mod_inst_name << " not found.\n";
    return;
  }

  odb::dbInst* db_inst = nullptr;
  odb::dbModInst* db_mod_inst = nullptr;
  db_network_->staToDb(sta_inst, db_inst, db_mod_inst);

  if (!db_mod_inst) {
    return;  // Only for ModInst for now
  }

  odb::dbModule* master = db_mod_inst->getMaster();
  if (!master) {
    return;
  }

  std::vector<std::string> lines;
  Graph* g = graph();

  auto get_cell_type = [&](Pin* p) -> std::string {
    if (!p) {
      return "NO_PIN";
    }
    if (network()->isTopLevelPort(p)) {
      const PortDirection* dir = network()->direction(p);
      if (dir) {
        if (dir->isInput()) {
          return "PI";
        }
        if (dir->isOutput()) {
          return "PO";
        }
        if (dir->isBidirect()) {
          return "PIO";
        }
      }
      return "PORT";
    }
    Instance* inst = network()->instance(p);
    if (inst) {
      LibertyCell* cell = network()->libertyCell(inst);
      if (cell) {
        return cell->name();
      }
      return "BLOCK";
    }
    return "UNKNOWN";
  };

  // Iterate over all leaf instances inside the hierarchical module
  for (odb::dbInst* inst : master->getInsts()) {
    for (odb::dbITerm* iterm : inst->getITerms()) {
      Pin* pin = db_network_->dbToSta(iterm);
      Vertex* vertex = g->pinDrvrVertex(pin);
      if (!vertex) {
        vertex = g->pinLoadVertex(pin);
      }

      if (!vertex) {
        continue;
      }

      std::string src_cell_name = get_cell_type(pin);
      std::string src_str = fmt::format("{} ({}, V_ID:{}, LV:{})",
                                        network()->name(pin),
                                        src_cell_name,
                                        g->id(vertex),
                                        vertex->level());

      // Outgoing Edges
      sta::VertexOutEdgeIterator out_iter(vertex, g);
      while (out_iter.hasNext()) {
        sta::Edge* edge = out_iter.next();
        Vertex* to = edge->to(g);
        Pin* to_pin = to->pin();

        std::string dst_cell_name = get_cell_type(to_pin);
        std::string dst_pin_name = to_pin ? network()->name(to_pin) : "NO_PIN";

        std::string dst_str = fmt::format("{} ({}, V_ID:{}, LV:{})",
                                          dst_pin_name,
                                          dst_cell_name,
                                          g->id(to),
                                          to->level());

        lines.push_back(fmt::format("{} -> {}", src_str, dst_str));
      }

      // Incoming Edges - Check for external drivers
      sta::VertexInEdgeIterator in_iter(vertex, g);
      while (in_iter.hasNext()) {
        sta::Edge* edge = in_iter.next();
        Vertex* from = edge->from(g);
        Pin* from_pin = from->pin();

        bool is_external = false;
        if (from_pin) {
          std::string_view pin_name = network()->name(from_pin);
          std::string mod_prefix = db_mod_inst->getName();
          mod_prefix += "/";  // e.g., "_202_/"
          if (!pin_name.starts_with(mod_prefix)) {
            is_external = true;
          }
        } else {
          is_external = true;
        }

        if (is_external) {
          std::string from_cell_name = get_cell_type(from_pin);
          std::string from_pin_name
              = from_pin ? network()->name(from_pin) : "NO_PIN";

          std::string from_str = fmt::format("{} ({}, V_ID:{}, LV:{})",
                                             from_pin_name,
                                             from_cell_name,
                                             g->id(from),
                                             from->level());

          // Print as outgoing from the external source
          lines.push_back(fmt::format("{} -> {}", from_str, src_str));
        }
      }
    }
  }

  std::ranges::sort(lines);

  out << "DEBUG GRAPH DUMP for " << db_mod_inst->getName()
      << " (Master: " << master->getName() << ")\n";
  for (const std::string& line : lines) {
    out << line << "\n";
  }
  out.close();
  logger_->report("Dumped sorted STA graph connections to {}", filename);
}

}  // namespace sta
