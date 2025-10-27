// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include "odb/db.h"
#include "tcl.h"
#include "utl/Logger.h"

namespace rsz {

class Resizer;

void initResizer(Tcl_Interp* tcl_interp);

}  // namespace rsz
