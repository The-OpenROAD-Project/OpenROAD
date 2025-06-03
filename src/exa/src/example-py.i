// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

// Swig uses this to generate Python interface methods that are be called
// from "openroad -python".  This is intended for using the Example's methods
// directly rather than through commands as in TCL.

%module exa_py

%{
#include "exa/example.h"
#include "ord/OpenRoad.hh"
%}

%include "../../Exception-py.i"

%include "exa/example.h"
