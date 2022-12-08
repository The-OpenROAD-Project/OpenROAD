/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2022, The Regents of the University of California
// All rights reserved.
//
// BSD 3-Clause License
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
//
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <QComboBox>
#include <QDockWidget>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QMenu>
#include <QPainterPath>
#include <QPushButton>
#include <QTabWidget>
#include <QToolTip>
#include <variant>

#include "gui/gui.h"
#include "staGuiInterface.h"

namespace sta {
class dbSta;
}

namespace utl {
class Logger;
}

namespace gui {

class ClockNodeGraphicsViewItem;

class ClockTreeRenderer : public Renderer
{
 public:
  ClockTreeRenderer(ClockTree* tree);
  ~ClockTreeRenderer() {}

  virtual void drawObjects(Painter& painter) override;

  void setPathTo(odb::dbITerm* term);
  void clearPathTo();

 private:
  ClockTree* tree_;

  odb::dbITerm* path_to_;
};

// Handles drawing a dbNet in the clock tree scene
class ClockNetGraphicsViewItem : public QGraphicsItem
{
 public:
  ClockNetGraphicsViewItem(
      odb::dbNet* net,
      const std::vector<ClockNodeGraphicsViewItem*>& source_nodes,
      const std::vector<ClockNodeGraphicsViewItem*>& sink_nodes,
      QGraphicsItem* parent = nullptr);

  QRectF boundingRect() const override;
  void paint(QPainter* painter,
             const QStyleOptionGraphicsItem* option,
             QWidget* widget) override;

  virtual QPainterPath shape() const override { return path_; }

  void buildPath();

 private:
  odb::dbNet* net_;
  std::vector<ClockNodeGraphicsViewItem*> source_nodes_;
  std::vector<ClockNodeGraphicsViewItem*> sink_nodes_;

  QPainterPath path_;

  constexpr static qreal trunk_rounding_ = 0.1;
  constexpr static qreal leaf_rounding_ = 0.1;

  void buildTrunkJunction(const std::vector<ClockNodeGraphicsViewItem*>& nodes,
                          bool top_anchor,
                          const QPointF& trunk_junction);
  void addLeafPath(const QPointF& start,
                   const qreal y_trunk,
                   const QPointF& end);

  void setNetInformation();
};

// Base class for handling drawing of clock tree objects like buffers, roots,
// and leaves.
class ClockNodeGraphicsViewItem : public QGraphicsItem
{
 public:
  ClockNodeGraphicsViewItem(QGraphicsItem* parent = nullptr);
  ~ClockNodeGraphicsViewItem() {}

  virtual QString getType() const = 0;
  virtual QString getName() const { return name_; };

  void setupToolTip();
  void setExtraToolTip(const QString& tooltip) { extra_tooltip_ = tooltip; }

  qreal getSize() const { return size_; }
  void scaleSize(double scale) { size_ *= scale; }

  // returns the input pin location in the scene
  virtual QPointF getTopAnchor() const;
  // returns the output pin location in the scene
  virtual QPointF getBottomAnchor() const;

  void setName(odb::dbInst* inst);
  void setName(odb::dbITerm* term);
  void setName(odb::dbBTerm* term);

  constexpr static Qt::GlobalColor buffer_color_ = Qt::blue;
  constexpr static Qt::GlobalColor root_color_ = Qt::red;
  constexpr static Qt::GlobalColor clock_gate_color_ = Qt::magenta;
  constexpr static Qt::GlobalColor leaf_color_ = Qt::red;

  constexpr static qreal default_size_ = 100.0;

  static QString getITermName(odb::dbITerm* term);

 private:
  qreal size_;
  QString name_;
  QString extra_tooltip_;
};

// Handles drawing the root node for a tree
class ClockRootNodeGraphicsViewItem : public ClockNodeGraphicsViewItem
{
 public:
  ClockRootNodeGraphicsViewItem(odb::dbITerm* term,
                                QGraphicsItem* parent = nullptr);
  ClockRootNodeGraphicsViewItem(odb::dbBTerm* term,
                                QGraphicsItem* parent = nullptr);
  ~ClockRootNodeGraphicsViewItem() {}

  virtual QPointF getTopAnchor() const override;
  virtual QPointF getBottomAnchor() const override;

  virtual QString getType() const override { return "Root"; }

  QRectF boundingRect() const override;
  void paint(QPainter* painter,
             const QStyleOptionGraphicsItem* option,
             QWidget* widget) override;

  virtual QPainterPath shape() const override;

 private:
  QPolygonF getPolygon() const;
};

// Handles drawing the buffer/inverter nodes in the tree
class ClockBufferNodeGraphicsViewItem : public ClockNodeGraphicsViewItem
{
 public:
  ClockBufferNodeGraphicsViewItem(odb::dbITerm* input_term,
                                  odb::dbITerm* output_term,
                                  qreal delay_y,
                                  QGraphicsItem* parent = nullptr);
  ~ClockBufferNodeGraphicsViewItem() {}

  void setIsInverter(bool inverter) { inverter_ = inverter; }

  virtual QString getType() const override
  {
    return inverter_ ? "Inverter" : "Buffer";
  }

  virtual QPointF getBottomAnchor() const override;

  QRectF boundingRect() const override;
  void paint(QPainter* painter,
             const QStyleOptionGraphicsItem* option,
             QWidget* widget) override;

  virtual QPainterPath shape() const override;

  static QPolygonF getBufferShape(qreal size);

 private:
  qreal delay_y_;
  QString input_pin_;
  QString output_pin_;

  bool inverter_;

