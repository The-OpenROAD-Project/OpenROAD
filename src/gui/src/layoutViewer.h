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

#include "layoutViewer.h"

#include <QApplication>
#include <QDateTime>
#include <QFileDialog>
#include <QFont>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QImageWriter>
#include <QLabel>
#include <QLineEdit>
#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QScrollBar>
#include <QSizePolicy>
#include <QStaticText>
#include <QToolButton>
#include <QToolTip>
#include <QTranslator>
#include <boost/geometry.hpp>
#include <deque>
#include <iostream>
#include <limits>
#include <tuple>
#include <vector>

#include "colorGenerator.h"
#include "db.h"
#include "dbDescriptors.h"
#include "dbTransform.h"
#include "gui/gui.h"
#include "gui_utils.h"
#include "highlightGroupDialog.h"
#include "mainWindow.h"
#include "ruler.h"
#include "scriptWidget.h"
#include "search.h"
#include "utl/Logger.h"

// Qt's coordinate system is defined with the origin at the UPPER-left
// and y values increase as you move DOWN the screen.  All EDA tools
// and formats use the origin at the LOWER-left with y increasing
// as you move UP the screen.  This mismatch is painful.
//
// To workaround it the painter is setup with shifted and flipped
// coordinates to better match EDA style.  However that also
// flips the text which has to be reversed again to account for this.
// In short, yuck!
//
// The pixelsPerDBU_ field stores pixels per DBU.  This adds additional
// trickiness to the coordinates.

namespace gui {

using namespace odb;

// This class wraps the QPainter in the abstract Painter API for
// Renderer instances to use.
class GuiPainter : public Painter
{
 public:
  GuiPainter(QPainter* painter,
             Options* options,
             const odb::Rect& bounds,
             qreal pixels_per_dbu,
             int dbu_per_micron)
      : Painter(options, bounds, pixels_per_dbu),
        painter_(painter),
        dbu_per_micron_(dbu_per_micron)
  {
  }

  Color getPenColor() override
  {
    QColor color = painter_->pen().color();
    return Color(color.red(), color.green(), color.blue(), color.alpha());
  }

  void setPen(odb::dbTechLayer* layer, bool cosmetic = false) override
  {
    QPen pen(getOptions()->color(layer));
    pen.setCosmetic(cosmetic);
    painter_->setPen(pen);
  }

  void setPen(const Color& color, bool cosmetic = false, int width = 1) override
  {
    QPen pen(QColor(color.r, color.g, color.b, color.a));
    pen.setCosmetic(cosmetic);
    pen.setWidth(width);
    painter_->setPen(pen);
  }

  virtual void setPenWidth(int width) override
  {
    QPen pen(painter_->pen().color());
    pen.setCosmetic(painter_->pen().isCosmetic());
    pen.setWidth(width);
    painter_->setPen(pen);
  }

  void setBrush(odb::dbTechLayer* layer, int alpha = -1) override
  {
    QColor color = getOptions()->color(layer);
    Qt::BrushStyle brush_pattern = getOptions()->pattern(layer);
    if (alpha >= 0) {
      color.setAlpha(alpha);
    }
    painter_->setBrush(QBrush(color, brush_pattern));
  }

  void setBrush(const Color& color, const Brush& style = Brush::SOLID) override
  {
    const QColor qcolor(color.r, color.g, color.b, color.a);

    Qt::BrushStyle brush_pattern;
    if (color == Painter::transparent) {
      // if color is transparent, make it no brush
      brush_pattern = Qt::NoBrush;
    } else {
      switch (style) {
        case NONE:
          brush_pattern = Qt::NoBrush;
          break;
        case DIAGONAL:
          brush_pattern = Qt::DiagCrossPattern;
          break;
        case CROSS:
          brush_pattern = Qt::CrossPattern;
          break;
        case DOTS:
          brush_pattern = Qt::Dense6Pattern;
          break;
        case SOLID:
        default:
          brush_pattern = Qt::SolidPattern;
          break;
      }
    }

    painter_->setBrush(QBrush(qcolor, brush_pattern));
  }

  void saveState() override { painter_->save(); }

  void restoreState() override { painter_->restore(); }

  void drawOctagon(const odb::Oct& oct) override
  {
    std::vector<Point> points = oct.getPoints();
    drawPolygon(points);
  }
  void drawRect(const odb::Rect& rect, int roundX = 0, int roundY = 0) override
  {
    if (roundX > 0 || roundY > 0)
      painter_->drawRoundedRect(
          QRect(rect.xMin(), rect.yMin(), rect.dx(), rect.dy()),
          roundX,
          roundY,
          Qt::RelativeSize);
    else
      painter_->drawRect(QRect(rect.xMin(), rect.yMin(), rect.dx(), rect.dy()));
  }
  void drawPolygon(const std::vector<odb::Point>& points) override
  {
    QPolygon poly;
    for (const auto& pt : points) {
      poly.append(QPoint(pt.x(), pt.y()));
    }
    painter_->drawPolygon(poly);
  }
  void drawLine(const odb::Point& p1, const odb::Point& p2) override
  {
    painter_->drawLine(p1.x(), p1.y(), p2.x(), p2.y());
  }
  using Painter::drawLine;

  void drawCircle(int x, int y, int r) override
  {
    painter_->drawEllipse(QPoint(x, y), r, r);
  }

  void drawX(int x, int y, int size) override
  {
    const int o = size / 2;
    painter_->drawLine(x - o, y - o, x + o, y + o);
    painter_->drawLine(x - o, y + o, x + o, y - o);
  }

  const odb::Point determineStringOrigin(int x,
                                         int y,
                                         Anchor anchor,
                                         const QString& text,
                                         bool rotate_90 = false)
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

  // NOTE: The constant height text s drawn with this function, hence
  //       the transformation is mapped to the base transformation and
  //       the world co-ordinates are mapped to the window co-ordinates
  //       before drawing.
  void drawString(int x,
                  int y,
                  Anchor anchor,
                  const std::string& s,
                  bool rotate_90 = false) override
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

  virtual const odb::Rect stringBoundaries(int x,
                                           int y,
                                           Anchor anchor,
                                           const std::string& s) override
  {
    const QString text = QString::fromStdString(s);
    const odb::Point origin = determineStringOrigin(x, y, anchor, text);
    const qreal scale_adjust = 1.0 / getPixelsPerDBU();

    const QRect text_bbox = painter_->fontMetrics().boundingRect(text);
    const int xMin = origin.x() - text_bbox.left() * scale_adjust;
    const int yMin = origin.y() - text_bbox.bottom() * scale_adjust;
    const int xMax = xMin + text_bbox.width() * scale_adjust;
    const int yMax = yMin + text_bbox.height() * scale_adjust;
    return {xMin, yMin, xMax, yMax};
  }

  void drawRuler(int x0,
                 int y0,
                 int x1,
                 int y1,
                 bool euclidian = true,
                 const std::string& label = "") override
  {
    if (euclidian) {
      drawRuler(x0, y0, x1, y1, label);
    } else {
      const int x_dist = std::abs(x0 - x1);
      const int y_dist = std::abs(y0 - y1);
      std::string x_label = label;
      std::string y_label = "";
      if (y_dist > x_dist) {
        std::swap(x_label, y_label);
      }
      const odb::Point mid_pt = Ruler::getManhattanJoinPt({x0, y0}, {x1, y1});
      drawRuler(x0, y0, mid_pt.x(), mid_pt.y(), x_label);
      drawRuler(mid_pt.x(), mid_pt.y(), x1, y1, y_label);
    }
  }

  QPainter* getPainter() { return painter_; }

 private:
  QPainter* painter_;
  int dbu_per_micron_;

  void drawRuler(int x0, int y0, int x1, int y1, const std::string& label)
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
};

LayoutViewer::LayoutViewer(Options* options,
                           ScriptWidget* output_widget,
                           const SelectionSet& selected,
                           const HighlightSet& highlighted,
                           const std::vector<std::unique_ptr<Ruler>>& rulers,
                           Gui* gui,
                           std::function<bool(void)> usingDBU,
                           std::function<bool(void)> showRulerAsEuclidian,
                           QWidget* parent)
    : QWidget(parent),
      block_(nullptr),
      options_(options),
      output_widget_(output_widget),
      selected_(selected),
      highlighted_(highlighted),
      rulers_(rulers),
      scroller_(nullptr),
      pixels_per_dbu_(1.0),
      fit_pixels_per_dbu_(1.0),
      min_depth_(0),
      max_depth_(99),
      rubber_band_showing_(false),
      gui_(gui),
      usingDBU_(usingDBU),
      showRulerAsEuclidian_(showRulerAsEuclidian),
      building_ruler_(false),
      ruler_start_(nullptr),
      snap_edge_showing_(false),
      snap_edge_(),
      inspector_selection_(Selected()),
      focus_(Selected()),
      animate_selection_(nullptr),
      block_drawing_(nullptr),
      repaint_requested_(true),
      last_paint_time_(),
      repaint_interval_(0),
      logger_(nullptr),
      layout_context_menu_(new QMenu(tr("Layout Menu"), this))
{
  setMouseTracking(true);

  addMenuAndActions();

  connect(&search_, SIGNAL(modified()), this, SLOT(fullRepaint()));

  connect(&search_,
          SIGNAL(newBlock(odb::dbBlock*)),
          this,
          SLOT(setBlock(odb::dbBlock*)));

  connect(output_widget_,
          SIGNAL(commandExecuted(bool)),
          this,
          SLOT(setResetRepaintInterval()));
  connect(output_widget_,
          SIGNAL(executionPaused()),
          this,
          SLOT(setResetRepaintInterval()));
  connect(output_widget_,
          SIGNAL(commandAboutToExecute()),
          this,
          SLOT(setLongRepaintInterval()));
}

void LayoutViewer::updateModuleVisibility(odb::dbModule* module, bool visible)
{
  modules_[module].visible = visible;
  fullRepaint();
}

void LayoutViewer::updateModuleColor(odb::dbModule* module,
                                     const QColor& color,
                                     bool user_selected)
{
  modules_[module].color = color;
  if (user_selected) {
    modules_[module].user_color = color;
  }
  fullRepaint();
}

void LayoutViewer::populateModuleColors()
{
  modules_.clear();

  if (block_ == nullptr) {
    return;
  }

  ColorGenerator generator;

  for (auto* module : block_->getModules()) {
    auto color = generator.getQColor();
    modules_[module] = {color, color, color, true};
  }
}

void LayoutViewer::setBlock(odb::dbBlock* block)
{
  block_ = block;

  populateModuleColors();

  updateScaleAndCentering(scroller_->maximumViewportSize());
  fit();
}

void LayoutViewer::setLogger(utl::Logger* logger)
{
  logger_ = logger;
}

void LayoutViewer::startRulerBuild()
{
  building_ruler_ = true;
  snap_edge_showing_ = false;
}

void LayoutViewer::cancelRulerBuild()
{
  building_ruler_ = false;
  snap_edge_showing_ = false;
  ruler_start_ = nullptr;

  update();
}

Rect LayoutViewer::getBounds() const
{
  Rect bbox = block_->getBBox()->getBox();

  Rect die = block_->getDieArea();

  bbox.merge(die);

  return bbox;
}

Rect LayoutViewer::getPaddedRect(const Rect& rect, double factor)
{
  const int margin = factor * std::max(rect.dx(), rect.dy());
  return Rect(rect.xMin() - margin,
              rect.yMin() - margin,
              rect.xMax() + margin,
              rect.yMax() + margin);
}

qreal LayoutViewer::computePixelsPerDBU(const QSize& size, const Rect& dbu_rect)
{
  return std::min(size.width() / (double) dbu_rect.dx(),
                  size.height() / (double) dbu_rect.dy());
}

void LayoutViewer::setPixelsPerDBU(qreal pixels_per_dbu)
{
  if (!hasDesign()) {
    return;
  }

  const Rect fitted_bb = getPaddedRect(getBounds());
  // ensure max size is not exceeded
  qreal maximum_pixels_per_dbu_
      = 0.98
        * computePixelsPerDBU(QSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX),
                              fitted_bb);
  qreal target_pixels_per_dbu
      = std::min(pixels_per_dbu, maximum_pixels_per_dbu_);

  const QSize new_size(ceil(fitted_bb.dx() * target_pixels_per_dbu),
                       ceil(fitted_bb.dy() * target_pixels_per_dbu));

  resize(new_size.expandedTo(scroller_->maximumViewportSize()));
  update();
}

odb::Point LayoutViewer::getVisibleCenter()
{
  return center_;
}

void LayoutViewer::setResolution(qreal pixels_per_dbu)
{
  odb::Point center = getVisibleCenter();
  setPixelsPerDBU(pixels_per_dbu);
  centerAt(center);
}

void LayoutViewer::updateCenter(int dx, int dy)
{
  // modify the center according to the dx and dy
  center_.setX(center_.x() - dx / pixels_per_dbu_);
  center_.setY(center_.y() + dy / pixels_per_dbu_);
}

