// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "rcx/multiChipSpefWriter.h"

namespace rcx {

MultiChipSpefWriter::MultiChipSpefWriter(odb::dbDatabase* db,
                                         utl::Logger* logger)
    : db_{db}, logger_{logger}
{
}

void MultiChipSpefWriter::run(const SpefOptions& /* options */)
{
  logger_->error(utl::RCX, 516, "3D SPEF writing is not yet supported.");
}

}  // namespace rcx
