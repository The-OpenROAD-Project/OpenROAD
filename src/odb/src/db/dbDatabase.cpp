// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbDatabase.h"

#include <cstdint>

#include "dbChip.h"
#include "dbChipBumpInst.h"
#include "dbChipConn.h"
#include "dbChipInst.h"
#include "dbChipNet.h"
#include "dbChipRegionInst.h"
#include "dbCore.h"
#include "dbProperty.h"
#include "dbTable.h"
#include "odb/db.h"
#include "odb/dbSet.h"
// User Code Begin Includes
#include <algorithm>
#include <atomic>
#include <cassert>
#include <cerrno>
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

#include "dbBTerm.h"
#include "dbBlock.h"
#include "dbCCSeg.h"
#include "dbCapNode.h"
#include "dbChipBumpInstItr.h"
#include "dbChipConnItr.h"
#include "dbChipInstItr.h"
#include "dbChipNetItr.h"
#include "dbChipRegionInstItr.h"
#include "dbCore.h"
#include "dbGDSLib.h"
#include "dbITerm.h"
#include "dbJournal.h"
#include "dbLib.h"
#include "dbNameCache.h"
#include "dbNet.h"
#include "dbPropertyItr.h"
#include "dbRSeg.h"
#include "dbTech.h"
#include "odb/dbBlockCallBackObj.h"
#include "odb/dbDatabaseObserver.h"
#include "odb/dbObject.h"
#include "odb/dbStream.h"
#include "utl/Logger.h"
// User Code End Includes
namespace odb {
template class dbTable<_dbDatabase>;
// User Code Begin Static
//
// Magic number is: ATHENADB
//
constexpr int kMagic1 = 0x41544845;  // ATHE
constexpr int kMagic2 = 0x4E414442;  // NADB

static dbTable<_dbDatabase>* db_tbl = nullptr;
// Must be held to access db_tbl
static std::mutex* db_tbl_mutex = new std::mutex;
static std::atomic<uint32_t> db_unique_id = 0;
// User Code End Static

bool _dbDatabase::operator==(const _dbDatabase& rhs) const
{
  if (master_id_ != rhs.master_id_) {
    return false;
  }
  if (chip_ != rhs.chip_) {
    return false;
  }
  if (dbu_per_micron_ != rhs.dbu_per_micron_) {
    return false;
  }
  if (*chip_tbl_ != *rhs.chip_tbl_) {
    return false;
  }
  if (chip_hash_ != rhs.chip_hash_) {
    return false;
  }
  if (*prop_tbl_ != *rhs.prop_tbl_) {
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
  if (*tech_tbl_ != *rhs.tech_tbl_) {
    return false;
  }

  if (*lib_tbl_ != *rhs.lib_tbl_) {
    return false;
  }
  if (*gds_lib_tbl_ != *rhs.gds_lib_tbl_) {
    return false;
  }

  if (*name_cache_ != *rhs.name_cache_) {
    return false;
  }
  // User Code End ==
  return true;
}

bool _dbDatabase::operator<(const _dbDatabase& rhs) const
{
  // User Code Begin <
  if (master_id_ >= rhs.master_id_) {
    return false;
  }
  if (chip_ >= rhs.chip_) {
    return false;
  }
  // User Code End <
  return true;
}

_dbDatabase::_dbDatabase(_dbDatabase* db)
{
  dbu_per_micron_ = 0;
  chip_tbl_ = new dbTable<_dbChip, 2>(
      this, this, (GetObjTbl_t) &_dbDatabase::getObjectTable, dbChipObj);
  chip_hash_.setTable(chip_tbl_);
  prop_tbl_ = new dbTable<_dbProperty>(
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
  magic1_ = kMagic1;
  magic2_ = kMagic2;
  schema_major_ = kSchemaMajor;
  schema_minor_ = kSchemaMinor;
  master_id_ = 0;
  logger_ = utl::Logger::defaultLogger();
  unique_id_ = db_unique_id++;
  hierarchy_ = false;

  gds_lib_tbl_ = new dbTable<_dbGDSLib, 2>(
      this, this, (GetObjTbl_t) &_dbDatabase::getObjectTable, dbGdsLibObj);

  tech_tbl_ = new dbTable<_dbTech, 2>(
      this, this, (GetObjTbl_t) &_dbDatabase::getObjectTable, dbTechObj);

  lib_tbl_ = new dbTable<_dbLib>(
      this, this, (GetObjTbl_t) &_dbDatabase::getObjectTable, dbLibObj);

  name_cache_ = new _dbNameCache(
      this, this, (GetObjTbl_t) &_dbDatabase::getObjectTable);

  prop_itr_ = new dbPropertyItr(prop_tbl_);

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
  stream >> obj.magic1_;

  if (obj.magic1_ != kMagic1) {
    throw std::runtime_error("database file is not an OpenDB Database");
  }

  stream >> obj.magic2_;

  if (obj.magic2_ != kMagic2) {
    throw std::runtime_error("database file is not an OpenDB Database");
  }

  stream >> obj.schema_major_;

  if (obj.schema_major_ != kSchemaMajor) {
    throw std::runtime_error("Incompatible database schema revision");
  }

  stream >> obj.schema_minor_;

  if (obj.schema_minor_ < kSchemaInitial) {
    throw std::runtime_error("incompatible database schema revision");
  }

  if (obj.schema_minor_ > kSchemaMinor) {
    throw std::runtime_error(
        fmt::format("incompatible database schema revision {}.{} > {}.{}",
                    obj.schema_major_,
                    obj.schema_minor_,
                    kSchemaMajor,
                    kSchemaMinor));
  }

  stream >> obj.master_id_;

  stream >> obj.chip_;

  dbId<_dbTech> old_db_tech;
  if (!obj.isSchema(kSchemaBlockTech)) {
    stream >> old_db_tech;
  }
  stream >> *obj.tech_tbl_;
  stream >> *obj.lib_tbl_;
  stream >> *obj.chip_tbl_;
  if (obj.isSchema(kSchemaGdsLibInBlock)) {
    stream >> *obj.gds_lib_tbl_;
  }
  stream >> *obj.prop_tbl_;
  stream >> *obj.name_cache_;
  if (obj.isSchema(kSchemaChipHashTable)) {
    stream >> obj.chip_hash_;
  }
  if (obj.isSchema(kSchemaChipInst)) {
    stream >> *obj.chip_inst_tbl_;
  }
  if (obj.isSchema(kSchemaChipRegion)) {
    stream >> *obj.chip_region_inst_tbl_;
  }
  if (obj.isSchema(kSchemaChipRegion)) {
    stream >> *obj.chip_conn_tbl_;
  }
  if (obj.isSchema(kSchemaChipBump)) {
    stream >> *obj.chip_bump_inst_tbl_;
  }
  if (obj.isSchema(kSchemaChipBump)) {
    stream >> *obj.chip_net_tbl_;
  }
  if (obj.isSchema(kSchemaDbuPerMicron)) {
    if (obj.isLessThanSchema(kSchemaRemoveDbuPerMicron)) {
      // Should already have a value from dbTech, so only need to update this if
      // its been set.
      uint32_t dbu_per_micron;
      stream >> dbu_per_micron;
      if (dbu_per_micron != 0) {
        obj.dbu_per_micron_ = dbu_per_micron;
      }
    } else {
      stream >> obj.dbu_per_micron_;
    }
  }
  if (obj.isSchema(kSchemaHierarchyFlag)) {
    stream >> obj.hierarchy_;
  } else {
    obj.hierarchy_ = false;
  }
  // Set the _tech on the block & libs now they are loaded
  if (!obj.isSchema(kSchemaBlockTech)) {
    if (obj.chip_) {
      _dbChip* chip = obj.chip_tbl_->getPtr(obj.chip_);
      chip->tech_ = old_db_tech;
    }

    auto db_public = (dbDatabase*) &obj;
    for (auto lib : db_public->getLibs()) {
      _dbLib* lib_impl = (_dbLib*) lib;
      lib_impl->tech_ = old_db_tech;
    }
  }

  // Fix up the owner id of properties of this db, this value changes.
  const uint32_t oid = obj.getId();

  for (_dbProperty* p : dbSet<_dbProperty>(&obj, obj.prop_tbl_)) {
    p->owner_ = oid;
  }

  // Set the revision of the database to the current revision
  obj.schema_major_ = kSchemaMajor;
  obj.schema_minor_ = kSchemaMinor;

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
  stream << obj.magic1_;
  stream << obj.magic2_;
  stream << obj.schema_major_;
  stream << obj.schema_minor_;
  stream << obj.master_id_;
  stream << obj.chip_;
  stream << *obj.tech_tbl_;
  stream << *obj.lib_tbl_;
  stream << *obj.chip_tbl_;
  stream << *obj.gds_lib_tbl_;
  stream << NamedTable("prop_tbl", obj.prop_tbl_);
  stream << *obj.name_cache_;
  stream << obj.chip_hash_;
  stream << *obj.chip_inst_tbl_;
  stream << *obj.chip_region_inst_tbl_;
  stream << *obj.chip_conn_tbl_;
  stream << *obj.chip_bump_inst_tbl_;
  stream << *obj.chip_net_tbl_;
  stream << obj.dbu_per_micron_;
  stream << obj.hierarchy_;
  // User Code End <<
  return stream;
}

dbObjectTable* _dbDatabase::getObjectTable(dbObjectType type)
{
  switch (type) {
    case dbChipObj:
      return chip_tbl_;
    case dbPropertyObj:
      return prop_tbl_;
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
      return tech_tbl_;

    case dbLibObj:
      return lib_tbl_;

    case dbGdsLibObj:
      return gds_lib_tbl_;
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

  chip_tbl_->collectMemInfo(info.children["chip_tbl_"]);

  prop_tbl_->collectMemInfo(info.children["prop_tbl_"]);

  chip_inst_tbl_->collectMemInfo(info.children["chip_inst_tbl_"]);

  chip_region_inst_tbl_->collectMemInfo(info.children["chip_region_inst_tbl_"]);

  chip_conn_tbl_->collectMemInfo(info.children["chip_conn_tbl_"]);

  chip_bump_inst_tbl_->collectMemInfo(info.children["chip_bump_inst_tbl_"]);

  chip_net_tbl_->collectMemInfo(info.children["chip_net_tbl_"]);

  // User Code Begin collectMemInfo
  tech_tbl_->collectMemInfo(info.children["tech"]);
  lib_tbl_->collectMemInfo(info.children["lib"]);
  gds_lib_tbl_->collectMemInfo(info.children["gds_lib"]);
  name_cache_->collectMemInfo(info.children["name_cache"]);
  // User Code End collectMemInfo
}

_dbDatabase::~_dbDatabase()
{
  delete chip_tbl_;
  delete prop_tbl_;
  delete chip_inst_tbl_;
  delete chip_region_inst_tbl_;
  delete chip_conn_tbl_;
  delete chip_bump_inst_tbl_;
  delete chip_net_tbl_;
  // User Code Begin Destructor
  delete tech_tbl_;
  delete lib_tbl_;
  delete gds_lib_tbl_;
  delete name_cache_;
  delete prop_itr_;
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
  magic1_ = kMagic1;
  magic2_ = kMagic2;
  schema_major_ = kSchemaMajor;
  schema_minor_ = kSchemaMinor;
  master_id_ = 0;
  logger_ = nullptr;
  unique_id_ = id;
  dbu_per_micron_ = 0;
  hierarchy_ = false;

  chip_tbl_ = new dbTable<_dbChip, 2>(
      this, this, (GetObjTbl_t) &_dbDatabase::getObjectTable, dbChipObj);

  gds_lib_tbl_ = new dbTable<_dbGDSLib, 2>(
      this, this, (GetObjTbl_t) &_dbDatabase::getObjectTable, dbGdsLibObj);

  tech_tbl_ = new dbTable<_dbTech, 2>(
      this, this, (GetObjTbl_t) &_dbDatabase::getObjectTable, dbTechObj);

  lib_tbl_ = new dbTable<_dbLib>(
      this, this, (GetObjTbl_t) &_dbDatabase::getObjectTable, dbLibObj);

  prop_tbl_ = new dbTable<_dbProperty>(
      this, this, (GetObjTbl_t) &_dbDatabase::getObjectTable, dbPropertyObj);

  name_cache_ = new _dbNameCache(
      this, this, (GetObjTbl_t) &_dbDatabase::getObjectTable);

  prop_itr_ = new dbPropertyItr(prop_tbl_);

  chip_inst_itr_ = new dbChipInstItr(chip_inst_tbl_);

  chip_region_inst_itr_ = new dbChipRegionInstItr(chip_region_inst_tbl_);

  chip_conn_itr_ = new dbChipConnItr(chip_conn_tbl_);

  chip_bump_inst_itr_ = new dbChipBumpInstItr(chip_bump_inst_tbl_);

  chip_net_itr_ = new dbChipNetItr(chip_net_tbl_);
}

utl::Logger* _dbDatabase::getLogger() const
{
  if (!logger_) {
    std::cerr << "[CRITICAL ODB-0001] No logger is installed in odb.\n";
    exit(1);
  }
  return logger_;
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

void dbDatabase::setDbuPerMicron(uint32_t dbu_per_micron)
{
  _dbDatabase* obj = (_dbDatabase*) this;

  obj->dbu_per_micron_ = dbu_per_micron;
}

uint32_t dbDatabase::getDbuPerMicron() const
{
  _dbDatabase* obj = (_dbDatabase*) this;
  return obj->dbu_per_micron_;
}

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
  return dbSet<dbProperty>(obj, obj->prop_tbl_);
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
  db->chip_ = chip->getImpl()->getOID();
}

dbSet<dbLib> dbDatabase::getLibs()
{
  _dbDatabase* db = (_dbDatabase*) this;
  return dbSet<dbLib>(db, db->lib_tbl_);
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
  return dbSet<dbTech>(db, db->tech_tbl_);
}

dbTech* dbDatabase::findTech(const char* name)
{
  for (auto tech : getTechs()) {
    auto tech_impl = (_dbTech*) tech;
    if (tech_impl->name_ == name) {
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
    auto masterIt = std::ranges::find(unused_masters, master);
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

uint32_t dbDatabase::getNumberOfMasters()
{
  _dbDatabase* db = (_dbDatabase*) this;
  return db->master_id_;
}

dbChip* dbDatabase::getChip()
{
  _dbDatabase* db = (_dbDatabase*) this;

  if (db->chip_ == 0) {
    return nullptr;
  }

  return (dbChip*) db->chip_tbl_->getPtr(db->chip_);
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
  impl->logger_->error(
      utl::ODB, 432, "getTech() is obsolete in a multi-tech db");
}

void dbDatabase::setHierarchy(bool value)
{
  _dbDatabase* db = reinterpret_cast<_dbDatabase*>(this);
  db->hierarchy_ = value;
}

bool dbDatabase::hasHierarchy() const
{
  const _dbDatabase* db = reinterpret_cast<const _dbDatabase*>(this);
  return db->hierarchy_;
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
  if (block->journal_) {
    endEco(block_);
  }
  block->journal_ = new dbJournal(block_);
  assert(block->journal_);
  debugPrint(block_->getImpl()->getLogger(),
             utl::ODB,
             "DB_ECO",
             2,
             "ECO: Started ECO #{}",
             block->journal_stack_.size());
}

void dbDatabase::endEco(dbBlock* block_)
{
  _dbBlock* block = (_dbBlock*) block_;
  assert(block->journal_);
  block->journal_stack_.push(block->journal_);
  block->journal_ = nullptr;
  debugPrint(block_->getImpl()->getLogger(),
             utl::ODB,
             "DB_ECO",
             2,
             "ECO: Ended ECO #{} (size {}) and pushed to ECO stack",
             block->journal_stack_.size() - 1,
             block->journal_stack_.top()->size());
}

void dbDatabase::commitEco(dbBlock* block_)
{
  _dbBlock* block = (_dbBlock*) block_;
  // Commit the current ECO or the last ECO into stack
  assert(block->journal_ || !block->journal_stack_.empty());
  if (!block->journal_) {
    block->journal_ = block->journal_stack_.top();
    block->journal_stack_.pop();
  }
  if (!block->journal_stack_.empty()) {
    dbJournal* prev_journal = block->journal_stack_.top();
    int old_size = prev_journal->size();
    prev_journal->append(block->journal_);
    debugPrint(block_->getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               2,
               "ECO: Merged ECO #{} (size {}) with ECO #{} (size {} -> {})",
               block->journal_stack_.size(),
               block->journal_->size(),
               block->journal_stack_.size() - 1,
               old_size,
               block->journal_stack_.top()->size());
  } else {
    debugPrint(block_->getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               2,
               "ECO: Committed ECO #{} (size {}) and removed from ECO stack",
               block->journal_stack_.size(),
               block->journal_->size());
  }
  delete block->journal_;
  block->journal_ = nullptr;
}

void dbDatabase::undoEco(dbBlock* block_)
{
  _dbBlock* block = (_dbBlock*) block_;
  assert(block->journal_ || !block->journal_stack_.empty());
  if (!block->journal_) {
    block->journal_ = block->journal_stack_.top();
    block->journal_stack_.pop();
  }
  debugPrint(block_->getImpl()->getLogger(),
             utl::ODB,
             "DB_ECO",
             2,
             "ECO: Undid ECO #{} (size {})",
             block->journal_stack_.size(),
             block->journal_->size());
  dbJournal* journal = block->journal_;
  block->journal_ = nullptr;
  journal->undo();
  delete journal;
}

bool dbDatabase::ecoEmpty(dbBlock* block_)
{
  _dbBlock* block = (_dbBlock*) block_;
  if (block->journal_) {
    return block->journal_->empty();
  }
  return false;
}

bool dbDatabase::ecoStackEmpty(dbBlock* block_)
{
  _dbBlock* block = (_dbBlock*) block_;
  return block->journal_stack_.empty();
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

  delete block->journal_;
  block->journal_ = nullptr;
  eco->redo();
  block->journal_ = eco;
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

  if (block->journal_) {
    dbOStream stream(block->getDatabase(), file);
    stream << *block->journal_;
  } else if (!block->journal_stack_.empty()) {
    dbOStream stream(block->getDatabase(), file);
    stream << *block->journal_stack_.top();
  }
}

void dbDatabase::setLogger(utl::Logger* logger)
{
  _dbDatabase* _db = (_dbDatabase*) this;
  _db->logger_ = logger;
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
  int id = db->unique_id_;
  db->~_dbDatabase();
  new (db) _dbDatabase(db, id);
}

void dbDatabase::destroy(dbDatabase* db_)
{
  std::lock_guard<std::mutex> lock(*db_tbl_mutex);
  _dbDatabase* db = (_dbDatabase*) db_;
  db_tbl->destroy(db);
}

dbDatabase* dbDatabase::getDatabase(uint32_t dbid)
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
        for (auto [name, child] : info.children) {
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
  block->setCoreArea(block->computeCoreArea());

  _dbDatabase* db = (_dbDatabase*) this;
  for (dbDatabaseObserver* observer : db->observers_) {
    if (floorplan) {
      observer->postReadFloorplanDef(block);
    } else {
      observer->postReadDef(block);
    }
  }
}

void dbDatabase::triggerPostRead3Dbx(dbChip* chip)
{
  _dbDatabase* db = (_dbDatabase*) this;
  for (dbDatabaseObserver* observer : db->observers_) {
    observer->postRead3Dbx(chip);
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
