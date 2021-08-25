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
#include <map>
#include <string>

#include "db.h"
#include "dbArrayTable.h"
#include "dbBTerm.h"
#include "dbBlock.h"
#include "dbBlockCallBackObj.h"
#include "dbCCSeg.h"
#include "dbCapNode.h"
#include "dbChip.h"
#include "dbExtControl.h"
#include "dbITerm.h"
#include "dbJournal.h"
#include "dbLib.h"
#include "dbNameCache.h"
#include "dbNet.h"
#include "dbProperty.h"
#include "dbPropertyItr.h"
#include "dbRSeg.h"
#include "dbStream.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "dbTech.h"
#include "dbWire.h"
#include "utl/Logger.h"

namespace odb {

//
// Magic number is: ATHENADB
//
#define ADS_DB_MAGIC1 0x41544845  // ATHE
#define ADS_DB_MAGIC2 0x4E414442  // NADB

template class dbTable<_dbDatabase>;

static dbTable<_dbDatabase>* db_tbl = NULL;
static uint db_unique_id = 0;

bool _dbDatabase::operator==(const _dbDatabase& rhs) const
{
  //
  // For the time being the fields,
  // magic1, magic2, schema_major, schema_minor,
  // unique_id, and file,
  // are not used for comparison.
  //
  if (_master_id != rhs._master_id)
    return false;

  if (_chip != rhs._chip)
    return false;

  if (_tech != rhs._tech)
    return false;

  if (*_tech_tbl != *rhs._tech_tbl)
    return false;

  if (*_lib_tbl != *rhs._lib_tbl)
    return false;

  if (*_chip_tbl != *rhs._chip_tbl)
    return false;

  if (*_prop_tbl != *rhs._prop_tbl)
    return false;

  if (*_name_cache != *rhs._name_cache)
    return false;

  return true;
}

void _dbDatabase::differences(dbDiff& diff,
                              const char* field,
                              const _dbDatabase& rhs) const
{
  DIFF_BEGIN
  DIFF_FIELD(_master_id);
  DIFF_FIELD(_chip);
  DIFF_FIELD(_tech);
  DIFF_TABLE_NO_DEEP(_tech_tbl);
  DIFF_TABLE_NO_DEEP(_lib_tbl);
  DIFF_TABLE_NO_DEEP(_chip_tbl);
  DIFF_TABLE_NO_DEEP(_prop_tbl);
  DIFF_NAME_CACHE(_name_cache);
  DIFF_END
}

void _dbDatabase::out(dbDiff& diff, char side, const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(_master_id);
  DIFF_OUT_FIELD(_chip);
  DIFF_OUT_FIELD(_tech);
  DIFF_OUT_TABLE_NO_DEEP(_tech_tbl);
  DIFF_OUT_TABLE_NO_DEEP(_lib_tbl);
  DIFF_OUT_TABLE_NO_DEEP(_chip_tbl);
  DIFF_OUT_TABLE_NO_DEEP(_prop_tbl);
  DIFF_OUT_NAME_CACHE(_name_cache);
  DIFF_END
}

dbObjectTable* _dbDatabase::getObjectTable(dbObjectType type)
{
  switch (type) {
    case dbDatabaseObj:
      return db_tbl;

    case dbTechObj:
      return _tech_tbl;

    case dbLibObj:
      return _lib_tbl;

    case dbChipObj:
      return _chip_tbl;

    case dbPropertyObj:
      return _prop_tbl;
    default:
      break;
  }

  return NULL;
}

////////////////////////////////////////////////////////////////////
//
// _dbDatabase - Methods
//
////////////////////////////////////////////////////////////////////

_dbDatabase::_dbDatabase(_dbDatabase* /* unused: db */)
{
  _magic1 = ADS_DB_MAGIC1;
  _magic2 = ADS_DB_MAGIC2;
  _schema_major = db_schema_major;
  _schema_minor = db_schema_minor;
  _master_id = 0;
  _file = NULL;
  _logger = nullptr;
  _unique_id = db_unique_id++;

  _chip_tbl = new dbTable<_dbChip>(
      this, this, (GetObjTbl_t) &_dbDatabase::getObjectTable, dbChipObj, 2, 1);
  ZALLOCATED(_chip_tbl);

  _tech_tbl = new dbTable<_dbTech>(
      this, this, (GetObjTbl_t) &_dbDatabase::getObjectTable, dbTechObj, 2, 1);
  ZALLOCATED(_tech_tbl);

  _lib_tbl = new dbTable<_dbLib>(
      this, this, (GetObjTbl_t) &_dbDatabase::getObjectTable, dbLibObj);
  ZALLOCATED(_lib_tbl);

  _prop_tbl = new dbTable<_dbProperty>(
      this, this, (GetObjTbl_t) &_dbDatabase::getObjectTable, dbPropertyObj);
  ZALLOCATED(_prop_tbl);

  _name_cache = new _dbNameCache(
      this, this, (GetObjTbl_t) &_dbDatabase::getObjectTable);
  ZALLOCATED(_name_cache);

  _prop_itr = new dbPropertyItr(_prop_tbl);
  ZALLOCATED(_prop_itr);
}

//
// This constructor is use by dbDatabase::clear(), so the the unique-id is
// reset.
//
_dbDatabase::_dbDatabase(_dbDatabase* /* unused: db */, int id)
{
  _magic1 = ADS_DB_MAGIC1;
  _magic2 = ADS_DB_MAGIC2;
  _schema_major = db_schema_major;
  _schema_minor = db_schema_minor;
  _master_id = 0;
  _file = NULL;
  _logger = nullptr;
  _unique_id = id;

  _chip_tbl = new dbTable<_dbChip>(
      this, this, (GetObjTbl_t) &_dbDatabase::getObjectTable, dbChipObj, 2, 1);
  ZALLOCATED(_chip_tbl);

  _tech_tbl = new dbTable<_dbTech>(
      this, this, (GetObjTbl_t) &_dbDatabase::getObjectTable, dbTechObj, 2, 1);
  ZALLOCATED(_tech_tbl);

  _lib_tbl = new dbTable<_dbLib>(
      this, this, (GetObjTbl_t) &_dbDatabase::getObjectTable, dbLibObj);
  ZALLOCATED(_lib_tbl);

  _prop_tbl = new dbTable<_dbProperty>(
      this, this, (GetObjTbl_t) &_dbDatabase::getObjectTable, dbPropertyObj);
  ZALLOCATED(_prop_tbl);

  _name_cache = new _dbNameCache(
      this, this, (GetObjTbl_t) &_dbDatabase::getObjectTable);
  ZALLOCATED(_name_cache);

  _prop_itr = new dbPropertyItr(_prop_tbl);
  ZALLOCATED(_prop_itr);
}

_dbDatabase::_dbDatabase(_dbDatabase* /* unused: db */, const _dbDatabase& d)
    : _magic1(d._magic1),
      _magic2(d._magic2),
      _schema_major(d._schema_major),
      _schema_minor(d._schema_minor),
      _master_id(d._master_id),
      _chip(d._chip),
      _tech(d._tech),
      _unique_id(db_unique_id++),
      _file(NULL),
      _logger(nullptr)
{
  if (d._file) {
    _file = strdup(d._file);
    ZALLOCATED(_file);
  }

  _chip_tbl = new dbTable<_dbChip>(this, this, *d._chip_tbl);
  ZALLOCATED(_chip_tbl);

  _tech_tbl = new dbTable<_dbTech>(this, this, *d._tech_tbl);
  ZALLOCATED(_tech_tbl);

  _lib_tbl = new dbTable<_dbLib>(this, this, *d._lib_tbl);
  ZALLOCATED(_lib_tbl);

  _prop_tbl = new dbTable<_dbProperty>(this, this, *d._prop_tbl);
  ZALLOCATED(_prop_tbl);

  _name_cache = new _dbNameCache(this, this, *d._name_cache);
  ZALLOCATED(_name_cache);

  _prop_itr = new dbPropertyItr(_prop_tbl);
  ZALLOCATED(_prop_itr);
}

_dbDatabase::~_dbDatabase()
{
  delete _tech_tbl;
  delete _lib_tbl;
  delete _chip_tbl;
  delete _prop_tbl;
  delete _name_cache;
  // dimitri_fix
  // delete _prop_itr;

  if (_file)
    free(_file);
}

dbOStream& operator<<(dbOStream& stream, const _dbDatabase& db)
{
  stream << db._magic1;
  stream << db._magic2;
  stream << db._schema_major;
  stream << db._schema_minor;
  stream << db._master_id;
  stream << db._chip;
  stream << db._tech;
  stream << *db._tech_tbl;
  stream << *db._lib_tbl;
  stream << *db._chip_tbl;
  stream << *db._prop_tbl;
  stream << *db._name_cache;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, _dbDatabase& db)
{
  stream >> db._magic1;

  if (db._magic1 != ADS_DB_MAGIC1)
    throw ZException("database file is not an OpenDB Database");

  stream >> db._magic2;

  if (db._magic2 != ADS_DB_MAGIC2)
    throw ZException("database file is not an OpenDB Database");

  stream >> db._schema_major;

  if (db._schema_major != db_schema_major)
    throw ZException("Incompatible database schema revision");

  stream >> db._schema_minor;

  if (db._schema_minor < db_schema_initial)
    throw ZException("incompatible database schema revision");

  stream >> db._master_id;

  stream >> db._chip;
  stream >> db._tech;
  stream >> *db._tech_tbl;
  stream >> *db._lib_tbl;
  stream >> *db._chip_tbl;
  stream >> *db._prop_tbl;
  stream >> *db._name_cache;

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

    if (strcmp(lib->_name, name) == 0)
      return (dbLib*) lib;
  }