void LayoutViewer::centerAt(const odb::Point& focus)
{
  QPointF pt = dbuToScreen(focus);
  const QPointF shift_window(0.5 * scroller_->horizontalScrollBar()->pageStep(),
                             0.5 * scroller_->verticalScrollBar()->pageStep());
  // apply shift to corner of the window, instead of the center
  pt -= shift_window;

  // set the new position of the scrollbars
  // returns 0 if the value was possible
  // return the actual value of the center if the set point
  // was outside the range of the scrollbar
  auto setScrollBar = [](QScrollBar* bar, int value) -> int {
    const int max_value = bar->maximum();
    const int min_value = bar->minimum();
    if (value > max_value) {
      bar->setValue(max_value);
      return max_value + bar->pageStep() / 2;
    } else if (value < min_value) {
      bar->setValue(min_value);
      return bar->pageStep() / 2;
    } else {
      bar->setValue(value);
      return 0;
    }
  };

  const int x_val = setScrollBar(scroller_->horizontalScrollBar(), pt.x());
  const int y_val = setScrollBar(scroller_->verticalScrollBar(), pt.y());

  // set the center now, since center is modified by the updateCenter
  // we only care of the focus point
  center_ = focus;

  // account for layout window edges from setScrollBar
  odb::Point adjusted_pt = screenToDBU(QPointF(x_val, y_val));
  if (x_val != 0) {
    center_.setX(adjusted_pt.x());
  }
  if (y_val != 0) {
    center_.setY(adjusted_pt.y());
  }
}

void LayoutViewer::zoomIn()
{
  zoomIn(getVisibleCenter(), false);
}

void LayoutViewer::zoomIn(const odb::Point& focus, bool do_delta_focus)
{
  zoom(focus, 1 * zoom_scale_factor_, do_delta_focus);
}

void LayoutViewer::zoomOut()
{
  zoomOut(getVisibleCenter(), false);
}

void LayoutViewer::zoomOut(const odb::Point& focus, bool do_delta_focus)
{
  zoom(focus, 1 / zoom_scale_factor_, do_delta_focus);
}

void LayoutViewer::zoom(const odb::Point& focus,
                        qreal factor,
                        bool do_delta_focus)
{
  qreal old_pixels_per_dbu = pixels_per_dbu_;

  // focus to center, this is only used if doing delta_focus
  // this holds the distance (x and y) from the desired focus point and the
  // current center so the new center can be computed and ensure that the new
  // center is in line with the old center.
  odb::Point center_delta(focus.x() - center_.x(), focus.y() - center_.y());

  // update resolution
  setPixelsPerDBU(pixels_per_dbu_ * factor);

  odb::Point new_center = focus;
  if (do_delta_focus) {
    qreal actual_factor = pixels_per_dbu_ / old_pixels_per_dbu;
    // new center based on focus
    // adjust such that the new center follows the mouse
    new_center = odb::Point(focus.x() - center_delta.x() / actual_factor,
                            focus.y() - center_delta.y() / actual_factor);
  }

  centerAt(new_center);
}

void LayoutViewer::zoomTo(const Rect& rect_dbu)
{
  const Rect padded_rect = getPaddedRect(rect_dbu);

  // set resolution required to view the whole padded rect
  setPixelsPerDBU(
      computePixelsPerDBU(scroller_->maximumViewportSize(), padded_rect));

  // center the layout at the middle of the rect
  centerAt(Point(rect_dbu.xMin() + rect_dbu.dx() / 2,
                 rect_dbu.yMin() + rect_dbu.dy() / 2));
}

int LayoutViewer::edgeToPointDistance(const odb::Point& pt,
                                      const Edge& edge) const
{
  using BPoint = boost::geometry::model::d2::point_xy<int>;
  using BLine = boost::geometry::model::linestring<BPoint>;

  const BPoint bpt(pt.x(), pt.y());

  BLine bline;
  bline.push_back(BPoint(edge.first.x(), edge.first.y()));
  bline.push_back(BPoint(edge.second.x(), edge.second.y()));

  const auto distance = boost::geometry::distance(bpt, bline);

  return distance;
}

bool LayoutViewer::compareEdges(const Edge& lhs, const Edge& rhs) const
{
  const uint64 lhs_length_sqrd = std::pow(lhs.first.x() - lhs.second.x(), 2)
                                 + std::pow(lhs.first.y() - lhs.second.y(), 2);
  const uint64 rhs_length_sqrd = std::pow(rhs.first.x() - rhs.second.x(), 2)
                                 + std::pow(rhs.first.y() - rhs.second.y(), 2);

  return lhs_length_sqrd < rhs_length_sqrd;
}

std::pair<LayoutViewer::Edge, bool> LayoutViewer::searchNearestEdge(
    const odb::Point& pt,
    bool horizontal,
    bool vertical)
{
  if (!hasDesign()) {
    return {Edge(), false};
  }

  const int search_radius = block_->getDbUnitsPerMicron();

  Edge closest_edge;
  int edge_distance = std::numeric_limits<int>::max();

  auto check_rect
      = [this, &pt, &closest_edge, &edge_distance, &horizontal, &vertical](
            const odb::Rect& rect) {
          const odb::Point ll(rect.xMin(), rect.yMin());
          const odb::Point lr(rect.xMax(), rect.yMin());
          const odb::Point ul(rect.xMin(), rect.yMax());
          const odb::Point ur(rect.xMax(), rect.yMax());
          if (horizontal) {
            const Edge top_edge{ul, ur};
            const int top = edgeToPointDistance(pt, top_edge);
            if (top < edge_distance) {
              edge_distance = top;
              closest_edge = top_edge;
            } else if (top == edge_distance
                       && compareEdges(top_edge, closest_edge)) {
              edge_distance = top;
              closest_edge = top_edge;
            }
            const Edge bottom_edge{ll, lr};
            const int bottom = edgeToPointDistance(pt, bottom_edge);
            if (bottom < edge_distance) {
              edge_distance = bottom;
              closest_edge = bottom_edge;
            } else if (bottom == edge_distance
                       && compareEdges(bottom_edge, closest_edge)) {
              edge_distance = bottom;
              closest_edge = bottom_edge;
            }
          }
          if (vertical) {
            const Edge left_edge{ll, ul};
            const int left = edgeToPointDistance(pt, left_edge);
            if (left < edge_distance) {
              edge_distance = left;
              closest_edge = left_edge;
            } else if (left == edge_distance
                       && compareEdges(left_edge, closest_edge)) {
              edge_distance = left;
              closest_edge = left_edge;
            }
            const Edge right_edge{lr, ur};
            const int right = edgeToPointDistance(pt, right_edge);
            if (right < edge_distance) {
              edge_distance = right;
              closest_edge = right_edge;
            } else if (right == edge_distance
                       && compareEdges(right_edge, closest_edge)) {
              edge_distance = right;
              closest_edge = right_edge;
            }
          }
        };

  auto convert_box_to_rect = [](const Search::Box& box) -> odb::Rect {
    const auto min_corner = box.min_corner();
    const auto max_corner = box.max_corner();
    const odb::Point ll(min_corner.x(), min_corner.y());
    const odb::Point ur(max_corner.x(), max_corner.y());

    return odb::Rect(ll, ur);
  };

  // get die bounding box
  Rect bbox = block_->getDieArea();
  check_rect(bbox);

  if (options_->areRegionsVisible()) {
    for (auto* region : block_->getRegions()) {
      for (auto* box : region->getBoundaries()) {
        odb::Rect region_box = box->getBox();
        if (region_box.area() > 0) {
          check_rect(region_box);
        }
      }
    }
  }

  odb::Rect search_line;
  if (horizontal) {
    search_line = odb::Rect(
        pt.x(), pt.y() - search_radius, pt.x(), pt.y() + search_radius);
  } else if (vertical) {
    search_line = odb::Rect(
        pt.x() - search_radius, pt.y(), pt.x() + search_radius, pt.y());
  } else {
    search_line = odb::Rect(pt.x() - search_radius,
                            pt.y() - search_radius,
                            pt.x() + search_radius,
                            pt.y() + search_radius);
  }

  auto inst_range = search_.searchInsts(search_line.xMin(),
                                        search_line.yMin(),
                                        search_line.xMax(),
                                        search_line.yMax(),
                                        instanceSizeLimit());

  const bool inst_pins_visible = options_->areInstancePinsVisible();
  const bool inst_osb_visible = options_->areInstanceBlockagesVisible();
  const bool inst_internals_visible = inst_pins_visible || inst_osb_visible;
  // Cache the search results as we will iterate over the instances
  // for each layer.
  std::vector<dbInst*> insts;
  for (auto& [box, inst] : inst_range) {
    if (options_->isInstanceVisible(inst)) {
      if (inst_internals_visible) {
        // only add inst if it can be used for pin or obs search
        insts.push_back(inst);
      }
      check_rect(convert_box_to_rect(box));
    }
  }

  const int shape_limit = shapeSizeLimit();

  // look for edges in metal shapes
  dbTech* tech = block_->getDataBase()->getTech();
  for (auto layer : tech->getLayers()) {
    if (!options_->isVisible(layer)) {
      continue;
    }

    for (auto inst : insts) {
      dbMaster* master = inst->getMaster();

      const Boxes* inst_boxes = boxesByLayer(master, layer);
      if (inst_boxes == nullptr) {
        continue;
      }
      dbTransform inst_xfm;
      inst->getTransform(inst_xfm);

      if (inst_osb_visible) {
        for (auto& box : inst_boxes->obs) {
          odb::Rect trans_box(box.left(), box.bottom(), box.right(), box.top());
          inst_xfm.apply(trans_box);
          check_rect(trans_box);
        }
      }
      if (inst_pins_visible) {
        for (auto& box : inst_boxes->mterms) {
          odb::Rect trans_box(box.left(), box.bottom(), box.right(), box.top());
          inst_xfm.apply(trans_box);
          check_rect(trans_box);
        }
      }
    }

    auto box_shapes = search_.searchBoxShapes(layer,
                                              search_line.xMin(),
                                              search_line.yMin(),
                                              search_line.xMax(),
                                              search_line.yMax(),
                                              shape_limit);
    for (auto& [box, net] : box_shapes) {
      if (isNetVisible(net)) {
        check_rect(convert_box_to_rect(box));
      }
    }

    auto polygon_shapes = search_.searchPolygonShapes(layer,
                                                      search_line.xMin(),
                                                      search_line.yMin(),
                                                      search_line.xMax(),
                                                      search_line.yMax(),
                                                      shape_limit);
    for (auto& [box, poly, net] : polygon_shapes) {
      if (isNetVisible(net)) {
        check_rect(convert_box_to_rect(box));
      }
    }

    if (options_->areFillsVisible()) {
      auto fills = search_.searchFills(layer,
                                       search_line.xMin(),
                                       search_line.yMin(),
                                       search_line.xMax(),
                                       search_line.yMax(),
                                       shape_limit);
      for (auto& [box, fill] : fills) {
        check_rect(convert_box_to_rect(box));
      }
    }

    if (options_->areObstructionsVisible()) {
      auto obs = search_.searchObstructions(layer,
                                            search_line.xMin(),
                                            search_line.yMin(),
                                            search_line.xMax(),
                                            search_line.yMax(),
                                            shape_limit);
      for (auto& [box, ob] : obs) {
        check_rect(convert_box_to_rect(box));
      }
    }
  }

  if (options_->areBlockagesVisible()) {
    auto blcks = search_.searchBlockages(search_line.xMin(),
                                         search_line.yMin(),
                                         search_line.xMax(),
                                         search_line.yMax(),
                                         shape_limit);
    for (auto& [box, blck] : blcks) {
      check_rect(convert_box_to_rect(box));
    }
  }

  if (options_->areRowsVisible()) {
    for (const auto& [row, row_site] : getRowRects(search_line)) {
      check_rect(row_site);
    }
  }

  const bool ok = edge_distance != std::numeric_limits<int>::max();
  if (!ok) {
    return {Edge(), false};
  }

  // check if edge should point instead
  const int point_snap_distance = std::max(10.0 / pixels_per_dbu_,  // pixels
                                           10.0);                   // DBU

  const std::array<odb::Point, 3> snap_points = {
      closest_edge.first,  // first corner
      odb::Point(
          (closest_edge.first.x() + closest_edge.second.x()) / 2,
          (closest_edge.first.y() + closest_edge.second.y()) / 2),  // midpoint
      closest_edge.second  // last corner
  };
  for (const auto& s_pt : snap_points) {
    if (std::abs(s_pt.x() - pt.x()) < point_snap_distance
        && std::abs(s_pt.y() - pt.y()) < point_snap_distance) {
      // close to point, so snap to that
      closest_edge.first = s_pt;
      closest_edge.second = s_pt;
    }
  }

  return {closest_edge, true};
}

