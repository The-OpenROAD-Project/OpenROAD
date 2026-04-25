// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "Optimizer.hh"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <memory>
#include <string>
#include <vector>

#include "MeasuredVtSwapPolicy.hh"
#include "OptPolicy.hh"
#include "OptimizerTypes.hh"
#include "SetupLegacyMtPolicy.hh"
#include "SetupLegacyPolicy.hh"
#include "SetupMt1Policy.hh"
#include "rsz/Resizer.hh"
#include "utl/scope.h"

namespace rsz {

Optimizer::Optimizer(Resizer* resizer)
    : resizer_(*resizer), committer_(resizer_)
{
}

Optimizer::PolicySelection Optimizer::selectPolicy() const
{
  // Map the user-facing environment knob to one concrete policy object.
  const char* policy = std::getenv("RSZ_POLICY");
  if (policy == nullptr) {
    return PolicySelection::kLegacy;
  }
  std::string name(policy);
  std::ranges::transform(name, name.begin(), [](const unsigned char ch) {
    return static_cast<char>(std::tolower(ch));
  });
  if (name == "legacy_mt") {
    return PolicySelection::kLegacyMt;
  }
  if (name == "vtswapmt1" || name == "vtswap_mt1" || name == "mt1"
      || name == "vtswapmtpolicy" || name == "vtswapmt1policy"
      || name == "mt1policy") {
    return PolicySelection::kVtSwapMt1;
  }
  if (name == "measured_vt_swap" || name == "measured_vt"
      || name == "measuredvtswappolicy") {
    return PolicySelection::kMeasuredVtSwap;
  }
  return PolicySelection::kLegacy;
}

void Optimizer::selectActivePolicy()
{
  // Instantiate exactly one policy so the run loop stays policy-agnostic.
  switch (selectPolicy()) {
    case PolicySelection::kVtSwapMt1:
      opt_policy_ = std::make_unique<SetupMt1Policy>(resizer_, committer_);
      break;
    case PolicySelection::kMeasuredVtSwap:
      opt_policy_
          = std::make_unique<MeasuredVtSwapPolicy>(resizer_, committer_);
      break;
    case PolicySelection::kLegacyMt:
      opt_policy_ = std::make_unique<SetupLegacyMtPolicy>(resizer_, committer_);
      break;
    case PolicySelection::kLegacy:
      opt_policy_ = std::make_unique<SetupLegacyPolicy>(resizer_, committer_);
      break;
  }
}

Optimizer::~Optimizer() = default;

void Optimizer::configure(const OptimizerRunConfig& config)
{
  config_ = config;
}

bool Optimizer::runActivePolicy()
{
  // Drive the selected policy until it reports convergence.
  opt_policy_->start(config_);
  while (!opt_policy_->converged()) {
    opt_policy_->iterate();
  }
  return opt_policy_->result();
}

bool Optimizer::run()
{
  // Keep the footprint override local to this optimizer run.
  utl::SetAndRestore<bool> match_cell_footprint_override(
      resizer_.matchCellFootprint(), config_.match_cell_footprint);

  // Reuse the legacy setup preamble before the selected policy starts.
  resizer_.runRepairSetupPreamble();
  selectActivePolicy();
  committer_.init();
  return runActivePolicy();
}

bool Optimizer::converged() const
{
  return opt_policy_->converged();
}

bool Optimizer::repairSetup(double setup_margin,
                            double repair_tns_end_percent,
                            int max_passes,
                            int max_iterations,
                            int max_repairs_per_pass,
                            bool match_cell_footprint,
                            bool verbose,
                            const std::vector<MoveType>& sequence,
                            const char* phases,
                            bool skip_pin_swap,
                            bool skip_gate_cloning,
                            bool skip_size_down,
                            bool skip_buffering,
                            bool skip_buffer_removal,
                            bool skip_last_gasp,
                            bool skip_vt_swap,
                            bool skip_crit_vt_swap)
{
  configure(makeRepairSetupConfig(setup_margin,
                                  repair_tns_end_percent,
                                  max_passes,
                                  max_iterations,
                                  max_repairs_per_pass,
                                  match_cell_footprint,
                                  verbose,
                                  sequence,
                                  phases,
                                  skip_pin_swap,
                                  skip_gate_cloning,
                                  skip_size_down,
                                  skip_buffering,
                                  skip_buffer_removal,
                                  skip_last_gasp,
                                  skip_vt_swap,
                                  skip_crit_vt_swap));
  return run();
}

bool Optimizer::repairSetup(const sta::Pin* end_pin)
{
  if (end_pin == nullptr) {
    return false;
  }

  OptimizerRunConfig config;
  config.setup_slack_margin = 0.0;
  config.max_repairs_per_pass = 1;
  config.match_cell_footprint = resizer_.matchCellFootprint();
  config.sequence = {MoveType::kUnbuffer,
                     MoveType::kVtSwap,
                     MoveType::kSizeDown,
                     MoveType::kSizeUp,
                     MoveType::kSwapPins,
                     MoveType::kBuffer,
                     MoveType::kClone,
                     MoveType::kSplitLoad};

  utl::SetAndRestore<bool> match_cell_footprint_override(
      resizer_.matchCellFootprint(), config.match_cell_footprint);
  resizer_.runRepairSetupPreamble();

  SetupLegacyPolicy policy(resizer_, committer_);
  policy.start(config);
  committer_.init();
  return policy.repairSetupPin(end_pin);
}

OptimizerRunConfig Optimizer::makeRepairSetupConfig(
    const double setup_margin,
    const double repair_tns_end_percent,
    const int max_passes,
    const int max_iterations,
    const int max_repairs_per_pass,
    const bool match_cell_footprint,
    const bool verbose,
    const std::vector<MoveType>& sequence,
    const char* phases,
    const bool skip_pin_swap,
    const bool skip_gate_cloning,
    const bool skip_size_down,
    const bool skip_buffering,
    const bool skip_buffer_removal,
    const bool skip_last_gasp,
    const bool skip_vt_swap,
    const bool skip_crit_vt_swap)
{
  OptimizerRunConfig config;
  // Freeze all Tcl-facing knobs into one immutable policy config object.
  config.setup_slack_margin = setup_margin;
  config.repair_tns_end_percent = repair_tns_end_percent;
  config.max_passes = max_passes;
  config.max_iterations = max_iterations;
  config.max_repairs_per_pass = max_repairs_per_pass;
  config.match_cell_footprint = match_cell_footprint;
  config.verbose = verbose;
  config.sequence = sequence;
  config.phases = phases != nullptr ? phases : "";
  config.skip_pin_swap = skip_pin_swap;
  config.skip_gate_cloning = skip_gate_cloning;
  config.skip_size_down = skip_size_down;
  config.skip_buffering = skip_buffering;
  config.skip_buffer_removal = skip_buffer_removal;
  config.skip_last_gasp = skip_last_gasp;
  config.skip_vt_swap = skip_vt_swap;
  config.skip_crit_vt_swap = skip_crit_vt_swap;
  return config;
}

MoveCommitter& Optimizer::committer()
{
  return committer_;
}

}  // namespace rsz
