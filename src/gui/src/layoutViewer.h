///////////////////////////////////////////////////////////////////////////////
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

#pragma once

#include <QFrame>
#include <QLine>
#include <QMainWindow>
#include <QMap>
#include <QMenu>
#include <QOpenGLWidget>
#include <QScrollArea>
#include <QShortcut>
#include <map>
#include <memory>
#include <vector>

#include "gui/gui.h"
#include "odb/dbBlockCallBackObj.h"
#include "options.h"
#include "search.h"

namespace utl {
class Logger;
}

namespace odb {
class dbBlock;
class dbDatabase;
class dbInst;
class dbMaster;
class dbTransform;
class dbTechLayer;
}  // namespace odb

namespace gui {

class LayoutScroll;
class Ruler;

// This class draws the layout.  It supports:
//   * zoom in/out with ctrl-mousewheel
//   * rubber band zoom with right mouse button
//   * fit with 'F' key
// The display follows the display options for visibility.
//
// This object resizes with zooming but only the visible
// portion of this widget is ever drawn.
class LayoutViewer : public QWidget, public odb::dbBlockCallBackObj
{
  Q_OBJECT

 public:
  enum CONTEXT_MENU_ACTIONS
  {
    SELECT_CONNECTED_INST_ACT,
    SELECT_OUTPUT_NETS_ACT,
    SELECT_INPUT_NETS_ACT,
    SELECT_ALL_NETS_ACT,

    HIGHLIGHT_CONNECTED_INST_ACT,
    HIGHLIGHT_OUTPUT_NETS_ACT,
    HIGHLIGHT_INPUT_NETS_ACT,
    HIGHLIGHT_ALL_NETS_ACT,

    VIEW_ZOOMIN_ACT,
    VIEW_ZOOMOUT_ACT,
    VIEW_ZOOMFIT_ACT,

    SAVE_WHOLE_IMAGE_ACT,
    SAVE_VISIBLE_IMAGE_ACT,

    CLEAR_SELECTIONS_ACT,
    CLEAR_HIGHLIGHTS_ACT,
    CLEAR_RULERS_ACT,
    CLEAR_ALL_ACT
  };
  // makeSelected is so that we don't have to pass in the whole
  // MainWindow just to get access to one method.  Communication
  // should happen through signals & slots in all other cases.
  LayoutViewer(Options* options,
               const SelectionSet& selected,
               const HighlightSet& highlighted,
               const std::vector<std::unique_ptr<Ruler>>& rulers,
               std::function<Selected(const std::any&)> makeSelected,
               QWidget* parent = nullptr);

  void setDb(odb::dbDatabase* db);
  void setLogger(utl::Logger* logger);
  qreal getPixelsPerDBU() { return pixels_per_dbu_; }
  void setScroller(LayoutScroll* scroller);

  // conversion functions
  odb::Rect screenToDBU(const QRectF& rect);
  odb::Point screenToDBU(const QPointF& point);
  QRectF dbuToScreen(const odb::Rect& dbu_rect);
  QPointF dbuToScreen(const odb::Point& dbu_point);

  // save image of the layout
  void saveImage(const QString& filepath, const odb::Rect& rect = odb::Rect());

  // From QWidget
  virtual void paintEvent(QPaintEvent* event) override;
  virtual void resizeEvent(QResizeEvent* event) override;
  virtual void mousePressEvent(QMouseEvent* event) override;
  virtual void mouseMoveEvent(QMouseEvent* event) override;
  virtual void mouseReleaseEvent(QMouseEvent* event) override;

  // From dbBlockCallBackObj
  virtual void inDbNetDestroy(odb::dbNet* net) override;
  virtual void inDbInstDestroy(odb::dbInst* inst) override;
  virtual void inDbInstSwapMasterAfter(odb::dbInst* inst) override;
  virtual void inDbInstPlacementStatusBefore(
      odb::dbInst* inst,
      const odb::dbPlacementStatus& status) override;
  virtual void inDbPostMoveInst(odb::dbInst* inst) override;
  virtual void inDbBPinDestroy(odb::dbBPin* pin) override;
  virtual void inDbFillCreate(odb::dbFill* fill) override;
  virtual void inDbWireCreate(odb::dbWire* wire) override;
  virtual void inDbWireDestroy(odb::dbWire* wire) override;
  virtual void inDbSWireCreate(odb::dbSWire* wire) override;
  virtual void inDbSWireDestroy(odb::dbSWire* wire) override;
  virtual void inDbBlockSetDieArea(odb::dbBlock* block) override;
  virtual void inDbBlockageCreate(odb::dbBlockage* blockage) override;
  virtual void inDbObstructionCreate(odb::dbObstruction* obs) override;
  virtual void inDbObstructionDestroy(odb::dbObstruction* obs) override;

