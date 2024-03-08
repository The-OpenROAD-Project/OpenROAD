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
#include "dbScanChain.h"

#include "db.h"
#include "dbBlock.h"
#include "dbDatabase.h"
#include "dbDft.h"
#include "dbDiff.hpp"
#include "dbScanInst.h"
#include "dbScanPartition.h"
#include "dbScanPin.h"
#include "dbSet.h"
#include "dbTable.h"
#include "dbTable.hpp"
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
  if (*scan_partitions_ != *rhs.scan_partitions_) {
    return false;
  }
  if (*scan_insts_ != *rhs.scan_insts_) {
    return false;
  }

  return true;
}

bool _dbScanChain::operator<(const _dbScanChain& rhs) const
{
  return true;
}

void _dbScanChain::differences(dbDiff& diff,
                               const char* field,
                               const _dbScanChain& rhs) const
{
  DIFF_BEGIN
  DIFF_FIELD(name_);
  DIFF_FIELD(scan_in_);
  DIFF_FIELD(scan_out_);
  DIFF_FIELD(scan_enable_);
  DIFF_FIELD(test_mode_);
  DIFF_TABLE(scan_partitions_);
  DIFF_TABLE(scan_insts_);
  DIFF_END
}

void _dbScanChain::out(dbDiff& diff, char side, const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(name_);
  DIFF_OUT_FIELD(scan_in_);
  DIFF_OUT_FIELD(scan_out_);
  DIFF_OUT_FIELD(scan_enable_);
  DIFF_OUT_FIELD(test_mode_);
  DIFF_OUT_TABLE(scan_partitions_);
  DIFF_OUT_TABLE(scan_insts_);

  DIFF_END
}

_dbScanChain::_dbScanChain(_dbDatabase* db)
{
  scan_partitions_ = new dbTable<_dbScanPartition>(
      db,
      this,
      (GetObjTbl_t) &_dbScanChain::getObjectTable,
      dbScanPartitionObj);
  scan_insts_ = new dbTable<_dbScanInst>(
      db, this, (GetObjTbl_t) &_dbScanChain::getObjectTable, dbScanInstObj);
}

_dbScanChain::_dbScanChain(_dbDatabase* db, const _dbScanChain& r)
{
  name_ = r.name_;
  scan_in_ = r.scan_in_;
  scan_out_ = r.scan_out_;
  scan_enable_ = r.scan_enable_;
  test_mode_ = r.test_mode_;
  scan_partitions_
      = new dbTable<_dbScanPartition>(db, this, *r.scan_partitions_);
  scan_insts_ = new dbTable<_dbScanInst>(db, this, *r.scan_insts_);
}

dbIStream& operator>>(dbIStream& stream, _dbScanChain& obj)
{
  stream >> obj.name_;
  stream >> obj.scan_in_;
  stream >> obj.scan_out_;
  stream >> obj.scan_enable_;
  stream >> obj.test_mode_;
  stream >> *obj.scan_partitions_;
  stream >> *obj.scan_insts_;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbScanChain& obj)
{
  stream << obj.name_;
  stream << obj.scan_in_;
  stream << obj.scan_out_;
  stream << obj.scan_enable_;
  stream << obj.test_mode_;
  stream << *obj.scan_partitions_;
  stream << *obj.scan_insts_;
  return stream;
}

dbObjectTable* _dbScanChain::getObjectTable(dbObjectType type)
{
  switch (type) {
    case dbScanPartitionObj:
      return scan_partitions_;
    case dbScanInstObj:
      return scan_insts_;
    default:
      break;
  }
  return getTable()->getObjectTable(type);
}

_dbScanChain::~_dbScanChain()
{
  delete scan_partitions_;
  delete scan_insts_;
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

dbSet<dbScanInst> dbScanChain::getScanInsts() const
{
  _dbScanChain* obj = (_dbScanChain*) this;
  return dbSet<dbScanInst>(obj, obj->scan_insts_);
}

// User Code Begin dbScanChainPublicMethods

std::string_view dbScanChain::getName() const
{
  _dbScanChain* scan_chain = (_dbScanChain*) this;
  return scan_chain->name_;
}

void dbScanChain::setName(std::string_view name)
{
  _dbScanChain* scan_chain = (_dbScanChain*) this;
  scan_chain->name_ = name;
}

dbBTerm* _dbScanChain::getPin(_dbScanChain* scan_chain,
                              dbId<dbBTerm> _dbScanChain::*field)
{
  _dbDft* dft = (_dbDft*) scan_chain->getOwner();
  _dbBlock* block = (_dbBlock*) dft->getOwner();

  return (dbBTerm*) block->_bterm_tbl->getPtr(
      (dbId<_dbBTerm>) (scan_chain->*field));
}

void _dbScanChain::setPin(_dbScanChain* scan_chain,
                          dbId<dbBTerm> _dbScanChain::*field,
                          dbBTerm* pin)
{
  _dbBTerm* bterm = (_dbBTerm*) pin;
  scan_chain->*field = bterm->getId();
}

void dbScanChain::setScanIn(dbBTerm* scan_in)
{
  _dbScanChain::setPin((_dbScanChain*) this, &_dbScanChain::scan_in_, scan_in);
}

dbBTerm* dbScanChain::getScanIn() const
{
  return _dbScanChain::getPin((_dbScanChain*) this, &_dbScanChain::scan_in_);
}

void dbScanChain::setScanOut(dbBTerm* scan_out)
{
  _dbScanChain::setPin(
      (_dbScanChain*) this, &_dbScanChain::scan_out_, scan_out);
}

dbBTerm* dbScanChain::getScanOut() const
{
  return _dbScanChain::getPin((_dbScanChain*) this, &_dbScanChain::scan_out_);
}

void dbScanChain::setScanEnable(dbBTerm* scan_enable)
{
  _dbScanChain::setPin(
      (_dbScanChain*) this, &_dbScanChain::scan_enable_, scan_enable);
}

dbBTerm* dbScanChain::getScanEnable() const
{
  return _dbScanChain::getPin((_dbScanChain*) this,
                              &_dbScanChain::scan_enable_);
}

void dbScanChain::setTestMode(std::string_view test_mode)
{
  _dbScanChain* scan_chain = (_dbScanChain*) this;
  scan_chain->test_mode_ = test_mode;
}

std::string_view dbScanChain::getTestMode() const
{
  _dbScanChain* scan_chain = (_dbScanChain*) this;
  return scan_chain->test_mode_;
}

dbScanChain* dbScanChain::create(dbDft* dft)
{
  _dbDft* obj = (_dbDft*) dft;
  return (dbScanChain*) obj->scan_chains_->create();
}

// User Code End dbScanChainPublicMethods
}  // namespace odb
   // Generator Code End Cpp
