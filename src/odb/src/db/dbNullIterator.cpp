// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbNullIterator.h"

namespace odb {

dbNullIterator dbNullIterator::null_iterator;

bool dbNullIterator::reversible()
{
  return false;
}

bool dbNullIterator::orderReversed()
{
  return false;
}

void dbNullIterator::reverse(dbObject*)
{
}

uint dbNullIterator::sequential()
{
  return 0;
}

uint dbNullIterator::size(dbObject*)
{
  return 0;
}

uint dbNullIterator::begin(dbObject*)
{
  return 0;
}

uint dbNullIterator::end(dbObject*)
{
  return 0;
}

uint dbNullIterator::next(uint, ...)
{
  return 0;
}

dbObject* dbNullIterator::getObject(uint, ...)
{
  return nullptr;
}

}  // namespace odb
