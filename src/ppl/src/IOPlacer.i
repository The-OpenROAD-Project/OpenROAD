/////////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2019, The Regents of the University of California
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
#include "ord/OpenRoad.hh"

namespace ord {
// Defined in OpenRoad.i
ppl::IOPlacer* getIOPlacer();
} // namespace ord

using ord::getIOPlacer;
using ppl::Edge;
using ppl::Direction;
using ppl::PinList;
using ppl::PinGroup;
using std::vector;
using std::set;

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

template <class TYPE>
set<TYPE> *
tclSetStdSeq(Tcl_Obj *const source,
        swig_type_info *swig_type,
        Tcl_Interp *interp)
{
  int argc;
  Tcl_Obj **argv;

  if (Tcl_ListObjGetElements(interp, source, &argc, &argv) == TCL_OK
      && argc > 0) {
    set<TYPE> *seq = new set<TYPE>;
    for (int i = 0; i < argc; i++) {
      void *obj;
      // Ignore returned TCL_ERROR because can't get swig_type_info.
      SWIG_ConvertPtr(argv[i], &obj, swig_type, false);
      seq->insert(reinterpret_cast<TYPE>(obj));
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

%typemap(in) PinGroup* {
  $1 = tclListStdSeq<odb::dbBTerm*>($input, SWIGTYPE_p_odb__dbBTerm, interp);
}

%typemap(in) PinList* {
  $1 = tclSetStdSeq<odb::dbBTerm*>($input, SWIGTYPE_p_odb__dbBTerm, interp);
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

void add_direction_constraint(Direction direction, Edge edge,
                               int begin, int end)
{
  getIOPlacer()->addDirectionConstraint(direction, edge, begin, end);
}

void
add_top_layer_constraint(PinList *pin_list,
                         int x1, int y1,
                         int x2, int y2)
{
  getIOPlacer()->addTopLayerConstraint(pin_list, x1, y1, x2, y2);
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
add_pin_group(PinGroup *pin_group)
{
  getIOPlacer()->addPinGroup(pin_group);
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
set_min_distance_in_tracks(bool in_tracks)
{
  getIOPlacer()->getParameters()->setMinDistanceInTracks(in_tracks);
}

void
create_pin_shape_pattern(int layer, int x_step, int y_step,
                         int llx, int lly, int urx, int ury,
                         int width, int height, int keepout)
{
  getIOPlacer()->addTopLayerPinPattern(layer, x_step, y_step, llx, lly, urx, ury, width, height, keepout);
}

int
get_top_layer()
{
  return getIOPlacer()->getTopLayer();
}

void
place_pin(odb::dbBTerm* bterm, int layer,
          int x, int y, int width, int height,
          bool force_to_die_bound)
{
  getIOPlacer()->placePin(bterm, layer, x, y, width, height, force_to_die_bound);
}

void
clear()
{
  getIOPlacer()->clear();
}

void
clear_constraints()
{
  getIOPlacer()->clearConstraints();
}

} // namespace

%} // inline
