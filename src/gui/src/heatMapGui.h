// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <memory>

#include "gui/gui.h"
#include "gui/heatMap.h"

namespace gui {

std::unique_ptr<Renderer> makeHeatMapRenderer(HeatMapDataSource& datasource);
void showHeatMapSetupDialog(HeatMapDataSource* source);

}  // namespace gui
