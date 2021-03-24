/////////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2019, University of California, San Diego.
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
#include "ppl/IOPlacer.h"
#include "Parameters.h"
#include "openroad/OpenRoad.hh"

namespace ord {
// Defined in OpenRoad.i
ppl::IOPlacer* getIOPlacer();
} // namespace ord

using ord::getIOPlacer;
using ppl::Edge;
using ppl::Direction;
using ppl::PinList;
using std::vector;

template <class TYPE>
vector<TYPE> *
tclListStdSeq(Tcl_Obj *const source,
	      swig_type_info *swig_type,
	      Tcl_Interp *interp)
{
  int argc;
  Tcl_Obj **argv;

  if (Tcl_ListObjGetElements(interp, source, &argc, &argv) == TCL_OK
      && argc > 0) {
    vector<TYPE> *seq = new vector<TYPE>;
    for (int i = 0; i < argc; i++) {
      void *obj;
      // Ignore returned TCL_ERROR because can't get swig_type_info.
      SWIG_ConvertPtr(argv[i], &obj, swig_type, false);
      seq->push_back(reinterpret_cast<TYPE>(obj));
    }
    return seq;
  }
  else
    return nullptr;
}

%}

////////////////////////////////////////////////////////////////
//
// SWIG type definitions.
//
////////////////////////////////////////////////////////////////

%typemap(in) PinList* {
  $1 = tclListStdSeq<odb::dbBTerm*>($input, SWIGTYPE_p_odb__dbBTerm, interp);
}

%include "../../Exception.i"

%inline %{

namespace ppl {

void
set_num_slots(int numSlots)
{
  getIOPlacer()->getParameters()->setNumSlots(numSlots);
}

void
set_slots_factor(float factor)
{
  getIOPlacer()->getParameters()->setSlotsFactor(factor);
}

void
set_force_spread(bool force)
{
  getIOPlacer()->getParameters()->setForceSpread(force);
}

void
set_usage_factor(float usage)
{
  getIOPlacer()->getParameters()->setUsageFactor(usage);
}

Edge
get_edge(const char* edge)
{
  return getIOPlacer()->getEdge(edge);
}

Direction
get_direction(const char* direction)
{
  return getIOPlacer()->getDirection(direction);
}

void
exclude_interval(Edge edge, int begin, int end)
{
  getIOPlacer()->excludeInterval(edge, begin, end);
}

void
add_names_constraint(PinList *pin_list, Edge edge, int begin, int end)
{
  getIOPlacer()->addNamesConstraint(pin_list, edge, begin, end);
}

PinList* create_names_constraint(Edge edge, int begin, int end)
{
  return getIOPlacer()->createNamesConstraint(edge, begin, end);
}

void add_direction_constraint(Direction direction, Edge edge,
                               int begin, int end)
{
  getIOPlacer()->addDirectionConstraint(direction, edge, begin, end);
}

void
set_hor_length(float length)
{
  getIOPlacer()->getParameters()->setHorizontalLength(length);
}

void
set_ver_length_extend(float length)
{
  getIOPlacer()->getParameters()->setVerticalLengthExtend(length);
}

void
set_hor_length_extend(float length)
{
  getIOPlacer()->getParameters()->setHorizontalLengthExtend(length);
}

void
set_ver_length(float length)
{
  getIOPlacer()->getParameters()->setVerticalLength(length);
}

void
add_hor_layer(int layer)
{
  getIOPlacer()->addHorLayer(layer);
}

void
add_ver_layer(int layer)
{
  getIOPlacer()->addVerLayer(layer);
}

void
add_pin_group(PinList *pin_list)
{
  getIOPlacer()->addPinGroup(pin_list);
}

PinList*
create_pin_group()
{
  return getIOPlacer()->createPinGroup();
}

void
add_pin_to_list(odb::dbBTerm* pin, PinList* pin_group)
{
  getIOPlacer()->addPinToList(pin, pin_group);
}

void
run_io_placement(bool randomMode)
{
  getIOPlacer()->run(randomMode);
}

void
set_report_hpwl(bool report)
{
  getIOPlacer()->getParameters()->setReportHPWL(report);
}

int
compute_io_nets_hpwl()
{
  return getIOPlacer()->returnIONetsHPWL();
}

void
set_rand_seed(double seed)
{
  getIOPlacer()->getParameters()->setRandSeed(seed);
}

void
set_hor_thick_multiplier(float length)
{
  getIOPlacer()->getParameters()->setHorizontalThicknessMultiplier(length);
}

void
set_ver_thick_multiplier(float length)
{
  getIOPlacer()->getParameters()->setVerticalThicknessMultiplier(length);
}

void
set_corner_avoidance(int offset)
{
  getIOPlacer()->getParameters()->setCornerAvoidance(offset);
}

void
set_min_distance(int minDist)
{
  getIOPlacer()->getParameters()->setMinDistance(minDist);
}

void
clear()
{
  getIOPlacer()->clear();
}

} // namespace

%} // inline
