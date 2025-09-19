// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

%module ant_py

%{

#include "odb/geom.h"

#include "ant/AntennaChecker.hh"
#include "ord/OpenRoad.hh"

namespace ord {
// Defined in OpenRoad.i
odb::dbDatabase *getDb();
}

using namespace odb;

%}


%include <std_vector.i>
%template(ViolationVector) std::vector<ant::Violation>;

%import "odb.i"
%include "../../Exception-py.i"

%include "ant/AntennaChecker.hh"
