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
#include "db_sta/McmmReport.hh"
#include "db_sta/ClosureReport.hh"
#include "db_sta/IncrementalStaReport.hh"
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

// Per-endpoint PBA report (setup or hold) with negative->positive summary.
void
report_pba_endpoints_cmd(int max_paths,
                         const MinMax *min_max)
{
  ord::OpenRoad *openroad = ord::getOpenRoad();
  sta::dbSta *sta = openroad->getSta();
  sta->ensureLinked();
  sta::reportPbaEndpoints(sta, max_paths, min_max);
}

// PBA closure decision surface: list genuine post-PBA violations.
void
report_pba_closure_cmd(int max_paths,
                       const MinMax *min_max,
                       bool only_violations)
{
  ord::OpenRoad *openroad = ord::getOpenRoad();
  sta::dbSta *sta = openroad->getSta();
  sta->ensureLinked();
  sta::reportPbaClosure(sta, max_paths, min_max, only_violations);
}

// Machine-readable endpoint variant for testing. One string per endpoint:
//   "<endpoint> <gba_slack> <pba_slack> <recovered> <gba_viol> <pba_viol>"
// where the two trailing flags are 0/1.
StringSeq
pba_endpoint_report_lines(int max_paths,
                          const MinMax *min_max)
{
  ord::OpenRoad *openroad = ord::getOpenRoad();
  sta::dbSta *sta = openroad->getSta();
  sta->ensureLinked();
  std::vector<sta::PbaEndpointResult> results =
      sta::computePbaEndpoints(sta, max_paths, min_max);
  StringSeq lines;
  for (const sta::PbaEndpointResult &e : results) {
    char buf[512];
    std::snprintf(buf, sizeof(buf), "%s %.6e %.6e %.6e %d %d",
                  e.endpoint.c_str(),
                  e.gba_slack,
                  e.pba_slack,
                  e.recovered,
                  e.gba_violated ? 1 : 0,
                  e.pba_violated ? 1 : 0);
    lines.push_back(buf);
  }
  return lines;
}

// Machine-readable closure summary for testing. Returns four counters:
//   "<endpoints> <gba_violations> <recovered_endpoints> <pba_violations>"
const char *
pba_closure_summary(int max_paths,
                    const MinMax *min_max)
{
  ord::OpenRoad *openroad = ord::getOpenRoad();
  sta::dbSta *sta = openroad->getSta();
  sta->ensureLinked();
  std::vector<sta::PbaEndpointResult> results =
      sta::computePbaEndpoints(sta, max_paths, min_max);
  sta::PbaClosureSummary s = sta::summarizeClosure(results);
  static std::string out;
  out = std::to_string(s.endpoints) + " "
        + std::to_string(s.gba_violations) + " "
        + std::to_string(s.recovered_endpoints) + " "
        + std::to_string(s.pba_violations);
  return out.c_str();
}

// MCMM -- Multi-Corner Multi-Mode cross-corner worst-slack report (additive,
// report-only). For the top max_endpoints critical endpoints, reports the
// per-corner worst slack, the worst slack across all active corners and the
// limiting corner name. Uses OpenSTA's own cross-corner minimum; does NOT
// change report_checks / GBA results and does NOT mutate the timing graph.
void
report_mcmm_slack_cmd(int max_endpoints,
                      const MinMax *min_max)
{
  ord::OpenRoad *openroad = ord::getOpenRoad();
  sta::dbSta *sta = openroad->getSta();
  sta->ensureLinked();
  sta::reportMcmmSlack(sta, max_endpoints, min_max);
}

// Machine-readable variant for testing. Returns one string per endpoint:
//   "<endpoint> <worst_slack> <worst_corner> <c0name>=<c0slack|NA> ..."
// with slacks in seconds (STA internal units). Per-corner entries are listed
// in scene-index order; a corner with no constrained path for the endpoint
// is reported as "<name>=NA".
StringSeq
mcmm_report_lines(int max_endpoints,
                  const MinMax *min_max)
{
  ord::OpenRoad *openroad = ord::getOpenRoad();
  sta::dbSta *sta = openroad->getSta();
  sta->ensureLinked();
  std::vector<sta::McmmEndpointResult> results =
      sta::computeMcmmSlack(sta, max_endpoints, min_max);
  StringSeq lines;
  for (const sta::McmmEndpointResult &r : results) {
    std::string line = r.endpoint;
    char buf[256];
    std::snprintf(buf, sizeof(buf), " %.6e %s",
                  r.worst_slack, r.worst_corner.c_str());
    line += buf;
    for (const sta::McmmCornerSlack &col : r.corners) {
      if (col.valid) {
        std::snprintf(buf, sizeof(buf), " %s=%.6e",
                      col.corner.c_str(), col.slack);
      } else {
        std::snprintf(buf, sizeof(buf), " %s=NA", col.corner.c_str());
      }
      line += buf;
    }
    lines.push_back(line);
  }
  return lines;
}

// CPPR slice 2 -- endpoint closure decision surface (additive, report-only).
// Per-endpoint CPPR-adjusted slack aggregation: raw slack, credit applied,
// CPPR-adjusted slack and the post-CPPR closure status. Does NOT change
// report_checks / GBA results and does NOT mutate the timing graph.
void
report_cppr_endpoints_cmd(int max_paths,
                          const MinMax *min_max)
{
  ord::OpenRoad *openroad = ord::getOpenRoad();
  sta::dbSta *sta = openroad->getSta();
  sta->ensureLinked();
  sta::reportCpprEndpoints(sta, max_paths, min_max);
}

// CPPR slice 2 -- closure decision surface: classify raw-GBA-failing
// endpoints into common-path-pessimism artifacts (pass once CPPR removes the
// double-counted pessimism) vs genuine post-CPPR violations.
void
report_cppr_closure_cmd(int max_paths,
                        const MinMax *min_max,
                        bool only_violations)
{
  ord::OpenRoad *openroad = ord::getOpenRoad();
  sta::dbSta *sta = openroad->getSta();
  sta->ensureLinked();
  sta::reportCpprClosure(sta, max_paths, min_max, only_violations);
}