  return NULL;
}

dbMaster* dbDatabase::findMaster(const char* name)
{
  dbSet<dbLib> libs = getLibs();
  dbSet<dbLib>::iterator it;
  for (it = libs.begin(); it != libs.end(); it++) {
    dbLib* lib = *it;
    dbMaster* master = lib->findMaster(name);
    if (master)
      return master;
  }
  return NULL;
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

  if (db->_chip == 0)
    return NULL;

  return (dbChip*) db->_chip_tbl->getPtr(db->_chip);
}

dbTech* dbDatabase::getTech()
{
  _dbDatabase* db = (_dbDatabase*) this;

  if (db->_tech == 0)
    return NULL;

  return (dbTech*) db->_tech_tbl->getPtr(db->_tech);
}

void dbDatabase::read(FILE* file)
{
  _dbDatabase* db = (_dbDatabase*) this;
  dbIStream stream(db, file);
  stream >> *db;
}

void dbDatabase::readTech(FILE* file)
{
  _dbDatabase* db = (_dbDatabase*) this;
  _dbTech* tech = (_dbTech*) getTech();

  if (tech == NULL)
    tech = (_dbTech*) dbTech::create(this);
  else {
    tech->~_dbTech();
    new (tech) _dbTech(db);
  }

  dbIStream stream(db, file);
  stream >> *tech;
}

