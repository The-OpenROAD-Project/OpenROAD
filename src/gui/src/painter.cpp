// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#include "painter.h"

#include <QColor>
#include <QFont>
#include <QString>
#include <QTransform>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <map>
#include <string>

#include "gui/gui.h"
#include "odb/geom.h"
#include "utl/Logger.h"

namespace gui {

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
//////////////////////////////////////////////////////////////////////////

odb::Point GuiPainter::determineStringOrigin(int x,
                                             int y,
                                             Anchor anchor,
                                             const QString& text,
                                             bool rotate_90)
{
  const QRect text_bbox = painter_->fontMetrics().boundingRect(text);
  const QPoint text_bbox_center = text_bbox.center();

  const qreal scale_adjust = 1.0 / getPixelsPerDBU();
  int sx = 0;
  int sy = 0;
  if (anchor == kBottomLeft) {
    // default for Qt
  } else if (anchor == kBottomRight) {
    sx -= text_bbox.right();
  } else if (anchor == kTopLeft) {
    sy += text_bbox.top();
  } else if (anchor == kTopRight) {
    sx -= text_bbox.right();
    sy += text_bbox.top();
  } else if (anchor == kCenter) {
    sx -= text_bbox_center.x();
    sy += text_bbox_center.y();
  } else if (anchor == kBottomCenter) {
    sx -= text_bbox_center.x();
  } else if (anchor == kTopCenter) {
    sx -= text_bbox_center.x();
    sy += text_bbox.top();
  } else if (anchor == kLeftCenter) {
    sy += text_bbox_center.y();
  } else {
    // RIGHT_CENTER
    sx -= text_bbox.right();
    sy += text_bbox_center.y();
  }
  // current units of sx, sy are pixels, so convert to DBU
  sx *= scale_adjust;
  sy *= scale_adjust;
  // add desired text location in DBU
  if (rotate_90) {
    sx *= -1;
    std::swap(sx, sy);
  }
  sx += x;
  sy += y;

  return {sx, sy};
}

void GuiPainter::drawString(int x,
                            int y,
                            Anchor anchor,
                            const std::string& s,
                            bool rotate_90)
{
  const QString text = QString::fromStdString(s);
  const qreal scale_adjust = 1.0 / getPixelsPerDBU();

  odb::Point origin;
  if (rotate_90) {
    // rotating text requires anchor to be Qt default
    origin = determineStringOrigin(x, y, anchor, text, rotate_90);
  } else {
    origin = determineStringOrigin(x, y, anchor, text, rotate_90);
  }

  const QTransform transform = painter_->transform();
  painter_->translate(origin.x(), origin.y());
  painter_->scale(scale_adjust, -scale_adjust);  // undo original scaling
  if (rotate_90) {
    painter_->rotate(90);
  }
  painter_->drawText(
      0, 0, text);  // origin of painter is desired location, so paint at 0, 0
  painter_->setTransform(transform);
}

void GuiPainter::drawRuler(int x0,
                           int y0,
                           int x1,
                           int y1,
                           const std::string& label)
{
  const QColor ruler_color_qt = getOptions()->rulerColor();
  const Color ruler_color(ruler_color_qt.red(),
                          ruler_color_qt.green(),
                          ruler_color_qt.blue(),
                          ruler_color_qt.alpha());
  const QFont ruler_font = getOptions()->rulerFont();
  const QFont restore_font = painter_->font();

  setPen(ruler_color, true);
  setBrush(ruler_color);

  const double x_len = x1 - x0;
  const double y_len = y1 - y0;
  const double len = std::sqrt(x_len * x_len + y_len * y_len);
  if (len == 0) {
    // zero length ruler
    return;
  }

  const QTransform initial_xfm = painter_->transform();

  painter_->translate(x0, y0);
  qreal ruler_angle;
  if (x_len == 0) {
    if (y1 > y0) {
      ruler_angle = 90;
    } else {
      ruler_angle = -90;
    }
  } else if (y_len == 0) {
    if (x1 > x0) {
      ruler_angle = 0;
    } else {
      ruler_angle = -180;
    }
  } else {
    ruler_angle = 57.295779 * std::atan(std::abs(y_len / x_len));  // 180 / pi
    if (x_len < 0) {  // adjust for negative dx
      ruler_angle = 180 - ruler_angle;
    }
    if (y_len < 0) {  // adjust for negative dy
      ruler_angle = -ruler_angle;
    }
  }
  painter_->rotate(ruler_angle);

  const qreal len_microns = len / (qreal) dbu_per_micron_;

  const bool flip_direction = -90 >= ruler_angle || ruler_angle > 90;

  // draw center line
  drawLine(0, 0, len, 0);
  // draw endcaps (arrows) (5 px or 2 DBU if very close)
  int endcap_size = std::max(2.0, 5.0 / getPixelsPerDBU());
  if (flip_direction) {
    endcap_size = -endcap_size;
  }
  drawLine(0, -endcap_size, 0, 0);
  drawLine(len, -endcap_size, len, 0);

  // tick mark interval in microns
  qreal major_tick_mark_interval
      = std::pow(10.0, std::floor(std::log10(len_microns)));
  qreal minor_tick_mark_interval = major_tick_mark_interval / 10;
  const int min_tick_spacing = 10;  // pixels
  const bool do_minor_ticks
      = minor_tick_mark_interval * dbu_per_micron_ * getPixelsPerDBU()
        > min_tick_spacing;

  // draw tick marks
  const int minor_tick_size = endcap_size / 2;
  const int major_tick_interval = major_tick_mark_interval * dbu_per_micron_;
  const int minor_tick_interval = minor_tick_mark_interval * dbu_per_micron_;
  // major ticks
  if (major_tick_interval * getPixelsPerDBU()
      >= min_tick_spacing) {  // only draw tick marks if they are spaces apart
    for (int tick = 0; tick < len; tick += major_tick_interval) {
      if (do_minor_ticks) {
        for (int m = 1; m < 10; m++) {
          const int m_tick = tick + m * minor_tick_interval;
          if (m_tick >= len) {
            break;
          }
          drawLine(m_tick, -minor_tick_size, m_tick, 0);
        }
      }
      if (tick == 0) {
        // don't draw tick mark over end cap
        continue;
      }
      drawLine(tick, -endcap_size, tick, 0);
    }
  }

  setPen(kWhite);
  painter_->setFont(ruler_font);
  painter_->translate(len / 2, 0);
  if (flip_direction) {
    // flip text to keep it in the right position
    painter_->scale(-1, -1);
  }
  std::string text_length = Descriptor::Property::convert_dbu(len, false);
  if (!label.empty()) {
    // label on next to length
    drawString(0, 0, kBottomCenter, label + ": " + text_length);
  } else {
    drawString(0, 0, kBottomCenter, text_length);
  }
  painter_->setFont(restore_font);

  painter_->setTransform(initial_xfm);
}

void GuiPainter::setFont(const Font& font)
{
  const QFont qfont(QString::fromStdString(font.name), font.size);

  painter_->setFont(qfont);
}

}  // namespace gui
