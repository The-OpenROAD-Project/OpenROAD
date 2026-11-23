// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

%{
#include <string>
#include <vector>

#include "ord/OpenRoad.hh"
#include "pex/Pex.h"

namespace {

pex::Pex* getPex()
{
  return ord::OpenRoad::openRoad()->getPex();
}

}  // namespace
%}

%include "../../Exception.i"

// A Tcl list of strings in, as a vector held for the call.
%typemap(in) const std::vector<std::string>& (std::vector<std::string> list) {
  Tcl_Size argc;
  Tcl_Obj** argv;
  if (Tcl_ListObjGetElements(interp, $input, &argc, &argv) != TCL_OK) {
    return TCL_ERROR;
  }
  for (Tcl_Size i = 0; i < argc; i++) {
    list.emplace_back(Tcl_GetString(argv[i]));
  }
  $1 = &list;
}

%inline %{

namespace pex {

void build_model_cmd(const char* itf,
                     const char* out,
                     const std::vector<std::string>& layers,
                     double tol)
{
  getPex()->buildModel(
      itf, out, layers, tol, ord::OpenRoad::openRoad()->getThreadCount());
}

void read_model_cmd(const char* itf, const char* model)
{
  getPex()->readModel(itf, model);
}

void extract_cmd(double max_res,
                 double coupling_threshold,
                 double coupling_ratio,
                 bool coupling_to_ground)
{
  pex::ExtractOptions options;
  options.max_res = max_res;
  options.coupling_threshold = coupling_threshold;
  options.coupling_ratio = coupling_ratio;
  options.coupling_to_ground = coupling_to_ground;
  options.threads = ord::OpenRoad::openRoad()->getThreadCount();
  getPex()->extract(options);
}

void write_spef_cmd(const char* path, const std::vector<std::string>& nets)
{
  getPex()->writeSpef(path, nets);
}

void compare_cmd(const std::vector<std::string>& nets,
                 int sample,
                 double halo,
                 double tol)
{
  getPex()->compare(
      nets, sample, halo, tol, ord::OpenRoad::openRoad()->getThreadCount());
}

}  // namespace pex

%}  // inline
