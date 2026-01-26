// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <unistd.h>

#include <cstdio>
#include <list>
#include <string>
#include <vector>

#include "odb/db.h"

namespace odb {

class cdl
{
 public:
  static bool writeCdl(utl::Logger* logger,
                       dbBlock* block,
                       const char* outFileName,
                       const std::vector<const char*>& mastersFileNames,
                       bool includeFillers = false);
};

}  // namespace odb
