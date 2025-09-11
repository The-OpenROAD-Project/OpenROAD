// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbDatabase.h"

#include "dbChip.h"
#include "dbChipBumpInst.h"
#include "dbChipConn.h"
#include "dbChipInst.h"
#include "dbChipNet.h"
#include "dbChipRegionInst.h"
#include "dbProperty.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "odb/db.h"
#include "odb/dbSet.h"
// User Code Begin Includes
#include <algorithm>
#include <atomic>
#include <cassert>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <functional>
#include <ios>
#include <iostream>
#include <istream>
#include <map>
#include <mutex>
#include <ostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "dbArrayTable.h"
#include "dbBTerm.h"
#include "dbBlock.h"
#include "dbCCSeg.h"
#include "dbCapNode.h"
#include "dbChipBumpInstItr.h"
#include "dbChipConnItr.h"
#include "dbChipInstItr.h"
#include "dbChipNetItr.h"
#include "dbChipRegionInstItr.h"
#include "dbGDSLib.h"
#include "dbITerm.h"
#include "dbJournal.h"
#include "dbLib.h"
#include "dbNameCache.h"
#include "dbNet.h"
#include "dbProperty.h"
#include "dbPropertyItr.h"
#include "dbRSeg.h"
#include "dbTech.h"
#include "dbWire.h"
#include "odb/dbBlockCallBackObj.h"
#include "odb/dbExtControl.h"
#include "odb/dbStream.h"
#include "utl/Logger.h"
// User Code End Includes
namespace odb {
template class dbTable<_dbDatabase>;
// User Code Begin Static
//
// Magic number is: ATHENADB
//
constexpr int DB_MAGIC1 = 0x41544845;  // ATHE
constexpr int DB_MAGIC2 = 0x4E414442;  // NADB

static dbTable<_dbDatabase>* db_tbl = nullptr;
// Must be held to access db_tbl
static std::mutex* db_tbl_mutex = new std::mutex;
static std::atomic<uint> db_unique_id = 0;
// User Code End Static

bool _dbDatabase::operator==(const _dbDatabase& rhs) const
{
  if (_master_id != rhs._master_id) {
    return false;
  }
  if (_chip != rhs._chip) {
    return false;
  }
  if (*chip_tbl_ != *rhs.chip_tbl_) {
    return false;
  }
  if (chip_hash_ != rhs.chip_hash_) {
    return false;
  }
  if (*_prop_tbl != *rhs._prop_tbl) {
    return false;
  }
  if (*chip_inst_tbl_ != *rhs.chip_inst_tbl_) {
    return false;
  }
  if (*chip_region_inst_tbl_ != *rhs.chip_region_inst_tbl_) {
    return false;
  }
  if (*chip_conn_tbl_ != *rhs.chip_conn_tbl_) {
    return false;
  }
  if (*chip_bump_inst_tbl_ != *rhs.chip_bump_inst_tbl_) {
    return false;
  }
  if (*chip_net_tbl_ != *rhs.chip_net_tbl_) {
    return false;
  }

  // User Code Begin ==
  //
  // For the time being the fields,
  // magic1, magic2, schema_major, schema_minor,
  // unique_id, and file,
  // are not used for comparison.
  //
  if (*_tech_tbl != *rhs._tech_tbl) {
    return false;
  }

  if (*_lib_tbl != *rhs._lib_tbl) {
    return false;
  }
  if (*_gds_lib_tbl != *rhs._gds_lib_tbl) {
    return false;
  }

  if (*_name_cache != *rhs._name_cache) {
    return false;
  }
  // User Code End ==
  return true;
}

bool _dbDatabase::operator<(const _dbDatabase& rhs) const
{
  // User Code Begin <
  if (_master_id >= rhs._master_id) {
    return false;
  }
  if (_chip >= rhs._chip) {
    return false;
  }
  // User Code End <
  return true;
}

_dbDatabase::_dbDatabase(_dbDatabase* db)
{
  chip_tbl_ = new dbTable<_dbChip, 2>(
      this, this, (GetObjTbl_t) &_dbDatabase::getObjectTable, dbChipObj);
  chip_hash_.setTable(chip_tbl_);
  _prop_tbl = new dbTable<_dbProperty>(
      this, this, (GetObjTbl_t) &_dbDatabase::getObjectTable, dbPropertyObj);
  chip_inst_tbl_ = new dbTable<_dbChipInst>(
      this, this, (GetObjTbl_t) &_dbDatabase::getObjectTable, dbChipInstObj);
  chip_region_inst_tbl_ = new dbTable<_dbChipRegionInst>(
      this,
      this,
      (GetObjTbl_t) &_dbDatabase::getObjectTable,
      dbChipRegionInstObj);
  chip_conn_tbl_ = new dbTable<_dbChipConn>(
      this, this, (GetObjTbl_t) &_dbDatabase::getObjectTable, dbChipConnObj);
  chip_bump_inst_tbl_
      = new dbTable<_dbChipBumpInst>(this,
                                     this,
                                     (GetObjTbl_t) &_dbDatabase::getObjectTable,
                                     dbChipBumpInstObj);
  chip_net_tbl_ = new dbTable<_dbChipNet>(
      this, this, (GetObjTbl_t) &_dbDatabase::getObjectTable, dbChipNetObj);
  // User Code Begin Constructor
  _magic1 = DB_MAGIC1;
  _magic2 = DB_MAGIC2;
  _schema_major = db_schema_major;
  _schema_minor = db_schema_minor;
  _master_id = 0;
  _logger = nullptr;
  _unique_id = db_unique_id++;

  _gds_lib_tbl = new dbTable<_dbGDSLib, 2>(
      this, this, (GetObjTbl_t) &_dbDatabase::getObjectTable, dbGdsLibObj);

  _tech_tbl = new dbTable<_dbTech, 2>(
      this, this, (GetObjTbl_t) &_dbDatabase::getObjectTable, dbTechObj);

  _lib_tbl = new dbTable<_dbLib>(
      this, this, (GetObjTbl_t) &_dbDatabase::getObjectTable, dbLibObj);

  _name_cache = new _dbNameCache(
      this, this, (GetObjTbl_t) &_dbDatabase::getObjectTable);

  _prop_itr = new dbPropertyItr(_prop_tbl);

  chip_inst_itr_ = new dbChipInstItr(chip_inst_tbl_);

  chip_region_inst_itr_ = new dbChipRegionInstItr(chip_region_inst_tbl_);

  chip_conn_itr_ = new dbChipConnItr(chip_conn_tbl_);

  chip_bump_inst_itr_ = new dbChipBumpInstItr(chip_bump_inst_tbl_);

  chip_net_itr_ = new dbChipNetItr(chip_net_tbl_);
  // User Code End Constructor
}

dbIStream& operator>>(dbIStream& stream, _dbDatabase& obj)
{
  // User Code Begin >>
  stream >> obj._magic1;

  if (obj._magic1 != DB_MAGIC1) {
    throw std::runtime_error("database file is not an OpenDB Database");
  }

  stream >> obj._magic2;

  if (obj._magic2 != DB_MAGIC2) {
    throw std::runtime_error("database file is not an OpenDB Database");
  }

  stream >> obj._schema_major;

  if (obj._schema_major != db_schema_major) {
    throw std::runtime_error("Incompatible database schema revision");
  }

  stream >> obj._schema_minor;

  if (obj._schema_minor < db_schema_initial) {
    throw std::runtime_error("incompatible database schema revision");
  }

  if (obj._schema_minor > db_schema_minor) {
    throw std::runtime_error(
        fmt::format("incompatible database schema revision {}.{} > {}.{}",
                    obj._schema_major,
                    obj._schema_minor,
                    db_schema_major,
                    db_schema_minor));
  }

  stream >> obj._master_id;

  stream >> obj._chip;

  dbId<_dbTech> old_db_tech;
  if (!obj.isSchema(db_schema_block_tech)) {
    stream >> old_db_tech;
  }
  stream >> *obj._tech_tbl;
  stream >> *obj._lib_tbl;
  stream >> *obj.chip_tbl_;
  if (obj.isSchema(db_schema_gds_lib_in_block)) {
    stream >> *obj._gds_lib_tbl;
  }
  stream >> *obj._prop_tbl;
  stream >> *obj._name_cache;
  if (obj.isSchema(db_schema_chip_hash_table)) {
    stream >> obj.chip_hash_;
  }
  if (obj.isSchema(db_schema_chip_inst)) {
    stream >> *obj.chip_inst_tbl_;
  }
  if (obj.isSchema(db_schema_chip_region)) {
    stream >> *obj.chip_region_inst_tbl_;
  }
  if (obj.isSchema(db_schema_chip_region)) {
    stream >> *obj.chip_conn_tbl_;
  }
  if (obj.isSchema(db_schema_chip_bump)) {
    stream >> *obj.chip_bump_inst_tbl_;
  }
  if (obj.isSchema(db_schema_chip_bump)) {
    stream >> *obj.chip_net_tbl_;
  }
  // Set the _tech on the block & libs now they are loaded
  if (!obj.isSchema(db_schema_block_tech)) {
    if (obj._chip) {
      _dbChip* chip = obj.chip_tbl_->getPtr(obj._chip);
      chip->tech_ = old_db_tech;
    }

    auto db_public = (dbDatabase*) &obj;
    for (auto lib : db_public->getLibs()) {
      _dbLib* lib_impl = (_dbLib*) lib;
      lib_impl->_tech = old_db_tech;
    }
  }

  // Fix up the owner id of properties of this db, this value changes.
  const uint oid = obj.getId();

  for (_dbProperty* p : dbSet<_dbProperty>(&obj, obj._prop_tbl)) {
    p->_owner = oid;
  }

  // Set the revision of the database to the current revision
  obj._schema_major = db_schema_major;
  obj._schema_minor = db_schema_minor;

  // Set the chipinsts_map_ of the chip
  dbDatabase* db = (dbDatabase*) &obj;
  for (const auto& inst : db->getChipInsts()) {
    _dbChip* parent_chip = (_dbChip*) inst->getParentChip();
    parent_chip->chipinsts_map_[inst->getName()] = inst->getId();
  }
  // Set the region_insts_map_ of the chipinst
  for (const auto& chip_region_inst : db->getChipRegionInsts()) {
    _dbChipInst* chipinst = (_dbChipInst*) chip_region_inst->getChipInst();
    chipinst->region_insts_map_[chip_region_inst->getChipRegion()->getId()]
        = chip_region_inst->getId();
  }
  // User Code End >>
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbDatabase& obj)
{
  dbOStreamScope scope(stream, "dbDatabase");
  // User Code Begin <<
  stream << obj._magic1;
  stream << obj._magic2;
  stream << obj._schema_major;
  stream << obj._schema_minor;
  stream << obj._master_id;
  stream << obj._chip;
  stream << *obj._tech_tbl;
  stream << *obj._lib_tbl;
  stream << *obj.chip_tbl_;
  stream << *obj._gds_lib_tbl;
  stream << NamedTable("prop_tbl", obj._prop_tbl);
  stream << *obj._name_cache;
  stream << obj.chip_hash_;
  stream << *obj.chip_inst_tbl_;
  stream << *obj.chip_region_inst_tbl_;
  stream << *obj.chip_conn_tbl_;
  stream << *obj.chip_bump_inst_tbl_;
  stream << *obj.chip_net_tbl_;
  // User Code End <<
  return stream;
}

dbObjectTable* _dbDatabase::getObjectTable(dbObjectType type)
{
  switch (type) {
    case dbChipObj:
      return chip_tbl_;
    case dbPropertyObj:
      return _prop_tbl;
    case dbChipInstObj:
      return chip_inst_tbl_;
    case dbChipRegionInstObj:
      return chip_region_inst_tbl_;
    case dbChipConnObj:
      return chip_conn_tbl_;
    case dbChipBumpInstObj:
      return chip_bump_inst_tbl_;
    case dbChipNetObj:
      return chip_net_tbl_;
      // User Code Begin getObjectTable
    case dbTechObj:
      return _tech_tbl;

    case dbLibObj:
      return _lib_tbl;

    case dbGdsLibObj:
      return _gds_lib_tbl;
    // User Code End getObjectTable
    default:
      break;
  }
  return getTable()->getObjectTable(type);
}
void _dbDatabase::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  chip_tbl_->collectMemInfo(info.children_["chip_tbl_"]);

