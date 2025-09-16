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
  _writer = std::make_unique<Impl>(logger);
}

DefOut::~DefOut() = default;

void DefOut::selectNet(dbNet* net)
{
  _writer->selectNet(net);
}

void DefOut::selectInst(dbInst* inst)
{
  _writer->selectInst(inst);
}

void DefOut::setVersion(Version v)
{
  _writer->setVersion(v);
}

bool DefOut::writeBlock(dbBlock* block, const char* def_file)
{
  return _writer->writeBlock(block, def_file);
}

}  // namespace odb
