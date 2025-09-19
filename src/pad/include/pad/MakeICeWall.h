// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#pragma once

#include <tcl.h>

#include "pad/ICeWall.h"

namespace odb {
class dbDatabase;
}

namespace utl {
class Logger;
}

namespace pad {

void initICeWall(pad::ICeWall* icewall,
                 odb::dbDatabase* db,
                 utl::Logger* logger,
                 Tcl_Interp* tcl_interp);

pad::ICeWall* makeICeWall();

void deleteICeWall(pad::ICeWall* icewall);

}  // namespace pad
