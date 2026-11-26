// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#include "SimulatedAnnealingCore.h"

#include <fstream>
#include <string>

#include "mpl-util.h"
#include "object.h"
#include "odb/db.h"
#include "utl/Logger.h"

namespace mpl {

template <class T>
void SimulatedAnnealingCore<T>::report(const Penalty& penalty) const
{
  logger_->report(
      "{:>15s} | {:>8.4f} | {:>7.4f} | {:>14.4f} | {:>7.4f} ",
      penalty.name,
      penalty.weight,
      penalty.value,
      penalty.normalization_factor,
      penalty.weight * penalty.value / penalty.normalization_factor);
}

template <class T>
void SimulatedAnnealingCore<T>::reportWeights() const
{
  logger_->report(
      "\n  Penalty Type  |  Weight  |  Value  |  Norm. Factor  |  Cost");
  logger_->report(
      "---------------------------------------------------------------");
  for (const Penalty& penalty : penalties_) {
    report(penalty);
  }
}

template <class T>
void SimulatedAnnealingCore<T>::reportTotalCost() const
{
  logger_->report(
      "---------------------------------------------------------------");
  logger_->report("  Total Cost  {:>49.4f} \n", getNormCost());
}

template <class T>
void SimulatedAnnealingCore<T>::reportLocations() const
{
  if constexpr (std::is_same_v<T, HardMacro>) {
    logger_->report("     Id     |                                Location");
  } else {
    logger_->report(" Cluster Id |                                Location");
  }

  logger_->report("-----------------------------------------------------");

  // First the moveable macros. I.e., those from the sequence pair.
  for (const int macro_id : pos_seq_) {
    int display_id;
    if constexpr (std::is_same_v<T, SoftMacro>) {
      const SoftMacro& soft_macro = macros_[macro_id];
      Cluster* cluster = soft_macro.getCluster();
      display_id = cluster->getId();
    } else {
      display_id = macro_id;
    }

    const T& macro = macros_[macro_id];
    logger_->report("{:>11d} | ({:^8.2f} {:^8.2f}) ({:^8.2f} {:^8.2f})",
                    display_id,
                    block_->dbuToMicrons(macro.getX()),
                    block_->dbuToMicrons(macro.getY()),
                    block_->dbuToMicrons(macro.getWidth()),
                    block_->dbuToMicrons(macro.getHeight()));
  }

  // Then, the fixed terminals.
  const int number_of_moveable_macros = static_cast<int>(pos_seq_.size());
  for (int i = number_of_moveable_macros; i < macros_.size(); ++i) {
    const T& macro = macros_[i];
    logger_->report("{:>11s} | ({:^8.2f} {:^8.2f}) ({:^8.2f} {:^8.2f})",
                    "fixed",
                    block_->dbuToMicrons(macro.getX()),
                    block_->dbuToMicrons(macro.getY()),
                    block_->dbuToMicrons(macro.getWidth()),
                    block_->dbuToMicrons(macro.getHeight()));
  }
  logger_->report("");
}

template <class T>
void SimulatedAnnealingCore<T>::printResults() const
{
  reportWeights();
  reportTotalCost();
  if (logger_->debugCheck(utl::MPL, "hierarchical_macro_placement", 2)) {
    reportLocations();
  }
}

template <class T>
void SimulatedAnnealingCore<T>::writeCostFile(
    const std::string& file_name) const
{
  std::ofstream file(file_name);
  for (auto i = 0; i < cost_list_.size(); i++) {
    file << T_list_[i] << "  " << cost_list_[i] << '\n';
  }
}

template class SimulatedAnnealingCore<SoftMacro>;
template class SimulatedAnnealingCore<HardMacro>;

}  // namespace mpl
