// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "AntennaReport.hh"

#include <string>
#include <string_view>

#include "ant/AntennaChecker.hh"
#include "odb/db.h"
#include "utl/Logger.h"  // fmt

namespace ant {

namespace {

void formatRatio(std::string& report,
                 std::string_view ratio_name,
                 std::string_view rule_name,
                 const RatioCheck& check)
{
  report += fmt::format(
      "      {}: {:7.2f}\n      Required ratio: {:7.2f} ({}) {}\n",
      ratio_name,
      check.actual,
      check.required,
      rule_name,
      check.violated ? "(VIOLATED)" : "");
}

}  // namespace

std::string formatNetCheck(const NetCheck& net_check)
{
  std::string report = fmt::format("Net: {}\n", net_check.net->getConstName());
  for (const PinCheck& pin : net_check.pins) {
    odb::dbMTerm* mterm = pin.gate->getMTerm();
    report += fmt::format("  Pin:   {}/{} ({})\n",
                          pin.gate->getInst()->getConstName(),
                          mterm->getConstName(),
                          mterm->getMaster()->getConstName());
    for (const LayerCheck& layer : pin.layers) {
      report += fmt::format("    Layer: {}\n", layer.layer->getConstName());
      formatRatio(report, "Partial area ratio", "Gate area", layer.par);
      if (layer.car) {
        formatRatio(
            report, "Cumulative area ratio", "Cumulative area", *layer.car);
      }
      if (layer.psr) {
        formatRatio(report, "Partial area ratio", "Side area", *layer.psr);
      }
      if (layer.csr) {
        formatRatio(report,
                    "Cumulative area ratio",
                    "Cumulative side area",
                    *layer.csr);
      }
      report += "\n";
    }
    report += "\n";
  }
  return report;
}

}  // namespace ant
