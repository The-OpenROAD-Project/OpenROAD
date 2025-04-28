// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

%module psm_py

%{
#include "ord/OpenRoad.hh"
#include "psm/pdnsim.h"
#include "odb/db.h"

%}

%include <std_string.i>
%include <std_map.i>

%import "odb.i"
%include "../../Exception-py.i"

%template(IRDropByPoint) std::map<odb::Point, double>;

// For getIRDropForLayer
WRAP_OBJECT_RETURN_REF(psm::PDNSim::IRDropByPoint, ir_drop);

%include "psm/pdnsim.h"
