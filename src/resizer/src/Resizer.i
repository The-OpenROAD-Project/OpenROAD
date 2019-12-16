%module resizer

// OpenSTA, Static Timing Analyzer
// Copyright (c) 2019, Parallax Software, Inc.
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

%{

#include "Machine.hh"
#include "Error.hh"
#include "Liberty.hh"
#include "resizer/Resizer.hh"

namespace ord {
// Defined in OpenRoad.i
sta::Resizer *
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

void
networkLibertyLibraries(Network *network,
			// Return value.
			LibertyLibrarySeq &libs)
{
  
  LibertyLibraryIterator *lib_iter = network->libertyLibraryIterator();
  while (lib_iter->hasNext()) {
    LibertyLibrary *lib = lib_iter->next();
    libs.push_back(lib);
  }
  delete lib_iter;
}

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
using sta::RiseFall;
using sta::tclListSeqLibertyLibrary;
using sta::tclListSeqLibertyCell;

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

////////////////////////////////////////////////////////////////
//
// C++ functions visible as TCL functions.
//
////////////////////////////////////////////////////////////////

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
set_max_utilization(double max_utilization)
{
  ensureLinked();
  Resizer *resizer = getResizer();
  resizer->setMaxUtilization(max_utilization);
}

void
set_dont_use(LibertyCellSeq *dont_use)
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
rebuffer_nets(bool repair_max_cap,
	      bool repair_max_slew,
	      LibertyCell *buffer_cell)
{
  ensureLinked();
  Resizer *resizer = getResizer();
  resizer->rebufferNets(repair_max_cap, repair_max_slew, buffer_cell);
}

void
resize_instance_to_target_slew(Instance *inst)
{
  ensureLinked();
  Resizer *resizer = getResizer();
  resizer->resizeToTargetSlew(inst);
}

void
rebuffer_net(Net *net,
	     LibertyCell *buffer_cell)
{
  ensureLinked();
  Resizer *resizer = getResizer();
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

float
design_area()
{
  ensureLinked();
  Resizer *resizer = getResizer();
  return resizer->designArea();
}

%} // inline
