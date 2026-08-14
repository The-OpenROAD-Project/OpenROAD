// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Swig uses this to generate Python interface methods that are called from
// "openroad -python".  This is intended for using the Watermark methods
// directly rather than through commands as in TCL, which matters because the
// placement and CTS embedders run under the Python interpreter and can then
// check their own work in the same process.

%module wmk_py

%{
#include "ord/OpenRoad.hh"
#include "wmk/Watermark.h"
%}

%include <std_string.i>
%include "../../Exception-py.i"

%include "wmk/Watermark.h"
