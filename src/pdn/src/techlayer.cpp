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

#include "techlayer.h"

#include "utl/Logger.h"

#include <cmath>
#include <limits>

namespace pdn {

TechLayer::TechLayer(odb::dbTechLayer* layer) : layer_(layer), grid_({})
{
}

int TechLayer::getSpacing(int width, int length) const
{
  // get the spacing the DB would use
  const int db_spacing = layer_->getSpacing(width, length);

  // Check the two widths table for spacing assuming same width metal
  const int two_widths_spacing = layer_->findTwSpacing(width, width, length);

  return std::max(db_spacing, two_widths_spacing);
}

int TechLayer::micronToDbu(const std::string& value) const
{
  return micronToDbu(std::stof(value));
}

int TechLayer::micronToDbu(double value) const
{
  return std::round(value * getLefUnits());
}

double TechLayer::dbuToMicron(int value) const
{
  return value / static_cast<double>(getLefUnits());
}

std::vector<std::vector<std::string>> TechLayer::tokenizeStringProperty(
    const std::string& property_name) const
{
  auto* property = odb::dbStringProperty::find(layer_, property_name.c_str());
  if (property == nullptr) {
    return {};
  }
  std::vector<std::vector<std::string>> tokenized_set;
  std::vector<std::string> tokenized;
  std::string token;
  for (const char& c : property->getValue()) {
    if (std::isspace(c)) {
      if (!token.empty()) {
        tokenized.push_back(token);
        token.clear();
      }
      continue;
    }

    if (c == ';') {
      tokenized_set.push_back(tokenized);
      tokenized.clear();
      continue;
    }

    token += c;
  }

  if (!token.empty()) {
    tokenized.push_back(token);
  }

  if (!tokenized.empty()) {
    tokenized_set.push_back(tokenized);
  }

  return tokenized_set;
}

void TechLayer::populateGrid(odb::dbBlock* block, odb::dbTechLayerDir dir)
{
  grid_.clear();

  if (dir == odb::dbTechLayerDir::NONE) {
    dir = layer_->getDirection();
  }

  auto* tracks = block->findTrackGrid(layer_);
  if (dir == odb::dbTechLayerDir::HORIZONTAL) {
    tracks->getGridY(grid_);
  } else {
    tracks->getGridX(grid_);
  }
}

int TechLayer::snapToGrid(int pos, int greater_than) const
{
  if (grid_.empty()) {
    return pos;
  }

  int delta_pos = 0;
  int delta = std::numeric_limits<int>::max();
  for (size_t i = 0; i < grid_.size(); i++) {
    const int grid_pos = grid_[i];
    if (grid_pos < greater_than) {
      // ignore since it is lower than the minimum
      continue;
    }

    // look for smallest delta
    const int new_delta = std::abs(pos - grid_pos);
    if (new_delta < delta) {
      delta_pos = grid_pos;
      delta = new_delta;
    } else {
      break;
    }
  }
  return delta_pos;
}

int TechLayer::snapToManufacturingGrid(odb::dbTech* tech, int pos, bool round_up)
{
  if (!tech->hasManufacturingGrid()) {
    return pos;
  }

  const int grid = tech->getManufacturingGrid();

  if (pos % grid != 0) {
    int round_pos = pos / grid;
    if (round_up) {
      round_pos += 1;
    }
    pos = round_pos * grid;
  }

  return pos;
}

bool TechLayer::checkIfManufacturingGrid(int value, utl::Logger* logger, const std::string& type) const
{
  auto* tech = layer_->getTech();
  if (!tech->hasManufacturingGrid()) {
    return true;
  }

  const int grid = tech->getManufacturingGrid();

  if (value % grid != 0) {
    logger->error(utl::PDN, 191, "{} of {:.4f} does not fit the manufacturing grid of {:.4f}.", type, dbuToMicron(value), dbuToMicron(grid));
    return false;
  }

  return true;
}

int TechLayer::snapToManufacturingGrid(int pos, bool round_up) const
{
  return snapToManufacturingGrid(layer_->getTech(), pos, round_up);
}

std::vector<TechLayer::ArraySpacing> TechLayer::getArraySpacing() const
{
  const auto tokenized_set = tokenizeStringProperty("LEF58_ARRAYSPACING");
  if (tokenized_set.empty()) {
    return {};
  }

  auto& tokenized = tokenized_set[0];

  // get cut spacing
  int cut_spacing = 0;
  auto cut_spacing_find
      = std::find(tokenized.begin(), tokenized.end(), "CUTSPACING");
  if (cut_spacing_find != tokenized.end()) {
    cut_spacing_find++;

    cut_spacing = micronToDbu(*cut_spacing_find);
  }
  int width = 0;
  auto width_find = std::find(tokenized.begin(), tokenized.end(), "WIDTH");
  if (width_find != tokenized.end()) {
    width_find++;

    width = micronToDbu(*width_find);
  }
  bool longarray = false;
  auto longarray_find
      = std::find(tokenized.begin(), tokenized.end(), "LONGARRAY");
  if (longarray_find != tokenized.end()) {
    longarray = true;
  }

  // get cuts
  std::vector<ArraySpacing> spacing;
  ArraySpacing props{0, false, 0, 0, 0};
  for (auto itr = tokenized.begin(); itr != tokenized.end(); itr++) {
    if (*itr == "ARRAYCUTS") {
      if (props.cuts != 0) {
        spacing.push_back(props);
      }
      itr++;
      const int cuts = std::stoi(*itr);
      props = ArraySpacing{width, longarray, cut_spacing, cuts, 0};
      continue;
    }
    if (*itr == "SPACING") {
      itr++;
      const int spacing = micronToDbu(*itr);
      props.array_spacing = spacing;
      continue;
    }
  }

  return spacing;
}

std::vector<TechLayer::MinCutRule> TechLayer::getMinCutRules() const
{
  std::vector<MinCutRule> rules;

  // get all the LEF55 rules
  for (auto* min_cut_rule : layer_->getMinCutRules()) {
    uint numcuts;
    uint rule_width;
    min_cut_rule->getMinimumCuts(numcuts, rule_width);

    rules.push_back(MinCutRule{
      nullptr,
      min_cut_rule->isAboveOnly(),
      min_cut_rule->isBelowOnly(),
      static_cast<int>(rule_width),
      static_cast<int>(numcuts)
    });
  }

  for (const auto& rule_set : tokenizeStringProperty("LEF58_MINIMUMCUT")) {
    int width = 0;
    auto width_find = std::find(rule_set.begin(), rule_set.end(), "WIDTH");
    if (width_find != rule_set.end()) {
      width_find++;

      width = micronToDbu(*width_find);
    }

    const bool fromabove = std::find(rule_set.begin(), rule_set.end(), "FROMABOVE") != rule_set.end();
    const bool frombelow = std::find(rule_set.begin(), rule_set.end(), "FROMBELOW") != rule_set.end();

    for (auto itr = rule_set.begin(); itr != rule_set.end(); itr++) {
      if (*itr == "CUTCLASS") {
        itr++;
        const std::string cutclass = *itr;
        itr++;
        const int cuts = std::stoi(*itr);

        auto* cut_class = layer_->getLowerLayer()->findTechLayerCutClassRule(cutclass.c_str());
        if (cut_class == nullptr) {
          cut_class = layer_->getUpperLayer()->findTechLayerCutClassRule(cutclass.c_str());
        }

        rules.push_back(MinCutRule{
          cut_class,
          frombelow, // same as ABOVE
          fromabove, // same as BELOW
          width,
          cuts
        });
      }
    }
  }

  return rules;
}

std::vector<TechLayer::WidthTable> TechLayer::getWidthTable() const
{
  auto width_tables = tokenizeStringProperty("LEF58_WIDTHTABLE");
  if (width_tables.empty()) {
    return {};
  }

  std::vector<WidthTable> tables;
  for (auto& width_table : width_tables) {
    WidthTable table{false, false, {}};
    width_table.erase(
        std::find(width_table.begin(), width_table.end(), "WIDTHTABLE"));
    auto find_wrongdirection
        = std::find(width_table.begin(), width_table.end(), "WRONGDIRECTION");
    if (find_wrongdirection != width_table.end()) {
      table.wrongdirection = true;
      width_table.erase(find_wrongdirection);
    }
    auto find_orthogonal
        = std::find(width_table.begin(), width_table.end(), "ORTHOGONAL");
    if (find_orthogonal != width_table.end()) {
      table.orthogonal = true;
      width_table.erase(find_orthogonal);
    }

    for (const auto& width : width_table) {
      table.widths.push_back(micronToDbu(width));
    }

    tables.push_back(table);
  }

  return tables;
}

}  // namespace pdn