// Machine-readable endpoint variant for testing. One string per endpoint:
//   "<endpoint> <raw_slack> <cppr_slack> <credit> <raw_viol> <cppr_viol>"
// with times in seconds (STA internal units); the two trailing flags are 0/1.
StringSeq
cppr_endpoint_report_lines(int max_paths,
                           const MinMax *min_max)
{
  ord::OpenRoad *openroad = ord::getOpenRoad();
  sta::dbSta *sta = openroad->getSta();
  sta->ensureLinked();
  std::vector<sta::CpprEndpointResult> results =
      sta::computeCpprEndpoints(sta, max_paths, min_max);
  StringSeq lines;
  for (const sta::CpprEndpointResult &e : results) {
    char buf[512];
    std::snprintf(buf, sizeof(buf), "%s %.6e %.6e %.6e %d %d",
                  e.endpoint.c_str(),
                  e.raw_slack,
                  e.cppr_slack,
                  e.credit,
                  e.raw_violated ? 1 : 0,
                  e.cppr_violated ? 1 : 0);
    lines.push_back(buf);
  }
  return lines;
}

// Machine-readable closure summary for testing. Returns four counters:
//   "<endpoints> <raw_violations> <recovered_endpoints> <cppr_violations>"
const char *
cppr_closure_summary(int max_paths,
                     const MinMax *min_max)
{
  ord::OpenRoad *openroad = ord::getOpenRoad();
  sta::dbSta *sta = openroad->getSta();
  sta->ensureLinked();
  std::vector<sta::CpprEndpointResult> results =
      sta::computeCpprEndpoints(sta, max_paths, min_max);
  sta::CpprClosureSummary s = sta::summarizeCpprClosure(results);
  static std::string out;
  out = std::to_string(s.endpoints) + " "
        + std::to_string(s.raw_violations) + " "
        + std::to_string(s.recovered_endpoints) + " "
        + std::to_string(s.cppr_violations);
  return out.c_str();
}

// Incremental STA -- after a bounded ECO edit (resize / cell swap / buffer
// insert), report the affected fanout cone and the post-edit slacks taken
// from OpenSTA's incremental query API. Additive / report-only: it does NOT
// invalidate timing or change the full-STA / report_checks path.
void
report_incremental_sta_cmd(StringSeq seed_insts,
                           const MinMax *min_max)
{
  ord::OpenRoad *openroad = ord::getOpenRoad();
  sta::dbSta *sta = openroad->getSta();
  sta->ensureLinked();
  std::vector<std::string> seeds(seed_insts.begin(), seed_insts.end());
  sta::reportIncrementalSta(sta, seeds, min_max);
}

// Machine-readable variant for testing. The first line is a summary:
//   "SUMMARY <affected_vertices> <affected_endpoints> <total_endpoints> \
//    <wns> <tns>"
// Each subsequent line is one endpoint (sorted worst-first):
//   "<endpoint> <slack> <in_cone:1|0>"
// Slacks are in seconds (STA internal units).
StringSeq
incremental_sta_report_lines(StringSeq seed_insts,
                             const MinMax *min_max)
{
  ord::OpenRoad *openroad = ord::getOpenRoad();
  sta::dbSta *sta = openroad->getSta();
  sta->ensureLinked();
  std::vector<std::string> seeds(seed_insts.begin(), seed_insts.end());
  sta::IncrementalStaResult r =
      sta::computeIncrementalSta(sta, seeds, min_max);
  StringSeq lines;
  char buf[512];
  std::snprintf(buf, sizeof(buf), "SUMMARY %d %d %d %.12e %.12e",
                r.affected_vertices, r.affected_endpoints,
                r.total_endpoints, r.wns, r.tns);
  lines.push_back(buf);
  for (const sta::IncrementalEndpointSlack &ep : r.endpoints) {
    std::snprintf(buf, sizeof(buf), "%s %.12e %d",
                  ep.endpoint.c_str(), ep.slack, ep.in_cone ? 1 : 0);
    lines.push_back(buf);
  }
  return lines;
}

// Unified pessimism-recovery closure report (additive, report-only). For the
// top max_paths critical endpoints, COMPOSES the existing PBA gate-slew
// recovery and CPPR common-path credit and classifies each raw-failing
// endpoint as a genuine post-recovery violation or an artifact cleared by
// CPPR / PBA / both. Does NOT change report_checks / GBA results and does
// NOT mutate the timing graph.
void
report_closure_cmd(int max_paths,
                   const MinMax *min_max,
                   bool only_violations)
{
  ord::OpenRoad *openroad = ord::getOpenRoad();
  sta::dbSta *sta = openroad->getSta();
  sta->ensureLinked();
  sta::reportClosure(sta, max_paths, min_max, only_violations);
}

// Machine-readable endpoint variant for testing. One string per endpoint:
//   "<endpoint> <raw_slack> <gba_slack> <recovered_slack> <cppr_credit> \
//    <pba_recovered> <raw_viol> <genuine> <cleared_by>"
// with slacks in seconds (STA internal units); raw_viol/genuine are 0/1 and
// cleared_by is one of "-"/"CPPR"/"PBA"/"BOTH".
StringSeq
closure_report_lines(int max_paths,
                     const MinMax *min_max)
{
  ord::OpenRoad *openroad = ord::getOpenRoad();
  sta::dbSta *sta = openroad->getSta();
  sta->ensureLinked();
  std::vector<sta::ClosureEndpointResult> results =
      sta::computeClosure(sta, max_paths, min_max);
  StringSeq lines;
  for (const sta::ClosureEndpointResult &e : results) {
    char buf[1024];
    std::snprintf(buf, sizeof(buf), "%s %.6e %.6e %.6e %.6e %.6e %d %d %s",
                  e.endpoint.c_str(),
                  e.raw_slack,
                  e.gba_slack,
                  e.recovered_slack,
                  e.cppr_credit,
                  e.pba_recovered,
                  e.raw_violated ? 1 : 0,
                  e.genuine ? 1 : 0,
                  sta::clearedByLabel(e.cleared_by));
    lines.push_back(buf);
  }
  return lines;
}

