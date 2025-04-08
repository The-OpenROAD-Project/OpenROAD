// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

%{
#include "odb/db.h"
#include "ord/OpenRoad.hh"
#include "tap/tapcell.h"

using std::set;
using std::string;
using std::vector;

%}

%include "../../Exception-py.i"
%include <std_string.i>
%include "tap/tapcell.h"
