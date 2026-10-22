// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include "script_view.h"
#include "tile_generator.h"
#include "web/core.h"

namespace web {

// What a saved image draws over the layout for `frame`, in Qt's order
// (RenderThread::draw), over an image filled with `background`.
ImageOverlays buildImageOverlays(ScriptView::Frame frame,
                                 const Painter::Color& background);

}  // namespace web