void LayoutViewer::selectAt(odb::Rect region, std::vector<Selected>& selections)
{
  if (!hasDesign()) {
    return;
  }

  // Look for the selected object in reverse layer order
  auto& renderers = Gui::get()->renderers();
  dbTech* tech = block_->getDataBase()->getTech();

  const int shape_limit = shapeSizeLimit();

  if (options_->areBlockagesVisible() && options_->areBlockagesSelectable()) {
    auto blockages = search_.searchBlockages(region.xMin(),
                                             region.yMin(),
                                             region.xMax(),
                                             region.yMax(),
                                             shape_limit);
    for (auto& [box, blockage] : blockages) {
      selections.push_back(gui_->makeSelected(blockage));
    }
  }

  // dbSet doesn't provide a reverse iterator so we have to copy it.
  std::deque<dbTechLayer*> rev_layers;
  for (auto layer : tech->getLayers()) {
    rev_layers.push_front(layer);
  }

  for (auto layer : rev_layers) {
    if (!options_->isVisible(layer) || !options_->isSelectable(layer)) {
      continue;
    }

    for (auto* renderer : renderers) {
      for (auto& selected : renderer->select(layer, region)) {
        // copy selected items from the renderer
        selections.push_back(selected);
      }
    }

    if (options_->areObstructionsVisible()
        && options_->areObstructionsSelectable()) {
      auto obs = search_.searchObstructions(layer,
                                            region.xMin(),
                                            region.yMin(),
                                            region.xMax(),
                                            region.yMax(),
                                            shape_limit);
      for (auto& [box, obs] : obs) {
        selections.push_back(gui_->makeSelected(obs));
      }
    }

    auto box_shapes = search_.searchBoxShapes(layer,
                                              region.xMin(),
                                              region.yMin(),
                                              region.xMax(),
                                              region.yMax(),
                                              shape_limit);

    // Just return the first one
    for (auto& [box, net] : box_shapes) {
      if (isNetVisible(net) && options_->isNetSelectable(net)) {
        selections.push_back(gui_->makeSelected(net));
      }
    }

    auto polygon_shapes = search_.searchPolygonShapes(layer,
                                                      region.xMin(),
                                                      region.yMin(),
                                                      region.xMax(),
                                                      region.yMax(),
                                                      shape_limit);

    // Just return the first one
    for (auto& [box, poly, net] : polygon_shapes) {
      if (isNetVisible(net) && options_->isNetSelectable(net)) {
        selections.push_back(gui_->makeSelected(net));
      }
    }
  }

  // Check for objects not in a layer
  for (auto* renderer : renderers) {
    for (auto& selected : renderer->select(nullptr, region)) {
      selections.push_back(selected);
    }
  }

  // Look for an instance since no shape was found
  auto insts = search_.searchInsts(region.xMin(),
                                   region.yMin(),
                                   region.xMax(),
                                   region.yMax(),
                                   instanceSizeLimit());

  for (auto& [box, inst] : insts) {
    if (options_->isInstanceVisible(inst)
        && options_->isInstanceSelectable(inst)) {
      selections.push_back(gui_->makeSelected(inst));
    }
  }

  if (options_->areRulersVisible() && options_->areRulersSelectable()) {
    // Look for rulers
    // because rulers are 1 pixel wide, we'll add another couple of pixels to
    // its width
    const int ruler_margin = 4 / pixels_per_dbu_;  // 4 pixels in each direction
    for (auto& ruler : rulers_) {
      if (ruler->fuzzyIntersection(region, ruler_margin)) {
        selections.push_back(gui_->makeSelected(ruler.get()));
      }
    }
  }

  if (options_->areRegionsVisible() && options_->areRegionsSelectable()) {
    for (auto db_region : block_->getRegions()) {
      for (auto box : db_region->getBoundaries()) {
        if (box->getBox().intersects(region)) {
          selections.push_back(gui_->makeSelected(db_region));
        }
      }
    }
  }

  if (options_->areRowsVisible() && options_->areRowsSelectable()) {
    for (const auto& [row_obj, rect] : getRowRects(region)) {
      if (row_obj->getObjectType() == odb::dbObjectType::dbRowObj) {
        selections.push_back(
            gui_->makeSelected(static_cast<odb::dbRow*>(row_obj)));
      } else {
        selections.push_back(gui_->makeSelected(DbSiteDescriptor::SpecificSite{
            static_cast<odb::dbSite*>(row_obj), rect}));
      }
    }
  }
}

int LayoutViewer::selectArea(const odb::Rect& area, bool append)
{
  if (!append) {
    emit selected(Selected());  // remove previous selections
  }

  auto selection_set = selectAt(area);
  emit addSelected(selection_set);
  return selection_set.size();
}

SelectionSet LayoutViewer::selectAt(odb::Rect region)
{
  std::vector<Selected> selections;
  selectAt(region, selections);

  SelectionSet selected;
  for (auto& select : selections) {
    selected.insert(select);
  }
  return selected;
}

Selected LayoutViewer::selectAtPoint(odb::Point pt_dbu)
{
  std::vector<Selected> selections;
  selectAt({pt_dbu.x(), pt_dbu.y(), pt_dbu.x(), pt_dbu.y()}, selections);

  if (!selections.empty()) {
    if (selections.size() == 1) {
      // one one item possible, so return that
      return selections[0];
    }

    // more than one item possible, so return the "next" one
    // method: look for the last selected item in the list and select the next
    // one that will emulate a circular queue so we don't just oscillate between
    // the first two
    std::vector<bool> is_selected;
    for (auto& sel : selections) {
      is_selected.push_back(selected_.count(sel) != 0);
    }
    if (std::all_of(
            is_selected.begin(), is_selected.end(), [](bool b) { return b; })) {
      // everything is selected, so just return first item
      return selections[0];
    }
    is_selected.push_back(
        is_selected[0]);  // add first element to make it a "loop"

    int next_selection_idx;
    const int selections_size = static_cast<int>(selections.size());
    // start at end of list and look for the selection item that is directly
    // after a selected item.
    for (next_selection_idx = selections_size; next_selection_idx > 0;
         next_selection_idx--) {
      // looking for true followed by false
      if (is_selected[next_selection_idx - 1]
          && !is_selected[next_selection_idx]) {
        break;
      }
    }
    if (next_selection_idx == selections_size) {
      // found at the end of the list, loop around
      next_selection_idx = 0;
    }

    return selections[next_selection_idx];
  }
  return Selected();
}

odb::Point LayoutViewer::findNextSnapPoint(const odb::Point& end_pt, bool snap)
{
  if (!snap) {
    return end_pt;
  } else {
    odb::Point snapped = end_pt;
    if (snap_edge_showing_) {
      if (snap_edge_.first.x() == snap_edge_.second.x()
          && snap_edge_.first.y() == snap_edge_.second.y()) {  // point snap
        return {snap_edge_.first.x(), snap_edge_.first.y()};
      }

      bool is_vertical_edge = snap_edge_.first.x() == snap_edge_.second.x();
      if (is_vertical_edge) {
        snapped.setX(snap_edge_.first.x());
      } else {
        snapped.setY(snap_edge_.first.y());
      }
    }

    return snapped;
  }
}

odb::Point LayoutViewer::findNextSnapPoint(const odb::Point& end_pt,
                                           const odb::Point& start_pt,
                                           bool snap)
{
  odb::Point snapped = findNextSnapPoint(end_pt, snap);
  if (snap) {
    // snap to horizontal or vertical ruler
    if (std::abs(start_pt.x() - snapped.x())
        < std::abs(start_pt.y() - snapped.y())) {
      // vertical
      snapped.setX(start_pt.x());
    } else {
      snapped.setY(start_pt.y());
    }
  }
  return snapped;
}

odb::Point LayoutViewer::findNextRulerPoint(const odb::Point& mouse)
{
  const bool do_snap = !(qGuiApp->keyboardModifiers() & Qt::ControlModifier);
  if (ruler_start_ == nullptr) {
    return findNextSnapPoint(mouse, do_snap);
  } else {
    const bool do_any_snap = qGuiApp->keyboardModifiers() & Qt::ShiftModifier;
    if (do_any_snap) {
      return findNextSnapPoint(mouse, do_snap);
    } else {
      return findNextSnapPoint(mouse, *ruler_start_, do_snap);
    }
  }
}

void LayoutViewer::mousePressEvent(QMouseEvent* event)
{
  if (!hasDesign()) {
    return;
  }

  mouse_press_pos_ = event->pos();
  Point pt_dbu = screenToDBU(mouse_press_pos_);
  if (building_ruler_ && event->button() == Qt::LeftButton) {
    // build ruler...
    odb::Point next_ruler_pt = findNextRulerPoint(pt_dbu);
    if (ruler_start_ == nullptr) {
      ruler_start_ = std::make_unique<odb::Point>(next_ruler_pt);
    } else {
      emit addRuler(ruler_start_->x(),
                    ruler_start_->y(),
                    next_ruler_pt.x(),
                    next_ruler_pt.y());
      cancelRulerBuild();
    }
  }

  rubber_band_.setTopLeft(mouse_press_pos_);
  rubber_band_.setBottomRight(mouse_press_pos_);
}

void LayoutViewer::mouseMoveEvent(QMouseEvent* event)
{
  if (!hasDesign()) {
    return;
  }

  mouse_move_pos_ = event->pos();

  // emit location in microns
  Point pt_dbu = screenToDBU(mouse_move_pos_);
  emit location(pt_dbu.x(), pt_dbu.y());

  if (building_ruler_) {
    if (!(qGuiApp->keyboardModifiers() & Qt::ControlModifier)) {
      // set to false and toggle to true if edges are available
      snap_edge_showing_ = false;

      bool do_ver = true;
      bool do_hor = true;

      if (ruler_start_ != nullptr
          && !(qGuiApp->keyboardModifiers() & Qt::ShiftModifier)) {
        const odb::Point mouse_pos = pt_dbu;

        if (std::abs(ruler_start_->x() - mouse_pos.x())
            < std::abs(ruler_start_->y() - mouse_pos.y())) {
          // mostly vertical, so don't look for vertical snaps
          do_ver = false;
        } else {
          // mostly horizontal, so don't look for horizontal snaps
          do_hor = false;
        }
      }

      const auto& [edge, ok] = searchNearestEdge(pt_dbu, do_hor, do_ver);
      snap_edge_ = edge;
      snap_edge_showing_ = ok;
    } else {
      snap_edge_showing_ = false;
    }

    update();
  }
  if (!rubber_band_.isNull()) {
    rubber_band_.setBottomRight(mouse_move_pos_);
    const QRect rect = rubber_band_.normalized();
    if (rect.width() >= 4 && rect.height() >= 4) {
      rubber_band_showing_ = true;
      setCursor(Qt::CrossCursor);
    }
    if (rubber_band_showing_) {
      update();
    }
  }
}

void LayoutViewer::mouseReleaseEvent(QMouseEvent* event)
{
  if (!hasDesign()) {
    return;
  }

  QPoint mouse_pos = event->pos();

  const QRect rubber_band = rubber_band_.normalized();
  if (!rubber_band_.isNull()) {
    rubber_band_ = QRect();
  }

  if (rubber_band_showing_) {
    Rect rubber_band_dbu = screenToDBU(rubber_band);

    rubber_band_showing_ = false;
    update();

    unsetCursor();

    // Clip to the block bounds
    Rect bbox = getPaddedRect(getBounds());

    rubber_band_dbu.set_xlo(qMax(rubber_band_dbu.xMin(), bbox.xMin()));
    rubber_band_dbu.set_ylo(qMax(rubber_band_dbu.yMin(), bbox.yMin()));
    rubber_band_dbu.set_xhi(qMin(rubber_band_dbu.xMax(), bbox.xMax()));
    rubber_band_dbu.set_yhi(qMin(rubber_band_dbu.yMax(), bbox.yMax()));

    if (event->button() == Qt::LeftButton) {
      auto selection = selectAt(rubber_band_dbu);
      if (!(qGuiApp->keyboardModifiers() & Qt::ShiftModifier)) {
        emit selected(Selected());  // remove previous selections
      }
      emit addSelected(selection);
    } else if (event->button() == Qt::RightButton) {
      zoomTo(rubber_band_dbu);
    }
  } else {
    if (event->button() == Qt::LeftButton) {
      Point pt_dbu = screenToDBU(mouse_pos);
      Selected selection = selectAtPoint(pt_dbu);
      if (qGuiApp->keyboardModifiers() & Qt::ShiftModifier) {
        emit addSelected(selection);
      } else {
        emit selected(selection,
                      qGuiApp->keyboardModifiers() & Qt::ControlModifier);
      }
    } else if (event->button() == Qt::RightButton) {
      if (!(QApplication::keyboardModifiers() & Qt::ControlModifier)) {
        showLayoutCustomMenu(event->pos());
      }
    }
  }
}

void LayoutViewer::resizeEvent(QResizeEvent* event)
{
  fullRepaint();

  if (hasDesign()) {
    updateScaleAndCentering(event->size());
  }
}

void LayoutViewer::updateScaleAndCentering(const QSize& new_size)
{
  if (hasDesign()) {
    const odb::Rect block_bounds = getBounds();

    // compute new pixels_per_dbu_
    pixels_per_dbu_
        = computePixelsPerDBU(new_size, getPaddedRect(block_bounds));

    // compute new centering shift
    // the offset necessary to center the block in the viewport.
    // expand area to fill whole scroller window
    const QSize new_area = new_size.expandedTo(scroller_->size());
    centering_shift_
        = QPoint((new_area.width() - block_bounds.dx() * pixels_per_dbu_) / 2,
                 (new_area.height() + block_bounds.dy() * pixels_per_dbu_) / 2);

    fullRepaint();
  }
}

QColor LayoutViewer::getColor(dbTechLayer* layer)
{
  return options_->color(layer);
}

Qt::BrushStyle LayoutViewer::getPattern(dbTechLayer* layer)
{
  return options_->pattern(layer);
}

