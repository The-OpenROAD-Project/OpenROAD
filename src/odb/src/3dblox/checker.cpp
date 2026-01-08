// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2026, The OpenROAD Authors

#include "checker.h"

#include "threeDBloxValidator.h"
#include "unfoldedModel.h"

namespace odb {

Checker::Checker(utl::Logger* logger) : logger_(logger)
{
}

void Checker::check(odb::dbChip* chip)
{
  UnfoldedModel model(logger_);
  model.build(chip);

  ThreeDBloxValidator validator(logger_);
  validator.validate(model, chip);
}

}  // namespace odb
