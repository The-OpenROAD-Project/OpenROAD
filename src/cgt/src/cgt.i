// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025, The OpenROAD Authors

%{
#include "cgt/ClockGating.h"

namespace ord {
// Defined in OpenRoad.i
OpenRoad *
getOpenRoad();
cgt::ClockGating *
getClockGating();
}


using namespace cgt;
using ord::getOpenRoad;
using ord::getClockGating;
%}

%include "../../Exception.i"

%inline %{

void
set_min_instances_cmd(int min_instances)
{
  cgt::ClockGating* cgt = getClockGating();
  cgt->setMinInstances(min_instances);
}

void
set_max_cover_cmd(int max_cover)
{
  cgt::ClockGating* cgt = getClockGating();
  cgt->setMaxCover(max_cover);
}

void
clock_gating_cmd(const char* dump_dir)
{
  cgt::ClockGating* cgt = getClockGating();
  cgt->setDumpDir(dump_dir);
  cgt->run();
}

%}