 signals:
  // indicates the current location of the mouse
  void location(qreal x, qreal y);

  // indicates a new object has been selected
  void selected(const Selected& selected, bool showConnectivity = false);

  // add additional object to selected set
  void addSelected(const Selected& selected);

  // add new ruler
  void addRuler(int x0, int y0, int x1, int y1);

 public slots:
  // zoom in the layout, keeping the current center_
  void zoomIn();

  // zoom in and change center to the focus point
  // do_delta_focus indicates (if true) that the center of the layout should zoom in around the focus
  // instead of making it the new center. This is used when scrolling with the mouse to keep the
  // mouse point steady in the layout
  void zoomIn(const odb::Point& focus, bool do_delta_focus = false);

  // zoom out the layout, keeping the current center_
  void zoomOut();

  // zoom out and change center to the focus point
  // do_delta_focus indicates (if true) that the center of the layout should zoom in around the focus
  // instead of making it the new center. This is used when scrolling with the mouse to keep the
  // mouse point steady in the layout
  void zoomOut(const odb::Point& focus, bool do_delta_focus = false);

  // zoom to the specified rect
  void zoomTo(const odb::Rect& rect_dbu);

  // indicates a design has been loaded
  void designLoaded(odb::dbBlock* block);

  // fit the whole design in the window
  void fit();

  // center layout at the focus point
  void centerAt(const odb::Point& focus);

  // indicate that the center has changed, due to the scrollarea changing
  void updateCenter(int dx, int dy);

  // set the layout resolution
  void setResolution(qreal dbu_per_pixel);

  // update the fit resolution (the maximum pixels_per_dbu without scroll bars)
  void viewportUpdated();

  void selectHighlightConnectedInst(bool selectFlag);
  void selectHighlightConnectedNets(bool selectFlag, bool output, bool input);

  void updateContextMenuItems();
  void showLayoutCustomMenu(QPoint pos);

  void startRulerBuild();
  void cancelRulerBuild();

 private:
  struct Boxes
  {
    std::vector<QRect> obs;
    std::vector<QRect> mterms;
  };

  struct GCellData
  {
    int hor_capacity_;
    int hor_usage_;
    int ver_capacity_;
    int ver_usage_;

    GCellData(int h_cap = 0, int h_usage = 0, int v_cap = 0, int v_usage = 0)
        : hor_capacity_(h_cap),
          hor_usage_(h_usage),
          ver_capacity_(v_cap),
          ver_usage_(v_usage)
    {
    }
  };
  using LayerBoxes = std::map<odb::dbTechLayer*, Boxes>;
  using CellBoxes = std::map<odb::dbMaster*, LayerBoxes>;

  void boxesByLayer(odb::dbMaster* master, LayerBoxes& boxes);
  const Boxes* boxesByLayer(odb::dbMaster* master, odb::dbTechLayer* layer);
  odb::dbBlock* getBlock();
  void setPixelsPerDBU(qreal pixels_per_dbu);
  void drawBlock(QPainter* painter,
                 const odb::Rect& bounds,
                 odb::dbBlock* block,
                 int depth);
  void addInstTransform(QTransform& xfm, const odb::dbTransform& inst_xfm);
  QColor getColor(odb::dbTechLayer* layer);
  Qt::BrushStyle getPattern(odb::dbTechLayer* layer);
  void updateRubberBandRegion();
  void drawTracks(odb::dbTechLayer* layer,
                  odb::dbBlock* block,
                  QPainter* painter,
                  const odb::Rect& bounds);

