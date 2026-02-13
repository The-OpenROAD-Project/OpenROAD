// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// clang-format off

%{

#include <cstdint>
#include <fstream>

#include "sta/Liberty.hh"
#include "sta/Parasitics.hh"
#include "sta/Network.hh"
#include "sta/Corner.hh"
#include "odb/db.h"
#include "rsz/Resizer.hh"
#include "sta/Delay.hh"
#include "db_sta/dbNetwork.hh"
#include "Graphics.hh"
#include "ord/OpenRoad.hh"

namespace ord {
// Defined in OpenRoad.i
rsz::Resizer *
getResizer();
utl::Logger*
getLogger();
void
ensureLinked();
}

namespace sta {

// The aliases are created to attach different conversion rules:
// TmpNetSeq, TmpPinSet pointers are freed when crossing into Tcl,
// NetSeq, PinSet pointers are not
using TmpNetSeq = NetSeq;
using TmpPinSet = PinSet;

} // namespace

using ord::getResizer;
using ord::ensureLinked;

using sta::Corner;
using sta::LibertyCellSeq;
using sta::LibertyCell;
using sta::Instance;
using sta::InstanceSeq;
using sta::InstanceSet;
using sta::Net;
using sta::NetSeq;
using sta::Pin;
using sta::PinSet;
using sta::TmpPinSet;
using sta::RiseFall;
using sta::TmpNetSeq;
using sta::LibertyPort;
using sta::Delay;
using sta::Slew;
using sta::dbNetwork;
using sta::Network;
using sta::stringEq;

using rsz::Resizer;
%}

// OpenSTA swig files
%include "tcl/StaTclTypes.i"

////////////////////////////////////////////////////////////////
//
// SWIG type definitions.
//
////////////////////////////////////////////////////////////////

%typemap(out) TmpNetSeq* {
  NetSeq *nets = $1;
  seqPtrTclList<NetSeq, Net>(nets, SWIGTYPE_p_Net, interp);
  delete nets;
}

%typemap(out) TmpPinSet* {
  PinSet *pins = $1;
  setPtrTclList<PinSet, Pin>(pins, SWIGTYPE_p_Pin, interp);
  delete pins;
}

%typemap(in) std::vector<rsz::MoveType> {
  const char* str = Tcl_GetString($input);
  $1 = Resizer::parseMoveSequence(std::string(str));
}



////////////////////////////////////////////////////////////////
//
// C++ functions visible as TCL functions.
//
////////////////////////////////////////////////////////////////

%include "../../Exception.i"
%include "std_string.i"

