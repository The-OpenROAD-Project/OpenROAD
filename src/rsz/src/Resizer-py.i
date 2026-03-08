// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025, The OpenROAD Authors

%{

#include "rsz/Resizer.hh"
#include "ord/OpenRoad.hh"

namespace ord {
rsz::Resizer* getResizer();
void ensureLinked();
}

using ord::getResizer;
using ord::ensureLinked;

%}

%include <std_string.i>
%include "../../Exception-py.i"

// Forward-declare STA types as opaque pointers so SWIG generates correct
// wrappers for functions that accept them.  Both constructor and destructor
// are suppressed because these objects are owned by OpenSTA internals.
%nodefaultctor sta::LibertyCell;
%nodefaultdtor sta::LibertyCell;
%nodefaultctor sta::LibertyPort;
%nodefaultdtor sta::LibertyPort;
%nodefaultctor sta::Instance;
%nodefaultdtor sta::Instance;
%nodefaultctor sta::Net;
%nodefaultdtor sta::Net;

namespace sta {
  class LibertyCell {};
  class LibertyPort {};
  class Instance {};
  class Net {};
}

%inline %{

// ---------------------------------------------------------------------------
// Dont-use / Dont-touch
// ---------------------------------------------------------------------------

void set_dont_use(sta::LibertyCell* lib_cell, bool dont_use)
{
  ensureLinked();
  getResizer()->setDontUse(lib_cell, dont_use);
}

void reset_dont_use()
{
  ensureLinked();
  getResizer()->resetDontUse();
}

void set_dont_touch_instance(sta::Instance* inst, bool dont_touch)
{
  ensureLinked();
  getResizer()->setDontTouch(inst, dont_touch);
}

void set_dont_touch_net(sta::Net* net, bool dont_touch)
{
  ensureLinked();
  getResizer()->setDontTouch(net, dont_touch);
}

void report_dont_use()
{
  ensureLinked();
  getResizer()->reportDontUse();
}

void report_dont_touch()
{
  ensureLinked();
  getResizer()->reportDontTouch();
}

// ---------------------------------------------------------------------------
// Port buffering / utilization
// ---------------------------------------------------------------------------

void set_max_utilization(double max_utilization)
{
  ensureLinked();
  getResizer()->setMaxUtilization(max_utilization);
}

void buffer_inputs(sta::LibertyCell* buffer_cell, bool verbose)
{
  ensureLinked();
  getResizer()->bufferInputs(buffer_cell, verbose);
}

void buffer_outputs(sta::LibertyCell* buffer_cell, bool verbose)
{
  ensureLinked();
  getResizer()->bufferOutputs(buffer_cell, verbose);
}

void remove_buffers_cmd()
{
  ensureLinked();
  getResizer()->removeBuffers({});
}

void balance_row_usage_cmd()
{
  ensureLinked();
  getResizer()->balanceRowUsage();
}

// ---------------------------------------------------------------------------
// Design repair
// ---------------------------------------------------------------------------

void repair_design_cmd(double max_length,
                       double slew_margin,
                       double cap_margin,
                       bool pre_placement,
                       bool match_cell_footprint,
                       bool verbose)
{
  ensureLinked();
  getResizer()->repairDesign(max_length, slew_margin, cap_margin,
                             pre_placement, match_cell_footprint, verbose);
}

void repair_clk_nets_cmd(double max_length)
{
  ensureLinked();
  getResizer()->repairClkNets(max_length);
}

void repair_clk_inverters_cmd()
{
  ensureLinked();
  getResizer()->repairClkInverters();
}

void repair_tie_fanout_cmd(sta::LibertyPort* tie_port,
                           double separation,
                           bool verbose)
{
  ensureLinked();
  getResizer()->repairTieFanout(tie_port, separation, verbose);
}

// ---------------------------------------------------------------------------
// Timing repair
// ---------------------------------------------------------------------------

bool repair_setup(double setup_margin,
                  double repair_tns_end_percent,
                  int max_passes,
                  int max_iterations,
                  int max_repairs_per_pass,
                  bool match_cell_footprint,
                  bool verbose,
                  const char* sequence,
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
  auto move_seq = rsz::Resizer::parseMoveSequence(std::string(sequence ? sequence : ""));
  return getResizer()->repairSetup(setup_margin, repair_tns_end_percent,
                                   max_passes, max_iterations,
                                   max_repairs_per_pass, match_cell_footprint,
                                   verbose, move_seq,
                                   skip_pin_swap, skip_gate_cloning,
                                   skip_size_down, skip_buffering,
                                   skip_buffer_removal, skip_last_gasp,
                                   skip_vt_swap, skip_crit_vt_swap);
}

bool repair_hold(double setup_margin,
                 double hold_margin,
                 bool allow_setup_violations,
                 float max_buffer_percent,
                 int max_passes,
                 int max_iterations,
                 bool match_cell_footprint,
                 bool verbose)
{
  ensureLinked();
  return getResizer()->repairHold(setup_margin, hold_margin,
                                  allow_setup_violations, max_buffer_percent,
                                  max_passes, max_iterations,
                                  match_cell_footprint, verbose);
}

bool recover_power(float recover_power_percent,
                   bool match_cell_footprint,
                   bool verbose)
{
  ensureLinked();
  return getResizer()->recoverPower(recover_power_percent,
                                    match_cell_footprint, verbose);
}

// ---------------------------------------------------------------------------
// Reporting
// ---------------------------------------------------------------------------

double design_area()
{
  ensureLinked();
  return getResizer()->designArea();
}

double utilization()
{
  ensureLinked();
  return getResizer()->utilization();
}

int find_floating_nets_count()
{
  ensureLinked();
  auto* nets = getResizer()->findFloatingNets();
  int count = nets ? static_cast<int>(nets->size()) : 0;
  delete nets;
  return count;
}

int find_floating_pins_count()
{
  ensureLinked();
  auto* pins = getResizer()->findFloatingPins();
  int count = pins ? static_cast<int>(pins->size()) : 0;
  delete pins;
  return count;
}

int find_overdriven_nets_count(bool include_parallel_driven)
{
  ensureLinked();
  auto* nets = getResizer()->findOverdrivenNets(include_parallel_driven);
  int count = nets ? static_cast<int>(nets->size()) : 0;
  delete nets;
  return count;
}

void report_long_wires_cmd(int count, int digits)
{
  ensureLinked();
  getResizer()->reportLongWires(count, digits);
}

void eliminate_dead_logic_cmd(bool clean_nets)
{
  ensureLinked();
  getResizer()->eliminateDeadLogic(clean_nets);
}

%}