  _prop_tbl->collectMemInfo(info.children_["_prop_tbl"]);

  chip_inst_tbl_->collectMemInfo(info.children_["chip_inst_tbl_"]);

  chip_region_inst_tbl_->collectMemInfo(
      info.children_["chip_region_inst_tbl_"]);

  chip_conn_tbl_->collectMemInfo(info.children_["chip_conn_tbl_"]);

  chip_bump_inst_tbl_->collectMemInfo(info.children_["chip_bump_inst_tbl_"]);

  chip_net_tbl_->collectMemInfo(info.children_["chip_net_tbl_"]);

  // User Code Begin collectMemInfo
  _tech_tbl->collectMemInfo(info.children_["tech"]);
  _lib_tbl->collectMemInfo(info.children_["lib"]);
  _gds_lib_tbl->collectMemInfo(info.children_["gds_lib"]);
  _name_cache->collectMemInfo(info.children_["name_cache"]);
  // User Code End collectMemInfo
}

_dbDatabase::~_dbDatabase()
{
  delete chip_tbl_;
  delete _prop_tbl;
  delete chip_inst_tbl_;
  delete chip_region_inst_tbl_;
  delete chip_conn_tbl_;
  delete chip_bump_inst_tbl_;
  delete chip_net_tbl_;
  // User Code Begin Destructor
  delete _tech_tbl;
  delete _lib_tbl;
  delete _gds_lib_tbl;
  delete _name_cache;
  delete _prop_itr;
  delete chip_inst_itr_;
  delete chip_region_inst_itr_;
  delete chip_conn_itr_;
  delete chip_bump_inst_itr_;
  delete chip_net_itr_;
  // User Code End Destructor
}

