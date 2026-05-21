// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

%module psm_py

%{
#include "include/ord/OpenRoad.hh"
#include "src/psm/include/psm/pdnsim.h"
#include "src/odb/include/odb/db.h"

%}

%include <std_string.i>
%include <std_map.i>

%import "odb.i"
%include "../../Exception-py.i"

%template(IRDropByPoint) std::map<odb::Point, double>;

// For getIRDropForLayer
WRAP_OBJECT_RETURN_REF(psm::PDNSim::IRDropByPoint, ir_drop);

%include "psm/pdnsim.h"
