// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

%{
#include <string>
#include <vector>

#include "fsv/FieldSolver.h"
#include "fsv/Geometry.h"
#include "fsv/Solver.h"
#include "ord/OpenRoad.hh"

namespace {

fsv::FieldSolver* getFieldSolver()
{
  return ord::OpenRoad::openRoad()->getFieldSolver();
}

}  // namespace
%}

%include "../../Exception.i"

// A Tcl list of net names.
%typemap(out) std::vector<std::string> {
  Tcl_Obj* list = Tcl_NewListObj(0, nullptr);
  for (const std::string& name : $1) {
    Tcl_ListObjAppendElement(
        interp, list, Tcl_NewStringObj(name.c_str(), name.size()));
  }
  Tcl_SetObjResult(interp, list);
}

// One master's row as a Tcl dict. Couplings are positive capacitances in fF,
// keyed by net name.
%typemap(out) fsv::NetCapacitance {
  const fsv::CapRow& row = $1.row;
  Tcl_Obj* dict = Tcl_NewDictObj();
  auto put = [&](Tcl_Obj* d, const std::string& key, Tcl_Obj* value) {
    Tcl_DictObjPut(
        interp, d, Tcl_NewStringObj(key.c_str(), key.size()), value);
  };
  Tcl_Obj* couplings = Tcl_NewDictObj();
  Tcl_Obj* sigmas = Tcl_NewDictObj();
  for (int net = 0; net < static_cast<int>(row.cap.size()); net++) {
    if (net != row.master) {
      put(couplings, $1.nets[net], Tcl_NewDoubleObj(row.coupling(net)));
      put(sigmas, $1.nets[net], Tcl_NewDoubleObj(row.sigma[net]));
    }
  }
  put(dict, "total", Tcl_NewDoubleObj(row.total()));
  put(dict, "total_sigma", Tcl_NewDoubleObj(row.sigma[row.master]));
  put(dict, "ground", Tcl_NewDoubleObj(-row.ground));
  put(dict, "ground_sigma", Tcl_NewDoubleObj(row.ground_sigma));
  put(dict, "couplings", couplings);
  put(dict, "coupling_sigmas", sigmas);
  put(dict, "walks", Tcl_NewWideIntObj(row.walks));
  put(dict, "hops", Tcl_NewWideIntObj(row.hops));
  put(dict, "seconds", Tcl_NewDoubleObj($1.seconds));
  Tcl_SetObjResult(interp, dict);
}

%inline %{

namespace fsv {

void read_itf_cmd(const char* path)
{
  getFieldSolver()->readItf(path);
}

void set_permittivity_cmd(double eps)
{
  getFieldSolver()->setPermittivity(eps);
}

void add_box_cmd(const char* net,
                 double x0,
                 double y0,
                 double z0,
                 double x1,
                 double y1,
                 double z1)
{
  getFieldSolver()->addBox(net, {{x0, y0, z0}, {x1, y1, z1}});
}

void add_layer_rect_cmd(const char* net,
                        const char* layer,
                        double x0,
                        double y0,
                        double x1,
                        double y1)
{
  getFieldSolver()->addLayerRect(net, layer, {x0, y0, x1, y1});
}

void clear_cmd()
{
  getFieldSolver()->clearGeometry();
}

std::vector<std::string> net_names_cmd()
{
  return getFieldSolver()->netNames();
}

fsv::NetCapacitance solve_net_cmd(const char* net,
                                  double tol_total,
                                  double tol_coupling,
                                  double coupling_threshold,
                                  long long min_walks,
                                  long long max_walks,
                                  long long seed,
                                  int panels)
{
  // A negative value keeps the default.
  fsv::SolverOptions solver_options;
  fsv::SolveOptions options;
  if (tol_total >= 0) {
    options.tol_total = tol_total;
  }
  if (tol_coupling >= 0) {
    options.tol_coupling = tol_coupling;
  }
  if (coupling_threshold >= 0) {
    options.coupling_threshold = coupling_threshold;
  }
  if (min_walks >= 0) {
    options.min_walks = min_walks;
  }
  if (max_walks >= 0) {
    options.max_walks = max_walks;
  }
  if (seed >= 0) {
    solver_options.seed = seed;
  }
  if (panels >= 0) {
    solver_options.panels = panels;
  }
  solver_options.threads = ord::OpenRoad::openRoad()->getThreadCount();
  return getFieldSolver()->solve(net, solver_options, options);
}

}  // namespace fsv

%}  // inline
