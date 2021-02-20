%module resizer

/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2019, OpenROAD
// All rights reserved.
//
// BSD 3-Clause License
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
///////////////////////////////////////////////////////////////////////////////

%{

#include <cstdint>

#include "sta/Liberty.hh"
#include "rsz/Resizer.hh"
#include "sta/Delay.hh"
#include "sta/Liberty.hh"

namespace ord {
// Defined in OpenRoad.i
rsz::Resizer *
getResizer();
void
ensureLinked();
}

namespace sta {

// Defined in StaTcl.i
LibertyLibrarySeq *
tclListSeqLibertyLibrary(Tcl_Obj *const source,
                         Tcl_Interp *interp);
LibertyCellSeq *
tclListSeqLibertyCell(Tcl_Obj *const source,
                      Tcl_Interp *interp);

typedef NetSeq TmpNetSeq;

} // namespace

using ord::getResizer;
using ord::ensureLinked;

using sta::Corner;
using sta::LibertyCellSeq;
using sta::LibertyLibrarySeq;
using sta::LibertyCell;
using sta::Instance;
using sta::Net;
using sta::Pin;
using sta::RiseFall;
using sta::tclListSeqLibertyLibrary;
using sta::tclListSeqLibertyCell;
using sta::NetSeq;
using sta::TmpNetSeq;
using sta::LibertyPort;
using sta::Delay;
using sta::Slew;
using sta::dbNetwork;

using rsz::Resizer;

%}

////////////////////////////////////////////////////////////////
//
// SWIG type definitions.
// (copied from StaTcl.i because I don't see how to share them.
//
////////////////////////////////////////////////////////////////

%typemap(in) RiseFall* {
  int length;
  const char *arg = Tcl_GetStringFromObj($input, &length);
  RiseFall *tr = RiseFall::find(arg);
  if (tr == nullptr) {
    Tcl_SetResult(interp,const_cast<char*>("Error: unknown transition name."),
                  TCL_STATIC);
    return TCL_ERROR;
  }
  $1 = tr;
}

%typemap(in) LibertyLibrarySeq* {
  $1 = tclListSeqLibertyLibrary($input, interp);
}

%typemap(in) LibertyCellSeq* {
  $1 = tclListSeqLibertyCell($input, interp);
}

%typemap(out) TmpNetSeq* {
  Tcl_Obj *list = Tcl_NewListObj(0, nullptr);
  NetSeq *nets = $1;
  NetSeq::Iterator net_iter(nets);
  while (net_iter.hasNext()) {
    Net *net = net_iter.next();
    Tcl_Obj *obj = SWIG_NewInstanceObj(net, SWIGTYPE_p_Net, false);
    Tcl_ListObjAppendElement(interp, list, obj);
  }
  delete nets;
  Tcl_SetObjResult(interp, list);
}

%typemap(out) NetSeq* {
  Tcl_Obj *list = Tcl_NewListObj(0, nullptr);
  NetSeq *nets = $1;
  NetSeq::Iterator net_iter(nets);
  while (net_iter.hasNext()) {
    Net *net = net_iter.next();
    Tcl_Obj *obj = SWIG_NewInstanceObj(net, SWIGTYPE_p_Net, false);
    Tcl_ListObjAppendElement(interp, list, obj);
  }
  Tcl_SetObjResult(interp, list);
}

%typemap(out) LibertyPort* {
  Tcl_Obj *obj = SWIG_NewInstanceObj($1, $1_descriptor, false);
  Tcl_SetObjResult(interp, obj);
}

////////////////////////////////////////////////////////////////
//
// C++ functions visible as TCL functions.
//
////////////////////////////////////////////////////////////////

%include "../../Exception.i"