// User Code Begin PrivateMethods
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

  chip_tbl_ = new dbTable<_dbChip, 2>(
      this, this, (GetObjTbl_t) &_dbDatabase::getObjectTable, dbChipObj);

  _gds_lib_tbl = new dbTable<_dbGDSLib, 2>(
      this, this, (GetObjTbl_t) &_dbDatabase::getObjectTable, dbGdsLibObj);

  _tech_tbl = new dbTable<_dbTech, 2>(
      this, this, (GetObjTbl_t) &_dbDatabase::getObjectTable, dbTechObj);

  _lib_tbl = new dbTable<_dbLib>(
      this, this, (GetObjTbl_t) &_dbDatabase::getObjectTable, dbLibObj);

  _prop_tbl = new dbTable<_dbProperty>(
      this, this, (GetObjTbl_t) &_dbDatabase::getObjectTable, dbPropertyObj);

  _name_cache = new _dbNameCache(
      this, this, (GetObjTbl_t) &_dbDatabase::getObjectTable);

  _prop_itr = new dbPropertyItr(_prop_tbl);

  chip_inst_itr_ = new dbChipInstItr(chip_inst_tbl_);

  chip_region_inst_itr_ = new dbChipRegionInstItr(chip_region_inst_tbl_);

  chip_conn_itr_ = new dbChipConnItr(chip_conn_tbl_);

  chip_bump_inst_itr_ = new dbChipBumpInstItr(chip_bump_inst_tbl_);

  chip_net_itr_ = new dbChipNetItr(chip_net_tbl_);
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

