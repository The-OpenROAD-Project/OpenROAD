// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#include "shape.h"

#include <algorithm>
#include <array>
#include <functional>
#include <memory>
#include <set>
#include <utility>
#include <vector>

#include "boost/geometry/geometry.hpp"
#include "boost/geometry/index/predicates.hpp"
#include "boost/polygon/polygon.hpp"
#include "grid.h"
#include "grid_component.h"
#include "odb/db.h"
#include "odb/dbSet.h"
#include "odb/dbTransform.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "techlayer.h"
#include "utl/Logger.h"
#include "via.h"

namespace pdn {

Shape::Shape(odb::dbTechLayer* layer,
             odb::dbNet* net,
             const odb::Rect& rect,
             const odb::dbWireShapeType& type)
    : layer_(layer),
      net_(net),
      rect_(rect),
      type_(type),
      shape_type_(SHAPE),
      allow_non_preferred_change_(false),
      is_locked_(false),
      obs_(rect_),
      grid_component_(nullptr)
{
}

Shape::Shape(odb::dbTechLayer* layer,
             const odb::Rect& rect,
             ShapeType shape_type)
    : layer_(layer),
      net_(nullptr),
      rect_(rect),
      type_(odb::dbWireShapeType::NONE),
      shape_type_(shape_type),
      allow_non_preferred_change_(false),
      is_locked_(false),
      obs_(rect_),
      grid_component_(nullptr)
{
}

Shape::~Shape() = default;

utl::Logger* Shape::getLogger() const
{
  return grid_component_->getLogger();
}

std::unique_ptr<Shape> Shape::copy() const
{
  auto shape = std::make_unique<Shape>(layer_, net_, rect_, type_);
  shape->shape_type_ = shape_type_;
  shape->obs_ = obs_;
  shape->iterm_connections_ = iterm_connections_;
  shape->bterm_connections_ = bterm_connections_;
  shape->allow_non_preferred_change_ = allow_non_preferred_change_;
  return shape;
}

void Shape::merge(Shape* shape)
{
  rect_.merge(shape->rect_);
  iterm_connections_.insert(shape->iterm_connections_.begin(),
                            shape->iterm_connections_.end());
  bterm_connections_.insert(shape->bterm_connections_.begin(),
                            shape->bterm_connections_.end());
  generateObstruction();
}

Shape::ObstructionHalo Shape::getObstructionHalo() const
{
  return {rect_.xMin() - obs_.xMin(),
          obs_.yMax() - rect_.yMax(),
          obs_.xMax() - rect_.xMax(),
          rect_.yMin() - obs_.yMin()};
}

odb::Rect Shape::getRectWithLargestObstructionHalo(
    const ObstructionHalo& halo) const
{
  const ObstructionHalo obs = getObstructionHalo();
  odb::Rect obs_rect = rect_;
  obs_rect.set_xlo(obs_rect.xMin() - std::max(obs.left, halo.left));
  obs_rect.set_yhi(obs_rect.yMax() + std::max(obs.top, halo.top));
  obs_rect.set_xhi(obs_rect.xMax() + std::max(obs.right, halo.right));
  obs_rect.set_ylo(obs_rect.yMin() - std::max(obs.bottom, halo.bottom));
  return obs_rect;
}

int Shape::getNumberOfConnections() const
{
  return vias_.size() + iterm_connections_.size() + bterm_connections_.size();
}

int Shape::getNumberOfConnectionsBelow() const
{
  int connections = 0;
  for (const auto& via : vias_) {
    if (via->getUpperLayer() == layer_) {
      connections++;
    }
  }

  return connections;
}

int Shape::getNumberOfConnectionsAbove() const
{
  int connections = 0;
  for (const auto& via : vias_) {
    if (via->getLowerLayer() == layer_) {
      connections++;
    }
  }

  return connections;
}

bool Shape::isValid() const
{
  // check if shape has a valid area
  if (layer_->hasArea()) {
    if (rect_.area() < layer_->getArea()) {
      return false;
    }
  }

  return true;
}

bool Shape::isWrongWay() const
{
  if (isHorizontal()
      && layer_->getDirection() == odb::dbTechLayerDir::HORIZONTAL) {
    return true;
  }
  if (isVertical() && layer_->getDirection() == odb::dbTechLayerDir::VERTICAL) {
    return true;
  }
  return false;
}

void Shape::updateIBTermConnections(std::set<odb::Rect>& terms)
{
  std::set<odb::Rect> remove_terms;
  for (const odb::Rect& term : terms) {
    if (!rect_.overlaps(term)) {
      remove_terms.insert(term);
    }
  }
  for (const odb::Rect& term : remove_terms) {
    terms.erase(term);
  }
}

void Shape::updateTermConnections()
{
  updateIBTermConnections(iterm_connections_);
  updateIBTermConnections(bterm_connections_);
}

bool Shape::hasTermConnections() const
{
  return hasITermConnections() || hasBTermConnections();
}

odb::Rect Shape::getMinimumRect() const
{
  odb::Rect intersected_rect;
  intersected_rect.mergeInit();
  // merge all bterms
  for (const auto& bterm : bterm_connections_) {
    intersected_rect.merge(bterm);
  }

  // merge all iterms
  for (const odb::Rect& iterm : iterm_connections_) {
    intersected_rect.merge(iterm);
  }

  // merge all vias
  for (auto& via : vias_) {
    intersected_rect.merge(via->getArea());
  }

  return intersected_rect;
}

bool Shape::cut(const ObstructionTree& obstructions,
                const Grid* ignore_grid,
                std::vector<std::unique_ptr<Shape>>& replacements) const
{
  return cut(
      obstructions, replacements, [ignore_grid](const ShapePtr& other) -> bool {
        if (other->shapeType() != GRID_OBS) {
          return true;
        }
        const GridObsShape* shape = static_cast<GridObsShape*>(other.get());
        return !shape->belongsTo(ignore_grid);
      });
}

bool Shape::cut(const ObstructionTree& obstructions,
                std::vector<std::unique_ptr<Shape>>& replacements,
                const std::function<bool(const ShapePtr&)>& obs_filter) const
{
  using boost::polygon::operators::operator-=;
  using Rectangle = boost::polygon::rectangle_data<int>;
  using Polygon90 = boost::polygon::polygon_90_with_holes_data<int>;
  using Polygon90Set = boost::polygon::polygon_90_set_data<int>;
  using Pt = Polygon90::point_type;

  const bool is_horizontal = isHorizontal();

  const ObstructionHalo obs_halo = getObstructionHalo();

  std::vector<Polygon90> shape_violations;
  for (auto it = obstructions.qbegin(bgi::intersects(getObstruction())
                                     && bgi::satisfies([&](const auto& other) {
                                          return layer_ == other->getLayer()
                                                 || other->getLayer()
                                                        == nullptr;
                                        })
                                     && bgi::satisfies(obs_filter));
       it != obstructions.qend();
       it++) {
    const auto& other_shape = *it;

    if (other_shape->net_ != nullptr && net_ == other_shape->net_) {
      // obstruction is of the same net, so see if the violation is completely
      // inside the new strap and therefore is okay
      if (is_horizontal) {
        if (rect_.yMin() <= other_shape->rect_.yMin()
            && rect_.yMax() >= other_shape->rect_.yMax()) {
          continue;
        }
      } else {
        if (rect_.xMin() <= other_shape->rect_.xMin()
            && rect_.xMax() >= other_shape->rect_.xMax()) {
          continue;
        }
      }
    }

    odb::Rect vio_rect
        = other_shape->getRectWithLargestObstructionHalo(obs_halo);
    // ensure the violation overlap fully with the shape to make cut correctly
    if (is_horizontal) {
      vio_rect.set_ylo(std::min(obs_.yMin(), vio_rect.yMin()));
      vio_rect.set_yhi(std::max(obs_.yMax(), vio_rect.yMax()));
    } else {
      vio_rect.set_xlo(std::min(obs_.xMin(), vio_rect.xMin()));
      vio_rect.set_xhi(std::max(obs_.xMax(), vio_rect.xMax()));
    }
    std::array<Pt, 4> pts = {Pt(vio_rect.xMin(), vio_rect.yMin()),
                             Pt(vio_rect.xMax(), vio_rect.yMin()),
                             Pt(vio_rect.xMax(), vio_rect.yMax()),
                             Pt(vio_rect.xMin(), vio_rect.yMax())};

    Polygon90 poly;
    poly.set(pts.begin(), pts.end());

    // save violating polygon
    shape_violations.push_back(poly);
  }

  // check if violations is empty and return no new shapes
  if (shape_violations.empty()) {
    return false;
  }

  const std::array<Pt, 4> pts = {Pt(obs_.xMin(), obs_.yMin()),
                                 Pt(obs_.xMax(), obs_.yMin()),
                                 Pt(obs_.xMax(), obs_.yMax()),
                                 Pt(obs_.xMin(), obs_.yMax())};

  Polygon90 poly;
  poly.set(pts.begin(), pts.end());
  std::array<Polygon90, 1> arr{poly};

  Polygon90Set new_shape(boost::polygon::HORIZONTAL, arr.begin(), arr.end());

  // remove all violations from the shape
  for (const auto& violation : shape_violations) {
    new_shape -= violation;
  }

  std::vector<Rectangle> rects;
  new_shape.get_rectangles(rects);

  for (auto& r : rects) {
    const odb::Rect new_obs_rect(xl(r), yl(r), xh(r), yh(r));
    if (!new_obs_rect.overlaps(rect_)) {
      // New shape will exceed original
      continue;
    }
    const odb::Rect new_rect = new_obs_rect.intersect(rect_);

    // check if new shape should be accepted,
    // only shapes with the same width will be used
    bool accept = false;
    if (is_horizontal) {
      accept = rect_.dy() == new_rect.dy();
    } else {
      accept = rect_.dx() == new_rect.dx();
    }

    if (accept) {
      auto new_shape = copy();
      new_shape->setRect(new_rect);
      new_shape->updateTermConnections();

      replacements.push_back(std::move(new_shape));
    }
  }
  return true;
}

bool Shape::hasDBConnectivity() const
{
  if (hasTermConnections() || type_ == odb::dbWireShapeType::FOLLOWPIN) {
    // if shape is connected to an instance or block pin allow it is valid
    // if shape is a followpin assume it will be connected
    return true;
  }

  for (const auto& via : vias_) {
    if (!via->isFailed()) {
      return true;
    }
  }

  return false;
}

bool Shape::hasInternalConnections() const
{
  if (hasITermConnections() || type_ == odb::dbWireShapeType::FOLLOWPIN) {
    // if shape is connected to an instance or block pin allow it is valid
    // if shape is a followpin assume it will be connected
    return true;
  }

  for (const auto& via : vias_) {
    if (!via->isFailed()) {
      return true;
    }
  }

  return false;
}

std::vector<odb::dbBox*> Shape::writeToDb(odb::dbSWire* swire,
                                          bool add_pins,
                                          bool make_rect_as_pin) const
{
  debugPrint(getLogger(),
             utl::PDN,
             "Shape",
             5,
             "Adding shape {} with pins {} and rect as pin {} / {} {} - {}",
             getReportText(),
             add_pins,
             make_rect_as_pin,
             is_locked_,
             hasITermConnections(),
             hasBTermConnections());

  if (!is_locked_ && !hasDBConnectivity()) {
    getLogger()->warn(
        utl::PDN, 200, "Removing floating shape: {}", getReportText());
    return {};
  }

  std::vector<odb::dbBox*> objs;

  objs.push_back(odb::dbSBox::create(swire,
                                     layer_,
                                     rect_.xMin(),
                                     rect_.yMin(),
                                     rect_.xMax(),
                                     rect_.yMax(),
                                     type_));

  if (add_pins) {
    if (make_rect_as_pin) {
      objs.push_back(addBPinToDb(rect_));
    }
    const odb::Rect block_area = getGridComponent()->getBlock()->getDieArea();
    for (const auto& bterm : bterm_connections_) {
      odb::Rect bterm_shape = bterm;
      // Adjust width of shape when bterm is on the edge of the die area
      if (bterm.xMin() == block_area.xMin()
          || bterm.xMax() == block_area.xMax()) {
        bterm_shape.set_ylo(rect_.yMin());
        bterm_shape.set_yhi(rect_.yMax());
      } else if (bterm.yMin() == block_area.yMin()
                 || bterm.yMax() == block_area.yMax()) {
        bterm_shape.set_xlo(rect_.xMin());
        bterm_shape.set_xhi(rect_.xMax());
      }
      objs.push_back(addBPinToDb(bterm_shape));
    }
  }
  return objs;
}

odb::dbBox* Shape::addBPinToDb(const odb::Rect& rect) const
{
  // find existing bterm, else make it
  odb::dbBTerm* bterm = nullptr;
  if (net_->getBTermCount() == 0) {
    bterm = getGridComponent()->getBlock()->findBTerm(net_->getConstName());
    if (bterm != nullptr) {
      odb::dbNet* net = bterm->getNet();
      if (net != nullptr && net != net_) {
        getLogger()->error(utl::PDN,
                           214,
                           "BTerm {} already exists for a different net ({})",
                           net_->getName(),
                           net->getName());
      } else {
        bterm->connect(net_);
      }
    } else {
      bterm = odb::dbBTerm::create(net_, net_->getConstName());
    }
    bterm->setIoType(odb::dbIoType::INOUT);
  } else {
    bterm = net_->get1stBTerm();
  }
  bterm->setSigType(net_->getSigType());
  bterm->setSpecial();

  odb::dbBox* box = nullptr;

  odb::dbBPin* pin = nullptr;
  auto pins = bterm->getBPins();
  for (auto* bpin : pins) {
    for (auto* box : bpin->getBoxes()) {
      if (box->getTechLayer() != layer_) {
        continue;
      }
      if (box->getBox() == rect) {
        pin = bpin;
      }
    }
  }

  if (pin == nullptr) {
    if (pins.empty()) {
      pin = odb::dbBPin::create(bterm);
    } else {
      pin = *pins.begin();
    }
    box = odb::dbBox::create(
        pin, layer_, rect.xMin(), rect.yMin(), rect.xMax(), rect.yMax());
  }

  pin->setPlacementStatus(odb::dbPlacementStatus::FIRM);

  return box;
}

void Shape::populateMapFromDb(odb::dbNet* net, ShapeVectorMap& map)
{
  // collect fixed bterms
  for (auto* bterm : net->getBTerms()) {
    for (auto* bpin : bterm->getBPins()) {
      if (!bpin->getPlacementStatus().isFixed()) {
        continue;
      }
      for (auto* box : bpin->getBoxes()) {
        auto* layer = box->getTechLayer();
        if (layer == nullptr) {
          continue;
        }
        if (layer->getRoutingLevel() == 0) {
          continue;
        }

        odb::Rect rect = box->getBox();

        ShapePtr shape = std::make_shared<Shape>(
            layer, net, rect, odb::dbWireShapeType::NONE);
        shape->setShapeType(Shape::FIXED);
        shape->generateObstruction();
        map[layer].push_back(std::move(shape));
      }
    }
  }

  // collect existing routing
  for (auto* swire : net->getSWires()) {
    for (auto* box : swire->getWires()) {
      if (box->isVia()) {
        odb::dbSet<odb::dbBox> via_boxes;
        const odb::dbTransform transform(box->getViaXY());
        if (auto* techvia = box->getTechVia()) {
          via_boxes = techvia->getBoxes();
        } else if (auto* blockvia = box->getBlockVia()) {
          via_boxes = blockvia->getBoxes();
        }
        for (auto* via_box : via_boxes) {
          odb::dbTechLayer* layer = via_box->getTechLayer();
          if (layer == nullptr) {
            continue;
          }
          odb::Rect rect = via_box->getBox();
          transform.apply(rect);
          ShapePtr shape = std::make_shared<Shape>(
              layer, net, rect, box->getWireShapeType());
          shape->setShapeType(Shape::FIXED);
          shape->generateObstruction();
          map[layer].push_back(std::move(shape));
        }
      } else {
        odb::dbTechLayer* layer = box->getTechLayer();

        odb::Rect rect = box->getBox();

        ShapePtr shape = std::make_shared<Shape>(
            layer, net, rect, box->getWireShapeType());
        shape->setShapeType(Shape::FIXED);
        if (box->getDirection() == odb::dbSBox::OCTILINEAR) {
          // cannot connect this this safely so make it an obstruction
          shape->setNet(nullptr);
          shape->setShapeType(Shape::OBS);
        }
        shape->generateObstruction();
        map[layer].push_back(std::move(shape));
      }
    }
  }
}

void Shape::generateObstruction()
{
  const int width = getWidth();
  const int length = getLength();
  const TechLayer layer(layer_);

  // first apply the spacing rules
  const int dbspacing = layer.getSpacing(width, length);
  odb::Rect spacingdb_rect;
  rect_.bloat(dbspacing, spacingdb_rect);

  // apply spacing table rules
  odb::Rect spacing_table_rect = rect_;
  const bool is_wrong_way = isWrongWay();
  for (auto* spacing_rule : layer_->getTechLayerSpacingTablePrlRules()) {
    if (spacing_rule->isWrongDirection() && !is_wrong_way) {
      continue;
    }

    const int spacing = spacing_rule->getSpacing(width, length);

    odb::Rect spacing_rule_rect = rect_;
    rect_.bloat(spacing, spacing_rule_rect);
    spacing_table_rect.merge(spacing_rule_rect);
  }

  // apply eol rules
  const bool is_horizontal = isHorizontal();
  odb::Rect eol_rect = rect_;
  for (auto* eol_rule : layer_->getTechLayerSpacingEolRules()) {
    if (width > eol_rule->getEolWidth()) {
      continue;
    }

    const int spacing = eol_rule->getEolSpace();

    odb::Rect eol_rule_rect = rect_;
    if (is_horizontal) {
      eol_rule_rect.set_xlo(eol_rule_rect.xMin() - spacing);
      eol_rule_rect.set_xhi(eol_rule_rect.xMax() + spacing);
    } else {
      eol_rule_rect.set_ylo(eol_rule_rect.yMin() - spacing);
      eol_rule_rect.set_yhi(eol_rule_rect.yMax() + spacing);
    }
    eol_rect.merge(eol_rule_rect);
  }

  // merge all to get most restrictive obstruction box
  obs_.mergeInit();
  obs_.merge(spacingdb_rect);
  obs_.merge(spacing_table_rect);
  obs_.merge(eol_rect);
}

std::string Shape::getDisplayText() const
{
  const std::string seperator = ":";
  std::string text;

  text += net_->getName() + seperator;
  text += layer_->getName() + seperator;
  if (grid_component_ != nullptr) {
    text += GridComponent::typeToString(grid_component_->type()) + seperator;
    text += grid_component_->getGrid()->getName();
  } else {
    text += "none";
  }

  return text;
}

bool Shape::isRemovable(bool assume_bterm) const
{
  if (!isModifiable()) {
    return false;
  }

  int min_conns = 2;
  if (assume_bterm) {
    min_conns = 1;
  }

  if (getNumberOfConnections() < min_conns) {
    // floating shape with one or zero connections
    return true;
  }

  return false;
}

bool Shape::isModifiable() const
{
  if (is_locked_) {
    return false;
  }
  return shape_type_ == SHAPE;
}

std::string Shape::getReportText() const
{
  std::string text = fmt::format(
      "{} on {}",
      getRectText(rect_, layer_->getTech()->getDbUnitsPerMicron()),
      layer_->getName());

  if (net_ != nullptr) {
    text = net_->getName() + " " + text;
  }
  return text;
}

std::string Shape::getRectText(const odb::Rect& rect, double dbu_to_micron)
{
  return fmt::format("({:.4f}, {:.4f}) - ({:.4f}, {:.4f})",
                     rect.xMin() / dbu_to_micron,
                     rect.yMin() / dbu_to_micron,
                     rect.xMax() / dbu_to_micron,
                     rect.yMax() / dbu_to_micron);
}

std::unique_ptr<Shape> Shape::extendTo(
    const odb::Rect& rect,
    const ObstructionTree& obstructions,
    Shape* orig_shape,
    const std::function<bool(const ShapePtr&)>& obs_filter) const
{
  std::unique_ptr<Shape> new_shape = copy();

  if (isHorizontal()) {
    new_shape->rect_.set_xlo(std::min(rect_.xMin(), rect.xMin()));
    new_shape->rect_.set_xhi(std::max(rect_.xMax(), rect.xMax()));
  } else if (isVertical()) {
    new_shape->rect_.set_ylo(std::min(rect_.yMin(), rect.yMin()));
    new_shape->rect_.set_yhi(std::max(rect_.yMax(), rect.yMax()));
  } else {
    return nullptr;
  }

  if (rect_ == new_shape->rect_) {
    // shape did not change
    return nullptr;
  }

  if (obstructions.qbegin(bgi::intersects(new_shape->getRect())
                          && bgi::satisfies([&orig_shape](const auto& other) {
                               // ignore violations that results from itself
                               return other.get() != orig_shape;
                             })
                          && bgi::satisfies(obs_filter))
      != obstructions.qend()) {
    // extension not possible
    return nullptr;
  }

  return new_shape;
}

Shape::ShapeTreeMap Shape::convertVectorToTree(ShapeVectorMap& vec)
{
  ShapeTreeMap trees;

  for (auto& [layer, vals] : vec) {
    trees[layer] = ShapeTree(vals.begin(), vals.end());
  }

  ShapeVectorMap empty;
  vec.swap(empty);

  return trees;
}

Shape::ObstructionTreeMap Shape::convertVectorToObstructionTree(
    ShapeVectorMap& vec)
{
  ObstructionTreeMap trees;

  for (auto& [layer, vals] : vec) {
    trees[layer] = ObstructionTree(vals.begin(), vals.end());
  }

  ShapeVectorMap empty;
  vec.swap(empty);

  return trees;
}

odb::dbTechLayerDir Shape::getLayerDirection() const
{
  return layer_->getDirection();
}

/////////

FollowPinShape::FollowPinShape(odb::dbTechLayer* layer,
                               odb::dbNet* net,
                               const odb::Rect& rect)
    : Shape(layer, net, rect, odb::dbWireShapeType::FOLLOWPIN)
{
}

std::unique_ptr<Shape> FollowPinShape::copy() const
{
  auto shape
      = std::make_unique<FollowPinShape>(getLayer(), getNet(), getRect());
  shape->generateObstruction();
  shape->rows_ = rows_;
  return shape;
}

void FollowPinShape::merge(Shape* shape)
{
  Shape::merge(shape);

  FollowPinShape* other = dynamic_cast<FollowPinShape*>(shape);
  if (other == nullptr) {
    return;
  }

  rows_.insert(other->rows_.begin(), other->rows_.end());
}

void FollowPinShape::updateTermConnections()
{
  Shape::updateTermConnections();

  // remove rows that no longer overlap with shape

  const odb::Rect& rect = getRect();
  std::set<odb::dbRow*> remove_rows;
  for (auto* row : rows_) {
    odb::Rect row_rect = row->getBBox();
    if (!rect.intersects(row_rect)) {
      remove_rows.insert(row);
    }
  }
  for (auto* row : remove_rows) {
    rows_.erase(row);
  }
}

odb::Rect FollowPinShape::getMinimumRect() const
{
  odb::Rect min_shape = Shape::getMinimumRect();

  const odb::Rect& rect = getRect();
  // copy width back
  const bool is_horizontal = isHorizontal();
  if (is_horizontal) {
    min_shape.set_ylo(rect.yMin());
    min_shape.set_yhi(rect.yMax());
  } else {
    min_shape.set_xlo(rect.xMin());
    min_shape.set_xhi(rect.xMax());
  }

  // merge with rows to ensure proper overlap
  for (auto* row : rows_) {
    odb::Rect row_rect = row->getBBox();
    if (is_horizontal) {
      min_shape.set_xlo(std::min(min_shape.xMin(), row_rect.xMin()));
      min_shape.set_xhi(std::max(min_shape.xMax(), row_rect.xMax()));
    } else {
      min_shape.set_ylo(std::min(min_shape.yMin(), row_rect.yMin()));
      min_shape.set_yhi(std::max(min_shape.yMax(), row_rect.yMax()));
    }
  }

  return min_shape;
}

bool FollowPinShape::cut(
    const ObstructionTree& obstructions,
    const Grid* ignore_grid,
    std::vector<std::unique_ptr<Shape>>& replacements) const
{
  return Shape::cut(
      obstructions, replacements, [](const ShapePtr& other) -> bool {
        // followpins can ignore grid level obstructions
        // grid level obstructions represent the other grids defined
        // followpins should only get cut from real obstructions and
        // not estimated obstructions
        return other->shapeType() != GRID_OBS;
      });
}

odb::dbTechLayerDir FollowPinShape::getLayerDirection() const
{
  if (isSquare()) {
    return getLayer()->getDirection();
  }

  if (isHorizontal()) {
    return odb::dbTechLayerDir::HORIZONTAL;
  }

  return odb::dbTechLayerDir::VERTICAL;
}

/////////////////////////////////////

GridObsShape::GridObsShape(odb::dbTechLayer* layer,
                           const odb::Rect& rect,
                           const Grid* grid)
    : Shape(layer, rect, Shape::GRID_OBS), grid_(grid)
{
  setObstruction(rect);
}

}  // namespace pdn
