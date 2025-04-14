// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#pragma once

#include <csignal>
#include <memory>

#include "utl/Progress.h"

namespace utl {

class Logger;

class CommandLineProgress : public Progress
{
 public:
  CommandLineProgress(Logger* logger);
  ~CommandLineProgress() override = default;
  void start(std::shared_ptr<ProgressReporter>& reporter) override;
  void update(ProgressReporter* reporter) override;
  void end(ProgressReporter* reporter) override;
  void deleted(ProgressReporter* reporter) override;
};

}  // namespace utl