  void drawInstanceOutlines(QPainter* painter,
                            const std::vector<odb::dbInst*>& insts);
  void drawInstanceShapes(odb::dbTechLayer* layer,
                          QPainter* painter,
                          const std::vector<odb::dbInst*>& insts);
  void drawInstanceNames(QPainter* painter,
                         const std::vector<odb::dbInst*>& insts);
  void drawBlockages(QPainter* painter,
                     const odb::Rect& bounds);
  void drawObstructions(odb::dbTechLayer* layer,
                        QPainter* painter,
                        const odb::Rect& bounds);
  void drawRows(odb::dbBlock* block,
                QPainter* painter,
                const odb::Rect& bounds);
  void drawSelected(Painter& painter);
  void drawHighlighted(Painter& painter);
  void drawCongestionMap(Painter& painter, const odb::Rect& bounds);
  void drawPinMarkers(QPainter* painter,
                      const odb::Rect& bounds,
                      odb::dbBlock* block);
  void drawRulers(Painter& painter);
  void drawScaleBar(QPainter* painter, odb::dbBlock* block, const QRect& rect);
  Selected selectAtPoint(odb::Point pt_dbu);

  void zoom(const odb::Point& focus, qreal factor, bool do_delta_focus);

  qreal computePixelsPerDBU(const QSize& size, const odb::Rect& dbu_rect);
  odb::Rect getPaddedRect(const odb::Rect& rect, double factor = 0.05);

  odb::Point getVisibleCenter();

  int fineViewableResolution();
  int nominalViewableResolution();
  int coarseViewableResolution();

  void generateCutLayerMaximumSizes();

  void addMenuAndActions();
  void updateShapes();

  using Edge = std::pair<odb::Point, odb::Point>;
  struct Edges {
    Edge horizontal;
    Edge vertical;
  };
  // search for nearest edge to point
  std::pair<Edge, bool> findEdge(const odb::Point& pt, bool horizontal);
  std::pair<Edges, bool> searchNearestEdge(const std::vector<Search::Box>& boxes, const odb::Point& pt);

  odb::Point findNextSnapPoint(const odb::Point& end_pt, bool snap = true);
  odb::Point findNextSnapPoint(const odb::Point& end_pt, const odb::Point& start_pt, bool snap = true);

  odb::Point findNextRulerPoint(const odb::Point& mouse);

  odb::dbDatabase* db_;
  Options* options_;
  const SelectionSet& selected_;
  const HighlightSet& highlighted_;
  const std::vector<std::unique_ptr<Ruler>>& rulers_;
  LayoutScroll* scroller_;

  // holds the current resolution for drawing the layout (units are pixels / dbu)
  qreal pixels_per_dbu_;

  // holds the resolution for drawing the layout where the whole layout fits in the window
  // (units are pixels / dbu)
  qreal fit_pixels_per_dbu_;
  int min_depth_;
  int max_depth_;
  Search search_;
  bool search_init_;
  CellBoxes cell_boxes_;
  QRect rubber_band_;  // screen coordinates
  QPoint mouse_press_pos_;
  QPoint mouse_move_pos_;
  bool rubber_band_showing_;
  std::function<Selected(const std::any&)> makeSelected_;

  bool building_ruler_;
  std::unique_ptr<odb::Point> ruler_start_;

  bool snap_edge_showing_;
  Edge snap_edge_;

  utl::Logger* logger_;
  bool design_loaded_;

  QMenu* layout_context_menu_;
  QMap<CONTEXT_MENU_ACTIONS, QAction*> menu_actions_;

  // shift required when drawing the layout to center the layout in the window (units: pixels)
  QPoint centering_shift_;

  // The center point of the layout visible in the window (in dbu).
  // this is needed to keep track of the center without a
  // dependence on the resolution
  odb::Point center_;

  // Cache of the maximum cut size per layer (units: dbu).
  // Used to determine when cuts are too small to be seen and should not be drawn.
  std::map<odb::dbTechLayer*, int> cut_maximum_size_;

  static constexpr qreal zoom_scale_factor_ = 1.2;

  const QColor background_ = Qt::black;
};

// The LayoutViewer widget can become quite large as you zoom
// in so it is stored in a scroll area.
class LayoutScroll : public QScrollArea
{
  Q_OBJECT
 public:
  LayoutScroll(LayoutViewer* viewer, QWidget* parent = 0);

 signals:
  // indicates that the viewport (visible area of the layout) has changed
  void viewportChanged();

  // indicates how far the viewport of the layout has shifted due to a resize event
  // or by the user manipulating the scrollbars
  void centerChanged(int dx, int dy);

 protected:
  void resizeEvent(QResizeEvent* event) override;
  void scrollContentsBy(int dx, int dy) override;
  void wheelEvent(QWheelEvent* event) override;

 private:
  LayoutViewer* viewer_;
};

}  // namespace gui
