// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#pragma once

#include "tcl.h"

namespace drt {

class TritonRoute;

void initGui(drt::TritonRoute* router);
void initTcl(Tcl_Interp* tcl_interp);

}  // namespace drt
