// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "rcx/multiChipExtractor.h"

namespace rcx {

MultiChipExtractor::MultiChipExtractor(odb::dbDatabase* db, utl::Logger* logger)
    : db_{db}, logger_{logger}
{
}

void MultiChipExtractor::run(const ExtractOptions& /* options */)
{
  logger_->error(utl::RCX, 515, "3D extraction is not yet supported.");
}

}  // namespace rcx
