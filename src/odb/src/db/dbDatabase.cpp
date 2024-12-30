///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2019, Nefelus Inc
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

#include "dbDatabase.h"

#include <algorithm>
#include <fstream>
#include <map>
#include <string>

#include "dbArrayTable.h"
#include "dbBTerm.h"
#include "dbBlock.h"
#include "dbCCSeg.h"
#include "dbCapNode.h"
#include "dbChip.h"
#include "dbGDSLib.h"
#include "dbITerm.h"
#include "dbJournal.h"
#include "dbLib.h"
#include "dbNameCache.h"
#include "dbNet.h"
#include "dbProperty.h"
#include "dbPropertyItr.h"
#include "dbRSeg.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "dbTech.h"
#include "dbWire.h"
#include "odb/db.h"
#include "odb/dbBlockCallBackObj.h"
#include "odb/dbExtControl.h"
#include "odb/dbStream.h"
#include "utl/Logger.h"

namespace odb {

//
// Magic number is: ATHENADB
//
constexpr int DB_MAGIC1 = 0x41544845;  // ATHE
constexpr int DB_MAGIC2 = 0x4E414442;  // NADB

template class dbTable<_dbDatabase>;

static dbTable<_dbDatabase>* db_tbl = nullptr;
// Must be held to access db_tbl
static std::mutex* db_tbl_mutex = new std::mutex;
static std::atomic<uint> db_unique_id = 0;

bool _dbDatabase::operator==(const _dbDatabase& rhs) const
{
  //
  // For the time being the fields,
  // magic1, magic2, schema_major, schema_minor,
  // unique_id, and file,
  // are not used for comparison.
  //
  if (_master_id != rhs._master_id) {
    return false;
  }

  if (_chip != rhs._chip) {
    return false;
  }

  if (*_tech_tbl != *rhs._tech_tbl) {
    return false;
  }

  if (*_lib_tbl != *rhs._lib_tbl) {
    return false;
  }

  if (*_chip_tbl != *rhs._chip_tbl) {
    return false;
  }

  if (*_gds_lib_tbl != *rhs._gds_lib_tbl) {
    return false;
  }

  if (*_prop_tbl != *rhs._prop_tbl) {
    return false;
  }

  if (*_name_cache != *rhs._name_cache) {
    return false;
  }

  return true;
}

void _dbDatabase::differences(dbDiff& diff,
                              const char* field,
                              const _dbDatabase& rhs) const
{
  DIFF_BEGIN
  DIFF_FIELD(_master_id);
  DIFF_FIELD(_chip);
  DIFF_TABLE_NO_DEEP(_tech_tbl);
  DIFF_TABLE_NO_DEEP(_lib_tbl);
  DIFF_TABLE_NO_DEEP(_chip_tbl);
  DIFF_TABLE_NO_DEEP(_gds_lib_tbl);
  DIFF_TABLE_NO_DEEP(_prop_tbl);
  DIFF_NAME_CACHE(_name_cache);
  DIFF_END
}

void _dbDatabase::out(dbDiff& diff, char side, const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(_master_id);
  DIFF_OUT_FIELD(_chip);
  DIFF_OUT_TABLE_NO_DEEP(_tech_tbl);
  DIFF_OUT_TABLE_NO_DEEP(_lib_tbl);
  DIFF_OUT_TABLE_NO_DEEP(_chip_tbl);
  DIFF_OUT_TABLE_NO_DEEP(_gds_lib_tbl);
  DIFF_OUT_TABLE_NO_DEEP(_prop_tbl);
  DIFF_OUT_NAME_CACHE(_name_cache);
  DIFF_END
}

dbObjectTable* _dbDatabase::getObjectTable(dbObjectType type)
{
  switch (type) {
    case dbTechObj:
      return _tech_tbl;

    case dbLibObj:
      return _lib_tbl;

    case dbChipObj:
      return _chip_tbl;

    case dbGdsLibObj:
      return _gds_lib_tbl;

    case dbPropertyObj:
      return _prop_tbl;
    default:
      getLogger()->critical(
          utl::ODB,
          438,
          "Internal inconsistency: no table found for type {}",
          type);
      break;
  }

  return nullptr;
}

////////////////////////////////////////////////////////////////////
//
// _dbDatabase - Methods
//
////////////////////////////////////////////////////////////////////

_dbDatabase::_dbDatabase(_dbDatabase* /* unused: db */)
{
  _magic1 = DB_MAGIC1;
  _magic2 = DB_MAGIC2;
  _schema_major = db_schema_major;
  _schema_minor = db_schema_minor;
  _master_id = 0;
  _logger = nullptr;
  _unique_id = db_unique_id++;

  _chip_tbl = new dbTable<_dbChip>(
      this, this, (GetObjTbl_t) &_dbDatabase::getObjectTable, dbChipObj, 2, 1);

  _gds_lib_tbl
      = new dbTable<_dbGDSLib>(this,
                               this,
                               (GetObjTbl_t) &_dbDatabase::getObjectTable,
                               dbGdsLibObj,
                               2,
                               1);

  _tech_tbl = new dbTable<_dbTech>(
      this, this, (GetObjTbl_t) &_dbDatabase::getObjectTable, dbTechObj, 2, 1);

  _lib_tbl = new dbTable<_dbLib>(
      this, this, (GetObjTbl_t) &_dbDatabase::getObjectTable, dbLibObj);

  _prop_tbl = new dbTable<_dbProperty>(
      this, this, (GetObjTbl_t) &_dbDatabase::getObjectTable, dbPropertyObj);

  _name_cache = new _dbNameCache(
      this, this, (GetObjTbl_t) &_dbDatabase::getObjectTable);

  _prop_itr = new dbPropertyItr(_prop_tbl);
}

//
// This constructor is use by dbDatabase::clear(), so the the unique-id is
// reset.
//
_dbDatabase::_dbDatabase(_dbDatabase* /* unused: db */, int id)
{
  _magic1 = DB_MAGIC1;
  _magic2 = DB_MAGIC2;
  _schema_major = db_schema_major;
  _schema_minor = db_schema_minor;
  _master_id = 0;
  _logger = nullptr;
  _unique_id = id;

  _chip_tbl = new dbTable<_dbChip>(
      this, this, (GetObjTbl_t) &_dbDatabase::getObjectTable, dbChipObj, 2, 1);

  _gds_lib_tbl
      = new dbTable<_dbGDSLib>(this,
                               this,
                               (GetObjTbl_t) &_dbDatabase::getObjectTable,
                               dbGdsLibObj,
                               2,
                               1);

  _tech_tbl = new dbTable<_dbTech>(
      this, this, (GetObjTbl_t) &_dbDatabase::getObjectTable, dbTechObj, 2, 1);

  _lib_tbl = new dbTable<_dbLib>(
      this, this, (GetObjTbl_t) &_dbDatabase::getObjectTable, dbLibObj);

  _prop_tbl = new dbTable<_dbProperty>(
      this, this, (GetObjTbl_t) &_dbDatabase::getObjectTable, dbPropertyObj);

  _name_cache = new _dbNameCache(
      this, this, (GetObjTbl_t) &_dbDatabase::getObjectTable);

  _prop_itr = new dbPropertyItr(_prop_tbl);
}

_dbDatabase::_dbDatabase(_dbDatabase* /* unused: db */, const _dbDatabase& d)
    : _magic1(d._magic1),
      _magic2(d._magic2),
      _schema_major(d._schema_major),
      _schema_minor(d._schema_minor),
      _master_id(d._master_id),
      _chip(d._chip),
      _unique_id(db_unique_id++),
      _logger(nullptr)
{
  _chip_tbl = new dbTable<_dbChip>(this, this, *d._chip_tbl);

  _gds_lib_tbl = new dbTable<_dbGDSLib>(this, this, *d._gds_lib_tbl);

  _tech_tbl = new dbTable<_dbTech>(this, this, *d._tech_tbl);

  _lib_tbl = new dbTable<_dbLib>(this, this, *d._lib_tbl);

  _prop_tbl = new dbTable<_dbProperty>(this, this, *d._prop_tbl);

  _name_cache = new _dbNameCache(this, this, *d._name_cache);

  _prop_itr = new dbPropertyItr(_prop_tbl);
}

_dbDatabase::~_dbDatabase()
{
  delete _tech_tbl;
  delete _lib_tbl;
  delete _chip_tbl;
  delete _gds_lib_tbl;
  delete _prop_tbl;
  delete _name_cache;
  delete _prop_itr;
}

dbOStream& operator<<(dbOStream& stream, const _dbDatabase& db)
{
  dbOStreamScope scope(stream, "dbDatabase");
  stream << db._magic1;
  stream << db._magic2;
  stream << db._schema_major;
  stream << db._schema_minor;
  stream << db._master_id;
  stream << db._chip;
  stream << *db._tech_tbl;
  stream << *db._lib_tbl;
  stream << *db._chip_tbl;
  stream << *db._gds_lib_tbl;
  stream << NamedTable("prop_tbl", db._prop_tbl);
  stream << *db._name_cache;
  stream << *db._gds_lib_tbl;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, _dbDatabase& db)
{
  stream >> db._magic1;

  if (db._magic1 != DB_MAGIC1) {
    throw ZException("database file is not an OpenDB Database");
  }

  stream >> db._magic2;

  if (db._magic2 != DB_MAGIC2) {
    throw ZException("database file is not an OpenDB Database");
  }

  stream >> db._schema_major;

  if (db._schema_major != db_schema_major) {
    throw ZException("Incompatible database schema revision");
  }

  stream >> db._schema_minor;

  if (db._schema_minor < db_schema_initial) {
    throw ZException("incompatible database schema revision");
  }

  if (db._schema_minor > db_schema_minor) {
    throw ZException("incompatible database schema revision %d.%d > %d.%d",
                     db._schema_major,
                     db._schema_minor,
                     db_schema_major,
                     db_schema_minor);
  }

  stream >> db._master_id;

  stream >> db._chip;

  dbId<_dbTech> old_db_tech;
  if (!db.isSchema(db_schema_block_tech)) {
    stream >> old_db_tech;
  }
  stream >> *db._tech_tbl;
  stream >> *db._lib_tbl;
  stream >> *db._chip_tbl;
  if (db.isSchema(db_schema_gds_lib_in_block)) {
    stream >> *db._gds_lib_tbl;
  }
  stream >> *db._prop_tbl;
  stream >> *db._name_cache;

  // Set the _tech on the block & libs now they are loaded
  if (!db.isSchema(db_schema_block_tech)) {
    if (db._chip) {
      _dbChip* chip = db._chip_tbl->getPtr(db._chip);
      if (chip->_top) {
        chip->_block_tbl->getPtr(chip->_top)->_tech = old_db_tech;
      }
    }

    auto db_public = (dbDatabase*) &db;
    for (auto lib : db_public->getLibs()) {
      _dbLib* lib_impl = (_dbLib*) lib;
      lib_impl->_tech = old_db_tech;
    }
  }

  // Fix up the owner id of properties of this db, this value changes.
  dbSet<_dbProperty> props(&db, db._prop_tbl);
  dbSet<_dbProperty>::iterator itr;
  uint oid = db.getId();

  for (itr = props.begin(); itr != props.end(); ++itr) {
    _dbProperty* p = *itr;
    p->_owner = oid;
  }

  // Set the revision of the database to the current revision
  db._schema_major = db_schema_major;
  db._schema_minor = db_schema_minor;
  return stream;
}

////////////////////////////////////////////////////////////////////
//
// dbDatabase - Methods
//
////////////////////////////////////////////////////////////////////

dbSet<dbLib> dbDatabase::getLibs()
{
  _dbDatabase* db = (_dbDatabase*) this;
  return dbSet<dbLib>(db, db->_lib_tbl);
}

dbLib* dbDatabase::findLib(const char* name)
{
  dbSet<dbLib> libs = getLibs();
  dbSet<dbLib>::iterator itr;

  for (itr = libs.begin(); itr != libs.end(); ++itr) {
    _dbLib* lib = (_dbLib*) *itr;

    if (strcmp(lib->_name, name) == 0) {
      return (dbLib*) lib;
    }
  }

  return nullptr;
}

dbSet<dbTech> dbDatabase::getTechs()
{
  _dbDatabase* db = (_dbDatabase*) this;
  return dbSet<dbTech>(db, db->_tech_tbl);
}

dbTech* dbDatabase::findTech(const char* name)
{
  for (auto tech : getTechs()) {
    auto tech_impl = (_dbTech*) tech;
    if (tech_impl->_name == name) {
      return tech;
    }
  }

  return nullptr;
}

dbMaster* dbDatabase::findMaster(const char* name)
{
  dbSet<dbLib> libs = getLibs();
  dbSet<dbLib>::iterator it;
  for (it = libs.begin(); it != libs.end(); it++) {
    dbLib* lib = *it;
    dbMaster* master = lib->findMaster(name);
    if (master) {
      return master;
    }
  }
  return nullptr;
}

// Remove unused masters
int dbDatabase::removeUnusedMasters()
{
  std::vector<dbMaster*> unused_masters;
  dbSet<dbLib> libs = getLibs();

  for (auto lib : libs) {
    dbSet<dbMaster> masters = lib->getMasters();
    // Collect all dbMasters for later comparision
    for (auto master : masters) {
      unused_masters.push_back(master);
    }
  }
  // Get instances from this Database
  dbChip* chip = getChip();
  dbBlock* block = chip->getBlock();
  dbSet<dbInst> insts = block->getInsts();

  for (auto inst : insts) {
    dbMaster* master = inst->getMaster();
    // Filter out the master that matches inst_master
    auto masterIt
        = std::find(unused_masters.begin(), unused_masters.end(), master);
    if (masterIt != unused_masters.end()) {
      // erase used maseters from container
      unused_masters.erase(masterIt);
    }
  }
  // Destroy remaining unused masters
  for (auto& elem : unused_masters) {
    dbMaster::destroy(elem);
  }
  return unused_masters.size();
}

dbSet<dbChip> dbDatabase::getChips()
{
  _dbDatabase* db = (_dbDatabase*) this;
  return dbSet<dbChip>(db, db->_chip_tbl);
}

uint dbDatabase::getNumberOfMasters()
{
  _dbDatabase* db = (_dbDatabase*) this;
  return db->_master_id;
}

dbChip* dbDatabase::getChip()
{
  _dbDatabase* db = (_dbDatabase*) this;

  if (db->_chip == 0) {
    return nullptr;
  }

  return (dbChip*) db->_chip_tbl->getPtr(db->_chip);
}

dbTech* dbDatabase::getTech()
{
  auto techs = getTechs();

  const int num_tech = techs.size();
  if (num_tech == 0) {
    return nullptr;
  }

  if (num_tech == 1) {
    return *techs.begin();
  }

  auto impl = (_dbDatabase*) this;
  impl->_logger->error(
      utl::ODB, 432, "getTech() is obsolete in a multi-tech db");
}

void dbDatabase::read(std::istream& file)
{
  _dbDatabase* db = (_dbDatabase*) this;
  dbIStream stream(db, file);
  stream >> *db;
}

void dbDatabase::write(std::ostream& file)
{
  _dbDatabase* db = (_dbDatabase*) this;
  dbOStream stream(db, file);
  stream << *db;
  file.flush();
}

void dbDatabase::beginEco(dbBlock* block_)
{
  _dbBlock* block = (_dbBlock*) block_;

  {
    delete block->_journal;
  }

  block->_journal = new dbJournal(block_);
  assert(block->_journal);
}

void dbDatabase::endEco(dbBlock* block_)
{
  _dbBlock* block = (_dbBlock*) block_;
  dbJournal* eco = block->_journal;
  block->_journal = nullptr;

  {
    delete block->_journal_pending;
  }

  block->_journal_pending = eco;
}

bool dbDatabase::ecoEmpty(dbBlock* block_)
{
  _dbBlock* block = (_dbBlock*) block_;

  if (block->_journal) {
    return block->_journal->empty();
  }

  return false;
}

int dbDatabase::checkEco(dbBlock* block_)
{
  _dbBlock* block = (_dbBlock*) block_;

  if (block->_journal) {
    return block->_journal->size();
  }
  return 0;
}

void dbDatabase::readEco(dbBlock* block_, const char* filename)
{
  _dbBlock* block = (_dbBlock*) block_;

  std::ifstream file;
  file.exceptions(std::ifstream::failbit | std::ifstream::badbit
                  | std::ios::eofbit);
  file.open(filename, std::ios::binary);

  dbIStream stream(block->getDatabase(), file);
  dbJournal* eco = new dbJournal(block_);
  assert(eco);
  stream >> *eco;

  {
    delete block->_journal_pending;
  }

  block->_journal_pending = eco;
}

void dbDatabase::writeEco(dbBlock* block_, const char* filename)
{
  _dbBlock* block = (_dbBlock*) block_;

  std::ofstream file(filename, std::ios::binary);
  if (!file) {
    int errnum = errno;
    block->getImpl()->getLogger()->error(
        utl::ODB, 2, "Error opening file {}", strerror(errnum));
    return;
  }

  file.exceptions(std::ifstream::failbit | std::ifstream::badbit
                  | std::ios::eofbit);

  if (block->_journal_pending) {
    dbOStream stream(block->getDatabase(), file);
    stream << *block->_journal_pending;
  }
}

void dbDatabase::commitEco(dbBlock* block_)
{
  _dbBlock* block = (_dbBlock*) block_;

  // TODO: Need a check to ensure the commit is not applied to the block of
  // which this eco was generated from.
  if (block->_journal_pending) {
    block->_journal_pending->redo();
    delete block->_journal_pending;
    block->_journal_pending = nullptr;
  }
}

void dbDatabase::undoEco(dbBlock* block_)
{
  _dbBlock* block = (_dbBlock*) block_;

  if (block->_journal_pending) {
    block->_journal_pending->undo();
    delete block->_journal_pending;
    block->_journal_pending = nullptr;
  }
}

void dbDatabase::setLogger(utl::Logger* logger)
{
  _dbDatabase* _db = (_dbDatabase*) this;
  _db->_logger = logger;
}

dbDatabase* dbDatabase::create()
{
  std::lock_guard<std::mutex> lock(*db_tbl_mutex);
  if (db_tbl == nullptr) {
    db_tbl = new dbTable<_dbDatabase>(
        nullptr, nullptr, (GetObjTbl_t) nullptr, dbDatabaseObj);
  }

  _dbDatabase* db = db_tbl->create();
  return (dbDatabase*) db;
}

void dbDatabase::clear()
{
  _dbDatabase* db = (_dbDatabase*) this;
  int id = db->_unique_id;
  db->~_dbDatabase();
  new (db) _dbDatabase(db, id);
}

void dbDatabase::destroy(dbDatabase* db_)
{
  std::lock_guard<std::mutex> lock(*db_tbl_mutex);
  _dbDatabase* db = (_dbDatabase*) db_;
  db_tbl->destroy(db);
}

dbDatabase* dbDatabase::duplicate(dbDatabase* db_)
{
  std::lock_guard<std::mutex> lock(*db_tbl_mutex);
  _dbDatabase* db = (_dbDatabase*) db_;
  _dbDatabase* d = db_tbl->duplicate(db);
  return (dbDatabase*) d;
}

dbDatabase* dbDatabase::getDatabase(uint dbid)
{
  std::lock_guard<std::mutex> lock(*db_tbl_mutex);
  return (dbDatabase*) db_tbl->getPtr(dbid);
}

dbDatabase* dbObject::getDb() const
{
  return (dbDatabase*) getImpl()->getDatabase();
}

utl::Logger* _dbDatabase::getLogger() const
{
  if (!_logger) {
    std::cerr << "[CRITICAL ODB-0001] No logger is installed in odb."
              << std::endl;
    exit(1);
  }
  return _logger;
}

utl::Logger* _dbObject::getLogger() const
{
  return getDatabase()->getLogger();
}

bool dbDatabase::diff(dbDatabase* db0_,
                      dbDatabase* db1_,
                      FILE* file,
                      int indent)
{
  _dbDatabase* db0 = (_dbDatabase*) db0_;
  _dbDatabase* db1 = (_dbDatabase*) db1_;
  dbDiff diff(file);
  diff.setIndentPerLevel(indent);
  db0->differences(diff, nullptr, *db1);
  return diff.hasDifferences();
}

}  // namespace odb
