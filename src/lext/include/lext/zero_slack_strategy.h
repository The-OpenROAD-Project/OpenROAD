// Copyright 2025 Google LLC
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#pragma once

#include "db_sta/dbSta.hh"
#include "lext/resynthesis_strategy.h"
#include "lext/unique_name.h"
#include "sta/Corner.hh"
#include "utl/Logger.h"

namespace lext {

class ZeroSlackStrategy : public ResynthesisStrategy
{
 public:
  explicit ZeroSlackStrategy(sta::Corner* corner = nullptr) : corner_(corner) {}
  void OptimizeDesign(sta::dbSta* sta,
                      UniqueName& name_generator,
                      utl::Logger* logger) override;

 private:
  sta::Corner* corner_;
};

}  // namespace lext
