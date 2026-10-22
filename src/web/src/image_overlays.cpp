// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "image_overlays.h"

#include <algorithm>
#include <utility>

#include "dbDescriptors.h"
#include "odb/db.h"
#include "script_view.h"
#include "tile_generator.h"
#include "timing_path_painter.h"
#include "web/clockTree.h"
#include "web/core.h"
#include "web/heatMap.h"
#include "web/timing_path_style.h"

namespace web {

ImageOverlays buildImageOverlays(ScriptView::Frame frame,
                                 const Painter::Color& background)
{
  ImageOverlays overlays;

  // HeatMapRenderer::drawObjects, the bins first, under everything else.
  if (frame.heat_map != nullptr) {
    overlays.passes.emplace_back([heat_map = frame.heat_map](Painter& painter) {
      heat_map->ensureMap();
      paintHeatMap(*heat_map, painter);
    });
    overlays.legends.emplace_back(
        [heat_map = frame.heat_map](Painter& painter) {
          paintHeatMapLegend(*heat_map, painter);
        });
  }

  // DRCRenderer::drawObjects, which Qt draws among the renderers, before the
  // selection.
  if (frame.markers != nullptr) {
    overlays.passes.emplace_back([markers = frame.markers](Painter& painter) {
      static_cast<const DbMarkerDescriptor*>(
          Gui::get()->getDescriptor<odb::dbMarker*>())
          ->paintMarkers(markers, painter);
    });
  }

  // TimingPathRenderer::drawObjects.
  if (frame.timing_path) {
    overlays.passes.emplace_back(
        [path = std::move(*frame.timing_path), parts = frame.timing_path_parts](
            Painter& painter) { paintTimingPath(path, parts, painter); });
    if (frame.timing_path_parts.legend) {
      overlays.legends.emplace_back(
          [](Painter& painter) { paintTimingPathLegend(painter); });
    }
  }

  // ClockTreeRenderer::drawObjects, registered once the viewer is shown.
  if (frame.clock_tree != nullptr) {
    overlays.passes.emplace_back(
        [tree = std::move(frame.clock_tree),
         depth = frame.clock_tree_depth,
         trunk = clockTreeTrunkColor(background)](Painter& painter) {
          paintClockTree(*tree, depth, trunk, painter);
        });
  }

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
