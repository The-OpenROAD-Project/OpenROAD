// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "dbStreamLayout.h"

#include <cstdint>
#include <ostream>
#include <string_view>

#include "dbDatabase.h"
#include "odb/db.h"
#include "odb/dbStream.h"

namespace odb {

dbStreamLayout::dbStreamLayout(std::ostream& out) : out_(out)
{
  out_ << "odb-layout 1\n";
}

void dbStreamLayout::addSlot(const char* table,
                             const uint64_t offset,
                             const uint64_t length)
{
  ++slots_;
  const bool extends = count_ > 0 && std::string_view(table) == table_
                       && length == length_
                       && offset == offset_ + length_ * count_;
  if (extends) {
    ++count_;
    return;
  }
  flushRun();
  table_ = table;
  offset_ = offset;
  length_ = length;
  count_ = 1;
}

void dbStreamLayout::finish(const uint64_t size)
{
  flushRun();
  out_ << "size " << size << '\n';
  out_.flush();
}

void dbStreamLayout::flushRun()
{
  if (count_ > 0) {
    out_ << table_ << ' ' << offset_ << ' ' << length_ << ' ' << count_ << '\n';
  }
  count_ = 0;
}

// Here rather than in dbDatabase.cpp, which the code generator owns.
void dbDatabase::write(std::ostream& file, std::ostream& layout)
{
  _dbDatabase* db = reinterpret_cast<_dbDatabase*>(this);
  dbStreamLayout recorder(layout);
  dbOStream stream(db, file);
  stream.setLayout(&recorder);
  stream << *db;
  stream.flush();
  file.flush();
  recorder.finish(stream.tell());
}

}  // namespace odb
