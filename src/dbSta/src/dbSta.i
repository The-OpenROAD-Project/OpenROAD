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
#include "sta/Parasitics.hh"
#include "sta/Scene.hh"
#include "sta/MinMax.hh"
#include "sta/Units.hh"
#include "utl/Logger.h"

#include <algorithm>
#include <vector>
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

////////////////////////////////////////////////////////////////
//
// Crosstalk / signal-integrity (SI) aware timing -- first slice.
//
// A bounding "poor-man's SI" Miller-factor coupling derate.  The coupling
// caps kept in the OpenSTA parasitic network (read_spef
// -keep_capacitive_coupling) are multiplied by a configurable Miller factor
// during pi/Arnoldi reduction (see src/sta/parasitics/ReduceParasitics.cc).
// We surface that existing, tested factor as a first-class OpenROAD command
// with independent setup/hold control and an auditable report.  A factor of
// 1.0 (the default) is a true no-op and reproduces baseline timing exactly.
//
////////////////////////////////////////////////////////////////

// Apply Miller coupling factors: mf_setup -> max (setup) parasitics,
// mf_hold -> min (hold) parasitics, for every scene.  When the same
// parasitics object backs both min and max (the common single read_spef
// case) the two factors collide; we apply the setup factor and warn.
void
set_coupling_miller_factor_cmd(float mf_setup, float mf_hold)
{
  ord::OpenRoad *openroad = ord::getOpenRoad();
  sta::dbSta *sta = openroad->getSta();
  sta::dbNetwork *db_network = openroad->getDbNetwork();
  utl::Logger *logger = openroad->getLogger();

  // Collect the distinct parasitics objects and apply the factor.  The
  // setup factor goes to the max parasitics, the hold factor to the min.
  bool shared_warned = false;
  std::vector<sta::Parasitics *> touched;
  for (sta::Scene *scene : sta->scenes()) {
    sta::Parasitics *par_min = scene->parasitics(sta::MinMax::min());
    sta::Parasitics *par_max = scene->parasitics(sta::MinMax::max());
    if (par_min == par_max) {
      // Single shared parasitics object: cannot hold distinct setup/hold
      // factors.  Honor the setup (max) factor for the bounding run.
      if (par_max != nullptr) {
        par_max->setCouplingCapFactor(mf_setup);
        touched.push_back(par_max);
      }
      if (!shared_warned && mf_setup != mf_hold) {
        logger->warn(utl::STA,
                     900,
                     "setup and hold Miller factors differ but min and max "
                     "share one parasitics object; using the setup factor "
                     "{:.3f}. Read SPEF separately with -min/-max for "
                     "independent setup/hold factors.",
                     mf_setup);
        shared_warned = true;
      }
    } else {
      if (par_max != nullptr) {
        par_max->setCouplingCapFactor(mf_setup);
        touched.push_back(par_max);
      }
      if (par_min != nullptr) {
        par_min->setCouplingCapFactor(mf_hold);
        touched.push_back(par_min);
      }
    }
  }

  // The pi/Elmore models are reduced from the coupling-cap network and
  // cached.  Drop those cached reductions so the next delay calc re-reduces
  // using the new Miller factor.  Without this the factor would only take
  // effect on a fresh read_spef.
  odb::dbChip *chip = openroad->getDb()->getChip();
  odb::dbBlock *block = (chip != nullptr) ? chip->getBlock() : nullptr;
  if (block != nullptr) {
    for (odb::dbNet *db_net : block->getNets()) {
      sta::Net *net = db_network->dbToSta(db_net);
      if (net == nullptr) {
        continue;
      }
      for (sta::Parasitics *par : touched) {
        par->deleteReducedParasitics(net);
      }
    }
  }

  // Force delays to be recomputed with the new factor on the next report.
  sta->delaysInvalid();
}