  constexpr static qreal bar_scale_size_ = 0.1;
};

// Handles drawing the register node for a tree
class ClockRegisterNodeGraphicsViewItem : public ClockNodeGraphicsViewItem
{
 public:
  ClockRegisterNodeGraphicsViewItem(odb::dbITerm* iterm,
                                    QGraphicsItem* parent = nullptr);
  ~ClockRegisterNodeGraphicsViewItem() {}

  virtual QString getType() const override { return "Register"; }

  QRectF boundingRect() const override;
  void paint(QPainter* painter,
             const QStyleOptionGraphicsItem* option,
             QWidget* widget) override;

  QAction* getHighlightAction() const { return highlight_path_; }

 protected:
  void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override;

 private:
  odb::dbITerm* term_;
  QMenu menu_;
  QAction* highlight_path_;

  QRectF getOutlineRect() const;
  QPolygonF getClockInputPolygon() const;
};

class ClockTreeScene : public QGraphicsScene
{
  Q_OBJECT

 public:
  ClockTreeScene(QWidget* parent = nullptr);

  bool isRendererEnabled() const { return draw_tree_->isChecked(); }
  void setRendererEnabled(bool enabled) { draw_tree_->setChecked(enabled); }

  void setClearPathEnable(bool enable) { clear_path_->setEnabled(enable); };

 signals:
  void selected(const Selected& selected);
  void enableRenderer(bool enable);
  void fit();
  void clearPath();
  void save();

 private slots:
  void triggeredClearPath();

 protected:
  void contextMenuEvent(
      QGraphicsSceneContextMenuEvent* contextMenuEvent) override;

 private:
  QMenu* menu_;
  QAction* draw_tree_;
  QAction* clear_path_;
};

class ClockTreeView : public QGraphicsView
{
  Q_OBJECT

 public:
  ClockTreeView(ClockTree* tree,
                const STAGuiInterface* sta,
                utl::Logger* logger,
                QWidget* parent = nullptr);
  ~ClockTreeView();

  ClockTree* getClockTree() const { return tree_.get(); }
  const char* getClockName() const;

  void showRenderer(bool show) const;

 signals:
  void selected(const Selected& selected);

 public slots:
  void fit();
  void save(const QString& path = "");

 private slots:
  void enableRenderer(bool enable);
  void selectionChanged();
  void highlightTo(odb::dbITerm* term);
  void clearHighlightTo();

 protected:
  void wheelEvent(QWheelEvent* event) override;
  void paintEvent(QPaintEvent* event) override;
  void mousePressEvent(QMouseEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;

 private:
  std::unique_ptr<ClockTree> tree_;
  std::unique_ptr<ClockTreeRenderer> renderer_;
  ClockTreeScene* scene_;
  utl::Logger* logger_;
  bool show_mouse_time_tick_;

  sta::Delay min_delay_;
  sta::Delay max_delay_;

  double unit_scale_;
  QString unit_suffix_;

  qreal leaf_scale_;
  qreal time_scale_;

  QRect rubber_band_;

  std::vector<qreal> bin_center_;

  std::vector<ClockNetGraphicsViewItem*> nets_;

  std::vector<ClockNodeGraphicsViewItem*> buildTree(const ClockTree* tree,
                                                    const STAGuiInterface* sta,
                                                    int center_index);

  struct PinArrival
  {
    sta::Pin* pin = nullptr;
    sta::Delay delay = 0.0;
  };
  ClockNodeGraphicsViewItem* addBufferToScene(qreal x,
                                              const PinArrival& input_pin,
                                              const PinArrival& output_pin,
                                              sta::dbNetwork* network);
  ClockNodeGraphicsViewItem* addRootToScene(qreal x,
                                            const PinArrival& output_pin,
                                            sta::dbNetwork* network);
  ClockNodeGraphicsViewItem* addLeafToScene(qreal x,
                                            const PinArrival& input_pin,
                                            sta::dbNetwork* network);

  constexpr static int default_scene_height_
      = 75.0 * ClockNodeGraphicsViewItem::default_size_;
  constexpr static qreal node_spacing_ = 0.2;

  qreal convertDelayToY(sta::Delay delay) const;
  sta::Delay convertYToDelay(qreal y) const;
  QString convertDelayToString(sta::Delay delay) const;

  void drawTimeScale(QPainter* painter,
                     const QRect& rect,
                     sta::Delay min_time,
                     sta::Delay max_time,
                     sta::Delay mouse_time) const;

  template <typename T>
  bool canConvertAndEmit(const QVariant& data)
  {
    T value = data.value<T>();
    if (value != nullptr) {
      emit selected(Gui::get()->makeSelected(value));
      return true;
    }
    return false;
  }
};

class ClockWidget : public QDockWidget, sta::dbNetworkObserver
{
  Q_OBJECT

 public:
  ClockWidget(QWidget* parent = nullptr);
  ~ClockWidget();

  void setLogger(utl::Logger* logger);
  void setSTA(sta::dbSta* sta);

  void saveImage(const std::string& clock_name, const std::string& path);

  virtual void postReadLiberty() override;

 signals:
  void selected(const Selected& selected);

 public slots:
  void setBlock(odb::dbBlock* block);
  void populate();

 protected:
  void hideEvent(QHideEvent* event) override;
  void showEvent(QShowEvent* event) override;

 private:
  utl::Logger* logger_;
  odb::dbBlock* block_;
  sta::dbSta* sta_;

  QPushButton* update_button_;
  QComboBox* corner_box_;

  QTabWidget* clocks_tab_;

  std::vector<std::unique_ptr<ClockTreeView>> views_;
};

}  // namespace gui
