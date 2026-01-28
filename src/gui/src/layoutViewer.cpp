// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#include "layoutViewer.h"

#include <QApplication>
#include <QColor>
#include <QDateTime>
#include <QFileDialog>
#include <QFont>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QImageWriter>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QScrollBar>
#include <QSizePolicy>
#include <QStaticText>
#include <QString>
#include <QToolButton>
#include <QToolTip>
#include <QTranslator>
#include <QWidget>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <deque>
#include <functional>
#include <limits>
#include <map>
#include <memory>
#include <set>
#include <tuple>
#include <utility>
#include <vector>

#include "boost/geometry/geometry.hpp"
#include "dbDescriptors.h"
#include "gui/gui.h"
#include "gui_utils.h"
#include "highlightGroupDialog.h"
#include "label.h"
#include "mainWindow.h"
#include "odb/db.h"
#include "odb/dbObject.h"
#include "odb/dbShape.h"
#include "odb/dbTransform.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "painter.h"
#include "ruler.h"
#include "scriptWidget.h"
#include "search.h"
#include "utl/Logger.h"
#include "utl/timer.h"

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

using odb::dbBlock;
using odb::dbBox;
using odb::dbInst;
using odb::dbMaster;
using odb::dbMPin;
using odb::dbMTerm;
using odb::dbOrientType;
using odb::dbPolygon;
using odb::dbRowDir;
using odb::dbSite;
using odb::dbTech;
using odb::dbTechLayer;
using odb::dbTechLayerType;
using odb::dbTransform;
using odb::Point;
using odb::Rect;
using utl::GUI;

LayoutViewer::LayoutViewer(
    Options* options,
    ScriptWidget* output_widget,
    const SelectionSet& selected,
    const HighlightSet& highlighted,
    const std::vector<std::unique_ptr<Ruler>>& rulers,
    const std::vector<std::unique_ptr<Label>>& labels,
    const std::map<odb::dbModule*, ModuleSettings>& module_settings,
    const std::set<odb::dbNet*>& focus_nets,
    const std::set<odb::dbNet*>& route_guides,
    const std::set<odb::dbNet*>& net_tracks,
    Gui* gui,
    const std::function<bool()>& using_dbu,
    const std::function<bool()>& show_ruler_as_euclidian,
    const std::function<bool()>& show_db_view,
    QWidget* parent)
    : QWidget(parent),
      chip_(nullptr),
      options_(options),
      output_widget_(output_widget),
      selected_(selected),
      highlighted_(highlighted),
      rulers_(rulers),
      labels_(labels),
      scroller_(nullptr),
      pixels_per_dbu_(1.0),
      fit_pixels_per_dbu_(1.0),
      min_depth_(0),
      max_depth_(99),
      rubber_band_showing_(false),
      is_view_dragging_(false),
      gui_(gui),
      using_dbu_(using_dbu),
      show_ruler_as_euclidian_(show_ruler_as_euclidian),
      show_db_view_(show_db_view),
      modules_(module_settings),
      building_ruler_(false),
      ruler_start_(nullptr),
      snap_edge_showing_(false),
      animate_selection_(nullptr),
      repaint_requested_(false),
      logger_(nullptr),
      layout_context_menu_(new QMenu(tr("Layout Menu"), this)),
      focus_nets_(focus_nets),
      route_guides_(route_guides),
      net_tracks_(net_tracks),
      viewer_thread_(this)
{
  setMouseTracking(true);

  addMenuAndActions();

  loading_timer_.setInterval(300 /*ms*/);

  connect(
      &viewer_thread_, &RenderThread::done, this, &LayoutViewer::updatePixmap);

  connect(&loading_timer_,
          &QTimer::timeout,
          this,
          &LayoutViewer::handleLoadingIndication);

  connect(&search_, &Search::modified, this, &LayoutViewer::fullRepaint);

  connect(&search_, &Search::newChip, this, &LayoutViewer::setChip);

  repaint_timer_.setSingleShot(true);
  repaint_timer_.callOnTimeout(this, &LayoutViewer::fullRepaint);
}

void LayoutViewer::handleLoadingIndication()
{
  if (!viewer_thread_.isRendering()) {
    loading_timer_.stop();
    return;
  }

  update();
}

void LayoutViewer::setLoadingState()
{
  loading_indicator_.clear();
  loading_timer_.start();
}

void LayoutViewer::setChip(odb::dbChip* chip)
{
  chip_ = chip;

  if (getBlock() && cut_maximum_size_.empty()) {
    generateCutLayerMaximumSizes();
  }

  updateScaleAndCentering(scroller_->maximumViewportSize());
  fit();
}

std::map<odb::dbChipInst*, odb::dbChip*> LayoutViewer::getChips() const
{
  std::map<odb::dbChipInst*, odb::dbChip*> chips;
  if (getChip() == nullptr) {
    return chips;
  }
  std::vector<odb::dbChip*> stack;

  chips[nullptr] = getChip();
  stack.push_back(getChip());
  while (!stack.empty()) {
    odb::dbChip* curr_chip = stack.back();
    stack.pop_back();

    for (auto chip_inst : curr_chip->getChipInsts()) {
      odb::dbChip* tmp_chip = chip_inst->getMasterChip();
      stack.push_back(tmp_chip);
      chips[chip_inst] = tmp_chip;
    }
  }
  return chips;
}

