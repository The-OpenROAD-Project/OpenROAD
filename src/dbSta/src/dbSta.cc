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
#include <fstream>
#include <map>
#include <memory>
#include <mutex>
#include <regex>
#include <set>
#include <string>
#include <vector>

#include "boost/json.hpp"
#include "boost/json/src.hpp"
#include "dbSdcNetwork.hh"
#include "db_sta/dbNetwork.hh"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "sta/Clock.hh"
#include "sta/Delay.hh"
#include "sta/EquivCells.hh"
#include "sta/Graph.hh"
#include "sta/Liberty.hh"
#include "sta/MinMax.hh"
#include "sta/NetworkClass.hh"
#include "sta/Path.hh"
#include "sta/PatternMatch.hh"
#include "sta/ReportTcl.hh"
#include "sta/Sdc.hh"
#include "sta/Sta.hh"
#include "sta/StaMain.hh"
#include "sta/Units.hh"
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

class dbStaCbk : public dbBlockCallBackObj
{
 public:
  dbStaCbk(dbSta* sta);
  void setNetwork(dbNetwork* network);
  void inDbInstCreate(dbInst* inst) override;
  void inDbInstDestroy(dbInst* inst) override;
  void inDbModuleCreate(dbModule* module) override;
  void inDbModuleDestroy(dbModule* module) override;
  void inDbInstSwapMasterBefore(dbInst* inst, dbMaster* master) override;
  void inDbInstSwapMasterAfter(dbInst* inst) override;
  void inDbNetDestroy(dbNet* net) override;
  void inDbModNetDestroy(dbModNet* modnet) override;
  void inDbITermPostConnect(dbITerm* iterm) override;
  void inDbITermPreDisconnect(dbITerm* iterm) override;
  void inDbITermDestroy(dbITerm* iterm) override;
  void inDbModITermPostConnect(dbModITerm* moditerm) override;
  void inDbModITermPreDisconnect(dbModITerm* moditerm) override;
  void inDbModITermDestroy(dbModITerm* moditerm) override;
  void inDbBTermPostConnect(dbBTerm* bterm) override;
  void inDbBTermPreDisconnect(dbBTerm* bterm) override;
  void inDbBTermCreate(dbBTerm*) override;
  void inDbBTermDestroy(dbBTerm* bterm) override;
  void inDbBTermSetIoType(dbBTerm* bterm, const dbIoType& io_type) override;
  void inDbBTermSetSigType(dbBTerm* bterm, const dbSigType& sig_type) override;

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

void dbSta::postReadLef(dbTech* tech, dbLib* library)
{
  if (library) {
    db_network_->readLefAfter(library);
  }
}

void dbSta::postReadDef(dbBlock* block)
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
        dbNet* db_net = nullptr;
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

std::set<dbNet*> dbSta::findClkNets(const Clock* clk)
{
  ensureClkNetwork();
  std::set<dbNet*> clk_nets;
  const PinSet* clk_pins = pins(clk);
  if (clk_pins) {
    for (const Pin* pin : *clk_pins) {
      dbNet* db_net = nullptr;
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
                                 std::vector<dbInst*>& insts)
{
  for (auto inst : module->getLeafInsts()) {
    addInstanceByTypeInstance(inst, inst_type_stats);
    insts.push_back(inst);
  }
}

void dbSta::countPhysicalOnlyInstancesByType(InstTypeMap& inst_type_stats,
                                             std::vector<dbInst*>& insts)
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

std::string toLowerCase(std::string str)
{
  std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) {
    return std::tolower(c);
  });
  return str;
}

void dbSta::reportCellUsage(odb::dbModule* module,
                            const bool verbose,
                            const char* file_name,
                            const char* stage_name)
{
  InstTypeMap instances_types;
  std::vector<dbInst*> insts;
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

  std::string file(file_name);
  if (!file.empty()) {
    std::map<std::string, CellUsageInfo> name_to_cell_usage_info;
    for (const dbInst* inst : insts) {
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
    dbInst* prev_inst = nullptr;  // Used to count only unique OR instances.
    while (path) {
      Pin* pin = path->vertex(sta_)->pin();
      Instance* sta_inst = sta_->cmdNetwork()->instance(pin);
      dbInst* inst = db_network_->staToDb(sta_inst);
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

  checkSanityDrvrVertexEdges();

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
  if (db_network_->isDriver(pin) == false) {
    return;
  }

  sta::Graph* graph = this->graph();
  sta::Vertex* drvr_vertex = graph->pinDrvrVertex(pin);

  if (drvr_vertex == nullptr) {
    logger_->warn(utl::STA,
                  2058,
                  "checkSanityDrvrVertexEdges: could not find driver vertex "
                  "for pin {}",
                  db_network_->pathName(pin));
    return;
  }

  // Store edges and load vertices to check for consistency
  std::set<std::string> edge_str_set;
  std::set<sta::Vertex*> visited_to_vertices;
  sta::VertexOutEdgeIterator edge_iter(drvr_vertex, graph);
  while (edge_iter.hasNext()) {
    sta::Edge* edge = edge_iter.next();
    sta::Vertex* to_vertex = edge->to(graph);

    // Check duplicate edges
    if (edge_str_set.find(edge->to_string(this)) != edge_str_set.end()) {
      logger_->error(utl::STA,
                     2059,
                     "Duplicate edge found: {}",
                     edge->to_string(this).c_str());
    }

    edge_str_set.insert(edge->to_string(this));
    visited_to_vertices.insert(to_vertex);
  }

  // Compare with ODB connectivity
  bool has_inconsistency = false;
  odb::dbObject* drvr_obj = db_network_->staToDb(pin);
  odb::dbNet* net = nullptr;
  if (drvr_obj) {
    if (drvr_obj->getObjectType() == odb::dbITermObj) {
      net = static_cast<odb::dbITerm*>(drvr_obj)->getNet();
    } else if (drvr_obj->getObjectType() == odb::dbBTermObj) {
      net = static_cast<odb::dbBTerm*>(drvr_obj)->getNet();
    }
  }

  if (net) {
    std::set<sta::Pin*> odb_loads;
    for (odb::dbITerm* iterm : net->getITerms()) {
      if (iterm != drvr_obj) {
        odb_loads.insert(db_network_->dbToSta(iterm));
      }
    }
    for (odb::dbBTerm* bterm : net->getBTerms()) {
      if (bterm != drvr_obj) {
        odb_loads.insert(db_network_->dbToSta(bterm));
      }
    }

    std::set<sta::Pin*> sta_loads;
    for (sta::Vertex* to_vertex : visited_to_vertices) {
      sta_loads.insert(to_vertex->pin());
    }

    // Loads in ODB must appear in STA edges.
    // - STA can have more edges than loads in ODB
    for (sta::Pin* odb_load : odb_loads) {
      if (sta_loads.find(odb_load) == sta_loads.end()) {
        logger_->warn(utl::STA,
                      2063,
                      "Inconsistent load: ODB has load '{}', but STA does not.",
                      db_network_->pathName(odb_load));
        has_inconsistency = true;
      }
    }
  }

  if (has_inconsistency) {
    logger_->error(utl::STA,
                   2064,
                   "Inconsistencies found in driver vertex edges for pin {}.",
                   db_network_->pathName(pin));
  }
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

void dbStaCbk::inDbModuleCreate(dbModule* module)
{
  network_->registerHierModule(network_->dbToSta(module));
}

void dbStaCbk::inDbModuleDestroy(dbModule* module)
{
  network_->unregisterHierModule(network_->dbToSta(module));
}

void dbStaCbk::inDbInstSwapMasterBefore(dbInst* inst, dbMaster* master)
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

void dbStaCbk::inDbInstSwapMasterAfter(dbInst* inst)
{
  Instance* sta_inst = network_->dbToSta(inst);

  if (swap_master_arcs_equiv_) {
    sta_->replaceEquivCellAfter(sta_inst);
  } else {
    sta_->replaceCellAfter(sta_inst);
  }
}

void dbStaCbk::inDbNetDestroy(dbNet* db_net)
{
  Net* net = network_->dbToSta(db_net);
  sta_->deleteNetBefore(net);
  network_->deleteNetBefore(net);
}

void dbStaCbk::inDbModNetDestroy(dbModNet* modnet)
{
  Net* net = network_->dbToSta(modnet);
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

void dbStaCbk::inDbModITermPostConnect(dbModITerm* moditerm)
{
  Pin* pin = network_->dbToSta(moditerm);
  network_->connectPinAfter(pin);
}

void dbStaCbk::inDbModITermPreDisconnect(dbModITerm* moditerm)
{
  Pin* pin = network_->dbToSta(moditerm);
  network_->disconnectPinBefore(pin);
}

void dbStaCbk::inDbModITermDestroy(dbModITerm* moditerm)
{
  sta_->deletePinBefore(network_->dbToSta(moditerm));
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

void dbStaCbk::inDbBTermSetSigType(dbBTerm* bterm, const dbSigType& sig_type)
{
  // sta can't handle such changes, see OpenROAD#6025, so just reset the whole
  // thing.
  sta_->networkChanged();
  // The above is insufficient, see OpenROAD#6089, clear the vertex id as a
  // workaround.
  bterm->staSetVertexId(object_id_null);
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

}  // namespace sta
