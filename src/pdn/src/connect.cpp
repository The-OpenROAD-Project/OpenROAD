//////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2022, The Regents of the University of California
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

#include "connect.h"

#include <cmath>
#include <regex>

#include "grid.h"
#include "odb/db.h"
#include "odb/dbTransform.h"
#include "techlayer.h"
#include "utl/Logger.h"

namespace pdn {

Connect::Connect(Grid* grid, odb::dbTechLayer* layer0, odb::dbTechLayer* layer1)
    : grid_(grid), layer0_(layer0), layer1_(layer1)
{
  if (layer0_->getRoutingLevel() > layer1_->getRoutingLevel()) {
    // ensure layer0 is below layer1
    std::swap(layer0_, layer1_);
  }

  auto* tech = layer0_->getTech();
  for (int l = layer0_->getNumber() + 1; l < layer1_->getNumber(); l++) {
    auto* layer = tech->findLayer(l);

    if (layer->getLef58Type() != odb::dbTechLayer::NONE) {
      // ignore non-cut and routing layers
      continue;
    }

    intermediate_layers_.push_back(layer);

    if (layer->getRoutingLevel() != 0) {
      intermediate_routing_layers_.push_back(layer);
    }
  }

  populateDBVias();
}

void Connect::populateDBVias()
{
  populateGenerateRules();
  populateTechVias();
}

void Connect::addFixedVia(odb::dbTechViaGenerateRule* via)
{
  fixed_generate_vias_.push_back(via);
  populateDBVias();
}

void Connect::addFixedVia(odb::dbTechVia* via)
{
  fixed_tech_vias_.push_back(via);
  populateDBVias();
}

void Connect::setCutPitch(int x, int y)
{
  cut_pitch_x_ = x;
  cut_pitch_y_ = y;
}

void Connect::setOnGrid(const std::vector<odb::dbTechLayer*>& layers)
{
  ongrid_.insert(layers.begin(), layers.end());
}

void Connect::setSplitCuts(const std::map<odb::dbTechLayer*, int>& splits)
{
  split_cuts_ = splits;
  // remove top and bottom layers of the stack
  split_cuts_.erase(layer0_);
  split_cuts_.erase(layer1_);
}

int Connect::getSplitCutPitch(odb::dbTechLayer* layer) const
{
  auto layer_itr = split_cuts_.find(layer);
  if (layer_itr == split_cuts_.end()) {
    return 0;
  }

  return layer_itr->second;
}

bool Connect::appliesToVia(const ViaPtr& via) const
{
  return via->getLowerLayer() == layer0_ && via->getUpperLayer() == layer1_;
}

bool Connect::isSingleLayerVia() const
{
  return intermediate_routing_layers_.empty();
}

bool Connect::isComplexStackedVia(const odb::Rect& lower,
                                  const odb::Rect& upper) const
{
  if (isSingleLayerVia()) {
    return false;
  }

  const odb::Rect intersection = lower.intersect(upper);
  const int min_width = intersection.minDXDY();

  for (auto* layer : intermediate_routing_layers_) {
    if (layer->getMinWidth() > min_width) {
      return true;
    }
  }

  return false;
}

std::vector<Connect::ViaLayerRects> Connect::generateViaRects(
    const odb::Rect& lower,
    const odb::Rect& upper) const
{
  const odb::Rect intersection = lower.intersect(upper);

  std::vector<ViaLayerRects> stack;
  stack.push_back({lower});
  for (int i = 0; i < intermediate_routing_layers_.size(); i++) {
    stack.push_back({intersection});
  }
  stack.push_back({upper});
  return stack;
}

std::vector<Connect::ViaLayerRects> Connect::generateComplexStackedViaRects(
    const odb::Rect& lower,
    const odb::Rect& upper) const
{
  auto adjust_rect =
      [](int min_width, bool is_x, odb::dbTech* tech, odb::Rect& intersection) {
        const int width = is_x ? intersection.dx() : intersection.dy();
        if (width < min_width) {
          // fix intersection to meet min width
          const int min_add = min_width - width;
          const int half_min_add0 = min_add / 2;
          const int half_min_add1 = min_add - half_min_add0;

          int new_min = -half_min_add0;
          int new_max = half_min_add1;
          if (is_x) {
            new_min += intersection.xMin();
            new_max += intersection.xMax();
          } else {
            new_min += intersection.yMin();
            new_max += intersection.yMax();
          }

          new_min = TechLayer::snapToManufacturingGrid(tech, new_min, false);
          new_max = TechLayer::snapToManufacturingGrid(tech, new_max, true);

          if (is_x) {
            intersection.set_xlo(new_min);
            intersection.set_xhi(new_max);
          } else {
            intersection.set_ylo(new_min);
            intersection.set_yhi(new_max);
          }
        }
      };

  std::vector<ViaLayerRects> stack;
  stack.push_back({lower});
  const odb::Rect intersection = lower.intersect(upper);
  for (auto* layer : intermediate_routing_layers_) {
    auto* tech = layer->getTech();

    odb::Rect level_intersection = intersection;

    const int min_width = getMinWidth(layer);
    adjust_rect(min_width, true, tech, level_intersection);
    adjust_rect(min_width, false, tech, level_intersection);

    stack.push_back({level_intersection});
  }
  stack.push_back({upper});

  return stack;
}

void Connect::generateMinEnclosureViaRects(
    std::vector<ViaLayerRects>& rects) const
{
  // fill possible rects with min width rects to ensure min enclosure is checked
  for (int i = 0; i < intermediate_routing_layers_.size(); i++) {
    auto* layer = intermediate_routing_layers_[i];
    auto& layer_rects = rects[i + 1];

    const bool is_horizontal
        = layer->getDirection() == odb::dbTechLayerDir::HORIZONTAL;
    const int min_width = layer->getWidth();

    ViaLayerRects new_rects;
    for (const auto& rect : layer_rects) {
      odb::Rect new_rect = rect;
      const odb::Point new_rect_center(new_rect.xMin() + new_rect.dx() / 2,
                                       new_rect.yMin() + new_rect.dy() / 2);
      if (is_horizontal) {
        new_rect.set_ylo(new_rect_center.y() - min_width / 2);
        new_rect.set_yhi(new_rect.yMin() + min_width);
      } else {
        new_rect.set_xlo(new_rect_center.x() - min_width / 2);
        new_rect.set_xhi(new_rect.xMin() + min_width);
      }

      new_rects.insert(new_rect);
    }

    layer_rects.insert(new_rects.begin(), new_rects.end());
  }
}

int Connect::getMinWidth(odb::dbTechLayer* layer) const
{
  const int min_width = layer->getMinWidth();

  auto* below = layer;
  while (below->getType() != odb::dbTechLayerType::CUT) {
    below = below->getLowerLayer();
    if (below == nullptr) {
      break;
    }
  }

  int below_max_enc = 0;
  if (below != nullptr) {
    below_max_enc = getMaxEnclosureFromCutLayer(below, min_width);
  }

  auto* above = layer;
  while (above->getType() != odb::dbTechLayerType::CUT) {
    above = above->getUpperLayer();
    if (above == nullptr) {
      break;
    }
  }

  int above_max_enc = 0;
  if (above != nullptr) {
    above_max_enc = getMaxEnclosureFromCutLayer(above, min_width);
  }

  // return the min width + the worst case enclosure to enclosure to ensure a
  // via will fit
  return min_width + 2 * std::max(below_max_enc, above_max_enc);
}

int Connect::getMaxEnclosureFromCutLayer(odb::dbTechLayer* layer,
                                         int min_width) const
{
  int max_enclosure = 0;
  for (auto* rule : layer->getTechLayerCutEnclosureRules()) {
    max_enclosure = std::max(max_enclosure, rule->getFirstOverhang());
    max_enclosure = std::max(max_enclosure, rule->getSecondOverhang());
  }

  for (auto* rule : generate_via_rules_) {
    bool use = false;
    int rule_max_enclosure = 0;
    for (uint i = 0; i < rule->getViaLayerRuleCount(); i++) {
      auto layer_rule = rule->getViaLayerRule(i);
      use |= layer_rule->getLayer() == layer;

      if (layer_rule->hasEnclosure()) {
        int enc0, enc1;
        layer_rule->getEnclosure(enc0, enc1);
        rule_max_enclosure = std::max(rule_max_enclosure, std::max(enc0, enc1));
      }
    }

    if (use) {
      max_enclosure = std::max(max_enclosure, rule_max_enclosure);
    }
  }

  for (auto* rule : tech_vias_) {
    bool use = false;
    int max_size = 0;
    for (auto* box : rule->getBoxes()) {
      use |= box->getTechLayer() == layer;

      odb::Rect rect = box->getBox();
      max_size = std::max(max_size, static_cast<int>(rect.maxDXDY()));
    }

    if (use) {
      max_enclosure = std::max(max_enclosure, (max_size - min_width) / 2);
    }
  }

  return max_enclosure;
}

bool Connect::containsIntermediateLayer(odb::dbTechLayer* layer) const
{
  return std::find(
             intermediate_layers_.begin(), intermediate_layers_.end(), layer)
         != intermediate_layers_.end();
}

std::vector<odb::dbTechLayer*> Connect::getAllLayers() const
{
  std::vector<odb::dbTechLayer*> layers;
  layers.push_back(layer0_);
  layers.insert(
      layers.end(), intermediate_layers_.begin(), intermediate_layers_.end());
  layers.push_back(layer1_);
  return layers;
}

std::vector<odb::dbTechLayer*> Connect::getAllRoutingLayers() const
{
  std::vector<odb::dbTechLayer*> layers;
  layers.push_back(layer0_);
  layers.insert(layers.end(),
                intermediate_routing_layers_.begin(),
                intermediate_routing_layers_.end());
  layers.push_back(layer1_);
  return layers;
}

bool Connect::overlaps(const Connect* other) const
{
  const int this_lower = layer0_->getNumber();
  const int this_upper = layer1_->getNumber();
  const int other_lower = other->getLowerLayer()->getNumber();
  const int other_upper = other->getUpperLayer()->getNumber();
  if (startsBelow(other)) {
    if (this_upper < other_lower) {
      // via is below other
      return false;
    }
  } else {
    if (other_upper < this_lower) {
      // other is below via
      return false;
    }
  }

  return true;
}

bool Connect::startsBelow(const Connect* other) const
{
  return layer0_->getNumber() < other->getLowerLayer()->getNumber();
}

void Connect::report() const
{
  auto* logger = grid_->getLogger();
  auto* block = grid_->getBlock();
  const double dbu_per_micron = block->getDbUnitsPerMicron();

  logger->report(
      "  Connect layers {} -> {}", layer0_->getName(), layer1_->getName());
  if (!fixed_generate_vias_.empty() || !fixed_tech_vias_.empty()) {
    std::string vias;
    for (auto* via : fixed_generate_vias_) {
      vias += via->getName() + " ";
    }
    for (auto* via : fixed_tech_vias_) {
      vias += via->getName() + " ";
    }
    logger->report("    Fixed vias: {}", vias);
  }
  if (cut_pitch_x_ != 0) {
    logger->report("    Cut pitch X-direction: {:.4f}",
                   cut_pitch_x_ / dbu_per_micron);
  }
  if (cut_pitch_y_ != 0) {
    logger->report("    Cut pitch Y-direction: {:.4f}",
                   cut_pitch_y_ / dbu_per_micron);
  }
  if (max_rows_ != 0) {
    logger->report("    Maximum rows: {}", max_rows_);
  }
  if (max_columns_ != 0) {
    logger->report("    Maximum columns: {}", max_columns_);
  }
  if (!ongrid_.empty()) {
    std::string layers;
    for (auto* l : ongrid_) {
      layers += l->getName() + " ";
    }
    logger->report("    Ongrid layers: {}", layers);
  }
  if (!split_cuts_.empty()) {
    logger->report("    Split cuts:");
    for (const auto& [layer, pitch] : split_cuts_) {
      logger->report("      Layer: {} with pitch {:.4f}",
                     layer->getName(),
                     pitch / dbu_per_micron);
    }
  }
}

void Connect::makeVia(odb::dbSWire* wire,
                      const ShapePtr& lower,
                      const ShapePtr& upper,
                      const odb::dbWireShapeType& type,
                      DbVia::ViaLayerShape& shapes)
{
  const odb::Rect& lower_rect = lower->getRect();
  const odb::Rect& upper_rect = upper->getRect();
  const odb::Rect intersection = lower_rect.intersect(upper_rect);

  auto* tech = layer0_->getTech();
  const int x = std::round(0.5 * (intersection.xMin() + intersection.xMax()));
  const int y = std::round(0.5 * (intersection.yMin() + intersection.yMax()));

  // check if off grid and don't add one if it is
  if (!TechLayer::checkIfManufacturingGrid(tech, x)
      || !TechLayer::checkIfManufacturingGrid(tech, y)) {
    DbGenerateDummyVia dummy_via(this, intersection, layer0_, layer1_, true);
    dummy_via.generate(wire->getBlock(), wire, type, 0, 0, grid_->getLogger());
    return;
  }

  const ViaIndex via_index
      = std::make_pair(intersection.dx(), intersection.dy());
  auto& via = vias_[via_index];

  bool skip_caching = false;
  // make the via stack if one is not available for the given size
  if (via == nullptr) {
    std::vector<ViaLayerRects> stack_rects;
    if (isComplexStackedVia(lower_rect, upper_rect)) {
      debugPrint(grid_->getLogger(),
                 utl::PDN,
                 "Via",
                 2,
                 "Tapered via required between {} and {} at ({:.4f}, {:.4f}).",
                 getLowerLayer()->getName(),
                 getUpperLayer()->getName(),
                 x / static_cast<double>(tech->getLefUnits()),
                 y / static_cast<double>(tech->getLefUnits()));

      stack_rects = generateComplexStackedViaRects(lower_rect, upper_rect);
    } else {
      stack_rects = generateViaRects(lower_rect, upper_rect);
    }
    generateMinEnclosureViaRects(stack_rects);

    std::vector<DbVia*> stack;
    std::vector<odb::dbTechLayer*> layers = getAllRoutingLayers();

    for (int i = 1; i < layers.size(); i++) {
      const auto& via_lower_rects = stack_rects[i - 1];
      const auto& via_upper_rects = stack_rects[i];
      auto* l0 = layers[i - 1];
      auto* l1 = layers[i];

      ViaGenerator::Constraint lower_constraint{false, false, true};
      if (lower->getLayer() == l0) {
        if (!lower->isModifiable() || lower->hasTermConnections()) {
          // lower is not modifiable to all sides must fit
          skip_caching = true;
          lower_constraint.must_fit_x = true;
          lower_constraint.must_fit_y = true;
          lower_constraint.intersection_only = false;
        } else {
          lower_constraint.must_fit_x = !lower->isHorizontal();
          lower_constraint.must_fit_y = !lower->isVertical();
        }
      }
      ViaGenerator::Constraint upper_constraint{false, false, true};
      if (upper->getLayer() == l1) {
        if (!upper->isModifiable() || upper->hasTermConnections()) {
          // upper is not modifiable to all sides must fit
          skip_caching = true;
          upper_constraint.must_fit_x = true;
          upper_constraint.must_fit_y = true;
          upper_constraint.intersection_only = false;
        } else {
          upper_constraint.must_fit_x = !upper->isHorizontal();
          upper_constraint.must_fit_y = !upper->isVertical();
        }
      }

      auto* new_via = makeSingleLayerVia(wire->getBlock(),
                                         l0,
                                         via_lower_rects,
                                         lower_constraint,
                                         l1,
                                         via_upper_rects,
                                         upper_constraint);
      if (new_via == nullptr) {
        // no via made, so build dummy via for warning
        for (auto* stack_via : stack) {
          // make sure stack is cleared
          delete stack_via;
        }
        stack.clear();
        odb::dbTransform xfm({-x, -y});
        odb::Rect area = intersection;
        xfm.apply(area);
        stack.push_back(
            new DbGenerateDummyVia(this, area, layer0_, layer1_, false));
        break;
      }
      stack.push_back(new_via);
    }

    via = std::make_unique<DbGenerateStackedVia>(
        stack, layer0_, wire->getBlock(), ongrid_);
  }

  shapes
      = via->generate(wire->getBlock(), wire, type, x, y, grid_->getLogger());

  if (skip_caching) {
    via = nullptr;
  }

  if (shapes.bottom.empty() && shapes.top.empty()) {
    addFailedVia(failedViaReason::RECHECK, intersection, wire->getNet());
  }
}

DbVia* Connect::generateDbVia(
    const std::vector<std::shared_ptr<ViaGenerator>>& generators,
    odb::dbBlock* block) const
{
  std::vector<std::shared_ptr<ViaGenerator>> vias;
  for (const auto& via : generators) {
    if (hasCutPitch()) {
      via->setCutPitchX(cut_pitch_x_);
      via->setCutPitchY(cut_pitch_y_);
    }
    via->setMaxRows(max_rows_);
    via->setMaxColumns(max_columns_);

    debugPrint(grid_->getLogger(),
               utl::PDN,
               "Via",
               1,
               "Cut class {} : {} - {}",
               via->getName(),
               via->getCutLayer()->getName(),
               via->hasCutClass() ? via->getCutClass()->getName() : "none");

    const int lower_split = getSplitCut(via->getBottomLayer());
    const int upper_split = getSplitCut(via->getTopLayer());
    if (lower_split != 0 || upper_split != 0) {
      const int pitch = std::max(lower_split, upper_split);
      via->setSplitCutArray(lower_split != 0, upper_split != 0);
      via->setCutPitchX(pitch);
      via->setCutPitchY(pitch);
    }

    const bool lower_is_internal = via->getBottomLayer() != layer0_;
    const bool upper_is_internal = via->getTopLayer() != layer1_;
    if (!via->build(lower_is_internal, upper_is_internal)) {
      debugPrint(grid_->getLogger(),
                 utl::PDN,
                 "Via",
                 2,
                 "{} was not buildable.",
                 via->getName());
      continue;
    }
    debugPrint(grid_->getLogger(),
               utl::PDN,
               "Via",
               2,
               "{} was buildable.",
               via->getName());

    vias.push_back(via);
  }

  debugPrint(grid_->getLogger(),
             utl::PDN,
             "Via",
             3,
             "Vias possible: {} from {} generators",
             vias.size(),
             generators.size());

  if (vias.empty()) {
    return nullptr;
  }

  std::stable_sort(vias.begin(),
                   vias.end(),
                   [](const std::shared_ptr<ViaGenerator>& lhs,
                      const std::shared_ptr<ViaGenerator>& rhs) {
                     return lhs->isPreferredOver(rhs.get());
                   });

  std::shared_ptr<ViaGenerator> best_rule = *vias.begin();
  DbVia* built_via = best_rule->generate(block);
  built_via->setGenerator(best_rule);

  return built_via;
}

DbVia* Connect::makeSingleLayerVia(
    odb::dbBlock* block,
    odb::dbTechLayer* lower,
    const std::set<odb::Rect>& lower_rects,
    const ViaGenerator::Constraint& lower_constraint,
    odb::dbTechLayer* upper,
    const std::set<odb::Rect>& upper_rects,
    const ViaGenerator::Constraint& upper_constraint) const
{
  debugPrint(grid_->getLogger(),
             utl::PDN,
             "Via",
             1,
             "Making via between {} and {}",
             lower->getName(),
             upper->getName());
  // look for generate vias
  std::vector<std::shared_ptr<ViaGenerator>> generate_vias;
  for (const auto& lower_rect : lower_rects) {
    for (const auto& upper_rect : upper_rects) {
      for (odb::dbTechViaGenerateRule* db_via : generate_via_rules_) {
        std::shared_ptr<GenerateViaGenerator> rule
            = std::make_shared<GenerateViaGenerator>(grid_->getLogger(),
                                                     db_via,
                                                     lower_rect,
                                                     lower_constraint,
                                                     upper_rect,
                                                     upper_constraint);

        if (!rule->isSetupValid(lower, upper)) {
          debugPrint(grid_->getLogger(),
                     utl::PDN,
                     "Via",
                     3,
                     "Generate via rule deemed not valid: {}",
                     rule->getName());
          continue;
        }

        generate_vias.push_back(std::move(rule));
      }
    }
  }
  debugPrint(grid_->getLogger(),
             utl::PDN,
             "Via",
             2,
             "Generate via rules available: {} from {}",
             generate_vias.size(),
             generate_via_rules_.size());

  DbVia* generate_via = generateDbVia(generate_vias, block);

  if (generate_via != nullptr) {
    return generate_via;
  }

  // fallback to tech vias if generate is not possible
  // look for generate vias
  std::vector<std::shared_ptr<ViaGenerator>> tech_vias;
  for (const auto& lower_rect : lower_rects) {
    for (const auto& upper_rect : upper_rects) {
      for (odb::dbTechVia* db_via : tech_vias_) {
        std::shared_ptr<TechViaGenerator> rule
            = std::make_shared<TechViaGenerator>(grid_->getLogger(),
                                                 db_via,
                                                 lower_rect,
                                                 lower_constraint,
                                                 upper_rect,
                                                 upper_constraint);

        if (!rule->isSetupValid(lower, upper)) {
          debugPrint(grid_->getLogger(),
                     utl::PDN,
                     "Via",
                     3,
                     "Tech via rule deemed not valid: {}",
                     rule->getName());
          continue;
        }

        tech_vias.push_back(std::move(rule));
      }
    }
  }
  debugPrint(grid_->getLogger(),
             utl::PDN,
             "Via",
             2,
             "Tech vias available: {} from {}",
             tech_vias.size(),
             tech_vias_.size());

  return generateDbVia(tech_vias, block);
}

void Connect::populateGenerateRules()
{
  generate_via_rules_.clear();

  odb::dbTech* tech = layer0_->getTech();

  const auto layers = getAllRoutingLayers();
  for (int l = 0; l < layers.size() - 1; l++) {
    odb::dbTechLayer* lower = layers[l];
    odb::dbTechLayer* upper = layers[l + 1];
    bool use_fixed_via = false;
    for (auto* rule : fixed_generate_vias_) {
      if (generateRuleContains(rule, lower, upper)) {
        generate_via_rules_.push_back(rule);
        use_fixed_via = true;
      }
    }
    if (use_fixed_via) {
      continue;
    }
    for (odb::dbTechViaGenerateRule* db_via : tech->getViaGenerateRules()) {
      if (generateRuleContains(db_via, lower, upper)) {
        generate_via_rules_.push_back(db_via);
      }
    }
  }

  debugPrint(grid_->getLogger(),
             utl::PDN,
             "Via",
             2,
             "Generate via rules: {}",
             generate_via_rules_.size());
  for (auto* via : generate_via_rules_) {
    debugPrint(grid_->getLogger(),
               utl::PDN,
               "Via",
               3,
               "Generate rule: {}",
               via->getName());
  }
}

void Connect::populateTechVias()
{
  tech_vias_.clear();

  odb::dbTech* tech = layer0_->getTech();

  const auto layers = getAllRoutingLayers();
  for (int l = 0; l < layers.size() - 1; l++) {
    odb::dbTechLayer* lower = layers[l];
    odb::dbTechLayer* upper = layers[l + 1];
    bool use_fixed_via = false;
    for (auto* via : fixed_tech_vias_) {
      if (techViaContains(via, lower, upper)) {
        tech_vias_.push_back(via);
        use_fixed_via = true;
      }
    }
    if (use_fixed_via) {
      continue;
    }
    for (odb::dbTechVia* db_via : tech->getVias()) {
      if (techViaContains(db_via, lower, upper)) {
        tech_vias_.push_back(db_via);
      }
    }
  }

  debugPrint(grid_->getLogger(),
             utl::PDN,
             "Via",
             2,
             "Tech via rules: {}",
             tech_vias_.size());
  for (auto* via : tech_vias_) {
    debugPrint(
        grid_->getLogger(), utl::PDN, "Via", 3, "Tech via: {}", via->getName());
  }
}

bool Connect::generateRuleContains(odb::dbTechViaGenerateRule* rule,
                                   odb::dbTechLayer* lower,
                                   odb::dbTechLayer* upper) const
{
  const uint layer_count = rule->getViaLayerRuleCount();
  if (layer_count != 3) {
    return false;
  }
  std::set<odb::dbTechLayer*> rule_layers;
  for (uint l = 0; l < layer_count; l++) {
    rule_layers.insert(rule->getViaLayerRule(l)->getLayer());
  }
  if (rule_layers.find(lower) != rule_layers.end()
      && rule_layers.find(upper) != rule_layers.end()) {
    return true;
  }
  return false;
}

bool Connect::techViaContains(odb::dbTechVia* via,
                              odb::dbTechLayer* lower,
                              odb::dbTechLayer* upper) const
{
  return via->getBottomLayer() == lower && via->getTopLayer() == upper;
}

int Connect::getSplitCut(odb::dbTechLayer* layer) const
{
  auto split_itr = split_cuts_.find(layer);
  if (split_itr != split_cuts_.end()) {
    return split_itr->second;
  }
  return 0;
}

void Connect::clearShapes()
{
  vias_.clear();
  failed_vias_.clear();
}

void Connect::filterVias(const std::string& filter)
{
  clearShapes();

  std::regex filt(filter);

  generate_via_rules_.erase(
      std::remove_if(
          generate_via_rules_.begin(),
          generate_via_rules_.end(),
          [&](auto* rule) { return std::regex_search(rule->getName(), filt); }),
      generate_via_rules_.end());

  tech_vias_.erase(
      std::remove_if(
          tech_vias_.begin(),
          tech_vias_.end(),
          [&](auto* rule) { return std::regex_search(rule->getName(), filt); }),
      tech_vias_.end());
}

void Connect::printViaReport() const
{
  auto* logger = getGrid()->getLogger();

  // check if debug is enabled
  if (!logger->debugCheck(utl::PDN, "Write", 0)) {
    return;
  }

  ViaReport report;
  for (const auto& [via_index, via] : vias_) {
    if (via == nullptr) {
      continue;
    }
    for (const auto& [via_name, count] : via->getViaReport()) {
      report[via_name] += count;
    }
  }

  debugPrint(logger,
             utl::PDN,
             "Write",
             1,
             "Vias ({}) from {} -> {}: {} types",
             grid_->getLongName(),
             layer0_->getName(),
             layer1_->getName(),
             report.size());

  for (const auto& [via_name, count] : report) {
    debugPrint(logger, utl::PDN, "Write", 2, "Via \"{}\": {}", via_name, count);
  }
}

void Connect::addFailedVia(failedViaReason reason,
                           const odb::Rect& rect,
                           odb::dbNet* net)
{
  failed_vias_[reason].insert({net, rect});
}

void Connect::writeFailedVias(std::ofstream& file) const
{
  const double dbumicrons = layer0_->getTech()->getLefUnits();

  for (const auto& [reason, shapes] : failed_vias_) {
    std::string reason_str;
    switch (reason) {
      case failedViaReason::OBSTRUCTED:
        reason_str = "Obstructed";
        break;
      case failedViaReason::OVERLAPPING:
        reason_str = "Overlapping";
        break;
      case failedViaReason::BUILD:
        reason_str = "Build";
        break;
      case failedViaReason::RIPUP:
        reason_str = "Ripup";
        break;
      case failedViaReason::RECHECK:
        reason_str = "Recheck";
        break;
      case failedViaReason::OTHER:
        reason_str = "Other";
        break;
    }
    reason_str += " - " + grid_->getLongName();
    reason_str += " - " + layer0_->getName() + " -> " + layer1_->getName();

    for (const auto& [net, shape] : shapes) {
      file << "violation type: " << reason_str << std::endl;
      file << "\tsrcs: net:" << net->getName() << std::endl;
      file << "\tbbox = " << Shape::getRectText(shape, dbumicrons)
           << " on Layer -" << std::endl;
    }
  }
}

}  // namespace pdn
