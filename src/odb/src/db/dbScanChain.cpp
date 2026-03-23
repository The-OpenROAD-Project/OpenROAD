// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbScanChain.h"

#include <string>

#include "dbBlock.h"
#include "dbCore.h"
#include "dbDatabase.h"
#include "dbDft.h"
#include "dbScanInst.h"
#include "dbScanPartition.h"
#include "dbScanPin.h"
#include "dbTable.h"
#include "odb/db.h"
#include "odb/dbSet.h"
// User Code Begin Includes
#include <string_view>
#include <variant>

#include "odb/dbObject.h"
// User Code End Includes
namespace odb {
template class dbTable<_dbScanChain>;

bool _dbScanChain::operator==(const _dbScanChain& rhs) const
{
  if (name_ != rhs.name_) {
    return false;
  }
  if (scan_in_ != rhs.scan_in_) {
    return false;
  }
  if (scan_out_ != rhs.scan_out_) {
    return false;
  }
  if (scan_enable_ != rhs.scan_enable_) {
    return false;
  }
  if (test_mode_ != rhs.test_mode_) {
    return false;
  }
  if (test_mode_name_ != rhs.test_mode_name_) {
    return false;
  }
  if (*scan_partitions_ != *rhs.scan_partitions_) {
    return false;
  }

  return true;
}

bool _dbScanChain::operator<(const _dbScanChain& rhs) const
{
  return true;
}

_dbScanChain::_dbScanChain(_dbDatabase* db)
{
  scan_partitions_ = new dbTable<_dbScanPartition>(
      db,
      this,
      (GetObjTbl_t) &_dbScanChain::getObjectTable,
      dbScanPartitionObj);
}

dbIStream& operator>>(dbIStream& stream, _dbScanChain& obj)
{
  stream >> obj.name_;
  stream >> obj.scan_in_;
  stream >> obj.scan_out_;
  stream >> obj.scan_enable_;
  stream >> obj.test_mode_;
  stream >> obj.test_mode_name_;
  stream >> *obj.scan_partitions_;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbScanChain& obj)
{
  stream << obj.name_;
  stream << obj.scan_in_;
  stream << obj.scan_out_;
  stream << obj.scan_enable_;
  stream << obj.test_mode_;
  stream << obj.test_mode_name_;
  stream << *obj.scan_partitions_;
  return stream;
}

dbObjectTable* _dbScanChain::getObjectTable(dbObjectType type)
{
  switch (type) {
    case dbScanPartitionObj:
      return scan_partitions_;
    default:
      break;
  }
  return getTable()->getObjectTable(type);
}
void _dbScanChain::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  scan_partitions_->collectMemInfo(info.children["scan_partitions_"]);
}

_dbScanChain::~_dbScanChain()
{
  delete scan_partitions_;
}

////////////////////////////////////////////////////////////////////
//
// dbScanChain - Methods
//
////////////////////////////////////////////////////////////////////

dbSet<dbScanPartition> dbScanChain::getScanPartitions() const
{
  _dbScanChain* obj = (_dbScanChain*) this;
  return dbSet<dbScanPartition>(obj, obj->scan_partitions_);
}

// User Code Begin dbScanChainPublicMethods

const std::string& dbScanChain::getName() const
{
  _dbScanChain* scan_chain = (_dbScanChain*) this;
  return scan_chain->name_;
}

void dbScanChain::setName(std::string_view name)
{
  _dbScanChain* scan_chain = (_dbScanChain*) this;
  scan_chain->name_ = name;
}

std::variant<dbBTerm*, dbITerm*> _dbScanChain::getPin(
    const dbId<dbScanPin>& scan_pin_id)
{
  _dbDft* dft = (_dbDft*) getOwner();
  return ((dbScanPin*) dft->scan_pins_->getPtr((dbId<_dbScanPin>) scan_pin_id))
      ->getPin();
}

void _dbScanChain::setPin(dbId<dbScanPin> _dbScanChain::*field, dbBTerm* pin)
{
  dbDft* dft = (dbDft*) getOwner();
  this->*field = dbScanPin::create(dft, pin);
}

void _dbScanChain::setPin(dbId<dbScanPin> _dbScanChain::*field, dbITerm* pin)
{
  dbDft* dft = (dbDft*) getOwner();
  this->*field = dbScanPin::create(dft, pin);
}

void dbScanChain::setScanIn(dbBTerm* scan_in)
{
  _dbScanChain* obj = (_dbScanChain*) this;
  obj->setPin(&_dbScanChain::scan_in_, scan_in);
}

void dbScanChain::setScanIn(dbITerm* scan_in)
{
  _dbScanChain* obj = (_dbScanChain*) this;
  obj->setPin(&_dbScanChain::scan_in_, scan_in);
}

std::variant<dbBTerm*, dbITerm*> dbScanChain::getScanIn() const
{
  _dbScanChain* obj = (_dbScanChain*) this;
  return obj->getPin(obj->scan_in_);
}

void dbScanChain::setScanOut(dbBTerm* scan_out)
{
  _dbScanChain* obj = (_dbScanChain*) this;
  obj->setPin(&_dbScanChain::scan_out_, scan_out);
}

void dbScanChain::setScanOut(dbITerm* scan_out)
{
  _dbScanChain* obj = (_dbScanChain*) this;
  obj->setPin(&_dbScanChain::scan_out_, scan_out);
}

std::variant<dbBTerm*, dbITerm*> dbScanChain::getScanOut() const
{
  _dbScanChain* obj = (_dbScanChain*) this;
  return obj->getPin(obj->scan_out_);
}

void dbScanChain::setScanEnable(dbBTerm* scan_enable)
{
  _dbScanChain* obj = (_dbScanChain*) this;
  obj->setPin(&_dbScanChain::scan_enable_, scan_enable);
}

void dbScanChain::setScanEnable(dbITerm* scan_enable)
{
  _dbScanChain* obj = (_dbScanChain*) this;
  obj->setPin(&_dbScanChain::scan_enable_, scan_enable);
}

std::variant<dbBTerm*, dbITerm*> dbScanChain::getScanEnable() const
{
  _dbScanChain* obj = (_dbScanChain*) this;
  return obj->getPin(obj->scan_enable_);
}

void dbScanChain::setTestMode(dbBTerm* test_mode)
{
  _dbScanChain* obj = (_dbScanChain*) this;
  obj->setPin(&_dbScanChain::test_mode_, test_mode);
}

void dbScanChain::setTestMode(dbITerm* test_mode)
{
  _dbScanChain* obj = (_dbScanChain*) this;
  obj->setPin(&_dbScanChain::test_mode_, test_mode);
}

std::variant<dbBTerm*, dbITerm*> dbScanChain::getTestMode() const
{
  _dbScanChain* obj = (_dbScanChain*) this;
  return obj->getPin(obj->test_mode_);
}

void dbScanChain::setTestModeName(const std::string& test_mode_name)
{
  _dbScanChain* scan_chain = (_dbScanChain*) this;
  scan_chain->test_mode_name_ = test_mode_name;
}

const std::string& dbScanChain::getTestModeName() const
{
  _dbScanChain* scan_chain = (_dbScanChain*) this;
  return scan_chain->test_mode_name_;
}

dbScanChain* dbScanChain::create(dbDft* dft)
{
  _dbDft* obj = (_dbDft*) dft;
  return (dbScanChain*) obj->scan_chains_->create();
}

// User Code End dbScanChainPublicMethods
}  // namespace odb
   // Generator Code End Cpp
