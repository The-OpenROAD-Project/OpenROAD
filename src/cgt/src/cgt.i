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
clock_gating_cmd(Tcl_Obj* instances,
                 Tcl_Obj* gate_cond_nets,
                 int min_instances,
                 int max_cover,
                 const char* group_instances,
                 const char* dump_dir)
{
  cgt::ClockGating* cgt = getClockGating();

  int list_len;
  Tcl_Obj **list_elems;
  Tcl_Interp* interp = getOpenRoad()->tclInterp();
  if (Tcl_ListObjGetElements(interp, instances, &list_len, &list_elems) != TCL_OK) {
      return;
  }
  for (int i = 0; i < list_len; i++) {
      const char *elem = Tcl_GetString(list_elems[i]);
      cgt->addInstance(elem);
  }
  if (Tcl_ListObjGetElements(interp, gate_cond_nets, &list_len, &list_elems) != TCL_OK) {
      return;
  }
  for (int i = 0; i < list_len; i++) {
      const char *elem = Tcl_GetString(list_elems[i]);
      cgt->addGateCondNet(elem);
  }
  cgt->setMinInstances(min_instances);
  cgt->setMaxCover(max_cover);
  if (strcmp(group_instances, "cell_prefix") == 0) {
    cgt->setGroupInstances(GroupInstances::CellPrefix);
  } else if (strcmp(group_instances, "net_prefix") == 0) {
    cgt->setGroupInstances(GroupInstances::NetPrefix);
  }
  cgt->setDumpDir(dump_dir);
  cgt->run();
}

%}
