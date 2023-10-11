//////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2023, The Regents of the University of California
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

#include "renderThread.h"

#include <QPainterPath>

#include "layoutViewer.h"
#include "odb/dbShape.h"
#include "odb/dbTransform.h"
#include "painter.h"
#include "utl/timer.h"

namespace gui {

using odb::dbBlock;
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
                          const Rulers& rulers)
{
  if (abort_) {
    return;
  }

  QMutexLocker locker(&mutex_);
  draw_rect_ = draw_rect;
  selected_ = selected;
  highlighted_ = highlighted;
  rulers_.clear();
  rulers_.reserve(rulers.size());
  for (const auto& ruler : rulers) {
    rulers_.emplace_back(new Ruler(*ruler));
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
    mutex_.lock();
    const QRect draw_bounds = draw_rect_;
    selected.swap(selected_);
    highlighted.swap(highlighted_);
    rulers.swap(rulers_);
    mutex_.unlock();
    QImage image(draw_bounds.width(),
                 draw_bounds.height(),
                 QImage::Format_ARGB32_Premultiplied);
    // drawing can be interrupted by setting restart_
    draw(image,
         draw_bounds,
         selected,
         highlighted,
         rulers,
         1.0,
         Qt::transparent);
    if (!restart_) {
      emit done(image, draw_bounds);
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

void RenderThread::drawRenderIndication(Painter& painter,
                                        const odb::Rect& bounds)
{
  QPainter* qpainter = static_cast<GuiPainter&>(painter).getPainter();
  const QFont initial_font = qpainter->font();
  QFont indication_render_font = qpainter->font();
  indication_render_font.setPointSize(16);

  qpainter->setFont(indication_render_font);

  std::string rendering_message = "Design loading...";
  painter.setPen(gui::Painter::white, true);
  painter.drawString(
      bounds.xCenter(), bounds.yCenter(), Painter::CENTER, rendering_message);

  qpainter->setFont(initial_font);
}

void RenderThread::draw(QImage& image,
                        const QRect& draw_bounds,
                        const SelectionSet& selected,
                        const HighlightSet& highlighted,
                        const Rulers& rulers,
                        qreal render_ratio,
                        const QColor& background)
{
  if (image.isNull()) {
    return;
  }
  // Prevent a paintEvent and a save_image call from interfering
  // (eg search RTree construction)
  std::lock_guard<std::mutex> lock(drawing_mutex_);
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
                         viewer_->block_->getDbUnitsPerMicron());

  if (!is_first_render_done_ && !restart_) {
    drawRenderIndication(gui_painter, dbu_bounds);
    emit done(image, draw_bounds);
    is_first_render_done_ = true;

    // Erase the first render indication so it does not remain on the screen
    // when the design is drawn for the first time
    image.fill(background);
  }

  drawBlock(&painter, viewer_->block_, dbu_bounds, 0);

  // draw selected and over top level and fast painting events
  drawSelected(gui_painter, selected);
  // Always last so on top
  drawHighlighted(gui_painter, highlighted);
  drawRulers(gui_painter, rulers);
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

  dbTrackGrid* grid = viewer_->block_->findTrackGrid(layer);
  if (!grid) {
    return;
  }

  Rect block_bounds = viewer_->block_->getDieArea();
  if (!block_bounds.intersects(bounds)) {
    return;
  }
  const Rect draw_bounds = block_bounds.intersect(bounds);
  const int min_resolution = viewer_->shapeSizeLimit();

  bool is_horizontal = layer->getDirection() == dbTechLayerDir::HORIZONTAL;
  std::vector<int> grids;
  if ((!is_horizontal && viewer_->options_->arePrefTracksVisible())
      || (is_horizontal && viewer_->options_->areNonPrefTracksVisible())) {
    bool show_grid = true;
    for (int i = 0; i < grid->getNumGridPatternsX(); i++) {
      int origin, line_count, step;
      grid->getGridPatternX(i, origin, line_count, step);
      show_grid &= step > min_resolution;
    }

    if (show_grid) {
      grid->getGridX(grids);
      for (int x : grids) {
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
      grid->getGridY(grids);
      for (int y : grids) {
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

  for (const auto& [row, row_site] : viewer_->getRowRects(block, bounds)) {
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
    selected.highlight(painter, Painter::highlight);
  }

  if (viewer_->focus_) {
    viewer_->focus_.highlight(painter,
                              Painter::highlight,
                              1,
                              Painter::highlight,
                              Painter::Brush::DIAGONAL);
  }
}

void RenderThread::drawHighlighted(Painter& painter,
                                   const HighlightSet& highlighted)
{
  int highlight_group = 0;
  for (auto& highlight_set : highlighted) {
    auto highlight_color = Painter::highlightColors[highlight_group];

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
      int x;
      int y;
      inst->getOrigin(x, y);
      painter->drawPoint(x, y);
    } else {
      // setup the instance's transform
      QTransform xfm = initial_xfm;
      dbTransform inst_xfm;
      inst->getTransform(inst_xfm);
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

    dbBlock* child;
    if (has_child_blocks
        && (child = inst->getBlock()->findChild(master->getName().c_str()))) {
      // setup the instance's transform
      QTransform xfm = initial_xfm;
      dbTransform inst_xfm;
      inst->getTransform(inst_xfm);
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
      for (auto& [box, inst] : inst_range) {
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
    drawTextInBBox(text_color, text_font, instance_box, name, painter, center);
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

    odb::dbTransform xform;
    inst->getTransform(xform);
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
  static const float size_limit = 0.9;
  static const float rotation_limit
      = 0.85;  // slightly lower to prevent oscillating rotations when zooming

  // limit non-core text to 1/2.0 (50%) of cell height or width
  static const float non_core_scale_limit = 2.0;

  const qreal scale_adjust = 1.0 / viewer_->pixels_per_dbu_;

  QRectF bbox_in_px = viewer_->dbuToScreen(bbox);

  QRectF text_bounding_box = font_metrics.boundingRect(name);

  bool do_rotate = false;
  if (text_bounding_box.width() > rotation_limit * bbox_in_px.width()) {
    // non-rotated text will not fit without elide
    if (bbox_in_px.height() > bbox_in_px.width()) {
      // check if more text will fit if rotated
      do_rotate = true;
    }
  }

  qreal text_height_check = non_core_scale_limit * text_bounding_box.height();
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
        name, Qt::ElideLeft, size_limit * bbox_in_px.height());
  } else {
    name = font_metrics.elidedText(
        name, Qt::ElideLeft, size_limit * bbox_in_px.width());
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
      const auto xOffset
          = (bbox_in_px.height() - text_bounding_box.width()) / 2;
      const auto yOffset
          = (bbox_in_px.width() - text_bounding_box.height()) / 2;
      painter->translate(-xOffset, -yOffset);
    }
  } else {
    // account for descent of font
    painter->translate(font_metrics.descent(), 0);
    if (center) {
      const auto xOffset = (bbox_in_px.width() - text_bounding_box.width()) / 2;
      const auto yOffset
          = (bbox_in_px.height() - text_bounding_box.height()) / 2;
      painter->translate(xOffset, -yOffset);
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
                                 const Rect& bounds)
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

  for (auto& [box, blockage] : blockage_range) {
    if (restart_) {
      break;
    }
    Rect bbox = blockage->getBBox()->getBox();
    painter->drawRect(bbox.xMin(), bbox.yMin(), bbox.dx(), bbox.dy());
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

  for (auto& [box, obs] : obstructions_range) {
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
  auto via_sbox_iter = viewer_->search_.searchViaSBoxShapes(block,
                                                            cut_layer,
                                                            bounds.xMin(),
                                                            bounds.yMin(),
                                                            bounds.xMax(),
                                                            bounds.yMax(),
                                                            shape_limit);

  std::vector<odb::dbShape> via_shapes;
  for (auto& [box, sbox, net] : via_sbox_iter) {
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

  if (draw_shapes) {
    drawInstanceShapes(layer, painter, insts, bounds, gui_painter);
  }

  drawObstructions(block, layer, painter, bounds);

  if (draw_shapes) {
    // Now draw the shapes
    QColor color = getColor(layer);
    Qt::BrushStyle brush_pattern = getPattern(layer);
    painter->setBrush(QBrush(color, brush_pattern));
    painter->setPen(QPen(color, 0));
    auto box_iter = viewer_->search_.searchBoxShapes(block,
                                                     layer,
                                                     bounds.xMin(),
                                                     bounds.yMin(),
                                                     bounds.xMax(),
                                                     bounds.yMax(),
                                                     shape_limit);

    for (auto& [box, net] : box_iter) {
      if (restart_) {
        break;
      }
      if (!viewer_->isNetVisible(net)) {
        continue;
      }
      const auto& ll = box.min_corner();
      const auto& ur = box.max_corner();
      painter->drawRect(
          QRect(ll.x(), ll.y(), ur.x() - ll.x(), ur.y() - ll.y()));
    }

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

    auto polygon_iter = viewer_->search_.searchPolygonShapes(block,
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
      const int size = poly.outer().size();
      QPolygon qpoly(size);
      for (int i = 0; i < size; i++) {
        qpoly.setPoint(i, poly.outer()[i].x(), poly.outer()[i].y());
      }
      painter->drawPolygon(qpoly);
    }

    // Now draw the fills
    if (viewer_->options_->areFillsVisible()) {
      QColor color = getColor(layer).lighter(50);
      Qt::BrushStyle brush_pattern = getPattern(layer);
      painter->setBrush(QBrush(color, brush_pattern));
      painter->setPen(QPen(color, 0));
      auto iter = viewer_->search_.searchFills(block,
                                               layer,
                                               bounds.xMin(),
                                               bounds.yMin(),
                                               bounds.xMax(),
                                               bounds.yMax(),
                                               shape_limit);

      for (auto& i : iter) {
        if (restart_) {
          break;
        }
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
    drawNetTracks(gui_painter, layer);
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

// Draw the region of the block.  Depth is not yet used but
// is there for hierarchical design support.
void RenderThread::drawBlock(QPainter* painter,
                             dbBlock* block,
                             const Rect& bounds,
                             int depth)
{
  utl::Timer timer;

  utl::Timer manufacturing_grid_timer;
  const int instance_limit = viewer_->instanceSizeLimit();

  GuiPainter gui_painter(painter,
                         viewer_->options_,
                         bounds,
                         viewer_->pixels_per_dbu_,
                         block->getDbUnitsPerMicron());

  // Draw die area, if set
  painter->setPen(QPen(Qt::gray, 0));
  painter->setBrush(QBrush());
  Rect bbox = block->getDieArea();
  if (bbox.area() > 0) {
    painter->drawRect(bbox.xMin(), bbox.yMin(), bbox.dx(), bbox.dy());
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
  for (auto& [box, inst] : inst_range) {
    if (restart_) {
      break;
    }
    if (viewer_->options_->isInstanceVisible(inst)) {
      insts.push_back(inst);
    }
  }
  debugPrint(logger_, GUI, "draw", 1, "inst search {}", inst_timer);

  utl::Timer insts_outline;
  drawInstanceOutlines(painter, insts);
  debugPrint(logger_, GUI, "draw", 1, "inst outline render {}", insts_outline);

  // draw blockages
  utl::Timer inst_blockages;
  drawBlockages(painter, block, bounds);
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
    drawAccessPoints(gui_painter, insts);
  }
  debugPrint(logger_, GUI, "draw", 1, "access points {}", inst_access_points);

  utl::Timer inst_module_view;
  drawModuleView(painter, insts);
  debugPrint(logger_, GUI, "draw", 1, "module view {}", inst_module_view);

  utl::Timer inst_regions;
  drawRegions(painter, block);
  debugPrint(logger_, GUI, "draw", 1, "regions {}", inst_regions);

  utl::Timer inst_io_pins;
  if (viewer_->options_->areIOPinsVisible()) {
    drawIOPins(gui_painter, block, bounds);
  }
  debugPrint(logger_, GUI, "draw", 1, "io pins {}", inst_io_pins);

  utl::Timer inst_cell_grid;
  drawGCellGrid(painter, bounds);
  debugPrint(logger_, GUI, "draw", 1, "save cell grid {}", inst_cell_grid);

  utl::Timer inst_save_restore;
  for (auto* renderer : Gui::get()->renderers()) {
    if (restart_) {
      break;
    }
    gui_painter.saveState();
    renderer->drawObjects(gui_painter);
    gui_painter.restoreState();
  }
  debugPrint(logger_, GUI, "draw", 1, "renderers {}", inst_save_restore);

  debugPrint(logger_, GUI, "draw", 1, "total render {}", timer);
}

void RenderThread::drawGCellGrid(QPainter* painter, const odb::Rect& bounds)
{
  if (!viewer_->options_->isGCellGridVisible()) {
    return;
  }

  odb::dbGCellGrid* grid = viewer_->block_->getGCellGrid();

  if (grid == nullptr) {
    return;
  }

  const odb::Rect draw_bounds = bounds.intersect(viewer_->block_->getDieArea());

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

  odb::dbTech* tech = viewer_->block_->getDb()->getTech();
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
  painter.setPen(layer);
  painter.setBrush(layer);
  for (auto net : viewer_->route_guides_) {
    if (restart_) {
      break;
    }
    for (auto guide : net->getGuides()) {
      if (guide->getLayer() != layer) {
        continue;
      }
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
                                    const std::vector<odb::dbInst*>& insts)
{
  const int shape_limit = viewer_->shapeSizeLimit();
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
  for (auto term : viewer_->block_->getBTerms()) {
    if (restart_) {
      break;
    }
    for (auto pin : term->getBPins()) {
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

    const auto setting = viewer_->modules_.at(module);

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

void RenderThread::drawIOPins(Painter& painter,
                              odb::dbBlock* block,
                              const odb::Rect& bounds)
{
  auto die_area = block->getDieArea();
  auto die_width = die_area.dx();
  auto die_height = die_area.dy();

  const double scale_factor
      = 0.02;  // 4 Percent of bounds is used to draw pin-markers
  const int die_max_dim
      = std::min(std::max(die_width, die_height), bounds.maxDXDY());
  const double abs_min_dim = 8.0;  // prevent markers from falling apart
  const double max_dim = std::max(scale_factor * die_max_dim, abs_min_dim);

  QPainter* qpainter = static_cast<GuiPainter&>(painter).getPainter();
  const QFont initial_font = qpainter->font();
  QFont marker_font = viewer_->options_->pinMarkersFont();
  const QFontMetrics font_metrics(marker_font);

  QString largest_text;
  for (auto pin : block->getBTerms()) {
    QString current_text = QString::fromStdString(pin->getName());
    if (font_metrics.boundingRect(current_text).width()
        > font_metrics.boundingRect(largest_text).width()) {
      largest_text = current_text;
    }
  }

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
        - std::ceil(max_dim) * viewer_->pixels_per_dbu_;  // in pixels

  int font_size = marker_font.pointSize();
  int largest_text_width = font_metrics.boundingRect(largest_text).width();
  const int drawing_font_size = 6;  // in points

  // when the size is minimum the text won't be drawn
  const int minimum_font_size = drawing_font_size - 1;

  while (largest_text_width > available_space) {
    if (font_size == minimum_font_size) {
      break;
    }
    font_size -= 1;
    marker_font.setPointSize(font_size);
    QFontMetrics current_font_metrics(marker_font);
    largest_text_width
        = current_font_metrics.boundingRect(largest_text).width();
  }

  qpainter->setFont(marker_font);

  // draw names of pins when text height is at least 6 pts
  const bool draw_names = font_size >= drawing_font_size;
  const int text_margin = 2.0 / viewer_->pixels_per_dbu_;

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

  // RTree used to search for overlapping shapes and decide if rotation of
  // text is needed.
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

  for (odb::dbBTerm* term : block->getBTerms()) {
    if (restart_) {
      break;
    }
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

        auto dist_to_left = std::abs(box->xMin() - die_area.xMin());
        auto dist_to_right = std::abs(box->xMax() - die_area.xMax());
        auto dist_to_top = std::abs(box->yMax() - die_area.yMax());
        auto dist_to_bot = std::abs(box->yMin() - die_area.yMin());
        std::vector<int> dists{
            dist_to_left, dist_to_right, dist_to_top, dist_to_bot};
        int arg_min = std::distance(
            dists.begin(), std::min_element(dists.begin(), dists.end()));

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

        // draw marker to indicate signal direction
        painter.drawPolygon(marker);

        if (!viewer_->isNetVisible(term->getNet())) {
          // draw pin's geometry only when it's not
          // already being drawn by its Net
          painter.drawRect(box->getBox());
        }

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
    if (restart_) {
      break;
    }

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
    auto color = painter.getPenColor();
    color.a = 255;
    painter.setPen(color);
    painter.setBrush(color);

    painter.drawString(pin.pt.x(), pin.pt.y(), anchor, pin.text, do_rotate);
  }

  qpainter->setFont(initial_font);
}

}  // namespace gui
