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
#include <QFont>
#include <QGridLayout>
#include <QHBoxLayout>
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
             const QTransform& base_transform,
             int dbu_height,
             qreal pixels_per_dbu,
             int dbu_per_micron)
      : painter_(painter),
        options_(options),
        base_transform_(base_transform),
        dbu_height_(dbu_height),
        pixels_per_dbu_(pixels_per_dbu),
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
    QPen pen(options_->color(layer));
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
    QColor color = options_->color(layer);
    Qt::BrushStyle brush_pattern = options_->pattern(layer);
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
      QPolygon qpoly(size);
      for (int i = 0; i < size; i++)
        qpoly.setPoint(i, points[i].getX(), points[i].getY());
      painter_->drawPolygon(qpoly);
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
  void drawLine(const odb::Point& p1, const odb::Point& p2) override
  {
    painter_->drawLine(p1.x(), p1.y(), p2.x(), p2.y());
  }
  using Painter::drawLine;

  void setTransparentBrush() override { painter_->setBrush(Qt::transparent); }
  void drawCircle(int x, int y, int r) override
  {
    painter_->drawEllipse(QPoint(x, y), r, r);
  }

  // NOTE: The constant height text s drawn with this function, hence
  //       the trasnsformation is mapped to the base transformation and
  //       the world co-ordinates are mapped to the window co-ordinates
  //       before drawing.
  void drawString(int x, int y, int offset, const std::string& s) override
  {
    painter_->save();
    painter_->setTransform(base_transform_);
    int sx = x * pixels_per_dbu_;
    int sy = (dbu_height_ - y) * pixels_per_dbu_;
    painter_->setPen(QPen(Qt::white, 0));
    painter_->setBrush(QBrush());
    painter_->drawText(sx, sy, QString::fromStdString(s));
    painter_->restore();
  }

  void drawRuler(int x0, int y0, int x1, int y1) override
  {
    setPen(ruler_color, true);
    setBrush(ruler_color);

    std::stringstream ss;
    const int x_len = std::abs(x0 - x1);
    const int y_len = std::abs(y0 - y1);

    ss << std::fixed << std::setprecision(2)
       << std::max(x_len, y_len) / (qreal) dbu_per_micron_;

    drawLine(x0, y0, x1, y1);

    if (x_len < y_len) {
      drawString(x0, (y0 + y1) / 2, 0, ss.str());
    } else {
      drawString((x0 + x1) / 2, y0, 0, ss.str());
    }
  }

 private:
  QPainter* painter_;
  Options* options_;
  const QTransform base_transform_;
  int dbu_height_;
  qreal pixels_per_dbu_;
  int dbu_per_micron_;
};

