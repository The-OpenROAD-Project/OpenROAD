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
%template(NetCheckVector) std::vector<ant::NetCheck>;
%template(PinCheckVector) std::vector<ant::PinCheck>;
%template(LayerCheckVector) std::vector<ant::LayerCheck>;

// An unset ratio (std::optional) reads as None.
%typemap(out) std::optional<ant::RatioCheck>* {
  if ($1 && $1->has_value()) {
    $result = SWIG_NewPointerObj(new ant::RatioCheck(**$1),
                                 $descriptor(ant::RatioCheck*),
                                 SWIG_POINTER_OWN);
  } else {
    Py_INCREF(Py_None);
    $result = Py_None;
  }
}
// Read the result lists by value, as tuples: a proxy for the member vector
// would dangle once its parent is freed, and iterating one aborts on
// SWIG's stop_iteration, which the %exception handler does not catch.
%naturalvar ant::DesignCheck::nets;
%naturalvar ant::NetCheck::pins;
%naturalvar ant::PinCheck::layers;
%immutable ant::DesignCheck::nets;
%immutable ant::NetCheck::pins;
%immutable ant::PinCheck::layers;
%immutable ant::LayerCheck::psr;
%immutable ant::LayerCheck::csr;

%import "odb.i"
%include "../../Exception-py.i"

%include "ant/AntennaChecker.hh"
