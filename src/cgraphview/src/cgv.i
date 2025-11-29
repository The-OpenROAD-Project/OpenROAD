// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

// Swig uses this to generate TCL interface methods that are be called
// from example.tcl.  Swig's handling of complex types can be difficult
// to master so it is easiest to use simple data types as arguments.

%{
#include "ord/OpenRoad.hh"
#include "cgv/cgv.h"
#include "graphics.h"
%}

%include "../../Exception.i"

%inline %{

namespace cgv {

void
display_csv_cmd(const char* file_name)
{
  cgv::CGV *c = ord::OpenRoad::openRoad()->getCGV();
  c->displayCSV(file_name);
}

void set_debug_cmd()
{
  cgv::CGV *c = ord::OpenRoad::openRoad()->getCGV();
  if (cgv::Graphics::guiActive()) {
    std::unique_ptr<Observer> graphics = std::make_unique<Graphics>();
    c->setDebug(graphics);
  }
}

} // namespace exa

%} // inline