LayoutViewer::LayoutViewer(
    Options* options,
    const SelectionSet& selected,
    const HighlightSet& highlighted,
    const std::vector<QLine>& rulers,
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

void LayoutViewer::setPixelsPerDBU(qreal pixels_per_dbu)
{
  pixels_per_dbu_ = pixels_per_dbu;
  dbBlock* block = getBlock();
  if (!block) {
    return;
  }

  Rect bbox = getBounds(block);

  QSize size(ceil(bbox.xMax() * pixels_per_dbu),
             ceil(bbox.yMax() * pixels_per_dbu));
  resize(size);
  setMinimumSize(size);  // needed by scroll area
  update();
}

void LayoutViewer::zoomIn()
{
  setPixelsPerDBU(pixels_per_dbu_ * 1.2);
}

void LayoutViewer::zoomOut()
{
  setPixelsPerDBU(pixels_per_dbu_ / 1.2);
}

void LayoutViewer::zoomTo(const Rect& rect_dbu)
{
  QSize viewport = scroller_->maximumViewportSize();
  qreal pixels_per_dbu = std::min(viewport.width() / (double) rect_dbu.dx(),
                                  viewport.height() / (double) rect_dbu.dy());
  setPixelsPerDBU(pixels_per_dbu);

  QRectF screen_rect = dbuToScreen(rect_dbu);

  // Center the region
  int w = (scroller_->width() - screen_rect.width()) / 2;
  int h = (scroller_->height() - screen_rect.height()) / 2;

  scroller_->horizontalScrollBar()->setValue(screen_rect.left() - w);
  scroller_->verticalScrollBar()->setValue(screen_rect.top() - h);
}

void LayoutViewer::updateRubberBandRegion()
{
  QRect rect = rubber_band_.normalized();
  int unit = ceil(2 / pixels_per_dbu_);
  update(rect.left(), rect.top() - unit / 2, rect.width(), unit);
  update(rect.left() - unit / 2, rect.top(), unit, rect.height());
  update(rect.left(), rect.bottom() - unit / 2, rect.width(), unit);
  update(rect.right() - unit / 2, rect.top(), unit, rect.height());
}

Selected LayoutViewer::selectAtPoint(odb::Point pt_dbu)
{
  // Look for the selected object in reverse layer order
  auto& renderers = Gui::get()->renderers();
  dbTech* tech = getBlock()->getDataBase()->getTech();
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
      Selected selected = renderer->select(layer, pt_dbu);
      if (selected) {
        return selected;
      }
    }

    auto shapes = search_.searchShapes(
        layer, pt_dbu.x(), pt_dbu.y(), pt_dbu.x(), pt_dbu.y());

    // Just return the first one
    for (auto iter : shapes) {
      dbNet* net = std::get<2>(iter);
      if (options_->isNetVisible(net)) {
        return makeSelected_(net);
      }
    }
  }

  // Check for objects not in a layer
  for (auto* renderer : renderers) {
    Selected selected = renderer->select(nullptr, pt_dbu);
    if (selected) {
      return selected;
    }
  }

  // Look for an instance since no shape was found
  auto insts
      = search_.searchInsts(pt_dbu.x(), pt_dbu.y(), pt_dbu.x(), pt_dbu.y());

  // Just return the first one

  if (insts.begin() != insts.end()) {
    return makeSelected_(std::get<2>(*insts.begin()));
  }
  return Selected();
}

