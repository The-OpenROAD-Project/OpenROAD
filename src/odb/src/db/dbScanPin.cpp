// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbScanPin.h"

#include <variant>

#include "dbBlock.h"
#include "dbCore.h"
#include "dbDatabase.h"
#include "dbDft.h"
#include "dbTable.h"
#include "odb/db.h"
// User Code Begin Includes
#include <type_traits>
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

_dbScanPin::_dbScanPin(_dbDatabase* db)
{
}

dbIStream& operator>>(dbIStream& stream, _dbScanPin& obj)
{
  stream >> obj.pin_;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbScanPin& obj)
{
  stream << obj.pin_;
  return stream;
}

void _dbScanPin::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);
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
  const _dbDft* dft = (_dbDft*) scan_pin->getOwner();
  const _dbBlock* block = (_dbBlock*) dft->getOwner();

  return std::visit(
      [block](auto&& pin) {
        using T = std::decay_t<decltype(pin)>;
        if constexpr (std::is_same_v<T, dbId<_dbBTerm>>) {
          return std::variant<dbBTerm*, dbITerm*>(
              (dbBTerm*) block->bterm_tbl_->getPtr(pin));
        } else if constexpr (std::is_same_v<T, dbId<_dbITerm>>) {
          return std::variant<dbBTerm*, dbITerm*>(
              (dbITerm*) block->iterm_tbl_->getPtr(pin));
        } else {
          static_assert(always_false_v<T>, "non-exhaustive visitor!");
        }
      },
      scan_pin->pin_);
}

void dbScanPin::setPin(dbBTerm* bterm)
{
  _dbScanPin* scan_pin = (_dbScanPin*) this;
  scan_pin->pin_.emplace<dbId<_dbBTerm>>(((_dbBTerm*) bterm)->getId());
}

void dbScanPin::setPin(dbITerm* iterm)
{
  _dbScanPin* scan_pin = (_dbScanPin*) this;
  scan_pin->pin_.emplace<dbId<_dbITerm>>(((_dbITerm*) iterm)->getId());
}

dbId<dbScanPin> dbScanPin::create(dbDft* dft, dbBTerm* bterm)
{
  _dbDft* obj = (_dbDft*) dft;
  _dbScanPin* scan_pin = (_dbScanPin*) obj->scan_pins_->create();
  ((dbScanPin*) scan_pin)->setPin(bterm);
  return scan_pin->getId();
}

dbId<dbScanPin> dbScanPin::create(dbDft* dft, dbITerm* iterm)
{
  _dbDft* obj = (_dbDft*) dft;
  _dbScanPin* scan_pin = (_dbScanPin*) obj->scan_pins_->create();
  ((dbScanPin*) scan_pin)->setPin(iterm);
  return scan_pin->getId();
}

// User Code End dbScanPinPublicMethods
}  // namespace odb
   // Generator Code End Cpp
