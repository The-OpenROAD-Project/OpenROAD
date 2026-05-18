// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "syn/synthesis.h"

#include "elab/driver.h"
#include "odb/db.h"
#include "utl/Logger.h"

namespace syn {

Synthesis::Synthesis(odb::dbDatabase* db, sta::dbSta* sta, utl::Logger* logger)
    : db_(db), logger_(logger)
{
  dbStaState::init(sta);
}

Synthesis::~Synthesis() = default;

bool Synthesis::elaborate(const std::vector<std::string>& args)
{
  auto result = syn::elaborate(args, sta_);
  if (!result) {
    logger_->error(utl::SYN, 1, "Elaboration failed.");
    return false;
  }
  graph_ = std::move(result);
  return true;
}

}  // namespace syn
