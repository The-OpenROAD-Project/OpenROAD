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
#include <QDebug>
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
#include <iostream>
#include <tuple>
#include <vector>

#include "db.h"
#include "dbTransform.h"
#include "gui/gui.h"
#include "highlightGroupDialog.h"
#include "mainWindow.h"
#include "search.h"
#include "utl/Logger.h"

#include "ruler.h"

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

static Rect getBounds(dbBlock* block)
{
  Rect bbox;
  block->getBBox()->getBox(bbox);

  Rect die;
  block->getDieArea(die);

  bbox.merge(die);

  return bbox;
}

// This class wraps the QPainter in the abstract Painter API for
// Renderer instances to use.
class GuiPainter : public Painter
{
 public:
  GuiPainter(QPainter* painter,
             Options* options,
             qreal pixels_per_dbu,
             int dbu_per_micron)
      : Painter(options, pixels_per_dbu),
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

  void setBrush(const Color& color) override
  {
    painter_->setBrush(QColor(color.r, color.g, color.b, color.a));
  }
  void drawGeomShape(const odb::GeomShape* shape) override
  {
    std::vector<Point> points = shape->getPoints();
    const int size = points.size();
    if (size == 5) {
      painter_->drawRect(QRect(QPoint(shape->xMin(), shape->yMin()),
                               QPoint(shape->xMax(), shape->yMax())));
    } else {
      drawPolygon(points);
    }
  }
  void drawRect(const odb::Rect& rect, int roundX = 0, int roundY = 0) override
  {
    if (roundX > 0 || roundY > 0)
      painter_->drawRoundRect(QRect(QPoint(rect.xMin(), rect.yMin()),
                                    QPoint(rect.xMax(), rect.yMax())),
                              roundX,
                              roundY);
    else
      painter_->drawRect(QRect(QPoint(rect.xMin(), rect.yMin()),
                               QPoint(rect.xMax(), rect.yMax())));
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

  void setTransparentBrush() override { painter_->setBrush(Qt::transparent); }
  void setHashedBrush(const Color& color) override
  {
    painter_->setBrush(QBrush(QColor(color.r, color.g, color.b, color.a), Qt::DiagCrossPattern));
  }

  void drawCircle(int x, int y, int r) override
  {
    painter_->drawEllipse(QPoint(x, y), r, r);
  }

  // NOTE: The constant height text s drawn with this function, hence
  //       the trasnsformation is mapped to the base transformation and
  //       the world co-ordinates are mapped to the window co-ordinates
  //       before drawing.
  void drawString(int x, int y, Anchor anchor, const std::string& s) override
  {
    const QString text = QString::fromStdString(s);
    const QRect text_bbox = painter_->fontMetrics().boundingRect(text);
    const qreal scale_adjust = 1.0 / getPixelsPerDBU();
    int sx = 0;
    int sy = 0;
    if (anchor == BOTTOM_LEFT) {
      // default for Qt
    } else if (anchor == BOTTOM_RIGHT) {
      sx -= text_bbox.right();
    } else if (anchor == TOP_LEFT) {
      sy -= text_bbox.top();
    } else if (anchor == TOP_RIGHT) {
      sx -= text_bbox.right();
      sy -= text_bbox.top();
    } else if (anchor == CENTER) {
      sx -= text_bbox.width() / 2;
      sy += text_bbox.height() / 2;
    } else if (anchor == BOTTOM_CENTER) {
      sx -= text_bbox.width() / 2;
    } else if (anchor == TOP_CENTER) {
      sx -= text_bbox.width() / 2;
      sy -= text_bbox.top();
    } else if (anchor == LEFT_CENTER) {
      sy += text_bbox.height() / 2;
    } else {
      // RIGHT_CENTER
      sx -= text_bbox.right();
      sy += text_bbox.height() / 2;
    }
    // current units of sx, sy are pixels, so convert to DBU
    sx *= scale_adjust;
    sy *= scale_adjust;
    // add desired text location in DBU
    sx += x;
    sy += y;
    const QTransform transform = painter_->transform();
    painter_->translate(sx, sy);
    painter_->scale(scale_adjust, -scale_adjust); // undo original scaling
    painter_->drawText(0, 0, text); // origin of painter is desired location, so paint at 0, 0
    painter_->setTransform(transform);
  }

  void drawRuler(int x0, int y0, int x1, int y1, const std::string& label = "") override
  {
    const QColor ruler_color_qt = getOptions()->rulerColor();
    const Color ruler_color(ruler_color_qt.red(), ruler_color_qt.green(), ruler_color_qt.blue(), ruler_color_qt.alpha());
    const QFont ruler_font = getOptions()->rulerFont();
    const QFont restore_font = painter_->font();

    setPen(ruler_color, true);
    setBrush(ruler_color);

    std::stringstream ss;
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
      ruler_angle = 57.295779 * std::atan(std::abs(y_len / x_len)); // 180 / pi
      if (x_len < 0) { // adjust for negative dx
        ruler_angle = 180 - ruler_angle;
      }
      if (y_len < 0) { // adjust for negative dy
       ruler_angle = -ruler_angle;
      }
    }
    painter_->rotate(ruler_angle);

    const int precision = std::ceil(std::log10(dbu_per_micron_));
    const qreal len_microns = len / (qreal) dbu_per_micron_;

    ss << std::fixed << std::setprecision(precision) << len_microns;

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
    qreal major_tick_mark_interval = std::pow(10.0, std::floor(std::log10(len_microns)));
    const int major_ticks = std::floor(len_microns / major_tick_mark_interval);
    qreal minor_tick_mark_interval = major_tick_mark_interval / 10;
    const int min_tick_spacing = 10; // pixels
    const bool do_minor_ticks = minor_tick_mark_interval * dbu_per_micron_ * getPixelsPerDBU() > min_tick_spacing;

    // draw tick marks
    const int minor_tick_size = endcap_size / 2;
    const int major_tick_interval = major_tick_mark_interval * dbu_per_micron_;
    const int minor_tick_interval = minor_tick_mark_interval * dbu_per_micron_;
    //major ticks
    if (major_tick_interval * getPixelsPerDBU() >= min_tick_spacing) { // only draw tick marks if they are spaces apart
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
    const int x = len / 2;
    if (!label.empty()) {
      // label on next to length
      drawString(0, 0, BOTTOM_CENTER, label + ": " + ss.str());
    } else {
      drawString(0, 0, BOTTOM_CENTER, ss.str());
    }
    painter_->setFont(restore_font);

    painter_->setTransform(initial_xfm);
  }

 private:
  QPainter* painter_;
  int dbu_per_micron_;
};

LayoutViewer::LayoutViewer(
    Options* options,
    const SelectionSet& selected,
    const HighlightSet& highlighted,
    const std::vector<std::unique_ptr<Ruler>>& rulers,
    std::function<Selected(const std::any&)> makeSelected,
    QWidget* parent)
    : QWidget(parent),
      db_(nullptr),
      options_(options),
      selected_(selected),
      highlighted_(highlighted),
      rulers_(rulers),
      scroller_(nullptr),
      pixels_per_dbu_(1.0),
      fit_pixels_per_dbu_(1.0),
      min_depth_(0),
      max_depth_(99),
      search_init_(false),
      rubber_band_showing_(false),
      makeSelected_(makeSelected),
      building_ruler_(false),
      ruler_start_(nullptr),
      snap_edge_showing_(false),
      snap_edge_(),
      block_drawing_(nullptr),
      logger_(nullptr),
      design_loaded_(false),
      layout_context_menu_(new QMenu(tr("Layout Menu"), this))
{
  setMouseTracking(true);
  resize(100, 100);  // just a placeholder until we load the design

  addMenuAndActions();
}

void LayoutViewer::setDb(dbDatabase* db)
{
  if (db_ != db) {
    update();
  }
  db_ = db;
}

void LayoutViewer::setLogger(utl::Logger* logger)
{
  logger_ = logger;
}

dbBlock* LayoutViewer::getBlock()
{
  if (!db_) {
    return nullptr;
  }

  dbChip* chip = db_->getChip();
  if (!chip) {
    return nullptr;
  }

  dbBlock* block = chip->getBlock();
  return block;
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

Rect LayoutViewer::getPaddedRect(const Rect& rect, double factor)
{
  const int margin = factor * std::max(rect.dx(), rect.dy());
  return Rect(
      rect.xMin() - margin,
      rect.yMin() - margin,
      rect.xMax() + margin,
      rect.yMax() + margin
      );
}

qreal LayoutViewer::computePixelsPerDBU(const QSize& size, const Rect& dbu_rect)
{
  return std::min(
      size.width()  / (double) dbu_rect.dx(),
      size.height() / (double) dbu_rect.dy());
}

void LayoutViewer::setPixelsPerDBU(qreal pixels_per_dbu)
{
  dbBlock* block = getBlock();
  if (!block) {
    return;
  }

  const Rect fitted_bb = getPaddedRect(getBounds(block));
  // ensure max size is not exceeded
  qreal maximum_pixels_per_dbu_ = 0.98*computePixelsPerDBU(QSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX), fitted_bb);
  qreal target_pixels_per_dbu = std::min(pixels_per_dbu, maximum_pixels_per_dbu_);

  const QSize new_size(
      ceil(fitted_bb.dx() * target_pixels_per_dbu),
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
  const QPointF shift_window(
      0.5 * scroller_->horizontalScrollBar()->pageStep(),
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

void LayoutViewer::zoom(const odb::Point& focus, qreal factor, bool do_delta_focus)
{
  qreal old_pixels_per_dbu = pixels_per_dbu_;

  // focus to center, this is only used if doing delta_focus
  // this holds the distance (x and y) from the desired focus point and the current center
  // so the new center can be computed and ensure that the new center is in line with the old center.
  odb::Point center_delta(focus.x() - center_.x(), focus.y() - center_.y());

  // update resolution
  setPixelsPerDBU(pixels_per_dbu_ * factor);

  odb::Point new_center = focus;
  if (do_delta_focus) {
    qreal actual_factor = pixels_per_dbu_ / old_pixels_per_dbu;
    // new center based on focus
    // adjust such that the new center follows the mouse
    new_center = odb::Point(
        focus.x() - center_delta.x() / actual_factor,
        focus.y() - center_delta.y() / actual_factor);
  }

  centerAt(new_center);
}

void LayoutViewer::zoomTo(const Rect& rect_dbu)
{
  const Rect padded_rect = getPaddedRect(rect_dbu);

  // set resolution required to view the whole padded rect
  setPixelsPerDBU(computePixelsPerDBU(scroller_->maximumViewportSize(), padded_rect));

  // center the layout at the middle of the rect
  centerAt(Point(rect_dbu.xMin() + rect_dbu.dx()/2, rect_dbu.yMin() + rect_dbu.dy()/2));
}

std::pair<LayoutViewer::Edges, bool> LayoutViewer::searchNearestEdge(const std::vector<Search::Box>& boxes, const odb::Point& pt)
{
  // find closest edges for each object returned
  std::vector<Edges> candidates;
  for (auto& box : boxes) {
    auto pt0 = box.min_corner();
    auto pt1 = box.max_corner();

    Edge vertical;
    // vertical edges
    if (std::abs(pt.x() - pt0.get<0>()) < std::abs(pt.x() - pt1.get<0>())) {
      // closer to pt0
      vertical.first = odb::Point(pt0.get<0>(), pt0.get<1>());
      vertical.second = odb::Point(pt0.get<0>(), pt1.get<1>());
    } else {
      // closer to pt1
      vertical.first = odb::Point(pt1.get<0>(), pt0.get<1>());
      vertical.second = odb::Point(pt1.get<0>(), pt1.get<1>());
    }

    Edge horizontal;
    // horizontal edges
    if (std::abs(pt.y() - pt0.get<1>()) < std::abs(pt.y() - pt1.get<1>())) {
      // closer to pt0
      horizontal.first = odb::Point(pt0.get<0>(), pt0.get<1>());
      horizontal.second = odb::Point(pt1.get<0>(), pt0.get<1>());
    } else {
      // closer to pt1
      horizontal.first = odb::Point(pt0.get<0>(), pt1.get<1>());
      horizontal.second = odb::Point(pt1.get<0>(), pt1.get<1>());
    }

    candidates.push_back({horizontal, vertical});
  }

  if (candidates.empty()) {
    return {Edges(), false};
  }

  // find closest edge overall, start with last one
  Edges closest_edges = candidates.back();
  candidates.pop_back();
  for (auto& edge_set : candidates) {
    // vertical edges
    if (std::abs(pt.x() - edge_set.vertical.first.x()) < std::abs(pt.x() - closest_edges.vertical.first.x())) {
      closest_edges.vertical = edge_set.vertical;
    }

    // horizontal edges
    if (std::abs(pt.y() - edge_set.horizontal.first.y()) < std::abs(pt.y() - closest_edges.horizontal.first.y())) {
      closest_edges.horizontal = edge_set.horizontal;
    }
  }

  return {closest_edges, true};
}

std::pair<LayoutViewer::Edge, bool> LayoutViewer::findEdge(const odb::Point& pt, bool horizontal)
{
  odb::dbBlock* block = getBlock();
  if (db_ == nullptr || block == nullptr) {
    return {Edge(), false};
  }

  const int search_radius = block->getDbUnitsPerMicron();

  std::vector<Search::Box> boxes;

  odb::Rect search_line;
  if (horizontal) {
    search_line = odb::Rect(pt.x(), pt.y() - search_radius, pt.x(), pt.y() + search_radius);
  } else {
    search_line = odb::Rect(pt.x() - search_radius, pt.y(), pt.x() + search_radius, pt.y());
  }

  auto inst_range = search_.searchInsts(search_line.xMin(), search_line.yMin(), search_line.xMax(), search_line.yMax());

  // Cache the search results as we will iterate over the instances
  // for each layer.
  std::vector<dbInst*> insts;
  for (auto& [box, poly, inst] : inst_range) {
    if (options_->isInstanceVisible(inst)) {
      insts.push_back(inst);
      boxes.push_back(box);
    }
  }

  // look for edges in metal shapes
  dbTech* tech = db_->getTech();
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

      if (options_->areObstructionsVisible()) {
        for (auto& box : inst_boxes->obs) {
          odb::Rect trans_box(box.left(), box.bottom(), box.right(), box.top());
          inst_xfm.apply(trans_box);
          if (trans_box.intersects(search_line)) {
            boxes.push_back({Search::Point(trans_box.xMin(), trans_box.yMin()), Search::Point(trans_box.xMax(), trans_box.yMax())});
          }
        }
      }
      for (auto& box : inst_boxes->mterms) {
        odb::Rect trans_box(box.left(), box.bottom(), box.right(), box.top());
        inst_xfm.apply(trans_box);

        if (trans_box.intersects(search_line)) {
          boxes.push_back({Search::Point(trans_box.xMin(), trans_box.yMin()), Search::Point(trans_box.xMax(), trans_box.yMax())});
        }
      }
    }

    auto shapes = search_.searchShapes(layer, search_line.xMin(), search_line.yMin(), search_line.xMax(), search_line.yMax());
    for (auto& [box, poly, net] : shapes) {
      if (options_->isNetVisible(net)) {
        boxes.push_back(box);
      }
    }

    if (options_->areFillsVisible()) {
      auto fills = search_.searchFills(layer, search_line.xMin(), search_line.yMin(), search_line.xMax(), search_line.yMax());
      for (auto& [box, poly, fill] : fills) {
        boxes.push_back(box);
      }
    }

    if (options_->areObstructionsVisible()) {
      auto obs = search_.searchObstructions(layer, search_line.xMin(), search_line.yMin(), search_line.xMax(), search_line.yMax());
      for (auto& [box, poly, ob] : obs) {
        boxes.push_back(box);
      }
    }
  }

  if (options_->areBlockagesVisible()) {
    auto blcks = search_.searchBlockages(search_line.xMin(), search_line.yMin(), search_line.xMax(), search_line.yMax());
    for (auto& [box, poly, blck] : blcks) {
      boxes.push_back(box);
    }
  }

  const auto& [shape_edges, ok] = searchNearestEdge(boxes, pt);
  if (!ok) {
    return {Edge(), false};
  }

  Edge selected_edge;
  if (horizontal) {
    selected_edge = shape_edges.horizontal;
  } else {
    selected_edge = shape_edges.vertical;
  }

  // check if edge should point instead
  const int point_snap_distance = std::max(10.0 / pixels_per_dbu_, // pixels
                                           10.0); //DBU

  std::array<odb::Point, 3> snap_points = {
      snap_edge_.first, // first corner
      odb::Point(
          (snap_edge_.first.x() + snap_edge_.second.x()) / 2,
          (snap_edge_.first.y() + snap_edge_.second.y()) / 2),// midpoint
      snap_edge_.second // last corner
  };
  for (const auto& s_pt : snap_points) {
    if (std::abs(s_pt.x() - pt.x()) < point_snap_distance && std::abs(s_pt.y() - pt.y()) < point_snap_distance) {
      // close to point, so snap to that
      snap_edge_.first = s_pt;
      snap_edge_.second = s_pt;
    }
  }

  return {selected_edge, true};
}

Selected LayoutViewer::selectAtPoint(odb::Point pt_dbu)
{
  if (db_ == nullptr) {
    return Selected();
  }

  // Look for the selected object in reverse layer order
  auto& renderers = Gui::get()->renderers();
  dbTech* tech = db_->getTech();
  std::vector<Selected> selections;

  if (options_->areBlockagesVisible() && options_->areBlockagesSelectable()) {
    auto blockages = search_.searchBlockages(pt_dbu.x(), pt_dbu.y(), pt_dbu.x(), pt_dbu.y());
    for (auto iter : blockages) {
      selections.push_back(makeSelected_(std::get<2>(iter)));
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
      for (auto selected : renderer->select(layer, pt_dbu)) {
        // copy selected items from the renderer
        selections.push_back(selected);
      }
    }

    if (options_->areObstructionsVisible() && options_->areObstructionsSelectable()) {
      auto obs = search_.searchObstructions(
          layer, pt_dbu.x(), pt_dbu.y(), pt_dbu.x(), pt_dbu.y());
      for (auto iter : obs) {
        selections.push_back(makeSelected_(std::get<2>(iter)));
      }
    }

    auto shapes = search_.searchShapes(
        layer, pt_dbu.x(), pt_dbu.y(), pt_dbu.x(), pt_dbu.y());

    // Just return the first one
    for (auto iter : shapes) {
      dbNet* net = std::get<2>(iter);
      if (options_->isNetVisible(net) && options_->isNetSelectable(net)) {
        selections.push_back(makeSelected_(net));
      }
    }
  }

  // Check for objects not in a layer
  for (auto* renderer : renderers) {
    for (auto selected : renderer->select(nullptr, pt_dbu)) {
      selections.push_back(selected);
    }
  }

  // Look for an instance since no shape was found
  auto insts
      = search_.searchInsts(pt_dbu.x(), pt_dbu.y(), pt_dbu.x(), pt_dbu.y());

  for (auto& inst : insts) {
    dbInst* inst_ptr = std::get<2>(inst);
    if (options_->isInstanceVisible(inst_ptr) && options_->isInstanceSelectable(inst_ptr)) {
      selections.push_back(makeSelected_(inst_ptr));
    }
  }

  if (options_->areRulersVisible() && options_->areRulersSelectable()) {
    // Look for rulers
    // because rulers are 1 pixel wide, we'll add another couple of pixels to its width
    const int ruler_margin = 4 / pixels_per_dbu_; // 4 pixels in each direction
    for (auto& ruler : rulers_) {
      if (ruler->fuzzyIntersection(pt_dbu, ruler_margin)) {
        selections.push_back(makeSelected_(ruler.get()));
      }
    }
  }

  if (!selections.empty()) {
    if (selections.size() == 1) {
      // one one item possible, so return that
      return selections[0];
    }

    // more than one item possible, so return the "next" one
    // method: look for the last selected item in the list and select the next one
    // that will emulate a circular queue so we don't just oscillate between the first two
    std::vector<bool> is_selected;
    for (auto& sel : selections) {
      is_selected.push_back(selected_.count(sel) != 0);
    }
    if (std::all_of(is_selected.begin(), is_selected.end(), [](bool b) { return b; })) {
      // everything is selected, so just return first item
      return selections[0];
    }
    is_selected.push_back(is_selected[0]); // add first element to make it a "loop"

    int next_selection_idx;
    // start at end of list and look for the selection item that is directly after a selected item.
    for (next_selection_idx = selections.size(); next_selection_idx > 0; next_selection_idx--) {
      // looking for true followed by false
      if (is_selected[next_selection_idx-1] && !is_selected[next_selection_idx]) {
        break;
      }
    }
    if (next_selection_idx == selections.size()) {
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
      if (snap_edge_.first == snap_edge_.second) { // point snap
        return snap_edge_.first;
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

odb::Point LayoutViewer::findNextSnapPoint(const odb::Point& end_pt, const odb::Point& start_pt, bool snap)
{
  odb::Point snapped = findNextSnapPoint(end_pt, snap);
  if (snap) {
    // snap to horizontal or vertical ruler
    if (std::abs(start_pt.x() - snapped.x()) < std::abs(start_pt.y() - snapped.y())) {
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
  odb::dbBlock* block = getBlock();
  if (block == nullptr) {
    return;
  }
  
  mouse_press_pos_ = event->pos();
  if (event->button() == Qt::LeftButton) {
    Point pt_dbu = screenToDBU(event->pos());
    if (building_ruler_) {
      // build ruler...
      odb::Point next_ruler_pt = findNextRulerPoint(pt_dbu);
      if (ruler_start_ == nullptr) {
        ruler_start_ = std::make_unique<odb::Point>(next_ruler_pt);
      } else {
        emit addRuler(ruler_start_->x(), ruler_start_->y(), next_ruler_pt.x(), next_ruler_pt.y());
        cancelRulerBuild();
      }
    } else if (qGuiApp->keyboardModifiers() & Qt::ShiftModifier) {
      emit addSelected(selectAtPoint(pt_dbu));
    } else if (qGuiApp->keyboardModifiers() & Qt::ControlModifier) {
      emit selected(selectAtPoint(pt_dbu), true);
    } else {
      emit selected(selectAtPoint(pt_dbu), false);
    }
  } else if (event->button() == Qt::RightButton) {
    Point pt_dbu = screenToDBU(event->pos());
    rubber_band_showing_ = true;
    rubber_band_.setTopLeft(event->pos());
    rubber_band_.setBottomRight(event->pos());
    update();
    setCursor(Qt::CrossCursor);
  }
}

void LayoutViewer::mouseMoveEvent(QMouseEvent* event)
{
  dbBlock* block = getBlock();
  if (block == nullptr) {
    return;
  }

  // emit location in microns
  Point pt_dbu = screenToDBU(event->pos());
  qreal to_dbu = block->getDbUnitsPerMicron();
  emit location(pt_dbu.x() / to_dbu, pt_dbu.y() / to_dbu);
  mouse_move_pos_ = event->pos();
  if (building_ruler_) {
    if (!(qGuiApp->keyboardModifiers() & Qt::ControlModifier)) {
      // set to false and toggle to true if edges are available
      snap_edge_showing_ = false;

      bool do_ver = true;
      bool do_hor = true;

      if (ruler_start_ != nullptr && !(qGuiApp->keyboardModifiers() & Qt::ShiftModifier)) {
        const odb::Point mouse_pos = screenToDBU(mouse_move_pos_);

        if (std::abs(ruler_start_->x() - mouse_pos.x()) < std::abs(ruler_start_->y() - mouse_pos.y())) {
          // mostly vertical, so don't look for vertical snaps
          do_ver = false;
        } else {
          // mostly horizontal, so don't look for horizontal snaps
          do_hor = false;
        }
      }

      if (do_ver) {
        const auto& [edge_ver, ok_ver] = findEdge(pt_dbu, false);

        if (ok_ver && do_ver) {
          snap_edge_ = edge_ver;
          snap_edge_showing_ = true;
        }
      }
      if (do_hor) {
        const auto& [edge_hor, ok_hor] = findEdge(pt_dbu, true);
        if (ok_hor) {
          if (!snap_edge_showing_) {
            snap_edge_ = edge_hor;
            snap_edge_showing_ = true;
          } else if ( // check if horizontal is closer
              std::abs(snap_edge_.first.x() - pt_dbu.x()) >
              std::abs(edge_hor.first.y() - pt_dbu.y())) {
            snap_edge_ = edge_hor;
          }
        }
      }
    } else {
      snap_edge_showing_ = false;
    }

    update();
  }
  if (rubber_band_showing_) {
    update();
    rubber_band_.setBottomRight(event->pos());
    update();
  }
}

void LayoutViewer::mouseReleaseEvent(QMouseEvent* event)
{
  dbBlock* block = getBlock();
  if (block == nullptr) {
    return;
  }

  if (event->button() == Qt::RightButton && rubber_band_showing_) {
    rubber_band_showing_ = false;
    update();
    unsetCursor();
    QRect rect = rubber_band_.normalized();
    if (!(QApplication::keyboardModifiers() & Qt::ControlModifier)
        && (rect.width() < 4 || rect.height() < 4)) {
      showLayoutCustomMenu(event->pos());
      return;  // ignore clicks not intended to be drags
    }

    const Rect block_bounds = getBounds(block);

    Rect rubber_band_dbu = screenToDBU(rect);
    // Clip to the block bounds
    Rect bbox = getPaddedRect(block_bounds);

    rubber_band_dbu.set_xlo(qMax(rubber_band_dbu.xMin(), bbox.xMin()));
    rubber_band_dbu.set_ylo(qMax(rubber_band_dbu.yMin(), bbox.yMin()));
    rubber_band_dbu.set_xhi(qMin(rubber_band_dbu.xMax(), bbox.xMax()));
    rubber_band_dbu.set_yhi(qMin(rubber_band_dbu.yMax(), bbox.yMax()));
    zoomTo(rubber_band_dbu);
  }
}

void LayoutViewer::resizeEvent(QResizeEvent* event)
{
  dbBlock* block = getBlock();
  if (block != nullptr) {
    const QSize new_layout_size = event->size();

    const odb::Rect block_bounds = getBounds(block);

    // compute new pixels_per_dbu_
    pixels_per_dbu_ = computePixelsPerDBU(new_layout_size, getPaddedRect(block_bounds));

    // compute new centering shift
    // the offset necessary to center the block in the viewport.
    // expand area to fill whole scroller window
    const QSize new_area = new_layout_size.expandedTo(scroller_->size());
    centering_shift_ = QPoint(
        (new_area.width()  - block_bounds.dx() * pixels_per_dbu_) / 2,
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
  // store obstructions
  for (dbBox* box : master->getObstructions()) {
    dbTechLayer* layer = box->getTechLayer();
    dbTechLayerType type = layer->getType();
    if (type != dbTechLayerType::ROUTING && type != dbTechLayerType::CUT) {
      continue;
    }
    boxes[layer].obs.emplace_back(QRect(QPoint(box->xMin(), box->yMin()),
                                        QPoint(box->xMax(), box->yMax())));
  }

  // store mterms
  for (dbMTerm* mterm : master->getMTerms()) {
    for (dbMPin* mpin : mterm->getMPins()) {
      for (dbBox* box : mpin->getGeometry()) {
        dbTechLayer* layer = box->getTechLayer();
        dbTechLayerType type = layer->getType();
        if (type != dbTechLayerType::ROUTING && type != dbTechLayerType::CUT) {
          continue;
        }
        boxes[layer].mterms.emplace_back(
            QRect(QPoint(box->xMin(), box->yMin()),
                  QPoint(box->xMax(), box->yMax())));
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
                              dbBlock* block,
                              QPainter* painter,
                              const Rect& bounds)
{
  if (!options_->arePrefTracksVisible()
      && !options_->areNonPrefTracksVisible()) {
    return;
  }

  dbTrackGrid* grid = block->findTrackGrid(layer);
  if (!grid) {
    return;
  }

  int min_resolution = nominalViewableResolution();
  Rect block_bounds;
  block->getBBox()->getBox(block_bounds);
  const Rect draw_bounds = block_bounds.intersect(bounds);

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

void LayoutViewer::drawRows(dbBlock* block,
                            QPainter* painter,
                            const Rect& bounds)
{
  if (!options_->areRowsVisible()) {
    return;
  }
  int min_resolution = nominalViewableResolution();
  // three possible draw cases:
  // 1) resolution allows for individual sites -> draw all
  // 2) individual sites too small -> just draw row outlines
  // 3) row is too small -> dont draw anything

  QPen pen(options_->rowColor());
  pen.setCosmetic(true);
  painter->setPen(pen);
  painter->setBrush(Qt::NoBrush);
  for (dbRow* row : block->getRows()) {
    int x;
    int y;
    row->getOrigin(x, y);

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
    if (!w_visible) {
      // individual sites not visible, just draw the row
      if (dir == dbRowDir::HORIZONTAL) {
        w = spacing*count;
      }
      else {
        h = spacing*count;
      }
      count = 1;
    }
    if (h_visible) {
      // row height can be seen
      for (int i = 0; i < count; ++i) {
        const Rect row(x, y, x + w, y + h);
        if (row.intersects(bounds)) {
          // only paint rows that can be seen
          painter->drawRect(QRect(QPoint(x, y), QPoint(x + w, y + h)));
        }

        if (dir == dbRowDir::HORIZONTAL) {
          x += spacing;
        } else {
          y += spacing;
        }
      }
    }
  }
}

void LayoutViewer::drawSelected(Painter& painter)
{
  for (auto& selected : selected_) {
    selected.highlight(painter);
  }
}

void LayoutViewer::drawHighlighted(Painter& painter)
{
  int highlight_group = 0;
  for (auto& highlight_set : highlighted_) {
    for (auto& highlighted : highlight_set)
      highlighted.highlight(painter, false /* select_flag*/, highlight_group);
    highlight_group++;
  }
}

void LayoutViewer::drawRulers(Painter& painter)
{
  if (!options_->areRulersVisible()) {
    return;
  }

  for (auto& ruler : rulers_) {
    painter.drawRuler(
        ruler->getPt0().x(), ruler->getPt0().y(), ruler->getPt1().x(), ruler->getPt1().y(), ruler->getLabel());
  }
}

void LayoutViewer::drawCongestionMap(Painter& painter, const odb::Rect& bounds)
{
  if (!options_->isCongestionVisible()) {
    return;
  }

  auto block = getBlock();
  if (block == nullptr)
    return;
  auto grid = block->getGCellGrid();
  if (grid == nullptr)
    return;

  auto gcell_congestion_data = grid->getCongestionMap();
  if (gcell_congestion_data.empty()) {
    return;
  }

  std::vector<int> x_grid, y_grid;
  uint x_grid_sz, y_grid_sz;
  grid->getGridX(x_grid);
  x_grid_sz = x_grid.size();
  grid->getGridY(y_grid);
  y_grid_sz = y_grid.size();

  bool show_hor_congestion = options_->showHorizontalCongestion();
  bool show_ver_congestion = options_->showVerticalCongestion();
  auto min_congestion_to_show = options_->getMinCongestionToShow();
  auto max_congestion_to_show = options_->getMaxCongestionToShow();

  for (auto& [key, cong_data] : gcell_congestion_data) {
    uint x_idx = key.first;
    uint y_idx = key.second;

    if (x_idx >= x_grid_sz || y_idx >= y_grid_sz) {
      logger_->warn(utl::GUI, 4, "Skipping malformed GCell {} {} ({} {})",
                    x_idx, y_idx, x_grid_sz, y_grid_sz);
      continue;
    }

    auto gcell_rect = odb::Rect(
        x_grid[x_idx], y_grid[y_idx], x_grid[x_idx + 1], y_grid[y_idx + 1]);

    if (!gcell_rect.intersects(bounds))
      continue;

    auto hor_capacity = cong_data.horizontal_capacity;
    auto hor_usage = cong_data.horizontal_usage;
    auto ver_capacity = cong_data.vertical_capacity;
    auto ver_usage = cong_data.vertical_usage;

    //-1 indicates capacity is not well defined...
    float hor_congestion
        = hor_capacity != 0 ? (hor_usage * 100.0) / hor_capacity : -1;
    float ver_congestion
        = ver_capacity != 0 ? (ver_usage * 100.0) / ver_capacity : -1;

    float congestion = ver_congestion;
    if (show_hor_congestion && show_ver_congestion)
      congestion = std::max(hor_congestion, ver_congestion);
    else if (show_hor_congestion)
      congestion = hor_congestion;
    else
      congestion = ver_congestion;

    if (congestion <= 0 || congestion < min_congestion_to_show
        || congestion > max_congestion_to_show)
      continue;

    auto gcell_color = options_->getCongestionColor(congestion);
    Painter::Color color(
        gcell_color.red(), gcell_color.green(), gcell_color.blue(), 100);
    painter.setPen(color, true);
    painter.setBrush(color);
    painter.drawRect(gcell_rect);
  }
}

// Draw the instances bounds
void LayoutViewer::drawInstanceOutlines(QPainter* painter,
                                        const std::vector<odb::dbInst*>& insts)
{
  int minimum_height_for_tag = nominalViewableResolution();
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
    painter->drawRect(
        master_box.xMin(), master_box.yMin(), master_box.dx(), master_box.dy());

    // Draw an orientation tag in corner if useful in size
    int master_h = master->getHeight();
    if (master_h >= minimum_height_for_tag) {
      qreal master_w = master->getWidth();
      qreal tag_size = 0.1 * master_h;
      qreal tag_x = master_box.xMin() + std::min(tag_size / 2, master_w);
      qreal tag_y = master_box.yMin() + tag_size;
      painter->drawLine(QPointF(tag_x, master_box.yMin()),
                        QPointF(master_box.xMin(), tag_y));
    }
  }
  painter->setTransform(initial_xfm);
}

// Draw the instances' shapes
void LayoutViewer::drawInstanceShapes(dbTechLayer* layer,
                                      QPainter* painter,
                                      const std::vector<odb::dbInst*>& insts)
{
  int minimum_height = nominalViewableResolution();
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

    if (options_->areObstructionsVisible()) {
      painter->setBrush(color.lighter());
      for (auto& box : boxes->obs) {
        painter->drawRect(box);
      }
    }

    painter->setBrush(QBrush(color, brush_pattern));
    for (auto& box : boxes->mterms) {
      painter->drawRect(box);
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

  // core cell text should be 10% of cell height
  static const float size_target = 0.1;
  // text should not fill more than 90% of the instance height or width
  static const float size_limit = 0.9;
  static const float rotation_limit = 0.85; // slightly lower to prevent oscillating rotations when zooming

  // limit non-core text to 1/2.0 (50%) of cell height or width
  static const float non_core_scale_limit = 2.0;

  const float font_core_scale_height = size_target * pixels_per_dbu_;
  const float font_core_scale_width = size_limit * pixels_per_dbu_;

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

    Rect instance_box;
    inst->getBBox()->getBox(instance_box);

    QString name = inst->getName().c_str();
    QRectF instance_bbox_in_px = dbuToScreen(instance_box);

    QRectF text_bounding_box = font_metrics.boundingRect(name);

    bool do_rotate = false;
    if (text_bounding_box.width() > rotation_limit * instance_bbox_in_px.width()) {
      // non-rotated text will not fit without elide
      if (instance_bbox_in_px.height() > instance_bbox_in_px.width()) {
        // check if more text will fit if rotated
        do_rotate = true;
      }
    }

    qreal text_height_check = non_core_scale_limit * text_bounding_box.height();
    // don't show text if it's more than "non_core_scale_limit" of cell height/width
    // this keeps text from dominating the cell size
    if (!do_rotate && text_height_check > instance_bbox_in_px.height()) {
      continue;
    }
    if (do_rotate && text_height_check > instance_bbox_in_px.width()) {
      continue;
    }

    if (do_rotate) {
      name = font_metrics.elidedText(name, Qt::ElideLeft, size_limit * instance_bbox_in_px.height());
    } else {
      name = font_metrics.elidedText(name, Qt::ElideLeft, size_limit * instance_bbox_in_px.width());
    }

    painter->translate(instance_box.xMin(), instance_box.yMin());
    painter->scale(1.0 / pixels_per_dbu_, -1.0 / pixels_per_dbu_);
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

void LayoutViewer::drawBlockages(QPainter* painter,
                                 const Rect& bounds)
{
  if (!options_->areBlockagesVisible()) {
    return;
  }
  painter->setPen(Qt::NoPen);
  painter->setBrush(QBrush(options_->placementBlockageColor(), options_->placementBlockagePattern()));

  auto blockage_range = search_.searchBlockages(
      bounds.xMin(), bounds.yMin(), bounds.xMax(), bounds.yMax(), fineViewableResolution());

  for (auto& [box, poly, blockage] : blockage_range) {
    Rect bbox;
    blockage->getBBox()->getBox(bbox);
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

  auto obstructions_range = search_.searchObstructions(
      layer, bounds.xMin(), bounds.yMin(), bounds.xMax(), bounds.yMax(), fineViewableResolution());

  for (auto& [box, poly, obs] : obstructions_range) {
    Rect bbox;
    obs->getBBox()->getBox(bbox);
    painter->drawRect(bbox.xMin(), bbox.yMin(), bbox.dx(), bbox.dy());
  }
}

// Draw the region of the block.  Depth is not yet used but
// is there for hierarchical design support.
void LayoutViewer::drawBlock(QPainter* painter,
                             const Rect& bounds,
                             dbBlock* block,
                             int depth)
{
  int min_resolution = fineViewableResolution();  // 1 pixel in DBU
  int nominal_resolution = nominalViewableResolution();
  LayerBoxes boxes;
  QTransform initial_xfm = painter->transform();

  auto& renderers = Gui::get()->renderers();
  GuiPainter gui_painter(painter,
                         options_,
                         pixels_per_dbu_,
                         block->getDbUnitsPerMicron());

  // Draw die area, if set
  painter->setPen(QPen(Qt::gray, 0));
  painter->setBrush(QBrush());
  Rect bbox;
  block->getDieArea(bbox);
  if (bbox.area() > 0) {
    painter->drawRect(bbox.xMin(), bbox.yMin(), bbox.dx(), bbox.dy());
  }

  auto inst_range = search_.searchInsts(
      bounds.xMin(), bounds.yMin(), bounds.xMax(), bounds.yMax(), min_resolution);

  // Cache the search results as we will iterate over the instances
  // for each layer.
  std::vector<dbInst*> insts;
  insts.reserve(10000);
  for (auto& [box, poly, inst] : inst_range) {
    if (options_->isInstanceVisible(inst)) {
      insts.push_back(inst);
    }
  }

  drawInstanceOutlines(painter, insts);

  // draw blockages
  drawBlockages(painter, bounds);

  dbTech* tech = block->getDataBase()->getTech();
  for (dbTechLayer* layer : tech->getLayers()) {
    if (!options_->isVisible(layer)) {
      continue;
    }

    // Skip the cut layer if the cuts will be too small to see
    if (layer->getType() == dbTechLayerType::CUT && cut_maximum_size_[layer] < nominal_resolution) {
      continue;
    }

    drawInstanceShapes(layer, painter, insts);

    drawObstructions(layer, painter, bounds);

    // Now draw the shapes
    QColor color = getColor(layer);
    Qt::BrushStyle brush_pattern = getPattern(layer);
    painter->setBrush(QBrush(color, brush_pattern));
    painter->setPen(QPen(color, 0));
    auto iter = search_.searchShapes(layer,
                                     bounds.xMin(),
                                     bounds.yMin(),
                                     bounds.xMax(),
                                     bounds.yMax(),
                                     nominal_resolution);

    for (auto& i : iter) {
      if (!options_->isNetVisible(std::get<2>(i))) {
        continue;
      }
      auto poly = std::get<1>(i);
      int size = poly.outer().size();
      if (size == 5) {
        auto bbox = std::get<0>(i);
        const auto& ll = bbox.min_corner();
        const auto& ur = bbox.max_corner();
        painter->drawRect(
            QRect(QPoint(ll.x(), ll.y()), QPoint(ur.x(), ur.y())));
      } else {
        QPolygon qpoly(size);
        for (int i = 0; i < size; i++)
          qpoly.setPoint(i, poly.outer()[i].x(), poly.outer()[i].y());
        painter->drawPolygon(qpoly);
      }
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
                                      nominal_resolution);

      for (auto& i : iter) {
        const auto& ll = std::get<0>(i).min_corner();
        const auto& ur = std::get<0>(i).max_corner();
        painter->drawRect(
            QRect(QPoint(ll.x(), ll.y()), QPoint(ur.x(), ur.y())));
      }
    }

    drawTracks(layer, block, painter, bounds);
    for (auto* renderer : renderers) {
      renderer->drawLayer(layer, gui_painter);
    }
  }

  // draw instance names
  drawInstanceNames(painter, insts);

  drawRows(block, painter, bounds);
  for (auto* renderer : renderers) {
    renderer->drawObjects(gui_painter);
  }

  drawCongestionMap(gui_painter, bounds);
}

void LayoutViewer::drawPinMarkers(QPainter* painter,
                                  const odb::Rect& bounds,
                                  odb::dbBlock* block)
{
  auto block_bbox = block->getBBox();
  auto block_width = block_bbox->getWidth();
  auto block_height = block_bbox->getLength();
  double mult_factor = (2.0 * fit_pixels_per_dbu_) / (100 * pixels_per_dbu_);
  auto max_dim
      = std::max(block_width, block_height)
        * mult_factor;  // 4 Percent of bounds is used to draw pin-markers

  for (odb::dbBTerm* term : block->getBTerms()) {
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
        Rect pin_rect(box->xMin(), box->yMin(), box->xMax(), box->yMax());
        odb::dbTechLayer* layer = box->getTechLayer();
        Point pin_center(pin_rect.xMin() + pin_rect.dx() / 2,
                         pin_rect.yMin() + pin_rect.dy() / 2);
        QColor layer_color = options_->color(layer);

        QPainterPath path;
        auto dist_to_left = std::abs(pin_center.x() - block_bbox->xMin());
        auto dist_to_right = std::abs(pin_center.x() - block_bbox->xMax());
        auto dist_to_top = std::abs(pin_center.y() - block_bbox->yMax());
        auto dist_to_bot = std::abs(pin_center.y() - block_bbox->yMin());
        std::vector<int> dists{
            dist_to_left, dist_to_right, dist_to_top, dist_to_bot};
        Point pt1, pt2;
        int arg_min = std::distance(
            dists.begin(), std::min_element(dists.begin(), dists.end()));
        if (arg_min <= 1) {  // Closer to Left/Right Edge
          if (pin_dir == odb::dbIoType::INPUT) {
            pt1 = Point(pin_center.getX() + max_dim,
                        pin_center.getY() + max_dim / 4);
            pt2 = Point(pin_center.getX() + max_dim,
                        pin_center.getY() - max_dim / 4);
          } else {
            pt1 = Point(pin_center.getX() - max_dim,
                        pin_center.getY() - max_dim / 4);
            pt2 = Point(pin_center.getX() - max_dim,
                        pin_center.getY() + max_dim / 4);
          }
        } else {  // Closer to top/bot Edge
          if (pin_dir == odb::dbIoType::OUTPUT
              || pin_dir == odb::dbIoType::INOUT) {
            pt1 = Point(pin_center.getX() - max_dim / 4,
                        pin_center.getY() - max_dim);
            pt2 = Point(pin_center.getX() + max_dim / 4,
                        pin_center.getY() - max_dim);
          } else {
            pt1 = Point(pin_center.getX() - max_dim / 4,
                        pin_center.getY() + max_dim);
            pt2 = Point(pin_center.getX() + max_dim / 4,
                        pin_center.getY() + max_dim);
          }
        }

        painter->setPen(layer_color);
        path.moveTo(pt1.getX(), pt1.getY());

        path.lineTo(pt2.getX(), pt2.getY());

        path.lineTo(pin_center.getX(), pin_center.getY());
        path.lineTo(pt1.getX(), pt1.getY());

        painter->fillPath(path, QBrush(layer_color));
      }
    }
  }
}

odb::Point LayoutViewer::screenToDBU(const QPointF& point)
{
  // Flip the y-coordinate (see file level comments)
  return Point((point.x()-centering_shift_.x()) / pixels_per_dbu_,
               (centering_shift_.y()-point.y()) / pixels_per_dbu_);
}

Rect LayoutViewer::screenToDBU(const QRectF& screen_rect)
{
  int dbu_left = (int) floor((screen_rect.left()-centering_shift_.x()) / pixels_per_dbu_);
  int dbu_right = (int) ceil((screen_rect.right()-centering_shift_.x()) / pixels_per_dbu_);
  // Flip the y-coordinate (see file level comments)
  int dbu_top = (int) floor((centering_shift_.y()-screen_rect.top()) / pixels_per_dbu_);
  int dbu_bottom = (int) ceil((centering_shift_.y()-screen_rect.bottom()) / pixels_per_dbu_);

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
  qreal screen_left   = centering_shift_.x() + dbu_rect.xMin() * pixels_per_dbu_;
  qreal screen_right  = centering_shift_.x() + dbu_rect.xMax() * pixels_per_dbu_;
  qreal screen_top    = centering_shift_.y() - dbu_rect.yMax() * pixels_per_dbu_;
  qreal screen_bottom = centering_shift_.y() - dbu_rect.yMin() * pixels_per_dbu_;

  return QRectF(QPointF(screen_left, screen_top),
                QPointF(screen_right, screen_bottom));
}

void LayoutViewer::updateBlockPainting(const QRect& area, odb::dbBlock* block)
{
  if (block_drawing_ != nullptr) {
    // no changes detected, so no need to update
    return;
  }

  // build new drawing of layout
  block_drawing_ = std::make_unique<QPixmap>(area.width(), area.height());
  block_drawing_->fill(Qt::transparent);

  QPainter block_painter(block_drawing_.get());
  block_painter.setRenderHints(QPainter::Antialiasing);

  // apply transforms
  block_painter.translate(-area.topLeft());
  block_painter.translate(centering_shift_);
  // apply scaling
  block_painter.scale(pixels_per_dbu_, -pixels_per_dbu_);

  const Rect dbu_bounds = screenToDBU(area);

  // paint layout
  drawBlock(&block_painter, dbu_bounds, block, 0);
  if (options_->arePinMarkersVisible()) {
    drawPinMarkers(&block_painter, dbu_bounds, block);
  }
}

void LayoutViewer::paintEvent(QPaintEvent* event)
{
  dbBlock* block = getBlock();
  if (!block) {
    return;
  }

  QPainter painter(this);
  painter.setRenderHints(QPainter::Antialiasing);

  // Fill draw region with black
  painter.setPen(QPen(background_, 0));
  painter.setBrush(background_);
  painter.drawRect(event->rect());

  if (!design_loaded_) {
    return;
  }

  if (!search_init_) {
    search_.init(block);
    search_init_ = true;
  }

  if (cut_maximum_size_.empty()) {
    generateCutLayerMaximumSizes();
  }

  // check if we can use the old image
  const QRect draw_bounds = visibleRegion().boundingRect();
  updateBlockPainting(draw_bounds, block);

  // draw cached block
  painter.drawPixmap(draw_bounds.topLeft(), *block_drawing_);

  painter.save();
  painter.translate(centering_shift_);
  painter.scale(pixels_per_dbu_, -pixels_per_dbu_);

  GuiPainter gui_painter(&painter,
                         options_,
                         pixels_per_dbu_,
                         block->getDbUnitsPerMicron());

  // draw selected and over top level and fast painting events
  drawSelected(gui_painter);
  // Always last so on top
  drawHighlighted(gui_painter);
  drawRulers(gui_painter);

  // draw partial ruler if present
  if (building_ruler_ && ruler_start_ != nullptr) {
    odb::Point snapped_mouse_pos = findNextRulerPoint(screenToDBU(mouse_move_pos_));
    gui_painter.drawRuler(ruler_start_->x(), ruler_start_->y(), snapped_mouse_pos.x(), snapped_mouse_pos.y());
  }

  // draw edge currently considered snapped to
  if (snap_edge_showing_) {
    painter.setPen(QPen(Qt::white, 0));
    painter.setBrush(QBrush());
    if (snap_edge_.first != snap_edge_.second) {
      painter.drawLine(
          QLine(
              QPoint(snap_edge_.first.x(), snap_edge_.first.y()),
              QPoint(snap_edge_.second.x(), snap_edge_.second.y())));
    } else {
      painter.drawEllipse(QPointF(snap_edge_.first.x(), snap_edge_.first.y()),
                          5.0 / pixels_per_dbu_,
                          5.0 / pixels_per_dbu_);
    }
  }

  painter.restore();

  // use bounding Rect as event might just be the rubber_band
  drawScaleBar(&painter, block, draw_bounds);

  if (rubber_band_showing_) {
    painter.setPen(QPen(Qt::white, 0));
    painter.setBrush(QBrush());
    painter.drawRect(rubber_band_.normalized());
  }
}

void LayoutViewer::fullRepaint()
{
  block_drawing_ = nullptr;
  update();
}

void LayoutViewer::drawScaleBar(QPainter* painter, odb::dbBlock* block, const QRect& rect)
{
  if (!options_->isScaleBarVisible()) {
    return;
  }

  const qreal pixels_per_mircon = pixels_per_dbu_ * block->getDbUnitsPerMicron();
  const qreal window_width = rect.width() / pixels_per_mircon;
  const qreal target_width = 0.1 * window_width;

  const qreal peg_width = std::pow(10.0, std::floor(std::log10(target_width))); // microns
  int peg_incr = std::round(target_width / peg_width);
  const qreal bar_size = peg_incr * peg_width;
  if (peg_incr == 1) {
    // make 10 segments if 1
    peg_incr = 10;
  }

  const int bar_height = 10; // px

  int bar_width = bar_size * pixels_per_mircon;

  double scale_unit;
  QString unit_text;
  if (bar_size > 1000) {
    scale_unit = 0.001;
    unit_text = "mm";
  } else if (bar_size > 1) {
    scale_unit = 1;
    unit_text = "\u03bcm"; // um
  } else if (bar_size > 0.001) {
    scale_unit = 1000;
    unit_text = "nm";
  } else {
    scale_unit = 1e6;
    unit_text = "pm";
  }

  auto color = Qt::white;
  painter->setPen(QPen(color, 2));
  painter->setBrush(Qt::transparent);

  const QRectF scale_bar_outline(rect.left() + 10, rect.bottom() - 20, bar_width, bar_height);

  // draw base half bar shape |_|
  painter->drawLine(scale_bar_outline.topLeft(), scale_bar_outline.bottomLeft());
  painter->drawLine(scale_bar_outline.bottomLeft(), scale_bar_outline.bottomRight());
  painter->drawLine(scale_bar_outline.bottomRight(), scale_bar_outline.topRight());

  const QFontMetrics font_metric = painter->fontMetrics();

  const int text_px_offset = 2;
  const int text_keep_out = font_metric.averageCharWidth(); // ensure text has approx one characters spacing

  // draw total size over right size
  const QString bar_text_end = QString::number(static_cast<int>(bar_size * scale_unit)) + unit_text;
  const QRect end_box = font_metric.boundingRect(bar_text_end);
  painter->drawText(
      scale_bar_outline.topRight() - QPointF(end_box.center().x(), text_px_offset),
      bar_text_end);
  const qreal end_offset  = scale_bar_outline.right() - 0.5 * end_box.width() - text_keep_out;

  // draw "0" over left side
  const QRect zero_box = font_metric.boundingRect("0");
  // dont draw if the 0 is too close or overlapping with the right side text
  if (scale_bar_outline.left() + zero_box.center().x() < end_offset) {
    painter->drawText(
        scale_bar_outline.topLeft() - QPointF(zero_box.center().x(), text_px_offset),
        "0");
  }

  // margin around 0 to avoid drawing first available increment
  const qreal zero_offset = scale_bar_outline.left() + 0.5 * zero_box.width() + text_keep_out;
  const qreal segment_width = static_cast<double>(bar_width) / peg_incr;
  const double peg_increment = bar_size / peg_incr;
  bool middle_shown = false; // flag to indicate the middle tick marker has been drawn

  if (segment_width > 4) {
    // draw pegs, don't draw if they are basically overlapping
    for (int i = 1; i < peg_incr; i++) {
      QPointF p1(scale_bar_outline.left() + i * segment_width, scale_bar_outline.bottom());
      QPointF p2 = p1 - QPointF(0, bar_height / 2);

      if (!middle_shown) {
        // only one peg increment
        QString peg_text = QString::number(static_cast<int>(i * peg_increment * scale_unit));
        QRect peg_text_box = font_metric.boundingRect(peg_text);

        // check if text will fit next to "0"
        if (p1.x() - 0.5 * peg_text_box.width() > zero_offset) {
          middle_shown = true;

          // check to make sure there is room at the end
          if (p1.x() + 0.5 * peg_text_box.width() < end_offset) {
            p2 = p1 - QPointF(0, 3 * bar_height / 4.0); // make this peg a little taller
            painter->drawText(
                QPointF(p2.x() - peg_text_box.center().x(), scale_bar_outline.top() - text_px_offset),
                peg_text);
          }
        }
      }

      painter->drawLine(p1, p2);
    }
  }
}

void LayoutViewer::updateShapes()
{
  // This is not very smart - we just clear all the search structure
  // rather than try to surgically update it.
  if (search_init_) {
    search_.clear();
    search_init_ = false;
  }
  fullRepaint();
}

void LayoutViewer::fit()
{
  dbBlock* block = getBlock();
  if (block == nullptr) {
    return;
  }

  Rect bbox = getBounds(block);
  if (bbox.dx() == 0 || bbox.dy() == 0) {
    return;
  }

  zoomTo(bbox);
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
  design_loaded_ = true;
  addOwner(block);  // register as a callback object
  fit();
}

void LayoutViewer::setScroller(LayoutScroll* scroller)
{
  scroller_ = scroller;

  // ensure changes in the scroll area are announced to the layout viewer
  connect(scroller_, SIGNAL(viewportChanged()), this, SLOT(viewportUpdated()));
  connect(scroller_, SIGNAL(centerChanged(int, int)), this, SLOT(updateCenter(int, int)));
  connect(scroller_, SIGNAL(centerChanged(int, int)), this, SLOT(fullRepaint()));
}

void LayoutViewer::viewportUpdated()
{
  odb::dbBlock* block = getBlock();
  if (block == nullptr) {
    return;
  }

  bool zoomed_in = fit_pixels_per_dbu_ < pixels_per_dbu_;

  // determine new fit_pixels_per_dbu_ based on current viewport size
  fit_pixels_per_dbu_ = computePixelsPerDBU(
      scroller_->maximumViewportSize(),
      getPaddedRect(getBounds(block)));

  // when zoomed in don't update size,
  // else update size of window
  if (!zoomed_in) {
    resize(scroller_->maximumViewportSize());
  }
}

void LayoutViewer::saveImage(const QString& filepath, const Rect& region)
{
  dbBlock* block = getBlock();
  if (block == nullptr) {
    return;
  }

  QList<QByteArray> valid_extensions = QImageWriter::supportedImageFormats();

  QString save_filepath;
  if (filepath.isEmpty()) {
    QString images_filter = "Images (";
    for (const QString& ext : valid_extensions) {
      images_filter += "*." + ext + " ";
    }
    images_filter += ")";

    save_filepath = QFileDialog::getSaveFileName(
        this,
        tr("Save Layout"),
        "",
        images_filter);
  } else {
    save_filepath = filepath;
  }

  if (save_filepath.isEmpty()) {
    return;
  }

  // check for a valid extension, if not found add .png
  if (!std::any_of(
      valid_extensions.begin(),
      valid_extensions.end(),
      [save_filepath](const QString& ext) {
        return save_filepath.endsWith("."+ext);
      })) {
    save_filepath += ".png";
    logger_->warn(utl::GUI, 10, "File path does not end with a valid extension, new path is: {}", save_filepath.toStdString());
  }

  QRegion save_region;
  if (region.dx() == 0 || region.dy() == 0) {
    // default to just that is currently visible
    save_region = visibleRegion();
  } else {
    const QRectF screen_region = dbuToScreen(region);
    save_region = QRegion(
        screen_region.left(),  screen_region.top(),
        screen_region.width(), screen_region.height());
  }

  const QRect bounding_rect = save_region.boundingRect();
  QImage img(bounding_rect.width(), bounding_rect.height(), QImage::Format_ARGB32_Premultiplied);
  if (!img.isNull()) {
    img.fill(background_);
    render(&img, {0, 0}, save_region);
    if (!img.save(save_filepath)) {
      logger_->warn(utl::GUI, 11, "Failed to write image: {}", save_filepath.toStdString());
    }
  } else {
    logger_->warn(utl::GUI, 12, "Image is too big to be generated: {}px x {}px", bounding_rect.width(), bounding_rect.height());
  }
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
  menu_actions_[SAVE_VISIBLE_IMAGE_ACT] = save_menu->addAction(tr("Visible layout"));
  menu_actions_[SAVE_WHOLE_IMAGE_ACT] = save_menu->addAction(tr("Entire layout"));

  // Clear Actions
  menu_actions_[CLEAR_SELECTIONS_ACT] = clear_menu->addAction(tr("Selections"));
  menu_actions_[CLEAR_HIGHLIGHTS_ACT] = clear_menu->addAction(tr("Highlights"));
  menu_actions_[CLEAR_RULERS_ACT] = clear_menu->addAction(tr("Rulers"));
  menu_actions_[CLEAR_ALL_ACT] = clear_menu->addAction(tr("All"));

  // Connect Slots to Actions...
  connect(menu_actions_[SELECT_CONNECTED_INST_ACT],
          &QAction::triggered,
          this,
          [this]() { this->selectHighlightConnectedInst(true); });
  connect(menu_actions_[SELECT_OUTPUT_NETS_ACT],
          &QAction::triggered,
          this,
          [this]() { this->selectHighlightConnectedNets(true, true, false); });
  connect(menu_actions_[SELECT_INPUT_NETS_ACT],
          &QAction::triggered,
          this,
          [this]() { this->selectHighlightConnectedNets(true, false, true); });
  connect(
      menu_actions_[SELECT_ALL_NETS_ACT], &QAction::triggered, this, [this]() {
        this->selectHighlightConnectedNets(true, true, true);
      });

  connect(menu_actions_[HIGHLIGHT_CONNECTED_INST_ACT],
          &QAction::triggered,
          this,
          [this]() { this->selectHighlightConnectedInst(false); });
  connect(menu_actions_[HIGHLIGHT_OUTPUT_NETS_ACT],
          &QAction::triggered,
          this,
          [this]() { this->selectHighlightConnectedNets(false, true, false); });
  connect(menu_actions_[HIGHLIGHT_INPUT_NETS_ACT],
          &QAction::triggered,
          this,
          [this]() { this->selectHighlightConnectedNets(false, false, true); });
  connect(menu_actions_[HIGHLIGHT_ALL_NETS_ACT],
          &QAction::triggered,
          this,
          [this]() { this->selectHighlightConnectedNets(false, true, true); });

  connect(menu_actions_[VIEW_ZOOMIN_ACT], &QAction::triggered, this, [this]() {
    this->zoomIn();
  });
  connect(menu_actions_[VIEW_ZOOMOUT_ACT], &QAction::triggered, this, [this]() {
    this->zoomOut();
  });
  connect(menu_actions_[VIEW_ZOOMFIT_ACT], &QAction::triggered, this, [this]() {
    this->fit();
  });

  connect(menu_actions_[SAVE_VISIBLE_IMAGE_ACT], &QAction::triggered, this, [this]() {
    saveImage("");
  });
  connect(menu_actions_[SAVE_WHOLE_IMAGE_ACT], &QAction::triggered, this, [this]() {
    const QSize whole_size = size();
    saveImage("", screenToDBU(QRectF(0, 0, whole_size.width(), whole_size.height())));
  });

  connect(
      menu_actions_[CLEAR_SELECTIONS_ACT], &QAction::triggered, this, [this]() {
        Gui::get()->clearSelections();
      });
  connect(
      menu_actions_[CLEAR_HIGHLIGHTS_ACT], &QAction::triggered, this, [this]() {
        Gui::get()->clearHighlights(-1);
      });
  connect(menu_actions_[CLEAR_RULERS_ACT], &QAction::triggered, this, [this]() {
    Gui::get()->clearRulers();
  });
  connect(menu_actions_[CLEAR_ALL_ACT], &QAction::triggered, this, [this]() {
    Gui::get()->clearSelections();
    Gui::get()->clearHighlights(-1);
    Gui::get()->clearRulers();
  });
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
  // make sure the whole visible layout is updated, not just the newly visible part
  widget()->update();
}

// Handles zoom in/out on ctrl-wheel
void LayoutScroll::wheelEvent(QWheelEvent* event)
{
  if (!event->modifiers().testFlag(Qt::ControlModifier)) {
    QScrollArea::wheelEvent(event);
    return;
  }

  const odb::Point mouse_pos = viewer_->screenToDBU(viewer_->mapFromGlobal(QCursor::pos()));
  if (event->angleDelta().y() > 0) {
    viewer_->zoomIn(mouse_pos, true);
  } else {
    viewer_->zoomOut(mouse_pos, true);
  }
  // ensure changes are processed before the next wheel event to prevent zoomIn and Out
  // from jumping around on the ScrollBars
  QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
}

void LayoutViewer::generateCutLayerMaximumSizes()
{
  if (db_ == nullptr) {
    return;
  }

  dbTech* tech = db_->getTech();
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
              width = std::max(width, static_cast<int>(std::max(box->getDX(), box->getDY())));
            }
          }
        }
      }
      cut_maximum_size_[layer] = width;
    }
  }
}

void LayoutViewer::inDbNetDestroy(dbNet* net)
{
  updateShapes();
}

void LayoutViewer::inDbInstDestroy(dbInst* inst)
{
  if (inst->isPlaced()) {
    updateShapes();
  }
}

void LayoutViewer::inDbInstSwapMasterAfter(dbInst* inst)
{
  if (inst->isPlaced()) {
    updateShapes();
  }
}

void LayoutViewer::inDbInstPlacementStatusBefore(
    dbInst* inst,
    const dbPlacementStatus& status)
{
  if (inst->getPlacementStatus().isPlaced() != status.isPlaced()) {
    updateShapes();
  }
}

void LayoutViewer::inDbPostMoveInst(dbInst* inst)
{
  if (inst->isPlaced()) {
    updateShapes();
  }
}

void LayoutViewer::inDbBPinDestroy(dbBPin* pin)
{
  updateShapes();
}

void LayoutViewer::inDbFillCreate(dbFill* fill)
{
  updateShapes();
}

void LayoutViewer::inDbWireCreate(dbWire* wire)
{
  updateShapes();
}

void LayoutViewer::inDbWireDestroy(dbWire* wire)
{
  updateShapes();
}

void LayoutViewer::inDbSWireCreate(dbSWire* wire)
{
  updateShapes();
}

void LayoutViewer::inDbSWireDestroy(dbSWire* wire)
{
  updateShapes();
}

void LayoutViewer::inDbBlockageCreate(odb::dbBlockage* blockage)
{
  updateShapes();
}

void LayoutViewer::inDbObstructionCreate(odb::dbObstruction* obs)
{
  updateShapes();
}

void LayoutViewer::inDbObstructionDestroy(odb::dbObstruction* obs)
{
  updateShapes();
}

void LayoutViewer::inDbBlockSetDieArea(odb::dbBlock* block)
{
  // This happens when initialize_floorplan is run and it make sense
  // to fit as current zoom will be on a zero sized block.
  fit();
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
