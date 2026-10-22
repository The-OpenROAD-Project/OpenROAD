// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "clock_tree_diagram.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iomanip>
#include <ios>
#include <limits>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "color.h"
#include "db_sta/dbNetwork.hh"
#include "odb/db.h"
#include "odb/geom.h"
#include "sta/Delay.hh"
#include "sta/FuncExpr.hh"
#include "sta/Liberty.hh"
#include "sta/MinMax.hh"
#include "sta/NetworkClass.hh"
#include "sta/Transition.hh"
#include "sta/Units.hh"
#include "tile_generator.h"
#include "web/clockTree.h"
#include "web/core.h"

namespace web {

namespace {

using Kind = ClockTreeDiagram::Kind;
using Node = ClockTreeDiagram::Node;
using Point = ClockTreeDiagram::Point;
using Segment = ClockTreeDiagram::Segment;

// ClockNodeGraphicsViewItem's, ClockBufferNodeGraphicsViewItem's,
// ClockNetGraphicsViewItem's and ClockTreeView's constants.
constexpr double kDefaultSize = 100.0;
constexpr double kNodeSpacing = 0.2;
constexpr double kBarScaleSize = 0.1;
constexpr double kTrunkRounding = 0.1;
constexpr double kLeafRounding = 0.1;
// The size Qt's default font draws the time scale's labels at, and how far
// a label's cell reaches below its baseline.
constexpr double kTimeScaleFontPx = 18.5;
constexpr double kTimeScaleDescent = 0.2 * kTimeScaleFontPx;

// The Qt::GlobalColor each kind is outlined in.
Painter::Color outlineColor(const Kind kind)
{
  switch (kind) {
    case Kind::kRoot:
    case Kind::kRegister:
      return Painter::kRed;
    case Kind::kBuffer:
      return Painter::kBlue;
    case Kind::kInverter:
    case Kind::kMacro:
      return Painter::kDarkCyan;
    case Kind::kClockGate:
      return Painter::kMagenta;
    case Kind::kAssumedClockGate:
      return Painter::kDarkGray;
  }
  return Painter::kDarkGray;
}

bool isLeaf(const Kind kind)
{
  return kind == Kind::kRegister || kind == Kind::kMacro;
}

Point topAnchor(const Node& node)
{
  if (node.kind == Kind::kRoot) {
    return {.x = node.pos.x, .y = node.pos.y - node.size};
  }
  return node.pos;
}

Point bottomAnchor(const Node& node)
{
  if (node.kind == Kind::kRoot) {
    return node.pos;
  }
  if (isLeaf(node.kind)) {
    return {.x = node.pos.x, .y = node.pos.y + node.size};
  }
  return {.x = node.pos.x, .y = node.pos.y + node.delay_y};
}

struct Box
{
  double x0 = std::numeric_limits<double>::max();
  double y0 = std::numeric_limits<double>::max();
  double x1 = std::numeric_limits<double>::lowest();
  double y1 = std::numeric_limits<double>::lowest();

  void add(const double x, const double y)
  {
    x0 = std::min(x0, x);
    y0 = std::min(y0, y);
    x1 = std::max(x1, x);
    y1 = std::max(y1, y);
  }
  double width() const { return x1 - x0; }
  double height() const { return y1 - y0; }
};

// The node's shape().boundingRect(), around its position.
Box nodeBounds(const Node& node)
{
  const double s = node.size;
  Box box;
  auto add_fin = [&box, s](const double delay) {
    box.add(0, s);
    box.add(-s / 20, delay);
    box.add(s / 20, delay);
  };
  switch (node.kind) {
    case Kind::kRoot:
      box.add(-s / 2, -s);
      box.add(s / 2, 0);
      break;
    case Kind::kBuffer:
    case Kind::kInverter: {
      const double bar = s * kBarScaleSize;
      const double tip = node.kind == Kind::kInverter ? s - bar : s;
      box.add(-tip / 2, 0);
      box.add(tip / 2, tip);
      if (node.kind == Kind::kInverter) {
        box.add(-bar / 2, s - bar);
        box.add(bar / 2, s);
      }
      add_fin(node.delay_y);
      break;
    }
    case Kind::kClockGate:
    case Kind::kAssumedClockGate:
      box.add(-s / 2, 0);
      box.add(s / 2, s);
      add_fin(node.delay_y);
      break;
    case Kind::kRegister:
      box.add(-s / 2, 0);
      box.add(s / 2, s);
      break;
    case Kind::kMacro:
      box.add(-s / 2, 0);
      box.add(s / 2, std::max(s, node.delay_y));
      break;
  }
  return box;
}

odb::dbITerm* toITerm(sta::dbNetwork* network, const sta::Pin* pin)
{
  if (pin == nullptr) {
    return nullptr;
  }
  odb::dbITerm* iterm = nullptr;
  odb::dbBTerm* bterm = nullptr;
  odb::dbModITerm* moditerm = nullptr;
  network->staToDb(pin, iterm, bterm, moditerm);
  return iterm;
}

// ClockTreeView::build and buildTree, with each node and net as it adds them.
class Builder
{
 public:
  Builder(const ClockTree& tree, ClockTreeDiagram& diagram)
      : tree_(tree), diagram_(diagram), network_(tree.getNetwork())
  {
  }

