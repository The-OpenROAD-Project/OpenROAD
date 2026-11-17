// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// The Qt-free half of the rendering API: Painter's name tables, Renderer's
// bookkeeping, and the legend drawing, which is expressed entirely through
// the abstract Painter.
// Split out of gui.cpp so that every build, with Qt or without, gets the one
// definition of these.

#include <algorithm>
#include <iterator>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "odb/geom.h"
#include "utl/Logger.h"
#include "web/core.h"

namespace web {

Renderer::~Renderer()
{
  web::Gui::get()->unregisterRenderer(this);
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

std::map<std::string, Painter::Color> Painter::colors()
{
  return {{"black", Painter::kBlack},
          {"white", Painter::kWhite},
          {"dark_gray", Painter::kDarkGray},
          {"gray", Painter::kGray},
          {"light_gray", Painter::kLightGray},
          {"red", Painter::kRed},
          {"green", Painter::kGreen},
          {"blue", Painter::kBlue},
          {"cyan", Painter::kCyan},
          {"magenta", Painter::kMagenta},
          {"yellow", Painter::kYellow},
          {"dark_red", Painter::kDarkRed},
          {"dark_green", Painter::kDarkGreen},
          {"dark_blue", Painter::kDarkBlue},
          {"dark_cyan", Painter::kDarkCyan},
          {"dark_magenta", Painter::kDarkMagenta},
          {"dark_yellow", Painter::kDarkYellow},
          {"orange", Painter::kOrange},
          {"purple", Painter::kPurple},
          {"lime", Painter::kLime},
          {"teal", Painter::kTeal},
          {"pink", Painter::kPink},
          {"brown", Painter::kBrown},
          {"indigo", Painter::kIndigo},
          {"turquoise", Painter::kTurquoise},
          {"transparent", Painter::kTransparent}};
}

Painter::Color Painter::stringToColor(const std::string& color,
                                      utl::Logger* logger)
{
  const auto defined_colors = colors();
  auto find_color = defined_colors.find(color);
  if (find_color != defined_colors.end()) {
    return find_color->second;
  }

  if (color[0] == '#' && (color.size() == 7 || color.size() == 9)) {
    uint32_t hex_color = 0;
    for (int i = 1; i < color.size(); i++) {
      const char c = std::tolower(color[i]);
      hex_color *= 16;
      if (c >= '0' && c <= '9') {
        hex_color += c - '0';
      } else if (c >= 'a' && c <= 'f') {
        hex_color += c - 'a' + 10;
      } else {
        logger->error(utl::GUI, 43, "Unable to decode color: {}", color);
      }
    }
    if (color.size() == 7) {
      hex_color *= 256;
      hex_color += 255;
    }
    Painter::Color new_color;
    new_color.r = (hex_color & 0xff000000) >> 24;
    new_color.g = (hex_color & 0x00ff0000) >> 16;
    new_color.b = (hex_color & 0x0000ff00) >> 8;
    new_color.a = hex_color & 0x000000ff;

    return new_color;
  }
  logger->error(utl::GUI, 42, "Color not recognized: {}", color);

  return Painter::kBlack;
}

std::string Painter::colorToString(const Color& color)
{
  for (const auto& [name, c] : colors()) {
    if (c == color) {
      return name;
    }
  }

  return fmt::format(
      "#{:02X}{:02X}{:02X}{:02X}", color.r, color.g, color.b, color.a);
}

std::map<std::string, Painter::Anchor> Painter::anchors()
{
  return {{"bottom left", Painter::Anchor::kBottomLeft},
          {"bottom right", Painter::Anchor::kBottomRight},
          {"top left", Painter::Anchor::kTopLeft},
          {"top right", Painter::Anchor::kTopRight},
          {"center", Painter::Anchor::kCenter},
          {"bottom center", Painter::Anchor::kBottomCenter},
          {"top center", Painter::Anchor::kTopCenter},
          {"left center", Painter::Anchor::kLeftCenter},
          {"right center", Painter::Anchor::kRightCenter}};
}

Painter::Anchor Painter::stringToAnchor(const std::string& anchor,
                                        utl::Logger* logger)
{
  const auto defined_anchors = anchors();
  auto find_anchor = defined_anchors.find(anchor);
  if (find_anchor != defined_anchors.end()) {
    return find_anchor->second;
  }

  logger->error(utl::GUI, 45, "Anchor not recognized: {}", anchor);

  return Anchor::kCenter;
}

std::string Painter::anchorToString(const Anchor& anchor)
{
  for (const auto& [name, c] : anchors()) {
    if (c == anchor) {
      return name;
    }
  }

  return "unknown";
}

}  // namespace web
