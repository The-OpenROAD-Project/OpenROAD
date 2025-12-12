// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#pragma once

#include "drt/TritonRoute.h"
#include "frDesign.h"
#include "utl/CallBack.h"
#include "utl/Logger.h"
namespace drt {

class frDesign;
class FlexPA;

class PACallBack : public utl::CallBack
{
 public:
  PACallBack(TritonRoute* router) : router_(router) {}

  ~PACallBack() override = default;

  void onPinAccessUpdateRequired() override
  {
    auto design = router_->getDesign();
    if (design == nullptr || design->getTopBlock() == nullptr) {
      return;
    }
    router_->updateDirtyPAData();
  }

 private:
  TritonRoute* router_;
};

}  // namespace drt