void LayoutViewer::addInstTransform(QTransform& xfm,
                                    const dbTransform& inst_xfm)
{
  xfm.translate(inst_xfm.getOffset().getX(), inst_xfm.getOffset().getY());

  switch (inst_xfm.getOrient()) {
    case dbOrientType::R0:
      break;
    case dbOrientType::R90:
      xfm.rotate(90);
      break;
    case dbOrientType::R180:
      xfm.rotate(180);
      break;
    case dbOrientType::R270:
      xfm.rotate(270);
      break;
    case dbOrientType::MY:
      xfm.scale(-1, 1);
      break;
    case dbOrientType::MYR90:
      xfm.rotate(90);
      xfm.scale(-1, 1);
      break;
    case dbOrientType::MX:
      xfm.scale(1, -1);
      break;
    case dbOrientType::MXR90:
      xfm.rotate(90);
      xfm.scale(1, -1);
      break;
    default:
      break;  // error
  }
}

// Cache the boxes for shapes in obs/mterm by layer per master for
// drawing performance
void LayoutViewer::boxesByLayer(dbMaster* master, LayerBoxes& boxes)
{
  auto box_to_qrect = [](odb::dbBox* box) -> QRect {
    return QRect(box->xMin(),
                 box->yMin(),
                 box->xMax() - box->xMin(),
                 box->yMax() - box->yMin());
  };

  // store obstructions
  for (dbBox* box : master->getObstructions()) {
    dbTechLayer* layer = box->getTechLayer();
    dbTechLayerType type = layer->getType();
    if (type != dbTechLayerType::ROUTING && type != dbTechLayerType::CUT) {
      continue;
    }
    boxes[layer].obs.emplace_back(box_to_qrect(box));
  }

  // store mterms
  for (dbMTerm* mterm : master->getMTerms()) {
    for (dbMPin* mpin : mterm->getMPins()) {
      for (dbBox* box : mpin->getGeometry()) {
        if (box->isVia()) {
          odb::dbTechVia* tech_via = box->getTechVia();
          if (tech_via == nullptr) {
            continue;
          }

          const odb::dbTransform via_transform(box->getViaXY());
          for (auto* via_box : tech_via->getBoxes()) {
            odb::Rect box_rect = via_box->getBox();
            dbTechLayer* layer = via_box->getTechLayer();
            dbTechLayerType type = layer->getType();
            if (type != dbTechLayerType::ROUTING
                && type != dbTechLayerType::CUT) {
              continue;
            }
            via_transform.apply(box_rect);
            boxes[layer].mterms.emplace_back(box_to_qrect(via_box));
          }
        } else {
          dbTechLayer* layer = box->getTechLayer();
          dbTechLayerType type = layer->getType();
          if (type != dbTechLayerType::ROUTING
              && type != dbTechLayerType::CUT) {
            continue;
          }
          boxes[layer].mterms.emplace_back(box_to_qrect(box));
        }
      }
    }
  }
}

// Get the boxes for the given layer & master from the cache,
// populating the cache if necessary
const LayoutViewer::Boxes* LayoutViewer::boxesByLayer(dbMaster* master,
                                                      dbTechLayer* layer)
{
  auto it = cell_boxes_.find(master);
  if (it == cell_boxes_.end()) {
    LayerBoxes& boxes = cell_boxes_[master];
    boxesByLayer(master, boxes);
  }
  it = cell_boxes_.find(master);
  LayerBoxes& boxes = it->second;

  auto layer_it = boxes.find(layer);
  if (layer_it != boxes.end()) {
    return &layer_it->second;
  }
  return nullptr;
}

void LayoutViewer::drawTracks(dbTechLayer* layer,
                              QPainter* painter,
                              const Rect& bounds)
{
  if (!options_->arePrefTracksVisible()
      && !options_->areNonPrefTracksVisible()) {
    return;
  }

  dbTrackGrid* grid = block_->findTrackGrid(layer);
  if (!grid) {
    return;
  }

  Rect block_bounds = block_->getDieArea();
  if (!block_bounds.intersects(bounds)) {
    return;
  }
  const Rect draw_bounds = block_bounds.intersect(bounds);
  const int min_resolution = shapeSizeLimit();

  bool is_horizontal = layer->getDirection() == dbTechLayerDir::HORIZONTAL;
  std::vector<int> grids;
  if ((!is_horizontal && options_->arePrefTracksVisible())
      || (is_horizontal && options_->areNonPrefTracksVisible())) {
    bool show_grid = true;
    for (int i = 0; i < grid->getNumGridPatternsX(); i++) {
      int origin, line_count, step;
      grid->getGridPatternX(i, origin, line_count, step);
      show_grid &= step > min_resolution;
    }

    if (show_grid) {
      grid->getGridX(grids);
      for (int x : grids) {
        if (x < draw_bounds.xMin()) {
          continue;
        }
        if (x > draw_bounds.xMax()) {
          break;
        }
        painter->drawLine(x, draw_bounds.yMin(), x, draw_bounds.yMax());
      }
    }
  }

  if ((is_horizontal && options_->arePrefTracksVisible())
      || (!is_horizontal && options_->areNonPrefTracksVisible())) {
    bool show_grid = true;
    for (int i = 0; i < grid->getNumGridPatternsY(); i++) {
      int origin, line_count, step;
      grid->getGridPatternY(i, origin, line_count, step);
      show_grid &= step > min_resolution;
    }

    if (show_grid) {
      grid->getGridY(grids);
      for (int y : grids) {
        if (y < draw_bounds.yMin()) {
          continue;
        }
        if (y > draw_bounds.yMax()) {
          break;
        }
        painter->drawLine(draw_bounds.xMin(), y, draw_bounds.xMax(), y);
      }
    }
  }
}

void LayoutViewer::drawRows(QPainter* painter, const Rect& bounds)
{
  if (!options_->areRowsVisible()) {
    return;
  }

  QPen pen(options_->rowColor());
  pen.setCosmetic(true);
  painter->setPen(pen);
  painter->setBrush(Qt::NoBrush);

  for (const auto& [row, row_site] : getRowRects(bounds)) {
    painter->drawRect(
        row_site.xMin(), row_site.yMin(), row_site.dx(), row_site.dy());
  }
}

std::vector<std::pair<odb::dbObject*, odb::Rect>> LayoutViewer::getRowRects(
    const odb::Rect& bounds)
{
  int min_resolution = nominalViewableResolution();
  if (options_->isDetailedVisibility()) {
    min_resolution = 0;
  }

  auto rows = search_.searchRows(bounds.xMin(),
                                 bounds.yMin(),
                                 bounds.xMax(),
                                 bounds.yMax(),
                                 min_resolution);

  std::vector<std::pair<odb::dbObject*, odb::Rect>> rects;
  for (auto& [box, row] : rows) {
    int x;
    int y;
    row->getOrigin(x, y);

    rects.emplace_back(row, row->getBBox());

    dbSite* site = row->getSite();
    int spacing = row->getSpacing();
    int w = site->getWidth();
    int h = site->getHeight();

    bool w_visible = w >= min_resolution;
    bool h_visible = h >= min_resolution;

    switch (row->getOrient()) {
      case dbOrientType::R0:
      case dbOrientType::R180:
      case dbOrientType::MY:
      case dbOrientType::MX:
        /* do nothing */
        break;

      case dbOrientType::R90:
      case dbOrientType::R270:
      case dbOrientType::MYR90:
      case dbOrientType::MXR90:
        std::swap(w, h);
    }

    dbRowDir dir = row->getDirection();
    int count = row->getSiteCount();
    odb::dbObject* obj = row;
    if (!w_visible) {
      // individual sites not visible, just draw the row
      if (dir == dbRowDir::HORIZONTAL) {
        w = spacing * count;
      } else {
        h = spacing * count;
      }
      count = 1;
    } else {
      obj = site;
    }
    if (h_visible) {
      // row height can be seen
      for (int i = 0; i < count; ++i) {
        const Rect row_rect(x, y, x + w, y + h);
        if (row_rect.intersects(bounds)) {
          // only paint rows that can be seen
          rects.emplace_back(obj, row_rect);
        }

        if (dir == dbRowDir::HORIZONTAL) {
          x += spacing;
        } else {
          y += spacing;
        }
      }
    }
  }

  return rects;
}

void LayoutViewer::selection(const Selected& selection)
{
  inspector_selection_ = selection;
  if (selected_.size() > 1) {
    selectionAnimation(selection);
  } else {
    // stop animation
    selectionAnimation(Selected());
  }
  focus_ = Selected();  // reset focus
  update();
}

void LayoutViewer::selectionFocus(const Selected& focus)
{
  focus_ = focus;
  selectionAnimation(focus_);
  update();
}

void LayoutViewer::selectionAnimation(const Selected& selection,
                                      int repeats,
                                      int update_interval)
{
  if (animate_selection_ != nullptr) {
    animate_selection_->timer->stop();
    animate_selection_ = nullptr;
  }

  if (selection) {
    const int state_reset_interval = 3;
    animate_selection_ = std::make_unique<AnimatedSelected>(
        AnimatedSelected{selection,
                         0,
                         state_reset_interval * repeats,
                         state_reset_interval,
                         nullptr});
    animate_selection_->timer = std::make_unique<QTimer>();
    animate_selection_->timer->setInterval(update_interval);

    const qint64 max_animate_time
        = QDateTime::currentMSecsSinceEpoch()
          + (animate_selection_->max_state_count + 2) * update_interval;
    connect(animate_selection_->timer.get(),
            &QTimer::timeout,
            [this, max_animate_time]() {
              if (animate_selection_ == nullptr) {
                return;
              }

              animate_selection_->state_count++;
              if (animate_selection_->max_state_count != 0
                  &&  // if max_state_count == 0 animate until new animation is
                      // selected
                  (animate_selection_->state_count
                       == animate_selection_->max_state_count
                   || QDateTime::currentMSecsSinceEpoch() > max_animate_time)) {
                animate_selection_->timer->stop();
                animate_selection_ = nullptr;
              }

              update();
            });

    animate_selection_->timer->start();
  }
}

void LayoutViewer::drawSelected(Painter& painter)
{
  if (!options_->areSelectedVisible()) {
    return;
  }

  for (auto& selected : selected_) {
    selected.highlight(painter, Painter::highlight);
  }

  if (animate_selection_ != nullptr) {
    auto brush = Painter::transparent;

    const int pen_width
        = animate_selection_->state_count % animate_selection_->state_modulo
          + 1;
    if (pen_width == 1) {
      // flash with brush, since pen width is the same as normal
      brush = Painter::highlight;
      brush.a = 100;
    }

    animate_selection_->selection.highlight(
        painter, Painter::highlight, pen_width, brush);
  }

  if (focus_) {
    focus_.highlight(painter,
                     Painter::highlight,
                     1,
                     Painter::highlight,
                     Painter::Brush::DIAGONAL);
  }
}

void LayoutViewer::drawHighlighted(Painter& painter)
{
  int highlight_group = 0;
  for (auto& highlight_set : highlighted_) {
    auto highlight_color = Painter::highlightColors[highlight_group];

    for (auto& highlighted : highlight_set) {
      highlighted.highlight(painter, highlight_color, 1, highlight_color);
    }

    highlight_group++;
  }
}

void LayoutViewer::drawRulers(Painter& painter)
{
  if (!options_->areRulersVisible()) {
    return;
  }

  for (auto& ruler : rulers_) {
    painter.drawRuler(ruler->getPt0().x(),
                      ruler->getPt0().y(),
                      ruler->getPt1().x(),
                      ruler->getPt1().y(),
                      ruler->isEuclidian(),
                      ruler->getLabel());
  }
}

// Draw the instances bounds
void LayoutViewer::drawInstanceOutlines(QPainter* painter,
                                        const std::vector<odb::dbInst*>& insts)
{
  int minimum_height_for_tag = nominalViewableResolution();
  int minimum_size = fineViewableResolution();
  const QTransform initial_xfm = painter->transform();

  painter->setPen(QPen(Qt::gray, 0));
  painter->setBrush(QBrush());
  for (auto inst : insts) {
    dbMaster* master = inst->getMaster();
    // setup the instance's transform
    QTransform xfm = initial_xfm;
    dbTransform inst_xfm;
    inst->getTransform(inst_xfm);
    addInstTransform(xfm, inst_xfm);
    painter->setTransform(xfm);

    // draw bbox
    Rect master_box;
    master->getPlacementBoundary(master_box);

    if (minimum_size > master_box.dx() && minimum_size > master_box.dy()) {
      painter->drawPoint(master_box.xMin(), master_box.yMin());
    } else {
      painter->drawRect(master_box.xMin(),
                        master_box.yMin(),
                        master_box.dx(),
                        master_box.dy());

      // Draw an orientation tag in corner if useful in size
      qreal master_h = master->getHeight();
      if (master_h >= minimum_height_for_tag) {
        qreal master_w = master->getWidth();
        qreal tag_width = std::min(0.25 * master_w, 0.125 * master_h);
        qreal tag_x = master_box.xMin() + tag_width;
        qreal tag_y = master_box.yMin() + tag_width * 2;
        painter->drawLine(QPointF(tag_x, master_box.yMin()),
                          QPointF(master_box.xMin(), tag_y));
      }
    }
  }
  painter->setTransform(initial_xfm);
}

