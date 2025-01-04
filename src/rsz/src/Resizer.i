/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2019, The Regents of the University of California
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
#include <fstream>

#include "sta/Liberty.hh"
#include "sta/Network.hh"
#include "sta/Corner.hh"
#include "rsz/Resizer.hh"
#include "sta/Delay.hh"
#include "sta/Liberty.hh"
#include "db_sta/dbNetwork.hh"

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

// Defined in StaTcl.i
LibertyCellSeq *
tclListSeqLibertyCell(Tcl_Obj *const source,
                      Tcl_Interp *interp);
PinSet *
tclListSetPin(Tcl_Obj *source,
              Tcl_Interp *interp);

using TmpNetSeq = NetSeq ;
using TmpPinSet = PinSet;

} // namespace

using ord::getResizer;
using ord::ensureLinked;

using sta::Corner;
using sta::LibertyCellSeq;
using sta::LibertyCell;
using sta::Instance;
using sta::InstanceSeq;
using sta::Net;
using sta::NetSeq;
using sta::Pin;
using sta::PinSet;
using sta::TmpPinSet;
using sta::RiseFall;
using sta::tclListSeqLibertyCell;
using sta::tclListSetPin;
using sta::TmpNetSeq;
using sta::LibertyPort;
using sta::Delay;
using sta::Slew;
using sta::dbNetwork;
using sta::Network;
using sta::stringEq;

using rsz::Resizer;
using rsz::ParasiticsSrc;

template <class SET_TYPE, class OBJECT_TYPE>
SET_TYPE *
tclListNetworkSet(Tcl_Obj *const source,
                  swig_type_info *swig_type,
                  Tcl_Interp *interp,
                  const Network *network)
{
  int argc;
  Tcl_Obj **argv;
  if (Tcl_ListObjGetElements(interp, source, &argc, &argv) == TCL_OK
      && argc > 0) {
    SET_TYPE *set = new SET_TYPE(network);
    for (int i = 0; i < argc; i++) {
      void *obj;
      // Ignore returned TCL_ERROR because can't get swig_type_info.
      SWIG_ConvertPtr(argv[i], &obj, swig_type, false);
      set->insert(reinterpret_cast<OBJECT_TYPE*>(obj));
    }
    return set;
  }
  else
    return nullptr;
}
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

%typemap(in) LibertyCellSeq* {
  $1 = tclListSeqLibertyCell($input, interp);
}

%typemap(out) TmpNetSeq* {
  Tcl_Obj *list = Tcl_NewListObj(0, nullptr);
  NetSeq *nets = $1;
  NetSeq::Iterator net_iter(nets);
  while (net_iter.hasNext()) {
    const Net *net = net_iter.next();
    Tcl_Obj *obj = SWIG_NewInstanceObj(const_cast<Net*>(net), SWIGTYPE_p_Net, false);
    Tcl_ListObjAppendElement(interp, list, obj);
  }
  delete nets;
  Tcl_SetObjResult(interp, list);
}

%typemap(out) TmpPinSet* {
  Tcl_Obj *list = Tcl_NewListObj(0, nullptr);
  PinSet *pins = $1;
  PinSet::Iterator pin_iter(pins);
  while (pin_iter.hasNext()) {
    const Pin *pin = pin_iter.next();
    Tcl_Obj *obj = SWIG_NewInstanceObj(const_cast<Pin*>(pin), SWIGTYPE_p_Pin, false);
    Tcl_ListObjAppendElement(interp, list, obj);
  }
  delete pins;
  Tcl_SetObjResult(interp, list);
}

%typemap(out) NetSeq* {
  Tcl_Obj *list = Tcl_NewListObj(0, nullptr);
  NetSeq *nets = $1;
  NetSeq::Iterator net_iter(nets);
  while (net_iter.hasNext()) {
    const Net *net = net_iter.next();
    Tcl_Obj *obj = SWIG_NewInstanceObj(const_cast<Net*>(net), SWIGTYPE_p_Net, false);
    Tcl_ListObjAppendElement(interp, list, obj);
  }
  Tcl_SetObjResult(interp, list);
}

