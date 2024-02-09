///////////////////////////////////////////////////////////////////////////////
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

#pragma once

#include <QMutex>
#include <QPainter>
#include <QThread>
#include <QWaitCondition>
#include <mutex>

#include "gui/gui.h"
#include "odb/db.h"
#include "ruler.h"
#include "utl/Logger.h"

namespace gui {

class LayoutViewer;
class GuiPainter;

class RenderThread : public QThread
{
  Q_OBJECT

 public:
  RenderThread(LayoutViewer* viewer);

  void setLogger(utl::Logger* logger);

  void render(const QRect& draw_rect,
              const SelectionSet& selected,
              const HighlightSet& highlighted,
              const Rulers& rulers);

  void exit();

  // Only to be used by save_image for synchronous rendering
  void draw(QImage& image,
            const QRect& draw_bounds,
            const SelectionSet& selected,
            const HighlightSet& highlighted,
            const Rulers& rulers,
            qreal render_ratio,
            const QColor& background);

  bool isFirstRenderDone() { return is_first_render_done_; };
  bool isRendering() { return is_rendering_; };

 signals:
  void done(const QImage& image, const QRect& bounds);

 private:
  void run() override;

  void setupIOPins(odb::dbBlock* block, const odb::Rect& bounds);

  void drawBlock(QPainter* painter,
                 odb::dbBlock* block,
                 const odb::Rect& bounds,
                 int depth);
  void drawLayer(QPainter* painter,
                 odb::dbBlock* block,
                 odb::dbTechLayer* layer,
                 const std::vector<odb::dbInst*>& insts,
                 const odb::Rect& bounds,
                 GuiPainter& gui_painter);
  void drawRegions(QPainter* painter, odb::dbBlock* block);
  void drawTracks(odb::dbTechLayer* layer,
                  QPainter* painter,
                  const odb::Rect& bounds);

  void drawInstanceOutlines(QPainter* painter,
                            const std::vector<odb::dbInst*>& insts);
  void drawInstanceShapes(odb::dbTechLayer* layer,
                          QPainter* painter,
                          const std::vector<odb::dbInst*>& insts,
                          const odb::Rect& bounds,
                          GuiPainter& gui_painter);
  void drawInstanceNames(QPainter* painter,
                         const std::vector<odb::dbInst*>& insts);
  void drawITermLabels(QPainter* painter,
                       const std::vector<odb::dbInst*>& insts);
  bool drawTextInBBox(const QColor& text_color,
                      const QFont& text_font,
                      odb::Rect bbox,
                      QString name,
                      QPainter* painter,
                      bool center);

  void drawBlockages(QPainter* painter,
                     odb::dbBlock* block,
                     const odb::Rect& bounds);
  void drawObstructions(odb::dbBlock* block,
                        odb::dbTechLayer* layer,
                        QPainter* painter,
                        const odb::Rect& bounds);
  void drawRows(QPainter* painter,
                odb::dbBlock* block,
                const odb::Rect& bounds);
  void drawViaShapes(QPainter* painter,
                     odb::dbBlock* block,
                     odb::dbTechLayer* cut_layer,
                     odb::dbTechLayer* draw_layer,
                     const odb::Rect& bounds,
                     int shape_limit);
  void drawManufacturingGrid(QPainter* painter, const odb::Rect& bounds);
  void drawGCellGrid(QPainter* painter, const odb::Rect& bounds);
  void drawSelected(Painter& painter, const SelectionSet& selected);
  void drawHighlighted(Painter& painter, const HighlightSet& highlighted);
  void drawIOPins(Painter& painter,
                  odb::dbBlock* block,
                  const odb::Rect& bounds,
                  odb::dbTechLayer* layer);
  void drawAccessPoints(Painter& painter,
                        const std::vector<odb::dbInst*>& insts);
  void drawRouteGuides(Painter& painter, odb::dbTechLayer* layer);
  void drawNetTracks(Painter& painter, odb::dbTechLayer* layer);
  void drawModuleView(QPainter* painter,
                      const std::vector<odb::dbInst*>& insts);
  void drawRulers(Painter& painter, const Rulers& rulers);

  bool instanceBelowMinSize(odb::dbInst* inst);

  void addInstTransform(QTransform& xfm, const odb::dbTransform& inst_xfm);
  QColor getColor(odb::dbTechLayer* layer);
  Qt::BrushStyle getPattern(odb::dbTechLayer* layer);

  void drawDesignLoadingMessage(Painter& painter, const odb::Rect& bounds);

  utl::Logger* logger_ = nullptr;
  LayoutViewer* viewer_;
  std::mutex drawing_mutex_;

  // These variables are cached copies of what's passed to render().
  // The draw method will the make a local copy of them to avoid any
  // updates during drawing. These should not be accessed from any
  // drawing methods.
  QRect draw_rect_;
  SelectionSet selected_;
  HighlightSet highlighted_;
  Rulers rulers_;

  QMutex mutex_;
  QWaitCondition condition_;
  bool restart_ = false;
  bool abort_ = false;
  bool is_rendering_ = false;
  bool is_first_render_done_ = false;

  QFont pin_font_;
  bool pin_draw_names_ = false;
  double pin_max_size_ = 0.0;
  std::map<odb::dbTechLayer*,
           std::vector<std::pair<odb::dbBTerm*, odb::dbBox*>>>
      pins_;
};

}  // namespace gui
