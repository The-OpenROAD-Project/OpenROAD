// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// The Qt-free half of the rendering API: Renderer's bookkeeping and the
// legend drawing, which is expressed entirely through the abstract Painter.
// Split out of gui.cpp so a build with no Qt gets one definition of these
// instead of gui.cpp's and stub.cpp's competing copies.

#include <algorithm>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

#include "gui/core.h"
#include "odb/geom.h"

namespace gui {

Renderer::~Renderer()
{
  gui::Gui::get()->unregisterRenderer(this);
}

void Renderer::redraw()
{
  Gui::get()->redraw();
}

bool Renderer::checkDisplayControl(const std::string& name)
{
  return Gui::get()->checkDisplayControlsVisible(displayControlPath(name));
}

void Renderer::setDisplayControl(const std::string& name, bool value)
{
  Gui::get()->setDisplayControlsVisible(displayControlPath(name), value);
}

void Renderer::addDisplayControl(
    const std::string& name,
    bool initial_visible,
    const DisplayControlCallback& setup,
    const std::vector<std::string>& mutual_exclusivity)
{
  auto& control = controls_[name];

  control.visibility = initial_visible;
  control.interactive_setup = setup;
  control.mutual_exclusivity.insert(mutual_exclusivity.begin(),
                                    mutual_exclusivity.end());
}

Renderer::Settings Renderer::getSettings()
{
  Settings settings;
  for (const auto& [key, init_value] : controls_) {
    settings[key] = checkDisplayControl(key);
  }
  return settings;
}

void Renderer::setSettings(const Renderer::Settings& settings)
{
  for (auto& [key, control] : controls_) {
    setSetting<bool>(settings, key, control.visibility);
    setDisplayControl(key, control.visibility);
  }
}

//////////////////////////////////////////////////////////////////

void SpectrumGenerator::drawLegend(
    Painter& painter,
    const std::vector<std::pair<int, std::string>>& legend_key) const
{
  const int color_count = getColorCount();
  std::vector<Painter::Color> colors;
  colors.reserve(color_count);
  for (int i = 0; i < color_count; i += kLegendColorIncrement) {
    const double color_idx = (color_count - 1 - i) / scale_;
    colors.push_back(getColor(color_idx / color_count));
  }
  std::vector<std::pair<Painter::Color, std::string>> legend_key_colors;
  for (const auto& [legend_value, legend_text] : legend_key) {
    const int idx = std::clamp(legend_value / kLegendColorIncrement,
                               0,
                               static_cast<int>(colors.size()) - 1);
    legend_key_colors.push_back({colors[idx], legend_text});
  }
  LinearLegend legend(colors);
  legend.setLegendKey(legend_key_colors);
  legend.draw(painter);
}

/////////////////////////////////////////////////

LinearLegend::LinearLegend(const std::vector<Painter::Color>& colors)
    : colors_(colors)
{
}

void LinearLegend::setLegendKey(
    const std::vector<std::pair<Painter::Color, std::string>>& legend_key)
{
  legend_key_ = legend_key;
}

void LinearLegend::draw(Painter& painter) const
{
  const odb::Rect& bounds = painter.getBounds();
  const double pixel_per_dbu = painter.getPixelsPerDBU();
  const int legend_offset = 20 / pixel_per_dbu;  // 20 pixels
  const double box_height = 1 / pixel_per_dbu;   // 1 pixels
  const int legend_width = 20 / pixel_per_dbu;   // 20 pixels
  const int text_offset = 2 / pixel_per_dbu;
  const int legend_top = bounds.yMax() - legend_offset;
  const int legend_right = bounds.xMax() - legend_offset;
  const int legend_left = legend_right - legend_width;
  const Painter::Anchor key_anchor = Painter::Anchor::kRightCenter;

  odb::Rect legend_bounds(
      legend_left, legend_top, legend_right + text_offset, legend_top);

  const int color_count = colors_.size();

  std::vector<std::pair<odb::Point, std::string>> legend_key_points;
  for (const auto& [legend_color, legend_text] : legend_key_) {
    const auto find_color = std::ranges::find(colors_, legend_color);
    if (find_color == colors_.end()) {
      continue;
    }
    const int legend_value
        = std::distance(colors_.begin(), find_color);  // index in colors_

    const int text_right = legend_left - text_offset;
    const int box_top
        = legend_top - ((color_count - legend_value) * box_height);

    legend_key_points.push_back({{text_right, box_top}, legend_text});
    const odb::Rect text_bounds = painter.stringBoundaries(
        text_right, box_top, key_anchor, legend_text);

    legend_bounds.merge(text_bounds);
  }

  // draw background
  painter.setPen(Painter::kDarkGray, true);
  painter.setBrush(Painter::kDarkGray);
  painter.drawRect(legend_bounds, 10, 10);

  // draw color map
  double box_top = legend_top;
  for (int i = 0; i < color_count; i++) {
    painter.setPen(colors_[i], true);
    painter.drawLine(odb::Point(legend_left, box_top),
                     odb::Point(legend_right, box_top));
    box_top -= box_height;
  }

  // draw key values
  painter.setPen(Painter::kBlack, true);
  painter.setBrush(Painter::kTransparent);
  for (const auto& [pt, text] : legend_key_points) {
    painter.drawString(pt.x(), pt.y(), key_anchor, text);
  }
  painter.drawRect(odb::Rect(legend_left, box_top, legend_right, legend_top));
}

/////////////////////////////////////////////////

void DiscreteLegend::addLegendKey(const Painter::Color& color,
                                  const std::string& text)
{
  color_key_.emplace_back(color, text);
}

void DiscreteLegend::draw(Painter& painter) const
{
  if (color_key_.empty()) {
    // The keys are the whole legend; without them the bounds below stay
    // zero-height and the background draws as a sliver in the corner.
    return;
  }

  const odb::Rect& bounds = painter.getBounds();
  const double pixel_per_dbu = painter.getPixelsPerDBU();
  const int legend_offset = 20 / pixel_per_dbu;  // 20 pixels
  const int legend_width = 20 / pixel_per_dbu;   // 20 pixels
  const int text_offset = 2 / pixel_per_dbu;
  const int color_offset = 2 * text_offset;
  const int legend_top = bounds.yMax() - legend_offset;
  const int legend_right = bounds.xMax() - legend_offset;
  const int legend_left = legend_right - legend_width;

  odb::Rect legend_bounds(
      legend_left, legend_top, legend_right + text_offset, legend_top);

  std::vector<std::pair<odb::Rect, std::string>> legend_key_rects;
  std::vector<std::pair<odb::Rect, Painter::Color>> legend_color_rects;
  int last_text_top = legend_top;
  for (const auto& [legend_color, legend_text] : color_key_) {
    const int text_right = legend_left - text_offset;

    const odb::Rect key_rect = painter.stringBoundaries(
        text_right, last_text_top, Painter::Anchor::kTopRight, legend_text);

    last_text_top = key_rect.yMin();

    legend_key_rects.push_back({key_rect, legend_text});
    legend_color_rects.push_back({{legend_left + color_offset,
                                   key_rect.yMin() + color_offset,
                                   legend_right - color_offset,
                                   key_rect.yMax() - color_offset},
                                  legend_color});
    legend_bounds.merge(key_rect);
  }

  // draw background
  painter.setPen(Painter::kDarkGray, true);
  painter.setBrush(Painter::kDarkGray);
  painter.drawRect(legend_bounds, 10, 10);

  // draw color map
  painter.setPen(Painter::kBlack, true);
  for (const auto& [rect, color] : legend_color_rects) {
    painter.setBrush(color);
    painter.drawRect(rect);
  }

  // draw key values
  painter.setBrush(Painter::kTransparent);
  for (const auto& [rect, text] : legend_key_rects) {
    painter.drawString(
        rect.xMax(), rect.yCenter(), Painter::Anchor::kRightCenter, text);
  }
}

}  // namespace gui
