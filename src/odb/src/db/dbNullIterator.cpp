// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbNullIterator.h"

#include <cstdint>

namespace odb {

dbNullIterator dbNullIterator::null_iterator;

bool dbNullIterator::reversible() const
{
  return false;
}

bool dbNullIterator::orderReversed() const
{
  return false;
}

void dbNullIterator::reverse(dbObject*)
{
}

uint32_t dbNullIterator::sequential() const
{
  return 0;
}

uint32_t dbNullIterator::size(dbObject*) const
{
  return 0;
}

uint32_t dbNullIterator::begin(dbObject*) const
{
  return 0;
}

uint32_t dbNullIterator::end(dbObject*) const
{
  return 0;
}

uint32_t dbNullIterator::next(uint32_t, ...) const
{
  return 0;
}

dbObject* dbNullIterator::getObject(uint32_t, ...)
{
  return nullptr;
}

}  // namespace odb
