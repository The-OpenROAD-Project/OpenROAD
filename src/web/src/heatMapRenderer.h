// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <memory>

#include "web/core.h"
#include "web/heatMap.h"

namespace web {

// The Renderer that paints a heat map.  It draws entirely through Painter,
// so it lives with the Qt-free half; only the setup dialog the renderer's
// display control opens is Qt, and that goes through Dialogs.
std::unique_ptr<Renderer> makeHeatMapRenderer(HeatMapDataSource& datasource);

}  // namespace web
