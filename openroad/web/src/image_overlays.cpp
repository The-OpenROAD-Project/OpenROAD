// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "image_overlays.h"

#include <algorithm>
#include <utility>

#include "script_view.h"
#include "tile_generator.h"
#include "web/core.h"

namespace web {

ImageOverlays buildImageOverlays(ScriptView::Frame frame)
{
  ImageOverlays overlays;

  // RenderThread::drawSelected.
  if (!frame.selection.empty()) {
    overlays.passes.emplace_back(
        [selection = std::move(frame.selection)](Painter& painter) {
          for (const Selected& selected : selection) {
            selected.highlight(painter, Painter::kHighlight);
          }
        });
  }

  // RenderThread::drawHighlighted: each group in its own color, filled.
  const bool any_highlight = std::ranges::any_of(
      frame.highlights,
      [](const SelectionSet& group) { return !group.empty(); });
  if (any_highlight) {
    overlays.passes.emplace_back(
        [highlights = std::move(frame.highlights)](Painter& painter) {
          for (int group = 0; group < kNumHighlightSet; ++group) {
            const Painter::Color& color = Painter::kHighlightColors[group];
            for (const Selected& highlighted : highlights[group]) {
              highlighted.highlight(painter, color, 1, color);
            }
          }
        });
  }

  return overlays;
}

}  // namespace web
