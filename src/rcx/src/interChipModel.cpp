// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "rcx/interChipModel.h"

#include <fstream>
#include <sstream>
#include <string>
#include <string_view>

#include "utl/Logger.h"

namespace {

constexpr std::string_view kHybridBondVia = "HBV";
constexpr std::string_view kViaResistanceTable = "VIA_RESISTANCE";
constexpr std::string_view kTableEnd = "END";

}  // namespace

namespace rcx {

InterChipModel parseInterChipRules(const std::string& rules_file_path,
                                   utl::Logger* logger)
{
  std::ifstream file(rules_file_path);

  if (!file.is_open()) {
    logger->error(utl::RCX,
                  516,
                  "Could not open assembly rules file {}.",
                  rules_file_path);
  }

  InterChipModel inter_chip_model;
  bool found_hybrid_bond_via = false;
  bool inside_via_resistance_table = false;

  std::string line;
  while (std::getline(file, line)) {
    line = line.substr(0, line.find('#'));

    std::istringstream line_stream(line);
    std::string token;
    if (!(line_stream >> token)) {
      continue;
    }

    if (token == kViaResistanceTable) {
      inside_via_resistance_table = true;
      continue;
    }

    if (!inside_via_resistance_table) {
      continue;
    }

    if (token == kTableEnd) {
      break;
    }

    if (token == kHybridBondVia) {
      line_stream >> inter_chip_model.resistance;
      found_hybrid_bond_via = true;
      break;
    }
  }

  if (!found_hybrid_bond_via) {
    logger->error(
        utl::RCX,
        538,
        "Could not find HBV via resistance in assembly rules file {}.",
        rules_file_path);
  }

  return inter_chip_model;
}

}  // namespace rcx
