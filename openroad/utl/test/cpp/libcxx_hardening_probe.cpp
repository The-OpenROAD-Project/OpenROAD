// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <cstddef>
#include <vector>

#include "LibcxxAbort.h"
#include "ord/OpenRoad.hh"
#include "utl/Logger.h"
#include "utl/StackTrace.h"

// Exercise the abort override before an OpenRoad logger has been created.
ord::OpenRoad* ord::OpenRoad::openRoad()
{
  return nullptr;
}

int main(int argc, char* argv[])
{
  utl::initStackTrace(argv[0]);
  if (argc == 2) {
    utl::Logger logger(argv[1]);
    ord::logLibcxxFailure(logger, "test frame\n", "test condition");
  }
  std::vector<int> values(1);
  volatile std::size_t index = 1;
  volatile int observed = values[index];
  (void) observed;
  return 0;
}
