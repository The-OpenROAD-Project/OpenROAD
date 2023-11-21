//////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2019, The Regents of the University of California
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#include "painter.h"

namespace gui {

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
  if (anchor == BOTTOM_LEFT) {
    // default for Qt
  } else if (anchor == BOTTOM_RIGHT) {
    sx -= text_bbox.right();
  } else if (anchor == TOP_LEFT) {
    sy += text_bbox.top();
  } else if (anchor == TOP_RIGHT) {
    sx -= text_bbox.right();
    sy += text_bbox.top();
  } else if (anchor == CENTER) {
    sx -= text_bbox_center.x();
    sy += text_bbox_center.y();
  } else if (anchor == BOTTOM_CENTER) {
    sx -= text_bbox_center.x();
  } else if (anchor == TOP_CENTER) {
    sx -= text_bbox_center.x();
    sy += text_bbox.top();
  } else if (anchor == LEFT_CENTER) {
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

  setPen(white);
  painter_->setFont(ruler_font);
  painter_->translate(len / 2, 0);
  if (flip_direction) {
    // flip text to keep it in the right position
    painter_->scale(-1, -1);
  }
  std::string text_length = Descriptor::Property::convert_dbu(len, false);
  if (!label.empty()) {
    // label on next to length
    drawString(0, 0, BOTTOM_CENTER, label + ": " + text_length);
  } else {
    drawString(0, 0, BOTTOM_CENTER, text_length);
  }
  painter_->setFont(restore_font);

  painter_->setTransform(initial_xfm);
}

}  // namespace gui
