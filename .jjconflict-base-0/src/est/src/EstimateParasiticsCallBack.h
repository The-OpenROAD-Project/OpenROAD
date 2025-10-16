// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#pragma once

#include "est/EstimateParasitics.h"
#include "utl/CallBack.h"
#include "utl/Logger.h"

namespace est {

class frDesign;
class FlexPA;

class EstimateParasiticsCallBack : public utl::CallBack
{
 public:
  EstimateParasiticsCallBack(EstimateParasitics* estimate_parasitics)
      : estimate_parasitics_(estimate_parasitics)
  {
  }

  ~EstimateParasiticsCallBack() override = default;

  void onEstimateParasiticsRequired() override;

 private:
  EstimateParasitics* estimate_parasitics_;
};

}  // namespace est