// Draw the instances' shapes
void LayoutViewer::drawInstanceShapes(dbTechLayer* layer,
                                      QPainter* painter,
                                      const std::vector<odb::dbInst*>& insts)
{
  const bool show_blockages = options_->areInstanceBlockagesVisible();
  const bool show_pins = options_->areInstancePinsVisible();
  if (!show_blockages && !show_pins) {
    return;
  }

  const int minimum_height = nominalViewableResolution();
  const QTransform initial_xfm = painter->transform();
  // Draw the instances' shapes
  for (auto inst : insts) {
    dbMaster* master = inst->getMaster();
    if (master->getHeight() < minimum_height) {
      continue;
    }

    const Boxes* boxes = boxesByLayer(master, layer);

    if (boxes == nullptr) {
      continue;  // no shapes on this layer
    }

    // setup the instance's transform
    QTransform xfm = initial_xfm;
    dbTransform inst_xfm;
    inst->getTransform(inst_xfm);
    addInstTransform(xfm, inst_xfm);
    painter->setTransform(xfm);

    // Only draw the pins/obs if they are big enough to be useful
    painter->setPen(Qt::NoPen);
    QColor color = getColor(layer);
    Qt::BrushStyle brush_pattern = getPattern(layer);

    if (show_blockages) {
      painter->setBrush(color.lighter());
      for (const auto& box : boxes->obs) {
        painter->drawRect(box);
      }
    }

    if (show_pins) {
      painter->setBrush(QBrush(color, brush_pattern));
      for (const auto& box : boxes->mterms) {
        painter->drawRect(box);
      }
    }
  }

  painter->setTransform(initial_xfm);
}

// Draw the instances' names
void LayoutViewer::drawInstanceNames(QPainter* painter,
                                     const std::vector<odb::dbInst*>& insts)
{
  if (!options_->areInstanceNamesVisible()) {
    return;
  }

  const int minimum_size = coarseViewableResolution();
  const QTransform initial_xfm = painter->transform();

  const QColor text_color = options_->instanceNameColor();
  painter->setPen(QPen(text_color, 0));
  painter->setBrush(QBrush(text_color));

  const QFont initial_font = painter->font();
  const QFont text_font = options_->instanceNameFont();
  const QFontMetricsF font_metrics(text_font);

  // minimum pixel height for text (10px)
  if (font_metrics.ascent() < 10) {
    // text is too small
    return;
  }

  // text should not fill more than 90% of the instance height or width
  static const float size_limit = 0.9;
  static const float rotation_limit
      = 0.85;  // slightly lower to prevent oscillating rotations when zooming

  // limit non-core text to 1/2.0 (50%) of cell height or width
  static const float non_core_scale_limit = 2.0;

  const qreal scale_adjust = 1.0 / pixels_per_dbu_;

  painter->setFont(text_font);
  for (auto inst : insts) {
    dbMaster* master = inst->getMaster();
    int master_height = master->getHeight();
    int master_width = master->getHeight();

    if (master_height < minimum_size) {
      continue;
    } else if (!inst->getMaster()->isCore() && master_width < minimum_size) {
      // if core cell, just check master height
      continue;
    }

    Rect instance_box = inst->getBBox()->getBox();

    QString name = inst->getName().c_str();
    QRectF instance_bbox_in_px = dbuToScreen(instance_box);

    QRectF text_bounding_box = font_metrics.boundingRect(name);

    bool do_rotate = false;
    if (text_bounding_box.width()
        > rotation_limit * instance_bbox_in_px.width()) {
      // non-rotated text will not fit without elide
      if (instance_bbox_in_px.height() > instance_bbox_in_px.width()) {
        // check if more text will fit if rotated
        do_rotate = true;
      }
    }

    qreal text_height_check = non_core_scale_limit * text_bounding_box.height();
    // don't show text if it's more than "non_core_scale_limit" of cell
    // height/width this keeps text from dominating the cell size
    if (!do_rotate && text_height_check > instance_bbox_in_px.height()) {
      continue;
    }
    if (do_rotate && text_height_check > instance_bbox_in_px.width()) {
      continue;
    }

    if (do_rotate) {
      name = font_metrics.elidedText(
          name, Qt::ElideLeft, size_limit * instance_bbox_in_px.height());
    } else {
      name = font_metrics.elidedText(
          name, Qt::ElideLeft, size_limit * instance_bbox_in_px.width());
    }

    painter->translate(instance_box.xMin(), instance_box.yMin());
    painter->scale(scale_adjust, -scale_adjust);
    if (do_rotate) {
      text_bounding_box = font_metrics.boundingRect(name);
      painter->rotate(90);
      painter->translate(-text_bounding_box.width(), 0);
      // account for descent of font
      painter->translate(-font_metrics.descent(), 0);
    } else {
      // account for descent of font
      painter->translate(font_metrics.descent(), 0);
    }
    painter->drawText(0, 0, name);

    painter->setTransform(initial_xfm);
  }
  painter->setFont(initial_font);
}

void LayoutViewer::drawBlockages(QPainter* painter, const Rect& bounds)
{
  if (!options_->areBlockagesVisible()) {
    return;
  }
  painter->setPen(Qt::NoPen);
  painter->setBrush(QBrush(options_->placementBlockageColor(),
                           options_->placementBlockagePattern()));

  auto blockage_range = search_.searchBlockages(bounds.xMin(),
                                                bounds.yMin(),
                                                bounds.xMax(),
                                                bounds.yMax(),
                                                shapeSizeLimit());

  for (auto& [box, blockage] : blockage_range) {
    Rect bbox = blockage->getBBox()->getBox();
    painter->drawRect(bbox.xMin(), bbox.yMin(), bbox.dx(), bbox.dy());
  }
}

void LayoutViewer::drawObstructions(dbTechLayer* layer,
                                    QPainter* painter,
                                    const Rect& bounds)
{
  if (!options_->areObstructionsVisible() || !options_->isVisible(layer)) {
    return;
  }

  painter->setPen(Qt::NoPen);
  QColor color = getColor(layer).darker();
  Qt::BrushStyle brush_pattern = getPattern(layer);
  painter->setBrush(QBrush(color, brush_pattern));

  auto obstructions_range = search_.searchObstructions(layer,
                                                       bounds.xMin(),
                                                       bounds.yMin(),
                                                       bounds.xMax(),
                                                       bounds.yMax(),
                                                       shapeSizeLimit());

  for (auto& [box, obs] : obstructions_range) {
    Rect bbox = obs->getBBox()->getBox();
    painter->drawRect(bbox.xMin(), bbox.yMin(), bbox.dx(), bbox.dy());
  }
}

// Draw the region of the block.  Depth is not yet used but
// is there for hierarchical design support.
void LayoutViewer::drawBlock(QPainter* painter, const Rect& bounds, int depth)
{
  const int instance_limit = instanceSizeLimit();
  const int shape_limit = shapeSizeLimit();

  LayerBoxes boxes;

  auto& renderers = Gui::get()->renderers();
  GuiPainter gui_painter(painter,
                         options_,
                         bounds,
                         pixels_per_dbu_,
                         block_->getDbUnitsPerMicron());

  // Draw die area, if set
  painter->setPen(QPen(Qt::gray, 0));
  painter->setBrush(QBrush());
  Rect bbox = block_->getDieArea();
  if (bbox.area() > 0) {
    painter->drawRect(bbox.xMin(), bbox.yMin(), bbox.dx(), bbox.dy());
  }

  drawManufacturingGrid(painter, bounds);

  auto inst_range = search_.searchInsts(bounds.xMin(),
                                        bounds.yMin(),
                                        bounds.xMax(),
                                        bounds.yMax(),
                                        instance_limit);

  // Cache the search results as we will iterate over the instances
  // for each layer.
  std::vector<dbInst*> insts;
  insts.reserve(10000);
  for (auto& [box, inst] : inst_range) {
    if (options_->isInstanceVisible(inst)) {
      insts.push_back(inst);
    }
  }

  drawInstanceOutlines(painter, insts);

  // draw blockages
  drawBlockages(painter, bounds);

  dbTech* tech = block_->getDataBase()->getTech();
  for (dbTechLayer* layer : tech->getLayers()) {
    if (!options_->isVisible(layer)) {
      continue;
    }

    // Skip the cut layer if the cuts will be too small to see
    const bool draw_shapes = !(layer->getType() == dbTechLayerType::CUT
                               && cut_maximum_size_[layer] < shape_limit);

    if (draw_shapes) {
      drawInstanceShapes(layer, painter, insts);
    }

    drawObstructions(layer, painter, bounds);

    if (draw_shapes) {
      // Now draw the shapes
      QColor color = getColor(layer);
      Qt::BrushStyle brush_pattern = getPattern(layer);
      painter->setBrush(QBrush(color, brush_pattern));
      painter->setPen(QPen(color, 0));
      auto box_iter = search_.searchBoxShapes(layer,
                                              bounds.xMin(),
                                              bounds.yMin(),
                                              bounds.xMax(),
                                              bounds.yMax(),
                                              instance_limit);

      for (auto& [box, net] : box_iter) {
        if (!isNetVisible(net)) {
          continue;
        }
        const auto& ll = box.min_corner();
        const auto& ur = box.max_corner();
        painter->drawRect(
            QRect(ll.x(), ll.y(), ur.x() - ll.x(), ur.y() - ll.y()));
      }

      auto polygon_iter = search_.searchPolygonShapes(layer,
                                                      bounds.xMin(),
                                                      bounds.yMin(),
                                                      bounds.xMax(),
                                                      bounds.yMax(),
                                                      instance_limit);

      for (auto& [box, poly, net] : polygon_iter) {
        if (!isNetVisible(net)) {
          continue;
        }
        const int size = poly.outer().size();
        QPolygon qpoly(size);
        for (int i = 0; i < size; i++) {
          qpoly.setPoint(i, poly.outer()[i].x(), poly.outer()[i].y());
        }
        painter->drawPolygon(qpoly);
      }

      // Now draw the fills
      if (options_->areFillsVisible()) {
        QColor color = getColor(layer).lighter(50);
        Qt::BrushStyle brush_pattern = getPattern(layer);
        painter->setBrush(QBrush(color, brush_pattern));
        painter->setPen(QPen(color, 0));
        auto iter = search_.searchFills(layer,
                                        bounds.xMin(),
                                        bounds.yMin(),
                                        bounds.xMax(),
                                        bounds.yMax(),
                                        shape_limit);

        for (auto& i : iter) {
          const auto& ll = std::get<0>(i).min_corner();
          const auto& ur = std::get<0>(i).max_corner();
          painter->drawRect(
              QRect(ll.x(), ll.y(), ur.x() - ll.x(), ur.y() - ll.y()));
        }
      }
    }

    if (draw_shapes) {
      drawTracks(layer, painter, bounds);
      drawRouteGuides(gui_painter, layer);
    }

    for (auto* renderer : renderers) {
      gui_painter.saveState();
      renderer->drawLayer(layer, gui_painter);
      gui_painter.restoreState();
    }
  }
  // draw instance names
  drawInstanceNames(painter, insts);

  drawRows(painter, bounds);
  if (options_->areAccessPointsVisible()) {
    drawAccessPoints(gui_painter, insts);
  }

  drawModuleView(painter, insts);

  drawRegions(painter);

  if (options_->arePinMarkersVisible()) {
    drawPinMarkers(gui_painter, bounds);
  }

  drawGCellGrid(painter, bounds);

  for (auto* renderer : renderers) {
    gui_painter.saveState();
    renderer->drawObjects(gui_painter);
    gui_painter.restoreState();
  }
}

void LayoutViewer::drawGCellGrid(QPainter* painter, const odb::Rect& bounds)
{
  if (!options_->isGCellGridVisible()) {
    return;
  }

  odb::dbGCellGrid* grid = block_->getGCellGrid();

  if (grid == nullptr) {
    return;
  }

  const odb::Rect draw_bounds = bounds.intersect(block_->getDieArea());

  std::vector<int> x_grid, y_grid;
  grid->getGridX(x_grid);
  grid->getGridY(y_grid);

  painter->setPen(QPen(Qt::white, 0));

  for (const auto& x : x_grid) {
    if (x < draw_bounds.xMin() || draw_bounds.xMax() < x) {
      continue;
    }

    painter->drawLine(x, draw_bounds.yMin(), x, draw_bounds.yMax());
  }

  for (const auto& y : y_grid) {
    if (y < draw_bounds.yMin() || draw_bounds.yMax() < y) {
      continue;
    }

    painter->drawLine(draw_bounds.xMin(), y, draw_bounds.xMax(), y);
  }
}

void LayoutViewer::drawManufacturingGrid(QPainter* painter,
                                         const odb::Rect& bounds)
{
  if (!options_->isManufacturingGridVisible()) {
    return;
  }

  odb::dbTech* tech = block_->getDb()->getTech();
  if (!tech->hasManufacturingGrid()) {
    return;
  }

  const int grid = tech->getManufacturingGrid();  // DBU

  const int pixels_per_grid_point = grid * pixels_per_dbu_;
  const int pixels_per_grid_point_limit
      = 5;  // want 5 pixels between each grid point

  if (pixels_per_grid_point < pixels_per_grid_point_limit) {
    return;
  }

  const int first_x = ((bounds.xMin() / grid) + 1) * grid;
  const int last_x = (bounds.xMax() / grid) * grid;
  const int first_y = ((bounds.yMin() / grid) + 1) * grid;
  const int last_y = (bounds.yMax() / grid) * grid;

  QPolygon points;
  for (int x = first_x; x <= last_x; x += grid) {
    for (int y = first_y; y <= last_y; y += grid) {
      points.append(QPoint(x, y));
    }
  }

  painter->setPen(QPen(Qt::white, 0));
  painter->drawPoints(points);
}

