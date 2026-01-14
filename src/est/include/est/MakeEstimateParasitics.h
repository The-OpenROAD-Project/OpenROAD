// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include "odb/db.h"
#include "tcl.h"
#include "utl/Logger.h"

namespace est {

class EstimateParasitics;

void initGui(est::EstimateParasitics* estimate_parasitics);
void initTcl(Tcl_Interp* tcl_interp);

}  // namespace est