// User Code End PrivateMethods

////////////////////////////////////////////////////////////////////
//
// dbDatabase - Methods
//
////////////////////////////////////////////////////////////////////

dbSet<dbChip> dbDatabase::getChips() const
{
  _dbDatabase* obj = (_dbDatabase*) this;
  return dbSet<dbChip>(obj, obj->chip_tbl_);
}

dbChip* dbDatabase::findChip(const char* name) const
{
  _dbDatabase* obj = (_dbDatabase*) this;
  return (dbChip*) obj->chip_hash_.find(name);
}

dbSet<dbProperty> dbDatabase::getProperties() const
{
  _dbDatabase* obj = (_dbDatabase*) this;
  return dbSet<dbProperty>(obj, obj->_prop_tbl);
}

dbSet<dbChipInst> dbDatabase::getChipInsts() const
{
  _dbDatabase* obj = (_dbDatabase*) this;
  return dbSet<dbChipInst>(obj, obj->chip_inst_tbl_);
}

dbSet<dbChipRegionInst> dbDatabase::getChipRegionInsts() const
{
  _dbDatabase* obj = (_dbDatabase*) this;
  return dbSet<dbChipRegionInst>(obj, obj->chip_region_inst_tbl_);
}

dbSet<dbChipConn> dbDatabase::getChipConns() const
{
  _dbDatabase* obj = (_dbDatabase*) this;
  return dbSet<dbChipConn>(obj, obj->chip_conn_tbl_);
}

