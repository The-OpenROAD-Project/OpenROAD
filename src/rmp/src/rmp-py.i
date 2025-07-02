// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

%{
#include "rmp/Restructure.h"
#include "rmp/blif.h"
#include "sta/Corner.hh"
#include "ord/OpenRoad.hh"
#include "odb/db.h"
#include "sta/Liberty.hh"
#include <string>

namespace ord {
// Defined in OpenRoad.i
rmp::Restructure *
getRestructure();

OpenRoad *
getOpenRoad();
}

using namespace rmp;
using ord::getRestructure;
using ord::getOpenRoad;
using odb::dbInst;
using sta::LibertyPort;
using sta::Corner;
%}

%include "../../Exception-py.i"

%include <typemaps.i>
%include <std_string.i>
%include "rmp/blif.h"
%include "rmp/Restructure.h"
