///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2022, The Regents of the University of California
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

// Generator Code Begin Cpp
#include "dbScanPin.h"

#include "db.h"
#include "dbDatabase.h"
#include "dbDiff.hpp"
#include "dbTable.h"
#include "dbTable.hpp"

// User Code Begin Includes
namespace {
template <class>
inline constexpr bool always_false_v = false;
}  // namespace
// User Code End Includes
namespace odb {
template class dbTable<_dbScanPin>;

bool _dbScanPin::operator==(const _dbScanPin& rhs) const
{
  // User Code Begin ==
  if (pin_ != rhs.pin_) {
    return false;
  }
  // User Code End ==
  return true;
}

bool _dbScanPin::operator<(const _dbScanPin& rhs) const
{
  // User Code Begin <
  return std::visit(
      [rhs](auto&& pin) {
        return std::visit([&pin](auto&& rhs_pin) { return pin < rhs_pin; },
                          rhs.pin_);
      },
      pin_);
  // User Code End <
  return true;
}

void _dbScanPin::differences(dbDiff& diff,
                             const char* field,
                             const _dbScanPin& rhs) const
{
  DIFF_BEGIN
  // User Code Begin Differences
  std::visit(
      [&diff, rhs](auto&& pin) {
        std::visit(
            [&diff, &pin](auto&& rhs_pin) { diff.diff("pin_", pin, rhs_pin); },
            rhs.pin_);
      },
      pin_);
  // User Code End Differences
  DIFF_END
}

void _dbScanPin::out(dbDiff& diff, char side, const char* field) const
{
  DIFF_OUT_BEGIN

  // User Code Begin Out
  std::visit([&diff, side](auto&& ptr) { diff.out(side, "pin_", ptr); }, pin_);
  // User Code End Out
  DIFF_END
}

_dbScanPin::_dbScanPin(_dbDatabase* db)
{
}

_dbScanPin::_dbScanPin(_dbDatabase* db, const _dbScanPin& r)
{
  // User Code Begin CopyConstructor
  pin_ = r.pin_;
  // User Code End CopyConstructor
}

dbIStream& operator>>(dbIStream& stream, _dbScanPin& obj)
{
  // User Code Begin >>
  int index = 0;
  stream >> index;
  if (index == 0) {
    dbId<_dbBTerm> bterm;
    stream >> bterm;
    obj.pin_ = bterm;
  } else if (index == 1) {
    dbId<_dbITerm> iterm;
    stream >> iterm;
    obj.pin_ = iterm;
  }
  // User Code End >>
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbScanPin& obj)
{
  // User Code Begin <<
  int index = obj.pin_.index();
  stream << index;
  if (index == 0) {
    stream << std::get<0>(obj.pin_);
  } else if (index == 1) {
    stream << std::get<1>(obj.pin_);
  }
  // User Code End <<
  return stream;
}

////////////////////////////////////////////////////////////////////
//
// dbScanPin - Methods
//
////////////////////////////////////////////////////////////////////

// User Code Begin dbScanPinPublicMethods
std::variant<dbBTerm*, dbITerm*> dbScanPin::getPin() const
{
  const _dbScanPin* scan_pin = (_dbScanPin*) this;
  const _dbBlock* block = (_dbBlock*) scan_pin->getOwner();

  return std::visit(
      [block](auto&& pin) {
        using T = std::decay_t<decltype(pin)>;
        if constexpr (std::is_same_v<T, dbId<_dbBTerm>>) {
          return (dbBTerm*) block->_bterm_tbl->getPtr(pin);
        } else if constexpr (std::is_same_v<T, dbId<_dbITerm>>) {
          return (dbBTerm*) block->_iterm_tbl->getPtr(pin);
        } else {
          static_assert(always_false_v<T>, "non-exhaustive visitor!");
        }
      },
      scan_pin->pin_);
}

void dbScanPin::setPin(dbBTerm* bterm)
{
  _dbScanPin* scan_pin = (_dbScanPin*) this;
  scan_pin->pin_.emplace<dbId<_dbBTerm>>(((_dbBTerm*) this)->getId());
}

void dbScanPin::setPin(dbITerm* iterm)
{
  _dbScanPin* scan_pin = (_dbScanPin*) this;
  scan_pin->pin_.emplace<dbId<_dbITerm>>(((_dbITerm*) this)->getId());
}

// User Code End dbScanPinPublicMethods
}  // namespace odb
   // Generator Code End Cpp
