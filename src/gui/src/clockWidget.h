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
#include <QRadioButton>
#include <QSpinBox>
#include <QTabWidget>
#include <QToolTip>
#include <optional>
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
class ColorGenerator;

enum class RendererState
{
  OnlyShowOnActiveWidget,
  AlwaysShow,
  NeverShow
};

class ClockTreeRenderer : public Renderer
{
 public:
  ClockTreeRenderer(ClockTree* tree);
  ~ClockTreeRenderer() {}

  virtual void drawObjects(Painter& painter) override;

  void setPathTo(odb::dbITerm* term);
  void clearPathTo();

  void setTree(ClockTree* tree);
  void resetTree();

  void setMaxColorDepth(int depth);
  int getMaxColorDepth() const { return max_depth_; }

 private:
  ClockTree* tree_;
  int max_depth_;

  odb::dbITerm* path_to_;

  static constexpr int pen_width_ = 2;

  void drawTree(Painter& painter,
                const Descriptor* descriptor,
                ColorGenerator& colorgenerator,
                ClockTree* tree,
                int depth);
  void setPen(Painter& painter, const Painter::Color& color);
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

  QRectF boundingRect() const override;
  void paint(QPainter* painter,
             const QStyleOptionGraphicsItem* option,
             QWidget* widget) override;

  virtual QString getType() const = 0;
  virtual QString getName() const { return name_; };
  virtual QString getInstName() const { return inst_name_; };
  virtual QColor getColor() const = 0;

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
  constexpr static Qt::GlobalColor inverter_color_ = Qt::darkCyan;
  constexpr static Qt::GlobalColor root_color_ = Qt::red;
  constexpr static Qt::GlobalColor clock_gate_color_ = Qt::magenta;
  constexpr static Qt::GlobalColor unknown_color_ = Qt::darkGray;
  constexpr static Qt::GlobalColor leaf_register_color_ = Qt::red;
  constexpr static Qt::GlobalColor leaf_macro_color_ = Qt::darkCyan;

  constexpr static qreal default_size_ = 100.0;

  static QString getITermName(odb::dbITerm* term);
  static QString getITermInstName(odb::dbITerm* term);

 protected:
  void addDelayFin(QPainterPath& path, const qreal delay) const;

 private:
  qreal size_;
  QString name_;
  QString inst_name_;
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
  virtual QColor getColor() const override { return root_color_; }

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
  virtual QColor getColor() const override
  {
    return inverter_ ? inverter_color_ : buffer_color_;
  }

  virtual QPointF getBottomAnchor() const override;

  virtual QPainterPath shape() const override;

  static QPolygonF getBufferShape(qreal size);

 private:
  qreal delay_y_;
  QString input_pin_;
  QString output_pin_;

  bool inverter_;

  constexpr static qreal bar_scale_size_ = 0.1;
};

// Handles drawing macro or register leaf cell
class ClockLeafNodeGraphicsViewItem : public ClockNodeGraphicsViewItem
{
 public:
  ClockLeafNodeGraphicsViewItem(odb::dbITerm* iterm,
                                QGraphicsItem* parent = nullptr);
  ~ClockLeafNodeGraphicsViewItem() override = default;

  QString getType() const override = 0;
  QColor getColor() const override = 0;

  QRectF boundingRect() const override;
  void paint(QPainter* painter,
             const QStyleOptionGraphicsItem* option,
             QWidget* widget) override;

  QAction* getHighlightAction() const { return highlight_path_; }

 protected:
  void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override;
  QMenu menu_;
  QAction* highlight_path_;

  virtual QRectF getOutlineRect() const;
  QPolygonF getClockInputPolygon() const;
};

// Handles drawing register cell node for a tree
class ClockRegisterNodeGraphicsViewItem : public ClockLeafNodeGraphicsViewItem
{
 public:
  ClockRegisterNodeGraphicsViewItem(odb::dbITerm* iterm,
                                    QGraphicsItem* parent = nullptr)
      : ClockLeafNodeGraphicsViewItem(iterm, parent)
  {
  }
  ~ClockRegisterNodeGraphicsViewItem() override = default;
  QString getType() const override { return "Register"; }
  QColor getColor() const override { return leaf_register_color_; }
};

// Handles drawing macro cell node for a tree
class ClockMacroNodeGraphicsViewItem : public ClockLeafNodeGraphicsViewItem
{
 public:
  ClockMacroNodeGraphicsViewItem(odb::dbITerm* iterm,
                                 qreal insertion_delay_as_height,
                                 QGraphicsItem* parent = nullptr)
      : ClockLeafNodeGraphicsViewItem(iterm, parent),
        insertion_delay_as_height_(insertion_delay_as_height)
  {
  }
  ~ClockMacroNodeGraphicsViewItem() override = default;
  QString getType() const override { return "Macro"; }
  QColor getColor() const override { return leaf_macro_color_; }

 protected:
  QRectF getOutlineRect() const override;

  const qreal insertion_delay_as_height_;
};

// Handles drawing the clock gate and non-inverter/buffers nodes in the tree
class ClockGateNodeGraphicsViewItem : public ClockNodeGraphicsViewItem
{
 public:
  ClockGateNodeGraphicsViewItem(odb::dbITerm* input_term,
                                odb::dbITerm* output_term,
                                qreal delay_y,
                                QGraphicsItem* parent = nullptr);
  ~ClockGateNodeGraphicsViewItem() {}

