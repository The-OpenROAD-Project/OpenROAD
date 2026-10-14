// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#ifdef BAZEL
%module(package="src.gpl") gpl_py
#else
%module gpl
#endif

%{
#include "gpl/Replace.h"
#include "grt/GlobalRouteOptions.h"
#include "odb/db.h"
#include "rsz/RepairDesignOptions.hh"

namespace ord {
gpl::Replace*
getReplace();

}

using ord::getReplace;
using gpl::Replace;

%}

%include "typemaps.i"
%include "std_vector.i"
%include "../../Exception-py.i"
%include "../../options-py.i"

// The embedded sub-tool options are classes of the rsz and grt modules.
%import "rsz/RepairDesignOptions.hh"
%import "grt/GlobalRouteOptions.h"

%include "gpl/Replace.h"
%options_struct(gpl::InitialPlaceOptions, InitialPlaceOptions)
%options_struct(gpl::NesterovOptions, NesterovOptions)
%options_struct(gpl::TimingOptions, TimingOptions)
%options_struct(gpl::VirtualCtsOptions, VirtualCtsOptions)
%options_struct(gpl::RoutabilityOptions, RoutabilityOptions)
%options_struct(gpl::PlaceOptions, PlaceOptions)

namespace std {
#ifndef SWIG_VECTOR_INT
#define SWIG_VECTOR_INT
    %template(IntVector) vector<int>;
#endif
}
