// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include "src/odb/include/odb/db.h"
#include "src/utl/include/utl/Logger.h"
#include "tcl.h"

namespace rsz {

class Resizer;

void initResizer(Tcl_Interp* tcl_interp);

}  // namespace rsz
