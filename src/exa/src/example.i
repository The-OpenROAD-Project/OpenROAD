// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

// Swig uses this to generate TCL interface methods that are be called
// from example.tcl.  Swig's handling of complex types can be difficult
// to master so it is easiest to use simple data types as arguments.

%{
#include "ord/OpenRoad.hh"
#include "exa/example.h"
#include "graphics.h"
%}

%include "../../Exception.i"

%inline %{

namespace exa {

void
make_instance_cmd(const char* name)
{
  exa::Example *example = ord::OpenRoad::openRoad()->getExample();
  example->makeInstance(name);
}

void set_debug_cmd()
{
  exa::Example *example = ord::OpenRoad::openRoad()->getExample();
  if (exa::Graphics::guiActive()) {
    std::unique_ptr<Observer> graphics = std::make_unique<Graphics>();
    example->setDebug(graphics);
  }
}

void db_log_test_cmd(int num_threads,
                       int num_entries,
                       int num_chunks)
{
  exa::Example* example = ord::OpenRoad::openRoad()->getExample();
  example->dbLogTest(num_threads, num_entries, num_chunks);
}

} // namespace exa

%} // inline
