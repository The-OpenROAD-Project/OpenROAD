// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "src/drt/src/rp/FlexRP.h"

#include "src/drt/src/frDesign.h"
#include "src/drt/src/frProfileTask.h"
#include "src/drt/src/global.h"
#include "src/utl/include/utl/Logger.h"

namespace drt {

FlexRP::FlexRP(frDesign* design,
               utl::Logger* logger,
               RouterConfiguration* router_cfg)
    : design_(design),
      tech_(design->getTech()),
      logger_(logger),
      router_cfg_(router_cfg)
{
}

void FlexRP::main()
{
  ProfileTask profile("RP:main");
  init();
  prep();
}

}  // namespace drt