%inline %{

namespace rsz {

void
report_net_parasitic(Net *net)
{
  Resizer *resizer = getResizer();
  Corner *corner = sta::Sta::sta()->cmdCorner();
  const ParasiticAnalysisPt *ap = corner->findParasiticAnalysisPt(sta::MinMax::max());
  auto parasitic = resizer->parasitics()->findParasiticNetwork(net, ap);
  if (parasitic) {
    resizer->parasitics()->report(parasitic);
  }
}

float
sum_parasitic_network_resist(Net *net)
{
  Resizer *resizer = getResizer();
  Corner *corner = sta::Sta::sta()->cmdCorner();
  const ParasiticAnalysisPt *ap = corner->findParasiticAnalysisPt(sta::MinMax::max());
  auto parasitic = resizer->parasitics()->findParasiticNetwork(net, ap);
  if (parasitic) {
    float ret = 0.0;
    for (auto resist : resizer->parasitics()->resistors(parasitic)) {
      ret += resizer->parasitics()->value(resist);
    }
    return ret;
  } else {
    return 0.0f;
  }
}

void
remove_buffers_cmd(InstanceSeq *insts)
{
  ensureLinked();
  Resizer *resizer = getResizer();
  if (insts) {
    resizer->removeBuffers(*insts);
    delete insts;
  } else {
    resizer->removeBuffers({});
  }
}

// Unbuffer net fully (for testing)
void
unbuffer_net_cmd(Net *net)
{
  ensureLinked();
  Resizer *resizer = getResizer();
  resizer->unbufferNet(net);
}

void
balance_row_usage_cmd()
{
  ensureLinked();
  Resizer *resizer = getResizer();
  resizer->balanceRowUsage();
}

void
set_max_utilization(double max_utilization)
{
  ensureLinked();
  Resizer *resizer = getResizer();
  resizer->setMaxUtilization(max_utilization);
}

void
set_dont_use(LibertyCell *lib_cell,
             bool dont_use)
{
  ensureLinked();
  Resizer *resizer = getResizer();
  resizer->setDontUse(lib_cell, dont_use);
}

void
reset_dont_use()
{
  ensureLinked();
  Resizer *resizer = getResizer();
  resizer->resetDontUse();
}

void
set_dont_touch_instance(Instance *inst,
                        bool dont_touch)
{
  ensureLinked();
  Resizer *resizer = getResizer();
  resizer->setDontTouch(inst, dont_touch);
}

void
report_dont_use()
{
  ensureLinked();
  Resizer *resizer = getResizer();
  resizer->reportDontUse();
}

void
report_dont_touch()
{
  ensureLinked();
  Resizer *resizer = getResizer();
  resizer->reportDontTouch();
}

void
set_dont_touch_net(Net *net,
                   bool dont_touch)
{
  ensureLinked();
  Resizer *resizer = getResizer();
  resizer->setDontTouch(net, dont_touch);
}

void
buffer_inputs(LibertyCell *buffer_cell, bool verbose)
{
  ensureLinked();
  Resizer *resizer = getResizer();
  resizer->bufferInputs(buffer_cell, verbose);
}

void
buffer_outputs(LibertyCell *buffer_cell, bool verbose)
{
  ensureLinked();
  Resizer *resizer = getResizer();
  resizer->bufferOutputs(buffer_cell, verbose);
}

void
resize_to_target_slew(const Pin *drvr_pin)
{
  ensureLinked();
  Resizer *resizer = getResizer();
  resizer->resizeDrvrToTargetSlew(drvr_pin);
}

double
resize_target_slew(const RiseFall *rf)
{
  ensureLinked();
  Resizer *resizer = getResizer();
  return resizer->targetSlew(rf);
}

double
resize_target_load_cap(LibertyCell *cell)
{
  ensureLinked();
  Resizer *resizer = getResizer();
  return resizer->targetLoadCap(cell);
}

float
design_area()
{
  ensureLinked();
  Resizer *resizer = getResizer();
  return resizer->designArea();
}

TmpNetSeq *
find_floating_nets()
{
  ensureLinked();
  Resizer *resizer = getResizer();
  return resizer->findFloatingNets();
}

TmpPinSet *
find_floating_pins()
{
  ensureLinked();
  Resizer *resizer = getResizer();
  return resizer->findFloatingPins();
}

TmpNetSeq *
find_overdriven_nets(bool include_parallel_driven)
{
  ensureLinked();
  Resizer *resizer = getResizer();
  return resizer->findOverdrivenNets(include_parallel_driven);
}

void
repair_tie_fanout_cmd(LibertyPort *tie_port,
                      double separation, // meters
                      bool verbose)
{
  ensureLinked();
  Resizer *resizer = getResizer();
  resizer->repairTieFanout(tie_port, separation, verbose);
}

void
repair_design_cmd(double max_length,
                  double slew_margin,
                  double cap_margin,
                  bool pre_placement,
                  bool match_cell_footprint,
                  bool verbose)
{
  ensureLinked();
  Resizer *resizer = getResizer();
  resizer->repairDesign(max_length,
                        slew_margin,
                        cap_margin,
                        pre_placement,
                        match_cell_footprint,
                        verbose);
}

int
repair_design_buffer_count()
{
  Resizer *resizer = getResizer();
  return resizer->repairDesignBufferCount();
}

void
repair_clk_nets_cmd(double max_length)
{
  ensureLinked();
  Resizer *resizer = getResizer();
  resizer->repairClkNets(max_length);
}

void
repair_clk_inverters_cmd()
{
  ensureLinked();
  Resizer *resizer = getResizer();
  resizer->repairClkInverters();
}

void
repair_net_cmd(Net *net,
               double max_length,
               double slew_margin,
               double cap_margin)
{
  ensureLinked();
  Resizer *resizer = getResizer();
  resizer->repairNet(net, max_length, slew_margin, cap_margin);
}

bool
repair_setup(double setup_margin,
             double repair_tns_end_percent,
             int max_passes,
             int max_iterations,
             int max_repairs_per_pass,
             bool match_cell_footprint,
             bool verbose,
             std::vector<rsz::MoveType> sequence,
             bool skip_pin_swap,
             bool skip_gate_cloning,
             bool skip_size_down,
             bool skip_buffering,
             bool skip_buffer_removal,
             bool skip_last_gasp,
             bool skip_vt_swap,
             bool skip_crit_vt_swap)
{
  ensureLinked();
  Resizer *resizer = getResizer();
  return resizer->repairSetup(setup_margin, repair_tns_end_percent,
                       max_passes, max_iterations,
                       max_repairs_per_pass, match_cell_footprint,
                       verbose, sequence,
                       skip_pin_swap, skip_gate_cloning,
                       skip_size_down,
                       skip_buffering, skip_buffer_removal,
                       skip_last_gasp, skip_vt_swap, skip_crit_vt_swap);
}

void
repair_setup_pin_cmd(Pin *end_pin)
{
  ensureLinked();
  Resizer *resizer = getResizer();
  resizer->repairSetup(end_pin);
}

void
report_swappable_pins_cmd()
{
  ensureLinked();
  Resizer *resizer = getResizer();
  resizer->reportSwappablePins();
}

bool
repair_hold(double setup_margin,
            double hold_margin,
            bool allow_setup_violations,
            float max_buffer_percent,
            int max_passes,
            int max_iterations,
            bool match_cell_footprint,
            bool verbose)
{
  ensureLinked();
  Resizer *resizer = getResizer();
  return resizer->repairHold(setup_margin, hold_margin,
                      allow_setup_violations,
                      max_buffer_percent, max_passes,
                      max_iterations, match_cell_footprint,
                      verbose);
}

void
repair_hold_pin(Pin *end_pin,
                double setup_margin,
                double hold_margin,
                bool allow_setup_violations,
                float max_buffer_percent,
                int max_passes)
{
  ensureLinked();
  Resizer *resizer = getResizer();
  resizer->repairHold(end_pin, setup_margin, hold_margin,
                      allow_setup_violations,
                      max_buffer_percent, max_passes);
}

int
hold_buffer_count()
{
  Resizer *resizer = getResizer();
  return resizer->holdBufferCount();
}

////////////////////////////////////////////////////////////////
bool
recover_power(float recover_power_percent, bool match_cell_footprint, bool verbose)
{
  ensureLinked();
  Resizer *resizer = getResizer();
  return resizer->recoverPower(recover_power_percent, match_cell_footprint, verbose);
}

////////////////////////////////////////////////////////////////

// Rebuffer one net (for testing).
// resizerPreamble() required.
void
rebuffer_net(const Pin *drvr_pin)
{
  ensureLinked();
  Resizer *resizer = getResizer();
  resizer->rebufferNet(drvr_pin);
}

void
report_long_wires_cmd(int count,
                      int digits)
{
  ensureLinked();
  Resizer *resizer = getResizer();
  return resizer->reportLongWires(count, digits);
}

////////////////////////////////////////////////////////////////

void
resize_slack_preamble()
{
  ensureLinked();
  Resizer *resizer = getResizer();
  resizer->resizeSlackPreamble();
}

void
find_resize_slacks()
{
  Resizer *resizer = getResizer();
  resizer->findResizeSlacks(true);
}

TmpNetSeq *
resize_worst_slack_nets()
{
  Resizer *resizer = getResizer();
  TmpNetSeq *seq = new TmpNetSeq;
  *seq = resizer->resizeWorstSlackNets();
  return seq;
}

float
resize_net_slack(Net *net)
{
  Resizer *resizer = getResizer();
  return resizer->resizeNetSlack(net).value();
}

////////////////////////////////////////////////////////////////

float
buffer_wire_delay(LibertyCell *buffer_cell,
                  float wire_length) // meters
{
  ensureLinked();
  Resizer *resizer = getResizer();
  Delay delay;
  Slew slew;
  resizer->bufferWireDelay(buffer_cell, wire_length,
                           delay, slew);
  return delay;
}

double
find_max_wire_length()
{
  ensureLinked();
  Resizer *resizer = getResizer();
  return resizer->findMaxWireLength();
}

double
find_buffer_max_wire_length(LibertyCell *buffer_cell,
                            const Corner *corner)
{
  ensureLinked();
  Resizer *resizer = getResizer();
  return resizer->findMaxWireLength(buffer_cell, corner);
}

double
default_max_slew()
{
  ensureLinked();
  Resizer *resizer = getResizer();
  sta::Network *network = resizer->network();
  sta::LibertyLibrary *lib = network->defaultLibertyLibrary();
  float max_slew = 0.0;
  if (lib) {
    bool exists;
    lib->defaultMaxSlew(max_slew, exists);
  }
  return max_slew;
}

// In meters
double
max_load_manhatten_distance(Net *net)
{
  ensureLinked();
  Resizer *resizer = getResizer();
  return resizer->maxLoadManhattenDistance(net);
}

double
utilization()
{
  ensureLinked();
  Resizer *resizer = getResizer();
  return resizer->utilization();
}

PinSet
find_fanin_fanouts(PinSet* pins)
{
  Resizer *resizer = getResizer();
  return resizer->findFaninFanouts(*pins);
}

void
set_debug_pin(const Pin *pin)
{
  Resizer *resizer = getResizer();
  resizer->setDebugPin(pin);
}

void
set_worst_slack_nets_percent(float percent)
{
  Resizer *resizer = getResizer();
  resizer->setWorstSlackNetsPercent(percent);
}

void
eliminate_dead_logic_cmd(bool clean_nets)
{
  ensureLinked();
  Resizer *resizer = getResizer();
  resizer->eliminateDeadLogic(clean_nets);
}

void report_equiv_cells_cmd(LibertyCell* cell, bool match_cell_footprint,
                            bool report_all_cells, bool report_vt_equiv)
{
  ensureLinked();
  Resizer* resizer = getResizer();
  resizer->reportEquivalentCells(cell, match_cell_footprint, report_all_cells,
                                 report_vt_equiv);
}

void report_buffers_cmd(bool filtered)
{
  ensureLinked();
  Resizer* resizer = getResizer();
  resizer->reportBuffers(filtered);
}

void
report_fast_buffer_sizes()
{
  Resizer *resizer = getResizer();
  resizer->reportFastBufferSizes();
}

void set_debug_cmd(const char* net_name,
                   const bool subdivide_step)
{
  Resizer* resizer = getResizer();

  odb::dbNet* net = nullptr;
  if (net_name) {
    auto block = ord::OpenRoad::openRoad()->getDb()->getChip()->getBlock();
    net = block->findNet(net_name);
  }

  auto graphics = std::make_shared<Graphics>();
  graphics->setNet(net);
  graphics->stopOnSubdivideStep(subdivide_step);
  resizer->setDebugGraphics(std::move(graphics));
}

void swap_arith_modules_cmd(int path_count,
                            const std::string& target,
                            float slack_margin)
{
  Resizer* resizer = getResizer();
  resizer->swapArithModules(path_count, target, slack_margin);
}

// Test stub
void
fully_rebuffer(Pin *pin)
{
  ensureLinked();
  Resizer *resizer = getResizer();
  resizer->resizeSlackPreamble();
  resizer->fullyRebuffer(pin);
}

Instance*
insert_buffer_after_driver_cmd(Net *net,
                              LibertyCell *buffer_cell,
                              double x, double y,
                              bool has_loc,
                              const char *new_buf_base_name,
                              const char *new_net_base_name)
{
  ensureLinked();
  Resizer *resizer = getResizer();
  resizer->initBlock();
  est::IncrementalParasiticsGuard guard(resizer->getEstimateParasitics());

  Instance* inst = nullptr;
  const char* buf_name = (new_buf_base_name && strcmp(new_buf_base_name, "NULL") != 0) ? new_buf_base_name : nullptr;
  const char* net_name = (new_net_base_name && strcmp(new_net_base_name, "NULL") != 0) ? new_net_base_name : nullptr;

  if (has_loc) {
    odb::Point loc(resizer->metersToDbu(x), resizer->metersToDbu(y));
    inst = resizer->insertBufferAfterDriver(net, buffer_cell, &loc, buf_name, net_name, odb::dbNameUniquifyType::ALWAYS);
  } else {
    inst = resizer->insertBufferAfterDriver(net, buffer_cell, nullptr, buf_name, net_name, odb::dbNameUniquifyType::ALWAYS);
  }
  return inst;
}

Instance*
insert_buffer_before_load_cmd(Pin *load_pin,
                             LibertyCell *buffer_cell,
                             double x, double y,
                             bool has_loc,
                             const char *new_buf_base_name,
                             const char *new_net_base_name)
{
  ensureLinked();
  Resizer *resizer = getResizer();
  resizer->initBlock();
  est::IncrementalParasiticsGuard guard(resizer->getEstimateParasitics());

  Instance* inst = nullptr;
  const char* buf_name = (new_buf_base_name && strcmp(new_buf_base_name, "NULL") != 0) ? new_buf_base_name : nullptr;
  const char* net_name = (new_net_base_name && strcmp(new_net_base_name, "NULL") != 0) ? new_net_base_name : nullptr;

  if (has_loc) {
    odb::Point loc(resizer->metersToDbu(x), resizer->metersToDbu(y));
    inst = resizer->insertBufferBeforeLoad(load_pin, buffer_cell, &loc, buf_name, net_name, odb::dbNameUniquifyType::ALWAYS);
  } else {
    inst = resizer->insertBufferBeforeLoad(load_pin, buffer_cell, nullptr, buf_name, net_name, odb::dbNameUniquifyType::ALWAYS);
  }
  return inst;
}

Instance*
insert_buffer_before_loads_cmd(Net *net,
                              PinSet *pins,
                              LibertyCell *buffer_cell,
                              double x, double y,
                              bool has_loc,
                              const char *new_buf_base_name,
                              const char *new_net_base_name,
                              bool loads_on_diff_nets)
{
  ensureLinked();
  Resizer *resizer = getResizer();
  resizer->initBlock();
  est::IncrementalParasiticsGuard guard(resizer->getEstimateParasitics());
  
  Instance* inst = nullptr;
  const char* buf_name = (new_buf_base_name && strcmp(new_buf_base_name, "NULL") != 0) ? new_buf_base_name : nullptr;
  const char* net_name = (new_net_base_name && strcmp(new_net_base_name, "NULL") != 0) ? new_net_base_name : nullptr;

  if (has_loc) {
    odb::Point loc(resizer->metersToDbu(x), resizer->metersToDbu(y));
    inst = resizer->insertBufferBeforeLoads(net, pins, buffer_cell, &loc, buf_name, net_name, odb::dbNameUniquifyType::ALWAYS, loads_on_diff_nets);
  } else {
    inst = resizer->insertBufferBeforeLoads(net, pins, buffer_cell, nullptr, buf_name, net_name, odb::dbNameUniquifyType::ALWAYS, loads_on_diff_nets);
  }

  delete pins;
  return inst;
}

void
set_clock_buffer_string_cmd(const char* clk_str)
{
  ensureLinked();
  Resizer *resizer = getResizer();
  resizer->setClockBufferString(clk_str);
}

void
set_clock_buffer_footprint_cmd(const char* footprint)
{
  ensureLinked();
  Resizer *resizer = getResizer();
  resizer->setClockBufferFootprint(footprint);
}

void
reset_clock_buffer_pattern_cmd()
{
  ensureLinked();
  Resizer *resizer = getResizer();
  resizer->resetClockBufferPattern();
}

bool
has_clock_buffer_string_cmd()
{
  Resizer *resizer = getResizer();
  return resizer->hasClockBufferString();
}

bool
has_clock_buffer_footprint_cmd()
{
  Resizer *resizer = getResizer();
  return resizer->hasClockBufferFootprint();
}

const char*
get_clock_buffer_string_cmd()
{
  Resizer *resizer = getResizer();
  return resizer->getClockBufferString().c_str();
}

const char*
get_clock_buffer_footprint_cmd()
{
  Resizer *resizer = getResizer();
  return resizer->getClockBufferFootprint().c_str();
}

void check_slew_after_buffer_rm(Pin *drvr_pin, Instance *buffer_instance, const Corner *corner)
{
  ensureLinked();
  Resizer *resizer = getResizer();
  std::map<const Pin*, float> load_pin_slew;
  
  ArcDelay old_delay[RiseFall::index_count];
  ArcDelay new_delay[RiseFall::index_count];
  Slew old_drvr_slew[RiseFall::index_count];
  Slew new_drvr_slew[RiseFall::index_count];
  float old_cap, new_cap;
  resizer->resizeSlackPreamble();
  if (!resizer->computeNewDelaysSlews(drvr_pin,
                                      buffer_instance,
                                      corner,
                                      old_delay,
                                      new_delay,
                                      old_drvr_slew,
                                      new_drvr_slew,
                                      old_cap,
                                      new_cap)) {  
    return;
  }
  (void) resizer->estimateSlewsAfterBufferRemoval
    (drvr_pin, buffer_instance, std::max(new_drvr_slew[RiseFall::riseIndex()],
                                         new_drvr_slew[RiseFall::fallIndex()]),
     corner, load_pin_slew);
}

} // namespace

%} // inline
