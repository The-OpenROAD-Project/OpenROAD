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

#include "grid.h"
#include "odb/db.h"
#include "odb/dbTransform.h"
#include "utl/Logger.h"

namespace pdn {

Connect::Connect(odb::dbTechLayer* layer0, odb::dbTechLayer* layer1)
    : grid_(nullptr),
      layer0_(layer0),
      layer1_(layer1),
      fixed_generate_vias_({}),
      fixed_tech_vias_({}),
      cut_pitch_x_(0),
      cut_pitch_y_(0)
{
  if (layer0_->getRoutingLevel() > layer1_->getRoutingLevel()) {
    // ensure layer0 is below layer1
    std::swap(layer0_, layer1_);
  }

  auto* tech = layer0_->getTech();
  for (int r = layer0_->getRoutingLevel() + 1; r < layer1_->getRoutingLevel();
       r++) {
    intermediate_routing_layers_.push_back(tech->findRoutingLayer(r));
  }
  for (int r = layer0_->getNumber() + 1; r < layer1_->getNumber(); r++) {
    intermediate_layers_.push_back(tech->findLayer(r));
  }

  populateGenerateRules();
  populateTechVias();
}

void Connect::addFixedVia(odb::dbTechViaGenerateRule* via)
{
  fixed_generate_vias_.push_back(via);
}

void Connect::addFixedVia(odb::dbTechVia* via)
{
  fixed_tech_vias_.push_back(via);
}

void Connect::setCutPitch(int x, int y)
{
  cut_pitch_x_ = x;
  cut_pitch_y_ = y;
}

bool Connect::appliesToVia(const ViaPtr& via) const
{
  return via->getLowerLayer() == layer0_ && via->getUpperLayer() == layer1_;
}

bool Connect::isSingleLayerVia() const
{
  return intermediate_routing_layers_.empty();
}

bool Connect::isTaperedVia(const odb::Rect& lower, const odb::Rect& upper) const
{
  const odb::Rect intersection = lower.intersect(upper);
  const int min_width = intersection.minDXDY();

  for (auto* layer : intermediate_routing_layers_) {
    if (layer->getMinWidth() > min_width) {
      return true;
    }
  }

  return false;
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
  double dbu_per_micron = block->getDbUnitsPerMicron();

  logger->info(utl::PDN,
               70,
               "  Connect layers {} -> {}",
               layer0_->getName(),
               layer1_->getName());
  if (!fixed_generate_vias_.empty() || !fixed_tech_vias_.empty()) {
    std::string vias;
    for (auto* via : fixed_generate_vias_) {
      vias += via->getName() + " ";
    }
    for (auto* via : fixed_tech_vias_) {
      vias += via->getName() + " ";
    }
    logger->info(utl::PDN, 71, "    Fixed vias: {}", vias);
  }
  if (cut_pitch_x_ != 0) {
    logger->info(utl::PDN,
                 72,
                 "    Cut pitch X-direction: {:.4f}",
                 cut_pitch_x_ / dbu_per_micron);
  }
  if (cut_pitch_y_ != 0) {
    logger->info(utl::PDN,
                 73,
                 "    Cut pitch Y-direction: {:.4f}",
                 cut_pitch_y_ / dbu_per_micron);
  }
}

void Connect::makeVia(odb::dbSWire* wire,
                      const odb::Rect& lower_rect,
                      const odb::Rect& upper_rect,
                      odb::dbWireShapeType type)
{
  const odb::Rect intersection = lower_rect.intersect(upper_rect);

  const std::pair<int, int> via_index
      = std::make_pair(intersection.dx(), intersection.dy());
  auto& via_stack = vias_[via_index];

  // make the via stack if one is not available for the given size
  if (via_stack.empty()) {
    std::vector<odb::dbTechLayer*> layers;
    layers.push_back(layer0_);
    layers.insert(layers.end(),
                  intermediate_routing_layers_.begin(),
                  intermediate_routing_layers_.end());
    layers.push_back(layer1_);

    for (int i = 1; i < layers.size(); i++) {
      auto* l0 = layers[i - 1];
      auto* l1 = layers[i];

      auto* new_via = makeSingleLayerVia(
          wire->getBlock(), l0, lower_rect, l1, upper_rect);
      if (new_via == nullptr) {
        grid_->getLogger()->warn(
            utl::PDN,
            110,
            "No via found between {} and {} at {}",
            l0->getName(),
            l1->getName(),
            Shape::getRectText(intersection, l0->getTech()->getLefUnits()));
        return;
      } else {
        via_stack.push_back({std::unique_ptr<DbVia>(new_via), l0, l1});
      }
    }
  }

  const int x = 0.5 * (intersection.xMin() + intersection.xMax());
  const int y = 0.5 * (intersection.yMin() + intersection.yMax());

  DbVia* prev_via = nullptr;
  odb::Rect top_of_previous;
  for (const auto& via : via_stack) {
    auto shapes = via.via->generate(wire->getBlock(), wire, type, x, y);

    bool add_patch_metal = false;
    if (prev_via != nullptr) {
      if (prev_via->isArray() || via.via->isArray()) {
        // part of via stack with array parts
        add_patch_metal = true;
      } else if (!shapes.bottom.contains(top_of_previous)
                 && !top_of_previous.contains(shapes.bottom)) {
        // the interface between vias does align and therefore add patch to
        // prevent min-step violations.
        add_patch_metal = true;
      }
    }

    if (add_patch_metal) {
      // add patch metal on layers between the bottom and top of the via stack
      top_of_previous.merge(shapes.bottom);
      odb::dbSBox::create(wire,
                          via.bottom,
                          top_of_previous.xMin(),
                          top_of_previous.yMin(),
                          top_of_previous.xMax(),
                          top_of_previous.yMax(),
                          type);
    }

    prev_via = via.via.get();
    top_of_previous = shapes.top;
  }
}

DbVia* Connect::makeSingleLayerVia(odb::dbBlock* block,
                                   odb::dbTechLayer* lower,
                                   const odb::Rect& lower_rect,
                                   odb::dbTechLayer* upper,
                                   const odb::Rect& upper_rect) const
{
  // look for generate vias
  std::vector<std::unique_ptr<GenerateVia>> generate_vias;
  for (odb::dbTechViaGenerateRule* db_via : generate_via_rules_) {
    std::unique_ptr<GenerateVia> rule
        = std::make_unique<GenerateVia>(db_via, lower_rect, upper_rect);

    if (rule->getBottomLayer() != lower || rule->getTopLayer() != upper) {
      continue;
    }

    if (!rule->isBottomValidForWidth(lower_rect.minDXDY())
        || !rule->isTopValidForWidth(upper_rect.minDXDY())) {
      continue;
    }

    generate_vias.push_back(std::move(rule));
  }

  // attempt to build all possible vias and pick the one with the largest cut area
  GenerateVia* best_rule = nullptr;
  int best_area = 0;
  for (const auto& via : generate_vias) {
    if (hasCutPitch()) {
      via->setCutPitchX(cut_pitch_x_);
      via->setCutPitchY(cut_pitch_y_);
    }

    const bool lower_is_internal = lower != layer0_;
    const bool upper_is_internal = upper != layer1_;
    via->determineRowsAndColumns(lower_is_internal, upper_is_internal);

    debugPrint(grid_->getLogger(), utl::PDN, "Via", 1, "Cut class {} - {}", via->getCutLayer()->getName(), via->getCutClass() == nullptr ? "none" : via->getCutClass()->getName());

    if (!via->checkMinCuts()) {
      // violates min cut rules
      continue;
    }

    debugPrint(grid_->getLogger(), utl::PDN, "Via", 1, "Cut class {} - {}", via->getCutLayer()->getName(), via->getCutClass() == nullptr ? "none" : via->getCutClass()->getName());

    if (!via->checkMinEnclosure()) {
      // violates min enclosure rules
      continue;
    }

    debugPrint(grid_->getLogger(), utl::PDN, "Via", 1, "Cut class {} - {}", via->getCutLayer()->getName(), via->getCutClass() == nullptr ? "none" : via->getCutClass()->getName());

    const int area = via->getCutArea();

    if (area > best_area) {
      best_rule = via.get();
      best_area = area;
    }
  }

  if (best_rule != nullptr) {
    return best_rule->generate(block);
  }

  // fallback to tech vias if generate is not possible
  auto* tech_via = findTechVia(lower, lower_rect, upper, upper_rect);
  if (tech_via != nullptr) {
    return new DbTechVia(tech_via);
  }

  return nullptr;
}

odb::dbTechVia* Connect::findTechVia(odb::dbTechLayer* lower,
                                     const odb::Rect& lower_rect,
                                     odb::dbTechLayer* upper,
                                     const odb::Rect& upper_rect) const
{
  const odb::Rect intersection = lower_rect.intersect(upper_rect);

  odb::dbTransform transform(
      odb::Point(0.5 * (intersection.xMin() + intersection.xMax()),
                 0.5 * (intersection.yMin() + intersection.yMax())));

  odb::dbTechVia* best_tech_via = nullptr;
  int best_cut_area = 0;

  for (odb::dbTechVia* db_via : tech_vias_) {
    if (db_via->getBottomLayer() != lower || db_via->getTopLayer() != upper) {
      continue;
    }

    odb::Rect bottom_rect;
    odb::Rect top_rect;
    bottom_rect.mergeInit();
    top_rect.mergeInit();
    int cut_area = 0;

    for (auto* box : db_via->getBoxes()) {
      odb::Rect box_rect;
      box->getBox(box_rect);
      if (box->getTechLayer() == db_via->getBottomLayer()) {
        bottom_rect.merge(box_rect);
      } else if (box->getTechLayer() == db_via->getTopLayer()) {
        top_rect.merge(box_rect);
      } else if (box->getTechLayer()->getType() == odb::dbTechLayerType::CUT) {
        cut_area += box_rect.area();
      }
    }

    transform.apply(bottom_rect);
    if (!lower_rect.contains(bottom_rect)) {
      continue;
    }

    transform.apply(top_rect);
    if (!upper_rect.contains(top_rect)) {
      continue;
    }

    if (best_cut_area < cut_area) {
      best_tech_via = db_via;
      best_cut_area = cut_area;
    }
  }

  return best_tech_via;
}

void Connect::populateGenerateRules()
{
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
}

void Connect::populateTechVias()
{
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
      if (db_via->getBottomLayer() != lower || db_via->getTopLayer() != upper) {
        continue;
      }
      tech_vias_.push_back(db_via);
    }
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

}  // namespace pdn