// Machine-readable closure summary for testing. Returns six counters:
//   "<endpoints> <raw_failing> <cleared_by_cppr> <cleared_by_pba> \
//    <cleared_by_both> <genuine>"
const char *
closure_summary(int max_paths,
                const MinMax *min_max)
{
  ord::OpenRoad *openroad = ord::getOpenRoad();
  sta::dbSta *sta = openroad->getSta();
  sta->ensureLinked();
  std::vector<sta::ClosureEndpointResult> results =
      sta::computeClosure(sta, max_paths, min_max);
  sta::ClosureSummary s = sta::summarizeClosure(results);
  static std::string out;
  out = std::to_string(s.endpoints) + " "
        + std::to_string(s.raw_failing) + " "
        + std::to_string(s.cleared_by_cppr) + " "
        + std::to_string(s.cleared_by_pba) + " "
        + std::to_string(s.cleared_by_both) + " "
        + std::to_string(s.genuine);
  return out.c_str();
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

// Per-edge switching window of a net: the arrival interval of its driver pin
// restricted to a single transition polarity (rise OR fall). Used by the
// crosstalk-delay slice-2 direction classifier to decide whether an aggressor
// switches in the SAME or OPPOSITE direction as the victim during overlap.
// Mirrors si_net_window() but queries one RiseFall edge instead of riseFall().
static SiWindow
si_net_edge_window(sta::dbSta *sta,
                   sta::dbNetwork *db_network,
                   odb::dbNet *db_net,
                   const sta::RiseFall *rf)
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
    return w;
  }
  sta::Graph *graph = sta->ensureGraph();
  if (graph == nullptr) {
    return w;
  }
  const sta::RiseFallBoth *rfb
      = (rf == sta::RiseFall::rise()) ? sta::RiseFallBoth::rise()
                                      : sta::RiseFallBoth::fall();
  float lo = std::numeric_limits<float>::infinity();
  float hi = -std::numeric_limits<float>::infinity();
  bool any = false;
  for (const sta::Pin *pin : *drvrs) {
    sta::Vertex *vertex = graph->pinDrvrVertex(pin);
    if (vertex == nullptr) {
      continue;
    }
    const sta::Arrival a_min
        = sta->arrival(vertex, rfb, sta->scenes(), sta::MinMax::min());
    const sta::Arrival a_max
        = sta->arrival(vertex, rfb, sta->scenes(), sta::MinMax::max());
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

// Temporal overlap length (seconds) between a victim window and an aggressor
// window widened by guardband g. Negative/zero means no overlap. Invalid
// windows return -1 (caller treats as "unknown"). Used by the slice-2
// direction classifier to pick the dominant edge pairing.
static double
si_window_overlap_amount(const SiWindow &victim, const SiWindow &aggr,
                         double g)
{
  if (!victim.valid || !aggr.valid) {
    return -1.0;
  }
  const double lo = std::max<double>(victim.lo, aggr.lo - g);
  const double hi = std::min<double>(victim.hi, aggr.hi + g);
  return hi - lo;  // > 0 iff they overlap
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

////////////////////////////////////////////////////////////////
//
// Crosstalk-aware TIMING -- effective-capacitance stage-delay adjustment.
//
// This slice turns the coupling-capacitance (CC) segments that rcx already
// extracts into an actual stage-delay adjustment, not just a report. A
// victim net's effective capacitance grows when a coupled aggressor switches
// in the SAME direction within the timing window (the Miller effect roughly
// doubles the apparent coupling cap), and shrinks when it switches in the
// OPPOSITE direction (the two terminals move together, so less charge is
// delivered to the coupling cap).
//
// Model (slice 1 -- switching-factor effective-C):
//   For each top-N victim, for every CC segment to an aggressor:
//     * If the aggressor's switching window OVERLAPS the victim's (reusing the
//       si_window infrastructure) the aggressor is "active in-window" and the
//       segment's effective coupling becomes  Cc_eff = Cc * (1 + k).
//       (k is the user switching factor; k = 1 reproduces the classic 2x
//        Miller bound for a simultaneously-opposite-switching aggressor.)
//     * If it does NOT overlap, the aggressor is quiet/decoupled in the
//       window and the segment is left at nominal Cc (no adjustment).
//   The added effective cap  dC = k * Cc_active  is realized by bloating the
//   stored coupling capacitors of the victim (snapshotting originals), exactly
//   like the noise-delay slice, so report_checks sees the degraded stage delay
//   after pi/Arnoldi reduction. The implied stage-delay delta is reported as
//   d(delay) ~= Rdrv * dC for auditability.
//
//   k may be negative to model a net-decoupling (opposite-switching) bound:
//   Cc_eff = Cc * (1 + k) with -1 < k < 0 shrinks the effective coupling.
//
// DEFAULT OFF (never enabled) == byte-identical baseline: nothing is mutated.
//
////////////////////////////////////////////////////////////////

namespace sta {

// Process-global crosstalk-delay state. enabled=false (default) == baseline.
//
// Slice 2 adds two flag-gated knobs; their defaults reproduce slice-1 exactly:
//   * direction=false  -> every active segment uses factor (1+k) (slice-1
//                         worst-case same-direction assumption).
//   * iterations=1     -> a single apply pass (slice-1 single-pass behavior).
struct XtalkDelayState
{
  bool enabled = false;
  double k = 1.0;                       // user switching factor (Ceff += k*Cc)
  double guardband = 0.0;               // s, widens aggressor window (overlap)
  int max_nets = 0;                     // top-N victims; <=0 == all
  bool direction = false;               // slice-2: per-edge direction tracking
  int iterations = 1;                   // slice-2: bounded re-convergence passes
  // OpenROAD-fork: si-hold -- hold/min-path slice. When false (default) the
  // engine targets setup (max) exactly as before -> byte-identical baseline.
  // When true it targets the early-arrival (hold) case: a same-direction
  // aggressor switching with the victim REDUCES the effective coupling cap
  // ((1-k) instead of (1+k)), speeding the victim and hurting hold, and the
  // mutation/re-reduction is applied to the MIN parasitics.
  bool hold = false;
  std::vector<SiCcSnapshot> snapshots;  // for byte-identical restore
};

static XtalkDelayState &
xtalkDelayState()
{
  static XtalkDelayState state;
  return state;
}

// Restore every coupling cap the xtalk-delay adjustment bloated. Idempotent.
static void
xtalk_delay_restore_caps()
{
  XtalkDelayState &st = xtalkDelayState();
  for (const SiCcSnapshot &s : st.snapshots) {
    s.parasitics->setCapacitorValue(s.capacitor, s.orig_value);
  }
  st.snapshots.clear();
}

// Per-victim crosstalk-delay result, for reporting / test introspection.
struct XtalkDelayResult
{
  odb::dbNet *victim = nullptr;
  double cc_total = 0.0;     // fF -- total coupling cap (all aggressors)
  double cc_active = 0.0;    // fF -- coupling cap of in-window aggressors
  double cc_same = 0.0;      // slice-2: fF of same-direction active segments
  double cc_opp = 0.0;       // slice-2: fF of opposite-direction active segs
  double dcap = 0.0;         // fF -- added effective cap = k * cc_active
  double rdrv = 0.0;         // ohm -- victim driver on-resistance
  double ddelay = 0.0;       // s  -- implied stage-delay delta = Rdrv * dC
  int aggr_total = 0;        // CC segments seen
  int aggr_active = 0;       // CC segments with overlapping (active) window
  int aggr_same = 0;         // slice-2: active segments classified same-dir
  int aggr_opp = 0;          // slice-2: active segments classified opposite-dir
  bool applied = false;      // true if the bloat was applied to the graph
};

// Core engine. For the top-N coupled victims, scale every in-window CC segment
// so the effective coupling capacitance reflects aggressor switching;
// out-of-window segments are left at nominal. mutate=true applies the bloat
// (snapshotting originals); mutate=false is a pure read-only what-if. Always
// restores prior bloat first so repeated calls are stable.
//
// direction=false (slice-1 behavior): every active segment uses factor (1+k),
//   the worst-case same-direction (Miller aggravation) assumption.
// direction=true (slice-2): each active victim/aggressor pair is classified by
//   the dominant overlapping edge pairing. Same-polarity edges (both rise or
//   both fall) aggravate -> factor (1+k); opposite-polarity edges (one rise,
//   one fall) decouple (Miller) -> factor (1-k). The signed dCap accumulates
//   per segment accordingly.
//
// OpenROAD-fork: si-hold -- hold=false (default) is the unchanged setup/max
// engine above (byte-identical baseline). hold=true targets the early-arrival
// (hold/min) case: the effective-cap factor for a same-direction aggressor is
// (1-k) (the coupling node tracks the victim, less charge transfer -> smaller
// effective Cc -> the victim arrives EARLIER, hurting hold). Opposite-direction
// becomes (1+k). The mutation and re-reduction target the MIN parasitics so
// report_checks -path_delay min sees the degraded (earlier) arrival.
static std::vector<XtalkDelayResult>
xtalk_delay_apply(sta::dbSta *sta,
                  sta::dbNetwork *db_network,
                  odb::dbBlock *block,
                  int corner,
                  double k,
                  double guardband,
                  int max_nets,
                  bool direction,
                  bool mutate,
                  bool hold)
{
  std::vector<XtalkDelayResult> results;
  XtalkDelayState &st = xtalkDelayState();

  if (mutate) {
    xtalk_delay_restore_caps();
  }

  // Per-segment effective-cap factors are computed in the segment loop below:
  // (1+k) for same-direction (slice-1 worst case / direction off) and (1-k)
  // for opposite-direction when direction tracking is on. k == 0 is a no-op
  // (factor 1.0) so the feature degrades gracefully to baseline; k > -1 keeps
  // the same-direction effective cap non-negative.

  // Rank candidate victims by total coupling cap (same ordering as the SI
  // hotspot / window reports) so top-N selects the highest-risk nets.
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

  // Per-edge (rise/fall) window caches for the slice-2 direction classifier.
  // Only populated/queried when direction tracking is on, so direction=false
  // is byte-identical to slice-1 (these never run).
  std::unordered_map<odb::dbNet *, SiWindow> rise_cache, fall_cache;
  auto get_edge_window
      = [&](odb::dbNet *dn, const sta::RiseFall *rf) -> const SiWindow & {
    auto &cache = (rf == sta::RiseFall::rise()) ? rise_cache : fall_cache;
    auto it = cache.find(dn);
    if (it != cache.end()) {
      return it->second;
    }
    SiWindow w = si_net_edge_window(sta, db_network, dn, rf);
    return cache.emplace(dn, w).first->second;
  };

  // Classify an active victim/aggressor pair as same-direction (true) or
  // opposite-direction (false). We score the four edge pairings (victim
  // rise/fall x aggressor rise/fall) by their temporal overlap and pick the
  // dominant one; same-polarity pairings (RR, FF) aggravate, cross pairings
  // (RF, FR) decouple. Ties and unknown windows fall back to same-direction
  // (the conservative slice-1 worst case), so direction tracking never makes
  // timing more optimistic than slice-1 unless an opposite pairing strictly
  // dominates.
  auto classify_same_dir
      = [&](odb::dbNet *victim_db, odb::dbNet *aggr_db) -> bool {
    const SiWindow &vr = get_edge_window(victim_db, sta::RiseFall::rise());
    const SiWindow &vf = get_edge_window(victim_db, sta::RiseFall::fall());
    const SiWindow &ar = get_edge_window(aggr_db, sta::RiseFall::rise());
    const SiWindow &af = get_edge_window(aggr_db, sta::RiseFall::fall());
    const double rr = si_window_overlap_amount(vr, ar, guardband);  // same
    const double ff = si_window_overlap_amount(vf, af, guardband);  // same
    const double rf = si_window_overlap_amount(vr, af, guardband);  // opp
    const double fr = si_window_overlap_amount(vf, ar, guardband);  // opp
    const double same = std::max(rr, ff);
    const double opp = std::max(rf, fr);
    // If neither pairing is known to overlap (all invalid/non-overlapping),
    // keep the conservative same-direction assumption.
    if (opp > same) {
      return false;  // opposite direction strictly dominates -> decouple
    }
    return true;
  };
  for (int i = 0; i < n; i++) {
    odb::dbNet *victim_db = cands[i].net;
    sta::Net *victim = db_network->dbToSta(victim_db);
    if (victim == nullptr) {
      continue;
    }
    const SiWindow &vw = get_window(victim_db);

    XtalkDelayResult r;
    r.victim = victim_db;

    double cc_total_f = 0.0;   // Farads
    double cc_active_f = 0.0;  // Farads
    double cc_same_f = 0.0;    // Farads (same-direction active segments)
    double cc_opp_f = 0.0;     // Farads (opposite-direction active segments)
    // Signed added effective cap (Farads): sum over active segments of
    // (factor - 1) * Cc, where factor is (1+k) for same-direction and (1-k)
    // for opposite-direction (only when direction tracking is on). With
    // direction off this collapses to k * cc_active_f (slice-1).
    double dcap_f = 0.0;

    // Walk the victim's coupling capacitors in every parasitics object (min/max
    // of every scene). A coupling cap has its two nodes on different nets; the
    // aggressor is the net of the node that is not the victim.
    struct ActiveCap { sta::Parasitics *par; sta::ParasiticCapacitor *cap;
                       float orig; float factor; };
    std::vector<ActiveCap> active_caps;
    for (sta::Scene *scene : sta->scenes()) {
      // OpenROAD-fork: si-hold -- setup (hold=false) walks {max, min} exactly
      // as before (unchanged, byte-identical). Hold targets ONLY the min
      // parasitics so the early-arrival degradation lands on the min/hold
      // timing and never perturbs the max/setup path with the wrong sign.
      sta::Parasitics *pars[2];
      if (hold) {
        pars[0] = scene->parasitics(sta::MinMax::min());
        pars[1] = nullptr;
      } else {
        pars[0] = scene->parasitics(sta::MinMax::max());
        pars[1] = scene->parasitics(sta::MinMax::min());
      }
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
          sta::ParasiticNode *nd1 = par->node1(cap);
          sta::ParasiticNode *nd2 = par->node2(cap);
          const sta::Net *net1 = par->net(nd1, db_network);
          const sta::Net *net2 = par->net(nd2, db_network);
          const sta::Net *aggr_net = nullptr;
          if (net1 == victim && net2 != nullptr && net2 != victim) {
            aggr_net = net2;
          } else if (net2 == victim && net1 != nullptr && net1 != victim) {
            aggr_net = net1;
          } else {
            continue;  // grounded cap or not a victim coupling cap
          }

          const double cc = par->value(cap);  // Farads (internal)
          cc_total_f += cc;
          r.aggr_total++;

          odb::dbNet *aggr_db = db_network->staToDb(aggr_net);
          const SiWindow &aw = get_window(aggr_db);
          if (si_windows_overlap(vw, aw, guardband)) {
            // Aggressor switches in-window: this segment is "active". The
            // effective-cap factor depends on the switching direction:
            //   direction OFF (slice-1): always (1 + k) -- worst-case Miller
            //     aggravation, same for every active segment.
            //   direction ON (slice-2): same-direction -> (1 + k); opposite
            //     -> (1 - k) (Miller decoupling).
            cc_active_f += cc;
            r.aggr_active++;
            // OpenROAD-fork: si-hold -- the same-direction effective-cap factor
            // is (1+k) for setup (Miller aggravation slows the victim) but
            // (1-k) for hold (the coupling node tracks the victim, the
            // effective Cc shrinks and the victim arrives earlier). The
            // opposite-direction factor is the mirror image. hold=false keeps
            // the original setup signs exactly.
            const double same_factor = hold ? (1.0 - k) : (1.0 + k);
            const double opp_factor = hold ? (1.0 + k) : (1.0 - k);
            double seg_factor = same_factor;
            if (direction) {
              const bool same = classify_same_dir(victim_db, aggr_db);
              if (same) {
                seg_factor = same_factor;
                r.aggr_same++;
                cc_same_f += cc;
              } else {
                seg_factor = opp_factor;
                r.aggr_opp++;
                cc_opp_f += cc;
              }
            } else {
              r.aggr_same++;  // slice-1: all active treated same-direction
              cc_same_f += cc;
            }
            dcap_f += (seg_factor - 1.0) * cc;
            active_caps.push_back(
                {par, cap, static_cast<float>(cc),
                 static_cast<float>(seg_factor)});
          }
          // Out-of-window aggressor: left at nominal Cc (no adjustment).
        }
      }
    }

    r.cc_total = cc_total_f * 1e15;            // fF
    r.cc_active = cc_active_f * 1e15;          // fF
    r.cc_same = cc_same_f * 1e15;              // fF
    r.cc_opp = cc_opp_f * 1e15;                // fF
    r.dcap = dcap_f * 1e15;                    // fF (signed)

    // Implied stage-delay delta from the added load: d(delay) ~= Rdrv * dC.
    const double rdrv
        = noise_driver_resistance(sta, db_network, victim_db, nullptr);
    r.rdrv = rdrv;
    r.ddelay = (rdrv > 0.0) ? (rdrv * dcap_f) : 0.0;

    if (mutate && k != 0.0 && !active_caps.empty()) {
      for (const ActiveCap &ac : active_caps) {
        // factor==1 (k cancels, e.g. opposite-dir with k that nets to nominal)
        // would be a no-op; still snapshot so restore is symmetric and exact.
        st.snapshots.push_back({ac.par, ac.cap, ac.orig});
        ac.par->setCapacitorValue(
            ac.cap, static_cast<float>(ac.orig * ac.factor));
      }
      r.applied = true;
    }

    results.push_back(r);
  }

  // Rank reported rows by magnitude of the implied delay delta (largest first).
  std::sort(results.begin(), results.end(),
            [](const XtalkDelayResult &a, const XtalkDelayResult &b) {
              const double aa = (a.ddelay < 0.0) ? -a.ddelay : a.ddelay;
              const double bb = (b.ddelay < 0.0) ? -b.ddelay : b.ddelay;
              return aa > bb;
            });
  return results;
}

// Drop the cached pi/Arnoldi reduction of every applied victim so the next
// timing query re-reduces with the updated (bloated) coupling caps. Shared by
// the apply/report/converge paths.
static void
xtalk_delay_rereduce(sta::dbSta *sta,
                     sta::dbNetwork *db_network,
                     const std::vector<XtalkDelayResult> &results)
{
  for (const XtalkDelayResult &r : results) {
    if (!r.applied) {
      continue;
    }
    sta::Net *net = db_network->dbToSta(r.victim);
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
}

// Bounded iterative re-convergence. A victim's delay depends on aggressor
// arrival windows, which themselves depend on the aggressors' (now degraded)
// delays. We therefore re-run the apply pass a small, fixed number of times so
// the windows settle:
//
//   pass 1: windows from baseline delays            (== slice-1 single pass)
//   pass p: windows from the delays produced by pass p-1
//
// Each pass fully restores prior bloat first (apply does this when mutate), so
// passes are not cumulative -- they re-decide which segments are active and in
// which direction using the latest windows. Between passes we re-reduce the
// touched victims and invalidate delays so the next window query sees the new
// timing. iterations<=1 runs exactly one pass == slice-1 behavior.
//
// Reports per-pass setup TNS and the movement vs the previous pass so the user
// can see convergence. tns_out (if non-null) receives the TNS after each pass.
static std::vector<XtalkDelayResult>
xtalk_delay_converge(sta::dbSta *sta,
                     sta::dbNetwork *db_network,
                     odb::dbBlock *block,
                     int corner,
                     double k,
                     double guardband,
                     int max_nets,
                     bool direction,
                     int iterations,
                     bool hold,
                     std::vector<double> *tns_out)
{
  const int passes = (iterations < 1) ? 1 : iterations;
  // OpenROAD-fork: si-hold -- trace/converge on the corner that the mode
  // targets: setup TNS for the max path, hold TNS for the min path.
  const sta::MinMax *trace_mm = hold ? sta::MinMax::min() : sta::MinMax::max();
  std::vector<XtalkDelayResult> results;
  for (int p = 0; p < passes; p++) {
    results = xtalk_delay_apply(sta, db_network, block, corner, k, guardband,
                                max_nets, direction, /* mutate */ true, hold);
    xtalk_delay_rereduce(sta, db_network, results);
    sta->delaysInvalid();
    if (tns_out != nullptr) {
      // Force timing update so the recorded TNS reflects this pass and the
      // next pass's windows see the degraded delays.
      const double tns
          = sta::delayAsFloat(sta->totalNegativeSlack(trace_mm));
      tns_out->push_back(tns);
    }
  }
  return results;
}

} // namespace sta

// Enable/disable the crosstalk-aware effective-C stage-delay adjustment.
// enable=false (default) restores every bloated coupling cap byte-identically,
// drops cached parasitic reductions, and invalidates delays so the next timing
// query is exact baseline. enable=true scales in-window CC segments of the
// top-N victims by (1 + k) and forces re-reduction of the touched victims.
void
set_xtalk_delay_factor_cmd(bool enable, double k, double guardband,
                           int max_nets, int corner, bool direction,
                           int iterations, bool hold)
{
  ord::OpenRoad *openroad = ord::getOpenRoad();
  sta::dbSta *sta = openroad->getSta();
  sta::dbNetwork *db_network = openroad->getDbNetwork();
  utl::Logger *logger = openroad->getLogger();
  odb::dbDatabase *db = openroad->getDb();
  sta::XtalkDelayState &st = sta::xtalkDelayState();

  odb::dbChip *chip = db->getChip();
  if (chip == nullptr) {
    logger->error(utl::STA, 940, "No chip loaded.");
  }
  odb::dbBlock *block = chip->getBlock();
  if (block == nullptr) {
    logger->error(utl::STA, 941, "No block loaded.");
  }
  const int corner_count = block->getCornerCount();
  const int valid_count = (corner_count == 0) ? 1 : corner_count;
  if (corner < 0 || corner >= valid_count) {
    logger->error(utl::STA, 942, "corner {} out of range [0, {}).", corner,
                  valid_count);
  }

  if (!enable) {
    sta::xtalk_delay_restore_caps();
    st.enabled = false;
    // Drop cached reductions so timing returns to baseline coupling.
    for (sta::Scene *scene : sta->scenes()) {
      sta::Parasitics *par_min = scene->parasitics(sta::MinMax::min());
      sta::Parasitics *par_max = scene->parasitics(sta::MinMax::max());
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
    sta->delaysInvalid();
    return;
  }

  st.enabled = true;
  st.k = k;
  st.guardband = guardband;
  st.max_nets = max_nets;
  st.direction = direction;
  st.iterations = (iterations < 1) ? 1 : iterations;
  st.hold = hold;  // OpenROAD-fork: si-hold

  // Apply the adjustment with bounded re-convergence. iterations==1 is exactly
  // the slice-1 single pass; >1 lets aggressor windows settle as victim delays
  // degrade. Per-pass TNS is reported so the user can see convergence.
  // For the slice-1-equivalent path (single pass, direction off, setup) we
  // pass no tns sink so the engine behaves byte-identically to slice-1 (no
  // extra timing query is forced).
  const bool want_trace = (st.iterations > 1) || direction || hold;
  std::vector<double> tns_trace;
  std::vector<sta::XtalkDelayResult> results = sta::xtalk_delay_converge(
      sta, db_network, block, corner, k, guardband, max_nets, direction,
      st.iterations, hold, want_trace ? &tns_trace : nullptr);

  if (want_trace) {
    sta::Report *report = sta->report();
    const char *corner_label = hold ? "hold" : "setup";
    report->report(
        "Crosstalk-aware delay ({}): {} pass(es), direction tracking {}.",
        corner_label, st.iterations, direction ? "ON" : "off");
    double prev = 0.0;
    for (size_t p = 0; p < tns_trace.size(); p++) {
      const double tns = tns_trace[p];
      if (p == 0) {
        report->report("  pass {}: {} TNS = {:.6e}",
                       static_cast<int>(p + 1), corner_label, tns);
      } else {
        report->report("  pass {}: {} TNS = {:.6e}  (moved {:.3e})",
                       static_cast<int>(p + 1), corner_label, tns, tns - prev);
      }
      prev = tns;
    }
  }
}

// Report, per victim: total Cc, in-window (active) Cc, the added effective cap
// dC = k*Cc_active, the victim driver resistance, the implied stage-delay delta
// (Rdrv*dC), the # of active aggressors, and whether it was applied. When the
// feature is enabled this re-applies the adjustment (timing reflects it); when
// disabled it is a pure read-only what-if using the supplied k/guardband.
void
report_xtalk_delay_cmd(double k, double guardband, int max_nets, int corner,
                       bool direction, bool hold)
{
  ord::OpenRoad *openroad = ord::getOpenRoad();
  sta::dbSta *sta = openroad->getSta();
  sta::dbNetwork *db_network = openroad->getDbNetwork();
  utl::Logger *logger = openroad->getLogger();
  odb::dbDatabase *db = openroad->getDb();
  sta::XtalkDelayState &st = sta::xtalkDelayState();

  odb::dbChip *chip = db->getChip();
  if (chip == nullptr) {
    logger->error(utl::STA, 943, "No chip loaded.");
  }
  odb::dbBlock *block = chip->getBlock();
  if (block == nullptr) {
    logger->error(utl::STA, 944, "No block loaded.");
  }
  const int corner_count = block->getCornerCount();
  const int valid_count = (corner_count == 0) ? 1 : corner_count;
  if (corner < 0 || corner >= valid_count) {
    logger->error(utl::STA, 945, "corner {} out of range [0, {}).", corner,
                  valid_count);
  }

  // When enabled, report with the configured knobs and re-apply (mutate) so
  // the reported state matches the timing the engine sees. When disabled, run a
  // read-only what-if with the knobs passed to the report command.
  const bool mutate = st.enabled;
  const double use_k = st.enabled ? st.k : k;
  const double use_gb = st.enabled ? st.guardband : guardband;
  const int use_mn = st.enabled ? st.max_nets : max_nets;
  const bool use_dir = st.enabled ? st.direction : direction;
  const bool use_hold = st.enabled ? st.hold : hold;  // OpenROAD-fork: si-hold

  std::vector<sta::XtalkDelayResult> results = sta::xtalk_delay_apply(
      sta, db_network, block, corner, use_k, use_gb, use_mn, use_dir, mutate,
      use_hold);

  if (mutate) {
    sta::xtalk_delay_rereduce(sta, db_network, results);
    sta->delaysInvalid();
  }

  sta::Report *report = sta->report();
  report->report(
      "Crosstalk-aware delay report (corner {}, mode {}, xtalk-delay {}, "
      "k={:.3f}, guardband={:.3e}, direction {})",
      corner,
      use_hold ? "hold/min" : "setup/max",
      st.enabled ? "ENABLED" : "disabled(what-if)",
      use_k,
      use_gb,
      use_dir ? "ON" : "off");
  report->report(
      "{:<28} {:>10} {:>11} {:>11} {:>11} {:>11} {:>7} {:>5} {:>5} {:>7}",
      "Victim net", "Cc(fF)", "CcActive(fF)", "dCap(fF)", "Rdrv(ohm)",
      "dDelay(s)", "Active", "Same", "Opp", "Applied");
  report->report(
      "--------------------------------------------------------------------"
      "--------------------------------------------");
  int n_applied = 0;
  for (const sta::XtalkDelayResult &r : results) {
    if (r.applied) {
      n_applied++;
    }
    report->report(
        "{:<28} {:>10.4f} {:>11.4f} {:>11.4f} {:>11.2f} {:>11.4e} {:>7} {:>5} "
        "{:>5} {:>7}",
        r.victim->getConstName(),
        r.cc_total,
        r.cc_active,
        r.dcap,
        r.rdrv,
        r.ddelay,
        r.aggr_active,
        r.aggr_same,
        r.aggr_opp,
        r.applied ? "yes" : "no");
  }
  report->report(
      "Reported {} victim nets; {} {} an effective-C adjustment.",
      static_cast<int>(results.size()),
      n_applied,
      st.enabled ? "received" : "would receive");
}

// Return the implied crosstalk stage-delay delta (seconds) for a single named
// net at a corner using the supplied switching factor k and guardband, or a
// sentinel < -1e30 if the net is not found. Read-only what-if (never mutates
// the graph). Used by tests to assert the model deterministically against a
// hand calculation.
double
xtalk_delay_ddelay_for_net_cmd(const char *net_name, double k,
                               double guardband, int corner)
{
  ord::OpenRoad *openroad = ord::getOpenRoad();
  sta::dbSta *sta = openroad->getSta();
  sta::dbNetwork *db_network = openroad->getDbNetwork();
  odb::dbChip *chip = openroad->getDb()->getChip();
  odb::dbBlock *block = (chip != nullptr) ? chip->getBlock() : nullptr;
  if (block == nullptr) {
    return -1e30;
  }
  odb::dbNet *net = block->findNet(net_name);
  if (net == nullptr) {
    return -1e30;
  }
  // Read-only what-if over just this victim (max_nets large so it is included).
  // Direction tracking off => slice-1 behavior (used by the slice-1 test).
  std::vector<sta::XtalkDelayResult> results = sta::xtalk_delay_apply(
      sta, db_network, block, corner, k, guardband, /* max_nets */ 0,
      /* direction */ false, /* mutate */ false, /* hold */ false);
  for (const sta::XtalkDelayResult &r : results) {
    if (r.victim == net) {
      return r.ddelay;
    }
  }
  return -1e30;
}

// Slice-2: read-only implied crosstalk stage-delay delta (seconds) for a named
// net WITH per-edge direction classification. Same-direction aggressors push
// the delta positive (effective cap up), opposite-direction aggressors push it
// negative (Miller decoupling). Sentinel < -1e30 if the net is not found.
// Used by the slice-2 test to assert the signs of the delta differ.
double
xtalk_delay_ddelay_dir_for_net_cmd(const char *net_name, double k,
                                   double guardband, int corner)
{
  ord::OpenRoad *openroad = ord::getOpenRoad();
  sta::dbSta *sta = openroad->getSta();
  sta::dbNetwork *db_network = openroad->getDbNetwork();
  odb::dbChip *chip = openroad->getDb()->getChip();
  odb::dbBlock *block = (chip != nullptr) ? chip->getBlock() : nullptr;
  if (block == nullptr) {
    return -1e30;
  }
  odb::dbNet *net = block->findNet(net_name);
  if (net == nullptr) {
    return -1e30;
  }
  std::vector<sta::XtalkDelayResult> results = sta::xtalk_delay_apply(
      sta, db_network, block, corner, k, guardband, /* max_nets */ 0,
      /* direction */ true, /* mutate */ false, /* hold */ false);
  for (const sta::XtalkDelayResult &r : results) {
    if (r.victim == net) {
      return r.ddelay;
    }
  }
  return -1e30;
}

// Slice-2: number of active aggressor segments of a named net classified as
// same-direction (kind=0) or opposite-direction (kind=1) with per-edge
// direction tracking on. Returns -1 if the net is not found. Read-only.
int
xtalk_delay_dir_count_for_net_cmd(const char *net_name, int kind,
                                  double guardband, int corner)
{
  ord::OpenRoad *openroad = ord::getOpenRoad();
  sta::dbSta *sta = openroad->getSta();
  sta::dbNetwork *db_network = openroad->getDbNetwork();
  odb::dbChip *chip = openroad->getDb()->getChip();
  odb::dbBlock *block = (chip != nullptr) ? chip->getBlock() : nullptr;
  if (block == nullptr) {
    return -1;
  }
  odb::dbNet *net = block->findNet(net_name);
  if (net == nullptr) {
    return -1;
  }
  std::vector<sta::XtalkDelayResult> results = sta::xtalk_delay_apply(
      sta, db_network, block, corner, /* k */ 1.0, guardband,
      /* max_nets */ 0, /* direction */ true, /* mutate */ false, /* hold */ false);
  for (const sta::XtalkDelayResult &r : results) {
    if (r.victim == net) {
      return (kind == 0) ? r.aggr_same : r.aggr_opp;
    }
  }
  return -1;
}

// Slice-2: same-direction (kind=0) or opposite-direction (kind=1) active
// coupling cap (fF) for a named net with per-edge direction tracking on.
// Returns < 0 if the net is not found. Read-only. Lets the test assert the
// hand calculation dDelay = Rdrv * k * (CcSame - CcOpp) * 1e-15.
double
xtalk_delay_dir_cc_for_net_cmd(const char *net_name, int kind,
                               double guardband, int corner)
{
  ord::OpenRoad *openroad = ord::getOpenRoad();
  sta::dbSta *sta = openroad->getSta();
  sta::dbNetwork *db_network = openroad->getDbNetwork();
  odb::dbChip *chip = openroad->getDb()->getChip();
  odb::dbBlock *block = (chip != nullptr) ? chip->getBlock() : nullptr;
  if (block == nullptr) {
    return -1.0;
  }
  odb::dbNet *net = block->findNet(net_name);
  if (net == nullptr) {
    return -1.0;
  }
  std::vector<sta::XtalkDelayResult> results = sta::xtalk_delay_apply(
      sta, db_network, block, corner, /* k */ 1.0, guardband,
      /* max_nets */ 0, /* direction */ true, /* mutate */ false, /* hold */ false);
  for (const sta::XtalkDelayResult &r : results) {
    if (r.victim == net) {
      return (kind == 0) ? r.cc_same : r.cc_opp;
    }
  }
  return -1.0;
}

// Return the in-window (active) coupling cap (fF) for a single named net, or
// a sentinel < 0 if not found. Read-only. Used by tests to assert which
// aggressors are counted as switching in-window.
double
xtalk_delay_active_cc_for_net_cmd(const char *net_name, double guardband,
                                  int corner)
{
  ord::OpenRoad *openroad = ord::getOpenRoad();
  sta::dbSta *sta = openroad->getSta();
  sta::dbNetwork *db_network = openroad->getDbNetwork();
  odb::dbChip *chip = openroad->getDb()->getChip();
  odb::dbBlock *block = (chip != nullptr) ? chip->getBlock() : nullptr;
  if (block == nullptr) {
    return -1.0;
  }
  odb::dbNet *net = block->findNet(net_name);
  if (net == nullptr) {
    return -1.0;
  }
  std::vector<sta::XtalkDelayResult> results = sta::xtalk_delay_apply(
      sta, db_network, block, corner, /* k */ 1.0, guardband,
      /* max_nets */ 0, /* direction */ false, /* mutate */ false, /* hold */ false);
  for (const sta::XtalkDelayResult &r : results) {
    if (r.victim == net) {
      return r.cc_active;
    }
  }
  return -1.0;
}

// OpenROAD-fork: si-hold -- read-only implied HOLD-path crosstalk stage-delay
// delta (seconds) for a named net. With direction OFF every in-window
// aggressor is treated same-direction, so the hold effective-cap factor is
// (1-k): the effective coupling cap SHRINKS, the victim arrives EARLIER, and
// the delta is NEGATIVE (a speed-up that erodes hold slack). This is the exact
// sign mirror of xtalk_delay_ddelay_for_net_cmd (setup), so a test can assert
// hold_ddelay == -setup_ddelay for the same net/k. Sentinel < -1e30 if the net
// is not found. Never mutates the graph.
double
xtalk_delay_hold_ddelay_for_net_cmd(const char *net_name, double k,
                                    double guardband, int corner)
{
  ord::OpenRoad *openroad = ord::getOpenRoad();
  sta::dbSta *sta = openroad->getSta();
  sta::dbNetwork *db_network = openroad->getDbNetwork();
  odb::dbChip *chip = openroad->getDb()->getChip();
  odb::dbBlock *block = (chip != nullptr) ? chip->getBlock() : nullptr;
  if (block == nullptr) {
    return -1e30;
  }
  odb::dbNet *net = block->findNet(net_name);
  if (net == nullptr) {
    return -1e30;
  }
  std::vector<sta::XtalkDelayResult> results = sta::xtalk_delay_apply(
      sta, db_network, block, corner, k, guardband, /* max_nets */ 0,
      /* direction */ false, /* mutate */ false, /* hold */ true);
  for (const sta::XtalkDelayResult &r : results) {
    if (r.victim == net) {
      return r.ddelay;
    }
  }
  return -1e30;
}

%} // inline