void LayoutViewer::drawRegions(QPainter* painter)
{
  if (!options_->areRegionsVisible()) {
    return;
  }

  painter->setPen(QPen(Qt::gray, 0));
  painter->setBrush(QBrush(options_->regionColor(), options_->regionPattern()));

  for (auto* region : block_->getRegions()) {
    for (auto* box : region->getBoundaries()) {
      odb::Rect region_box = box->getBox();
      if (region_box.area() > 0) {
        painter->drawRect(region_box.xMin(),
                          region_box.yMin(),
                          region_box.dx(),
                          region_box.dy());
      }
    }
  }
}

void LayoutViewer::drawRouteGuides(Painter& painter, odb::dbTechLayer* layer)
{
  if (route_guides_.empty())
    return;
  painter.setPen(layer);
  painter.setBrush(layer);
  for (auto net : route_guides_) {
    for (auto guide : net->getGuides()) {
      if (guide->getLayer() != layer)
        continue;
      painter.drawRect(guide->getBox());
    }
  }
}
void LayoutViewer::drawAccessPoints(Painter& painter,
                                    const std::vector<odb::dbInst*>& insts)
{
  const int shape_limit = shapeSizeLimit();
  const int shape_size = 100;  // units DBU
  if (shape_limit > shape_size) {
    return;
  }

  const Painter::Color has_access = Painter::green;
  const Painter::Color not_access = Painter::red;

  auto draw = [&](odb::dbAccessPoint* ap, const odb::dbTransform& transform) {
    if (ap == nullptr) {
      return;
    }
    if (!options_->isVisible(ap->getLayer())) {
      return;
    }

    Point pt = ap->getPoint();
    transform.apply(pt);

    auto color = ap->hasAccess() ? has_access : not_access;
    painter.setPen(color, /* cosmetic */ true);
    painter.drawX(pt.x(), pt.y(), shape_size);
  };

  if (options_->areInstancePinsVisible()) {
    for (auto* inst : insts) {
      int x, y;
      inst->getLocation(x, y);
      odb::dbTransform xform({x, y});

      for (auto term : inst->getITerms()) {
        for (auto ap : term->getPrefAccessPoints()) {
          draw(ap, xform);
        }
      }
    }
  }
  for (auto term : block_->getBTerms()) {
    for (auto pin : term->getBPins()) {
      for (auto ap : pin->getAccessPoints()) {
        draw(ap, {});
      }
    }
  }
}

void LayoutViewer::drawModuleView(QPainter* painter,
                                  const std::vector<odb::dbInst*>& insts)
{
  if (!options_->isModuleView()) {
    return;
  }

  for (auto* inst : insts) {
    auto* module = inst->getModule();

    if (module == nullptr) {
      continue;
    }

    const auto setting = modules_[module];

    if (!setting.visible) {
      continue;
    }

    odb::Rect inst_outline = inst->getBBox()->getBox();

    auto color = setting.color;
    painter->setPen(QPen(color, 0));
    color.setAlpha(100);
    painter->setBrush(QBrush(color));
    painter->drawRect(inst_outline.xMin(),
                      inst_outline.yMin(),
                      inst_outline.dx(),
                      inst_outline.dy());
  }
}

void LayoutViewer::drawPinMarkers(Painter& painter, const odb::Rect& bounds)
{
  auto block_bbox = block_->getBBox();
  auto block_width = block_bbox->getWidth();
  auto block_height = block_bbox->getLength();
  const double scale_factor
      = 0.02;  // 4 Percent of bounds is used to draw pin-markers
  const int block_max_dim
      = std::min(std::max(block_width, block_height), bounds.maxDXDY());
  const double abs_min_dim = 8.0;  // prevent markers from falling apart
  const double max_dim = std::max(scale_factor * block_max_dim, abs_min_dim);

  QPainter* qpainter = static_cast<GuiPainter&>(painter).getPainter();
  const QFont initial_font = qpainter->font();
  QFont marker_font = options_->pinMarkersFont();
  qpainter->setFont(marker_font);

  const QFontMetrics font_metrics(marker_font);
  // draw names of pins when 100 pins would fit on an edge
  const bool draw_names = std::max(block_width, block_height) * pixels_per_dbu_
                          > 100 * font_metrics.height();
  const int text_margin = 2.0 / pixels_per_dbu_;

  // templates of pin markers (block top)
  const std::vector<Point> in_marker{// arrow head pointing in to block
                                     Point(max_dim / 4, max_dim),
                                     Point(0, 0),
                                     Point(-max_dim / 4, max_dim),
                                     Point(max_dim / 4, max_dim)};
  const std::vector<Point> out_marker{// arrow head pointing out of block
                                      Point(0, max_dim),
                                      Point(-max_dim / 4, 0),
                                      Point(max_dim / 4, 0),
                                      Point(0, max_dim)};
  const std::vector<Point> bi_marker{// diamond
                                     Point(0, 0),
                                     Point(-max_dim / 4, max_dim / 2),
                                     Point(0, max_dim),
                                     Point(max_dim / 4, max_dim / 2),
                                     Point(0, 0)};

  // RTree used to search for overlapping shapes and decide if rotation of text
  // is needed.
  bgi::rtree<Search::Box, bgi::quadratic<16>> pin_text_spec_shapes;
  struct PinText
  {
    Search::Box rect;
    bool can_rotate;
    odb::dbTechLayer* layer;
    std::string text;
    odb::Point pt;
    Painter::Anchor anchor;
  };
  std::vector<PinText> pin_text_spec;

  for (odb::dbBTerm* term : block_->getBTerms()) {
    for (odb::dbBPin* pin : term->getBPins()) {
      odb::dbPlacementStatus status = pin->getPlacementStatus();
      if (!status.isPlaced()) {
        continue;
      }
      auto pin_dir = term->getIoType();
      for (odb::dbBox* box : pin->getBoxes()) {
        if (!box) {
          continue;
        }
        Point pin_center((box->xMin() + box->xMax()) / 2,
                         (box->yMin() + box->yMax()) / 2);

        auto dist_to_left = std::abs(box->xMin() - block_bbox->xMin());
        auto dist_to_right = std::abs(box->xMax() - block_bbox->xMax());
        auto dist_to_top = std::abs(box->yMax() - block_bbox->yMax());
        auto dist_to_bot = std::abs(box->yMin() - block_bbox->yMin());
        std::vector<int> dists{
            dist_to_left, dist_to_right, dist_to_top, dist_to_bot};
        int arg_min = std::distance(
            dists.begin(), std::min_element(dists.begin(), dists.end()));

        odb::dbTransform xfm(pin_center);
        if (arg_min == 0) {  // left
          xfm.setOrient(dbOrientType::R90);
          if (dist_to_left == 0) {  // touching edge so draw on edge
            xfm.setOffset({block_bbox->xMin(), pin_center.y()});
          }
        } else if (arg_min == 1) {  // right
          xfm.setOrient(dbOrientType::R270);
          if (dist_to_right == 0) {  // touching edge so draw on edge
            xfm.setOffset({block_bbox->xMax(), pin_center.y()});
          }
        } else if (arg_min == 2) {  // top
          // none needed
          if (dist_to_top == 0) {  // touching edge so draw on edge
            xfm.setOffset({pin_center.x(), block_bbox->yMax()});
          }
        } else {  // bottom
          xfm.setOrient(dbOrientType::MX);
          if (dist_to_bot == 0) {  // touching edge so draw on edge
            xfm.setOffset({pin_center.x(), block_bbox->yMin()});
          }
        }

        odb::dbTechLayer* layer = box->getTechLayer();
        painter.setPen(layer);
        painter.setBrush(layer);

        // select marker
        const std::vector<Point>* template_points = &bi_marker;
        if (pin_dir == odb::dbIoType::INPUT) {
          template_points = &in_marker;
        } else if (pin_dir == odb::dbIoType::OUTPUT) {
          template_points = &out_marker;
        }

        // make new marker based on pin location
        std::vector<Point> marker;
        for (const auto& pt : *template_points) {
          Point new_pt = pt;
          xfm.apply(new_pt);
          marker.push_back(new_pt);
        }

        painter.drawPolygon(marker);

        if (draw_names) {
          Point text_anchor_pt = xfm.getOffset();

          auto text_anchor = Painter::BOTTOM_CENTER;
          if (arg_min == 0) {  // left
            text_anchor = Painter::RIGHT_CENTER;
            text_anchor_pt.setX(text_anchor_pt.x() - max_dim - text_margin);
          } else if (arg_min == 1) {  // right
            text_anchor = Painter::LEFT_CENTER;
            text_anchor_pt.setX(text_anchor_pt.x() + max_dim + text_margin);
          } else if (arg_min == 2) {  // top
            text_anchor = Painter::BOTTOM_CENTER;
            text_anchor_pt.setY(text_anchor_pt.y() + max_dim + text_margin);
          } else {  // bottom
            text_anchor = Painter::TOP_CENTER;
            text_anchor_pt.setY(text_anchor_pt.y() - max_dim - text_margin);
          }

          PinText pin_specs;
          pin_specs.layer = layer;
          pin_specs.text = term->getName();
          pin_specs.pt = text_anchor_pt;
          pin_specs.anchor = text_anchor;
          pin_specs.can_rotate = arg_min == 2 || arg_min == 3;
          // only need bounding box when rotation is possible
          if (pin_specs.can_rotate) {
            odb::Rect text_rect = painter.stringBoundaries(pin_specs.pt.x(),
                                                           pin_specs.pt.y(),
                                                           pin_specs.anchor,
                                                           pin_specs.text);
            text_rect.bloat(text_margin, text_rect);
            pin_specs.rect = Search::Box(
                Search::Point(text_rect.xMin(), text_rect.yMin()),
                Search::Point(text_rect.xMax(), text_rect.yMax()));
            pin_text_spec_shapes.insert(pin_specs.rect);
          } else {
            pin_specs.rect = Search::Box();
          }
          pin_text_spec.push_back(pin_specs);
        }
      }
    }
  }

  for (const auto& pin : pin_text_spec) {
    odb::dbTechLayer* layer = pin.layer;

    bool do_rotate = false;
    auto anchor = pin.anchor;
    if (pin.can_rotate) {
      if (pin_text_spec_shapes.qbegin(bgi::intersects(pin.rect)
                                      && bgi::satisfies([&](const auto& other) {
                                           return !bg::equals(other, pin.rect);
                                         }))
          != pin_text_spec_shapes.qend()) {
        // adjust anchor
        if (pin.anchor == Painter::BOTTOM_CENTER) {
          anchor = Painter::RIGHT_CENTER;
        } else if (pin.anchor == Painter::TOP_CENTER) {
          anchor = Painter::LEFT_CENTER;
        }
        do_rotate = true;
      }
    }

    painter.setPen(layer);
    painter.setBrush(layer);

    painter.drawString(pin.pt.x(), pin.pt.y(), anchor, pin.text, do_rotate);
  }

  qpainter->setFont(initial_font);
}

odb::Point LayoutViewer::screenToDBU(const QPointF& point)
{
  // Flip the y-coordinate (see file level comments)
  return Point((point.x() - centering_shift_.x()) / pixels_per_dbu_,
               (centering_shift_.y() - point.y()) / pixels_per_dbu_);
}

Rect LayoutViewer::screenToDBU(const QRectF& screen_rect)
{
  int dbu_left = (int) floor((screen_rect.left() - centering_shift_.x())
                             / pixels_per_dbu_);
  int dbu_right = (int) ceil((screen_rect.right() - centering_shift_.x())
                             / pixels_per_dbu_);
  // Flip the y-coordinate (see file level comments)
  int dbu_top = (int) floor((centering_shift_.y() - screen_rect.top())
                            / pixels_per_dbu_);
  int dbu_bottom = (int) ceil((centering_shift_.y() - screen_rect.bottom())
                              / pixels_per_dbu_);

  return Rect(dbu_left, dbu_bottom, dbu_right, dbu_top);
}

QPointF LayoutViewer::dbuToScreen(const Point& dbu_point)
{
  // Flip the y-coordinate (see file level comments)
  qreal x = centering_shift_.x() + dbu_point.x() * pixels_per_dbu_;
  qreal y = centering_shift_.y() - dbu_point.y() * pixels_per_dbu_;

  return QPointF(x, y);
}

QRectF LayoutViewer::dbuToScreen(const Rect& dbu_rect)
{
  // Flip the y-coordinate (see file level comments)
  qreal screen_left = centering_shift_.x() + dbu_rect.xMin() * pixels_per_dbu_;
  qreal screen_right = centering_shift_.x() + dbu_rect.xMax() * pixels_per_dbu_;
  qreal screen_top = centering_shift_.y() - dbu_rect.yMax() * pixels_per_dbu_;
  qreal screen_bottom
      = centering_shift_.y() - dbu_rect.yMin() * pixels_per_dbu_;

  return QRectF(QPointF(screen_left, screen_top),
                QPointF(screen_right, screen_bottom));
}