dbSet<dbChipBumpInst> dbDatabase::getChipBumpInsts() const
{
  _dbDatabase* obj = (_dbDatabase*) this;
  return dbSet<dbChipBumpInst>(obj, obj->chip_bump_inst_tbl_);
}

dbSet<dbChipNet> dbDatabase::getChipNets() const
{
  _dbDatabase* obj = (_dbDatabase*) this;
  return dbSet<dbChipNet>(obj, obj->chip_net_tbl_);
}

// User Code Begin dbDatabasePublicMethods

void dbDatabase::setTopChip(dbChip* chip)
{
  _dbDatabase* db = (_dbDatabase*) this;
  db->_chip = chip->getImpl()->getOID();
}

dbSet<dbLib> dbDatabase::getLibs()
{
  _dbDatabase* db = (_dbDatabase*) this;
  return dbSet<dbLib>(db, db->_lib_tbl);
}

dbLib* dbDatabase::findLib(const char* name)
{
  for (dbLib* lib : getLibs()) {
    if (strcmp(lib->getConstName(), name) == 0) {
      return lib;
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
  for (dbLib* lib : getLibs()) {
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

  return (dbChip*) db->chip_tbl_->getPtr(db->_chip);
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
  ((dbDatabase*) db)->triggerPostReadDb();
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

dbDatabase* dbDatabase::getDatabase(uint dbid)
{
  std::lock_guard<std::mutex> lock(*db_tbl_mutex);
  return (dbDatabase*) db_tbl->getPtr(dbid);
}

dbDatabase* dbObject::getDb() const
{
  return (dbDatabase*) getImpl()->getDatabase();
}

void dbDatabase::report()
{
  _dbDatabase* db = (_dbDatabase*) this;
  MemInfo root;
  db->collectMemInfo(root);
  utl::Logger* logger = db->getLogger();
  std::function<int64_t(MemInfo&, const std::string&, int)> print =
      [&](MemInfo& info, const std::string& name, int depth) {
        double avg_size = 0;
        int64_t total_size = info.size;
        if (info.cnt > 0) {
          avg_size = info.size / static_cast<double>(info.cnt);
        }

        logger->report("{:40s} cnt={:10d} size={:12d} (avg elem={:12.1f})",
                       name.c_str(),
                       info.cnt,
                       info.size,
                       avg_size);
        for (auto [name, child] : info.children_) {
          total_size += print(child, std::string(depth, ' ') + name, depth + 1);
        }
        return total_size;
      };
  auto total_size = print(root, "dbDatabase", 1);
  logger->report("Total size = {}", total_size);
}

void dbDatabase::addObserver(dbDatabaseObserver* observer)
{
  observer->setUnregisterObserver(
      [this, observer] { removeObserver(observer); });
  _dbDatabase* db = (_dbDatabase*) this;
  db->observers_.insert(observer);
}

void dbDatabase::removeObserver(dbDatabaseObserver* observer)
{
  observer->setUnregisterObserver(nullptr);
  _dbDatabase* db = (_dbDatabase*) this;
  db->observers_.erase(observer);
}

void dbDatabase::triggerPostReadLef(dbTech* tech, dbLib* library)
{
  _dbDatabase* db = (_dbDatabase*) this;
  for (dbDatabaseObserver* observer : db->observers_) {
    observer->postReadLef(tech, library);
  }
}

void dbDatabase::triggerPostReadDef(dbBlock* block, const bool floorplan)
{
  _dbDatabase* db = (_dbDatabase*) this;
  for (dbDatabaseObserver* observer : db->observers_) {
    if (floorplan) {
      observer->postReadFloorplanDef(block);
    } else {
      observer->postReadDef(block);
    }
  }
}

void dbDatabase::triggerPostReadDb()
{
  _dbDatabase* db = (_dbDatabase*) this;
  for (dbDatabaseObserver* observer : db->observers_) {
    observer->postReadDb(this);
  }
}

// User Code End dbDatabasePublicMethods
}  // namespace odb
// Generator Code End Cpp