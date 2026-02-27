// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#include "renderThread.h"

#include <QColor>
#include <QMutexLocker>
#include <QPainter>
#include <QPainterPath>
#include <QPolygon>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <exception>
#include <iterator>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "absl/synchronization/mutex.h"
#include "boost/geometry/geometry.hpp"
#include "boost/geometry/index/parameters.hpp"
#include "boost/geometry/index/predicates.hpp"
#include "boost/geometry/index/rtree.hpp"
#include "gui/gui.h"
#include "label.h"
#include "layoutViewer.h"
#include "odb/db.h"
#include "odb/dbObject.h"
#include "odb/dbShape.h"
#include "odb/dbTransform.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "painter.h"
#include "ruler.h"
#include "utl/Logger.h"
#include "utl/timer.h"

namespace gui {

namespace bg = boost::geometry;

using odb::dbBlock;
using odb::dbChip;
using odb::dbInst;
using odb::dbMaster;
using odb::dbOrientType;
using odb::dbTech;
using odb::dbTechLayer;
using odb::dbTechLayerDir;
using odb::dbTechLayerType;
using odb::dbTrackGrid;
using odb::dbTransform;
using odb::Point;
using odb::Rect;

using utl::GUI;

RenderThread::RenderThread(LayoutViewer* viewer) : viewer_(viewer)
{
}

void RenderThread::exit()
{
  mutex_.lock();
  abort_ = true;
  condition_.wakeOne();
  mutex_.unlock();

  wait();
}

void RenderThread::setLogger(utl::Logger* logger)
{
  logger_ = logger;
}

// Inspiration taken from the Qt mandelbrot example
void RenderThread::render(const QRect& draw_rect,
                          const SelectionSet& selected,
                          const HighlightSet& highlighted,
                          const Rulers& rulers,
                          const Labels& labels)
{
  if (abort_) {
    return;
  }

  is_rendering_ = true;

  QMutexLocker locker(&mutex_);
  draw_rect_ = draw_rect;
  selected_ = selected;
  highlighted_ = highlighted;
  rulers_.clear();
  rulers_.reserve(rulers.size());
  for (const auto& ruler : rulers) {
    rulers_.emplace_back(new Ruler(*ruler));
  }
  labels_.clear();
  labels_.reserve(labels.size());
  for (const auto& label : labels) {
    labels_.emplace_back(new Label(*label));
  }

  if (!isRunning()) {
    start(LowPriority);
  } else {
    restart_ = true;
    condition_.wakeOne();
  }
}

void RenderThread::run()
{
  forever
  {
    SelectionSet selected;
    HighlightSet highlighted;
    Rulers rulers;
    Labels labels;
    mutex_.lock();
    const QRect draw_bounds = draw_rect_;
    selected.swap(selected_);
    highlighted.swap(highlighted_);
    rulers.swap(rulers_);
    labels.swap(labels_);
    mutex_.unlock();
    QImage image(draw_bounds.width(),
                 draw_bounds.height(),
                 QImage::Format_ARGB32_Premultiplied);
    // drawing can be interrupted by setting restart_
    try {
      draw(image,
           draw_bounds,
           selected,
           highlighted,
           rulers,
           labels,
           1.0,
           Qt::transparent);
    } catch (const std::exception& e) {
      logger_->warn(
          GUI, 102, "An exception occurred during rendering: {}", e.what());
    } catch (...) {
      logger_->warn(GUI, 103, "An unknown exception occurred during rendering");
    }

    if (!restart_) {
      is_rendering_ = false;

      emit done(image, draw_bounds);

      if (!is_first_render_done_) {
        is_first_render_done_ = true;
      }
    }
    if (abort_) {
      return;
    }

    mutex_.lock();
    if (!restart_) {
      condition_.wait(&mutex_);
    }
    restart_ = false;
    mutex_.unlock();
  }
}

void RenderThread::drawDesignLoadingMessage(Painter& painter,
                                            const odb::Rect& bounds)
{
  QPainter* qpainter = static_cast<GuiPainter&>(painter).getPainter();
  const QFont initial_font = qpainter->font();
  QFont design_loading_font = qpainter->font();
  design_loading_font.setPointSize(16);

  qpainter->setFont(design_loading_font);

  std::string message = "Design loading...";
  painter.setPen(gui::Painter::kWhite, true);
  painter.drawString(
      bounds.xCenter(), bounds.yCenter(), Painter::kCenter, message);

  qpainter->setFont(initial_font);
}

void RenderThread::draw(QImage& image,
                        const QRect& draw_bounds,
                        const SelectionSet& selected,
                        const HighlightSet& highlighted,
                        const Rulers& rulers,
                        const Labels& labels,
                        qreal render_ratio,
                        const QColor& background)
{
  if (image.isNull()) {
    return;
  }
  // Prevent a paintEvent and a save_image call from interfering
  // (eg search RTree construction)
  absl::MutexLock lock(&drawing_mutex_);
  QPainter painter(&image);
  painter.setRenderHints(QPainter::Antialiasing);

  // Fill draw region with the background
  image.fill(background);

  // Setup the transform to map coordinates from dbu to pixels
  painter.translate(-draw_bounds.topLeft());
  painter.translate(viewer_->centering_shift_);
  painter.scale(viewer_->pixels_per_dbu_, -viewer_->pixels_per_dbu_);
  painter.scale(render_ratio, render_ratio);

  const Rect dbu_bounds = viewer_->screenToDBU(draw_bounds);

  GuiPainter gui_painter(&painter,
                         viewer_->options_,
                         viewer_->screenToDBU(draw_bounds),
                         viewer_->pixels_per_dbu_,
                         viewer_->getChip()->getDb()->getDbuPerMicron());

  if (!is_first_render_done_ && !restart_) {
    drawDesignLoadingMessage(gui_painter, dbu_bounds);
    emit done(image, draw_bounds);

    // Erase the first render indication so it does not remain on the screen
    // when the design is drawn for the first time
    image.fill(background);
  }

  drawChips(&painter, viewer_->getChip(), dbu_bounds, 0);
  for (auto* renderer : Gui::get()->renderers()) {
    if (restart_) {
      break;
    }
    gui_painter.saveState();
    renderer->drawObjects(gui_painter);
    gui_painter.restoreState();
  }
  // draw selected and over top level and fast painting events
  drawSelected(gui_painter, selected);
  // Always last so on top
  drawHighlighted(gui_painter, highlighted);
  drawRulers(gui_painter, rulers);
  drawLabels(gui_painter, labels);
}

QColor RenderThread::getColor(dbTechLayer* layer)
{
  return viewer_->options_->color(layer);
}

Qt::BrushStyle RenderThread::getPattern(dbTechLayer* layer)
{
  return viewer_->options_->pattern(layer);
}

void RenderThread::addInstTransform(QTransform& xfm,
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

bool RenderThread::instanceBelowMinSize(dbInst* inst)
{
  dbMaster* master = inst->getMaster();
  int master_height = master->getHeight();
  int master_width = master->getHeight();
  const int minimum_size = viewer_->coarseViewableResolution();

  if (master_height < minimum_size) {
    return true;
  }
  if (!inst->getMaster()->isCore() && master_width < minimum_size) {
    // if core cell, just check master height
    return true;
  }
  return false;
}

void RenderThread::drawTracks(dbTechLayer* layer,
                              QPainter* painter,
                              const Rect& bounds)
{
  if (!viewer_->options_->arePrefTracksVisible()
      && !viewer_->options_->areNonPrefTracksVisible()) {
    return;
  }

  dbTrackGrid* grid = viewer_->getBlock()->findTrackGrid(layer);
  if (!grid) {
    return;
  }

  Rect block_bounds = viewer_->getBlock()->getDieArea();
  if (!block_bounds.intersects(bounds)) {
    return;
  }
  const Rect draw_bounds = block_bounds.intersect(bounds);
  const int min_resolution = viewer_->shapeSizeLimit();

  QPen pen(getColor(layer));
  pen.setCosmetic(true);
  painter->setPen(pen);
  painter->setBrush(Qt::NoBrush);

  bool is_horizontal = layer->getDirection() == dbTechLayerDir::HORIZONTAL;
  if ((!is_horizontal && viewer_->options_->arePrefTracksVisible())
      || (is_horizontal && viewer_->options_->areNonPrefTracksVisible())) {
    bool show_grid = true;
    for (int i = 0; i < grid->getNumGridPatternsX(); i++) {
      int origin, line_count, step;
      grid->getGridPatternX(i, origin, line_count, step);
      show_grid &= step > min_resolution;
    }

    if (show_grid) {
      for (int x : grid->getGridX()) {
        if (restart_) {
          break;
        }
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

  if ((is_horizontal && viewer_->options_->arePrefTracksVisible())
      || (!is_horizontal && viewer_->options_->areNonPrefTracksVisible())) {
    bool show_grid = true;
    for (int i = 0; i < grid->getNumGridPatternsY(); i++) {
      int origin, line_count, step;
      grid->getGridPatternY(i, origin, line_count, step);
      show_grid &= step > min_resolution;
    }

    if (show_grid) {
      for (int y : grid->getGridY()) {
        if (restart_) {
          break;
        }
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

void RenderThread::drawRows(QPainter* painter,
                            odb::dbBlock* block,
                            const Rect& bounds)
{
  if (!viewer_->options_->areSitesVisible()) {
    return;
  }

  for (const auto& [row, row_site, index] :
       viewer_->getRowRects(block, bounds)) {
    if (restart_) {
      break;
    }
    odb::dbSite* site = nullptr;
    if (row->getObjectType() == odb::dbObjectType::dbSiteObj) {
      site = static_cast<odb::dbSite*>(row);
    } else {
      site = static_cast<odb::dbRow*>(row)->getSite();
    }
    if (viewer_->options_->isSiteVisible(site)) {
      QPen pen(viewer_->options_->siteColor(site));
      pen.setCosmetic(true);
      painter->setPen(pen);
      painter->setBrush(Qt::NoBrush);
      painter->drawRect(
          row_site.xMin(), row_site.yMin(), row_site.dx(), row_site.dy());
    }
  }
}

void RenderThread::drawSelected(Painter& painter, const SelectionSet& selected)
{
  if (!viewer_->options_->areSelectedVisible()) {
    return;
  }

  for (auto& selected : selected) {
    selected.highlight(painter, Painter::kHighlight);
  }

  if (viewer_->focus_) {
    viewer_->focus_.highlight(painter,
                              Painter::kHighlight,
                              1,
                              Painter::kHighlight,
                              Painter::Brush::kDiagonal);
  }
}

void RenderThread::drawHighlighted(Painter& painter,
                                   const HighlightSet& highlighted)
{
  int highlight_group = 0;
  for (auto& highlight_set : highlighted) {
    auto highlight_color = Painter::kHighlightColors[highlight_group];

    for (auto& highlighted : highlight_set) {
      highlighted.highlight(painter, highlight_color, 1, highlight_color);
    }

    highlight_group++;
  }
}

void RenderThread::drawRulers(Painter& painter, const Rulers& rulers)
{
  if (!viewer_->options_->areRulersVisible()) {
    return;
  }

  for (auto& ruler : rulers) {
    painter.drawRuler(ruler->getPt0().x(),
                      ruler->getPt0().y(),
                      ruler->getPt1().x(),
                      ruler->getPt1().y(),
                      ruler->isEuclidian(),
                      ruler->getLabel());
  }
}

void RenderThread::drawLabels(Painter& painter, const Labels& labels)
{
  if (!viewer_->options_->areLabelsVisible()) {
    return;
  }

  painter.saveState();

  const QFont qfont = viewer_->options_->labelFont();
  for (auto& label : labels) {
    const Painter::Color color = label->getColor();

    painter.setPen(color, true);
    painter.setBrush(color);

    const auto size = label->getSize();
    const Painter::Font font(qfont.family().toStdString(),
                             size.value_or(qfont.pointSize()));
    painter.setFont(font);

    painter.drawString(label->getPt().x(),
                       label->getPt().y(),
                       label->getAnchor(),
                       label->getText());
  }

  painter.restoreState();
}

// Draw the instances bounds
void RenderThread::drawInstanceOutlines(QPainter* painter,
                                        const std::vector<odb::dbInst*>& insts)
{
  int minimum_height_for_tag = viewer_->nominalViewableResolution();
  int minimum_size = viewer_->fineViewableResolution();
  const QTransform initial_xfm = painter->transform();

  painter->setPen(QPen(Qt::gray, 0));
  painter->setBrush(QBrush());
  for (auto inst : insts) {
    if (restart_) {
      break;
    }
    dbMaster* master = inst->getMaster();

    if (minimum_size > master->getWidth()
        && minimum_size > master->getHeight()) {
      painter->setTransform(initial_xfm);
      auto origin = inst->getOrigin();
      painter->drawPoint(origin.x(), origin.y());
    } else {
      // setup the instance's transform
      QTransform xfm = initial_xfm;
      const dbTransform inst_xfm = inst->getTransform();
      addInstTransform(xfm, inst_xfm);
      painter->setTransform(xfm);

      // draw bbox
      Rect master_box;
      master->getPlacementBoundary(master_box);

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
void RenderThread::drawInstanceShapes(dbTechLayer* layer,
                                      QPainter* painter,
                                      const std::vector<odb::dbInst*>& insts,
                                      const Rect& bounds,
                                      GuiPainter& gui_painter)
{
  const bool show_blockages = viewer_->options_->areInstanceBlockagesVisible();
  const bool show_pins = viewer_->options_->areInstancePinsVisible();
  if (!show_blockages && !show_pins) {
    return;
  }

  const int minimum_height = viewer_->nominalViewableResolution();
  const QTransform initial_xfm = painter->transform();
  std::vector<dbInst*> child_insts;
  const int instance_limit = viewer_->instanceSizeLimit();
  const bool has_child_blocks
      = insts.empty() ? false : !insts[0]->getBlock()->getChildren().empty();

  // Draw the instances' shapes
  for (auto inst : insts) {
    if (restart_) {
      break;
    }
    dbMaster* master = inst->getMaster();
    if (master->getHeight() < minimum_height) {
      continue;
    }

    dbBlock* child
        = has_child_blocks
              ? inst->getBlock()->findChild(master->getName().c_str())
              : nullptr;
    if (child) {
      // setup the instance's transform
      QTransform xfm = initial_xfm;
      const dbTransform inst_xfm = inst->getTransform();
      addInstTransform(xfm, inst_xfm);
      painter->setTransform(xfm);
      Rect bbox = child->getBBox()->getBox();
      // TODO: Use transformed bounds for search
      auto inst_range = viewer_->search_.searchInsts(child,
                                                     bbox.xMin(),
                                                     bbox.yMin(),
                                                     bbox.xMax(),
                                                     bbox.yMax(),
                                                     instance_limit);
      child_insts.clear();
      child_insts.reserve(10000);
      for (auto* inst : inst_range) {
        if (viewer_->options_->isInstanceVisible(inst)) {
          child_insts.push_back(inst);
        }
      }

      drawLayer(painter, child, layer, child_insts, bbox, gui_painter);
      continue;
    }

    const auto boxes = viewer_->boxesByLayer(master, layer);

    if (boxes == nullptr) {
      continue;  // no shapes on this layer
    }

    // setup the instance's transform
    QTransform xfm = initial_xfm;
    const dbTransform inst_xfm = inst->getTransform();
    addInstTransform(xfm, inst_xfm);
    painter->setTransform(xfm);

    // Only draw the pins/obs if they are big enough to be useful
    painter->setPen(Qt::NoPen);
    QColor color = getColor(layer);
    Qt::BrushStyle brush_pattern = getPattern(layer);

    if (show_blockages) {
      painter->setBrush(color.lighter());
      for (const auto& poly : boxes->obs) {
        painter->drawPolygon(poly);
      }
    }

    if (show_pins) {
      painter->setBrush(QBrush(color, brush_pattern));
      for (const auto& [mterm, polys] : boxes->mterms) {
        for (const auto& poly : polys) {
          painter->drawPolygon(poly);
        }
      }
    }
  }

  painter->setTransform(initial_xfm);
}

// Draw the instances' names
void RenderThread::drawInstanceNames(QPainter* painter,
                                     const std::vector<odb::dbInst*>& insts)
{
  if (!viewer_->options_->areInstanceNamesVisible()) {
    return;
  }

  const QColor text_color = viewer_->options_->instanceNameColor();
  const QFont initial_font = painter->font();
  const QFont text_font = viewer_->options_->instanceNameFont();

  painter->setFont(text_font);
  for (auto inst : insts) {
    if (restart_) {
      break;
    }

    if (instanceBelowMinSize(inst)) {
      continue;
    }

    Rect instance_box = inst->getBBox()->getBox();
    QString name = inst->getName().c_str();
    auto master = inst->getMaster();
    auto center = master->isBlock() || master->isPad();
    drawTextInBBox(
        text_color, text_font, instance_box, std::move(name), painter, center);
  }
  painter->setFont(initial_font);
}

// Draw the instances ITerm names
void RenderThread::drawITermLabels(QPainter* painter,
                                   const std::vector<odb::dbInst*>& insts)
{
  if (!viewer_->options_->areInstancePinsVisible()
      || !viewer_->options_->areInstancePinNamesVisible()) {
    return;
  }

  const QColor text_color = viewer_->options_->itermLabelColor();
  const QFont text_font = viewer_->options_->itermLabelFont();
  const QFont initial_font = painter->font();

  painter->setFont(text_font);
  for (auto inst : insts) {
    if (restart_) {
      break;
    }

    if (instanceBelowMinSize(inst)) {
      continue;
    }

    const odb::dbTransform xform = inst->getTransform();
    for (auto inst_iterm : inst->getITerms()) {
      bool drawn = false;
      for (auto* mpin : inst_iterm->getMTerm()->getMPins()) {
        for (auto* geom : mpin->getGeometry()) {
          const auto layer = geom->getTechLayer();
          if (layer == nullptr) {
            continue;
          }
          if (viewer_->options_->isVisible(layer)) {
            Rect pin_rect = geom->getBox();
            xform.apply(pin_rect);
            const QString name = inst_iterm->getMTerm()->getConstName();
            drawn = drawTextInBBox(
                text_color, text_font, pin_rect, name, painter, false);
          }
          if (drawn) {
            // Only draw on the first box
            break;
          }
        }
        if (drawn) {
          break;
        }
      }
    }
  }

  painter->setFont(initial_font);
}

bool RenderThread::drawTextInBBox(const QColor& text_color,
                                  const QFont& text_font,
                                  Rect bbox,
                                  QString name,
                                  QPainter* painter,
                                  bool center)
{
  const QFontMetricsF font_metrics(text_font);

  // minimum pixel height for text (10px)
  if (font_metrics.ascent() < 10) {
    // text is too small
    return false;
  }

  // text should not fill more than 90% of the instance height or width
  static const float kSizeLimit = 0.9;
  // slightly lower to prevent oscillating rotations when zooming
  static const float kRotationLimit = 0.85;

  // limit non-core text to 1/2.0 (50%) of cell height or width
  static const float kNonCoreScaleLimit = 2.0;

  const qreal scale_adjust = 1.0 / viewer_->pixels_per_dbu_;

  QRectF bbox_in_px = viewer_->dbuToScreen(bbox);

  QRectF text_bounding_box = font_metrics.boundingRect(name);

  bool do_rotate = false;
  if (text_bounding_box.width() > kRotationLimit * bbox_in_px.width()) {
    // non-rotated text will not fit without elide
    if (bbox_in_px.height() > bbox_in_px.width()) {
      // check if more text will fit if rotated
      do_rotate = true;
    }
  }

  qreal text_height_check = kNonCoreScaleLimit * text_bounding_box.height();
  // don't show text if it's more than "non_core_scale_limit" of cell
  // height/width this keeps text from dominating the cell size
  if (!do_rotate && text_height_check > bbox_in_px.height()) {
    return false;
  }
  if (do_rotate && text_height_check > bbox_in_px.width()) {
    return false;
  }

  if (do_rotate) {
    name = font_metrics.elidedText(
        name, Qt::ElideLeft, kSizeLimit * bbox_in_px.height());
  } else {
    name = font_metrics.elidedText(
        name, Qt::ElideLeft, kSizeLimit * bbox_in_px.width());
  }
  text_bounding_box = font_metrics.boundingRect(name);

  const QTransform initial_xfm = painter->transform();

  painter->translate(bbox.xMin(), bbox.yMin());
  painter->scale(scale_adjust, -scale_adjust);
  if (do_rotate) {
    painter->rotate(90);
    painter->translate(-text_bounding_box.width(), 0);
    // account for descent of font
    painter->translate(-font_metrics.descent(), 0);
    if (center) {
      const auto x_offset
          = (bbox_in_px.height() - text_bounding_box.width()) / 2;
      const auto y_offset
          = (bbox_in_px.width() - text_bounding_box.height()) / 2;
      painter->translate(-x_offset, -y_offset);
    }
  } else {
    // account for descent of font
    painter->translate(font_metrics.descent(), 0);
    if (center) {
      const auto x_offset
          = (bbox_in_px.width() - text_bounding_box.width()) / 2;
      const auto y_offset
          = (bbox_in_px.height() - text_bounding_box.height()) / 2;
      painter->translate(x_offset, -y_offset);
    }
  }
  if (center) {
    QPainterPath path;
    path.addText(0, 0, painter->font(), name);
    painter->strokePath(path, QPen(Qt::black, 2));  // outline
    painter->fillPath(path, QBrush(text_color));    // fill
  } else {
    painter->setPen(QPen(text_color, 0));
    painter->setBrush(QBrush(text_color));
    painter->drawText(0, 0, name);
  }

  painter->setTransform(initial_xfm);

  return true;
}

void RenderThread::drawBlockages(QPainter* painter,
                                 odb::dbBlock* block,
                                 const Rect& bounds,
                                 const std::vector<odb::dbInst*>& insts)
{
  if (!viewer_->options_->areBlockagesVisible()) {
    return;
  }
  painter->setPen(Qt::NoPen);
  painter->setBrush(QBrush(viewer_->options_->placementBlockageColor(),
                           viewer_->options_->placementBlockagePattern()));

  auto blockage_range
      = viewer_->search_.searchBlockages(block,
                                         bounds.xMin(),
                                         bounds.yMin(),
                                         bounds.xMax(),
                                         bounds.yMax(),
                                         viewer_->shapeSizeLimit());

  for (auto* blockage : blockage_range) {
    if (restart_) {
      break;
    }
    Rect bbox = blockage->getBBox()->getBox();
    painter->drawRect(bbox.xMin(), bbox.yMin(), bbox.dx(), bbox.dy());
  }

  for (odb::dbInst* inst : insts) {
    if (restart_) {
      break;
    }
    odb::dbBox* halo = inst->getHalo();
    if (halo != nullptr) {
      Rect instbox = inst->getBBox()->getBox();
      Rect halobox = halo->getBox();
      instbox.set_xlo(instbox.xMin() - halobox.xMin());
      instbox.set_ylo(instbox.yMin() - halobox.yMin());
      instbox.set_xhi(instbox.xMax() + halobox.xMax());
      instbox.set_yhi(instbox.yMax() + halobox.yMax());
      painter->drawRect(
          instbox.xMin(), instbox.yMin(), instbox.dx(), instbox.dy());
    }
  }
}

void RenderThread::drawObstructions(odb::dbBlock* block,
                                    dbTechLayer* layer,
                                    QPainter* painter,
                                    const Rect& bounds)
{
  if (!viewer_->options_->areObstructionsVisible()
      || !viewer_->options_->isVisible(layer)) {
    return;
  }

  painter->setPen(Qt::NoPen);
  QColor color = getColor(layer).darker();
  Qt::BrushStyle brush_pattern = getPattern(layer);
  painter->setBrush(QBrush(color, brush_pattern));

  auto obstructions_range
      = viewer_->search_.searchObstructions(block,
                                            layer,
                                            bounds.xMin(),
                                            bounds.yMin(),
                                            bounds.xMax(),
                                            bounds.yMax(),
                                            viewer_->shapeSizeLimit());

  for (auto* obs : obstructions_range) {
    if (restart_) {
      break;
    }
    Rect bbox = obs->getBBox()->getBox();
    painter->drawRect(bbox.xMin(), bbox.yMin(), bbox.dx(), bbox.dy());
  }
}

void RenderThread::drawViaShapes(QPainter* painter,
                                 odb::dbBlock* block,
                                 dbTechLayer* cut_layer,
                                 dbTechLayer* draw_layer,
                                 const Rect& bounds,
                                 const int shape_limit)
{
  auto via_sbox_iter = viewer_->search_.searchSNetViaShapes(block,
                                                            cut_layer,
                                                            bounds.xMin(),
                                                            bounds.yMin(),
                                                            bounds.xMax(),
                                                            bounds.yMax(),
                                                            shape_limit);

  std::vector<odb::dbShape> via_shapes;
  for (const auto& [sbox, net] : via_sbox_iter) {
    if (restart_) {
      break;
    }
    if (!viewer_->isNetVisible(net)) {
      continue;
    }

    sbox->getViaLayerBoxes(draw_layer, via_shapes);
    for (auto& shape : via_shapes) {
      if (shape.getTechLayer() == draw_layer) {
        painter->drawRect(shape.xMin(),
                          shape.yMin(),
                          shape.xMax() - shape.xMin(),
                          shape.yMax() - shape.yMin());
      }
    }
  }
}

void RenderThread::drawLayer(QPainter* painter,
                             odb::dbBlock* block,
                             dbTechLayer* layer,
                             const std::vector<dbInst*>& insts,
                             const Rect& bounds,
                             GuiPainter& gui_painter)
{
  if (!viewer_->options_->isVisible(layer)) {
    return;
  }
  utl::Timer layer_timer;

  const int shape_limit = viewer_->shapeSizeLimit();

  // Skip the cut layer if the cuts will be too small to see
  const bool draw_shapes
      = !(layer->getType() == dbTechLayerType::CUT
          && viewer_->cut_maximum_size_[layer] < shape_limit);
  const bool layer_is_routing = layer->getType() == dbTechLayerType::CUT
                                || layer->getType() == dbTechLayerType::ROUTING;

  if (draw_shapes) {
    drawInstanceShapes(layer, painter, insts, bounds, gui_painter);
  }

  drawObstructions(block, layer, painter, bounds);

  const bool draw_routing
      = viewer_->options_->areRoutingSegmentsVisible() && layer_is_routing;
  const bool draw_vias
      = viewer_->options_->areRoutingViasVisible() && layer_is_routing;
  // Now draw the shapes
  QColor color = getColor(layer);
  Qt::BrushStyle brush_pattern = getPattern(layer);
  painter->setBrush(QBrush(color, brush_pattern));
  painter->setPen(QPen(color, 0));
  if (draw_shapes) {
    if (draw_routing || draw_vias) {
      auto box_iter = viewer_->search_.searchBoxShapes(block,
                                                       layer,
                                                       bounds.xMin(),
                                                       bounds.yMin(),
                                                       bounds.xMax(),
                                                       bounds.yMax(),
                                                       shape_limit);

      for (auto& [box, is_via, net] : box_iter) {
        if (restart_) {
          break;
        }
        if (!draw_routing && !is_via) {
          // Don't draw since it's a segment
          continue;
        }
        if (!draw_vias && is_via) {
          // Don't draw since it's a via
          continue;
        }
        if (!viewer_->isNetVisible(net)) {
          continue;
        }
        const auto& ll = box.ll();
        painter->drawRect(QRect(ll.x(), ll.y(), box.dx(), box.dy()));
      }
    }

    if (viewer_->options_->areSpecialRoutingViasVisible() && layer_is_routing) {
      if (layer->getType() == dbTechLayerType::CUT) {
        drawViaShapes(painter, block, layer, layer, bounds, shape_limit);
      } else {
        // Get the enclosure shapes from any vias on the cut layers
        // above or below this one.  Skip enclosure shapes if they
        // will be too small based on the cut size (enclosure shapes
        // are generally only slightly larger).
        if (auto upper = layer->getUpperLayer()) {
          if (viewer_->cut_maximum_size_[upper] >= shape_limit) {
            drawViaShapes(painter, block, upper, layer, bounds, shape_limit);
          }
        }
        if (auto lower = layer->getLowerLayer()) {
          if (viewer_->cut_maximum_size_[lower] >= shape_limit) {
            drawViaShapes(painter, block, lower, layer, bounds, shape_limit);
          }
        }
      }
    }

    if (viewer_->options_->areSpecialRoutingSegmentsVisible()
        && layer_is_routing) {
      auto polygon_iter = viewer_->search_.searchSNetShapes(block,
                                                            layer,
                                                            bounds.xMin(),
                                                            bounds.yMin(),
                                                            bounds.xMax(),
                                                            bounds.yMax(),
                                                            shape_limit);

      for (auto& [box, poly, net] : polygon_iter) {
        if (restart_) {
          break;
        }
        if (!viewer_->isNetVisible(net)) {
          continue;
        }
        const auto& points = poly.getPoints();
        QPolygon qpoly(points.size());
        for (int i = 0; i < points.size(); i++) {
          const auto& pt = points[i];
          qpoly.setPoint(i, pt.x(), pt.y());
        }
        painter->drawPolygon(qpoly);
      }
    }

    // Now draw the fills
    if (viewer_->options_->areFillsVisible()) {
      QColor fill_color = getColor(layer).lighter(50);
      painter->setBrush(QBrush(fill_color, brush_pattern));
      painter->setPen(QPen(fill_color, 0));
      auto iter = viewer_->search_.searchFills(block,
                                               layer,
                                               bounds.xMin(),
                                               bounds.yMin(),
                                               bounds.xMax(),
                                               bounds.yMax(),
                                               shape_limit);

      for (auto* fill : iter) {
        if (restart_) {
          break;
        }
        odb::Rect box;
        fill->getRect(box);
        const auto& ll = box.ll();
        painter->drawRect(QRect(ll.x(), ll.y(), box.dx(), box.dy()));
      }
    }
  }

  painter->setBrush(QBrush(color, brush_pattern));
  painter->setPen(QPen(color, 0));

  if (draw_shapes) {
    if (viewer_->options_->areIOPinsVisible()) {
      utl::Timer io_pins;
      drawIOPins(gui_painter, block, bounds, layer);
      debugPrint(logger_,
                 GUI,
                 "draw",
                 1,
                 "io pins on {} {}",
                 layer->getName(),
                 io_pins);
    }

    drawTracks(layer, painter, bounds);
    drawRouteGuides(gui_painter, layer);
    drawNetTracks(gui_painter, layer);
  }

  if (viewer_->options_->areFocusedNetsGuidesVisible()) {
    drawNetsRouteGuides(gui_painter, viewer_->focus_nets_, layer);
  }

  for (auto* renderer : Gui::get()->renderers()) {
    if (restart_) {
      break;
    }
    gui_painter.saveState();
    renderer->drawLayer(layer, gui_painter);
    gui_painter.restoreState();
  }
  debugPrint(logger_,
             GUI,
             "draw",
             1,
             "layer {} render {}",
             layer->getName(),
             layer_timer);
}

void RenderThread::drawChips(QPainter* painter,
                             dbChip* chip,
                             const Rect& bounds,
                             int depth)
{
  drawChip(painter, chip, bounds, depth);

  const QTransform initial_xfm = painter->transform();
  for (odb::dbChipInst* inst : chip->getChipInsts()) {
    QTransform xfm = initial_xfm;
    const dbTransform inst_xfm = inst->getTransform();
    addInstTransform(xfm, inst_xfm);
    painter->setTransform(xfm);

    // Project bounds into inst
    dbTransform inverse_xfm;
    inst_xfm.invert(inverse_xfm);
    Rect new_bounds = bounds;
    inverse_xfm.apply(new_bounds);

    drawChips(painter, inst->getMasterChip(), new_bounds, depth);
  }
  painter->setTransform(initial_xfm);
}

// Draw the region of the chip.  Depth is not yet used but
// is there for hierarchical design support.
void RenderThread::drawChip(QPainter* painter,
                            dbChip* chip,
                            const Rect& bounds,
                            int depth)
{
  utl::Timer timer;

  utl::Timer manufacturing_grid_timer;
  const int instance_limit = viewer_->instanceSizeLimit();

  // Draw die area, if set
  painter->setPen(QPen(Qt::gray, 0));
  painter->setBrush(QBrush());

  dbBlock* block = chip->getBlock();
  if (!block) {
    painter->drawRect(0, 0, chip->getWidth(), chip->getHeight());
    return;
  }

  GuiPainter gui_painter(painter,
                         viewer_->options_,
                         bounds,
                         viewer_->pixels_per_dbu_,
                         chip->getDb()->getDbuPerMicron());

  odb::Polygon die_area = block->getDieAreaPolygon();

  if (die_area.getEnclosingRect().area() > 0) {
    gui_painter.drawPolygon(die_area);
  }

  // Draw core area, if set
  const odb::Polygon core_area = block->getCoreAreaPolygon();

  if (core_area.getEnclosingRect().area() > 0) {
    gui_painter.drawPolygon(core_area);
  }

  drawManufacturingGrid(painter, bounds);
  debugPrint(logger_,
             GUI,
             "draw",
             1,
             "manufacturing grid {}",
             manufacturing_grid_timer);

  utl::Timer inst_timer;
  auto inst_range = viewer_->search_.searchInsts(block,
                                                 bounds.xMin(),
                                                 bounds.yMin(),
                                                 bounds.xMax(),
                                                 bounds.yMax(),
                                                 instance_limit);

  // Cache the search results as we will iterate over the instances
  // for each layer.
  std::vector<dbInst*> insts;
  insts.reserve(10000);
  for (auto* inst : inst_range) {
    if (restart_) {
      break;
    }
    if (viewer_->options_->isInstanceVisible(inst)) {
      insts.push_back(inst);
    }
  }
  debugPrint(logger_, GUI, "draw", 1, "inst search {}", inst_timer);

  utl::Timer io_pins_setup;
  setupIOPins(block, bounds);
  debugPrint(logger_, GUI, "draw", 1, "io pins setup {}", io_pins_setup);

  utl::Timer insts_outline;
  drawInstanceOutlines(painter, insts);
  debugPrint(logger_, GUI, "draw", 1, "inst outline render {}", insts_outline);

  // draw blockages
  utl::Timer inst_blockages;
  drawBlockages(painter, block, bounds, insts);
  debugPrint(logger_, GUI, "draw", 1, "blockages {}", inst_blockages);

  dbTech* tech = block->getTech();
  std::set<dbTech*> child_techs;
  for (auto child : block->getChildren()) {
    dbTech* child_tech = child->getTech();
    if (child_tech != tech) {
      child_techs.insert(child_tech);
    }
  }

  for (dbTech* child_tech : child_techs) {
    for (dbTechLayer* layer : child_tech->getLayers()) {
      if (restart_) {
        break;
      }
      drawLayer(painter, block, layer, insts, bounds, gui_painter);
    }
  }

  for (dbTechLayer* layer : tech->getLayers()) {
    if (restart_) {
      break;
    }
    drawLayer(painter, block, layer, insts, bounds, gui_painter);
  }

  utl::Timer inst_names;
  drawInstanceNames(painter, insts);
  debugPrint(logger_, GUI, "draw", 1, "instance names {}", inst_names);

  utl::Timer inst_iterms;
  drawITermLabels(painter, insts);
  debugPrint(logger_, GUI, "draw", 1, "instance iterms {}", inst_iterms);

  utl::Timer inst_rows;
  drawRows(painter, block, bounds);
  debugPrint(logger_, GUI, "draw", 1, "rows {}", inst_rows);

  utl::Timer inst_access_points;
  if (viewer_->options_->areAccessPointsVisible()) {
    drawAccessPoints(gui_painter, block, bounds, insts);
  }
  debugPrint(logger_, GUI, "draw", 1, "access points {}", inst_access_points);

  utl::Timer inst_module_view;
  drawModuleView(painter, insts);
  debugPrint(logger_, GUI, "draw", 1, "module view {}", inst_module_view);

  utl::Timer inst_regions;
  drawRegions(painter, block);
  debugPrint(logger_, GUI, "draw", 1, "regions {}", inst_regions);

  utl::Timer inst_cell_grid;
  drawGCellGrid(painter, bounds);
  debugPrint(logger_, GUI, "draw", 1, "save cell grid {}", inst_cell_grid);

  debugPrint(logger_, GUI, "draw", 1, "total render {}", timer);
}

void RenderThread::drawGCellGrid(QPainter* painter, const odb::Rect& bounds)
{
  if (!viewer_->options_->isGCellGridVisible()) {
    return;
  }

  odb::dbGCellGrid* grid = viewer_->getBlock()->getGCellGrid();

  if (grid == nullptr) {
    return;
  }

  const auto die_area = viewer_->getBlock()->getDieArea();

  if (!bounds.intersects(die_area)) {
    return;
  }

  const odb::Rect draw_bounds = bounds.intersect(die_area);

  std::vector<int> x_grid, y_grid;
  grid->getGridX(x_grid);
  grid->getGridY(y_grid);

  painter->setPen(QPen(Qt::white, 0));

  for (const auto& x : x_grid) {
    if (restart_) {
      break;
    }
    if (x < draw_bounds.xMin() || draw_bounds.xMax() < x) {
      continue;
    }

    painter->drawLine(x, draw_bounds.yMin(), x, draw_bounds.yMax());
  }

  for (const auto& y : y_grid) {
    if (restart_) {
      break;
    }
    if (y < draw_bounds.yMin() || draw_bounds.yMax() < y) {
      continue;
    }

    painter->drawLine(draw_bounds.xMin(), y, draw_bounds.xMax(), y);
  }
}

void RenderThread::drawManufacturingGrid(QPainter* painter,
                                         const odb::Rect& bounds)
{
  if (!viewer_->options_->isManufacturingGridVisible()) {
    return;
  }

  odb::dbTech* tech = viewer_->getBlock()->getDb()->getTech();
  if (!tech->hasManufacturingGrid()) {
    return;
  }

  const int grid = tech->getManufacturingGrid();  // DBU

  const int pixels_per_grid_point = grid * viewer_->pixels_per_dbu_;
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
    if (restart_) {
      break;
    }
    for (int y = first_y; y <= last_y; y += grid) {
      points.append(QPoint(x, y));
    }
  }

  painter->setPen(QPen(Qt::white, 0));
  painter->drawPoints(points);
}

void RenderThread::drawRegions(QPainter* painter, odb::dbBlock* block)
{
  if (!viewer_->options_->areRegionsVisible()) {
    return;
  }

  painter->setPen(QPen(Qt::gray, 0));
  painter->setBrush(QBrush(viewer_->options_->regionColor(),
                           viewer_->options_->regionPattern()));

  for (auto* region : block->getRegions()) {
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

void RenderThread::drawRouteGuides(Painter& painter, odb::dbTechLayer* layer)
{
  if (viewer_->route_guides_.empty()) {
    return;
  }

  drawNetsRouteGuides(painter, viewer_->route_guides_, layer);
}

void RenderThread::drawNetsRouteGuides(Painter& painter,
                                       const std::set<odb::dbNet*>& nets,
                                       odb::dbTechLayer* layer)
{
  painter.setPen(layer);
  painter.setBrush(layer);

  for (odb::dbNet* net : nets) {
    if (restart_) {
      break;
    }

    drawNetRouteGuides(painter, net, layer);
  }
}

void RenderThread::drawNetRouteGuides(Painter& painter,
                                      odb::dbNet* net,
                                      odb::dbTechLayer* layer)
{
  for (odb::dbGuide* guide : net->getGuides()) {
    if (guide->getLayer() == layer) {
      painter.drawRect(guide->getBox());
    }
  }
}

void RenderThread::drawNetTracks(Painter& painter, odb::dbTechLayer* layer)
{
  if (viewer_->net_tracks_.empty()) {
    return;
  }
  painter.setPen(layer);
  painter.setBrush(layer);
  for (auto net : viewer_->net_tracks_) {
    if (restart_) {
      break;
    }
    for (auto track : net->getTracks()) {
      if (track->getLayer() != layer) {
        continue;
      }
      painter.drawRect(track->getBox());
    }
  }
}

void RenderThread::drawAccessPoints(Painter& painter,
                                    odb::dbBlock* block,
                                    const odb::Rect& bounds,
                                    const std::vector<odb::dbInst*>& insts)
{
  const int shape_limit = viewer_->shapeSizeLimit();
  const int shape_size = 100;  // units DBU
  if (shape_limit > shape_size) {
    return;
  }

  const Painter::Color has_access = Painter::kGreen;
  const Painter::Color not_access = Painter::kRed;

  auto draw = [&](odb::dbAccessPoint* ap, const odb::dbTransform& transform) {
    if (ap == nullptr) {
      return;
    }
    if (!viewer_->options_->isVisible(ap->getLayer())) {
      return;
    }

    Point pt = ap->getPoint();
    transform.apply(pt);

    auto color = ap->hasAccess() ? has_access : not_access;
    painter.setPen(color, /* cosmetic */ true);
    painter.drawX(pt.x(), pt.y(), shape_size);
  };

  if (viewer_->options_->areInstancePinsVisible()) {
    for (auto* inst : insts) {
      if (restart_) {
        break;
      }
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

  odb::dbTech* tech = block->getTech();
  for (odb::dbTechLayer* layer : tech->getLayers()) {
    for (const auto& [box, pin] : viewer_->search_.searchBPins(block,
                                                               layer,
                                                               bounds.xMin(),
                                                               bounds.yMin(),
                                                               bounds.xMax(),
                                                               bounds.yMax())) {
      if (restart_) {
        break;
      }
      for (auto ap : pin->getAccessPoints()) {
        draw(ap, {});
      }
    }
  }
}

void RenderThread::drawModuleView(QPainter* painter,
                                  const std::vector<odb::dbInst*>& insts)
{
  if (!viewer_->options_->isModuleView()) {
    return;
  }

  for (auto* inst : insts) {
    if (restart_) {
      break;
    }
    auto* module = inst->getModule();

    if (module == nullptr) {
      continue;
    }

    const auto& setting = viewer_->modules_.at(module);

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

void RenderThread::setupIOPins(odb::dbBlock* block, const odb::Rect& bounds)
{
  if (!viewer_->options_->areIOPinsVisible()) {
    pin_draw_names_ = false;
    return;
  }

  const auto die_area = block->getDieArea();

  pin_draw_names_ = viewer_->options_->areIOPinNamesVisible();
  const double scale_factor
      = 0.02;  // 4 Percent of bounds is used to draw pin-markers
  const int die_max_dim = std::min(die_area.maxDXDY(), bounds.maxDXDY());
  const double abs_min_dim = 8.0;  // prevent markers from falling apart
  pin_max_size_ = std::max(scale_factor * die_max_dim, abs_min_dim);
  if (pin_draw_names_) {
    pin_font_ = viewer_->options_->ioPinMarkersFont();
    const QFontMetrics font_metrics(pin_font_);

    const int vertical_gap
        = (viewer_->geometry().height()
           - viewer_->getBounds().dy() * viewer_->pixels_per_dbu_)
          / 2;
    const int horizontal_gap
        = (viewer_->geometry().width()
           - viewer_->getBounds().dx() * viewer_->pixels_per_dbu_)
          / 2;

    const int available_space
        = std::min(vertical_gap, horizontal_gap)
          - std::ceil(pin_max_size_) * viewer_->pixels_per_dbu_;  // in pixels

    int font_size = pin_font_.pointSize();
    const int drawing_font_size = 6;  // in points

    // when the size is minimum the text won't be drawn
    const int minimum_font_size = drawing_font_size - 1;

    int largest_text_width = 0;
    std::set<odb::dbBTerm*> checked;
    odb::dbTech* tech = block->getTech();
    for (odb::dbTechLayer* layer : tech->getLayers()) {
      if (restart_) {
        return;
      }
      if (font_size <= minimum_font_size) {
        break;
      }
      for (const auto& [box, pin] :
           viewer_->search_.searchBPins(block,
                                        layer,
                                        bounds.xMin(),
                                        bounds.yMin(),
                                        bounds.xMax(),
                                        bounds.yMax())) {
        if (restart_) {
          return;
        }
        odb::dbBTerm* bterm = pin->getBTerm();
        if (checked.find(bterm) != checked.end()) {
          continue;
        }
        checked.insert(bterm);

        QString current_text = QString::fromStdString(pin->getName());
        const int text_width = font_metrics.boundingRect(current_text).width();
        largest_text_width = std::max(text_width, largest_text_width);
        while (largest_text_width > available_space) {
          if (font_size <= minimum_font_size) {
            break;
          }
          font_size -= 1;
          pin_font_.setPointSize(font_size);
          QFontMetrics current_font_metrics(pin_font_);
          largest_text_width
              = current_font_metrics.boundingRect(current_text).width();
        }

        if (font_size <= minimum_font_size) {
          break;
        }
      }
    }

    // draw names of pins when text height is at least 6 pts
    pin_draw_names_ = font_size >= drawing_font_size;
  } else {
    pin_draw_names_ = false;
  }
}

void RenderThread::drawIOPins(Painter& painter,
                              odb::dbBlock* block,
                              const odb::Rect& bounds,
                              odb::dbTechLayer* layer)
{
  if (!viewer_->options_->areIOPinsVisible()
      || !viewer_->options_->areIOPinNamesVisible()) {
    return;
  }

  const auto die_area = block->getDieArea();

  QPainter* qpainter = static_cast<GuiPainter&>(painter).getPainter();
  const QFont initial_font = qpainter->font();
  qpainter->setFont(pin_font_);

  const int text_margin = 2.0 / viewer_->pixels_per_dbu_;

  // templates of pin markers (block top)
  const std::vector<Point> in_marker{// arrow head pointing in to block
                                     Point(pin_max_size_ / 4, pin_max_size_),
                                     Point(0, 0),
                                     Point(-pin_max_size_ / 4, pin_max_size_),
                                     Point(pin_max_size_ / 4, pin_max_size_)};
  const std::vector<Point> out_marker{// arrow head pointing out of block
                                      Point(0, pin_max_size_),
                                      Point(-pin_max_size_ / 4, 0),
                                      Point(pin_max_size_ / 4, 0),
                                      Point(0, pin_max_size_)};
  const std::vector<Point> bi_marker{
      // diamond
      Point(0, 0),
      Point(-pin_max_size_ / 4, pin_max_size_ / 2),
      Point(0, pin_max_size_),
      Point(pin_max_size_ / 4, pin_max_size_ / 2),
      Point(0, 0)};

  // RTree used to search for overlapping shapes and decide if rotation of
  // text is needed.
  using PinShapeTree = bgi::rtree<odb::Rect, bgi::quadratic<16>>;
  PinShapeTree pin_text_spec_shapes;
  struct PinText
  {
    std::optional<odb::Rect> rect;
    bool can_rotate;
    std::string text;
    odb::Point pt;
    Painter::Anchor anchor;
  };
  std::vector<PinText> pin_text_spec;

  const int min_bpin_size = viewer_->options_->isDetailedVisibility()
                                ? viewer_->fineViewableResolution()
                                : viewer_->nominalViewableResolution();
  const int64_t max_lin_bpins
      = min_bpin_size > 0 ? bounds.minDXDY() / min_bpin_size : bounds.minDXDY();
  const int64_t max_bpins
      = std::min(kMaxBPinsPerLayer, max_lin_bpins * max_lin_bpins);

  painter.setPen(layer);
  painter.setBrush(layer);

  auto bpins = viewer_->search_.searchBPins(
      block, layer, bounds.xMin(), bounds.yMin(), bounds.xMax(), bounds.yMax());
  debugPrint(logger_,
             GUI,
             "draw",
             2,
             "found {} bpins on layer {}, drawing limit {} ({}) bpins",
             bpins.size(),
             layer->getName(),
             max_bpins,
             max_lin_bpins);
  if (bpins.size() > max_bpins) {
    return;
  }

  std::vector<odb::Rect> pin_text_spec_shape_rects;
  for (const auto& [box, pin] : bpins) {
    if (restart_) {
      break;
    }

    odb::dbBTerm* term = pin->getBTerm();
    if (!viewer_->options_->isNetVisible(term->getNet())) {
      continue;
    }

    const Point pin_center = box->getBox().center();

    auto dist_to_left = std::abs(box->xMin() - die_area.xMin());
    auto dist_to_right = std::abs(box->xMax() - die_area.xMax());
    auto dist_to_top = std::abs(box->yMax() - die_area.yMax());
    auto dist_to_bot = std::abs(box->yMin() - die_area.yMin());
    std::vector<int> dists{
        dist_to_left, dist_to_right, dist_to_top, dist_to_bot};
    int arg_min = std::distance(dists.begin(), std::ranges::min_element(dists));

    odb::dbTransform xfm(pin_center);
    if (arg_min == 0) {  // left
      xfm.setOrient(dbOrientType::R90);
      if (dist_to_left == 0) {  // touching edge so draw on edge
        xfm.setOffset({die_area.xMin(), pin_center.y()});
      }
    } else if (arg_min == 1) {  // right
      xfm.setOrient(dbOrientType::R270);
      if (dist_to_right == 0) {  // touching edge so draw on edge
        xfm.setOffset({die_area.xMax(), pin_center.y()});
      }
    } else if (arg_min == 2) {  // top
      // none needed
      if (dist_to_top == 0) {  // touching edge so draw on edge
        xfm.setOffset({pin_center.x(), die_area.yMax()});
      }
    } else {  // bottom
      xfm.setOrient(dbOrientType::MX);
      if (dist_to_bot == 0) {  // touching edge so draw on edge
        xfm.setOffset({pin_center.x(), die_area.yMin()});
      }
    }

    // select marker
    const std::vector<Point>* template_points = &bi_marker;
    const auto pin_dir = term->getIoType();
    if (pin_dir == odb::dbIoType::INPUT) {
      template_points = &in_marker;
    } else if (pin_dir == odb::dbIoType::OUTPUT) {
      template_points = &out_marker;
    }

    // make new marker based on pin location
    std::vector<Point> marker;
    marker.reserve(template_points->size());
    for (const auto& pt : *template_points) {
      Point new_pt = pt;
      xfm.apply(new_pt);
      marker.push_back(new_pt);
    }

    // draw marker to indicate signal direction
    painter.drawPolygon(marker);

    painter.drawRect(box->getBox());

    if (pin_draw_names_) {
      Point text_anchor_pt = xfm.getOffset();

      auto text_anchor = Painter::kBottomCenter;
      if (arg_min == 0) {  // left
        text_anchor = Painter::kRightCenter;
        text_anchor_pt.setX(text_anchor_pt.x() - pin_max_size_ - text_margin);
      } else if (arg_min == 1) {  // right
        text_anchor = Painter::kLeftCenter;
        text_anchor_pt.setX(text_anchor_pt.x() + pin_max_size_ + text_margin);
      } else if (arg_min == 2) {  // top
        text_anchor = Painter::kBottomCenter;
        text_anchor_pt.setY(text_anchor_pt.y() + pin_max_size_ + text_margin);
      } else {  // bottom
        text_anchor = Painter::kTopCenter;
        text_anchor_pt.setY(text_anchor_pt.y() - pin_max_size_ - text_margin);
      }

      PinText pin_specs;
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
        pin_specs.rect = text_rect;
        pin_text_spec_shape_rects.push_back(text_rect);
      }
      pin_text_spec.push_back(std::move(pin_specs));
    }
  }
  if (!pin_text_spec_shape_rects.empty()) {
    pin_text_spec_shapes = PinShapeTree(pin_text_spec_shape_rects.begin(),
                                        pin_text_spec_shape_rects.end());
  }

  painter.setPen(layer);
  auto color = painter.getPenColor();
  color.a = 255;
  painter.setPen(color);
  painter.setBrush(color);

  for (const auto& pin : pin_text_spec) {
    if (restart_) {
      break;
    }

    bool do_rotate = false;
    auto anchor = pin.anchor;
    if (pin.can_rotate && pin.rect) {
      if (pin_text_spec_shapes.qbegin(bgi::intersects(*pin.rect)
                                      && bgi::satisfies([&](const auto& other) {
                                           return !bg::equals(other, *pin.rect);
                                         }))
          != pin_text_spec_shapes.qend()) {
        // adjust anchor
        if (pin.anchor == Painter::kBottomCenter) {
          anchor = Painter::kRightCenter;
        } else if (pin.anchor == Painter::kTopCenter) {
          anchor = Painter::kLeftCenter;
        }
        do_rotate = true;
      }
    }

    painter.drawString(pin.pt.x(), pin.pt.y(), anchor, pin.text, do_rotate);
  }

  qpainter->setFont(initial_font);
}

}  // namespace gui
