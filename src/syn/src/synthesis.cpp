// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include "syn/synthesis.h"

#include <memory>
#include <utility>

#include "odb/db.h"
#include "utl/Logger.h"

namespace syn {

Synthesis::Synthesis(odb::dbDatabase* db, sta::dbSta* sta, utl::Logger* logger)
    : db_(db), logger_(logger)
{
  dbStaState::init(sta);
}

Synthesis::~Synthesis() = default;

}  // namespace syn