void dbDatabase::readLib(FILE* file, dbLib* lib)
{
  _dbDatabase* db = (_dbDatabase*) this;
  _dbLib* l = (_dbLib*) lib;

  l->~_dbLib();
  new (l) _dbLib(db);

  dbIStream stream(db, file);
  stream >> *l;
}

void dbDatabase::readLibs(FILE* file)
{
  _dbDatabase* db = (_dbDatabase*) this;
  dbIStream stream(db, file);
  stream >> *db->_lib_tbl;
}

void dbDatabase::readBlock(FILE* file, dbBlock* block)
{
  _dbDatabase* db = (_dbDatabase*) this;
  _dbBlock* b = (_dbBlock*) block;
  b->~_dbBlock();
  new (b) _dbBlock(db);
  dbIStream stream(db, file);
  stream >> *b;
}

void dbDatabase::readNets(FILE* file, dbBlock* block)
{
  _dbDatabase* db = (_dbDatabase*) this;
  dbIStream stream(db, file);

  std::list<dbBlockCallBackObj*>* cbs = &(((_dbBlock*) block)->_callbacks);
  std::list<dbBlockCallBackObj*>::const_iterator cbitr;
  for (cbitr = cbs->begin(); cbitr != cbs->end(); ++cbitr)
    (**cbitr)().inDbBlockReadNetsBefore(
        block);  // client ECO optimzation - payam

  stream >> *((_dbBlock*) block)->_net_tbl;
}