void LayoutViewer::updateBlockPainting(const QRect& area)
{
  if (block_drawing_ != nullptr && !repaint_requested_) {
    // no changes detected, so no need to update
    return;
  }

  auto now = std::chrono::system_clock::now();
  if (block_drawing_ != nullptr
      && std::chrono::duration_cast<std::chrono::milliseconds>(
             now - last_paint_time_)
                 .count()
             < repaint_interval_) {
    // re-emit update since a paint will be needed later
    emit update();
    return;
  }

  last_paint_time_ = now;
  repaint_requested_ = false;

  // build new drawing of layout
  auto* block_drawing = new QPixmap(area.width(), area.height());
  block_drawing->fill(Qt::transparent);

  QPainter block_painter(block_drawing);
  block_painter.setRenderHints(QPainter::Antialiasing);

  // apply transforms
  block_painter.translate(-area.topLeft());
  block_painter.translate(centering_shift_);
  // apply scaling
  block_painter.scale(pixels_per_dbu_, -pixels_per_dbu_);

  const Rect dbu_bounds = screenToDBU(area);

  // paint layout
  drawBlock(&block_painter, dbu_bounds, 0);

  // save the cached layout
  block_drawing_ = std::unique_ptr<QPixmap>(block_drawing);
}

void LayoutViewer::paintEvent(QPaintEvent* event)
{
  QPainter painter(this);
  painter.setRenderHints(QPainter::Antialiasing);

  // Fill draw region with black
  painter.setPen(QPen(background_, 0));
  painter.setBrush(background_);
  painter.drawRect(event->rect());

  if (!hasDesign()) {
    return;
  }

  // buffer outputs during paint to prevent recursive calls
  output_widget_->bufferOutputs(true);

  if (cut_maximum_size_.empty()) {
    generateCutLayerMaximumSizes();
  }

  // check if we can use the old image
  const QRect draw_bounds = event->rect();
  updateBlockPainting(draw_bounds);

  // draw cached block
  painter.drawPixmap(draw_bounds.topLeft(), *block_drawing_);

  painter.save();
  painter.translate(centering_shift_);
  painter.scale(pixels_per_dbu_, -pixels_per_dbu_);

  GuiPainter gui_painter(&painter,
                         options_,
                         screenToDBU(draw_bounds),
                         pixels_per_dbu_,
                         block_->getDbUnitsPerMicron());

  // draw selected and over top level and fast painting events
  drawSelected(gui_painter);
  // Always last so on top
  drawHighlighted(gui_painter);
  drawRulers(gui_painter);

  // draw partial ruler if present
  if (building_ruler_ && ruler_start_ != nullptr) {
    odb::Point snapped_mouse_pos
        = findNextRulerPoint(screenToDBU(mouse_move_pos_));
    gui_painter.drawRuler(ruler_start_->x(),
                          ruler_start_->y(),
                          snapped_mouse_pos.x(),
                          snapped_mouse_pos.y(),
                          showRulerAsEuclidian_());
  }

  // draw edge currently considered snapped to
  if (snap_edge_showing_) {
    painter.setPen(QPen(Qt::white, 0));
    painter.setBrush(QBrush());
    if (snap_edge_.first == snap_edge_.second) {
      painter.drawEllipse(QPointF(snap_edge_.first.x(), snap_edge_.first.y()),
                          5.0 / pixels_per_dbu_,
                          5.0 / pixels_per_dbu_);
    } else {
      painter.drawLine(
          QLine(QPoint(snap_edge_.first.x(), snap_edge_.first.y()),
                QPoint(snap_edge_.second.x(), snap_edge_.second.y())));
    }
  }

  painter.restore();

  drawScaleBar(&painter, draw_bounds);

  if (rubber_band_showing_) {
    painter.setPen(QPen(Qt::white, 0));
    painter.setBrush(QBrush());
    painter.drawRect(rubber_band_.normalized());
  }

  painter.end();
  // painting is done, okay to update outputs again
  output_widget_->bufferOutputs(false);
}

void LayoutViewer::fullRepaint()
{
  repaint_requested_ = true;
  update();
}

void LayoutViewer::drawScaleBar(QPainter* painter, const QRect& rect)
{
  if (!options_->isScaleBarVisible()) {
    return;
  }

  const qreal pixels_per_mircon
      = pixels_per_dbu_ * block_->getDbUnitsPerMicron();
  const qreal window_width = rect.width() / pixels_per_mircon;
  const qreal target_width = 0.1 * window_width;

  const qreal peg_width
      = std::pow(10.0, std::floor(std::log10(target_width)));  // microns
  int peg_incr = std::round(target_width / peg_width);
  const qreal bar_size = peg_incr * peg_width;
  if (peg_incr == 1) {
    // make 10 segments if 1
    peg_incr = 10;
  }

  const int bar_height = 10;  // px

  int bar_width = bar_size * pixels_per_mircon;

  double scale_unit;
  QString unit_text;
  if (usingDBU_()) {
    scale_unit = block_->getDbUnitsPerMicron();
    unit_text = "";
  } else {
    if (bar_size > 1000) {
      scale_unit = 0.001;
      unit_text = "mm";
    } else if (bar_size > 1) {
      scale_unit = 1;
      unit_text = "\u03bcm";  // um
    } else if (bar_size > 0.001) {
      scale_unit = 1000;
      unit_text = "nm";
    } else {
      scale_unit = 1e6;
      unit_text = "pm";
    }
  }

  auto color = Qt::white;
  painter->setPen(QPen(color, 2));
  painter->setBrush(Qt::transparent);

  const QRectF scale_bar_outline(
      rect.left() + 10, rect.bottom() - 20, bar_width, bar_height);

  // draw base half bar shape |_|
  painter->drawLine(scale_bar_outline.topLeft(),
                    scale_bar_outline.bottomLeft());
  painter->drawLine(scale_bar_outline.bottomLeft(),
                    scale_bar_outline.bottomRight());
  painter->drawLine(scale_bar_outline.bottomRight(),
                    scale_bar_outline.topRight());

  const QFontMetrics font_metric = painter->fontMetrics();

  const int text_px_offset = 2;
  const int text_keep_out
      = font_metric.averageCharWidth();  // ensure text has approx one
                                         // characters spacing

  // draw total size over right size
  const QString bar_text_end
      = QString::number(static_cast<int>(bar_size * scale_unit)) + unit_text;
  const QRect end_box = font_metric.boundingRect(bar_text_end);
  painter->drawText(scale_bar_outline.topRight()
                        - QPointF(end_box.center().x(), text_px_offset),
                    bar_text_end);
  const qreal end_offset
      = scale_bar_outline.right() - 0.5 * end_box.width() - text_keep_out;

  // draw "0" over left side
  const QRect zero_box = font_metric.boundingRect("0");
  // dont draw if the 0 is too close or overlapping with the right side text
  if (scale_bar_outline.left() + zero_box.center().x() < end_offset) {
    painter->drawText(scale_bar_outline.topLeft()
                          - QPointF(zero_box.center().x(), text_px_offset),
                      "0");
  }

  // margin around 0 to avoid drawing first available increment
  const qreal zero_offset
      = scale_bar_outline.left() + 0.5 * zero_box.width() + text_keep_out;
  const qreal segment_width = static_cast<double>(bar_width) / peg_incr;
  const double peg_increment = bar_size / peg_incr;
  bool middle_shown
      = false;  // flag to indicate the middle tick marker has been drawn

  if (segment_width > 4) {
    // draw pegs, don't draw if they are basically overlapping
    for (int i = 1; i < peg_incr; i++) {
      QPointF p1(scale_bar_outline.left() + i * segment_width,
                 scale_bar_outline.bottom());
      QPointF p2 = p1 - QPointF(0, bar_height / 2);

      if (!middle_shown) {
        // only one peg increment
        QString peg_text
            = QString::number(static_cast<int>(i * peg_increment * scale_unit));
        QRect peg_text_box = font_metric.boundingRect(peg_text);

        // check if text will fit next to "0"
        if (p1.x() - 0.5 * peg_text_box.width() > zero_offset) {
          middle_shown = true;

          // check to make sure there is room at the end
          if (p1.x() + 0.5 * peg_text_box.width() < end_offset) {
            p2 = p1
                 - QPointF(
                     0, 3 * bar_height / 4.0);  // make this peg a little taller
            painter->drawText(QPointF(p2.x() - peg_text_box.center().x(),
                                      scale_bar_outline.top() - text_px_offset),
                              peg_text);
          }
        }
      }

      painter->drawLine(p1, p2);
    }
  }
}

void LayoutViewer::fit()
{
  if (!hasDesign()) {
    return;
  }

  zoomTo(getBounds());
  // ensure we save a correct value for fit_pixels_per_dbu_
  viewportUpdated();
}

void LayoutViewer::selectHighlightConnectedInst(bool select_flag)
{
  int highlight_group = 0;
  if (!select_flag) {
    HighlightGroupDialog dlg;
    dlg.exec();
    highlight_group = dlg.getSelectedHighlightGroup();
  }
  Gui::get()->selectHighlightConnectedInsts(select_flag, highlight_group);
}

void LayoutViewer::selectHighlightConnectedNets(bool select_flag,
                                                bool output,
                                                bool input)
{
  int highlight_group = 0;
  if (!select_flag) {
    HighlightGroupDialog dlg;
    dlg.exec();
    highlight_group = dlg.getSelectedHighlightGroup();
  }
  Gui::get()->selectHighlightConnectedNets(
      select_flag, output, input, highlight_group);
}

void LayoutViewer::updateContextMenuItems()
{
  if (Gui::get()->anyObjectInSet(true /*selection set*/, odb::dbInstObj)
      == false)  // No Instance in selected set
  {
    menu_actions_[SELECT_OUTPUT_NETS_ACT]->setDisabled(true);
    menu_actions_[SELECT_INPUT_NETS_ACT]->setDisabled(true);
    menu_actions_[SELECT_ALL_NETS_ACT]->setDisabled(true);

    menu_actions_[HIGHLIGHT_OUTPUT_NETS_ACT]->setDisabled(true);
    menu_actions_[HIGHLIGHT_INPUT_NETS_ACT]->setDisabled(true);
    menu_actions_[HIGHLIGHT_ALL_NETS_ACT]->setDisabled(true);
  } else {
    menu_actions_[SELECT_OUTPUT_NETS_ACT]->setDisabled(false);
    menu_actions_[SELECT_INPUT_NETS_ACT]->setDisabled(false);
    menu_actions_[SELECT_ALL_NETS_ACT]->setDisabled(false);

    menu_actions_[HIGHLIGHT_OUTPUT_NETS_ACT]->setDisabled(false);
    menu_actions_[HIGHLIGHT_INPUT_NETS_ACT]->setDisabled(false);
    menu_actions_[HIGHLIGHT_ALL_NETS_ACT]->setDisabled(false);
  }

  if (Gui::get()->anyObjectInSet(true, odb::dbNetObj)
      == false) {  // No Net in selected set
    menu_actions_[SELECT_CONNECTED_INST_ACT]->setDisabled(true);
    menu_actions_[HIGHLIGHT_CONNECTED_INST_ACT]->setDisabled(true);
  } else {
    menu_actions_[SELECT_CONNECTED_INST_ACT]->setDisabled(false);
    menu_actions_[HIGHLIGHT_CONNECTED_INST_ACT]->setDisabled(false);
  }
}

void LayoutViewer::showLayoutCustomMenu(QPoint pos)
{
  updateContextMenuItems();
  layout_context_menu_->popup(this->mapToGlobal(pos));
}

void LayoutViewer::designLoaded(dbBlock* block)
{
  search_.setBlock(block);
}

void LayoutViewer::setScroller(LayoutScroll* scroller)
{
  scroller_ = scroller;

  // ensure changes in the scroll area are announced to the layout viewer
  connect(scroller_, SIGNAL(viewportChanged()), this, SLOT(viewportUpdated()));
  connect(scroller_,
          SIGNAL(centerChanged(int, int)),
          this,
          SLOT(updateCenter(int, int)));
  connect(
      scroller_, SIGNAL(centerChanged(int, int)), this, SLOT(fullRepaint()));
}

void LayoutViewer::viewportUpdated()
{
  if (!hasDesign()) {
    resize(scroller_->maximumViewportSize());
    return;
  }

  bool zoomed_in = fit_pixels_per_dbu_ < pixels_per_dbu_;

  // determine new fit_pixels_per_dbu_ based on current viewport size
  fit_pixels_per_dbu_ = computePixelsPerDBU(scroller_->maximumViewportSize(),
                                            getPaddedRect(getBounds()));

  // when zoomed in don't update size,
  // else update size of window
  if (!zoomed_in) {
    resize(scroller_->maximumViewportSize());
  }
}

void LayoutViewer::saveImage(const QString& filepath,
                             const Rect& region,
                             double dbu_per_pixel)
{
  if (!hasDesign()) {
    return;
  }

  QString save_filepath = filepath;
  if (filepath.isEmpty()) {
    save_filepath = Utils::requestImageSavePath(this, "Save layout");
  }

  if (save_filepath.isEmpty()) {
    return;
  }

  save_filepath = Utils::fixImagePath(save_filepath, logger_);

  Rect save_area = region;
  if (region.dx() == 0 || region.dy() == 0) {
    // default to just that is currently visible
    save_area = screenToDBU(visibleRegion().boundingRect());
  }

  const qreal old_pixels_per_dbu = pixels_per_dbu_;
  if (dbu_per_pixel != 0) {
    pixels_per_dbu_ = 1.0 / dbu_per_pixel;
  }

  // convert back to pixels based on new resolution
  const QRectF screen_region = dbuToScreen(save_area);
  const QRegion save_region = QRegion(screen_region.left(),
                                      screen_region.top(),
                                      screen_region.width(),
                                      screen_region.height());

  const QRect bounding_rect = save_region.boundingRect();
  // need to remove cache to ensure image is correct
  std::unique_ptr<QPixmap> saved_cache = std::move(block_drawing_);
  const auto last_paint_time = last_paint_time_;
  block_drawing_ = nullptr;

  Utils::renderImage(save_filepath,
                     this,
                     bounding_rect.width(),
                     bounding_rect.height(),
                     bounding_rect,
                     background_,
                     logger_);

  // restore cache
  block_drawing_ = std::move(saved_cache);
  last_paint_time_ = last_paint_time;

  pixels_per_dbu_ = old_pixels_per_dbu;
}

