// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

// Transitional.  This code is web:: now and lives behind web/core.h; this lets
// a client that still spells it gui:: and includes gui/core.h go on compiling
// until it is moved over.  Deleted, with this directory, once they all are.

#include "web/core.h"

namespace gui {
using namespace web;  // NOLINT(build/namespaces)
}