  void build();

 private:
  std::vector<int> buildTree(const ClockTree& tree, int center_index);
  int addRoot(double x, const sta::Pin* pin, sta::Delay delay);
  int addCell(double x,
              const sta::Pin* input,
              sta::Delay input_delay,
              const sta::Pin* output,
              sta::Delay output_delay,
              const ClockTree& tree);
  int addLeaf(double x, const sta::Pin* pin, sta::Delay delay, bool visible);
  int addNode(double x, Node node, sta::Delay delay, const std::string& name);
  void addNet(const std::vector<int>& sources, const std::vector<int>& sinks);
  void addLeafPath(std::vector<Segment>& path,
                   const Point& start,
                   double y_trunk,
                   const Point& end);
  int findNode(const std::string& name) const;

  const ClockTree& tree_;
  ClockTreeDiagram& diagram_;
  sta::dbNetwork* network_;
  double leaf_scale_ = 1.0;
  std::vector<double> bin_center_;
  // By instance (or port) name: a cell met again is moved, not added.
  std::map<std::string, int> items_;
};

void Builder::build()
{
  diagram_.min_delay = sta::delayAsFloat(tree_.getMinimumArrival(true));
  diagram_.max_delay = sta::delayAsFloat(tree_.getMaximumArrival(true));
  const int max_leaves = tree_.getMaxLeaves(true);
  if (max_leaves > 0) {
    leaf_scale_ = 1.0 / ((1.0 + kNodeSpacing) * max_leaves);
  }

  const int tree_width = std::max(1, tree_.getTotalFanout(true));
  const double bin_pitch = (1.0 + kNodeSpacing) * kDefaultSize;
  for (int i = 0; i < tree_width; i++) {
    bin_center_.push_back(i * bin_pitch);
  }

  const double est_scene_width
      = (tree_width - 1) * (1 + kNodeSpacing) * kDefaultSize + kDefaultSize;
  double min_time_scale
      = kDefaultSize * (diagram_.max_delay - diagram_.min_delay)
        / sta::delayAsFloat(tree_.getMinimumDriverDelay(true));
  if (!std::isfinite(min_time_scale)) {
    min_time_scale = est_scene_width;
  }
  diagram_.time_scale = tree_width == 1
                            ? min_time_scale
                            : std::max(est_scene_width, min_time_scale);

  buildTree(tree_, static_cast<int>(bin_center_.size()) / 2);

  // QGraphicsScene::itemsBoundingRect, then ClockTreeView::build's margins.
  Box scene;
  for (const Node& node : diagram_.nodes) {
    const Box box = nodeBounds(node);
    scene.add(node.pos.x + box.x0, node.pos.y + box.y0);
    scene.add(node.pos.x + box.x1, node.pos.y + box.y1);
  }
  for (const auto& net : diagram_.nets) {
    for (const Segment& segment : net) {
      scene.add(segment.from.x, segment.from.y);
      scene.add(segment.to.x, segment.to.y);
    }
  }
  const double x_margin = 0.1 * scene.width();
  const double y_margin = 0.1 * scene.height();
  diagram_.scene_min = {.x = scene.x0 - 2 * x_margin, .y = scene.y0 - y_margin};
  diagram_.scene_max = {.x = scene.x1 + x_margin, .y = scene.y1 + y_margin};
}

std::vector<int> Builder::buildTree(const ClockTree& tree, int center_index)
{
  center_index
      = std::clamp(center_index, 0, static_cast<int>(bin_center_.size()) - 1);

  std::vector<int> drivers;
  Box driver_box;
  double driver_offset = 0;
  const ClockTree* parent_tree = tree.getParent();
  for (const auto& [driver, arrival] : tree.getDriverDelays()) {
    int node = -1;
    if (tree.isRoot()) {
      node = addRoot(driver_offset, driver, arrival);
    } else {
      const sta::Pin* input = nullptr;
      sta::Delay input_delay = 0.0;
      if (parent_tree != nullptr) {
        const auto& [parent_sink, time] = parent_tree->getPairedSink(driver);
        input = parent_sink;
        input_delay = time;
      }
      node = addCell(driver_offset, input, input_delay, driver, arrival, tree);
      if (!tree.isVisible()) {
        continue;
      }
    }
    if (node < 0) {
      continue;
    }

    driver_offset
        += (1.0 + kNodeSpacing) * nodeBounds(diagram_.nodes[node]).width();
    driver_box.add(diagram_.nodes[node].pos.x, diagram_.nodes[node].pos.y);
    drivers.push_back(node);
  }
  const double half_nodes_width
      = drivers.empty() ? 0.0 : driver_box.width() / 2;
  for (const int driver : drivers) {
    diagram_.nodes[driver].pos.x
        += bin_center_[center_index] - half_nodes_width;
  }

  int child_offset = center_index - (tree.getTotalFanout(true) / 2);
  std::vector<int> fanout;
  for (const auto& fan_tree : tree.getFanout()) {
    const int fan_width = fan_tree->getTotalFanout(true);
    const std::vector<int> fanout_nodes
        = buildTree(*fan_tree, child_offset + fan_width / 2);
    if (!tree.getSubtreeVisibility()) {
      continue;
    }
    fanout.insert(fanout.end(), fanout_nodes.begin(), fanout_nodes.end());

    child_offset += fan_width;
  }

  std::vector<int> leaves;
  Box leaves_box;
  double leaf_offset = 0;
  for (const auto& [leaf, arrival] : tree.getLeavesDelays()) {
    const int node
        = addLeaf(leaf_offset, leaf, arrival, tree.getSubtreeVisibility());
    if (node < 0 || !tree.getSubtreeVisibility()) {
      continue;
    }
    leaf_offset
        += (1.0 + kNodeSpacing) * nodeBounds(diagram_.nodes[node]).width();
    leaves_box.add(diagram_.nodes[node].pos.x, diagram_.nodes[node].pos.y);

    fanout.push_back(node);
    leaves.push_back(node);
  }
  child_offset
      = std::clamp(child_offset, 0, static_cast<int>(bin_center_.size()) - 1);
  const double half_leaves_width
      = leaves.empty() ? 0.0 : leaves_box.width() / 2;
  for (const int leaf : leaves) {
    diagram_.nodes[leaf].pos.x += bin_center_[child_offset] - half_leaves_width;
  }

  if (!fanout.empty()) {
    addNet(drivers, fanout);
  }
  return drivers;
}

int Builder::findNode(const std::string& name) const
{
  const auto it = items_.find(name);
  return it == items_.end() ? -1 : it->second;
}

int Builder::addRoot(const double x,
                     const sta::Pin* pin,
                     const sta::Delay delay)
{
  odb::dbITerm* iterm = nullptr;
  odb::dbBTerm* bterm = nullptr;
  odb::dbModITerm* moditerm = nullptr;
  network_->staToDb(pin, iterm, bterm, moditerm);
  if (iterm == nullptr && bterm == nullptr) {
    return -1;
  }
  const std::string name
      = iterm != nullptr ? iterm->getInst()->getName() : bterm->getName();
  if (const int node = findNode(name); node >= 0) {
    diagram_.nodes[node].pos = {.x = x, .y = diagram_.delayToY(delay)};
    return node;
  }
  return addNode(
      x, Node{.kind = Kind::kRoot, .size = kDefaultSize}, delay, name);
}

int Builder::addCell(const double x,
                     const sta::Pin* input,
                     const sta::Delay input_delay,
                     const sta::Pin* output,
                     const sta::Delay output_delay,
                     const ClockTree& tree)
{
  odb::dbITerm* input_term = toITerm(network_, input);
  odb::dbITerm* output_term = toITerm(network_, output);
  if (input_term == nullptr) {
    input_term = output_term;
  }
  if (input_term == nullptr) {
    return -1;
  }

  const double delay_y
      = diagram_.delayToY(output_delay) - diagram_.delayToY(input_delay);

  bool is_clockgate = false;
  bool is_inverter_buffer = false;
  bool is_inverter = false;
  if (sta::LibertyPort* lib_port = network_->libertyPort(output)) {
    sta::LibertyCell* cell = lib_port->libertyCell();
    if (cell->isClockGate()) {
      is_clockgate = true;
    } else if (cell->isBuffer() || cell->isInverter()) {
      is_inverter_buffer = true;
      const sta::FuncExpr* function = lib_port->function();
      is_inverter
          = function != nullptr && function->op() == sta::FuncExpr::Op::not_;
    }
  }

  const std::string name = input_term->getInst()->getName();
  int node = findNode(name);
  if (node >= 0) {
    diagram_.nodes[node].pos = {.x = x, .y = diagram_.delayToY(input_delay)};
    diagram_.nodes[node].delay_y = delay_y;
  } else {
    Kind kind = Kind::kAssumedClockGate;
    if (is_clockgate) {
      kind = Kind::kClockGate;
    } else if (is_inverter_buffer) {
      kind = is_inverter ? Kind::kInverter : Kind::kBuffer;
    }
    node = addNode(x,
                   Node{.kind = kind, .size = kDefaultSize, .delay_y = delay_y},
                   input_delay,
                   name);
  }
  diagram_.nodes[node].visible = tree.isVisible();
  return node;
}

int Builder::addLeaf(const double x,
                     const sta::Pin* pin,
                     const sta::Delay delay,
                     const bool visible)
{
  odb::dbITerm* iterm = toITerm(network_, pin);
  if (iterm == nullptr) {
    return -1;
  }
  odb::dbInst* inst = iterm->getInst();
  const std::string name = inst->getName();
  int node = findNode(name);
  if (node >= 0) {
    diagram_.nodes[node].pos = {.x = x, .y = diagram_.delayToY(delay)};
  } else {
    Node leaf{.kind = Kind::kRegister, .size = kDefaultSize * leaf_scale_};
    if (inst->getMaster()->getType().isBlock()) {
      // The insertion delay at the macro's clock pin, drawn as its height.
      float insertion_delay = 0.0;
      sta::LibertyCell* lib_cell
          = network_->libertyCell(network_->dbToSta(inst));
      odb::dbMTerm* mterm = iterm->getMTerm();
      if (lib_cell != nullptr && mterm != nullptr) {
        if (sta::LibertyPort* lib_port
            = lib_cell->findLibertyPort(mterm->getConstName())) {
          const float rise = lib_port->clkTreeDelay(
              0.0, sta::RiseFall::rise(), sta::MinMax::max());
          const float fall = lib_port->clkTreeDelay(
              0.0, sta::RiseFall::fall(), sta::MinMax::max());
          insertion_delay = rise + fall;
          if (rise != 0 && fall != 0) {
            insertion_delay /= 2.0;
          }
        }
      }
      leaf.kind = Kind::kMacro;
      leaf.delay_y = diagram_.delayToY(insertion_delay);
    }
    node = addNode(x, leaf, delay, name);
  }
  diagram_.nodes[node].visible = visible;
  return node;
}

int Builder::addNode(const double x,
                     Node node,
                     const sta::Delay delay,
                     const std::string& name)
{
  node.pos = {.x = x, .y = diagram_.delayToY(delay)};
  node.name = name;
  diagram_.nodes.push_back(node);
  const int index = static_cast<int>(diagram_.nodes.size()) - 1;
  items_[name] = index;
  return index;
}

// ClockNetGraphicsViewItem::buildPath: a trunk down from the drivers' center,
// joined to each driver and each sink.
void Builder::addNet(const std::vector<int>& sources,
                     const std::vector<int>& sinks)
{
  if (sources.empty()) {
    return;
  }
  double source_ymin = std::numeric_limits<double>::lowest();
  double source_xcenter = 0;
  for (const int source : sources) {
    const Point anchor = bottomAnchor(diagram_.nodes[source]);
    source_ymin = std::max(source_ymin, anchor.y);
    source_xcenter += anchor.x;
  }
  source_xcenter /= static_cast<double>(sources.size());

  double sink_ymax = std::numeric_limits<double>::max();
  for (const int sink : sinks) {
    sink_ymax = std::min(sink_ymax, topAnchor(diagram_.nodes[sink]).y);
  }

  const double trunk_distance = sink_ymax - source_ymin;
  const Point trunk_top{
      .x = source_xcenter,
      .y = source_ymin + kTrunkRounding * trunk_distance,
  };
  const Point trunk_bot{
      .x = source_xcenter,
      .y = sink_ymax - kTrunkRounding * trunk_distance,
  };

  std::vector<Segment> path;
  for (const int source : sources) {
    addLeafPath(
        path, bottomAnchor(diagram_.nodes[source]), trunk_top.y, trunk_top);
  }
  path.push_back({.from = trunk_top, .to = trunk_bot, .control = std::nullopt});
  for (const int sink : sinks) {
    addLeafPath(path, trunk_bot, trunk_bot.y, topAnchor(diagram_.nodes[sink]));
  }
  diagram_.nets.push_back(std::move(path));
}

void Builder::addLeafPath(std::vector<Segment>& path,
                          const Point& start,
                          const double y_trunk,
                          const Point& end)
{
  const double x_offset = kLeafRounding * (end.x - start.x);
  const Point mid{.x = end.x - x_offset, .y = y_trunk};
  path.push_back({.from = start, .to = mid, .control = std::nullopt});
  path.push_back(
      {.from = mid, .to = end, .control = Point{.x = end.x, .y = y_trunk}});
}

// Draws in output pixels, y down, onto a painter that works in units of
// 1/kUnits pixel of an image kSupersample times larger, y up.
class PixelPainter
{
 public:
  static constexpr int kSupersample = 2;
  static constexpr int kUnits = 8;

