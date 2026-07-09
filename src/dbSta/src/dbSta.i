// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// clang-format off

%{

#include <iomanip>
#include <sstream>

#include "odb/db.h"
#include "odb/PtrSetMap.h"
#include "db_sta/dbSta.hh"
#include "db_sta/dbNetwork.hh"
#include "db_sta/AocvDerate.hh"
#include "db_sta/IpChecker.hh"
#include "db_sta/MakeDbSta.hh"
#include "ord/OpenRoad.hh"
#include "sta/Graph.hh"
#include "sta/Network.hh"
#include "sta/Property.hh"
#include "sta/Report.hh"
#include "sta/StringUtil.hh"
#include "sta/Units.hh"
#include "sta/VerilogWriter.hh"

namespace ord {
// Defined in OpenRoad.i
OpenRoad *getOpenRoad();
}

using sta::Instance;

%}

%import "odb.i"
%include "../../Exception.i"
// OpenSTA swig files
%include "tcl/StaTclTypes.i"
%include "dcalc/DelayCalc.i"
%include "graph/Graph.i"
%include "liberty/Liberty.i"
%include "network/Network.i"
%include "network/NetworkEdit.i"
%include "parasitics/Parasitics.i"
%include "power/Power.i"
%include "sdc/Sdc.i"
%include "sdf/Sdf.i"
%include "search/Property.i"
%include "search/Search.i"
%include "spice/WriteSpice.i"
%include "util/Util.i"