  void setIsClockGate(bool gate) { is_clock_gate_ = gate; }

  virtual QString getType() const override;
  virtual QColor getColor() const override
  {
    return is_clock_gate_ ? clock_gate_color_ : unknown_color_;
  }

  virtual QPointF getBottomAnchor() const override;

  virtual QPainterPath shape() const override;

 private:
  qreal delay_y_;
  QString input_pin_;
  QString output_pin_;

  bool is_clock_gate_;
};

class ClockTreeScene : public QGraphicsScene
{
  Q_OBJECT

 public:
  ClockTreeScene(QWidget* parent = nullptr);

  void setRendererState(RendererState state);
  void setClearPathEnable(bool enable) { clear_path_->setEnabled(enable); };
  int getMaxColorDepth() const { return color_depth_->value(); }

 signals:
  void changeRendererState(RendererState state);
  void fit();
  void clearPath();
  void save();
  void colorDepth(int depth);

 private slots:
  void updateRendererState();
  void triggeredClearPath();

 protected:
  void contextMenuEvent(
      QGraphicsSceneContextMenuEvent* contextMenuEvent) override;

 private:
  QMenu* menu_;
  QAction* clear_path_;
  QSpinBox* color_depth_;
  std::map<RendererState, QRadioButton*> renderer_state_;
};

class ClockTreeView : public QGraphicsView
{
  Q_OBJECT

 public:
  ClockTreeView(std::shared_ptr<ClockTree> tree,
                const STAGuiInterface* sta,
                utl::Logger* logger,
                QWidget* parent = nullptr);

  std::shared_ptr<ClockTree>& getClockTree() { return tree_; }
  const char* getClockName() const;

  void updateRendererState() const;
  ClockTreeRenderer* getRenderer() const { return renderer_.get(); }
  ClockNodeGraphicsViewItem* getItemFromName(const std::string& name);
  void clearSelection() { scene_->clearSelection(); };
  std::set<ClockNodeGraphicsViewItem*> getNodes(const SelectionSet& selections);
  bool changeSelection(const SelectionSet& selections);
  void fitSelection();

 signals:
  void selected(const Selected& selected);

 public slots:
  void setRendererState(RendererState state);
  void fit();
  void save(const QString& path = "");

 private slots:
  void selectionChanged();
  void highlightTo(odb::dbITerm* term);
  void clearHighlightTo();
  void updateColorDepth(int depth);

 protected:
  void wheelEvent(QWheelEvent* event) override;
  void paintEvent(QPaintEvent* event) override;
  void mousePressEvent(QMouseEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;

 private:
  std::shared_ptr<ClockTree> tree_;
  std::unique_ptr<ClockTreeRenderer> renderer_;
  RendererState renderer_state_;
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
  std::map<std::string, ClockNodeGraphicsViewItem*> items_;

  struct PinArrival
  {
    const sta::Pin* pin = nullptr;
    sta::Delay delay = 0.0;
  };
  ClockNodeGraphicsViewItem* addCellToScene(qreal x,
                                            const PinArrival& input_pin,
                                            const PinArrival& output_pin,
                                            sta::dbNetwork* network);
  ClockNodeGraphicsViewItem* addRootToScene(qreal x,
                                            const PinArrival& output_pin,
                                            sta::dbNetwork* network);
  ClockNodeGraphicsViewItem* addLeafToScene(qreal x,
                                            const PinArrival& input_pin,
                                            sta::dbNetwork* network);
  void addNode(qreal x,
               ClockNodeGraphicsViewItem* node,
               const QString& tooltip,
               sta::Delay delay);

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

  void selectRendererTreeNet(odb::dbNet* net);
};

class ClockWidget : public QDockWidget, sta::dbNetworkObserver
{
  Q_OBJECT

 public:
  ClockWidget(QWidget* parent = nullptr);
  ~ClockWidget();

  void setLogger(utl::Logger* logger);
  void setSTA(sta::dbSta* sta);

  void saveImage(const std::string& clock_name,
                 const std::string& path,
                 const std::string& corner,
                 const std::optional<int>& width_px,
                 const std::optional<int>& height_px);
  void selectClock(const std::string& clock_name);

  virtual void postReadLiberty() override;

 signals:
  void selected(const Selected& selected);

 public slots:
  void setBlock(odb::dbBlock* block);
  void populate(sta::Corner* corner = nullptr);
  void fit();
  void findInCts(const Selected& selection);
  void findInCts(const SelectionSet& selections);

 private slots:
  void currentClockChanged(int index);

 protected:
  void hideEvent(QHideEvent* event) override;
  void showEvent(QShowEvent* event) override;

 private:
  utl::Logger* logger_;
  odb::dbBlock* block_;
  sta::dbSta* sta_;
  std::unique_ptr<STAGuiInterface> stagui_;

  QPushButton* update_button_;
  QPushButton* fit_button_;
  QComboBox* corner_box_;

  QTabWidget* clocks_tab_;

  std::vector<std::unique_ptr<ClockTreeView>> views_;
};

}  // namespace gui
