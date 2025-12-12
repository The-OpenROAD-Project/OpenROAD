// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbNullIterator.h"

#include "odb/odb.h"

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

uint dbNullIterator::sequential() const
{
  return 0;
}

uint dbNullIterator::size(dbObject*) const
{
  return 0;
}

uint dbNullIterator::begin(dbObject*) const
{
  return 0;
}

uint dbNullIterator::end(dbObject*) const
{
  return 0;
}

uint dbNullIterator::next(uint, ...) const
{
  return 0;
}

dbObject* dbNullIterator::getObject(uint, ...)
{
  return nullptr;
}

}  // namespace odb
