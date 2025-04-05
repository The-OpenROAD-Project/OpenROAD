// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "FlexRP.h"

#include "frProfileTask.h"

namespace drt {

FlexRP::FlexRP(frDesign* design,
               Logger* logger,
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