void LayoutViewer::mousePressEvent(QMouseEvent* event)
{
  odb::dbBlock* block = getBlock();
  if (block == nullptr) {
    return;
  }
  
  int dbu_height = getBounds(block).yMax();
  mouse_press_pos_ = event->pos();
  if (event->button() == Qt::LeftButton) {
    if (getBlock()) {
      Point pt_dbu = screenToDBU(event->pos());
      if (qGuiApp->keyboardModifiers() & Qt::ShiftModifier) {
        emit addSelected(selectAtPoint(pt_dbu));
      } else if (qGuiApp->keyboardModifiers() & Qt::ControlModifier) {
        emit selected(selectAtPoint(pt_dbu), true);
      } else {
        emit selected(selectAtPoint(pt_dbu), false);
      }
    }
  } else if (event->button() == Qt::RightButton) {
    rubber_band_showing_ = true;
    rubber_band_.setTopLeft(event->pos());
    rubber_band_.setBottomRight(event->pos());
    updateRubberBandRegion();
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
  if (rubber_band_showing_) {
    updateRubberBandRegion();
    rubber_band_.setBottomRight(event->pos());
    updateRubberBandRegion();
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
    updateRubberBandRegion();
    unsetCursor();
    QRect rect = rubber_band_.normalized();
    if (!(QApplication::keyboardModifiers() & Qt::ControlModifier)
        && (rect.width() < 4 || rect.height() < 4)) {
      showLayoutCustomMenu(event->pos());
      return;  // ignore clicks not intended to be drags
    }

    Rect rubber_band_dbu = screenToDBU(rect);
    // Clip to the block bounds
    Rect bbox = getBounds(block);

    rubber_band_dbu.set_xlo(qMax(rubber_band_dbu.xMin(), bbox.xMin()));
    rubber_band_dbu.set_ylo(qMax(rubber_band_dbu.yMin(), bbox.yMin()));
    rubber_band_dbu.set_xhi(qMin(rubber_band_dbu.xMax(), bbox.xMax()));
    rubber_band_dbu.set_yhi(qMin(rubber_band_dbu.yMax(), bbox.yMax()));

    if (QApplication::keyboardModifiers() & Qt::ControlModifier) {
      if (rect.width() < 10 && rect.height() < 10)
        return;
      int dbu_height = getBounds(block).yMax();
      auto mouse_release_pos = screenToDBU(event->pos());
      auto mouse_press_pos = screenToDBU(mouse_press_pos_);
      qreal to_dbu = block->getDbUnitsPerMicron();

      QLine ruler;
      if (rubber_band_dbu.dx() > rubber_band_dbu.dy()) {
        QPoint pt1 = QPoint(mouse_press_pos.x(), mouse_press_pos.y());
        QPoint pt2(mouse_release_pos.x(), pt1.y());
        if (pt1.x() < pt2.x())
          ruler = QLine(pt1, pt2);
        else
          ruler = QLine(pt2, pt1);
      } else {
        QPoint pt1 = QPoint(mouse_press_pos.x(), mouse_press_pos.y());
        QPoint pt2(pt1.x(), mouse_release_pos.y());
        if (pt1.y() < pt2.y())
          ruler = QLine(pt1, pt2);
        else
          ruler = QLine(pt2, pt1);
      }
      emit addRuler(
          ruler.p1().x(), ruler.p1().y(), ruler.p2().x(), ruler.p2().y());
      return;
    }
    zoomTo(rubber_band_dbu);
  }
}

void LayoutViewer::resizeEvent(QResizeEvent* event)
{
  dbBlock* block = getBlock();
  if (block) {
    Rect bbox = getBounds(block);
    pixels_per_dbu_ = std::min(event->size().width() / (double) bbox.xMax(),
                               event->size().height() / (double) bbox.yMax());
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
      xfm.scale(-1, 1);
      xfm.rotate(90);
      break;
    case dbOrientType::MX:
      xfm.scale(1, -1);
      break;
    case dbOrientType::MXR90:
      xfm.scale(1, -1);
      xfm.rotate(90);
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

  bool is_horizontal = layer->getDirection() == dbTechLayerDir::HORIZONTAL;
  std::vector<int> grids;
  if ((!is_horizontal && options_->arePrefTracksVisible())
      || (is_horizontal && options_->areNonPrefTracksVisible())) {
    grid->getGridX(grids);
    for (int x : grids) {
      if (x < bounds.xMin()) {
        continue;
      }
      if (x > bounds.xMax()) {
        break;
      }
      painter->drawLine(x, bounds.yMin(), x, bounds.yMax());
    }
  }

  if ((is_horizontal && options_->arePrefTracksVisible())
      || (!is_horizontal && options_->areNonPrefTracksVisible())) {
    grid->getGridY(grids);
    for (int y : grids) {
      if (y < bounds.yMin()) {
        continue;
      }
      if (y > bounds.yMax()) {
        break;
      }
      painter->drawLine(bounds.xMin(), y, bounds.xMax(), y);
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
  QPen pen(QColor(0, 0xff, 0, 0x70));
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
    for (int i = 0; i < count; ++i) {
      painter->drawRect(QRect(QPoint(x, y), QPoint(x + w, y + h)));
      if (dir == dbRowDir::HORIZONTAL) {
        x += spacing;
      } else {
        y += spacing;
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
  for (auto& ruler : rulers_) {
    painter.drawRuler(
        ruler.p1().x(), ruler.p1().y(), ruler.p2().x(), ruler.p2().y());
  }
}

void LayoutViewer::drawCongestionMap(Painter& painter, const odb::Rect& bounds)
{
  auto block = getBlock();
  if (block == nullptr)
    return;
  auto grid = block->getGCellGrid();
  if (grid == nullptr)
    return;
  std::vector<int> x_grid, y_grid;
  uint x_grid_sz, y_grid_sz;
  grid->getGridX(x_grid);
  x_grid_sz = x_grid.size();
  grid->getGridY(y_grid);
  y_grid_sz = y_grid.size();
  auto gcell_congestion_data = grid->getCongestionMap();

  if (!options_->isCongestionVisible() || gcell_congestion_data.empty())
    return;

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

// Draw the region of the block.  Depth is not yet used but
// is there for hierarchical design support.
void LayoutViewer::drawBlock(QPainter* painter,
                             const Rect& bounds,
                             dbBlock* block,
                             int depth,
                             const QTransform& base_tx)
{
  int pixel = 1 / pixels_per_dbu_;  // 1 pixel in DBU
  LayerBoxes boxes;
  QTransform initial_xfm = painter->transform();

  auto& renderers = Gui::get()->renderers();
  GuiPainter gui_painter(painter,
                         options_,
                         base_tx,
                         getBounds(block).yMax(),
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
      bounds.xMin(), bounds.yMin(), bounds.xMax(), bounds.yMax(), 1 * pixel);

  // Cache the search results as we will iterate over the instances
  // for each layer.
  std::vector<dbInst*> insts;
  insts.reserve(10000);
  for (auto& [box, poly, inst] : inst_range) {
    insts.push_back(inst);
  }

  // Draw the instances bounds
  for (auto inst : insts) {
    dbMaster* master = inst->getMaster();
    // setup the instance's transform
    QTransform xfm = painter->transform();
    dbTransform inst_xfm;
    inst->getTransform(inst_xfm);
    addInstTransform(xfm, inst_xfm);
    painter->setTransform(xfm);

    // draw bbox
    painter->setPen(QPen(Qt::gray, 0));
    painter->setBrush(QBrush());
    Rect master_box;
    master->getPlacementBoundary(master_box);
    painter->drawRect(
        master_box.xMin(), master_box.yMin(), master_box.dx(), master_box.dy());

    // Draw an orientation tag in corner if useful in size
    int master_h = master->getHeight();
    if (master_h >= 5 * pixel) {
      qreal master_w = master->getWidth();
      qreal tag_size = 0.1 * master_h;
      qreal tag_x = master_box.xMin() + std::min(tag_size / 2, master_w);
      qreal tag_y = master_box.yMin() + tag_size;
      painter->drawLine(QPointF(tag_x, master_box.yMin()),
                        QPointF(master_box.xMin(), tag_y));
    }
    painter->setTransform(initial_xfm);
  }

  dbTech* tech = block->getDataBase()->getTech();
  for (dbTechLayer* layer : tech->getLayers()) {
    if (!options_->isVisible(layer)) {
      continue;
    }

    // Skip the cut layer if the cuts will be too small to see
    const bool is_cut = layer->getType() == dbTechLayerType::CUT;
    if (is_cut && layer->getWidth() < 1 * pixel) {
      continue;
    }

    // Draw the instances' shapes
    for (auto inst : insts) {
      dbMaster* master = inst->getMaster();
      if (master->getHeight() < 5 * pixel) {
        continue;
      }

      const Boxes* boxes = boxesByLayer(master, layer);

      if (boxes == nullptr) {
        continue;  // no shapes on this layer
      }

      // setup the instance's transform
      QTransform xfm = painter->transform();
      dbTransform inst_xfm;
      inst->getTransform(inst_xfm);
      addInstTransform(xfm, inst_xfm);
      painter->setTransform(xfm);

      // Only draw the pins/obs if they are big enough to be useful
      painter->setPen(Qt::NoPen);
      QColor color = getColor(layer);
      Qt::BrushStyle brush_pattern = getPattern(layer);
      painter->setBrush(QBrush(color, brush_pattern));

      painter->setBrush(color.lighter());
      for (auto& box : boxes->obs) {
        painter->drawRect(box);
      }

      painter->setBrush(QBrush(color, brush_pattern));
      for (auto& box : boxes->mterms) {
        painter->drawRect(box);
      }

#if 0
        // TODO
        // draw text
        painter->setPen(QPen(Qt::yellow, 0));
        painter->setBrush(QBrush(Qt::yellow));
        auto name = master->getName();
        qreal font_scale = master_w
            / painter->fontMetrics().horizontalAdvance(name.c_str());

        font_scale = std::min(font_scale, 5000.0);
        QFont f = painter->font();
        f.setPointSizeF(f.pointSize() * pixels_per_dbu_);
        painter->setFont(f);

        painter->scale(1, -1);
        painter->drawText(0, -master_h, master_w, master_h,
                          Qt::AlignCenter, name.c_str());
#endif

      painter->setTransform(initial_xfm);
    }

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
                                     5 * pixel);

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
                                      5 * pixel);

      for (auto& i : iter) {
        const auto& ll = std::get<0>(i).min_corner();
        const auto& ur = std::get<0>(i).max_corner();
        int w = ur.x() - ll.x();
        int h = ur.y() - ll.y();
        painter->drawRect(
            QRect(QPoint(ll.x(), ll.y()), QPoint(ur.x(), ur.y())));
      }
    }

    drawTracks(layer, block, painter, bounds);
    for (auto* renderer : renderers) {
      renderer->drawLayer(layer, gui_painter);
    }
  }

  drawRows(block, painter, bounds);
  for (auto* renderer : renderers) {
    renderer->drawObjects(gui_painter);
  }

  drawCongestionMap(gui_painter, bounds);

  drawSelected(gui_painter);
  // Always last so on top
  drawHighlighted(gui_painter);
  drawRulers(gui_painter);
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

odb::Point LayoutViewer::screenToDBU(const QPoint& point)
{
  return Point(point.x() / pixels_per_dbu_,
               (height() - point.y()) / pixels_per_dbu_);
}

Rect LayoutViewer::screenToDBU(const QRect& screen_rect)
{
  int dbu_left = (int) floor(screen_rect.left() / pixels_per_dbu_);
  int dbu_right = (int) ceil(screen_rect.right() / pixels_per_dbu_);
  int dbu_top = (int) floor(screen_rect.top() / pixels_per_dbu_);
  int dbu_bottom = (int) ceil(screen_rect.bottom() / pixels_per_dbu_);

  // Flip the y-coordinate (see file level comments)
  dbBlock* block = getBlock();
  int dbu_height = getBounds(block).yMax();
  dbu_top = dbu_height - dbu_top;
  dbu_bottom = dbu_height - dbu_bottom;

  return Rect(dbu_left, dbu_bottom, dbu_right, dbu_top);
}

QRectF LayoutViewer::dbuToScreen(const Rect& dbu_rect)
{
  dbBlock* block = getBlock();
  int dbu_height = getBounds(block).yMax();

  // Flip the y-coordinate (see file level comments)
  qreal screen_left = dbu_rect.xMin() * pixels_per_dbu_;
  qreal screen_right = dbu_rect.xMax() * pixels_per_dbu_;
  qreal screen_top = (dbu_height - dbu_rect.yMax()) * pixels_per_dbu_;
  qreal screen_bottom = (dbu_height - dbu_rect.yMin()) * pixels_per_dbu_;

  return QRectF(QPointF(screen_left, screen_top),
                QPointF(screen_right, screen_bottom));
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
  painter.setPen(QPen(Qt::black, 0));
  painter.setBrush(Qt::black);
  painter.drawRect(event->rect());

  if (!design_loaded_) {
    return;
  }

  if (!search_init_) {
    search_.init(block);
    search_init_ = true;
  }

  // Coordinate system setup (see file level comments)
  const QTransform base_transform = painter.transform();
  painter.save();
  painter.translate(0, height());
  painter.scale(pixels_per_dbu_, -pixels_per_dbu_);

  Rect dbu_bounds = screenToDBU(event->rect());
  drawBlock(&painter, dbu_bounds, block, 0, base_transform);
  if (options_->arePinMarkersVisible())
    drawPinMarkers(&painter, dbu_bounds, block);

  painter.restore();

  if (rubber_band_showing_) {
    painter.setPen(QPen(Qt::white, 0));
    painter.setBrush(QBrush());
    if (QApplication::keyboardModifiers() & Qt::ControlModifier) {
      auto norm_rect = rubber_band_.normalized();
      QLine ruler;
      if (norm_rect.width() > norm_rect.height()) {
        if (mouse_press_pos_.y() > mouse_move_pos_.y())
          ruler = QLine(norm_rect.bottomLeft(), norm_rect.bottomRight());
        else
          ruler = QLine(norm_rect.topLeft(), norm_rect.topRight());
        painter.drawLine(ruler);
      } else {
        if (mouse_press_pos_.x() > mouse_move_pos_.x())
          ruler = QLine(norm_rect.topRight(), norm_rect.bottomRight());
        else
          ruler = QLine(norm_rect.topLeft(), norm_rect.bottomLeft());

        painter.drawLine(ruler);
      }
    } else {
      painter.drawRect(rubber_band_.normalized());
    }
    return;
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
  update();
}

void LayoutViewer::fit()
{
  dbBlock* block = getBlock();
  if (block == nullptr) {
    return;
  }

  Rect bbox = getBounds(block);
  if (bbox.xMax() == 0 || bbox.yMax() == 0) {
    return;
  }

  QSize viewport = scroller_->maximumViewportSize();
  qreal pixels_per_dbu
      = std::min((viewport.width() * 0.98) / (double) bbox.xMax(),
                 (viewport.height() * 0.98) / (double) bbox.yMax());
  setPixelsPerDBU(pixels_per_dbu);
  fit_pixels_per_dbu_ = pixels_per_dbu;
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
  if (QApplication::keyboardModifiers() & Qt::ControlModifier)
    return;
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
}

void LayoutViewer::addMenuAndActions()
{
  // Create Top Level Menu for the context Menu
  auto select_menu = layout_context_menu_->addMenu(tr("Select"));
  auto highlight_menu = layout_context_menu_->addMenu(tr("Highlight"));
  auto congestion_menu = layout_context_menu_->addMenu(tr("Congestion"));
  auto view_menu = layout_context_menu_->addMenu(tr("View"));
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
  setWidgetResizable(true);
  setWidget(viewer);
  viewer->setScroller(this);
}

// Handles zoom in/out on ctrl-wheel
void LayoutScroll::wheelEvent(QWheelEvent* event)
{
  if (!event->modifiers().testFlag(Qt::ControlModifier)) {
    QScrollArea::wheelEvent(event);
    return;
  }

  if (event->angleDelta().y() > 0) {
    zoomIn();
  } else {
    zoomOut();
  }
}

void LayoutScroll::zoomIn()
{
  qreal old_pixels_per_dbu = viewer_->getPixelsPerDBU();

  int scrollbar_x = horizontalScrollBar()->value();
  int scrollbar_y = verticalScrollBar()->value();
  QPointF pos_in_widget = mapFromGlobal(QCursor::pos()) - widget()->pos();

  viewer_->zoomIn();

  qreal new_pixels_per_dbu = viewer_->getPixelsPerDBU();
  QPointF delta = (new_pixels_per_dbu / old_pixels_per_dbu - 1) * pos_in_widget;

  horizontalScrollBar()->setValue(scrollbar_x + delta.x());
  verticalScrollBar()->setValue(scrollbar_y + delta.y());
}

void LayoutScroll::zoomOut()
{
  qreal old_pixels_per_dbu = viewer_->getPixelsPerDBU();

  int scrollbar_x = horizontalScrollBar()->value();
  int scrollbar_y = verticalScrollBar()->value();
  QPointF pos_in_widget = mapFromGlobal(QCursor::pos()) - widget()->pos();

  viewer_->zoomOut();

  qreal new_pixels_per_dbu = viewer_->getPixelsPerDBU();
  QPointF delta = (new_pixels_per_dbu / old_pixels_per_dbu - 1) * pos_in_widget;

  horizontalScrollBar()->setValue(scrollbar_x + delta.x());
  verticalScrollBar()->setValue(scrollbar_y + delta.y());
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

void LayoutViewer::inDbBlockSetDieArea(odb::dbBlock* block)
{
  // This happens when initialize_floorplan is run and it make sense
  // to fit as current zoom will be on a zero sized block.
  fit();
}

}  // namespace gui