  PixelPainter(Painter& painter, const int height)
      : painter_(painter), height_(height)
  {
  }

  odb::Point at(const double x, const double y) const
  {
    constexpr double kScale = static_cast<double>(kSupersample) * kUnits;
    return {static_cast<int>(std::lround(x * kScale)),
            static_cast<int>(std::lround((height_ - y) * kScale))};
  }
  int length(const double pixels) const
  {
    return static_cast<int>(std::lround(pixels * kSupersample * kUnits));
  }
  // A cosmetic pen, one output pixel wide.
  void setPen(const Painter::Color& color)
  {
    painter_.setPen(color, true, kSupersample);
  }
  void setBrush(const Color& color)
  {
    painter_.setBrush(toPainterColor(color), Painter::kSolid);
  }
  void noBrush() { painter_.setBrush(Painter::kTransparent, Painter::kNone); }
  Painter& painter() { return painter_; }

 private:
  Painter& painter_;
  const int height_;
};

// ClockTreeView::drawTimeScale, with no mouse tick, as save() draws it.
void drawTimeScale(const ClockTreeDiagram& diagram,
                   const int width,
                   const int height,
                   double min_time,
                   double max_time,
                   PixelPainter& out)
{
  min_time /= diagram.unit_scale;
  max_time /= diagram.unit_scale;
  const float time_range = static_cast<float>(max_time - min_time);
  if (!(time_range > 0)) {
    return;
  }

  constexpr int kDigits = 2;
  const int tick_length = std::min(20, width / 20);
  auto time_to_y = [&](const float time) {
    return static_cast<int>((time - min_time) / time_range * height);
  };

  out.setPen(Painter::kBlack);
  out.painter().drawLine(out.at(0, height - 1), out.at(0, 0));

  constexpr int kMinTickMarks = 2;
  const float scale_time = std::pow(10.0F, kDigits);
  float tick_interval
      = std::pow(10.0F, std::floor(std::log10(time_range * scale_time)))
        / scale_time;
  if (time_range / tick_interval < kMinTickMarks) {
    tick_interval /= 10;
  }
  tick_interval = std::max(tick_interval, 1 / scale_time);

  float tick = std::floor(static_cast<float>(min_time) / tick_interval)
               * tick_interval;
  while (tick < max_time) {
    const int y = time_to_y(tick);
    out.painter().drawLine(out.at(0, y), out.at(tick_length, y));
    // Qt prints a tick a rounding error below zero as "0.00".
    const float shown = std::abs(tick) < tick_interval / 2 ? 0.0F : tick;
    std::ostringstream text;
    text << std::fixed << std::setprecision(kDigits) << shown << " "
         << diagram.unit_suffix;
    // Qt's drawText puts the baseline, not the cell's bottom, on the tick.
    const odb::Point at = out.at(tick_length, y + kTimeScaleDescent);
    out.painter().drawString(at.x(), at.y(), Painter::kBottomLeft, text.str());
    tick += tick_interval;
  }
}

void drawFin(const Node& node,
             const ClockTreeDiagram::Point& center,
             const double scale,
             PixelPainter& out)
{
  const double s = node.size * scale;
  const double fin_y = center.y + node.delay_y * scale;
  out.painter().drawLine(out.at(center.x, center.y + s),
                         out.at(center.x, fin_y));
  out.painter().drawLine(out.at(center.x - s / 20, fin_y),
                         out.at(center.x + s / 20, fin_y));
}

void drawTriangle(const ClockTreeDiagram::Point& top_center,
                  const double size,
                  PixelPainter& out)
{
  const odb::Point left = out.at(top_center.x - size / 2, top_center.y);
  out.painter().drawPolygon({
      left,
      out.at(top_center.x + size / 2, top_center.y),
      out.at(top_center.x, top_center.y + size),
      left,
  });
}

// The node's paint(): its shape in the outline color, filled lighter.
void drawNode(const Node& node,
              const ClockTreeDiagram::Point& pos,
              const double scale,
              PixelPainter& out)
{
  const Painter::Color outline = outlineColor(node.kind);
  out.setPen(outline);
  out.setBrush(toTileColor(outline).lighter());
  const double s = node.size * scale;
  switch (node.kind) {
    case Kind::kRoot:
      drawTriangle({.x = pos.x, .y = pos.y - s}, s, out);
      break;
    case Kind::kBuffer:
      drawTriangle(pos, s, out);
      out.noBrush();
      drawFin(node, pos, scale, out);
      break;
    case Kind::kInverter: {
      const double bar = s * kBarScaleSize;
      drawTriangle(pos, s - bar, out);
      const odb::Point bubble = out.at(pos.x, pos.y + s - bar / 2);
      out.painter().drawCircle(bubble.x(), bubble.y(), out.length(bar / 2));
      out.noBrush();
      drawFin(node, pos, scale, out);
      break;
    }
    case Kind::kClockGate:
    case Kind::kAssumedClockGate: {
      const odb::Point center = out.at(pos.x, pos.y + s / 2);
      out.painter().drawCircle(center.x(), center.y(), out.length(s / 2));
      out.noBrush();
      drawFin(node, pos, scale, out);
      break;
    }
    case Kind::kRegister:
    case Kind::kMacro: {
      const double h
          = node.kind == Kind::kMacro ? std::max(s, node.delay_y * scale) : s;
      out.painter().drawRect(odb::Rect(out.at(pos.x - s / 2, pos.y + h),
                                       out.at(pos.x + s / 2, pos.y)));
      // The clock input.
      const odb::Point left = out.at(pos.x - s / 8, pos.y);
      out.painter().drawPolygon({
          left,
          out.at(pos.x + s / 8, pos.y),
          out.at(pos.x, pos.y + s / 4),
          left,
      });
      break;
    }
  }
}

}  // namespace

double ClockTreeDiagram::delayToY(const double delay) const
{
  const double range = max_delay - min_delay;
  if (!(range > 0)) {
    return 0;
  }
  return (delay - min_delay) / range * time_scale;
}

double ClockTreeDiagram::yToDelay(const double y) const
{
  if (!(time_scale > 0)) {
    return min_delay;
  }
  return y / time_scale * (max_delay - min_delay) + min_delay;
}

ClockTreeDiagram layoutClockTreeDiagram(const ClockTree& tree)
{
  ClockTreeDiagram diagram;
  const sta::Unit* unit = tree.getNetwork()->units()->timeUnit();
  diagram.unit_scale = unit->scale();
  diagram.unit_suffix = unit->scaleAbbrevSuffix();
  Builder(tree, diagram).build();
  return diagram;
}

std::vector<unsigned char> renderClockTreeDiagram(
    const ClockTreeDiagram& diagram,
    const int width,
    const int height,
    const TileGenerator& tiles)
{
  constexpr int kSs = PixelPainter::kSupersample;

  // QGraphicsView::fitInView with KeepAspectRatio, and its 2 pixel margin.
  const double scene_w = diagram.scene_max.x - diagram.scene_min.x;
  const double scene_h = diagram.scene_max.y - diagram.scene_min.y;
  double scale = 1.0;
  if (scene_w > 0 && scene_h > 0) {
    scale = std::min((width - 4) / scene_w, (height - 4) / scene_h);
  }
  const double center_x = (diagram.scene_min.x + diagram.scene_max.x) / 2;
  const double center_y = (diagram.scene_min.y + diagram.scene_max.y) / 2;
  auto to_px = [&](const Point& p) {
    return Point{
        .x = (p.x - center_x) * scale + width / 2.0,
        .y = (p.y - center_y) * scale + height / 2.0,
    };
  };

  auto pass = [&](Painter& painter) {
    PixelPainter out(painter, height);
    painter.setFont(Painter::Font(
        "sans-serif", static_cast<int>(std::lround(kTimeScaleFontPx * kSs))));

    out.setPen(Painter::kDarkGreen);
    out.noBrush();
    for (const auto& net : diagram.nets) {
      for (const Segment& segment : net) {
        const Point from = to_px(segment.from);
        const Point to = to_px(segment.to);
        if (!segment.control) {
          painter.drawLine(out.at(from.x, from.y), out.at(to.x, to.y));
          continue;
        }
        const Point control = to_px(*segment.control);
        constexpr int kPieces = 8;
        Point last = from;
        for (int i = 1; i <= kPieces; ++i) {
          const double t = static_cast<double>(i) / kPieces;
          const double u = 1 - t;
          const Point next{
              .x = u * u * from.x + 2 * u * t * control.x + t * t * to.x,
              .y = u * u * from.y + 2 * u * t * control.y + t * t * to.y,
          };
          painter.drawLine(out.at(last.x, last.y), out.at(next.x, next.y));
          last = next;
        }
      }
    }

    for (const Node& node : diagram.nodes) {
      if (node.visible) {
        drawNode(node, to_px(node.pos), scale, out);
      }
    }

    const double top = center_y - height / 2.0 / scale;
    const double bottom = center_y + height / 2.0 / scale;
    drawTimeScale(diagram,
                  width,
                  height,
                  diagram.yToDelay(top),
                  diagram.yToDelay(bottom),
                  out);
  };

  const std::vector<unsigned char> large
      = tiles.renderPainterImage(width * kSs,
                                 height * kSs,
                                 toTileColor(Painter::kWhite),
                                 PixelPainter::kUnits,
                                 pass);

  // Averaging each kSs x kSs block smooths the edges, as Qt's antialiasing.
  std::vector<unsigned char> image(4UL * width * height);
  const size_t large_width = static_cast<size_t>(width) * kSs;
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      for (int c = 0; c < 4; ++c) {
        int sum = 0;
        for (int dy = 0; dy < kSs; ++dy) {
          for (int dx = 0; dx < kSs; ++dx) {
            const size_t row = static_cast<size_t>(y) * kSs + dy;
            const size_t col = static_cast<size_t>(x) * kSs + dx;
            sum += large[4 * (row * large_width + col) + c];
          }
        }
        image[4 * (static_cast<size_t>(y) * width + x) + c]
            = static_cast<unsigned char>(sum / (kSs * kSs));
      }
    }
  }
  return image;
}

}  // namespace web