void LayoutViewer::addMenuAndActions()
{
  // Create Top Level Menu for the context Menu
  auto select_menu = layout_context_menu_->addMenu(tr("Select"));
  auto highlight_menu = layout_context_menu_->addMenu(tr("Highlight"));
  auto view_menu = layout_context_menu_->addMenu(tr("View"));
  auto save_menu = layout_context_menu_->addMenu(tr("Save"));
  auto clear_menu = layout_context_menu_->addMenu(tr("Clear"));
  // Create Actions

  // Select Actions
  menu_actions_[SELECT_CONNECTED_INST_ACT]
      = select_menu->addAction(tr("Connected Insts"));
  menu_actions_[SELECT_OUTPUT_NETS_ACT]
      = select_menu->addAction(tr("Output Nets"));
  menu_actions_[SELECT_INPUT_NETS_ACT]
      = select_menu->addAction(tr("Input Nets"));
  menu_actions_[SELECT_ALL_NETS_ACT] = select_menu->addAction(tr("All Nets"));

  // Highlight Actions
  menu_actions_[HIGHLIGHT_CONNECTED_INST_ACT]
      = highlight_menu->addAction(tr("Connected Insts"));
  menu_actions_[HIGHLIGHT_OUTPUT_NETS_ACT]
      = highlight_menu->addAction(tr("Output Nets"));
  menu_actions_[HIGHLIGHT_INPUT_NETS_ACT]
      = highlight_menu->addAction(tr("Input Nets"));
  menu_actions_[HIGHLIGHT_ALL_NETS_ACT]
      = highlight_menu->addAction(tr("All Nets"));

  // View Actions
  menu_actions_[VIEW_ZOOMIN_ACT] = view_menu->addAction(tr("Zoom In"));
  menu_actions_[VIEW_ZOOMOUT_ACT] = view_menu->addAction(tr("Zoom Out"));
  menu_actions_[VIEW_ZOOMFIT_ACT] = view_menu->addAction(tr("Fit"));

  // Save actions
  menu_actions_[SAVE_VISIBLE_IMAGE_ACT]
      = save_menu->addAction(tr("Visible layout"));
  menu_actions_[SAVE_WHOLE_IMAGE_ACT]
      = save_menu->addAction(tr("Entire layout"));

  // Clear Actions
  menu_actions_[CLEAR_SELECTIONS_ACT] = clear_menu->addAction(tr("Selections"));
  menu_actions_[CLEAR_HIGHLIGHTS_ACT] = clear_menu->addAction(tr("Highlights"));
  menu_actions_[CLEAR_RULERS_ACT] = clear_menu->addAction(tr("Rulers"));
  menu_actions_[CLEAR_FOCUS_ACT] = clear_menu->addAction(tr("Focus nets"));
  menu_actions_[CLEAR_GUIDES_ACT] = clear_menu->addAction(tr("Route Guides"));
  menu_actions_[CLEAR_ALL_ACT] = clear_menu->addAction(tr("All"));

  // Connect Slots to Actions...
  connect(menu_actions_[SELECT_CONNECTED_INST_ACT],
          &QAction::triggered,
          this,
          [this]() { selectHighlightConnectedInst(true); });
  connect(menu_actions_[SELECT_OUTPUT_NETS_ACT],
          &QAction::triggered,
          this,
          [this]() { selectHighlightConnectedNets(true, true, false); });
  connect(menu_actions_[SELECT_INPUT_NETS_ACT],
          &QAction::triggered,
          this,
          [this]() { selectHighlightConnectedNets(true, false, true); });
  connect(
      menu_actions_[SELECT_ALL_NETS_ACT], &QAction::triggered, this, [this]() {
        selectHighlightConnectedNets(true, true, true);
      });

  connect(menu_actions_[HIGHLIGHT_CONNECTED_INST_ACT],
          &QAction::triggered,
          this,
          [this]() { selectHighlightConnectedInst(false); });
  connect(menu_actions_[HIGHLIGHT_OUTPUT_NETS_ACT],
          &QAction::triggered,
          this,
          [this]() { selectHighlightConnectedNets(false, true, false); });
  connect(menu_actions_[HIGHLIGHT_INPUT_NETS_ACT],
          &QAction::triggered,
          this,
          [this]() { this->selectHighlightConnectedNets(false, false, true); });
  connect(menu_actions_[HIGHLIGHT_ALL_NETS_ACT],
          &QAction::triggered,
          this,
          [this]() { selectHighlightConnectedNets(false, true, true); });

  connect(menu_actions_[VIEW_ZOOMIN_ACT], &QAction::triggered, this, [this]() {
    zoomIn();
  });
  connect(menu_actions_[VIEW_ZOOMOUT_ACT], &QAction::triggered, this, [this]() {
    zoomOut();
  });
  connect(menu_actions_[VIEW_ZOOMFIT_ACT], &QAction::triggered, this, [this]() {
    fit();
  });

  connect(menu_actions_[SAVE_VISIBLE_IMAGE_ACT],
          &QAction::triggered,
          this,
          [this]() { saveImage(""); });
  connect(
      menu_actions_[SAVE_WHOLE_IMAGE_ACT], &QAction::triggered, this, [this]() {
        const QSize whole_size = size();
        saveImage(
            "",
            screenToDBU(QRectF(0, 0, whole_size.width(), whole_size.height())));
      });

  connect(menu_actions_[CLEAR_SELECTIONS_ACT], &QAction::triggered, this, []() {
    Gui::get()->clearSelections();
  });
  connect(menu_actions_[CLEAR_HIGHLIGHTS_ACT], &QAction::triggered, this, []() {
    Gui::get()->clearHighlights(-1);
  });
  connect(menu_actions_[CLEAR_RULERS_ACT], &QAction::triggered, this, []() {
    Gui::get()->clearRulers();
  });
  connect(menu_actions_[CLEAR_FOCUS_ACT], &QAction::triggered, this, []() {
    Gui::get()->clearFocusNets();
  });
  connect(menu_actions_[CLEAR_GUIDES_ACT], &QAction::triggered, this, []() {
    Gui::get()->clearRouteGuides();
  });
  connect(menu_actions_[CLEAR_ALL_ACT], &QAction::triggered, this, [this]() {
    menu_actions_[CLEAR_SELECTIONS_ACT]->trigger();
    menu_actions_[CLEAR_HIGHLIGHTS_ACT]->trigger();
    menu_actions_[CLEAR_RULERS_ACT]->trigger();
    menu_actions_[CLEAR_FOCUS_ACT]->trigger();
    menu_actions_[CLEAR_GUIDES_ACT]->trigger();
  });
}

void LayoutViewer::restoreTclCommands(std::vector<std::string>& cmds)
{
  cmds.push_back(fmt::format("gui::set_resolution {}", 1.0 / pixels_per_dbu_));

  if (block_ != nullptr) {
    const double dbu_per_micron = block_->getDbUnitsPerMicron();

    cmds.push_back(fmt::format("gui::center_at {} {}",
                               center_.x() / dbu_per_micron,
                               center_.y() / dbu_per_micron));
  }
}

bool LayoutViewer::hasDesign() const
{
  if (block_ == nullptr) {
    return false;
  }

  const Rect bounds = getBounds();
  if (bounds.dx() == 0 || bounds.dy() == 0) {
    return false;
  }

  return true;
}

void LayoutViewer::addFocusNet(odb::dbNet* net)
{
  const auto& [itr, inserted] = focus_nets_.insert(net);
  if (inserted) {
    emit focusNetsChanged();
    fullRepaint();
  }
}

void LayoutViewer::addRouteGuides(odb::dbNet* net)
{
  const auto& [itr, inserted] = route_guides_.insert(net);
  if (inserted) {
    fullRepaint();
  }
}

void LayoutViewer::removeFocusNet(odb::dbNet* net)
{
  if (focus_nets_.erase(net) > 0) {
    emit focusNetsChanged();
    fullRepaint();
  }
}

void LayoutViewer::removeRouteGuides(odb::dbNet* net)
{
  if (route_guides_.erase(net) > 0) {
    fullRepaint();
  }
}

void LayoutViewer::clearFocusNets()
{
  if (!focus_nets_.empty()) {
    focus_nets_.clear();
    emit focusNetsChanged();
    fullRepaint();
  }
}

void LayoutViewer::clearRouteGuides()
{
  if (!route_guides_.empty()) {
    route_guides_.clear();
    fullRepaint();
  }
}

bool LayoutViewer::isNetVisible(odb::dbNet* net)
{
  bool focus_visible = true;
  if (!focus_nets_.empty()) {
    focus_visible = focus_nets_.find(net) != focus_nets_.end();
  }

  return focus_visible && options_->isNetVisible(net);
}

void LayoutViewer::setResetRepaintInterval()
{
  repaint_interval_ = 0;  // no delay in repaints
}

void LayoutViewer::setLongRepaintInterval()
{
  repaint_interval_ = 1000;  // wait one second before repainting layout
}

////// LayoutScroll ///////
LayoutScroll::LayoutScroll(LayoutViewer* viewer, QWidget* parent)
    : QScrollArea(parent), viewer_(viewer)
{
  setWidgetResizable(false);
  setWidget(viewer);
  viewer->setScroller(this);
}

void LayoutScroll::resizeEvent(QResizeEvent* event)
{
  QScrollArea::resizeEvent(event);
  // announce that the viewport has changed
  emit viewportChanged();
}

void LayoutScroll::scrollContentsBy(int dx, int dy)
{
  QScrollArea::scrollContentsBy(dx, dy);
  // announce the amount the viewport has changed by
  emit centerChanged(dx, dy);
  // make sure the whole visible layout is updated, not just the newly visible
  // part
  widget()->update();
}

// Handles zoom in/out on ctrl-wheel
void LayoutScroll::wheelEvent(QWheelEvent* event)
{
  if (!event->modifiers().testFlag(Qt::ControlModifier)) {
    QScrollArea::wheelEvent(event);
    return;
  }

  const odb::Point mouse_pos
      = viewer_->screenToDBU(viewer_->mapFromGlobal(QCursor::pos()));
  if (event->angleDelta().y() > 0) {
    viewer_->zoomIn(mouse_pos, true);
  } else {
    viewer_->zoomOut(mouse_pos, true);
  }
  // ensure changes are processed before the next wheel event to prevent zoomIn
  // and Out from jumping around on the ScrollBars
  QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
}

void LayoutViewer::generateCutLayerMaximumSizes()
{
  if (!hasDesign()) {
    return;
  }

  dbTech* tech = block_->getDataBase()->getTech();
  if (tech == nullptr) {
    return;
  }

  for (auto layer : tech->getLayers()) {
    if (layer->getType() == dbTechLayerType::CUT) {
      int width = layer->getWidth();
      if (width == 0) {
        // width is not set, so looking through all vias to find max size
        for (auto via : tech->getVias()) {
          for (auto box : via->getBoxes()) {
            if (box->getTechLayer() == layer) {
              width = std::max(
                  width,
                  static_cast<int>(std::max(box->getDX(), box->getDY())));
            }
          }
        }
      }
      if (width == 0) {
        // no vias, look through all pins and obs to find max size.
        // This can happen for contacts in stdcell pins which are still
        // important for diff layer cut spacing checks.
        std::vector<dbMaster*> masters;
        block_->getMasters(masters);
        for (dbMaster* master : masters) {
          for (dbMTerm* term : master->getMTerms()) {
            for (dbMPin* pin : term->getMPins()) {
              for (dbBox* box : pin->getGeometry()) {
                if (box->getTechLayer() == layer) {
                  width = std::max(
                      width,
                      static_cast<int>(std::max(box->getDX(), box->getDY())));
                }
              }
            }
          }

          for (dbBox* box : master->getObstructions()) {
            if (box->getTechLayer() == layer) {
              width = std::max(
                  width,
                  static_cast<int>(std::max(box->getDX(), box->getDY())));
            }
          }
        }
      }
      cut_maximum_size_[layer] = width;
    }
  }
}

inline int LayoutViewer::instanceSizeLimit()
{
  if (options_->isDetailedVisibility()) {
    return 0;
  }

  return fineViewableResolution();
}

inline int LayoutViewer::shapeSizeLimit()
{
  if (options_->isDetailedVisibility()) {
    return fineViewableResolution();
  }

  return nominalViewableResolution();
}

inline int LayoutViewer::fineViewableResolution()
{
  return 1.0 / pixels_per_dbu_;
}

inline int LayoutViewer::nominalViewableResolution()
{
  return 5.0 / pixels_per_dbu_;
}

inline int LayoutViewer::coarseViewableResolution()
{
  return 10.0 / pixels_per_dbu_;
}

}  // namespace gui
