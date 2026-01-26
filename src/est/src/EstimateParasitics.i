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
#include "est/EstimateParasitics.h"
#include "sta/Delay.hh"
#include "db_sta/dbNetwork.hh"
#include "ord/OpenRoad.hh"

namespace ord {
// Defined in OpenRoad.i
est::EstimateParasitics* getEstimateParasitics();
utl::Logger* getLogger();
void ensureLinked();
}

namespace sta {

// The aliases are created to attach different conversion rules:
// TmpNetSeq, TmpPinSet pointers are freed when crossing into Tcl,
// NetSeq, PinSet pointers are not
using TmpNetSeq = NetSeq;
using TmpPinSet = PinSet;

} // namespace

using ord::getEstimateParasitics;
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

using est::EstimateParasitics;
using est::ParasiticsSrc;
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
    utl::Logger* logger = ord::getLogger();
    try {
      logger->error(utl::EST, 19, "Unknown parasitics source '{}'.", arg);
    } catch (const std::exception &e) {
      Tcl_SetResult(interp, const_cast<char*>(e.what()), TCL_STATIC);
      return TCL_ERROR;
    }
  }
}

////////////////////////////////////////////////////////////////
//
// C++ functions visible as TCL functions.
//
////////////////////////////////////////////////////////////////

%include "../../Exception.i"
%include "std_string.i"

%inline %{

namespace est {

void
set_layer_rc_cmd(odb::dbTechLayer *layer,
                 const Corner *corner,
                 float res,
                 float cap)
{
  est::EstimateParasitics *estimate_parasitics = getEstimateParasitics();
  estimate_parasitics->setLayerRC(layer, corner, res, cap);
}

void
add_clk_layer_cmd(odb::dbTechLayer *layer)
{
  est::EstimateParasitics *estimate_parasitics = getEstimateParasitics();
  estimate_parasitics->addClkLayer(layer);
}

void
add_signal_layer_cmd(odb::dbTechLayer *layer)
{
  est::EstimateParasitics *estimate_parasitics = getEstimateParasitics();
  estimate_parasitics->addSignalLayer(layer);
}

double
layer_resistance(odb::dbTechLayer *layer,
                 const Corner *corner)
{
  est::EstimateParasitics *estimate_parasitics = getEstimateParasitics();
  double res, cap;
  estimate_parasitics->layerRC(layer, corner, res, cap);
  return res;
}

double
layer_capacitance(odb::dbTechLayer *layer,
                  const Corner *corner)
{
  est::EstimateParasitics *estimate_parasitics = getEstimateParasitics();
  double res, cap;
  estimate_parasitics->layerRC(layer, corner, res, cap);
  return cap;
}

void
set_h_wire_signal_rc_cmd(const Corner *corner,
                         float res,
                         float cap)
{
  ensureLinked();
  est::EstimateParasitics *estimate_parasitics = getEstimateParasitics();
  estimate_parasitics->setHWireSignalRC(corner, res, cap);
}

void
set_v_wire_signal_rc_cmd(const Corner *corner,
                         float res,
                         float cap)
{
  ensureLinked();
  est::EstimateParasitics *estimate_parasitics = getEstimateParasitics();
  estimate_parasitics->setVWireSignalRC(corner, res, cap);
}

void
set_h_wire_clk_rc_cmd(const Corner *corner,
                      float res,
                      float cap)
{
  ensureLinked();
  est::EstimateParasitics *estimate_parasitics = getEstimateParasitics();
  estimate_parasitics->setHWireClkRC(corner, res, cap);
}

void
set_v_wire_clk_rc_cmd(const Corner *corner,
                      float res,
                      float cap)
{
  ensureLinked();
  est::EstimateParasitics *estimate_parasitics = getEstimateParasitics();
  estimate_parasitics->setVWireClkRC(corner, res, cap);
}

// ohms/meter
double
wire_signal_resistance(const Corner *corner)
{
  ensureLinked();
  est::EstimateParasitics *estimate_parasitics = getEstimateParasitics();
  return estimate_parasitics->wireSignalResistance(corner);
}

double
wire_clk_resistance(const Corner *corner)
{
  ensureLinked();
  est::EstimateParasitics *estimate_parasitics = getEstimateParasitics();
  return estimate_parasitics->wireClkResistance(corner);
}

// farads/meter
double
wire_signal_capacitance(const Corner *corner)
{
  ensureLinked();
  est::EstimateParasitics *estimate_parasitics = getEstimateParasitics();
  return estimate_parasitics->wireSignalCapacitance(corner);
}

double
wire_clk_capacitance(const Corner *corner)
{
  ensureLinked();
  est::EstimateParasitics *estimate_parasitics = getEstimateParasitics();
  return estimate_parasitics->wireClkCapacitance(corner);
}


void
estimate_parasitics_cmd(ParasiticsSrc src, const char* path)
{
  ensureLinked();
  est::EstimateParasitics *estimate_parasitics = getEstimateParasitics();
  std::map<Corner*, std::ostream*> spef_files;
  if (path != nullptr && std::strlen(path) > 0) {
    std::string file_path(path);
    if (!file_path.empty()) {
      for (Corner* corner : *estimate_parasitics->getDbNetwork()->corners()) {
        file_path = path;
        if (estimate_parasitics->getDbNetwork()->corners()->count() > 1) {
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
          utl::Logger* logger = ord::getLogger();
          logger->error(utl::EST,
                        7,
                        "Can't open file " + file_path);
        }
      }
    }
  }

  estimate_parasitics->estimateParasitics(src, spef_files);

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
  est::EstimateParasitics *estimate_parasitics = getEstimateParasitics();
  estimate_parasitics->estimateWireParasitic(net);
}

bool
have_estimated_parasitics()
{
  est::EstimateParasitics *estimate_parasitics = getEstimateParasitics();
  return estimate_parasitics->haveEstimatedParasitics();
}

void
set_parasitics_src(ParasiticsSrc src)
{
  est::EstimateParasitics *estimate_parasitics = getEstimateParasitics();
  estimate_parasitics->setParasiticsSrc(src);
}

void
highlight_steiner_tree(const Pin *drvr_pin)
{
  est::EstimateParasitics *estimate_parasitics = getEstimateParasitics();
  estimate_parasitics->highlightSteiner(drvr_pin);
}

} // namespace

%} // inline
