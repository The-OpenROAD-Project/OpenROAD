// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#pragma once

#include "db_sta/dbSta.hh"
#include "rmp/unique_name.h"
#include "utl/Logger.h"

namespace rmp {

class ResynthesisStrategy
{
 public:
  virtual ~ResynthesisStrategy() = default;
  virtual void OptimizeDesign(sta::dbSta* sta,
                              rmp::UniqueName& name_generator,
                              utl::Logger* logger)
      = 0;
};
}  // namespace rmp