void dbDatabase::readWires(FILE* file, dbBlock* block)
{
  _dbDatabase* db = (_dbDatabase*) this;
  dbIStream stream(db, file);
  stream >> *((_dbBlock*) block)->_wire_tbl;
}

void dbDatabase::readParasitics(FILE* file, dbBlock* block)
{
  _dbDatabase* db = (_dbDatabase*) this;
  dbIStream stream(db, file);
  stream >> ((_dbBlock*) block)->_num_ext_corners;
  stream >> ((_dbBlock*) block)->_corner_name_list;
  stream >> *((_dbBlock*) block)->_r_val_tbl;
  stream >> *((_dbBlock*) block)->_c_val_tbl;
  stream >> *((_dbBlock*) block)->_cc_val_tbl;
  stream >> *((_dbBlock*) block)->_cap_node_tbl;
  stream >> *((_dbBlock*) block)->_r_seg_tbl;
  stream >> *((_dbBlock*) block)->_cc_seg_tbl;
  stream >> *((_dbBlock*) block)->_extControl;

  ((_dbBlock*) block)->_corners_per_block
      = ((_dbBlock*) block)->_num_ext_corners;
}

void dbDatabase::readChip(FILE* file)
{
  _dbDatabase* db = (_dbDatabase*) this;
  _dbChip* chip = (_dbChip*) getChip();
  chip->~_dbChip();
  new (chip) _dbChip(db);
  dbIStream stream(db, file);
  stream >> *chip;
}

void dbDatabase::write(FILE* file)
{
  _dbDatabase* db = (_dbDatabase*) this;
  dbOStream stream(db, file);
  stream << *db;
  fflush(file);
}

void dbDatabase::writeTech(FILE* file)
{
  _dbDatabase* db = (_dbDatabase*) this;
  _dbTech* tech = (_dbTech*) getTech();

  if (tech == NULL)
    throw ZException("No technology");

  dbOStream stream(db, file);
  stream << *tech;
  fflush(file);
}

void dbDatabase::writeLib(FILE* file, dbLib* lib)
{
  _dbDatabase* db = (_dbDatabase*) this;
  dbOStream stream(db, file);
  stream << *(_dbLib*) lib;
  fflush(file);
}

void dbDatabase::writeLibs(FILE* file)
{
  _dbDatabase* db = (_dbDatabase*) this;
  dbOStream stream(db, file);
  stream << *db->_lib_tbl;
  fflush(file);
}

void dbDatabase::writeBlock(FILE* file, dbBlock* block)
{
  _dbDatabase* db = (_dbDatabase*) this;
  dbOStream stream(db, file);
  stream << *(_dbBlock*) block;
  fflush(file);
}

void dbDatabase::writeNets(FILE* file, dbBlock* block)
{
  _dbDatabase* db = (_dbDatabase*) this;
  dbOStream stream(db, file);
  stream << *((_dbBlock*) block)->_net_tbl;
  fflush(file);
}

void dbDatabase::writeWires(FILE* file, dbBlock* block)
{
  _dbDatabase* db = (_dbDatabase*) this;
  dbOStream stream(db, file);
  stream << *((_dbBlock*) block)->_wire_tbl;
  fflush(file);
}

void dbDatabase::writeParasitics(FILE* file, dbBlock* block)
{
  _dbDatabase* db = (_dbDatabase*) this;
  dbOStream stream(db, file);
  stream << ((_dbBlock*) block)->_num_ext_corners;
  stream << ((_dbBlock*) block)->_corner_name_list;
  stream << *((_dbBlock*) block)->_r_val_tbl;
  stream << *((_dbBlock*) block)->_c_val_tbl;
  stream << *((_dbBlock*) block)->_cc_val_tbl;
  stream << *((_dbBlock*) block)->_cap_node_tbl;
  stream << *((_dbBlock*) block)->_r_seg_tbl;
  stream << *((_dbBlock*) block)->_cc_seg_tbl;
  stream << *((_dbBlock*) block)->_extControl;
  fflush(file);
}

