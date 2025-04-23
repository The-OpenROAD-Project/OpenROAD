// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// dbSta, OpenSTA on OpenDB

// Network edit function flow
// tcl edit cmd -> dbNetwork -> db -> dbStaCbk -> Sta
// Edits from tcl, Sta api and db edits are all supported.

#include "db_sta/dbSta.hh"

#include <tcl.h>

#include <algorithm>  // min
#include <cmath>
#include <fstream>
#include <map>
#include <memory>
#include <mutex>
#include <regex>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "boost/json.hpp"
#include "boost/json/src.hpp"
#include "dbSdcNetwork.hh"
#include "db_sta/MakeDbSta.hh"
#include "db_sta/dbNetwork.hh"
#include "odb/db.h"
#include "ord/OpenRoad.hh"
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
#include "sta/StaMain.hh"
#include "sta/Units.hh"
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

// Helper class for histogram reporting.
class dbStaHistogram
{
 public:
  dbStaHistogram(Sta* sta, dbNetwork* network, Logger* logger);

  // Loads data_ with the slack value for each constrained endpoint.
  void loadSlackData(const MinMax* min_max);
  // Loads data_ with the logic depth for each constrained endpoint.
  void loadLogicDepthData(bool exclude_buffers, bool exclude_inverters);
  // Populates bins_ using the current data_, which must be loaded first.
  void populateHistogramBins(int num_bins);
  // Prints the histogram to the log. Width and precision are used to control
  // the number of digits when displaying each bin's range.
  void reportHistogram(int width, int precision) const;

 private:
  std::vector<float> data_;
  // Bins are defined in bins_ as equally sized windows of width bin_width_
  // starting with smallest value min_val_ at the start of bin 0.
  std::vector<int> bins_;
  float min_val_ = 0.0;
  float bin_width_ = 0.0;
  bool integer_bins_ = false;  // Enforce int bin width (for discrete metrics).

  Sta* sta_;
  dbNetwork* network_;
  Logger* logger_;
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
  void inDbBTermSetSigType(dbBTerm* bterm, const dbSigType& sig_type) override;

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
  db->addObserver(this);
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

dbStaHistogram::dbStaHistogram(Sta* sta, dbNetwork* network, Logger* logger)
    : sta_(sta), network_(network), logger_(logger)
{
}

void dbStaHistogram::loadSlackData(const MinMax* min_max)
{
  data_.clear();
  sta::Unit* time_unit = sta_->units()->timeUnit();
  for (sta::Vertex* vertex : *sta_->endpoints()) {
    float slack = sta_->vertexSlack(vertex, min_max);
    if (slack != sta::INF) {  // Ignore unconstrained paths.
      data_.push_back(time_unit->staToUser(slack));
    }
  }
  integer_bins_ = false;
}

void dbStaHistogram::loadLogicDepthData(bool exclude_buffers,
                                        bool exclude_inverters)
{
  data_.clear();
  sta_->worstSlack(MinMax::max());  // Update timing.
  for (sta::Vertex* vertex : *sta_->endpoints()) {
    int path_length = 0;
    Path* path = sta_->vertexWorstSlackPath(vertex, MinMax::max());
    dbInst* prev_inst = nullptr;  // Used to count only unique OR instances.
    while (path) {
      Pin* pin = path->vertex(sta_)->pin();
      Instance* sta_inst = sta_->cmdNetwork()->instance(pin);
      dbInst* inst = network_->staToDb(sta_inst);
      if (!network_->isTopLevelPort(pin) && inst != prev_inst) {
        prev_inst = inst;
        LibertyCell* lib_cell = network_->libertyCell(inst);
        if (lib_cell && (!exclude_buffers || !lib_cell->isBuffer())
            && (!exclude_inverters || !lib_cell->isInverter())) {
          path_length++;
        }
      }
      path = path->prevPath();
    }
    data_.push_back(path_length);
  }
  integer_bins_ = true;
}

void dbStaHistogram::populateHistogramBins(int num_bins)
{
  if (num_bins <= 0) {
    logger_->error(STA, 70, "The number of bins must be positive.");
    return;
  }
  if (data_.empty()) {
    logger_->error(STA, 71, "No data for the histogram has been loaded.");
    return;
  }
  std::sort(data_.begin(), data_.end());

  // Populate each bin with count.
  bins_.resize(num_bins, 0);
  min_val_ = data_.front();
  bin_width_ = (data_.back() - min_val_) / num_bins;
  if (bin_width_ == 0) {  // Special case for no variation in the data.
    bins_[0] = data_.size();
    return;
  }
  if (integer_bins_) {
    bin_width_ = std::ceil(bin_width_);
  }
  for (const float& val : data_) {
    int bin = static_cast<int>((val - min_val_) / bin_width_);
    if (bin >= num_bins) {  // Special case for val with the maximum value.
      bin = num_bins - 1;
    }
    bins_[bin]++;
  }
}

void dbStaHistogram::reportHistogram(int width, int precision) const
{
  constexpr int max_bin_width = 50;  // Max number of chars to print for a bin.
  if (data_.empty()) {
    logger_->error(STA, 72, "No data for the histogram has been loaded.");
    return;
  }
  const int num_bins = bins_.size();
  const int largest_bin = *std::max_element(bins_.begin(), bins_.end());

  // Print the histogram.
  for (int bin = 0; bin < num_bins; ++bin) {
    const float bin_start = min_val_ + bin * bin_width_;
    const float bin_end = min_val_ + (bin + 1) * bin_width_;
    int bar_length  // Round the bar length to its closest value.
        = (max_bin_width * bins_[bin] + largest_bin / 2) / largest_bin;
    if (bar_length == 0 && bins_[bin] > 0) {
      bar_length = 1;  // Better readability when non-zero bins have a bar.
    }
    logger_->report("[{:>{}.{}f}, {:>{}.{}f}{}: {} ({})",
                    bin_start,
                    width,
                    precision,
                    bin_end,
                    width,
                    precision,
                    // The final bin is also closed from the right.
                    bin == num_bins - 1 ? "]" : ")",
                    std::string(bar_length, '*'),
                    bins_[bin]);
  }
}

void dbSta::reportTimingHistogram(int num_bins, const MinMax* min_max) const
{
  dbStaHistogram histogram(sta_, db_network_, logger_);
  histogram.loadSlackData(min_max);
  histogram.populateHistogramBins(num_bins);
  histogram.reportHistogram(/*width=*/6, /*precision=*/3);
}

void dbSta::reportLogicDepthHistogram(int num_bins,
                                      bool exclude_buffers,
                                      bool exclude_inverters) const
{
  dbStaHistogram histogram(sta_, db_network_, logger_);
  histogram.loadLogicDepthData(exclude_buffers, exclude_inverters);
  histogram.populateHistogramBins(num_bins);
  histogram.reportHistogram(/*width=*/3, /*precision=*/0);
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