// Report the active Miller coupling factors (setup=max, hold=min) from the
// first scene's parasitics into out_setup/out_hold.  static so SWIG does not
// wrap it (called only from C++).
static void
get_coupling_miller_factors(sta::dbSta *sta, float *out_setup, float *out_hold)
{
  *out_setup = 1.0f;
  *out_hold = 1.0f;
  const sta::SceneSeq &scenes = sta->scenes();
  if (!scenes.empty()) {
    sta::Scene *scene = scenes[0];
    sta::Parasitics *par_max = scene->parasitics(sta::MinMax::max());
    sta::Parasitics *par_min = scene->parasitics(sta::MinMax::min());
    if (par_max != nullptr) {
      *out_setup = par_max->couplingCapFactor();
    }
    if (par_min != nullptr) {
      *out_hold = par_min->couplingCapFactor();
    }
  }
}

// Rank nets by total coupling capacitance and report coupling cap, ground
// cap, and coupling/ground ratio for the top max_nets.  Pure read-only SI
// hotspot triage using the odb coupling-cap API.
void
report_coupling_si_cmd(int max_nets, int corner)
{
  ord::OpenRoad *openroad = ord::getOpenRoad();
  sta::dbSta *sta = openroad->getSta();
  utl::Logger *logger = openroad->getLogger();
  odb::dbDatabase *db = openroad->getDb();

  odb::dbChip *chip = db->getChip();
  if (chip == nullptr) {
    logger->error(utl::STA, 901, "No chip loaded.");
  }
  odb::dbBlock *block = chip->getBlock();
  if (block == nullptr) {
    logger->error(utl::STA, 902, "No block loaded.");
  }

  const int corner_count = block->getCornerCount();
  if (corner < 0 || corner >= corner_count) {
    logger->error(utl::STA,
                  903,
                  "corner {} out of range [0, {}).",
                  corner,
                  corner_count);
  }

  struct NetCoupling
  {
    odb::dbNet *net;
    double coupling_cap;  // fF
    double total_cap;     // fF
  };
  std::vector<NetCoupling> ranked;
  for (odb::dbNet *net : block->getNets()) {
    if (net->getSigType().isSupply()) {
      continue;
    }
    const double cc = net->getTotalCouplingCap(corner);
    if (cc <= 0.0) {
      continue;
    }
    const double total = net->getTotalCapacitance(corner, /* cc */ true);
    ranked.push_back({net, cc, total});
  }

  std::sort(ranked.begin(),
            ranked.end(),
            [](const NetCoupling &a, const NetCoupling &b) {
              return a.coupling_cap > b.coupling_cap;
            });

  float mf_setup = 1.0f, mf_hold = 1.0f;
  get_coupling_miller_factors(sta, &mf_setup, &mf_hold);
  sta::Report *report = sta->report();
  report->report(
      "Coupling / SI hotspot report (corner {}, Miller factor setup={:.3f} "
      "hold={:.3f})",
      corner,
      mf_setup,
      mf_hold);
  report->report("{:<40} {:>12} {:>12} {:>10}", "Net", "Cc(fF)", "Cgnd(fF)",
                 "Cc/Cgnd");
  report->report(
      "------------------------------------------------------------------"
      "--------------");

  const int n = (max_nets <= 0)
                    ? static_cast<int>(ranked.size())
                    : std::min<int>(max_nets, ranked.size());
  for (int i = 0; i < n; i++) {
    const NetCoupling &nc = ranked[i];
    const double cgnd = nc.total_cap - nc.coupling_cap;
    const double ratio = (cgnd > 0.0) ? (nc.coupling_cap / cgnd) : 0.0;
    report->report("{:<40} {:>12.4f} {:>12.4f} {:>10.4f}",
                   nc.net->getConstName(),
                   nc.coupling_cap,
                   cgnd,
                   ratio);
  }
  report->report("Reported {} of {} coupled signal nets.",
                 n,
                 static_cast<int>(ranked.size()));
}

%} // inline