void dbDatabase::writeChip(FILE* file)
{
  _dbDatabase* db = (_dbDatabase*) this;
  _dbChip* chip = (_dbChip*) getChip();
  dbOStream stream(db, file);
  stream << *chip;
  fflush(file);
}

void dbDatabase::beginEco(dbBlock* block_)
{
  _dbBlock* block = (_dbBlock*) block_;

  if (block->_journal)
    delete block->_journal;

  block->_journal = new dbJournal(block_);
  assert(block->_journal);
}

void dbDatabase::endEco(dbBlock* block_)
{
  _dbBlock* block = (_dbBlock*) block_;
  dbJournal* eco = block->_journal;
  block->_journal = NULL;

  if (block->_journal_pending)
    delete block->_journal_pending;

  block->_journal_pending = eco;
}

bool dbDatabase::ecoEmpty(dbBlock* block_)
{
  _dbBlock* block = (_dbBlock*) block_;

  if (block->_journal)
    return block->_journal->empty();

  return false;
}

int dbDatabase::checkEco(dbBlock* block_)
{
  _dbBlock* block = (_dbBlock*) block_;

  if (block->_journal) {
    return block->_journal->size();
  } else {
    return 0;
  }
}

void dbDatabase::readEco(dbBlock* block_, const char* filename)
{
  _dbBlock* block = (_dbBlock*) block_;

  FILE* file = fopen(filename, "r");
  if (!file) {
    int errnum = errno;
    block->getImpl()->getLogger()->error(
        utl::ODB, 1, "Error opening file {}", strerror(errnum));
    return;
  }

  dbIStream stream(block->getDatabase(), file);
  dbJournal* eco = new dbJournal(block_);
  assert(eco);
  stream >> *eco;

  if (block->_journal_pending)
    delete block->_journal_pending;

  block->_journal_pending = eco;

  fclose(file);
}

void dbDatabase::writeEco(dbBlock* block_, const char* filename)
{
  _dbBlock* block = (_dbBlock*) block_;

  FILE* file = fopen(filename, "w");
  if (!file) {
    int errnum = errno;
    block->getImpl()->getLogger()->error(
        utl::ODB, 2, "Error opening file {}", strerror(errnum));
    return;
  }

  if (block->_journal_pending) {
    dbOStream stream(block->getDatabase(), file);
    stream << *block->_journal_pending;
  }

  fclose(file);
}

void dbDatabase::commitEco(dbBlock* block_)
{
  _dbBlock* block = (_dbBlock*) block_;

  // TODO: Need a check to ensure the commit is not applied to the block of
  // which
  //       this eco was generated from.
  if (block->_journal_pending) {
    block->_journal_pending->redo();
    delete block->_journal_pending;
    block->_journal_pending = NULL;
  }
}

dbDatabase* dbDatabase::create(utl::Logger* logger)
{
  if (db_tbl == NULL) {
    db_tbl = new dbTable<_dbDatabase>(
        NULL, NULL, (GetObjTbl_t) NULL, dbDatabaseObj);
    ZALLOCATED(db_tbl);
  }

  _dbDatabase* db = db_tbl->create();
  if(logger == nullptr)
    db->_logger = new utl::Logger();
  else
    db->_logger = logger;
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
  _dbDatabase* db = (_dbDatabase*) db_;
  db_tbl->destroy(db);
}

dbDatabase* dbDatabase::duplicate(dbDatabase* db_)
{
  _dbDatabase* db = (_dbDatabase*) db_;
  _dbDatabase* d = db_tbl->duplicate(db);
  return (dbDatabase*) d;
}

dbDatabase* dbDatabase::getDatabase(uint dbid)
{
  return (dbDatabase*) db_tbl->getPtr(dbid);
}

dbDatabase* dbObject::getDb() const
{
  return (dbDatabase*) getImpl()->getDatabase();
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
  db0->differences(diff, NULL, *db1);
  return diff.hasDifferences();
}

}  // namespace odb
