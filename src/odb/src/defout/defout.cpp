// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "odb/defout.h"

#include <cstdio>

#include "defout_impl.h"
#include "odb/db.h"

namespace odb {

defout::defout(utl::Logger* logger)
{
  _writer = new defout_impl(logger);
  assert(_writer);
}

defout::~defout()
{
  delete _writer;
}

void defout::setUseLayerAlias(bool value)
{
  _writer->setUseLayerAlias(value);
}

void defout::setUseNetInstIds(bool value)
{
  _writer->setUseNetInstIds(value);
}

void defout::setUseMasterIds(bool value)
{
  _writer->setUseMasterIds(value);
}

void defout::selectNet(dbNet* net)
{
  _writer->selectNet(net);
}

void defout::setVersion(Version v)
{
  _writer->setVersion(v);
}

bool defout::writeBlock(dbBlock* block, const char* def_file)
{
  return _writer->writeBlock(block, def_file);
}

}  // namespace odb
