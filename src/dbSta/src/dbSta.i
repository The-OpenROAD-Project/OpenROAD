// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// clang-format off

%{

#include <cstdio>

#include "db_sta/PbaReport.hh"
#include "sta/MinMax.hh"
#include <iomanip>
#include <sstream>

#include "odb/db.h"
#include "odb/PtrSetMap.h"
#include "db_sta/dbSta.hh"
#include "db_sta/dbNetwork.hh"
#include "db_sta/AocvDerate.hh"
#include "db_sta/IpChecker.hh"
#include "db_sta/MakeDbSta.hh"
#include "db_sta/CpprReport.hh"
#include "ord/OpenRoad.hh"
#include "sta/Graph.hh"
#include "sta/Network.hh"
#include "sta/Property.hh"
#include "sta/Report.hh"
#include "sta/Parasitics.hh"
#include "sta/Scene.hh"
#include "sta/MinMax.hh"
#include "sta/Transition.hh"
#include "sta/Delay.hh"
#include <cmath>
#include <limits>
#include <unordered_map>

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

// Snapshot of one parasitic coupling capacitor's original value so we can
// restore it exactly (disable / reset == byte-identical baseline).
struct SiCcSnapshot
{
  sta::Parasitics *parasitics;
  sta::ParasiticCapacitor *capacitor;
  float orig_value;
};


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

// Path-Based Analysis pessimism-recovery report (additive diagnostic).
// Reports, for the top max_paths GBA critical paths, the GBA slack, the
// PBA slack (after re-evaluating gate stages with path-specific slews) and
// the recovered pessimism. Does NOT change report_checks / GBA results.
void
report_pba_slack_cmd(int max_paths,
                     const MinMax *min_max)
{
  ord::OpenRoad *openroad = ord::getOpenRoad();
  sta::dbSta *sta = openroad->getSta();
  sta->ensureLinked();
  sta::reportPbaSlack(sta, max_paths, min_max);
}

// Machine-readable variant for testing. Returns one string per path:
//   "<endpoint> <gba_slack> <pba_slack> <recovered> <gate_stages>"
// with times in seconds (STA internal units).
StringSeq
pba_slack_report_lines(int max_paths,
                       const MinMax *min_max)
{
  ord::OpenRoad *openroad = ord::getOpenRoad();
  sta::dbSta *sta = openroad->getSta();
  sta->ensureLinked();
  std::vector<sta::PbaPathResult> results =
      sta::computePbaSlack(sta, max_paths, min_max);
  StringSeq lines;
  for (const sta::PbaPathResult &r : results) {
    char buf[512];
    std::snprintf(buf, sizeof(buf), "%s %.6e %.6e %.6e %d",
                  r.endpoint.c_str(),
                  r.gba_slack,
                  r.pba_slack,
                  r.recovered,
                  r.gate_stages);
    lines.push_back(buf);
  }
  return lines;
}

// CPPR / CRPR -- Clock Reconvergence Pessimism Removal report (additive,
// report-only diagnostic). For the top max_paths critical checks, reports
// the raw slack (common-path pessimism still double-counted), the CPPR
// slack (common-path credit applied -- the default GBA result) and the
// credited pessimism, plus the deepest shared clock pin. The credit is the
// engine's own (slack - slackNoCrpr); this does NOT change report_checks /
// GBA results and does NOT mutate the timing graph.
void
report_cppr_cmd(int max_paths,
                const MinMax *min_max)
{
  ord::OpenRoad *openroad = ord::getOpenRoad();
  sta::dbSta *sta = openroad->getSta();
  sta->ensureLinked();
  sta::reportCppr(sta, max_paths, min_max);
}

// Machine-readable variant for testing. Returns one string per check:
//   "<endpoint> <raw_slack> <cppr_slack> <credit> <common_pin>"
// with times in seconds (STA internal units). common_pin is "(none)" when
// no shared clock pin was identified.
StringSeq
cppr_report_lines(int max_paths,
                  const MinMax *min_max)
{
  ord::OpenRoad *openroad = ord::getOpenRoad();
  sta::dbSta *sta = openroad->getSta();
  sta->ensureLinked();
  std::vector<sta::CpprPathResult> results =
      sta::computeCpprSlack(sta, max_paths, min_max);
  StringSeq lines;
  for (const sta::CpprPathResult &r : results) {
    char buf[1024];
    std::snprintf(buf, sizeof(buf), "%s %.6e %.6e %.6e %s",
                  r.endpoint.c_str(),
                  r.raw_slack,
                  r.cppr_slack,
                  r.credit,
                  r.common_pin.empty() ? "(none)" : r.common_pin.c_str());
    lines.push_back(buf);
  }
  return lines;
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

////////////////////////////////////////////////////////////////
//
// Crosstalk / signal-integrity (SI) aware timing -- SECOND slice:
// timing-window aware coupling derate.
//
// The slice-1 Miller factor is a *blanket* worst-case derate: every coupling
// cap on every net is scaled, assuming every aggressor switches in the worst
// direction at the worst time.  This slice makes the derate window-aware: a
// victim net only keeps the worst-case coupling contribution from an aggressor
// whose switching window actually overlaps the victim's.  Aggressors whose
// windows do not overlap are gated out (their coupling cap is scaled back
// toward nominal), recovering the pessimism of the blanket factor.
//
// Mechanism (see SI_WINDOW_INVESTIGATION.md):
//   * A net's switching window is its driver pin's arrival interval
//     [arrival(min), arrival(max)] over both edges.
//   * For each victim's CC segments we find the aggressor net (the other net
//     on the seg) and its window.  Non-overlapping segs get their stored Cc
//     scaled by 1/mf_setup so that, after the blanket Miller factor is applied
//     during pi/Arnoldi reduction, the net contribution is nominal Cc.
//   * Original Cc is snapshotted so the feature is perfectly reversible
//     (disable / reset == byte-identical baseline).
//
// Default OFF reproduces slice-1 / baseline behavior exactly.
//
////////////////////////////////////////////////////////////////

namespace sta {

// A net's switching window: [lo, hi] arrival interval of its driver pin.
struct SiWindow
{
  bool valid = false;   // false => unknown (no driver / no timing); treated
                        // conservatively as "always overlaps".
  float lo = 0.0f;
  float hi = 0.0f;
};


// Process-global SI-window state.
struct SiWindowState
{
  bool enabled = false;
  double guardband = 0.0;          // seconds added to each side of aggressor
  int max_nets = 0;                // top-N victims to filter; <=0 == all
  std::vector<SiCcSnapshot> snapshots;  // for restore
};

static SiWindowState &
siWindowState()
{
  static SiWindowState state;
  return state;
}

// Restore every coupling cap we scaled back to its original value. Idempotent.
static void
si_window_restore_caps()
{
  SiWindowState &st = siWindowState();
  for (const SiCcSnapshot &s : st.snapshots) {
    s.parasitics->setCapacitorValue(s.capacitor, s.orig_value);
  }
  st.snapshots.clear();
}

// Compute the switching window of a net from its driver pin arrival interval.
static SiWindow
si_net_window(sta::dbSta *sta, sta::dbNetwork *db_network, odb::dbNet *db_net)
{
  SiWindow w;
  if (db_net == nullptr) {
    return w;
  }
  sta::Net *net = db_network->dbToSta(db_net);
  if (net == nullptr) {
    return w;
  }
  sta::PinSet *drvrs = db_network->drivers(net);
  if (drvrs == nullptr || drvrs->empty()) {
    return w;  // undriven / constant -> unknown window
  }
  // Use the timing graph vertex directly and guard against pins that have no
  // vertex (hierarchical pins, pins not in the graph) -- calling the pin-based
  // Sta::arrival on such a pin dereferences a null vertex.
  sta::Graph *graph = sta->ensureGraph();
  if (graph == nullptr) {
    return w;
  }
  float lo = std::numeric_limits<float>::infinity();
  float hi = -std::numeric_limits<float>::infinity();
  bool any = false;
  for (const sta::Pin *pin : *drvrs) {
    sta::Vertex *vertex = graph->pinDrvrVertex(pin);
    if (vertex == nullptr) {
      continue;
    }
    const sta::Arrival a_min = sta->arrival(
        vertex, sta::RiseFallBoth::riseFall(), sta->scenes(), sta::MinMax::min());
    const sta::Arrival a_max = sta->arrival(
        vertex, sta::RiseFallBoth::riseFall(), sta->scenes(), sta::MinMax::max());
    const float fmin = static_cast<float>(sta::delayAsFloat(a_min));
    const float fmax = static_cast<float>(sta::delayAsFloat(a_max));
    if (std::isfinite(fmin) && fmin < lo) {
      lo = fmin;
    }
    if (std::isfinite(fmax) && fmax > hi) {
      hi = fmax;
    }
    if (std::isfinite(fmin) || std::isfinite(fmax)) {
      any = true;
    }
  }
  if (any && lo <= hi) {
    w.valid = true;
    w.lo = lo;
    w.hi = hi;
  }
  return w;
}

// Two windows overlap iff aggressor (widened by guardband g) intersects victim.
// Unknown (invalid) windows are conservatively treated as overlapping so we
// never make timing more optimistic than the blanket factor.
static bool
si_windows_overlap(const SiWindow &victim, const SiWindow &aggr, double g)
{
  if (!victim.valid || !aggr.valid) {
    return true;
  }
  return (aggr.lo - g) <= victim.hi && victim.lo <= (aggr.hi + g);
}

// Per-victim window-filter result, for reporting.
struct SiVictimResult
{
  odb::dbNet *net;
  double total_cc;     // fF (all aggressors)
  double filtered_cc;  // fF (overlapping aggressors at worst case + gated at
                       // nominal, expressed as effective Cc after applying
                       // the scaling that reduction will see)
  int aggr_total;
  int aggr_gated;
};

// Core engine.  Recomputes window-filtered Cc for the top-N victims and scales
// non-overlapping CC segs by 1/mf_setup so the blanket Miller factor nets out
// to nominal on those segs.  Returns per-victim results (sorted by total Cc).
// Always restores previous scaling first so repeated calls are stable.
static std::vector<SiVictimResult>
si_window_apply(sta::dbSta *sta,
                sta::dbNetwork *db_network,
                odb::dbBlock *block,
                int corner,
                float mf_setup,
                double guardband,
                int max_nets,
                bool mutate)
{
  std::vector<SiVictimResult> results;
  SiWindowState &st = siWindowState();

  // Undo any previous scaling so we start from the true stored Cc.
  if (mutate) {
    si_window_restore_caps();
  }

  // The scale applied to a gated-out (non-overlapping) seg.  After the blanket
  // Miller factor mf_setup is applied during reduction, the effective
  // contribution becomes mf_setup * (1/mf_setup) * Cc = Cc (nominal).  If no
  // blanket factor is set (mf_setup <= 1) there is nothing to recover.
  const double gate_scale = (mf_setup > 1.0f) ? (1.0 / mf_setup) : 1.0;

  // Rank candidate victims by total coupling cap (same ordering as the SI
  // hotspot report) so top-N selects the highest-risk nets.
  struct Cand { odb::dbNet *net; double cc; };
  std::vector<Cand> cands;
  for (odb::dbNet *net : block->getNets()) {
    if (net->getSigType().isSupply()) {
      continue;
    }
    const double cc = net->getTotalCouplingCap(corner);
    if (cc <= 0.0) {
      continue;
    }
    cands.push_back({net, cc});
  }
  std::sort(cands.begin(), cands.end(),
            [](const Cand &a, const Cand &b) { return a.cc > b.cc; });

  const int n = (max_nets <= 0)
                    ? static_cast<int>(cands.size())
                    : std::min<int>(max_nets, cands.size());

  // Cache windows per net to avoid recomputing.
  std::unordered_map<odb::dbNet *, SiWindow> window_cache;
  auto get_window = [&](odb::dbNet *dn) -> const SiWindow & {
    auto it = window_cache.find(dn);
    if (it != window_cache.end()) {
      return it->second;
    }
    SiWindow w = si_net_window(sta, db_network, dn);
    return window_cache.emplace(dn, w).first->second;
  };

  for (int i = 0; i < n; i++) {
    odb::dbNet *victim_db = cands[i].net;
    sta::Net *victim = db_network->dbToSta(victim_db);
    if (victim == nullptr) {
      continue;
    }
    const SiWindow &vw = get_window(victim_db);

    double total_cc = 0.0;
    double filtered_cc = 0.0;
    int aggr_total = 0;
    int aggr_gated = 0;

    // Walk the coupling capacitors of this victim's parasitic network in every
    // parasitics object (min/max of every scene). A coupling cap is one whose
    // two nodes lie on different nets; the aggressor is the net of the node
    // that is not the victim.
    for (sta::Scene *scene : sta->scenes()) {
      sta::Parasitics *pars[2]
          = {scene->parasitics(sta::MinMax::max()),
             scene->parasitics(sta::MinMax::min())};
      sta::Parasitics *seen_prev = nullptr;
      for (sta::Parasitics *par : pars) {
        if (par == nullptr || par == seen_prev) {
          continue;  // skip null and shared min==max object (count once)
        }
        seen_prev = par;
        sta::Parasitic *pnet = par->findParasiticNetwork(victim);
        if (pnet == nullptr) {
          continue;
        }
        for (sta::ParasiticCapacitor *cap : par->capacitors(pnet)) {
          sta::ParasiticNode *n1 = par->node1(cap);
          sta::ParasiticNode *n2 = par->node2(cap);
          const sta::Net *net1 = par->net(n1, db_network);
          const sta::Net *net2 = par->net(n2, db_network);
          // Coupling cap: nodes on two different nets.
          const sta::Net *aggr_net = nullptr;
          if (net1 == victim && net2 != nullptr && net2 != victim) {
            aggr_net = net2;
          } else if (net2 == victim && net1 != nullptr && net1 != victim) {
            aggr_net = net1;
          } else {
            continue;  // grounded cap or not a victim coupling cap
          }

          const double cc = par->value(cap);          // Farads (internal)
          const double cc_ff = cc * 1e15;              // fF for reporting
          total_cc += cc_ff;
          aggr_total++;

          odb::dbNet *aggr_db = db_network->staToDb(aggr_net);
          const SiWindow &aw = get_window(aggr_db);

          if (si_windows_overlap(vw, aw, guardband)) {
            filtered_cc += cc_ff;  // overlap: keep worst case
          } else {
            // No overlap: gate out. Effective Cc -> nominal after the blanket
            // Miller factor re-multiplies during reduction.
            filtered_cc += cc_ff * gate_scale;
            aggr_gated++;
            if (mutate && gate_scale != 1.0) {
              st.snapshots.push_back(
                  {par, cap, static_cast<float>(cc)});
              par->setCapacitorValue(cap,
                                     static_cast<float>(cc * gate_scale));
            }
          }
        }
      }
    }

    results.push_back({victim_db, total_cc, filtered_cc, aggr_total,
                       aggr_gated});
  }

  return results;
}

}  // namespace sta

// Enable/disable window-aware coupling filtering.  enable=false (default)
// reproduces slice-1 / baseline behavior exactly: any previously applied
// scaling is undone and timing is invalidated so the next query is baseline.
void
set_si_timing_window_cmd(bool enable, double guardband, int max_nets)
{
  ord::OpenRoad *openroad = ord::getOpenRoad();
  sta::dbSta *sta = openroad->getSta();
  sta::SiWindowState &st = sta::siWindowState();

  if (!enable) {
    sta::si_window_restore_caps();
    st.enabled = false;
    // Drop cached reductions so timing returns to baseline coupling.
    sta::dbNetwork *db_network = openroad->getDbNetwork();
    for (sta::Scene *scene : sta->scenes()) {
      sta::Parasitics *par_min = scene->parasitics(sta::MinMax::min());
      sta::Parasitics *par_max = scene->parasitics(sta::MinMax::max());
      odb::dbChip *chip = openroad->getDb()->getChip();
      odb::dbBlock *block = (chip != nullptr) ? chip->getBlock() : nullptr;
      if (block != nullptr) {
        for (odb::dbNet *db_net : block->getNets()) {
          sta::Net *net = db_network->dbToSta(db_net);
          if (net == nullptr) {
            continue;
          }
          if (par_max != nullptr) {
            par_max->deleteReducedParasitics(net);
          }
          if (par_min != nullptr && par_min != par_max) {
            par_min->deleteReducedParasitics(net);
          }
        }
      }
    }
    sta->delaysInvalid();
    return;
  }

  st.enabled = true;
  st.guardband = guardband;
  st.max_nets = max_nets;
}

// Run the window filter and (optionally) mutate Cc, then report per victim:
// total Cc, window-filtered effective Cc, # aggressors gated, recovered Cc.
void
report_si_windows_cmd(int max_nets, int corner)
{
  ord::OpenRoad *openroad = ord::getOpenRoad();
  sta::dbSta *sta = openroad->getSta();
  sta::dbNetwork *db_network = openroad->getDbNetwork();
  utl::Logger *logger = openroad->getLogger();
  odb::dbDatabase *db = openroad->getDb();

  odb::dbChip *chip = db->getChip();
  if (chip == nullptr) {
    logger->error(utl::STA, 910, "No chip loaded.");
  }
  odb::dbBlock *block = chip->getBlock();
  if (block == nullptr) {
    logger->error(utl::STA, 911, "No block loaded.");
  }
  // A SPEF-only flow (no define_process_corner) reports cornerCount()==0 but
  // still exposes a single implicit corner 0 for the per-corner Cc accessors.
  // Treat corner 0 as valid in that case.
  const int corner_count = block->getCornerCount();
  const int valid_count = (corner_count == 0) ? 1 : corner_count;
  if (corner < 0 || corner >= valid_count) {
    logger->error(utl::STA, 912, "corner {} out of range [0, {}).", corner,
                  valid_count);
  }

  sta::SiWindowState &st = sta::siWindowState();
  float mf_setup = 1.0f, mf_hold = 1.0f;
  get_coupling_miller_factors(sta, &mf_setup, &mf_hold);

  // Only mutate stored Cc when the feature is enabled; otherwise this is a
  // pure read-only what-if report (no timing impact).
  const bool mutate = st.enabled;
  const double guardband = st.enabled ? st.guardband : 0.0;
  std::vector<sta::SiVictimResult> results = sta::si_window_apply(
      sta, db_network, block, corner, mf_setup, guardband, max_nets, mutate);

  if (mutate) {
    // Drop cached reductions for touched victims so re-reduction sees the
    // window-filtered Cc, then invalidate delays.
    for (const sta::SiVictimResult &r : results) {
      sta::Net *net = db_network->dbToSta(r.net);
      if (net == nullptr) {
        continue;
      }
      for (sta::Scene *scene : sta->scenes()) {
        sta::Parasitics *par_min = scene->parasitics(sta::MinMax::min());
        sta::Parasitics *par_max = scene->parasitics(sta::MinMax::max());
        if (par_max != nullptr) {
          par_max->deleteReducedParasitics(net);
        }
        if (par_min != nullptr && par_min != par_max) {
          par_min->deleteReducedParasitics(net);
        }
      }
    }
    sta->delaysInvalid();
  }

  sta::Report *report = sta->report();
  report->report(
      "SI timing-window coupling report (corner {}, window {}, Miller "
      "setup={:.3f} guardband={:.4g})",
      corner,
      st.enabled ? "ENABLED" : "disabled(what-if)",
      mf_setup,
      guardband);
  report->report("{:<36} {:>12} {:>12} {:>8} {:>8}",
                 "Victim net", "Cc(fF)", "Cc_win(fF)", "#aggr", "#gated");
  report->report(
      "------------------------------------------------------------"
      "--------------------");
  for (const sta::SiVictimResult &r : results) {
    report->report("{:<36} {:>12.4f} {:>12.4f} {:>8d} {:>8d}",
                   r.net->getConstName(),
                   r.total_cc,
                   r.filtered_cc,
                   r.aggr_total,
                   r.aggr_gated);
  }
  int total_gated = 0;
  double total_cc = 0.0, total_filtered = 0.0;
  for (const sta::SiVictimResult &r : results) {
    total_gated += r.aggr_gated;
    total_cc += r.total_cc;
    total_filtered += r.filtered_cc;
  }
  report->report(
      "Processed {} victim nets, gated {} non-overlapping aggressor segments; "
      "total Cc {:.4f} fF -> window-filtered {:.4f} fF.",
      static_cast<int>(results.size()),
      total_gated,
      total_cc,
      total_filtered);
}

// Return the number of aggressor CC segments that WOULD be gated out (windows
// non-overlapping) for the given corner, using the currently configured
// window state (guardband / max_nets). Read-only what-if (no Cc mutation, no
// timing change); used by tests to assert gating behavior deterministically.
int
si_window_gated_count_cmd(int corner)
{
  ord::OpenRoad *openroad = ord::getOpenRoad();
  sta::dbSta *sta = openroad->getSta();
  sta::dbNetwork *db_network = openroad->getDbNetwork();
  odb::dbChip *chip = openroad->getDb()->getChip();
  odb::dbBlock *block = (chip != nullptr) ? chip->getBlock() : nullptr;
  if (block == nullptr) {
    return 0;
  }
  sta::SiWindowState &st = sta::siWindowState();
  float mf_setup = 1.0f, mf_hold = 1.0f;
  get_coupling_miller_factors(sta, &mf_setup, &mf_hold);
  std::vector<sta::SiVictimResult> results
      = sta::si_window_apply(sta, db_network, block, corner, mf_setup,
                             st.guardband, st.max_nets, /* mutate */ false);
  int gated = 0;
  for (const sta::SiVictimResult &r : results) {
    gated += r.aggr_gated;
  }
  return gated;
}

%} // inline
