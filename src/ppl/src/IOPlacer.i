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
#include "IOPlacerRenderer.h"
#include "ord/OpenRoad.hh"


namespace ord {
// Defined in OpenRoad.i
ppl::IOPlacer* getIOPlacer();
} // namespace ord

using ord::getIOPlacer;
using ppl::Edge;
using ppl::Direction;
using ppl::PinSet;
using ppl::PinList;
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

%typemap(in) PinList* {
  $1 = tclListStdSeq<odb::dbBTerm*>($input, SWIGTYPE_p_odb__dbBTerm, interp);
}

%typemap(in) PinSet* {
  $1 = tclSetStdSeq<odb::dbBTerm*>($input, SWIGTYPE_p_odb__dbBTerm, interp);
}

%include "../../Exception.i"

%ignore ppl::IOPlacer::getRenderer;
%ignore ppl::IOPlacer::setRenderer;

%inline %{

namespace ppl {

void
set_slots_per_section(int slots_per_section)
{
  getIOPlacer()->getParameters()->setSlotsPerSection(slots_per_section);
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
add_names_constraint(PinSet *pin_list, Edge edge, int begin, int end)
{
  getIOPlacer()->addNamesConstraint(pin_list, edge, begin, end);
}

void add_direction_constraint(Direction direction, Edge edge,
                               int begin, int end)
{
  getIOPlacer()->addDirectionConstraint(direction, edge, begin, end);
}

void
add_top_layer_constraint(PinSet *pin_list,
                         int x1, int y1,
                         int x2, int y2)
{
  getIOPlacer()->addTopLayerConstraint(pin_list, odb::Rect(x1, y1, x2, y2));
}

void
add_mirrored_pins(odb::dbBTerm* bterm1, odb::dbBTerm* bterm2)
{
  getIOPlacer()->addMirroredPins(bterm1, bterm2);
}

void
set_hor_length(int length)
{
  getIOPlacer()->getParameters()->setHorizontalLength(length);
}

void
set_ver_length_extend(int length)
{
  getIOPlacer()->getParameters()->setVerticalLengthExtend(length);
}

void
set_hor_length_extend(int length)
{
  getIOPlacer()->getParameters()->setHorizontalLengthExtend(length);
}

void
set_ver_length(int length)
{
  getIOPlacer()->getParameters()->setVerticalLength(length);
}

void
add_hor_layer(odb::dbTechLayer* layer)
{
  getIOPlacer()->addHorLayer(layer);
}

void
add_ver_layer(odb::dbTechLayer* layer)
{
  getIOPlacer()->addVerLayer(layer);
}

void
add_pin_group(PinList *pin_list, bool order)
{
  getIOPlacer()->addPinGroup(pin_list, order);
}

void
run_hungarian_matching(bool randomMode)
{
  getIOPlacer()->runHungarianMatching(randomMode);
}

void
set_report_hpwl(bool report)
{
  getIOPlacer()->getParameters()->setReportHPWL(report);
}

int
compute_io_nets_hpwl()
{
  return getIOPlacer()->computeIONetsHPWL();
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

void set_pin_placement_file(const char* file_name)
{
  getIOPlacer()->getParameters()->setPinPlacementFile(file_name);
}

void
create_pin_shape_pattern(odb::dbTechLayer* layer, int x_step, int y_step,
                         const odb::Rect& region,
                         int pin_width, int pin_height, int keepout)
{
  getIOPlacer()->addTopLayerPinPattern(layer, x_step, y_step, region,
                                       pin_width, pin_height, keepout);
}

odb::dbTechLayer*
get_top_layer()
{
  return getIOPlacer()->getTopLayer();
}

void
place_pin(odb::dbBTerm* bterm, odb::dbTechLayer* layer,
          int x, int y, int width, int height,
          bool force_to_die_bound, bool placed_status)
{
  getIOPlacer()->placePin(bterm, layer, x, y, width, height, force_to_die_bound, placed_status);
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

void
set_simulated_annealing(float temperature,
                        int max_iterations,
                        int perturb_per_iter,
                        float alpha)
{
  getIOPlacer()->setAnnealingConfig(temperature, max_iterations, perturb_per_iter, alpha);
}

void
simulated_annealing_debug(int iters_between_paintings,
                          bool no_pause_mode)
{
  if (!gui::Gui::enabled()) {
    return;
  }

  IOPlacer* ioplacer = getIOPlacer();
  if(ioplacer->getRenderer() == nullptr) {
    ioplacer->setRenderer(std::make_unique<IOPlacerRenderer>());
  }

  getIOPlacer()->setAnnealingDebugOn();
  getIOPlacer()->setAnnealingDebugNoPauseMode(no_pause_mode);
  getIOPlacer()->setAnnealingDebugPaintInterval(iters_between_paintings);
}

void
run_annealing(bool random)
{
  getIOPlacer()->runAnnealing(random);
}

void
write_pin_placement(const char* file_name, bool placed)
{
  getIOPlacer()->writePinPlacement(file_name, placed);
}

} // namespace

%} // inline
