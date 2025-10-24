// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

// This is application startup related infrastructure.  The OpenROAD
// application will use these methods to create and initialize an
// Example instance.
//
// If Example needs to depend on other tools in OpenROAD those would be
// added as extra arguments to initExample and passed from OpenRoad::init.
// In that case it is important that initExample is called after those
// tools have been initialized.

#pragma once

#include "tcl.h"

namespace exa {

class Example;

void initExample(Tcl_Interp* tcl_interp);

}  // namespace exa
