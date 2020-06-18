%module resizer

/////////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2019, James Cherry, Parallax Software, Inc.
// All rights reserved.
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

#include "sta/Error.hh"
#include "sta/Liberty.hh"
#include "resizer/Resizer.hh"
#include "sta/Delay.hh"
#include "sta/Liberty.hh"

namespace ord {
// Defined in OpenRoad.i
sta::Resizer *
getResizer();
void
ensureLinked();
}

namespace sta {

Point
pinLocation(const Pin *pin,
	    const dbNetwork *network);

// Defined in StaTcl.i
LibertyLibrarySeq *
tclListSeqLibertyLibrary(Tcl_Obj *const source,
			 Tcl_Interp *interp);
LibertyCellSeq *
tclListSeqLibertyCell(Tcl_Obj *const source,
		      Tcl_Interp *interp);

} // namespace

using ord::getResizer;
using ord::ensureLinked;

using sta::Resizer;
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
using sta::LibertyPort;
using sta::Delay;
using sta::Slew;

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

%typemap(out) NetSeq* {
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

double
utilization()
{
  ensureLinked();
  Resizer *resizer = getResizer();
  return resizer->utilization();
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

// ohms/meter
double
wire_resistance()
{
  ensureLinked();
  Resizer *resizer = getResizer();
  return resizer->wireResistance();
}

// farads/meter
double
wire_capacitanceb()
{
  ensureLinked();
  Resizer *resizer = getResizer();
  return resizer->wireCapacitance();
}

void
estimate_wire_parasitics()
{
  ensureLinked();
  Resizer *resizer = getResizer();
  resizer->estimateWireParasitics();
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
}

void
resizer_preamble(LibertyLibrarySeq *resize_libs)
{
  ensureLinked();
  Resizer *resizer = getResizer();
  resizer->resizePreamble(resize_libs);
}

void
buffer_inputs(LibertyCell *buffer_cell)
{
  ensureLinked();
  Resizer *resizer = getResizer();
  resizer->bufferInputs(buffer_cell);
}

void
buffer_outputs(LibertyCell *buffer_cell)
{
  ensureLinked();
  Resizer *resizer = getResizer();
  resizer->bufferOutputs(buffer_cell);
}

void
resize_to_target_slew()
{
  ensureLinked();
  Resizer *resizer = getResizer();
  resizer->resizeToTargetSlew();
}

void
repair_max_cap_cmd(LibertyCell *buffer_cell)
{
  ensureLinked();
  Resizer *resizer = getResizer();
  resizer->repairMaxCap(buffer_cell);
}

void
repair_max_slew_cmd(LibertyCell *buffer_cell)
{
  ensureLinked();
  Resizer *resizer = getResizer();
  resizer->repairMaxSlew(buffer_cell);
}

void
repair_max_fanout_cmd(LibertyCell *buffer_cell)
{
  ensureLinked();
  Resizer *resizer = getResizer();
  resizer->repairMaxFanout(buffer_cell);
}

void
resize_driver_to_target_slew(const Pin *drvr_pin)
{
  ensureLinked();
  Resizer *resizer = getResizer();
  resizer->resizeToTargetSlew(drvr_pin);
}

// for testing
void
rebuffer_net(Net *net,
	     LibertyCell *buffer_cell)
{
  ensureLinked();
  Resizer *resizer = getResizer();
  LibertyLibrarySeq *resize_libs = new LibertyLibrarySeq;
  resize_libs->push_back(buffer_cell->libertyLibrary());
  resizer->resizePreamble(resize_libs);
  resizer->rebuffer(net, buffer_cell);
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

void
repair_pin_hold_violations(Pin *end_pin,
			   LibertyCell *buffer_cell)
{
  ensureLinked();
  Resizer *resizer = getResizer();
  resizer->repairHoldViolations(end_pin, buffer_cell);
}

void
repair_hold_violations_cmd(LibertyCell *buffer_cell)
{
  ensureLinked();
  Resizer *resizer = getResizer();
  resizer->repairHoldViolations(buffer_cell);
}

float
design_area()
{
  ensureLinked();
  Resizer *resizer = getResizer();
  return resizer->designArea();
}

NetSeq *
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
repair_long_wires_cmd(float max_length,
		      LibertyCell *buffer_cell)
{
  ensureLinked();
  Resizer *resizer = getResizer();
  return resizer->repairLongWires(max_length, buffer_cell);
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
find_max_wire_length(LibertyCell *buffer_cell)
{
  ensureLinked();
  Resizer *resizer = getResizer();
  return resizer->findMaxWireLength(buffer_cell);
}

double
find_max_slew_wire_length(float max_slew,
			  LibertyCell *buffer_cell)
{
  ensureLinked();
  Resizer *resizer = getResizer();
  return resizer->findMaxSlewWireLength(max_slew, buffer_cell);
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

void
write_net_svg(Net *net,
	      const char *filename)
{
  ensureLinked();
  Resizer *resizer = getResizer();
  resizer->writeNetSVG(net, filename);
}

const char *
pin_location(Pin *pin)
{
  ensureLinked();
  Resizer *resizer = getResizer();
  odb::Point loc = sta::pinLocation(pin, resizer->getDbNetwork());
  // return x/y as tcl list
  return sta::stringPrintTmp("%d %d", loc.getX(), loc.getY());
}

%} // inline
