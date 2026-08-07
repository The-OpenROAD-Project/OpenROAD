// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include "odb/db.h"
#include "rcx/ext_options.h"
#include "utl/Logger.h"

namespace rcx {

class MultiChipExtractor
{
 public:
  MultiChipExtractor(odb::dbDatabase* db, utl::Logger* logger);

  void run(const ExtractOptions& options);

 private:
  odb::dbDatabase* db_{nullptr};
  utl::Logger* logger_{nullptr};
};

}  // namespace rcx
