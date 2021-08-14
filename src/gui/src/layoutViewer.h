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
               const std::vector<QLine>& rulers,
               std::function<Selected(const std::any&)> makeSelected,
               QWidget* parent = nullptr);

  void setDb(odb::dbDatabase* db);
  void setLogger(utl::Logger* logger);
  qreal getPixelsPerDBU() { return pixels_per_dbu_; }
  void setScroller(LayoutScroll* scroller);

  // conversion functions
  odb::Rect screenToDBU(const QRect& rect);
  odb::Point screenToDBU(const QPoint& point);
  QRectF dbuToScreen(const odb::Rect& dbu_rect);
  QPointF dbuToScreen(const odb::Point& dbu_point);

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
  void location(qreal x, qreal y);
  void selected(const Selected& selected, bool showConnectivity = false);
  void addSelected(const Selected& selected);
  void addRuler(int x0, int y0, int x1, int y1);

 public slots:
  void zoomIn();
  void zoomIn(const odb::Point& focus, bool do_delta_focus = false);
  void zoomOut();
  void zoomOut(const odb::Point& focus, bool do_delta_focus = false);
  void zoomTo(const odb::Rect& rect_dbu);
  void designLoaded(odb::dbBlock* block);
  void fit();  // fit the whole design in the window

  void selectHighlightConnectedInst(bool selectFlag);
  void selectHighlightConnectedNets(bool selectFlag, bool output, bool input);

  void updateContextMenuItems();
  void showLayoutCustomMenu(QPoint pos);

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
  void setPixelsPerDBU(qreal pixels_per_dbu, bool do_resize = true);
  void drawBlock(QPainter* painter,
                 const odb::Rect& bounds,
                 odb::dbBlock* block,
                 int depth,
                 const QTransform& base_tx);
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
  Selected selectAtPoint(odb::Point pt_dbu);

  void zoom(const odb::Point& focus, qreal factor, bool do_delta_focus);

  qreal computePixelsPerDBU(const QSize& size, const odb::Rect& dbu_rect);
  odb::Rect getPaddedRect(const odb::Rect& rect, double factor = 0.05);

  // Compute and store the offset necessary to center the block in the viewport.
  void computeCenteringOffset();

  int fineViewableResolution();
  int nominalViewableResolution();
  int coarseViewableResolution();

  void generateCutLayerMaximumSizes();

  void addMenuAndActions();
  void updateShapes();

  odb::dbDatabase* db_;
  Options* options_;
  const SelectionSet& selected_;
  const HighlightSet& highlighted_;
  const std::vector<QLine>& rulers_;
  LayoutScroll* scroller_;
  qreal pixels_per_dbu_;
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

  utl::Logger* logger_;
  bool design_loaded_;

  QMenu* layout_context_menu_;
  QMap<CONTEXT_MENU_ACTIONS, QAction*> menu_actions_;

  QPoint centering_shift_;

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

 protected:
  void wheelEvent(QWheelEvent* event) override;

 private:
  LayoutViewer* viewer_;
};

}  // namespace gui
