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

#include <regex>

#include "grid.h"
#include "techlayer.h"
#include "odb/db.h"
#include "odb/dbTransform.h"
#include "utl/Logger.h"

namespace pdn {

Connect::Connect(Grid* grid, odb::dbTechLayer* layer0, odb::dbTechLayer* layer1)
    : grid_(grid),
      layer0_(layer0),
      layer1_(layer1),
      fixed_generate_vias_({}),
      fixed_tech_vias_({}),
      cut_pitch_x_(0),
      cut_pitch_y_(0),
      max_rows_(0),
      max_columns_(0)
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

void Connect::setOnGrid(const std::vector<odb::dbTechLayer*>& layers)
{
  ongrid_.insert(layers.begin(), layers.end());
  // remove top and bottom layers of the stack
  ongrid_.erase(layer0_);
  ongrid_.erase(layer1_);
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
  const double dbu_per_micron = block->getDbUnitsPerMicron();

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
  if (max_rows_ != 0) {
    logger->info(utl::PDN, 74, "    Maximum rows: {}", max_rows_);
  }
  if (max_columns_ != 0) {
    logger->info(utl::PDN, 75, "    Maximum columns: {}", max_columns_);
  }
  if (!ongrid_.empty()) {
    std::string layers;
    for (auto* l : ongrid_) {
      layers += l->getName() + " ";
    }
    logger->info(utl::PDN, 76, "    Ongrid layers: {}", layers);
  }
  if (!split_cuts_.empty()) {
    logger->info(utl::PDN, 77, "    Split cuts:");
    for (const auto& [layer, pitch] : split_cuts_) {
      logger->info(utl::PDN,
                   78,
                   "      Layer: {} with pitch {:.4f}",
                   layer->getName(),
                   pitch / dbu_per_micron);
    }
  }
}

void Connect::makeVia(odb::dbSWire* wire,
                      const odb::Rect& lower_rect,
                      const odb::Rect& upper_rect,
                      odb::dbWireShapeType type,
                      DbVia::ViaLayerShape& shapes)
{
  const odb::Rect intersection = lower_rect.intersect(upper_rect);

  const std::pair<int, int> via_index
      = std::make_pair(intersection.dx(), intersection.dy());
  auto& via = vias_[via_index];

  auto* tech = layer0_->getTech();
  const int x = TechLayer::snapToManufacturingGrid(tech, 0.5 * (intersection.xMin() + intersection.xMax()));
  const int y = TechLayer::snapToManufacturingGrid(tech, 0.5 * (intersection.yMin() + intersection.yMax()));

  // make the via stack if one is not available for the given size
  if (via == nullptr) {
    std::vector<DbVia*> stack;
    std::vector<odb::dbTechLayer*> layers = getAllRoutingLayers();

    for (int i = 1; i < layers.size(); i++) {
      auto* l0 = layers[i - 1];
      auto* l1 = layers[i];

      auto via_lower_rect = lower_rect;
      if (l0 != layer0_) {
        via_lower_rect = intersection;
      }
      auto via_upper_rect = upper_rect;
      if (l1 != layer1_) {
        via_upper_rect = intersection;
      }

      auto* new_via = makeSingleLayerVia(
          wire->getBlock(), l0, via_lower_rect, l1, via_upper_rect);
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
            new DbGenerateDummyVia(grid_->getLogger(), area, layer0_, layer1_));
        break;
      } else {
        stack.push_back(new_via);
      }
    }

    if (stack.size() > 1) {
      via = std::make_unique<DbGenerateStackedVia>(
          stack, layer0_, wire->getBlock(), ongrid_);
    } else {
      via = std::unique_ptr<DbVia>(stack[0]);
    }
  }

  shapes = via->generate(wire->getBlock(), wire, type, x, y);
}

DbVia* Connect::generateDbVia(
    const std::vector<std::unique_ptr<ViaGenerator>>& generators,
    odb::dbBlock* block) const
{
  ViaGenerator* best_rule = nullptr;
  int best_area = 0;
  for (const auto& via : generators) {
    if (hasCutPitch()) {
      via->setCutPitchX(cut_pitch_x_);
      via->setCutPitchY(cut_pitch_y_);
    }
    via->setMaxRows(max_rows_);
    via->setMaxColumns(max_columns_);

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
    via->determineRowsAndColumns(lower_is_internal, upper_is_internal);

    debugPrint(grid_->getLogger(),
               utl::PDN,
               "Via",
               1,
               "Cut class {} - {}",
               via->getCutLayer()->getName(),
               via->hasCutClass() ? via->getCutClass()->getName() : "none");

    if (!via->checkConstraints()) {
      continue;
    }

    const int area = via->getCutArea();
    debugPrint(grid_->getLogger(),
               utl::PDN,
               "Via",
               3,
               "{}: Current via area {} with {} cuts, best via area {}",
               via->getCutLayer()->getName(),
               area, via->getTotalCuts(), best_area);

    if (area > best_area) {
      best_rule = via.get();
      best_area = area;
    }
  }

  if (best_rule != nullptr) {
    return best_rule->generate(block);
  }

  return nullptr;
}

DbVia* Connect::makeSingleLayerVia(odb::dbBlock* block,
                                   odb::dbTechLayer* lower,
                                   const odb::Rect& lower_rect,
                                   odb::dbTechLayer* upper,
                                   const odb::Rect& upper_rect) const
{
  // look for generate vias
  std::vector<std::unique_ptr<ViaGenerator>> generate_vias;
  for (odb::dbTechViaGenerateRule* db_via : generate_via_rules_) {
    std::unique_ptr<GenerateViaGenerator> rule
        = std::make_unique<GenerateViaGenerator>(
            grid_->getLogger(), db_via, lower_rect, upper_rect);

    if (!rule->isSetupValid(lower, upper)) {
      continue;
    }

    generate_vias.push_back(std::move(rule));
  }

  DbVia* generate_via = generateDbVia(generate_vias, block);

  if (generate_via != nullptr) {
    return generate_via;
  }

  // fallback to tech vias if generate is not possible
  // look for generate vias
  std::vector<std::unique_ptr<ViaGenerator>> tech_vias;
  for (odb::dbTechVia* db_via : tech_vias_) {
    std::unique_ptr<TechViaGenerator> rule
        = std::make_unique<TechViaGenerator>(grid_->getLogger(), db_via, lower_rect, upper_rect);

    if (!rule->isSetupValid(lower, upper)) {
      continue;
    }

    tech_vias.push_back(std::move(rule));
  }

  return generateDbVia(tech_vias, block);
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

}  // namespace pdn
