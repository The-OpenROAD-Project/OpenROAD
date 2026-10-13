// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2026, The OpenROAD Authors

#pragma once

// The part of the gui API that only a Qt build provides.  Everything the
// rest of OpenROAD draws through -- web::Painter, web::Renderer,
// web::Descriptor/web::Selected, web::HeatMapDataSource, web::Gui itself -- is
// Qt-free and lives in gui/core.h. Include this header only if you need to
// start the Qt gui.

#include <string>

#include "web/core.h"

struct Tcl_Interp;

namespace gui {

// The main entry point
int startGui(int& argc,
             char* argv[],
             Tcl_Interp* interp,
             const std::string& script = "",
             bool interactive = true,
             bool load_settings = true,
             bool minimize = false);

}  // namespace gui
