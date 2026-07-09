// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// clang-format off

%{

#include "odb/db.h"
#include "odb/PtrSetMap.h"
#include "db_sta/dbSta.hh"
#include "db_sta/dbNetwork.hh"
#include "db_sta/PocvDerate.hh"
#include "sta/PocvMode.hh"  // OpenROAD-fork: LVF -- PocvMode for propagation
#include "db_sta/IpChecker.hh"
#include "db_sta/MakeDbSta.hh"
#include "ord/OpenRoad.hh"
#include "sta/Graph.hh"
#include "sta/Network.hh"
#include "sta/Property.hh"
#include "sta/Report.hh"
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

// Parametric statistical OCV (POCV / LVF) derate, first slice (report-only).
// See POCV_INVESTIGATION.md. Variation combines in QUADRATURE (root-sum-square)
// along the path, so this is intentionally a report-only recompute on already
// found paths, NOT a forward-search propagation hook.

// Process-global POCV sigma state, mirrored into Search::PocvSigma. Default
// (enabled=false, per_stage=0, n_sigma=0) == feature inactive == baseline.
static Search::PocvSigma &
pocvSigmaState()
{
  static Search::PocvSigma sigma;
  return sigma;
}

// OpenROAD-fork: POCV -- push the current sigma state onto Search. This only
// updates a default-OFF state holder that NO propagation-path code reads, so it
// never changes timing; it exists so the state lives with the timer and the
// report can read it back. No arrival invalidation is needed (search unchanged).
static void
pocvSyncState()
{
  ord::OpenRoad *openroad = ord::getOpenRoad();
  sta::dbSta *sta = openroad->getSta();
  sta->search()->setPocvSigma(pocvSigmaState());
}

// Set the per-stage fractional sigma (k) and the sign-off sigma multiple
// (n_sigma) and enable POCV. With k==0 or n_sigma==0 the feature stays inactive
// (POCV slack == flat slack).
void
pocv_sigma_set(float per_stage,
               float n_sigma)
{
  Search::PocvSigma &s = pocvSigmaState();
  s.enabled = true;
  s.per_stage = per_stage;
  s.n_sigma = n_sigma;
  pocvSyncState();
}

// OpenROAD-fork: LVF -- enable PROPAGATION-TIME POCV. In addition to the
// report-only state, this flips the timer into statistical (normal) delay-ops
// with the sign-off quantile = n_sigma, so the synthetic per-stage variance
// injected in Search::deratedDelayData accumulates in quadrature through the
// forward search and is read out as mean +/- n_sigma*sqrt(var) at the checks.
// Switching the mode invalidates arrivals (handled by Sta::setPocvMode).
void
pocv_sigma_set_propagate(float per_stage,
                         float n_sigma)
{
  Search::PocvSigma &s = pocvSigmaState();
  s.enabled = true;
  s.per_stage = per_stage;
  s.n_sigma = n_sigma;
  s.propagate = true;
  pocvSyncState();

  ord::OpenRoad *openroad = ord::getOpenRoad();
  sta::dbSta *sta = openroad->getSta();
  // n_sigma is the sign-off quantile used by DelayOpsNormal::asFloat.
  sta->setPocvQuantile(n_sigma);
  // Statistical readout of the accumulated variance.
  sta->setPocvMode(sta::PocvMode::normal);
}

void
pocv_sigma_clear()
{
  const bool was_propagate = pocvSigmaState().propagate;
  pocvSigmaState() = Search::PocvSigma();  // back to inactive default
  pocvSyncState();
  // OpenROAD-fork: LVF -- if propagation mode had been enabled, return the
  // timer to scalar delay-ops so timing is byte-identical to baseline again.
  if (was_propagate) {
    ord::OpenRoad *openroad = ord::getOpenRoad();
    sta::dbSta *sta = openroad->getSta();
    sta->setPocvMode(sta::PocvMode::scalar);
  }
}

bool
pocv_sigma_active()
{
  return pocvSigmaState().active();
}

// Recompute the POCV statistical slack for one path end and return a Tcl list:
//   {endpoint logic_depth flat_slack pocv_slack flat_sigma rss_sigma n_sigma}
// Slacks and sigmas are converted to the user time unit.
StringSeq
pocv_adjust_path_end(PathEnd *path_end)
{
  ord::OpenRoad *openroad = ord::getOpenRoad();
  sta::dbSta *sta = openroad->getSta();
  PocvPathResult r = pocvAdjustPathEnd(sta, path_end, pocvSigmaState());
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
  out.push_back(fmt(time_unit->staToUser(r.pocv_slack)));
  out.push_back(fmt(time_unit->staToUser(r.flat_sigma)));
  out.push_back(fmt(time_unit->staToUser(r.rss_sigma)));
  out.push_back(fmt(r.n_sigma));
  return out;
}

// OpenROAD-fork: LVF -- read the PROPAGATION-TIME statistical slack of a path
// end. During propagation-mode POCV the path's arrival carries an accumulated
// variance (sum of (k*d_i)^2). The Tcl `slack`/`arrival` properties expose only
// the MEAN (delayAsFloat single-arg), so this accessor exposes the full
// statistical picture for testing/reporting. Returns a Tcl list:
//   {mean_slack slack_std_dev stat_slack n_sigma}
// where stat_slack = mean_slack - n_sigma*slack_std_dev (worst-case readout:
// the slack std-dev combines arrival+required variance, and the statistical
// worst slack subtracts the sigma margin). With POCV inactive / propagate off,
// slack_std_dev == 0 so stat_slack == mean_slack (baseline-safe).
StringSeq
pocv_path_end_stat_slack(PathEnd *path_end)
{
  ord::OpenRoad *openroad = ord::getOpenRoad();
  sta::dbSta *sta = openroad->getSta();
  const sta::Slack slack = path_end->slack(sta);
  const float mean = slack.mean();
  const float std_dev = slack.stdDev();
  const float n_sigma = pocvSigmaState().n_sigma;
  // Worst-case statistical slack: pull the slack down by n_sigma sigmas.
  const float stat = mean - n_sigma * std_dev;
  sta::Unit *time_unit = sta->units()->timeUnit();
  auto fmt = [](double v) {
    std::ostringstream ss;
    ss.setf(std::ios::fixed);
    ss << std::setprecision(9) << v;
    return ss.str();
  };
  StringSeq out;
  out.push_back(fmt(time_unit->staToUser(mean)));
  out.push_back(fmt(time_unit->staToUser(std_dev)));
  out.push_back(fmt(time_unit->staToUser(stat)));
  out.push_back(fmt(n_sigma));
  return out;
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

%} // inline