%inline %{

namespace rsz {

void
remove_buffers_cmd()
{
  ensureLinked();
  Resizer *resizer = getResizer();
  resizer->removeBuffers();
}

void
set_wire_rc_cmd(float res,
                float cap,
                Corner *corner)
{
  ensureLinked();
  Resizer *resizer = getResizer();
  resizer->setWireRC(res, cap, corner);
}

void
set_wire_clk_rc_cmd(float res,
                    float cap,
                    Corner *corner)
{
  ensureLinked();
  Resizer *resizer = getResizer();
  resizer->setWireClkRC(res, cap, corner);
}

// ohms/meter
double
wire_resistance()
{
  ensureLinked();
  Resizer *resizer = getResizer();
  return resizer->wireResistance();
}

double
wire_clk_resistance()
{
  ensureLinked();
  Resizer *resizer = getResizer();
  return resizer->wireClkResistance();
}

// farads/meter
double
wire_capacitance()
{
  ensureLinked();
  Resizer *resizer = getResizer();
  return resizer->wireCapacitance();
}

double
wire_clk_capacitance()
{
  ensureLinked();
  Resizer *resizer = getResizer();
  return resizer->wireClkCapacitance();
}

void
estimate_parasitics_cmd()
{
  ensureLinked();
  Resizer *resizer = getResizer();
  resizer->estimateWireParasitics();
}

// For debugging. Does not protect against annotating power/gnd.
void
estimate_parasitic_net(const Net *net)
{
  ensureLinked();
  Resizer *resizer = getResizer();
  resizer->estimateWireParasitic(net);
}

bool
have_estimated_parasitics()
{
  Resizer *resizer = getResizer();
  return resizer->haveEstimatedParasitics();
}

void
set_max_utilization(double max_utilization)
{
  ensureLinked();
  Resizer *resizer = getResizer();
  resizer->setMaxUtilization(max_utilization);
}

void
set_dont_use_cmd(LibertyCellSeq *dont_use)
{
  ensureLinked();
  Resizer *resizer = getResizer();
  resizer->setDontUse(dont_use);
  delete dont_use;
}

void
resizer_preamble(LibertyLibrarySeq *resize_libs)
{
  ensureLinked();
  Resizer *resizer = getResizer();
  resizer->resizePreamble(resize_libs);
  delete resize_libs;
}

void
buffer_inputs()
{
  ensureLinked();
  Resizer *resizer = getResizer();
  resizer->bufferInputs();
}

void
buffer_outputs()
{
  ensureLinked();
  Resizer *resizer = getResizer();
  resizer->bufferOutputs();
}

void
resize_to_target_slew()
{
  ensureLinked();
  Resizer *resizer = getResizer();
  resizer->resizeToTargetSlew();
}

void
resize_driver_to_target_slew(const Pin *drvr_pin)
{
  ensureLinked();
  Resizer *resizer = getResizer();
  resizer->resizeToTargetSlew(drvr_pin);
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
repair_design_cmd(float max_length)
{
  ensureLinked();
  Resizer *resizer = getResizer();
  resizer->repairDesign(max_length);
}

void
repair_clk_nets_cmd(float max_length)
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
               float max_length)
{
  ensureLinked();
  Resizer *resizer = getResizer();
  resizer->repairNet(net, max_length); 
}

void
repair_setup(float slack_margin)
{
  ensureLinked();
  Resizer *resizer = getResizer();
  resizer->repairSetup(slack_margin);
}

void
repair_setup_pin(Pin *end_pin)
{
  ensureLinked();
  Resizer *resizer = getResizer();
  resizer->repairSetup(end_pin);
}

void
repair_hold_pin(Pin *end_pin,
                LibertyCell *buffer_cell,
                bool allow_setup_violations)
{
  ensureLinked();
  Resizer *resizer = getResizer();
  resizer->repairHold(end_pin, buffer_cell, 0.0, allow_setup_violations, 0.2);
}

void
repair_hold(float slack_margin,
            bool allow_setup_violations,
            float max_buffer_percent)
{
  ensureLinked();
  Resizer *resizer = getResizer();
  resizer->repairHold(slack_margin, allow_setup_violations, max_buffer_percent);
}

////////////////////////////////////////////////////////////////

// for testing
void
rebuffer_net(Net *net)
{
  ensureLinked();
  Resizer *resizer = getResizer();
  resizer->rebuffer(net);
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
  resizer->findResizeSlacks();
}

NetSeq *
resize_worst_slack_nets()
{
  Resizer *resizer = getResizer();
  return &resizer->resizeWorstSlackNets();
}

float
resize_net_slack(Net *net)
{
  Resizer *resizer = getResizer();
  return resizer->resizeNetSlack(net);
}

////////////////////////////////////////////////////////////////

float
buffer_delay(LibertyCell *buffer_cell,
             const RiseFall *rf)
{
  ensureLinked();
  Resizer *resizer = getResizer();
  return resizer->bufferDelay(buffer_cell, rf);
}

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
find_buffer_max_wire_length(LibertyCell *buffer_cell)
{
  ensureLinked();
  Resizer *resizer = getResizer();
  return resizer->findMaxWireLength(buffer_cell);
}

double
find_max_slew_wire_length(LibertyPort *drvr_port,
                          LibertyPort *load_port,
                          float max_slew)
{
  ensureLinked();
  Resizer *resizer = getResizer();
  return resizer->findMaxSlewWireLength(drvr_port, load_port, max_slew);
}

double
find_slew_load_cap(LibertyPort *drvr_port,
                   float max_slew)
{
  ensureLinked();
  Resizer *resizer = getResizer();
  return resizer->findSlewLoadCap(drvr_port, max_slew);
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

} // namespace

void
highlight_steiner_tree(const Net *net)
{
  Resizer *resizer = getResizer();
  resizer->highlightSteiner(net);
}

%} // inline
