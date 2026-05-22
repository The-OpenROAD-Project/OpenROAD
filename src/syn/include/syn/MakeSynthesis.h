// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include "tcl.h"

namespace syn {

class Synthesis;

void initSynthesis(Tcl_Interp* tcl_interp);

}  // namespace syn
