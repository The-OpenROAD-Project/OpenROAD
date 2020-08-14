///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2019, OpenROAD
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

#include <QMainWindow>
#include <QOpenGLWidget>
#include <QScrollArea>
#include <map>
#include <vector>

#include "options.h"
#include "search.h"

namespace odb {
class dbBlock;
class dbDatabase;
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
class LayoutViewer : public QWidget
{
  Q_OBJECT

 public:
  LayoutViewer(Options* options, QWidget* parent = nullptr);

  void  setDb(odb::dbDatabase* db);
  qreal getPixelsPerDBU() { return pixelsPerDBU_; }
  void  setScroller(LayoutScroll* scroller);

  // From QWidget
  virtual void paintEvent(QPaintEvent* event) override;
  virtual void resizeEvent(QResizeEvent* event) override;
  virtual void mousePressEvent(QMouseEvent* event) override;
  virtual void mouseMoveEvent(QMouseEvent* event) override;
  virtual void mouseReleaseEvent(QMouseEvent* event) override;

 public slots:
  void zoomIn();
  void zoomOut();
  void zoomTo(const odb::Rect& rect_dbu);
  void designLoaded(odb::dbBlock* block);
  void fit();  // fit the whole design in the window

 private:
  struct Boxes
  {
    std::vector<QRect> obs;
    std::vector<QRect> mterms;
  };
  using LayerBoxes = std::map<odb::dbTechLayer*, Boxes>;
  using CellBoxes  = std::map<odb::dbMaster*, LayerBoxes>;

  void          boxesByLayer(odb::dbMaster* master, LayerBoxes& boxes);
  const Boxes*  boxesByLayer(odb::dbMaster* master, odb::dbTechLayer* layer);
  odb::dbBlock* getBlock();
  void          setPixelsPerDBU(qreal pixelsPerDBU);
  void          drawBlock(QPainter*        painter,
                          const odb::Rect& bounds,
                          odb::dbBlock*    block,
                          int              depth);
  void   addInstTransform(QTransform& xfm, const odb::dbTransform& inst_xfm);
  QColor getColor(odb::dbTechLayer* layer);
  void   updateRubberBandRegion();
  void   drawTracks(odb::dbTechLayer* layer,
                    odb::dbBlock*     block,
                    QPainter*         painter,
                    const odb::Rect&  bounds);

  odb::Rect screenToDBU(const QRect& rect);
  QRectF    DBUToScreen(const odb::Rect& dbu_rect);

  odb::dbDatabase* db_;
  Options*         options_;
  LayoutScroll*    scroller_;
  qreal            pixelsPerDBU_;
  int              min_depth_;
  int              max_depth_;
  Search           search_;
  bool             search_init_;
  CellBoxes        cell_boxes_;
  QRect            rubber_band_;  // screen coordinates
  bool             rubber_band_showing_;
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
