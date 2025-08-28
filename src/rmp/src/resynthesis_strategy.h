// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#pragma once

#include "db_sta/dbSta.hh"
#include "rsz/Resizer.hh"
#include "utl/Logger.h"
#include "utl/unique_name.h"

namespace rmp {

class ResynthesisStrategy
{
 public:
  virtual ~ResynthesisStrategy() = default;
  virtual void OptimizeDesign(sta::dbSta* sta,
                              utl::UniqueName& name_generator,
                              rsz::Resizer* resizer,
                              utl::Logger* logger)
      = 0;
};
}  // namespace rmp