%typemap(out) LibertyPort* {
  Tcl_Obj *obj = SWIG_NewInstanceObj($1, $1_descriptor, false);
  Tcl_SetObjResult(interp, obj);
}

%typemap(in) PinSet* {
  Resizer *resizer = getResizer();
  dbNetwork *network = resizer->getDbNetwork();
  $1 = tclListNetworkSet<PinSet, Pin>($input, SWIGTYPE_p_Pin, interp, network);
}

%typemap(out) PinSet {
  Tcl_Obj *list = Tcl_NewListObj(0, nullptr);
  PinSet::Iterator pin_iter($1);
  while (pin_iter.hasNext()) {
    const Pin *pin = pin_iter.next();
    Tcl_Obj *obj = SWIG_NewInstanceObj(const_cast<Pin*>(pin), SWIGTYPE_p_Pin, false);
    Tcl_ListObjAppendElement(interp, list, obj);
  }
  Tcl_SetObjResult(interp, list);
}

%typemap(in) ParasiticsSrc {
  int length;
  const char *arg = Tcl_GetStringFromObj($input, &length);
  if (stringEq(arg, "placement"))
    $1 = ParasiticsSrc::placement;
  else if (stringEq(arg, "global_routing"))
    $1 = ParasiticsSrc::global_routing;
  else if (stringEq(arg, "detailed_routing"))
    $1 = ParasiticsSrc::detailed_routing;
  else {
    Tcl_SetResult(interp,const_cast<char*>("Error: parasitics source."), TCL_STATIC);
    return TCL_ERROR;
  }
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
set_layer_rc_cmd(odb::dbTechLayer *layer,
                 const Corner *corner,
                 float res,
                 float cap)
{
  Resizer *resizer = getResizer();
  resizer->setLayerRC(layer, corner, res, cap);
}

double
layer_resistance(odb::dbTechLayer *layer,
                 const Corner *corner)
{
  Resizer *resizer = getResizer();
  double res, cap;
  resizer->layerRC(layer, corner, res, cap);
  return res;
}

double
layer_capacitance(odb::dbTechLayer *layer,
                  const Corner *corner)
{
  Resizer *resizer = getResizer();
  double res, cap;
  resizer->layerRC(layer, corner, res, cap);
  return cap;
}

void
set_h_wire_signal_rc_cmd(const Corner *corner,
                         float res,
                         float cap)
{
  ensureLinked();
  Resizer *resizer = getResizer();
  resizer->setHWireSignalRC(corner, res, cap);
}

void
set_v_wire_signal_rc_cmd(const Corner *corner,
                         float res,
                         float cap)
{
  ensureLinked();
  Resizer *resizer = getResizer();
  resizer->setVWireSignalRC(corner, res, cap);
}

void
set_h_wire_clk_rc_cmd(const Corner *corner,
                      float res,
                      float cap)
{
  ensureLinked();
  Resizer *resizer = getResizer();
  resizer->setHWireClkRC(corner, res, cap);
}

void
set_v_wire_clk_rc_cmd(const Corner *corner,
                      float res,
                      float cap)
{
  ensureLinked();
  Resizer *resizer = getResizer();
  resizer->setVWireClkRC(corner, res, cap);
}

// ohms/meter
double
wire_signal_resistance(const Corner *corner)
{
  ensureLinked();
  Resizer *resizer = getResizer();
  return resizer->wireSignalResistance(corner);
}

double
wire_clk_resistance(const Corner *corner)
{
  ensureLinked();
  Resizer *resizer = getResizer();
  return resizer->wireClkResistance(corner);
}

// farads/meter
double
wire_signal_capacitance(const Corner *corner)
{
  ensureLinked();
  Resizer *resizer = getResizer();
  return resizer->wireSignalCapacitance(corner);
}

double
wire_clk_capacitance(const Corner *corner)
{
  ensureLinked();
  Resizer *resizer = getResizer();
  return resizer->wireClkCapacitance(corner);
}

void 
estimate_parasitics_cmd(ParasiticsSrc src, const char* path)
{
  ensureLinked();
  Resizer* resizer = getResizer();
  std::map<Corner*, std::ostream*> spef_files;
  if (path != nullptr && std::strlen(path) > 0) {
    std::string file_path(path);
    if (!file_path.empty()) {
      for (Corner* corner : *resizer->getDbNetwork()->corners()) {
        file_path = path;
        if (resizer->getDbNetwork()->corners()->count() > 1) {
          std::string suffix("_");
          suffix.append(corner->name());
          if (file_path.find(".spef") != std::string::npos
              || file_path.find(".SPEF") != std::string::npos) {
            file_path.insert(file_path.size() - 5, suffix);
          } else {
            file_path.append(suffix);
          }
        }

        std::ofstream* file = new std::ofstream(file_path);

        if (file->is_open()) {
          spef_files[corner] = std::move(file);
        } else {
          Logger* logger = ord::getLogger();
          logger->error(utl::RSZ,
                        7,
                        "Can't open file " + file_path);
        }
      }
    }
  }

  resizer->estimateParasitics(src, spef_files);

  for (auto [_, file] : spef_files) {
    file->flush();
    delete file;
  }
  spef_files.clear();
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

InstanceSeq*
init_insts_cmd()
{
  InstanceSeq* insts = new InstanceSeq;
  return insts;
}

void
add_to_insts_cmd(Instance* inst, InstanceSeq* insts)
{
  insts->emplace_back(inst);
}

void
delete_insts_cmd(InstanceSeq* insts)
{
  delete insts;
}

void
remove_buffers_cmd(InstanceSeq insts)
{
  ensureLinked();
  Resizer *resizer = getResizer();
  resizer->removeBuffers(std::move(insts));
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
                  double buffer_gain,
                  bool match_cell_footprint,
                  bool verbose)
{
  ensureLinked();
  Resizer *resizer = getResizer();
  resizer->repairDesign(max_length,
                        slew_margin,
                        cap_margin,
                        buffer_gain,
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
             bool match_cell_footprint, bool verbose,
             bool skip_pin_swap, bool skip_gate_cloning,
             bool skip_buffering, bool skip_buffer_removal,
             bool skip_last_gasp)
{
  ensureLinked();
  Resizer *resizer = getResizer();
  return resizer->repairSetup(setup_margin, repair_tns_end_percent,
                       max_passes, match_cell_footprint, verbose,
                       skip_pin_swap, skip_gate_cloning,
                       skip_buffering, skip_buffer_removal,
                       skip_last_gasp);
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
            bool match_cell_footprint,
            bool verbose)
{
  ensureLinked();
  Resizer *resizer = getResizer();
  return resizer->repairHold(setup_margin, hold_margin,
                      allow_setup_violations,
                      max_buffer_percent, max_passes,
                      match_cell_footprint, verbose);
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
recover_power(float recover_power_percent, bool match_cell_footprint)
{
  ensureLinked();
  Resizer *resizer = getResizer();
  return resizer->recoverPower(recover_power_percent, match_cell_footprint);
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

void
highlight_steiner_tree(const Pin *drvr_pin)
{
  Resizer *resizer = getResizer();
  resizer->highlightSteiner(drvr_pin);
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
set_parasitics_src(ParasiticsSrc src)
{
  Resizer *resizer = getResizer();
  resizer->setParasiticsSrc(src);
}

void
eliminate_dead_logic_cmd(bool clean_nets)
{
  ensureLinked();
  Resizer *resizer = getResizer();
  resizer->eliminateDeadLogic(clean_nets);
}

} // namespace

%} // inline
