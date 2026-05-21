// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

%{
#include "src/odb/include/odb/db.h"
#include "include/ord/OpenRoad.hh"
#include "src/tap/include/tap/tapcell.h"

using std::set;
using std::string;
using std::vector;

%}

%include "../../Exception-py.i"
%include <std_string.i>
%include "tap/tapcell.h"
