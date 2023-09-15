// Copyright 2023 Google LLC
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "gtest/gtest.h"
#include "src/cts/src/Clock.h"
#include "src/cts/src/HTreeBuilder.h"
#include "utl/Logger.h"

namespace cts {

TEST(HTreeBuilderTest, Instantiates)
{
  Clock clock(/*netName=*/"clk",
              /*clockPin=*/"p0",
              /*sdcClockName=*/"clk",
              /*clockPinX=*/0,
              /*clockPinY=*/0);

  utl::Logger logger;
  HTreeBuilder instance(
      /*options=*/nullptr, clock, /*parent=*/nullptr, &logger, nullptr);
}

}  // namespace cts
