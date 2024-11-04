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

#include "clockWidget.h"

#include <QApplication>
#include <QFontMetrics>
#include <QGraphicsRectItem>
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsTextItem>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QImage>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QToolTip>
#include <QVBoxLayout>
#include <QWheelEvent>
#include <QWidgetAction>

#include "colorGenerator.h"
#include "dbDescriptors.h"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "gui_utils.h"
#include "sta/Corner.hh"
#include "sta/FuncExpr.hh"
#include "sta/Liberty.hh"
#include "sta/Sdc.hh"
#include "sta/Units.hh"
#include "utl/Logger.h"

Q_DECLARE_METATYPE(odb::dbBTerm*);
Q_DECLARE_METATYPE(odb::dbITerm*);
Q_DECLARE_METATYPE(odb::dbInst*);
Q_DECLARE_METATYPE(odb::dbNet*);
Q_DECLARE_METATYPE(sta::Corner*);

namespace gui {

ClockTreeRenderer::ClockTreeRenderer(ClockTree* tree)
    : tree_(tree), max_depth_(1), path_to_(nullptr)
{
}

void ClockTreeRenderer::drawObjects(Painter& painter)
{
  if (tree_ == nullptr) {
    return;
  }

  auto* descriptor = Gui::get()->getDescriptor<odb::dbNet*>();
  if (descriptor == nullptr) {
    return;
  }

  ColorGenerator generator;
  drawTree(painter, descriptor, generator, tree_, 0);

  if (path_to_ != nullptr) {
    painter.setPen(Painter::cyan, true, pen_width_);
    auto* network = tree_->getNetwork();
    sta::Pin* pin = network->dbToSta(path_to_);
    if (pin == nullptr) {
      return;
    }
    for (const auto& [input_pin, output_pin] : tree_->findPathTo(pin)) {
      sta::Net* net = network->net(output_pin);
      odb::dbITerm* iterm;
      odb::dbBTerm* bterm;
      odb::dbModBTerm* modbterm;
      odb::dbModITerm* moditerm;

      network->staToDb(output_pin, iterm, bterm, moditerm, modbterm);
      descriptor->highlight(
          DbNetDescriptor::NetWithSink{network->staToDb(net), iterm}, painter);
    }
  }
}

void ClockTreeRenderer::drawTree(Painter& painter,
                                 const Descriptor* descriptor,
                                 ColorGenerator& colorgenerator,
                                 ClockTree* tree,
                                 int depth)
{
  odb::dbNet* net = tree->getNetwork()->staToDb(tree->getNet());
  descriptor->highlight(net, painter);

  const Painter::Color pen_color = painter.getPenColor();
  const bool change_color = depth < max_depth_ && tree->getSinkCount() > 1;
  if (change_color) {
    depth++;
  }
  for (const auto& fanout : tree->getFanout()) {
    if (change_color) {
      setPen(painter, colorgenerator.getColor());
    }
    drawTree(painter, descriptor, colorgenerator, fanout.get(), depth);
  }

  setPen(painter, pen_color);
}

void ClockTreeRenderer::setPen(Painter& painter, const Painter::Color& color)
{
  painter.setPenAndBrush(color, true, Painter::SOLID, pen_width_);
}

void ClockTreeRenderer::setPathTo(odb::dbITerm* term)
{
  path_to_ = term;
  redraw();
}

void ClockTreeRenderer::clearPathTo()
{
  path_to_ = nullptr;
  redraw();
}

void ClockTreeRenderer::setTree(ClockTree* tree)
{
  if (tree == nullptr) {
    resetTree();
  } else {
    tree_ = tree;
    redraw();
  }
}

void ClockTreeRenderer::resetTree()
{
  ClockTree* parent = tree_->getParent();
  while (parent != nullptr) {
    tree_ = parent;
    parent = tree_->getParent();
  }
  redraw();
}

void ClockTreeRenderer::setMaxColorDepth(int depth)
{
  max_depth_ = depth;
  redraw();
}

////////////////

ClockNetGraphicsViewItem::ClockNetGraphicsViewItem(
    odb::dbNet* net,
    const std::vector<ClockNodeGraphicsViewItem*>& source_nodes,
    const std::vector<ClockNodeGraphicsViewItem*>& sink_nodes,
    QGraphicsItem* parent)
    : QGraphicsItem(parent),
      net_(net),
      source_nodes_(source_nodes),
      sink_nodes_(sink_nodes)
{
  setAcceptHoverEvents(true);
  setZValue(0);
  setFlag(QGraphicsItem::ItemIsSelectable);
  setData(0, QVariant::fromValue(net_));

  setNetInformation();

  buildPath();
}

void ClockNetGraphicsViewItem::setNetInformation()
{
  QString tooltip = "Net: ";
  tooltip += QString::fromStdString(net_->getName());
  tooltip += "\n";
  tooltip += "Drivers: " + QString::number(source_nodes_.size());
  tooltip += "\n";
  tooltip += "Sinks: " + QString::number(sink_nodes_.size());
  setToolTip(tooltip);
}

void ClockNetGraphicsViewItem::buildPath()
{
  path_ = QPainterPath();

  qreal source_ymin = std::numeric_limits<qreal>::lowest();
  qreal source_xcenter = 0;

  for (auto* src : source_nodes_) {
    source_ymin = std::max(source_ymin, src->getBottomAnchor().y());
    source_xcenter += src->getBottomAnchor().x();
  }
  source_xcenter /= source_nodes_.size();

  qreal sink_ymax = std::numeric_limits<qreal>::max();

  for (auto* sink : sink_nodes_) {
    sink_ymax = std::min(sink_ymax, sink->getTopAnchor().y());
  }

  const qreal trunk_distance = sink_ymax - source_ymin;

  const qreal trunk_x = source_xcenter;
  const qreal trunk_topy = source_ymin + trunk_rounding_ * trunk_distance;
  const qreal trunk_boty = sink_ymax - trunk_rounding_ * trunk_distance;

  const QPointF trunk_top(trunk_x, trunk_topy);
  const QPointF trunk_bot(trunk_x, trunk_boty);

  buildTrunkJunction(source_nodes_, false, trunk_top);

  path_.moveTo(trunk_x, trunk_topy);
  path_.lineTo(trunk_x, trunk_boty);

  buildTrunkJunction(sink_nodes_, true, trunk_bot);
}

void ClockNetGraphicsViewItem::buildTrunkJunction(
    const std::vector<ClockNodeGraphicsViewItem*>& nodes,
    bool top_anchor,
    const QPointF& trunk_junction)
{
  qreal y_center;
  if (top_anchor) {
    y_center = std::numeric_limits<qreal>::max();
  } else {
    y_center = std::numeric_limits<qreal>::lowest();
  }

  for (auto* node : nodes) {
    QPointF anchor;
    if (top_anchor) {
      anchor = node->getTopAnchor();
    } else {
      anchor = node->getBottomAnchor();
    }
    const qreal node_center = (trunk_junction.y() + anchor.y()) / 2;
    if (top_anchor) {
      y_center = std::min(node_center, y_center);
    } else {
      y_center = std::max(node_center, y_center);
    }
  }

  for (auto* node : nodes) {
    if (top_anchor) {
      addLeafPath(trunk_junction, trunk_junction.y(), node->getTopAnchor());
    } else {
      addLeafPath(node->getBottomAnchor(), trunk_junction.y(), trunk_junction);
    }
  }
}

void ClockNetGraphicsViewItem::addLeafPath(const QPointF& start,
                                           const qreal y_trunk,
                                           const QPointF& end)
{
  path_.moveTo(start);

  const qreal x_dist = end.x() - start.x();

  const qreal x_offset = leaf_rounding_ * x_dist;

  const QPointF mid0(start.x() + x_offset, y_trunk);
  const QPointF control0(start.x(), y_trunk);

  const QPointF mid1(end.x() - x_offset, y_trunk);
  const QPointF control1(end.x(), y_trunk);

  path_.lineTo(mid1);
  path_.quadTo(control1, end);
}

QRectF ClockNetGraphicsViewItem::boundingRect() const
{
  return path_.boundingRect();
}

void ClockNetGraphicsViewItem::paint(QPainter* painter,
                                     const QStyleOptionGraphicsItem* option,
                                     QWidget* widget)
{
  QPen pen(Qt::darkGreen);
  pen.setWidth(1);
  pen.setCosmetic(true);

  if (isSelected()) {
    pen.setWidth(2);
  }

  painter->setPen(pen);
  painter->drawPath(path_);
}

////////////////

ClockNodeGraphicsViewItem::ClockNodeGraphicsViewItem(QGraphicsItem* parent)
    : QGraphicsItem(parent), size_(default_size_), name_(""), extra_tooltip_("")
{
  setAcceptHoverEvents(true);
  setZValue(1);
  setFlag(QGraphicsItem::ItemIsSelectable);
}

void ClockNodeGraphicsViewItem::setupToolTip()
{
  QString info = "Type: " + getType();
  info += "\n";

  info += "Name: " + getName();

  if (!extra_tooltip_.isEmpty()) {
    info += "\n";
    info += extra_tooltip_;
  }

  setToolTip(info);
}

QPointF ClockNodeGraphicsViewItem::getTopAnchor() const
{
  // anchored at input
  return pos();
}

QPointF ClockNodeGraphicsViewItem::getBottomAnchor() const
{
  QPointF pt = pos();
  pt.setY(pt.y() + size_);
  return pt;
}

QString ClockNodeGraphicsViewItem::getITermName(odb::dbITerm* term)
{
  return QString::fromStdString(term->getName());
}

QString ClockNodeGraphicsViewItem::getITermInstName(odb::dbITerm* term)
{
  return QString::fromStdString(term->getInst()->getName());
}

void ClockNodeGraphicsViewItem::setName(odb::dbITerm* term)
{
  name_ = getITermName(term);
  inst_name_ = getITermInstName(term);
}

void ClockNodeGraphicsViewItem::setName(odb::dbBTerm* term)
{
  name_ = term->getConstName();
  inst_name_ = name_;
}

void ClockNodeGraphicsViewItem::setName(odb::dbInst* inst)
{
  name_ = inst->getConstName();
  inst_name_ = name_;
}

void ClockNodeGraphicsViewItem::addDelayFin(QPainterPath& path,
                                            const qreal delay) const
{
  path.moveTo(0, size_);
  path.lineTo(0, delay);
  const qreal fin_width = size_ / 10;
  path.moveTo(-fin_width / 2, delay);
  path.lineTo(fin_width / 2, delay);
}

void ClockNodeGraphicsViewItem::paint(QPainter* painter,
                                      const QStyleOptionGraphicsItem* option,
                                      QWidget* widget)
{
  const QColor outline = getColor();
  const QColor fill(outline.lighter());

  QPen pen(outline);
  pen.setWidth(1);
  pen.setCosmetic(true);

  if (isSelected()) {
    pen.setWidth(2);
  }

  painter->setBrush(QBrush(fill, Qt::SolidPattern));

  painter->setPen(pen);
  painter->drawPath(shape());
}

QRectF ClockNodeGraphicsViewItem::boundingRect() const
{
  return shape().boundingRect();
}

////////////////

ClockBufferNodeGraphicsViewItem::ClockBufferNodeGraphicsViewItem(
    odb::dbITerm* input_term,
    odb::dbITerm* output_term,
    qreal delay_y,
    QGraphicsItem* parent)
    : ClockNodeGraphicsViewItem(parent),
      delay_y_(delay_y),
      input_pin_(input_term->getMTerm()->getConstName()),
      output_pin_(output_term->getMTerm()->getConstName()),
      inverter_(false)
{
  odb::dbInst* inst = input_term->getInst();
  setName(inst);
  setData(0, QVariant::fromValue(inst));
  setData(1, QVariant::fromValue(output_term->getNet()));
}

QPointF ClockBufferNodeGraphicsViewItem::getBottomAnchor() const
{
  QPointF pt = pos();
  pt.setY(pt.y() + delay_y_);
  return pt;
}

QPolygonF ClockBufferNodeGraphicsViewItem::getBufferShape(qreal size)
{
  QPolygonF buffer;
  buffer.append({-size / 2, 0});
  buffer.append({size / 2, 0});
  buffer.append({0, size});
  buffer.append({-size / 2, 0});

  return buffer;
}

QPainterPath ClockBufferNodeGraphicsViewItem::shape() const
{
  const qreal size = getSize();
  const qreal bar_size = size * bar_scale_size_;

  QPainterPath path;
  qreal buffer_size = size;
  if (inverter_) {
    buffer_size -= bar_size;
  }
  path.addPolygon(getBufferShape(buffer_size));
  if (inverter_) {
    path.addEllipse(
        QPointF(0, size - bar_size / 2), bar_size / 2, bar_size / 2);
  }
  addDelayFin(path, delay_y_);
  return path;
}

////////////////

ClockGateNodeGraphicsViewItem::ClockGateNodeGraphicsViewItem(
    odb::dbITerm* input_term,
    odb::dbITerm* output_term,
    qreal delay_y,
    QGraphicsItem* parent)
    : ClockNodeGraphicsViewItem(parent),
      delay_y_(delay_y),
      input_pin_(input_term->getMTerm()->getConstName()),
      output_pin_(output_term->getMTerm()->getConstName()),
      is_clock_gate_(true)
{
  odb::dbInst* inst = input_term->getInst();
  setName(inst);
  setData(0, QVariant::fromValue(inst));
}

QPointF ClockGateNodeGraphicsViewItem::getBottomAnchor() const
{
  QPointF pt = pos();
  pt.setY(pt.y() + delay_y_);
  return pt;
}

QPainterPath ClockGateNodeGraphicsViewItem::shape() const
{
  const qreal size = getSize();

  QPainterPath path;
  path.addEllipse(QPointF(0, size / 2), size / 2, size / 2);
  addDelayFin(path, delay_y_);
  return path;
}

QString ClockGateNodeGraphicsViewItem::getType() const
{
  if (is_clock_gate_) {
    return "Clock gate";
  }
  return "Assumed clock gate";
}

////////////////

ClockRootNodeGraphicsViewItem::ClockRootNodeGraphicsViewItem(
    odb::dbITerm* term,
    QGraphicsItem* parent)
    : ClockNodeGraphicsViewItem(parent)
{
  setName(term);
  setData(0, QVariant::fromValue(term));
}

ClockRootNodeGraphicsViewItem::ClockRootNodeGraphicsViewItem(
    odb::dbBTerm* term,
    QGraphicsItem* parent)
    : ClockNodeGraphicsViewItem(parent)
{
  setName(term);
  setData(0, QVariant::fromValue(term));
}

QPointF ClockRootNodeGraphicsViewItem::getTopAnchor() const
{
  QPointF pt = pos();
  pt.setY(pt.y() - getSize());
  return pt;
}

QPointF ClockRootNodeGraphicsViewItem::getBottomAnchor() const
{
  return pos();
}

QPolygonF ClockRootNodeGraphicsViewItem::getPolygon() const
{
  QPolygonF buffer = ClockBufferNodeGraphicsViewItem::getBufferShape(getSize());
  buffer.translate(0, -buffer.boundingRect().height());

  return buffer;
}

QPainterPath ClockRootNodeGraphicsViewItem::shape() const
{
  QPainterPath path;
  path.addPolygon(getPolygon());
  return path;
}

////////////////

ClockLeafNodeGraphicsViewItem::ClockLeafNodeGraphicsViewItem(
    odb::dbITerm* iterm,
    QGraphicsItem* parent)
    : ClockNodeGraphicsViewItem(parent),
      highlight_path_(new QAction("Highlight path", &menu_))
{
  setName(iterm);
  setData(0, QVariant::fromValue(iterm));

  menu_.addAction(highlight_path_);
}

QPolygonF ClockLeafNodeGraphicsViewItem::getClockInputPolygon() const
{
  const qreal size = getSize();
  QPolygonF poly;
  poly.append({-size / 8, 0});
  poly.append({size / 8, 0});
  poly.append({0, size / 4});
  poly.append({-size / 8, 0});
  return poly;
}

QRectF ClockLeafNodeGraphicsViewItem::getOutlineRect() const
{
  const qreal size = getSize();
  const QPointF ll(-size / 2, size);
  const QPointF ur(size / 2, 0);
  QRectF rect(ll, ur);
  return rect.normalized();
}

QRectF ClockLeafNodeGraphicsViewItem::boundingRect() const
{
  return getOutlineRect();
}

void ClockLeafNodeGraphicsViewItem::paint(
    QPainter* painter,
    const QStyleOptionGraphicsItem* option,
    QWidget* widget)
{
  const QColor outline = getColor();
  const QColor fill(outline.lighter());

  QPen pen(outline);
  pen.setWidth(1);
  pen.setCosmetic(true);

  if (isSelected()) {
    pen.setWidth(2);
  }

  painter->setBrush(QBrush(fill, Qt::SolidPattern));

  painter->setPen(pen);
  painter->drawRect(getOutlineRect());

  pen.setWidth(1);
  painter->setPen(pen);
  painter->drawPolygon(getClockInputPolygon());
}

void ClockLeafNodeGraphicsViewItem::contextMenuEvent(
    QGraphicsSceneContextMenuEvent* event)
{
  event->accept();
  menu_.popup(event->screenPos());
}

////////////////

ClockTreeScene::ClockTreeScene(QWidget* parent)
    : QGraphicsScene(parent),
      menu_(new QMenu(parent)),
      clear_path_(new QAction("Clear path", menu_)),
      color_depth_(new QSpinBox(menu_))
{
  QWidgetAction* renderer_widget = new QWidgetAction(menu_);
  QGroupBox* renderer_group = new QGroupBox("Draw tree", menu_);
  QVBoxLayout* renderer_layout = new QVBoxLayout;
  renderer_state_[RendererState::OnlyShowOnActiveWidget]
      = new QRadioButton("Only show when clock is selected", menu_);
  renderer_state_[RendererState::AlwaysShow]
      = new QRadioButton("Always show", menu_);
  renderer_state_[RendererState::NeverShow]
      = new QRadioButton("Never show", menu_);
  renderer_state_[RendererState::OnlyShowOnActiveWidget]->setChecked(true);
  for (const auto& [state, button] : renderer_state_) {
    renderer_layout->addWidget(button);
    connect(button,
            &QRadioButton::toggled,
            this,
            &ClockTreeScene::updateRendererState);
  }
  renderer_group->setLayout(renderer_layout);
  renderer_widget->setDefaultWidget(renderer_group);
  menu_->addAction(renderer_widget);

  QWidgetAction* color_depth_widget = new QWidgetAction(menu_);
  color_depth_widget->setDefaultWidget(color_depth_);
  color_depth_->setToolTip("Tree color depth");
  menu_->addAction(color_depth_widget);
  menu_->addAction(clear_path_);

  connect(clear_path_,
          &QAction::triggered,
          this,
          &ClockTreeScene::triggeredClearPath);
  connect(
      menu_->addAction("Fit"), &QAction::triggered, this, &ClockTreeScene::fit);
  connect(menu_->addAction("Save"),
          &QAction::triggered,
          this,
          &ClockTreeScene::save);

  clear_path_->setEnabled(false);

  color_depth_->setRange(0, 5);
  color_depth_->setValue(1);
  connect(color_depth_,
          qOverload<int>(&QSpinBox::valueChanged),
          this,
          &ClockTreeScene::colorDepth);
}

void ClockTreeScene::contextMenuEvent(QGraphicsSceneContextMenuEvent* event)
{
  QGraphicsScene::contextMenuEvent(event);
  if (!event->isAccepted()) {
    menu_->popup(event->screenPos());
  }
}

void ClockTreeScene::triggeredClearPath()
{
  emit clearPath();
  setClearPathEnable(false);
}

void ClockTreeScene::updateRendererState()
{
  for (const auto& [state, button] : renderer_state_) {
    if (button->isChecked()) {
      emit changeRendererState(state);
    }
  }
}

void ClockTreeScene::setRendererState(RendererState state)
{
  renderer_state_[state]->setChecked(true);
  updateRendererState();
}

////////////////

ClockTreeView::ClockTreeView(std::shared_ptr<ClockTree> tree,
                             const STAGuiInterface* sta,
                             utl::Logger* logger,
                             QWidget* parent)
    : QGraphicsView(new ClockTreeScene(parent), parent),
      tree_(std::move(tree)),
      renderer_(std::make_unique<ClockTreeRenderer>(tree_.get())),
      renderer_state_(RendererState::OnlyShowOnActiveWidget),
      scene_(nullptr),
      logger_(logger),
      show_mouse_time_tick_(true),
      min_delay_(tree_->getMinimumArrival()),
      max_delay_(tree_->getMaximumArrival()),
      unit_scale_(1.0),
      unit_suffix_(""),
      leaf_scale_(1.0 / ((1.0 + node_spacing_) * tree_->getMaxLeaves())),
      time_scale_(default_scene_height_)
{
  setDragMode(QGraphicsView::ScrollHandDrag);
  setMouseTracking(true);

  setRenderHint(QPainter::Antialiasing);
  setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
  setBackgroundBrush(Qt::white);

  scene_ = static_cast<ClockTreeScene*>(scene());

  sta::Unit* unit = sta->getSTA()->units()->timeUnit();
  unit_scale_ = unit->scale();
  unit_suffix_ = unit->scaleAbbreviation();
  unit_suffix_ += unit->suffix();

  const int tree_width = tree_->getTotalFanout();
  const qreal bin_pitch
      = (1.0 + node_spacing_) * ClockNodeGraphicsViewItem::default_size_;
  qreal bin_center = 0.0;
  for (int i = 0; i < tree_width; i++) {
    bin_center_.push_back(bin_center);
    bin_center += bin_pitch;
  }

  const qreal est_scene_width = (tree_width - 1) * (1 + node_spacing_)
                                    * ClockNodeGraphicsViewItem::default_size_
                                + ClockNodeGraphicsViewItem::default_size_;
  const qreal min_time_scale = ClockNodeGraphicsViewItem::default_size_
                               * (max_delay_ - min_delay_)
                               / tree_->getMinimumDriverDelay();
  time_scale_ = std::max(est_scene_width, min_time_scale);

  buildTree(tree_.get(), sta, bin_center_.size() / 2);
  for (auto* net : nets_) {
    net->buildPath();
  }

  QRectF scene_margin = scene_->sceneRect();
  const qreal x_margin = 0.1 * scene_margin.width();
  const qreal y_margin = 0.1 * scene_margin.height();
  scene_margin.adjust(-x_margin, -y_margin, x_margin, y_margin);
  scene_->setSceneRect(scene_margin);

  connect(scene_,
          &ClockTreeScene::selectionChanged,
          this,
          &ClockTreeView::selectionChanged);
  connect(scene_, &ClockTreeScene::fit, this, &ClockTreeView::fit);
  connect(scene_,
          &ClockTreeScene::clearPath,
          this,
          &ClockTreeView::clearHighlightTo);
  connect(scene_, &ClockTreeScene::save, [=] { save(); });
  connect(scene_,
          &ClockTreeScene::colorDepth,
          this,
          &ClockTreeView::updateColorDepth);
  connect(scene_,
          &ClockTreeScene::changeRendererState,
          this,
          &ClockTreeView::setRendererState);
}

void ClockTreeView::fit()
{
  fitInView(scene_->sceneRect(), Qt::KeepAspectRatio);
}

ClockNodeGraphicsViewItem* ClockTreeView::getItemFromName(
    const std::string& name)
{
  const auto& item_found = items_.find(name);
  if (item_found == items_.end()) {
    return nullptr;
  }
  return item_found->second;
}

std::set<ClockNodeGraphicsViewItem*> ClockTreeView::getNodes(
    const SelectionSet& selections)
{
  std::set<ClockNodeGraphicsViewItem*> nodes;
  for (const auto& selection : selections) {
    ClockNodeGraphicsViewItem* item = getItemFromName(selection.getName());
    if (item != nullptr) {
      nodes.insert(item);
    }
  }

  return nodes;
}

bool ClockTreeView::changeSelection(const SelectionSet& selections)
{
  std::set<ClockNodeGraphicsViewItem*> nodes = getNodes(selections);
  if (!nodes.empty()) {
    // remove old selection
    clearSelection();
    for (auto node : nodes) {
      node->setSelected(true);
    }
    return true;
  }
  return false;
}

void ClockTreeView::fitSelection()
{
  QList<QGraphicsItem*> items = scene_->selectedItems();
  if (items.empty()) {
    return;
  }

  QRectF selection_area;
  for (auto item : items) {
    selection_area = selection_area.united(item->sceneBoundingRect());
  }
  fitInView(selection_area, Qt::KeepAspectRatio);
}

void ClockTreeView::mouseMoveEvent(QMouseEvent* event)
{
  if (!rubber_band_.isNull()) {
    rubber_band_.setBottomLeft(event->pos());
  }
  QGraphicsView::mouseMoveEvent(event);
  update();
  viewport()->update();
}

void ClockTreeView::mousePressEvent(QMouseEvent* event)
{
  if (event->button() == Qt::LeftButton
      && event->modifiers().testFlag(Qt::ControlModifier)) {
    if (rubber_band_.isNull()) {
      rubber_band_ = QRect(event->pos(), event->pos());
    }
  } else {
    QGraphicsView::mousePressEvent(event);
  }
}

void ClockTreeView::mouseReleaseEvent(QMouseEvent* event)
{
  if (!rubber_band_.isNull()) {
    const QRect band = rubber_band_.normalized();
    if (std::max(band.height(), band.width())
        > 4) {  // must be greater than 4 pixels
      fitInView(mapToScene(band).boundingRect(), Qt::KeepAspectRatio);
    }
    rubber_band_ = QRect();
  }
  QGraphicsView::mouseReleaseEvent(event);
}

void ClockTreeView::paintEvent(QPaintEvent* event)
{
  QGraphicsView::paintEvent(event);

  const QRect view_rect = viewport()->rect();
  const QRectF region = mapToScene(view_rect).boundingRect();

  QPainter painter(viewport());
  painter.setRenderHints(QPainter::Antialiasing);

  if (!rubber_band_.isNull()) {
    painter.drawRect(rubber_band_.normalized());
  }

  drawTimeScale(&painter,
                view_rect,
                convertYToDelay(region.top()),
                convertYToDelay(region.bottom()),
                convertYToDelay(
                    mapToScene(viewport()->mapFromGlobal(QCursor::pos())).y()));
}

void ClockTreeView::drawTimeScale(QPainter* painter,
                                  const QRect& rect,
                                  sta::Delay min_time,
                                  sta::Delay max_time,
                                  sta::Delay mouse_time) const
{
  min_time /= unit_scale_;
  max_time /= unit_scale_;
  mouse_time /= unit_scale_;

  const sta::Delay time_range = max_time - min_time;

  const int digits = 2;

  const int tick_length = std::min(20, rect.width() / 20);

  auto time_to_y = [min_time, time_range, &rect](sta::Delay time) -> qreal {
    return (time - min_time) / time_range * static_cast<qreal>(rect.height());
  };

  auto time_to_string = [this](sta::Delay time, int digits) -> QString {
    return QString::number(time, 'f', digits) + QString(" ") + unit_suffix_;
  };

  // draw edge line
  painter->setPen(QPen(Qt::black, 0));
  painter->drawLine(rect.bottomLeft(), rect.topLeft());

  const int min_tick_marks = 2;
  const sta::Delay scale_time = std::pow(10, digits);
  sta::Delay tick_interval
      = std::pow(10, std::floor(std::log10(time_range * scale_time)))
        / scale_time;
  if (time_range / tick_interval < min_tick_marks) {
    tick_interval /= 10;
  }
  tick_interval = std::max(tick_interval, 1 / scale_time);

  sta::Delay tick = std::floor(min_time / tick_interval) * tick_interval;
  for (; tick < max_time; tick += tick_interval) {
    const QPoint tick_start(rect.left(), time_to_y(tick));
    const QPoint tick_end(tick_start.x() + tick_length, tick_start.y());
    painter->drawLine(tick_start, tick_end);
    painter->drawText(tick_end, time_to_string(tick, digits));
  }

  if (show_mouse_time_tick_) {
    // draw mouse position
    painter->setPen(QPen(Qt::gray, 0));
    const QPoint tick_start(rect.left(), time_to_y(mouse_time));
    const QPoint tick_end(tick_start.x() + tick_length / 2, tick_start.y());
    painter->drawLine(tick_start, tick_end);

    const QString mouse_text = time_to_string(mouse_time, digits + 1);
    const QFontMetrics fm(painter->font());

    QRect text_bounds = fm.boundingRect(mouse_text);
    text_bounds.moveTo(tick_end.x(),
                       tick_end.y() - text_bounds.height() + fm.descent());
    painter->setBrush(
        QBrush(painter->pen().color().lighter(), Qt::SolidPattern));
    painter->drawRect(text_bounds);

    painter->setPen(QPen(Qt::black, 0));
    painter->drawText(tick_end, time_to_string(mouse_time, digits + 1));
  }
}

void ClockTreeView::wheelEvent(QWheelEvent* event)
{
  const double factor = 1.10;

  const auto anchor = transformationAnchor();
  setTransformationAnchor(QGraphicsView::AnchorUnderMouse);

  if (event->angleDelta().y() > 0) {
    scale(factor, factor);
  } else if (event->angleDelta().y() < 0) {
    scale(1.0 / factor, 1.0 / factor);
  }

  setTransformationAnchor(anchor);
}

void ClockTreeView::selectionChanged()
{
  renderer_->resetTree();

  for (const auto& sel : scene_->selectedItems()) {
    const QVariant data = sel->data(0);
    if (canConvertAndEmit<odb::dbBTerm*>(data)) {
      continue;
    }
    if (canConvertAndEmit<odb::dbITerm*>(data)) {
      continue;
    }
    if (canConvertAndEmit<odb::dbNet*>(data)) {
      continue;
    }
    if (canConvertAndEmit<odb::dbInst*>(data)) {
      selectRendererTreeNet(sel->data(1).value<odb::dbNet*>());
      continue;
    }
  }
}

const char* ClockTreeView::getClockName() const
{
  return tree_->getClock()->name();
}

void ClockTreeView::setRendererState(RendererState state)
{
  if (renderer_state_ == state) {
    return;
  }

  renderer_state_ = state;
  updateRendererState();
}

void ClockTreeView::updateRendererState() const
{
  bool enable = false;
  switch (renderer_state_) {
    case RendererState::AlwaysShow:
      enable = true;
      break;
    case RendererState::NeverShow:
      enable = false;
      break;
    case RendererState::OnlyShowOnActiveWidget:
      enable = isVisible();
      break;
  }

  auto* gui = Gui::get();
  if (enable) {
    gui->registerRenderer(renderer_.get());
    scene_->setClearPathEnable(true);
  } else {
    gui->unregisterRenderer(renderer_.get());
    scene_->setClearPathEnable(false);
  }
}

qreal ClockTreeView::convertDelayToY(sta::Delay delay) const
{
  return (delay - min_delay_) / (max_delay_ - min_delay_) * time_scale_;
}

sta::Delay ClockTreeView::convertYToDelay(qreal y) const
{
  return y / time_scale_ * (max_delay_ - min_delay_) + min_delay_;
}

QString ClockTreeView::convertDelayToString(sta::Delay delay) const
{
  const sta::Unit* unit = tree_->getNetwork()->units()->timeUnit();
  std::string sdelay = unit->asString(delay, 3);
  sdelay += " ";
  sdelay += unit->scaleAbbreviation();
  sdelay += unit->suffix();
  return QString::fromStdString(sdelay);
}

std::vector<ClockNodeGraphicsViewItem*> ClockTreeView::buildTree(
    const ClockTree* tree,
    const STAGuiInterface* sta,
    int center_index)
{
  center_index = std::max(
      std::min(center_index, static_cast<int>(bin_center_.size() - 1)), 0);

  sta::dbNetwork* network = sta->getNetwork();

  std::vector<ClockNodeGraphicsViewItem*> drivers;
  QPolygonF driver_bbox;
  qreal driver_offset = 0;
  ClockTree* parent_tree = tree->getParent();
  for (const auto& [driver, arrival] : tree->getDriverDelays()) {
    PinArrival output_pin{driver, arrival};

    ClockNodeGraphicsViewItem* node;
    if (tree->isRoot()) {
      node = addRootToScene(driver_offset, output_pin, network);
    } else {
      PinArrival input_pin;
      if (parent_tree != nullptr) {
        const auto& [parent_sink, time] = parent_tree->getPairedSink(driver);
        input_pin.pin = parent_sink;
        input_pin.delay = time;
      }

      node = addCellToScene(driver_offset, input_pin, output_pin, network);
    }

    const QRectF bbox = node->boundingRect();
    driver_offset += (1.0 + node_spacing_) * bbox.width();

    driver_bbox.append(node->pos());

    drivers.push_back(node);
  }
  const qreal half_nodes_width = driver_bbox.boundingRect().width() / 2;
  for (auto* driver : drivers) {
    driver->moveBy(bin_center_[center_index] - half_nodes_width, 0);
  }

  int child_offset = center_index - (tree->getTotalFanout() / 2);
  std::vector<ClockNodeGraphicsViewItem*> fanout;
  for (const auto& fan_tree : tree->getFanout()) {
    const int fan_width = fan_tree->getTotalFanout();
    auto fanout_nodes
        = buildTree(fan_tree.get(), sta, child_offset + fan_width / 2);
    fanout.insert(fanout.end(), fanout_nodes.begin(), fanout_nodes.end());

    child_offset += fan_width;
  }

  std::vector<ClockNodeGraphicsViewItem*> leaves;
  QPolygonF leaves_bbox;
  qreal leaf_offset = 0;
  for (const auto& [leaf, arrival] : tree->getLeavesDelays()) {
    auto* leaf_rect
        = addLeafToScene(leaf_offset, PinArrival{leaf, arrival}, network);

    const QRectF bbox = leaf_rect->boundingRect();
    leaf_offset += (1.0 + node_spacing_) * bbox.width();

    leaves_bbox.append(leaf_rect->pos());

    fanout.push_back(leaf_rect);
    leaves.push_back(leaf_rect);
  }
  child_offset = std::max(
      std::min(child_offset, static_cast<int>(bin_center_.size() - 1)), 0);
  const qreal half_leaves_width = leaves_bbox.boundingRect().width() / 2;
  for (auto* leaf : leaves) {
    leaf->moveBy(bin_center_[child_offset] - half_leaves_width, 0);
  }

  if (!fanout.empty()) {
    auto* net_view = new ClockNetGraphicsViewItem(
        network->staToDb(tree->getNet()), drivers, fanout);
    nets_.push_back(net_view);
    scene_->addItem(net_view);
  }

  for (auto* driver : drivers) {
    driver->setupToolTip();
  }
  for (auto* fan : fanout) {
    fan->setupToolTip();
  }

  return drivers;
}

ClockNodeGraphicsViewItem* ClockTreeView::addRootToScene(
    qreal x,
    const PinArrival& output_pin,
    sta::dbNetwork* network)
{
  odb::dbITerm* iterm;
  odb::dbBTerm* bterm;
  odb::dbModBTerm* modbterm;
  odb::dbModITerm* moditerm;

  network->staToDb(output_pin.pin, iterm, bterm, moditerm, modbterm);

  ClockNodeGraphicsViewItem* node = nullptr;
  if (iterm != nullptr) {
    node = new ClockRootNodeGraphicsViewItem(iterm);
  } else {
    node = new ClockRootNodeGraphicsViewItem(bterm);
  }

  QString tooltip;
  tooltip += "Launch: " + convertDelayToString(output_pin.delay);

  addNode(x, node, tooltip, output_pin.delay);

  return node;
}

ClockNodeGraphicsViewItem* ClockTreeView::addLeafToScene(
    qreal x,
    const PinArrival& input_pin,
    sta::dbNetwork* network)
{
  odb::dbITerm* iterm;
  odb::dbBTerm* bterm;
  odb::dbModITerm* moditerm;
  odb::dbModBTerm* modbterm;

  network->staToDb(input_pin.pin, iterm, bterm, moditerm, modbterm);

  // distinguish between registers and macros
  ClockLeafNodeGraphicsViewItem* node;
  odb::dbInst* inst = iterm->getInst();
  float ins_delay = 0.0;
  if (inst->getMaster()->getType().isBlock()) {
    // add insertion delay at macro cell input pin
    sta::LibertyCell* libCell = network->libertyCell(network->dbToSta(inst));
    odb::dbMTerm* mterm = iterm->getMTerm();
    if (libCell && mterm) {
      sta::LibertyPort* libPort
          = libCell->findLibertyPort(mterm->getConstName());
      if (libPort) {
        const float rise = libPort->clkTreeDelay(
            0.0, sta::RiseFall::rise(), sta::MinMax::max());
        const float fall = libPort->clkTreeDelay(
            0.0, sta::RiseFall::fall(), sta::MinMax::max());

        if (rise != 0 || fall != 0) {
          ins_delay = (rise + fall) / 2.0;
        }
      }
    }

    node
        = new ClockMacroNodeGraphicsViewItem(iterm, convertDelayToY(ins_delay));
  } else {
    node = new ClockRegisterNodeGraphicsViewItem(iterm);
  }
  node->scaleSize(leaf_scale_);

  QString tooltip;
  tooltip += "Arrival: " + convertDelayToString(input_pin.delay + ins_delay);

  addNode(x, node, tooltip, input_pin.delay);

  connect(node->getHighlightAction(), &QAction::triggered, [this, iterm]() {
    emit highlightTo(iterm);
  });

  return node;
}

QRectF ClockMacroNodeGraphicsViewItem::getOutlineRect() const
{
  const qreal size = getSize();
  const QPointF ll(-size / 2, std::max(size, insertion_delay_as_height_));
  const QPointF ur(size / 2, 0);
  QRectF rect(ll, ur);
  return rect.normalized();
}

ClockNodeGraphicsViewItem* ClockTreeView::addCellToScene(
    qreal x,
    const PinArrival& input_pin,
    const PinArrival& output_pin,
    sta::dbNetwork* network)
{
  auto convert_pin = [&network](const sta::Pin* pin) -> odb::dbITerm* {
    odb::dbITerm* iterm;
    odb::dbBTerm* bterm;
    odb::dbModITerm* moditerm;
    odb::dbModBTerm* modbterm;
    network->staToDb(pin, iterm, bterm, moditerm, modbterm);
    return iterm;
  };

  odb::dbITerm* input_term = convert_pin(input_pin.pin);
  odb::dbITerm* output_term = convert_pin(output_pin.pin);

  const qreal delay_y
      = convertDelayToY(output_pin.delay) - convertDelayToY(input_pin.delay);

  ClockNodeGraphicsViewItem* node = nullptr;
  bool is_clockgate = false;
  bool is_inverter_buffer = false;
  sta::LibertyPort* lib_port = network->libertyPort(output_pin.pin);
  if (lib_port != nullptr) {
    sta::LibertyCell* cell = lib_port->libertyCell();
    if (cell->isClockGate()) {
      is_clockgate = true;
    } else {
      if (cell->isBuffer() || cell->isInverter()) {
        is_inverter_buffer = true;
      }
    }
  }

  if (is_clockgate) {
    node = new ClockGateNodeGraphicsViewItem(input_term, output_term, delay_y);
  } else if (is_inverter_buffer) {
    ClockBufferNodeGraphicsViewItem* buf_node
        = new ClockBufferNodeGraphicsViewItem(input_term, output_term, delay_y);
    node = buf_node;

    if (lib_port != nullptr) {
      auto function = lib_port->function();
      if (function && function->op() == sta::FuncExpr::op_not) {
        buf_node->setIsInverter(true);
      }
    }
  } else {
    ClockGateNodeGraphicsViewItem* gate_node
        = new ClockGateNodeGraphicsViewItem(input_term, output_term, delay_y);
    gate_node->setIsClockGate(false);
    node = gate_node;
  }

  QString tooltip;
  tooltip += "Input: " + ClockNodeGraphicsViewItem::getITermName(input_term);
  tooltip += "\n";
  tooltip += "Input arrival: " + convertDelayToString(input_pin.delay);
  tooltip += "\n";
  tooltip += "Output: " + ClockNodeGraphicsViewItem::getITermName(output_term);
  tooltip += "\n";
  tooltip += "Output launch: " + convertDelayToString(output_pin.delay);

  addNode(x, node, tooltip, input_pin.delay);

  return node;
}

void ClockTreeView::addNode(qreal x,
                            ClockNodeGraphicsViewItem* node,
                            const QString& tooltip,
                            sta::Delay delay)
{
  node->setPos({x, convertDelayToY(delay)});
  node->setExtraToolTip(tooltip);
  scene_->addItem(node);

  items_[node->getInstName().toStdString()] = node;
}

void ClockTreeView::highlightTo(odb::dbITerm* term)
{
  if (renderer_state_ == RendererState::NeverShow) {
    // need to enable the renderer
    scene_->setRendererState(RendererState::OnlyShowOnActiveWidget);
  }
  renderer_->setPathTo(term);
}

void ClockTreeView::clearHighlightTo()
{
  renderer_->clearPathTo();
}

void ClockTreeView::save(const QString& path)
{
  QString save_path = path;
  if (path.isEmpty()) {
    save_path = Utils::requestImageSavePath(this, "Save clock tree");
    if (save_path.isEmpty()) {
      return;
    }
  }
  save_path = Utils::fixImagePath(save_path, logger_);

  QRect render_rect = viewport()->rect();
  if (!render_rect.isValid()) {
    // When in offscreen mode the viewport is not sized
    render_rect = scene_->sceneRect().toRect();
    render_rect.translate(-render_rect.left(), -render_rect.top());
  }

  show_mouse_time_tick_ = false;
  Utils::renderImage(save_path,
                     viewport(),
                     render_rect.width(),
                     render_rect.height(),
                     render_rect,
                     Qt::white,
                     logger_);
  show_mouse_time_tick_ = true;
}

void ClockTreeView::selectRendererTreeNet(odb::dbNet* net)
{
  renderer_->setTree(tree_->findTree(net));
}

void ClockTreeView::updateColorDepth(int depth)
{
  renderer_->setMaxColorDepth(depth);
}

////////////////

ClockWidget::ClockWidget(QWidget* parent)
    : QDockWidget("Clock Tree Viewer", parent),
      logger_(nullptr),
      block_(nullptr),
      sta_(nullptr),
      stagui_(nullptr),
      update_button_(new QPushButton("Update", this)),
      fit_button_(new QPushButton("Fit", this)),
      corner_box_(new QComboBox(this)),
      clocks_tab_(new QTabWidget(this))
{
  setObjectName("clock_viewer");  // for settings

  QWidget* container = new QWidget(this);

  QHBoxLayout* button_layout = new QHBoxLayout;
  button_layout->addWidget(corner_box_);
  button_layout->addWidget(update_button_);
  button_layout->addWidget(fit_button_);

  corner_box_->setToolTip("Timing corner");

  QVBoxLayout* layout = new QVBoxLayout;
  layout->addLayout(button_layout);
  layout->addWidget(clocks_tab_);

  container->setLayout(layout);
  setWidget(container);

  connect(update_button_, &QPushButton::clicked, [=] { populate(); });
  connect(fit_button_, &QPushButton::clicked, this, &ClockWidget::fit);

  update_button_->setEnabled(false);
  fit_button_->setEnabled(false);

  connect(clocks_tab_,
          &QTabWidget::currentChanged,
          this,
          &ClockWidget::currentClockChanged);
}

ClockWidget::~ClockWidget()
{
  clocks_tab_->clear();
  views_.clear();
}

void ClockWidget::setSTA(sta::dbSta* sta)
{
  sta_ = sta;
  if (sta_ != nullptr) {
    stagui_ = std::make_unique<STAGuiInterface>(sta_);
    stagui_->setMaxPathCount(1);
    stagui_->setIncludeUnconstrainedPaths(false);

    sta_->getDbNetwork()->addObserver(this);
    postReadLiberty();
  }
}

void ClockWidget::setLogger(utl::Logger* logger)
{
  logger_ = logger;
}

void ClockWidget::setBlock(odb::dbBlock* block)
{
  update_button_->setEnabled(block != nullptr);
  fit_button_->setEnabled(block != nullptr);
  block_ = block;
}

void ClockWidget::populate(sta::Corner* corner)
{
  QApplication::setOverrideCursor(Qt::WaitCursor);

  clocks_tab_->clear();
  views_.clear();

  if (corner == nullptr) {
    corner = corner_box_->currentData().value<sta::Corner*>();
  } else {
    corner_box_->setCurrentText(corner->name());
  }
  stagui_->setCorner(corner);

  for (auto& tree : stagui_->getClockTrees()) {
    if (!tree->getNet()) {  // skip virtual clocks
      continue;
    }
    auto* view = new ClockTreeView(std::shared_ptr<ClockTree>(tree.release()),
                                   stagui_.get(),
                                   logger_,
                                   this);
    views_.emplace_back(view);
    clocks_tab_->addTab(view, view->getClockName());
    clocks_tab_->setCurrentWidget(view);
    view->fit();

    connect(view, &ClockTreeView::selected, this, &ClockWidget::selected);
  }
  if (clocks_tab_->count() > 0) {
    clocks_tab_->setCurrentIndex(0);
  }

  QApplication::restoreOverrideCursor();
}

void ClockWidget::hideEvent(QHideEvent* event)
{
  auto* gui = Gui::get();
  for (const auto& view : views_) {
    gui->unregisterRenderer(view->getRenderer());
  }
}

void ClockWidget::showEvent(QShowEvent* event)
{
  for (const auto& view : views_) {
    view->updateRendererState();
  }
}

void ClockWidget::postReadLiberty()
{
  corner_box_->clear();
  for (sta::Corner* corner : *sta_->corners()) {
    corner_box_->addItem(corner->name(), QVariant::fromValue(corner));
  }
}

void ClockWidget::selectClock(const std::string& clock_name)
{
  setVisible(true);

  if (views_.empty()) {
    populate(nullptr);
  }

  for (auto& view : views_) {
    if (view->getClockName() == clock_name) {
      clocks_tab_->setCurrentWidget(view.get());

      return;
    }
  }

  logger_->error(utl::GUI, 74, "Unable to find clock: {}", clock_name);
}

void ClockWidget::saveImage(const std::string& clock_name,
                            const std::string& path,
                            const std::string& corner,
                            const std::optional<int>& width_px,
                            const std::optional<int>& height_px)
{
  const bool visible = isVisible();
  if (!visible) {
    setVisible(true);
  }

  bool populate_views = views_.empty();
  sta::Corner* sta_corner = nullptr;
  if (!corner.empty() && corner != corner_box_->currentText().toStdString()) {
    populate_views = true;
    const int idx = corner_box_->findText(QString::fromStdString(corner));
    if (idx == -1) {
      logger_->error(utl::GUI, 89, "Unable to find \"{}\" corner", corner);
    }
    sta_corner = corner_box_->itemData(idx).value<sta::Corner*>();
  }

  if (populate_views) {
    populate(sta_corner);
  }

  selectClock(clock_name);

  ClockTreeView* view
      = static_cast<ClockTreeView*>(clocks_tab_->currentWidget());

  ClockTreeView print_view(view->getClockTree(), stagui_.get(), logger_, this);
  QSize view_size = view->size();
  if (width_px.has_value()) {
    view_size.setWidth(width_px.value());
  }
  if (height_px.has_value()) {
    view_size.setHeight(height_px.value());
  }
  print_view.scale(1, 1);  // mysteriously necessary sometimes
  print_view.resize(view_size);
  // Ensure the new view is sized correctly by Qt by processing the event
  // so fit will work
  QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
  print_view.fit();
  print_view.save(QString::fromStdString(path));

  setVisible(visible);
}

void ClockWidget::currentClockChanged(int index)
{
  for (const auto& view : views_) {
    view->updateRendererState();
  }
}

void ClockWidget::fit()
{
  if (!views_.empty() && clocks_tab_->currentIndex() < views_.size()) {
    views_[clocks_tab_->currentIndex()]->fit();
  }
}

void ClockWidget::findInCts(const Selected& selection)
{
  if (!selection) {
    return;
  }
  findInCts(SelectionSet({
      selection,
  }));
}

void ClockWidget::findInCts(const SelectionSet& selections)
{
  if (views_.empty()) {
    return;
  }
  if (selections.empty()) {
    return;
  }

  std::set<int> changed_views;
  std::set<int> not_changed_views;

  for (int i = 0; i < views_.size(); i++) {
    bool selection_changed = views_[i]->changeSelection(selections);

    if (selection_changed) {
      views_[i]->fitSelection();
      changed_views.insert(i);
    } else {
      not_changed_views.insert(i);
    }
  }

  if (!changed_views.empty()) {
    if (changed_views.find(clocks_tab_->currentIndex())
        == changed_views.end()) {
      // change the current view
      clocks_tab_->setCurrentIndex(*(changed_views.begin()));
    }
    for (int i : not_changed_views) {
      views_[i]->clearSelection();
    }
  }
}

}  // namespace gui