void LayoutViewer::setLogger(utl::Logger* logger)
{
  logger_ = logger;
  viewer_thread_.setLogger(logger);
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

odb::Point LayoutViewer::findNextRulerPoint(const odb::Point& mouse)
{
  const bool do_snap = !(qGuiApp->keyboardModifiers() & Qt::ControlModifier);
  if (ruler_start_ == nullptr) {
    return findNextSnapPoint(mouse, do_snap);
  }
  const bool do_any_snap = qGuiApp->keyboardModifiers() & Qt::ShiftModifier;
  if (do_any_snap) {
    return findNextSnapPoint(mouse, do_snap);
  }
  return findNextSnapPoint(mouse, *ruler_start_, do_snap);
}

Rect LayoutViewer::getBounds() const
{
  Rect bbox{0, 0, chip_->getWidth(), chip_->getHeight()};

  for (const auto it : getChips()) {
    auto chip_inst = it.first;
    auto chip = it.second;

    bbox.merge(chip_inst ? chip_inst->getBBox() : chip->getBBox());

    dbBlock* block = chip->getBlock();
    if (!block) {
      continue;
    }

    Rect die_area = block->getDieArea();
    if (chip_inst) {
      chip_inst->getTransform().apply(die_area);
    }
    bbox.merge(die_area);
  }
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

void LayoutViewer::setPixelsPerDBU(qreal target_pixels_per_dbu)
{
  if (!hasDesign()) {
    return;
  }

  bool scroll_bars_visible = scroller_->horizontalScrollBar()->isVisible()
                             || scroller_->verticalScrollBar()->isVisible();
  bool zoomed_out = pixels_per_dbu_ /*old*/ > target_pixels_per_dbu /*new*/;

  if (!scroll_bars_visible && zoomed_out) {
    return;
  }

  qreal current_viewer_x = this->size().width() / pixels_per_dbu_;
  qreal current_viewer_y = this->size().height() / pixels_per_dbu_;

  // ensure max size is not exceeded
  qreal maximum_pixels_per_dbu
      = 0.98
        * computePixelsPerDBU(QSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX),
                              Rect(0, 0, current_viewer_x, current_viewer_y));

  if (target_pixels_per_dbu >= maximum_pixels_per_dbu) {
    return;
  }

  const QSize new_size(ceil(current_viewer_x * target_pixels_per_dbu),
                       ceil(current_viewer_y * target_pixels_per_dbu));

  resize(new_size);
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

  if (!scroller_->isScrollingWithCursor()) {
    updateCursorCoordinates();
  }
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
  auto set_scroll_bar = [](QScrollBar* bar, int value) -> int {
    const int max_value = bar->maximum();
    const int min_value = bar->minimum();
    if (value > max_value) {
      bar->setValue(max_value);
      return max_value + bar->pageStep() / 2;
    }
    if (value < min_value) {
      bar->setValue(min_value);
      return bar->pageStep() / 2;
    }
    bar->setValue(value);
    return 0;
  };

  const int x_val = set_scroll_bar(scroller_->horizontalScrollBar(), pt.x());
  const int y_val = set_scroll_bar(scroller_->verticalScrollBar(), pt.y());
  // set the center now, since center is modified by the updateCenter
  // we only care of the focus point
  center_ = focus;

  // account for layout window edges from set_scroll_bar
  odb::Point adjusted_pt = screenToDBU(QPointF(x_val, y_val));
  if (x_val != 0) {
    center_.setX(adjusted_pt.x());
  }
  if (y_val != 0) {
    center_.setY(adjusted_pt.y());
  }
}

bool LayoutViewer::isCursorInsideViewport()
{
  QPoint mouse_pos = scroller_->mapFromGlobal(QCursor::pos());
  QRect layout_boundaries = scroller_->viewport()->rect();

  if (layout_boundaries.contains(mouse_pos)) {
    return true;
  }

  return false;
}

void LayoutViewer::updateCursorCoordinates()
{
  Point mouse = screenToDBU(mapFromGlobal(QCursor::pos()));
  emit location(mouse.x(), mouse.y());
}

void LayoutViewer::zoomIn()
{
  zoomIn(getVisibleCenter(), false);
}

void LayoutViewer::zoomIn(const odb::Point& focus, bool do_delta_focus)
{
  zoom(focus, 1 * kZoomScaleFactor, do_delta_focus);
}

void LayoutViewer::zoomOut()
{
  zoomOut(getVisibleCenter(), false);
}

void LayoutViewer::zoomOut(const odb::Point& focus, bool do_delta_focus)
{
  zoom(focus, 1 / kZoomScaleFactor, do_delta_focus);
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
  qreal pixels_per_DBU
      = computePixelsPerDBU(scroller_->maximumViewportSize(), rect_dbu);
  qreal pixels_per_DBU_with_margins
      = pixels_per_DBU / (1 + 2 * defaultZoomMargin);
  setPixelsPerDBU(pixels_per_DBU_with_margins);

  // center the layout at the middle of the rect
  centerAt(Point(rect_dbu.xMin() + rect_dbu.dx() / 2,
                 rect_dbu.yMin() + rect_dbu.dy() / 2));
}

void LayoutViewer::zoomTo(const odb::Point& focus, int diameter)
{
  odb::Point ref
      = odb::Point(focus.x() - (diameter / 2), focus.y() - (diameter / 2));
  zoomTo(odb::Rect(ref.x(), ref.y(), ref.x() + diameter, ref.y() + diameter));
}

int LayoutViewer::getVisibleDiameter()
{
  odb::Rect bounds = getVisibleBounds();
  // scrollbar
  int scrollBarWidth
      = std::ceil((qApp->style()->pixelMetric(QStyle::PM_ScrollBarExtent))
                  / pixels_per_dbu_);
  if (scroller_->verticalScrollBar()->isVisible()) {
    bounds.set_xhi(bounds.xMax() + scrollBarWidth);
  }
  if (scroller_->horizontalScrollBar()->isVisible()) {
    bounds.set_yhi(bounds.yMax() + scrollBarWidth);
  }

  // undo the margin
  const int smaller_side = std::min(bounds.dx(), bounds.dy());
  const int margin = std::ceil(smaller_side * 2 * defaultZoomMargin
                               / (1 + 2 * defaultZoomMargin));

  return std::min(bounds.dx(), bounds.dy()) - margin;
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
  const uint64_t lhs_length_sqrd
      = std::pow(lhs.first.x() - lhs.second.x(), 2)
        + std::pow(lhs.first.y() - lhs.second.y(), 2);
  const uint64_t rhs_length_sqrd
      = std::pow(rhs.first.x() - rhs.second.x(), 2)
        + std::pow(rhs.first.y() - rhs.second.y(), 2);

  return lhs_length_sqrd < rhs_length_sqrd;
}

void LayoutViewer::searchNearestViaEdge(
    dbBlock* block,
    dbTechLayer* cut_layer,
    dbTechLayer* search_layer,
    const Rect& search_line,
    const int shape_limit,
    const std::function<void(const Rect& rect)>& check_rect)
{
  auto via_shapes = search_.searchSNetViaShapes(block,
                                                cut_layer,
                                                search_line.xMin(),
                                                search_line.yMin(),
                                                search_line.xMax(),
                                                search_line.yMax(),
                                                shape_limit);
  std::vector<odb::dbShape> shapes;
  for (const auto& [sbox, net] : via_shapes) {
    if (isNetVisible(net)) {
      sbox->getViaLayerBoxes(search_layer, shapes);
      for (auto& shape : shapes) {
        check_rect(shape.getBox());
      }
    }
  }
}

std::pair<LayoutViewer::Edge, bool>
LayoutViewer::searchNearestEdge(odb::Point pt, bool horizontal, bool vertical)
{
  if (!hasDesign()) {
    return {Edge(), false};
  }

  const int search_radius = getDbuPerMicron();

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
  check_rect(chip_->getBBox());
  for (const auto it : getChips()) {
    auto chip_inst = it.first;
    auto chip = it.second;

    dbBlock* block = chip->getBlock();
    if (!block) {
      continue;
    }

    if (chip_inst) {
      // Convert to the chip_inst coordinates
      dbTransform inverse_transform = chip_inst->getTransform();
      inverse_transform.invert();
      inverse_transform.apply(search_line);
      inverse_transform.apply(pt);
      auto& [p1, p2] = closest_edge;
      inverse_transform.apply(p1);
      inverse_transform.apply(p2);
    }

    // get die bounding box
    check_rect(block->getDieArea());

    if (options_->areRegionsVisible()) {
      for (auto* region : block->getRegions()) {
        for (auto* box : region->getBoundaries()) {
          odb::Rect region_box = box->getBox();
          if (region_box.area() > 0) {
            check_rect(region_box);
          }
        }
      }
    }

    auto inst_range = search_.searchInsts(block,
                                          search_line.xMin(),
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
    for (auto* inst : inst_range) {
      if (options_->isInstanceVisible(inst)) {
        if (inst_internals_visible) {
          // only add inst if it can be used for pin or obs search
          insts.push_back(inst);
        }
        check_rect(inst->getBBox()->getBox());
      }
    }

    const int shape_limit = shapeSizeLimit();

    // look for edges in metal shapes
    dbTech* tech = block->getTech();
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
        const dbTransform inst_xfm = inst->getTransform();

        if (inst_osb_visible) {
          for (auto& box : inst_boxes->obs) {
            const QRect rect = box.boundingRect();
            odb::Rect trans_box(
                rect.left(), rect.bottom(), rect.right(), rect.top());
            inst_xfm.apply(trans_box);
            check_rect(trans_box);
          }
        }
        if (inst_pins_visible) {
          for (const auto& [mterm, boxes] : inst_boxes->mterms) {
            for (const auto& box : boxes) {
              const QRect rect = box.boundingRect();
              odb::Rect trans_box(
                  rect.left(), rect.bottom(), rect.right(), rect.top());
              inst_xfm.apply(trans_box);
              check_rect(trans_box);
            }
          }
        }
      }

      const bool routing_visible = options_->areRoutingSegmentsVisible();
      const bool vias_visible = options_->areRoutingViasVisible();
      const bool io_pins_visible = options_->areIOPinsVisible();
      if (routing_visible || vias_visible || io_pins_visible) {
        auto box_shapes = search_.searchBoxShapes(block,
                                                  layer,
                                                  search_line.xMin(),
                                                  search_line.yMin(),
                                                  search_line.xMax(),
                                                  search_line.yMax(),
                                                  shape_limit);
        for (const auto& [box, type, net] : box_shapes) {
          if (!routing_visible && type == Search::WIRE) {
            continue;
          }
          if (!vias_visible && type == Search::VIA) {
            continue;
          }
          if (!io_pins_visible && type == Search::BTERM) {
            continue;
          }
          if (isNetVisible(net)) {
            check_rect(box);
          }
        }
      }

      if (options_->areSpecialRoutingViasVisible()) {
        if (layer->getType() == dbTechLayerType::CUT) {
          searchNearestViaEdge(
              block, layer, layer, search_line, shape_limit, check_rect);
        } else {
          if (auto upper = layer->getUpperLayer()) {
            searchNearestViaEdge(
                block, upper, layer, search_line, shape_limit, check_rect);
          }
          if (auto lower = layer->getLowerLayer()) {
            searchNearestViaEdge(
                block, lower, layer, search_line, shape_limit, check_rect);
          }
        }
      }

      if (options_->areSpecialRoutingSegmentsVisible()) {
        auto polygon_shapes = search_.searchSNetShapes(block,
                                                       layer,
                                                       search_line.xMin(),
                                                       search_line.yMin(),
                                                       search_line.xMax(),
                                                       search_line.yMax(),
                                                       shape_limit);
        for (const auto& [box, poly, net] : polygon_shapes) {
          if (isNetVisible(net)) {
            check_rect(box->getBox());
          }
        }
      }

      if (options_->areFillsVisible()) {
        auto fills = search_.searchFills(block,
                                         layer,
                                         search_line.xMin(),
                                         search_line.yMin(),
                                         search_line.xMax(),
                                         search_line.yMax(),
                                         shape_limit);
        for (auto* fill : fills) {
          odb::Rect box;
          fill->getRect(box);
          check_rect(box);
        }
      }

      if (options_->areObstructionsVisible()) {
        auto obs = search_.searchObstructions(block,
                                              layer,
                                              search_line.xMin(),
                                              search_line.yMin(),
                                              search_line.xMax(),
                                              search_line.yMax(),
                                              shape_limit);
        for (auto* ob : obs) {
          check_rect(ob->getBBox()->getBox());
        }
      }
    }

    if (options_->areBlockagesVisible()) {
      auto blcks = search_.searchBlockages(block,
                                           search_line.xMin(),
                                           search_line.yMin(),
                                           search_line.xMax(),
                                           search_line.yMax(),
                                           shape_limit);
      for (auto* blck : blcks) {
        check_rect(blck->getBBox()->getBox());
      }
    }

    if (options_->areSitesVisible()) {
      for (const auto& [row, row_site, index] :
           getRowRects(block, search_line)) {
        odb::dbSite* site = nullptr;
        if (row->getObjectType() == odb::dbObjectType::dbSiteObj) {
          site = static_cast<odb::dbSite*>(row);
        } else {
          site = static_cast<odb::dbRow*>(row)->getSite();
        }
        if (options_->isSiteVisible(site) && options_->isSiteSelectable(site)) {
          check_rect(row_site);
        }
      }
    }

    if (chip_inst) {
      // Convert back to the global coordinates
      dbTransform transform = chip_inst->getTransform();
      transform.apply(search_line);
      transform.apply(pt);
      auto& [p1, p2] = closest_edge;
      transform.apply(p1);
      transform.apply(p2);
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

void LayoutViewer::selectViaShapesAt(dbBlock* block,
                                     dbTechLayer* cut_layer,
                                     dbTechLayer* select_layer,
                                     const Rect& region,
                                     const int shape_limit,
                                     std::vector<Selected>& selections)
{
  auto via_shapes = search_.searchSNetViaShapes(block,
                                                cut_layer,
                                                region.xMin(),
                                                region.yMin(),
                                                region.xMax(),
                                                region.yMax(),
                                                shape_limit);

  std::vector<odb::dbShape> shapes;
  for (const auto& [sbox, net] : via_shapes) {
    if (isNetVisible(net) && options_->isNetSelectable(net)) {
      sbox->getViaLayerBoxes(select_layer, shapes);
      for (auto& shape : shapes) {
        if (shape.getBox().intersects(region)) {
          selections.push_back(gui_->makeSelected(net));
        }
      }
    }
  }
}

void LayoutViewer::selectAt(odb::Rect region, std::vector<Selected>& selections)
{
  if (!hasDesign()) {
    return;
  }

  // Look for the selected object in reverse layer order
  auto& renderers = Gui::get()->renderers();
  dbTech* tech = getChip()->getTech();

  const int shape_limit = shapeSizeLimit();

  odb::dbBlock* block = getBlock();

  if (block && options_->areBlockagesVisible()
      && options_->areBlockagesSelectable()) {
    auto blockages = search_.searchBlockages(block,
                                             region.xMin(),
                                             region.yMin(),
                                             region.xMax(),
                                             region.yMax(),
                                             shape_limit);
    for (auto* blockage : blockages) {
      selections.push_back(gui_->makeSelected(blockage));
    }
  }

  // dbSet doesn't provide a reverse iterator so we have to copy it.
  std::deque<dbTechLayer*> rev_layers;
  if (tech) {
    for (auto layer : tech->getLayers()) {
      rev_layers.push_front(layer);
    }
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

    if (!block) {
      continue;
    }

    if (options_->areObstructionsVisible()
        && options_->areObstructionsSelectable()) {
      auto obs = search_.searchObstructions(block,
                                            layer,
                                            region.xMin(),
                                            region.yMin(),
                                            region.xMax(),
                                            region.yMax(),
                                            shape_limit);
      for (auto* ob : obs) {
        selections.push_back(gui_->makeSelected(ob));
      }
    }

    const bool routing_visible = options_->areRoutingSegmentsVisible();
    const bool vias_visible = options_->areRoutingViasVisible();
    const bool io_pins_visible = options_->areIOPinsVisible();
    if (routing_visible || vias_visible || io_pins_visible) {
      auto box_shapes = search_.searchBoxShapes(block,
                                                layer,
                                                region.xMin(),
                                                region.yMin(),
                                                region.xMax(),
                                                region.yMax(),
                                                shape_limit);

      for (auto& [box, type, net] : box_shapes) {
        if (!routing_visible && type == Search::WIRE) {
          continue;
        }
        if (!vias_visible && type == Search::VIA) {
          continue;
        }
        if (!io_pins_visible && type == Search::BTERM) {
          continue;
        }
        if (isNetVisible(net) && options_->isNetSelectable(net)) {
          selections.push_back(gui_->makeSelected(net));
        }
      }
    }

    if (options_->areSpecialRoutingViasVisible()) {
      if (layer->getType() == dbTechLayerType::CUT) {
        selectViaShapesAt(block, layer, layer, region, shape_limit, selections);
      } else {
        if (auto upper = layer->getUpperLayer()) {
          selectViaShapesAt(
              block, upper, layer, region, shape_limit, selections);
        }
        if (auto lower = layer->getLowerLayer()) {
          selectViaShapesAt(
              block, lower, layer, region, shape_limit, selections);
        }
      }
    }

    if (options_->areSpecialRoutingSegmentsVisible()) {
      auto polygon_shapes = search_.searchSNetShapes(block,
                                                     layer,
                                                     region.xMin(),
                                                     region.yMin(),
                                                     region.xMax(),
                                                     region.yMax(),
                                                     shape_limit);

      for (auto& [box, poly, net] : polygon_shapes) {
        if (isNetVisible(net) && options_->isNetSelectable(net)) {
          selections.push_back(gui_->makeSelected(net));
        }
      }
    }
  }

  // Check for objects not in a layer
  for (auto* renderer : renderers) {
    for (auto& selected : renderer->select(nullptr, region)) {
      selections.push_back(selected);
    }
  }

  if (block) {
    // Look for instances and ITerms
    auto insts = search_.searchInsts(block,
                                     region.xMin(),
                                     region.yMin(),
                                     region.xMax(),
                                     region.yMax(),
                                     instanceSizeLimit());

    for (auto* inst : insts) {
      if (options_->isInstanceVisible(inst)) {
        if (options_->isInstanceSelectable(inst)) {
          selections.push_back(gui_->makeSelected(inst));
        }
        if (options_->areInstancePinsVisible()
            && options_->areInstancePinsSelectable()) {
          const odb::dbTransform xform = inst->getTransform();
          for (const auto& [layer, boxes] : cell_boxes_[inst->getMaster()]) {
            if (options_->isVisible(layer) && options_->isSelectable(layer)) {
              for (const auto& [mterm, geoms] : boxes.mterms) {
                odb::dbITerm* iterm = inst->getITerm(mterm);
                for (const auto& geom : geoms) {
                  std::vector<odb::Point> points(geom.size());
                  for (const auto& pt : geom) {
                    points.emplace_back(pt.x(), pt.y());
                  }
                  odb::Polygon poly(points);
                  xform.apply(poly);
                  if (boost::geometry::intersects(poly, region)) {
                    selections.push_back(gui_->makeSelected(iterm));
                  }
                }
              }
            }
          }
        }
      }
    }

    // Look for BPins
    if (options_->areIOPinsVisible() && options_->areIOPinsSelectable()) {
      for (auto* layer : block->getTech()->getLayers()) {
        if (!options_->isVisible(layer)) {
          continue;
        }
        for (const auto& [box, bpin] : search_.searchBPins(block,
                                                           layer,
                                                           region.xMin(),
                                                           region.yMin(),
                                                           region.xMax(),
                                                           region.yMax(),
                                                           shape_limit)) {
          selections.push_back(gui_->makeSelected(bpin));
        }
      }
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

  if (options_->areLabelsVisible() && options_->areLabelsSelectable()) {
    for (auto& label : labels_) {
      if (label->getOutline().intersects(region)) {
        selections.push_back(gui_->makeSelected(label.get()));
      }
    }
  }

  if (block && options_->areRegionsVisible()
      && options_->areRegionsSelectable()) {
    for (auto db_region : block->getRegions()) {
      for (auto box : db_region->getBoundaries()) {
        if (box->getBox().intersects(region)) {
          selections.push_back(gui_->makeSelected(db_region));
        }
      }
    }
  }

  if (block && options_->areSitesVisible() && options_->areSitesSelectable()) {
    for (const auto& [row_obj, rect, index] : getRowRects(block, region)) {
      odb::dbSite* site = nullptr;
      if (row_obj->getObjectType() == odb::dbObjectType::dbSiteObj) {
        site = static_cast<odb::dbSite*>(row_obj);
      } else {
        site = static_cast<odb::dbRow*>(row_obj)->getSite();
      }

      if (!options_->isSiteVisible(site) || !options_->isSiteSelectable(site)) {
        continue;
      }

      if (row_obj->getObjectType() == odb::dbObjectType::dbRowObj) {
        selections.push_back(
            gui_->makeSelected(static_cast<odb::dbRow*>(row_obj)));
      } else {
        selections.push_back(gui_->makeSelected(
            DbSiteDescriptor::SpecificSite{site, rect, index}));
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

Selected LayoutViewer::selectAtPoint(const odb::Point& pt_dbu)
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
    // one that will emulate a circular queue so we don't just oscillate
    // between the first two
    std::vector<bool> is_selected;
    is_selected.reserve(selections.size());
    for (auto& sel : selections) {
      is_selected.push_back(selected_.contains(sel));
    }
    if (std::ranges::all_of(is_selected, [](bool b) { return b; })) {
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
  }
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

void LayoutViewer::mousePressEvent(QMouseEvent* event)
{
  if (!hasDesign() || !viewer_thread_.isFirstRenderDone()) {
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

  if (event->button() == Qt::MiddleButton && !rubber_band_showing_) {
    is_view_dragging_ = true;
    setCursor(Qt::ClosedHandCursor);
  } else {
    rubber_band_.setTopLeft(mouse_press_pos_);
    rubber_band_.setBottomRight(mouse_press_pos_);
  }
}

void LayoutViewer::mouseMoveEvent(QMouseEvent* event)
{
  if (!hasDesign() || !viewer_thread_.isFirstRenderDone()) {
    return;
  }

  mouse_move_pos_ = event->pos();

  // emit location in microns
  Point pt_dbu = screenToDBU(mouse_move_pos_);
  emit location(pt_dbu.x(), pt_dbu.y());

  if (is_view_dragging_) {
    QPoint dragging_delta = mouse_move_pos_ - mouse_press_pos_;

    scroller_->horizontalScrollBar()->setValue(
        scroller_->horizontalScrollBar()->value() - dragging_delta.x());
    scroller_->verticalScrollBar()->setValue(
        scroller_->verticalScrollBar()->value() - dragging_delta.y());
  }

  if (building_ruler_) {
    if (!(qGuiApp->keyboardModifiers() & Qt::ControlModifier)) {
      // set to false and toggle to true if edges are available
      snap_edge_showing_ = false;

      bool do_ver = true;
      bool do_hor = true;

      if (ruler_start_ != nullptr
          && !(qGuiApp->keyboardModifiers() & Qt::ShiftModifier)) {
        if (std::abs(ruler_start_->x() - pt_dbu.x())
            < std::abs(ruler_start_->y() - pt_dbu.y())) {
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
  if (!hasDesign() || !viewer_thread_.isFirstRenderDone()) {
    return;
  }

  if (event->button() == Qt::MiddleButton) {
    is_view_dragging_ = false;
    unsetCursor();
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
      if (!building_ruler_) {
        Point pt_dbu = screenToDBU(mouse_pos);
        Selected selection = selectAtPoint(pt_dbu);
        if (qGuiApp->keyboardModifiers() & Qt::ShiftModifier) {
          emit addSelected(selection);
        } else {
          emit selected(selection,
                        qGuiApp->keyboardModifiers() & Qt::ControlModifier);
        }
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
  if (hasDesign()) {
    updateScaleAndCentering(event->size());
  }
}

void LayoutViewer::updateScaleAndCentering(const QSize& new_size)
{
  if (hasDesign()) {
    const odb::Rect bounds = getBounds();

    // compute new pixels_per_dbu_
    pixels_per_dbu_ = computePixelsPerDBU(new_size, getPaddedRect(bounds));

    // expand area to fill whole scroller window
    const QSize new_area = new_size.expandedTo(scroller_->size());

    // Compute new centering shift - that is the offset necessary to center the
    // block in the viewport. We need to take into account not only the
    // dimensions (dx and dy) of the bounds but how far it is from the dbu
    // origin
    centering_shift_
        = QPoint(((new_area.width() - bounds.dx() * pixels_per_dbu_) / 2
                  - bounds.xMin() * pixels_per_dbu_),
                 ((new_area.height() + bounds.dy() * pixels_per_dbu_) / 2
                  + bounds.yMin() * pixels_per_dbu_));

    fullRepaint();
  }
}

// Cache the boxes for shapes in obs/mterm by layer per master for
// drawing performance
void LayoutViewer::boxesByLayer(dbMaster* master, LayerBoxes& boxes)
{
  const bool is_db_view = show_db_view_();
  auto box_to_qpolygon = [](odb::dbBox* box) -> QPolygon {
    QPolygon poly;
    for (const auto& pt : box->getBox().getPoints()) {
      poly.append(QPoint(pt.x(), pt.y()));
    }
    return poly;
  };
  auto pbox_to_qpolygon = [](odb::dbPolygon* box) -> QPolygon {
    QPolygon poly;
    for (const auto& pt : box->getPolygon().getPoints()) {
      poly.append(QPoint(pt.x(), pt.y()));
    }
    return poly;
  };

  // store obstructions
  if (!is_db_view) {
    for (dbPolygon* box : master->getPolygonObstructions()) {
      dbTechLayer* layer = box->getTechLayer();
      boxes[layer].obs.emplace_back(pbox_to_qpolygon(box));
    }
    for (dbBox* box : master->getObstructions(false)) {
      dbTechLayer* layer = box->getTechLayer();
      boxes[layer].obs.emplace_back(box_to_qpolygon(box));
    }
  } else {
    for (dbBox* box : master->getObstructions()) {
      dbTechLayer* layer = box->getTechLayer();
      boxes[layer].obs.emplace_back(box_to_qpolygon(box));
    }
  }

  // store mterms
  for (dbMTerm* mterm : master->getMTerms()) {
    for (dbMPin* mpin : mterm->getMPins()) {
      if (!is_db_view) {
        for (dbPolygon* box : mpin->getPolygonGeometry()) {
          dbTechLayer* layer = box->getTechLayer();
          boxes[layer].mterms[mterm].emplace_back(pbox_to_qpolygon(box));
        }
        for (dbBox* box : mpin->getGeometry(false)) {
          dbTechLayer* layer = box->getTechLayer();
          boxes[layer].mterms[mterm].emplace_back(box_to_qpolygon(box));
        }
      }
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
            via_transform.apply(box_rect);
            boxes[layer].mterms[mterm].emplace_back(
                QRect{box_rect.xMin(),
                      box_rect.yMin(),
                      box_rect.xMax() - box_rect.xMin(),
                      box_rect.yMax() - box_rect.yMin()});
          }
        } else if (is_db_view) {
          odb::Rect box_rect = box->getBox();
          dbTechLayer* layer = box->getTechLayer();
          boxes[layer].mterms[mterm].emplace_back(
              QRect{box_rect.xMin(),
                    box_rect.yMin(),
                    box_rect.xMax() - box_rect.xMin(),
                    box_rect.yMax() - box_rect.yMin()});
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

std::vector<std::tuple<odb::dbObject*, odb::Rect, int>>
LayoutViewer::getRowRects(odb::dbBlock* block, const odb::Rect& bounds)
{
  const int min_resolution_site = nominalViewableResolution();
  int min_resolution_row = min_resolution_site;
  if (options_->isDetailedVisibility()) {
    // we only do this for the row as the sites can be too
    // numerous and small to draw even in detailed mode
    min_resolution_row = 0;
  }

  auto rows = search_.searchRows(block,
                                 bounds.xMin(),
                                 bounds.yMin(),
                                 bounds.xMax(),
                                 bounds.yMax(),
                                 min_resolution_row);

  std::vector<std::tuple<odb::dbObject*, odb::Rect, int>> rects;
  for (auto& [box, row] : rows) {
    odb::Point pt = row->getOrigin();

    rects.emplace_back(row, row->getBBox(), 0);

    dbSite* site = row->getSite();
    int spacing = row->getSpacing();
    int w = site->getWidth();
    int h = site->getHeight();

    bool w_visible = w >= min_resolution_site;
    bool h_visible = h >= min_resolution_row;

    switch (row->getOrient().getValue()) {
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
        const Rect row_rect(pt.x(), pt.y(), pt.x() + w, pt.y() + h);
        if (row_rect.intersects(bounds)) {
          // only paint rows that can be seen
          rects.emplace_back(obj, row_rect, i);
        }

        if (dir == dbRowDir::HORIZONTAL) {
          pt.addX(spacing);
        } else {
          pt.addY(spacing);
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

    const qint64 max_animate_time = QDateTime::currentMSecsSinceEpoch()
                                    + (animate_selection_->max_state_count + 2)
                                          * (qint64) update_interval;
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

odb::Point LayoutViewer::screenToDBU(const QPointF& point) const
{
  // Flip the y-coordinate (see file level comments)
  return Point((point.x() - centering_shift_.x()) / pixels_per_dbu_,
               (centering_shift_.y() - point.y()) / pixels_per_dbu_);
}

Rect LayoutViewer::screenToDBU(const QRectF& screen_rect) const
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

QPointF LayoutViewer::dbuToScreen(const Point& dbu_point) const
{
  // Flip the y-coordinate (see file level comments)
  qreal x = centering_shift_.x() + dbu_point.x() * pixels_per_dbu_;
  qreal y = centering_shift_.y() - dbu_point.y() * pixels_per_dbu_;

  return QPointF(x, y);
}

QRectF LayoutViewer::dbuToScreen(const Rect& dbu_rect) const
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

void LayoutViewer::drawScaleBar(QPainter* painter, const QRect& rect)
{
  if (!options_->isScaleBarVisible()) {
    return;
  }

  const qreal pixels_per_mircon = pixels_per_dbu_ * getDbuPerMicron();
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
  if (using_dbu_()) {
    scale_unit = getDbuPerMicron();
    unit_text = "";
  } else {
    if (bar_size > 1000) {
      scale_unit = 0.001;
      unit_text = "mm";
    } else if (bar_size > 1) {
      scale_unit = 1;
      unit_text = "μm";
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
      QPointF p2 = p1 - QPointF(0, bar_height / 2.0);

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
                     0,
                     3 * bar_height / 4.0);  // make this peg a little taller
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

void LayoutViewer::updatePixmap(const QImage& image, const QRect& bounds)
{
  draw_pixmap_ = QPixmap::fromImage(image);
  draw_pixmap_bounds_ = bounds;
  update();
}

void LayoutViewer::drawLoadingIndicator(QPainter* painter, const QRect& bounds)
{
  const QRect background = computeIndicatorBackground(painter, bounds);
  const QColor background_color = options_->background();

  painter->fillRect(background, background_color);  // to help visualize

  const QColor text_color(255 - background_color.red(),
                          255 - background_color.green(),
                          255 - background_color.blue());

  painter->setPen(QPen(text_color, 2));
  painter->drawText(background.left(),
                    background.bottom(),
                    QString::fromStdString(loading_indicator_));

  if (loading_indicator_.size() == 3) {
    loading_indicator_.clear();
    return;
  }

  loading_indicator_ += ".";
}

QRect LayoutViewer::computeIndicatorBackground(QPainter* painter,
                                               const QRect& bounds) const
{
  painter->setFont(painter->font());

  QFontMetrics font_metrics(painter->font());

  const QRect rect
      = font_metrics.boundingRect(QString::fromStdString(loading_indicator_));
  const QRect background(bounds.left() + 2 /* px */,
                         bounds.top() + 2 /* px */,
                         rect.width(),
                         rect.height() / 2);

  return background;
}

int LayoutViewer::getDbuPerMicron() const
{
  return getChip()->getDb()->getDbuPerMicron();
}

void LayoutViewer::paintEvent(QPaintEvent* event)
{
  if (!hasDesign()) {
    return;
  }

  if (draw_pixmap_.isNull()) {
    return;
  }

  QPainter painter(this);
  painter.setRenderHints(QPainter::Antialiasing);

  painter.drawPixmap(event->rect().topLeft(), draw_pixmap_);

  if (!viewer_thread_.isFirstRenderDone()) {
    return;
  }

  const QRect draw_bounds = event->rect();

  GuiPainter gui_painter(&painter,
                         options_,
                         screenToDBU(draw_bounds),
                         pixels_per_dbu_,
                         getDbuPerMicron());

  // update label outlines
  for (const auto& label : labels_) {
    painter.save();
    const auto size = label->getSize();
    QFont font = options_->labelFont();
    if (size) {
      font.setPixelSize(size.value());
    }
    painter.setFont(font);
    label->setOutline(gui_painter.stringBoundaries(label->getPt().x(),
                                                   label->getPt().y(),
                                                   label->getAnchor(),
                                                   label->getText()));
    painter.restore();
  }

  if (viewer_thread_.isRendering()) {
    drawLoadingIndicator(&painter, draw_bounds);
  } else {
    // erase indicator
    const QRect background = computeIndicatorBackground(&painter, draw_bounds);
    painter.fillRect(background, Qt::transparent);
  }

  if (rubber_band_showing_) {
    painter.setPen(QPen(Qt::white, 0));
    painter.setBrush(QBrush());
    painter.drawRect(rubber_band_.normalized());
  }

  // buffer outputs during paint to prevent recursive calls
  output_widget_->bufferOutputs(true);

  drawScaleBar(&painter, draw_bounds);

  painter.translate(centering_shift_);
  painter.scale(pixels_per_dbu_, -pixels_per_dbu_);

  if (animate_selection_ != nullptr) {
    auto brush = Painter::kTransparent;

    const int pen_width
        = animate_selection_->state_count % animate_selection_->state_modulo
          + 1;
    if (pen_width == 1) {
      // flash with brush, since pen width is the same as normal
      brush = Painter::kHighlight;
      brush.a = 100;
    }

    odb::Rect bbox;
    bool draw_hightlight = true;
    if (animate_selection_->selection.getBBox(bbox)) {
      const int size = bbox.maxDXDY();

      const int min_size = highlightSizeLimit();

      if (size < min_size) {
        draw_hightlight = false;

        const int half_size = min_size / 2.0;
        const int bloat_by = half_size - size;

        odb::Rect draw_rect;
        bbox.bloat(bloat_by, draw_rect);
        gui_painter.setPen(Painter::kHighlight, true, pen_width);
        gui_painter.setBrush(brush, Painter::Brush::kSolid);

        gui_painter.drawRect(draw_rect, 0, 0);
      }
    }

    if (draw_hightlight) {
      animate_selection_->selection.highlight(
          gui_painter, Painter::kHighlight, pen_width, brush);
    }
  }

  // draw partial ruler if present
  if (building_ruler_ && ruler_start_ != nullptr) {
    odb::Point snapped_mouse_pos
        = findNextRulerPoint(screenToDBU(mouse_move_pos_));
    gui_painter.drawRuler(ruler_start_->x(),
                          ruler_start_->y(),
                          snapped_mouse_pos.x(),
                          snapped_mouse_pos.y(),
                          show_ruler_as_euclidian_());
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

  // painting is done, okay to update outputs again
  output_widget_->bufferOutputs(false);
}

void LayoutViewer::fullRepaint()
{
  if (command_executing_ && !paused_) {
    repaint_timer_.start(5 /* ms */);
    return;
  }

  update();
  if (hasDesign()) {
    QRect rect = scroller_->viewport()->geometry();
    rect.translate(scroller_->horizontalScrollBar()->value(),
                   scroller_->verticalScrollBar()->value());
    setLoadingState();
    viewer_thread_.render(rect, selected_, highlighted_, rulers_, labels_);
  }

  emit(viewUpdated());
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

void LayoutViewer::selectHighlightConnectedBufferTrees(bool select_flag,
                                                       int highlight_group)
{
  Gui::get()->selectHighlightConnectedBufferTrees(select_flag, highlight_group);
}

void LayoutViewer::updateContextMenuItems()
{
  if (Gui::get()->anyObjectInSet(true /*selection set*/, odb::dbInstObj)
      == false)  // No Instance in selected set
  {
    menu_actions_[kSelectOutputNetsAct]->setDisabled(true);
    menu_actions_[kSelectInputNetsAct]->setDisabled(true);
    menu_actions_[kSelectAllNetsAct]->setDisabled(true);
    menu_actions_[kSelectAllBufferTreesAct]->setDisabled(true);

    menu_actions_[kHighlightOutputNetsAct]->setDisabled(true);
    menu_actions_[kHighlightInputNetsAct]->setDisabled(true);
    menu_actions_[kHighlightAllNetsAct]->setDisabled(true);
    highlight_color_menu_->setDisabled(true);
  } else {
    menu_actions_[kSelectOutputNetsAct]->setDisabled(false);
    menu_actions_[kSelectInputNetsAct]->setDisabled(false);
    menu_actions_[kSelectAllNetsAct]->setDisabled(false);
    menu_actions_[kSelectAllBufferTreesAct]->setDisabled(false);

    menu_actions_[kHighlightOutputNetsAct]->setDisabled(false);
    menu_actions_[kHighlightInputNetsAct]->setDisabled(false);
    menu_actions_[kHighlightAllNetsAct]->setDisabled(false);
    highlight_color_menu_->setDisabled(false);
  }

  if (Gui::get()->anyObjectInSet(true, odb::dbNetObj)
      == false) {  // No Net in selected set
    menu_actions_[kSelectConnectedInstAct]->setDisabled(true);
    menu_actions_[kHighlightConnectedInstAct]->setDisabled(true);
  } else {
    menu_actions_[kSelectConnectedInstAct]->setDisabled(false);
    menu_actions_[kHighlightConnectedInstAct]->setDisabled(false);
  }
}

void LayoutViewer::showLayoutCustomMenu(QPoint pos)
{
  updateContextMenuItems();
  layout_context_menu_->popup(this->mapToGlobal(pos));
}

void LayoutViewer::chipLoaded(odb::dbChip* chip)
{
  search_.setTopChip(chip);
}

void LayoutViewer::setScroller(LayoutScroll* scroller)
{
  scroller_ = scroller;

  // ensure changes in the scroll area are announced to the layout viewer
  connect(scroller_,
          &LayoutScroll::viewportChanged,
          this,
          &LayoutViewer::viewportUpdated);
  connect(scroller_,
          &LayoutScroll::centerChanged,
          this,
          &LayoutViewer::updateCenter);
  connect(scroller_,
          &LayoutScroll::centerChanged,
          this,
          &LayoutViewer::fullRepaint);
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
  fullRepaint();
}

QImage LayoutViewer::createImage(const Rect& region,
                                 int width_px,
                                 double dbu_per_pixel)
{
  if (!hasDesign()) {
    return QImage();
  }

  Rect save_area = region;
  if (region.dx() == 0 || region.dy() == 0) {
    // default to just that is currently visible
    save_area = screenToDBU(visibleRegion().boundingRect());
  }

  const qreal old_pixels_per_dbu = pixels_per_dbu_;

  if (width_px != 0) {
    // Adapt resolution to width entered by user
    pixels_per_dbu_ = width_px / static_cast<double>(save_area.dx());
  }

  if (dbu_per_pixel != 0) {
    pixels_per_dbu_ = 1.0 / dbu_per_pixel;
  }

  // convert back to pixels based on new resolution
  const QRectF screen_region = dbuToScreen(save_area);
  const QRegion save_region = QRegion(screen_region.left(),
                                      screen_region.top(),
                                      screen_region.width(),
                                      screen_region.height());

  QRect bounding_rect = save_region.boundingRect();

  // We don't use Utils::renderImage as we need to have the
  // rendering be synchronous.  We directly call the draw()
  // method ourselves.

  const QSize initial_size
      = QSize(bounding_rect.width(), bounding_rect.height());
  const QSize img_size = Utils::adjustMaxImageSize(initial_size);

  if (img_size != initial_size) {
    logger_->warn(utl::GUI,
                  94,
                  "Resolution results in illegal size (max width/height "
                  "is {} pixels). Saving image with dimensions = {} x {}.",
                  Utils::kMaxImageSize,
                  img_size.width(),
                  img_size.height());

    // Set resolution according to adjusted size
    pixels_per_dbu_ = computePixelsPerDBU(img_size, save_area);

    const QRectF adjuted_screen_region = dbuToScreen(save_area);
    const QRegion adjusted_save_region
        = QRegion(adjuted_screen_region.left(),
                  adjuted_screen_region.top(),
                  adjuted_screen_region.width(),
                  adjuted_screen_region.height());

    bounding_rect = adjusted_save_region.boundingRect();
  }

  QImage img(img_size, QImage::Format_ARGB32_Premultiplied);

  const qreal render_ratio
      = static_cast<qreal>(std::max(img_size.width(), img_size.height()))
        / std::max(bounding_rect.width(), bounding_rect.height());

  viewer_thread_.draw(img,
                      bounding_rect,
                      selected_,
                      highlighted_,
                      rulers_,
                      labels_,
                      render_ratio,
                      options_->background());
  pixels_per_dbu_ = old_pixels_per_dbu;

  return img;
}

void LayoutViewer::saveImage(const QString& filepath,
                             const Rect& region,
                             int width_px,
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

  QImage img = createImage(region, width_px, dbu_per_pixel);

  if (!img.save(save_filepath)) {
    logger_->warn(
        utl::GUI, 78, "Failed to write image: {}", save_filepath.toStdString());
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
  menu_actions_[kSelectConnectedInstAct]
      = select_menu->addAction(tr("Connected Insts"));
  menu_actions_[kSelectOutputNetsAct]
      = select_menu->addAction(tr("Output Nets"));
  menu_actions_[kSelectInputNetsAct] = select_menu->addAction(tr("Input Nets"));
  menu_actions_[kSelectAllNetsAct] = select_menu->addAction(tr("All Nets"));
  menu_actions_[kSelectAllBufferTreesAct]
      = select_menu->addAction(tr("All buffer trees"));

  // Highlight Actions
  menu_actions_[kHighlightConnectedInstAct]
      = highlight_menu->addAction(tr("Connected Insts"));
  menu_actions_[kHighlightOutputNetsAct]
      = highlight_menu->addAction(tr("Output Nets"));
  menu_actions_[kHighlightInputNetsAct]
      = highlight_menu->addAction(tr("Input Nets"));
  menu_actions_[kHighlightAllNetsAct]
      = highlight_menu->addAction(tr("All Nets"));

  highlight_color_menu_ = highlight_menu->addMenu(tr("All buffer trees"));
  menu_actions_[kHighlightAllBufferTreesAct0]
      = highlight_color_menu_->addAction(tr("green"));
  menu_actions_[kHighlightAllBufferTreesAct1]
      = highlight_color_menu_->addAction(tr("yellow"));
  menu_actions_[kHighlightAllBufferTreesAct2]
      = highlight_color_menu_->addAction(tr("cyan"));
  menu_actions_[kHighlightAllBufferTreesAct3]
      = highlight_color_menu_->addAction(tr("magenta"));
  menu_actions_[kHighlightAllBufferTreesAct4]
      = highlight_color_menu_->addAction(tr("red"));
  menu_actions_[kHighlightAllBufferTreesAct5]
      = highlight_color_menu_->addAction(tr("dark_green"));
  menu_actions_[kHighlightAllBufferTreesAct6]
      = highlight_color_menu_->addAction(tr("dark_magenta"));
  menu_actions_[kHighlightAllBufferTreesAct7]
      = highlight_color_menu_->addAction(tr("blue"));

  // for { highlightColor : Painter::highlightColors[highlight_group]} {
  //   menu_actions_[kHighlightAllBufferTreesAct7]
  //       = highlight_color_menu_->addAction(tr("blue"));
  // }

  // View Actions
  menu_actions_[kViewZoominAct] = view_menu->addAction(tr("Zoom In"));
  menu_actions_[kViewZoomoutAct] = view_menu->addAction(tr("Zoom Out"));
  menu_actions_[kViewZoomfitAct] = view_menu->addAction(tr("Fit"));

  // Save actions
  menu_actions_[kSaveVisibleImageAct]
      = save_menu->addAction(tr("Visible layout"));
  menu_actions_[kSaveWholeImageAct] = save_menu->addAction(tr("Entire layout"));

  // Clear Actions
  menu_actions_[kClearSelectionsAct] = clear_menu->addAction(tr("Selections"));
  menu_actions_[kClearHighlightsAct] = clear_menu->addAction(tr("Highlights"));
  menu_actions_[kClearRulersAct] = clear_menu->addAction(tr("Rulers"));
  menu_actions_[kClearLabelsAct] = clear_menu->addAction(tr("Labels"));
  menu_actions_[kClearFocusAct] = clear_menu->addAction(tr("Focus nets"));
  menu_actions_[kClearGuidesAct] = clear_menu->addAction(tr("Route Guides"));
  menu_actions_[kClearNetTracksAct] = clear_menu->addAction(tr("Net Tracks"));
  menu_actions_[kClearAllAct] = clear_menu->addAction(tr("All"));

  // Connect Slots to Actions...
  connect(menu_actions_[kSelectConnectedInstAct],
          &QAction::triggered,
          [this]() { selectHighlightConnectedInst(true); });
  connect(menu_actions_[kSelectOutputNetsAct], &QAction::triggered, [this]() {
    selectHighlightConnectedNets(true, true, false);
  });
  connect(menu_actions_[kSelectInputNetsAct], &QAction::triggered, [this]() {
    selectHighlightConnectedNets(true, false, true);
  });
  connect(menu_actions_[kSelectAllNetsAct], &QAction::triggered, [this]() {
    selectHighlightConnectedNets(true, true, true);
  });
  connect(menu_actions_[kSelectAllBufferTreesAct],
          &QAction::triggered,
          [this]() { selectHighlightConnectedBufferTrees(true); });

  connect(menu_actions_[kHighlightConnectedInstAct],
          &QAction::triggered,
          [this]() { selectHighlightConnectedInst(false); });
  connect(menu_actions_[kHighlightOutputNetsAct],
          &QAction::triggered,
          [this]() { selectHighlightConnectedNets(false, true, false); });
  connect(menu_actions_[kHighlightInputNetsAct], &QAction::triggered, [this]() {
    this->selectHighlightConnectedNets(false, false, true);
  });
  connect(menu_actions_[kHighlightAllNetsAct], &QAction::triggered, [this]() {
    selectHighlightConnectedNets(false, true, true);
  });
  connect(menu_actions_[kHighlightAllBufferTreesAct0],
          &QAction::triggered,
          [this]() { selectHighlightConnectedBufferTrees(false, 0); });
  connect(menu_actions_[kHighlightAllBufferTreesAct1],
          &QAction::triggered,
          [this]() { selectHighlightConnectedBufferTrees(false, 1); });
  connect(menu_actions_[kHighlightAllBufferTreesAct2],
          &QAction::triggered,
          [this]() { selectHighlightConnectedBufferTrees(false, 2); });
  connect(menu_actions_[kHighlightAllBufferTreesAct3],
          &QAction::triggered,
          [this]() { selectHighlightConnectedBufferTrees(false, 3); });

  connect(menu_actions_[kViewZoominAct], &QAction::triggered, [this]() {
    zoomIn();
  });
  connect(menu_actions_[kViewZoomoutAct], &QAction::triggered, [this]() {
    zoomOut();
  });
  connect(
      menu_actions_[kViewZoomfitAct], &QAction::triggered, [this]() { fit(); });

  connect(menu_actions_[kSaveVisibleImageAct], &QAction::triggered, [this]() {
    saveImage("");
  });
  connect(menu_actions_[kSaveWholeImageAct], &QAction::triggered, [this]() {
    const QSize whole_size = size();
    saveImage(
        "", screenToDBU(QRectF(0, 0, whole_size.width(), whole_size.height())));
  });

  connect(menu_actions_[kClearSelectionsAct], &QAction::triggered, this, []() {
    Gui::get()->clearSelections();
  });
  connect(menu_actions_[kClearHighlightsAct], &QAction::triggered, this, []() {
    Gui::get()->clearHighlights(-1);
  });
  connect(menu_actions_[kClearRulersAct], &QAction::triggered, this, []() {
    Gui::get()->clearRulers();
  });
  connect(menu_actions_[kClearLabelsAct], &QAction::triggered, this, []() {
    Gui::get()->clearLabels();
  });
  connect(menu_actions_[kClearFocusAct], &QAction::triggered, this, []() {
    Gui::get()->clearFocusNets();
  });
  connect(menu_actions_[kClearGuidesAct], &QAction::triggered, this, []() {
    Gui::get()->clearRouteGuides();
  });
  connect(menu_actions_[kClearNetTracksAct], &QAction::triggered, this, []() {
    Gui::get()->clearNetTracks();
  });
  connect(menu_actions_[kClearAllAct], &QAction::triggered, [this]() {
    menu_actions_[kClearSelectionsAct]->trigger();
    menu_actions_[kClearHighlightsAct]->trigger();
    menu_actions_[kClearRulersAct]->trigger();
    menu_actions_[kClearLabelsAct]->trigger();
    menu_actions_[kClearFocusAct]->trigger();
    menu_actions_[kClearGuidesAct]->trigger();
    menu_actions_[kClearNetTracksAct]->trigger();
  });
}

void LayoutViewer::restoreTclCommands(std::vector<std::string>& cmds)
{
  cmds.push_back(fmt::format("gui::set_resolution {}", 1.0 / pixels_per_dbu_));

  if (getBlock() != nullptr) {
    const double dbu_per_micron = getDbuPerMicron();

    cmds.push_back(fmt::format("gui::center_at {} {}",
                               center_.x() / dbu_per_micron,
                               center_.y() / dbu_per_micron));
  }
}

bool LayoutViewer::hasDesign() const
{
  if (chip_ == nullptr) {
    return false;
  }

  const Rect bounds = getBounds();
  if (bounds.dx() == 0 || bounds.dy() == 0) {
    return false;
  }

  return true;
}

bool LayoutViewer::isNetVisible(odb::dbNet* net)
{
  bool focus_visible = true;
  if (!focus_nets_.empty()) {
    focus_visible = focus_nets_.find(net) != focus_nets_.end();
  }

  return focus_visible && options_->isNetVisible(net);
}

void LayoutViewer::generateCutLayerMaximumSizes()
{
  if (!hasDesign()) {
    return;
  }

  dbTech* tech = getBlock()->getTech();
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
        getBlock()->getMasters(masters);
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
      debugPrint(logger_,
                 GUI,
                 "cut_size",
                 1,
                 "Cut size for layer {} is {}",
                 layer->getName(),
                 width);
    }
  }
}

int LayoutViewer::instanceSizeLimit() const
{
  if (options_->isDetailedVisibility() || options_->isModuleView()) {
    return 0;
  }

  return fineViewableResolution();
}

int LayoutViewer::shapeSizeLimit() const
{
  if (options_->isDetailedVisibility()) {
    return fineViewableResolution();
  }

  return nominalViewableResolution();
}

int LayoutViewer::highlightSizeLimit() const
{
  return coarseViewableResolution();
}

int LayoutViewer::fineViewableResolution() const
{
  return 1.0 / pixels_per_dbu_;
}

int LayoutViewer::nominalViewableResolution() const
{
  return 5.0 / pixels_per_dbu_;
}

int LayoutViewer::coarseViewableResolution() const
{
  return 10.0 / pixels_per_dbu_;
}

void LayoutViewer::exit()
{
  viewer_thread_.exit();
  while (viewer_thread_.isRunning() && !viewer_thread_.isFinished()) {
    // wait for it to be done
  }
}

void LayoutViewer::commandAboutToExecute()
{
  command_executing_ = true;
  paused_ = false;
}

void LayoutViewer::commandFinishedExecuting()
{
  command_executing_ = false;
  update();
}

void LayoutViewer::executionPaused()
{
  paused_ = true;
}

void LayoutViewer::resetCache()
{
  cell_boxes_.clear();
  fullRepaint();
}

////// LayoutScroll ///////
LayoutScroll::LayoutScroll(
    LayoutViewer* viewer,
    const std::function<bool()>& default_mouse_wheel_zoom,
    const std::function<int()>& arrow_keys_scroll_step,
    QWidget* parent)
    : QScrollArea(parent),
      default_mouse_wheel_zoom_(default_mouse_wheel_zoom),
      arrow_keys_scroll_step_(arrow_keys_scroll_step),
      viewer_(viewer),
      scrolling_with_cursor_(false)
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

// Handles zoom in/out on ctrl-wheel when option mouse_wheel_zoom is not set and
// vice-versa
void LayoutScroll::wheelEvent(QWheelEvent* event)
{
  if (default_mouse_wheel_zoom_()
      == event->modifiers().testFlag(Qt::ControlModifier)) {
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
  // ensure changes are processed before the next wheel event to prevent
  // zoomIn and Out from jumping around on the ScrollBars
  QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
}

bool LayoutScroll::eventFilter(QObject* object, QEvent* event)
{
  if (event->type() == QEvent::MouseButtonPress) {
    QMouseEvent* press_event = static_cast<QMouseEvent*>(event);

    if (press_event->button() == Qt::LeftButton) {
      if (object == this->horizontalScrollBar()
          || object == this->verticalScrollBar()) {
        scrolling_with_cursor_ = true;
      }
    }
  }

  if (event->type() == QEvent::MouseButtonRelease) {
    QMouseEvent* release_event = static_cast<QMouseEvent*>(event);

    if (release_event->button() == Qt::LeftButton && scrolling_with_cursor_) {
      scrolling_with_cursor_ = false;

      // handle the case in which a user might click on one of the
      // scrollbars, hold the button and move the cursor away
      if (viewer_->isCursorInsideViewport()) {
        viewer_->updateCursorCoordinates();
      }
    }
  }

  return QScrollArea::eventFilter(object, event);
}

void LayoutScroll::keyPressEvent(QKeyEvent* event)
{
  switch (event->key()) {
    case Qt::Key_Up:
      verticalScrollBar()->setValue(verticalScrollBar()->value()
                                    - arrow_keys_scroll_step_());
      break;
    case Qt::Key_Down:
      verticalScrollBar()->setValue(verticalScrollBar()->value()
                                    + arrow_keys_scroll_step_());
      break;
    case Qt::Key_Left:
      horizontalScrollBar()->setValue(horizontalScrollBar()->value()
                                      - arrow_keys_scroll_step_());
      break;
    case Qt::Key_Right:
      horizontalScrollBar()->setValue(horizontalScrollBar()->value()
                                      + arrow_keys_scroll_step_());
      break;
    default:
      QScrollArea::keyPressEvent(event);
  }
}

bool LayoutScroll::isScrollingWithCursor()
{
  return scrolling_with_cursor_;
}

}  // namespace gui
