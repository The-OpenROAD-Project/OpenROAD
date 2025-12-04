// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "odb/defout.h"

#include <cassert>
#include <cstdio>
#include <memory>

#include "defout_impl.h"
#include "odb/db.h"

namespace odb {

DefOut::DefOut(utl::Logger* logger)
{
  writer_ = std::make_unique<Impl>(logger);
}

DefOut::~DefOut() = default;

void DefOut::selectNet(dbNet* net)
{
  writer_->selectNet(net);
}

void DefOut::selectInst(dbInst* inst)
{
  writer_->selectInst(inst);
}

void DefOut::setVersion(Version v)
{
  writer_->setVersion(v);
}

bool DefOut::writeBlock(dbBlock* block, const char* def_file)
{
  return writer_->writeBlock(block, def_file);
}

}  // namespace odb
