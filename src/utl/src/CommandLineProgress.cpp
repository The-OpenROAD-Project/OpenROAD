// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include "src/utl/src/CommandLineProgress.h"

#include <memory>

#include "src/utl/include/utl/Logger.h"
#include "src/utl/include/utl/Progress.h"

namespace utl {

CommandLineProgress::CommandLineProgress(Logger* logger) : Progress(logger)
{
}

void CommandLineProgress::start(std::shared_ptr<ProgressReporter>& reporter)
{
}

void CommandLineProgress::update(ProgressReporter* reporter)
{
}

void CommandLineProgress::end(ProgressReporter* reporter)
{
}

void CommandLineProgress::deleted(ProgressReporter* reporter)
{
}

}  // namespace utl