%inline %{

sta::Sta *
make_block_sta(odb::dbBlock *block)
{
  ord::OpenRoad *openroad = ord::getOpenRoad();
  return openroad->getSta()->makeBlockSta(block).release();
}

// For testing
void
find_logic_constants()
{
  ord::OpenRoad *openroad = ord::getOpenRoad();
  sta::dbSta *sta = openroad->getSta();
  sta->findLogicConstants();
}

std::vector<odb::dbNet*>
find_all_clk_nets()
{
  ord::OpenRoad *openroad = ord::getOpenRoad();
  sta::dbSta *sta = openroad->getSta();
  odb::PtrSet<odb::dbNet> clk_nets = sta->findClkNets();
  std::vector<odb::dbNet*> clk_nets1(clk_nets.begin(), clk_nets.end());
  return clk_nets1;
}

std::vector<odb::dbNet*>
find_clk_nets(const Clock *clk)
{
  ord::OpenRoad *openroad = ord::getOpenRoad();
  sta::dbSta *sta = openroad->getSta();
  odb::PtrSet<odb::dbNet> clk_nets = sta->findClkNets(clk);
  std::vector<odb::dbNet*> clk_nets1(clk_nets.begin(), clk_nets.end());
  return clk_nets1;
}

odb::dbInst *
sta_to_db_inst(Instance *inst)
{
  ord::OpenRoad *openroad = ord::getOpenRoad();
  sta::dbNetwork *db_network = openroad->getDbNetwork();
  odb::dbInst *db_inst = nullptr;
  odb::dbModInst* mod_inst = nullptr;
  db_network->staToDb(inst, db_inst, mod_inst);
  return db_inst;
}

odb::dbModInst *
sta_to_db_mod_inst(Instance *inst)
{
  ord::OpenRoad *openroad = ord::getOpenRoad();
  sta::dbNetwork *db_network = openroad->getDbNetwork();
  odb::dbInst *db_inst = nullptr;
  odb::dbModInst* mod_inst = nullptr;
  db_network->staToDb(inst, db_inst, mod_inst);
  return mod_inst;
}

odb::dbMTerm *
sta_to_db_mterm(LibertyPort *port)
{
  ord::OpenRoad *openroad = ord::getOpenRoad();
  sta::dbNetwork *db_network = openroad->getDbNetwork();
  return db_network->staToDb(port);
}

odb::dbBTerm *
sta_to_db_port(Port *port)
{
  ord::OpenRoad *openroad = ord::getOpenRoad();
  sta::dbNetwork *db_network = openroad->getDbNetwork();
  Pin *pin = db_network->findPin(db_network->topInstance(), port);
  odb::dbITerm *iterm = nullptr;
  odb::dbBTerm *bterm = nullptr;
  odb::dbModITerm *moditerm = nullptr;
  db_network->staToDb(pin, iterm, bterm, moditerm);
  return bterm;
}

odb::dbModBTerm *
sta_to_db_mod_port(Port *port)
{
  ord::OpenRoad *openroad = ord::getOpenRoad();
  sta::dbNetwork *db_network = openroad->getDbNetwork();
  odb::dbBTerm *bterm = nullptr;
  odb::dbMTerm *mterm = nullptr;
  odb::dbModBTerm *modbterm = nullptr;
  db_network->staToDb(port, bterm, mterm, modbterm);
  return modbterm;
}

odb::dbITerm *
sta_to_db_pin(Pin *pin)
{
  ord::OpenRoad *openroad = ord::getOpenRoad();
  sta::dbNetwork *db_network = openroad->getDbNetwork();
  odb::dbITerm *iterm = nullptr;
  odb::dbBTerm *bterm = nullptr;
  odb::dbModITerm *moditerm = nullptr;
  db_network->staToDb(pin, iterm, bterm, moditerm);
  return iterm;
}

odb::dbModITerm *
sta_to_db_mod_pin(Pin *pin)
{
  ord::OpenRoad *openroad = ord::getOpenRoad();
  sta::dbNetwork *db_network = openroad->getDbNetwork();
  odb::dbITerm *iterm = nullptr;
  odb::dbBTerm *bterm = nullptr;
  odb::dbModITerm *moditerm = nullptr;
  db_network->staToDb(pin, iterm, bterm, moditerm);
  return moditerm;
}

Port *
sta_pin_to_port(Pin *pin)
{
  ord::OpenRoad *openroad = ord::getOpenRoad();
  sta::dbNetwork *db_network = openroad->getDbNetwork();
  return db_network->port(pin);
}

odb::dbNet *
sta_to_db_net(Net *net)
{
  ord::OpenRoad *openroad = ord::getOpenRoad();
  sta::dbNetwork *db_network = openroad->getDbNetwork();
  odb::dbNet *db_net = nullptr;
  odb::dbModNet *db_mod_net = nullptr;
  db_network->staToDb(net, db_net, db_mod_net);
  return db_net;
}

odb::dbModNet *
sta_to_db_mod_net(Net *net)
{
  ord::OpenRoad *openroad = ord::getOpenRoad();
  sta::dbNetwork *db_network = openroad->getDbNetwork();
  odb::dbNet *db_net = nullptr;
  odb::dbModNet *db_mod_net = nullptr;
  db_network->staToDb(net, db_net, db_mod_net);
  return db_mod_net;
}

odb::dbMaster *
sta_to_db_master(LibertyCell *cell)
{
  ord::OpenRoad *openroad = ord::getOpenRoad();
  sta::dbNetwork *db_network = openroad->getDbNetwork();
  return db_network->staToDb(cell);
}

void
db_network_defined()
{
  ord::OpenRoad *openroad = ord::getOpenRoad();
  sta::dbNetwork *db_network = openroad->getDbNetwork();
  odb::dbDatabase *db = openroad->getDb();
  odb::dbChip *chip = db->getChip();
  odb::dbBlock *block = chip->getBlock();
  db_network->readDefAfter(block);
}

void
report_cell_usage_cmd(odb::dbModule* mod,
                      const bool verbose,
                      const char *file_name,
                      const char *stage_name)
{
  ord::OpenRoad *openroad = ord::getOpenRoad();
  sta::dbSta *sta = openroad->getSta();
  sta->ensureLinked();
  sta->reportCellUsage(mod, verbose, file_name, stage_name);
}

void
report_timing_histogram_cmd(int num_bins,
                             const MinMax* min_max,
                             float bin_size)
{
  ord::OpenRoad *openroad = ord::getOpenRoad();
  sta::dbSta *sta = openroad->getSta();
  sta->ensureLinked();
  sta->reportTimingHistogram(num_bins, min_max, bin_size);
}

void
report_logic_depth_histogram_cmd(int num_bins, bool exclude_buffers,
                         bool exclude_inverters)
{
  ord::OpenRoad *openroad = ord::getOpenRoad();
  sta::dbSta *sta = openroad->getSta();
  sta->ensureLinked();
  sta->reportLogicDepthHistogram(num_bins, exclude_buffers, exclude_inverters);
}

// Copied from sta/verilog/Verilog.i because we don't want sta::read_verilog
// that is in the same file.
void
write_verilog_cmd(const char *filename,
		  bool include_pwr_gnd,
		  CellSeq *remove_cells)
{
  // This does NOT want the SDC (cmd) network because it wants
  // to see the sta internal names.
  ord::OpenRoad *openroad = ord::getOpenRoad();  
  sta::dbSta *sta = openroad->getSta();
  Network *network = sta->network();
  sta::writeVerilog(filename, include_pwr_gnd, remove_cells, network);
  delete remove_cells;
}

void
replace_hier_module_cmd(odb::dbModInst* mod_inst, odb::dbModule* module)
{
  ord::OpenRoad *openroad = ord::getOpenRoad();
  sta::dbNetwork *db_network = openroad->getDbNetwork();
  (void) db_network->replaceHierModule(mod_inst, module);
}


namespace sta {

void
report_levelized_drvr_vertices(int max_count)
{
  ord::OpenRoad *openroad = ord::getOpenRoad();
  sta::dbSta *sta = openroad->getSta();
  sta->ensureGraph();
  const sta::VertexSeq &drvrs = sta->levelizedDrvrVertices();
  sta::Report *report = sta->report();
  const sta::Network *network = sta->network();
  report->report("driver vertex count {}", drvrs.size());
  sta::Level prev_level = -1;
  int n = 0;
  for (sta::Vertex *vertex : drvrs) {
    sta::Level level = vertex->level();
    if (level < prev_level) {
      report->report("level order violated {} {} < {}",
                     network->pathName(vertex->pin()),
                     level, prev_level);
    }
    if (n < max_count) {
      report->report("{} level={}",
                     network->pathName(vertex->pin()),
                     level);
    }
    prev_level = level;
    n++;
  }
}

void check_axioms_cmd()
{
  ord::OpenRoad* openroad = ord::getOpenRoad();
  sta::dbSta *sta = openroad->getSta();
  sta->ensureLinked();
  sta->getDbNetwork()->checkAxioms();
  sta->checkSanity();
}

bool parasitics_annotated(Pin *pin, Scene *scene) {
  auto parasitics = scene->parasitics(sta::MinMax::max());
  return parasitics->findParasiticNetwork(pin) != nullptr;
}

} // namespace sta

bool
check_ip_cmd(const char* master_name,
             bool check_all,
             int max_polygons,
             bool verbose)
{
  ord::OpenRoad* openroad = ord::getOpenRoad();
  odb::dbDatabase* db = openroad->getDb();
  sta::dbSta* sta = openroad->getSta();
  utl::Logger* logger = openroad->getLogger();

  sta::IpChecker checker(db, sta, logger);
  checker.setMaxPolygons(max_polygons);
  checker.setVerbose(verbose);

  if (check_all) {
    return checker.checkAll();
  } else {
    return checker.checkMaster(master_name);
  }
}

////////////////////////////////////////////////////////////////
// Depth-based (AOCV-style) OCV derate, first slice (report-only).
// See AOCV_INVESTIGATION.md.

namespace sta {

// Process-global table driven from Tcl. Empty == feature inactive.
static AocvDerateTable &
aocvTable()
{
  static AocvDerateTable table;
  return table;
}

// OpenROAD-fork: AOCV -- when true, the table is installed on Search so the
// depth derate is applied during forward arrival propagation (not just in the
// report-only path). Defaults false: propagation is byte-identical to baseline.
static bool &
aocvPropagateFlag()
{
  static bool propagate = false;
  return propagate;
}

// OpenROAD-fork: AOCV -- (re)install or remove the propagation hook on Search to
// match the current flag + table state. Installing nullptr (flag off) restores
// byte-identical baseline timing. Any change to the hook OR (when the hook is
// active) to the table contents requires re-running timing, so we invalidate
// Search's arrivals. We invalidate whenever propagation is currently enabled
// because the caller mutated the table or the flag; a pointer-equality guard is
// insufficient since the table object identity does not change when its rows do.
static void
aocvSyncPropagation()
{
  ord::OpenRoad *openroad = ord::getOpenRoad();
  sta::dbSta *sta = openroad->getSta();
  sta::Search *search = sta->search();
  const sta::AocvDepthDerate *hook
      = aocvPropagateFlag() ? &aocvTable() : nullptr;
  const bool hook_changed = (search->aocvDepthDerate() != hook);
  search->setAocvDepthDerate(hook);
  // Recompute arrivals if the hook was installed/removed, or if it is active
  // (table rows may have just changed under it).
  if (hook_changed || hook != nullptr) {
    sta->arrivalsInvalid();
  }
}

void
aocv_derate_clear()
{
  aocvTable().clear();
  aocvSyncPropagation();
}

bool
aocv_derate_active()
{
  return !aocvTable().empty();
}

// OpenROAD-fork: AOCV -- enable/disable propagation-time depth derating.
void
aocv_derate_set_propagate(bool enable)
{
  aocvPropagateFlag() = enable;
  aocvSyncPropagation();
}

bool
aocv_derate_propagate_enabled()
{
  return aocvPropagateFlag();
}

// Returns "" on success, else an error message.
std::string
aocv_derate_read_file(const char *filename)
{
  std::string error;
  if (!aocvTable().readFile(filename, error)) {
    return error;
  }
  aocvSyncPropagation();  // OpenROAD-fork: AOCV -- refresh live hook/arrivals
  return "";
}

void
aocv_derate_set_entry(int depth,
                      float late_derate,
                      float early_derate)
{
  aocvTable().setLate(depth, late_derate);
  aocvTable().setEarly(depth, early_derate);
  aocvSyncPropagation();  // OpenROAD-fork: AOCV -- refresh live hook/arrivals
}

// Recompute the AOCV-adjusted slack for one path end and return a Tcl list:
//   {endpoint logic_depth flat_slack aocv_slack late_derate}
// Slacks are converted to the user time unit.
StringSeq
aocv_adjust_path_end(PathEnd *path_end)
{
  ord::OpenRoad *openroad = ord::getOpenRoad();
  sta::dbSta *sta = openroad->getSta();
  AocvPathResult r = aocvAdjustPathEnd(sta, path_end, aocvTable());
  sta::Unit *time_unit = sta->units()->timeUnit();
  auto fmt = [](double v) {
    std::ostringstream ss;
    ss.setf(std::ios::fixed);
    ss << std::setprecision(9) << v;
    return ss.str();
  };
  StringSeq out;
  out.push_back(r.endpoint);
  out.push_back(std::to_string(r.logic_depth));
  out.push_back(fmt(time_unit->staToUser(r.flat_slack)));
  out.push_back(fmt(time_unit->staToUser(r.aocv_slack)));
  out.push_back(fmt(r.late_derate));
  return out;
}

} // namespace sta

%} // inline
