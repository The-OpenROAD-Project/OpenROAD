// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbChip.h"

#include <string>

#include "dbBlock.h"
#include "dbBlockItr.h"
#include "dbDatabase.h"
#include "dbNameCache.h"
#include "dbProperty.h"
#include "dbPropertyItr.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "dbTech.h"
#include "odb/db.h"
#include "odb/dbSet.h"
namespace odb {
template class dbTable<_dbChip>;

bool _dbChip::operator==(const _dbChip& rhs) const
{
  if (name_ != rhs.name_) {
    return false;
  }
  if (type_ != rhs.type_) {
    return false;
  }
  if (offset_ != rhs.offset_) {
    return false;
  }
  if (width_ != rhs.width_) {
    return false;
  }
  if (height_ != rhs.height_) {
    return false;
  }
  if (thickness_ != rhs.thickness_) {
    return false;
  }
  if (shrink_ != rhs.shrink_) {
    return false;
  }
  if (seal_ring_east_ != rhs.seal_ring_east_) {
    return false;
  }
  if (seal_ring_west_ != rhs.seal_ring_west_) {
    return false;
  }
  if (seal_ring_north_ != rhs.seal_ring_north_) {
    return false;
  }
  if (seal_ring_south_ != rhs.seal_ring_south_) {
    return false;
  }
  if (scribe_line_east_ != rhs.scribe_line_east_) {
    return false;
  }
  if (scribe_line_west_ != rhs.scribe_line_west_) {
    return false;
  }
  if (scribe_line_north_ != rhs.scribe_line_north_) {
    return false;
  }
  if (scribe_line_south_ != rhs.scribe_line_south_) {
    return false;
  }
  if (tsv_ != rhs.tsv_) {
    return false;
  }
  if (_top != rhs._top) {
    return false;
  }
  if (*_prop_tbl != *rhs._prop_tbl) {
    return false;
  }

  // User Code Begin ==
  if (*_block_tbl != *rhs._block_tbl) {
    return false;
  }
  if (*_name_cache != *rhs._name_cache) {
    return false;
  }
  // User Code End ==
  return true;
}

bool _dbChip::operator<(const _dbChip& rhs) const
{
  if (_top >= rhs._top) {
    return false;
  }

  return true;
}

_dbChip::_dbChip(_dbDatabase* db)
{
  type_ = 0;
  offset_ = {};
  width_ = 0;
  height_ = 0;
  thickness_ = 0;
  shrink_ = 0.0;
  seal_ring_east_ = 0;
  seal_ring_west_ = 0;
  seal_ring_north_ = 0;
  seal_ring_south_ = 0;
  scribe_line_east_ = 0;
  scribe_line_west_ = 0;
  scribe_line_north_ = 0;
  scribe_line_south_ = 0;
  tsv_ = false;
  _prop_tbl = new dbTable<_dbProperty>(
      db, this, (GetObjTbl_t) &_dbChip::getObjectTable, dbPropertyObj);
  // User Code Begin Constructor
  _block_tbl = new dbTable<_dbBlock>(
      db, this, (GetObjTbl_t) &_dbChip::getObjectTable, dbBlockObj);
  _name_cache
      = new _dbNameCache(db, this, (GetObjTbl_t) &_dbChip::getObjectTable);

  _block_itr = new dbBlockItr(_block_tbl);

  _prop_itr = new dbPropertyItr(_prop_tbl);
  // User Code End Constructor
}

dbIStream& operator>>(dbIStream& stream, _dbChip& obj)
{
  if (obj.getDatabase()->isSchema(db_schema_chip_extended)) {
    stream >> obj.name_;
  }
  if (obj.getDatabase()->isSchema(db_schema_chip_extended)) {
    stream >> obj.type_;
  }
  if (obj.getDatabase()->isSchema(db_schema_chip_extended)) {
    stream >> obj.offset_;
  }
  if (obj.getDatabase()->isSchema(db_schema_chip_extended)) {
    stream >> obj.width_;
  }
  if (obj.getDatabase()->isSchema(db_schema_chip_extended)) {
    stream >> obj.height_;
  }
  if (obj.getDatabase()->isSchema(db_schema_chip_extended)) {
    stream >> obj.thickness_;
  }
  if (obj.getDatabase()->isSchema(db_schema_chip_extended)) {
    stream >> obj.shrink_;
  }
  if (obj.getDatabase()->isSchema(db_schema_chip_extended)) {
    stream >> obj.seal_ring_east_;
  }
  if (obj.getDatabase()->isSchema(db_schema_chip_extended)) {
    stream >> obj.seal_ring_west_;
  }
  if (obj.getDatabase()->isSchema(db_schema_chip_extended)) {
    stream >> obj.seal_ring_north_;
  }
  if (obj.getDatabase()->isSchema(db_schema_chip_extended)) {
    stream >> obj.seal_ring_south_;
  }
  if (obj.getDatabase()->isSchema(db_schema_chip_extended)) {
    stream >> obj.scribe_line_east_;
  }
  if (obj.getDatabase()->isSchema(db_schema_chip_extended)) {
    stream >> obj.scribe_line_west_;
  }
  if (obj.getDatabase()->isSchema(db_schema_chip_extended)) {
    stream >> obj.scribe_line_north_;
  }
  if (obj.getDatabase()->isSchema(db_schema_chip_extended)) {
    stream >> obj.scribe_line_south_;
  }
  if (obj.getDatabase()->isSchema(db_schema_chip_extended)) {
    stream >> obj.tsv_;
  }
  stream >> obj._top;
  // User Code Begin >>
  stream >> *obj._block_tbl;
  stream >> *obj._prop_tbl;
  stream >> *obj._name_cache;
  // User Code End >>
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbChip& obj)
{
  dbOStreamScope scope(stream, "dbChip");
  stream << obj.name_;
  stream << obj.type_;
  stream << obj.offset_;
  stream << obj.width_;
  stream << obj.height_;
  stream << obj.thickness_;
  stream << obj.shrink_;
  stream << obj.seal_ring_east_;
  stream << obj.seal_ring_west_;
  stream << obj.seal_ring_north_;
  stream << obj.seal_ring_south_;
  stream << obj.scribe_line_east_;
  stream << obj.scribe_line_west_;
  stream << obj.scribe_line_north_;
  stream << obj.scribe_line_south_;
  stream << obj.tsv_;
  stream << obj._top;
  // User Code Begin <<
  stream << *obj._block_tbl;
  stream << NamedTable("prop_tbl", obj._prop_tbl);
  stream << *obj._name_cache;
  // User Code End <<
  return stream;
}

dbObjectTable* _dbChip::getObjectTable(dbObjectType type)
{
  switch (type) {
    case dbPropertyObj:
      return _prop_tbl;
      // User Code Begin getObjectTable
    case dbBlockObj:
      return _block_tbl;
    // User Code End getObjectTable
    default:
      break;
  }
  return getTable()->getObjectTable(type);
}
void _dbChip::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  _prop_tbl->collectMemInfo(info.children_["_prop_tbl"]);

  // User Code Begin collectMemInfo
  _block_tbl->collectMemInfo(info.children_["block"]);
  _name_cache->collectMemInfo(info.children_["name_cache"]);
  // User Code End collectMemInfo
}

_dbChip::~_dbChip()
{
  delete _prop_tbl;
  // User Code Begin Destructor
  delete _block_tbl;
  delete _name_cache;
  delete _block_itr;
  delete _prop_itr;
  // User Code End Destructor
}

////////////////////////////////////////////////////////////////////
//
// dbChip - Methods
//
////////////////////////////////////////////////////////////////////

std::string dbChip::getName() const
{
  _dbChip* obj = (_dbChip*) this;
  return obj->name_;
}

void dbChip::setOffset(Point offset)
{
  _dbChip* obj = (_dbChip*) this;

  obj->offset_ = offset;
}

Point dbChip::getOffset() const
{
  _dbChip* obj = (_dbChip*) this;
  return obj->offset_;
}

void dbChip::setWidth(int width)
{
  _dbChip* obj = (_dbChip*) this;

  obj->width_ = width;
}

int dbChip::getWidth() const
{
  _dbChip* obj = (_dbChip*) this;
  return obj->width_;
}

void dbChip::setHeight(int height)
{
  _dbChip* obj = (_dbChip*) this;

  obj->height_ = height;
}

int dbChip::getHeight() const
{
  _dbChip* obj = (_dbChip*) this;
  return obj->height_;
}

void dbChip::setThickness(int thickness)
{
  _dbChip* obj = (_dbChip*) this;

  obj->thickness_ = thickness;
}

int dbChip::getThickness() const
{
  _dbChip* obj = (_dbChip*) this;
  return obj->thickness_;
}

void dbChip::setShrink(float shrink)
{
  _dbChip* obj = (_dbChip*) this;

  obj->shrink_ = shrink;
}

float dbChip::getShrink() const
{
  _dbChip* obj = (_dbChip*) this;
  return obj->shrink_;
}

void dbChip::setSealRingEast(int seal_ring_east)
{
  _dbChip* obj = (_dbChip*) this;

  obj->seal_ring_east_ = seal_ring_east;
}

int dbChip::getSealRingEast() const
{
  _dbChip* obj = (_dbChip*) this;
  return obj->seal_ring_east_;
}

void dbChip::setSealRingWest(int seal_ring_west)
{
  _dbChip* obj = (_dbChip*) this;

  obj->seal_ring_west_ = seal_ring_west;
}

int dbChip::getSealRingWest() const
{
  _dbChip* obj = (_dbChip*) this;
  return obj->seal_ring_west_;
}

void dbChip::setSealRingNorth(int seal_ring_north)
{
  _dbChip* obj = (_dbChip*) this;

  obj->seal_ring_north_ = seal_ring_north;
}

int dbChip::getSealRingNorth() const
{
  _dbChip* obj = (_dbChip*) this;
  return obj->seal_ring_north_;
}

void dbChip::setSealRingSouth(int seal_ring_south)
{
  _dbChip* obj = (_dbChip*) this;

  obj->seal_ring_south_ = seal_ring_south;
}

int dbChip::getSealRingSouth() const
{
  _dbChip* obj = (_dbChip*) this;
  return obj->seal_ring_south_;
}

void dbChip::setScribeLineEast(int scribe_line_east)
{
  _dbChip* obj = (_dbChip*) this;

  obj->scribe_line_east_ = scribe_line_east;
}

int dbChip::getScribeLineEast() const
{
  _dbChip* obj = (_dbChip*) this;
  return obj->scribe_line_east_;
}

void dbChip::setScribeLineWest(int scribe_line_west)
{
  _dbChip* obj = (_dbChip*) this;

  obj->scribe_line_west_ = scribe_line_west;
}

int dbChip::getScribeLineWest() const
{
  _dbChip* obj = (_dbChip*) this;
  return obj->scribe_line_west_;
}

void dbChip::setScribeLineNorth(int scribe_line_north)
{
  _dbChip* obj = (_dbChip*) this;

  obj->scribe_line_north_ = scribe_line_north;
}

int dbChip::getScribeLineNorth() const
{
  _dbChip* obj = (_dbChip*) this;
  return obj->scribe_line_north_;
}

void dbChip::setScribeLineSouth(int scribe_line_south)
{
  _dbChip* obj = (_dbChip*) this;

  obj->scribe_line_south_ = scribe_line_south;
}

int dbChip::getScribeLineSouth() const
{
  _dbChip* obj = (_dbChip*) this;
  return obj->scribe_line_south_;
}

void dbChip::setTsv(bool tsv)
{
  _dbChip* obj = (_dbChip*) this;

  obj->tsv_ = tsv;
}

bool dbChip::isTsv() const
{
  _dbChip* obj = (_dbChip*) this;
  return obj->tsv_;
}

// User Code Begin dbChipPublicMethods
dbBlock* dbChip::getBlock()
{
  _dbChip* chip = (_dbChip*) this;

  if (chip->_top == 0) {
    return nullptr;
  }

  return (dbBlock*) chip->_block_tbl->getPtr(chip->_top);
}

dbChip* dbChip::create(dbDatabase* db_)
{
  _dbDatabase* db = (_dbDatabase*) db_;

  if (db->_chip != 0) {
    return nullptr;
  }

  _dbChip* chip = db->_chip_tbl->create();
  db->_chip = chip->getOID();
  return (dbChip*) chip;
}

dbChip* dbChip::getChip(dbDatabase* db_, uint dbid_)
{
  _dbDatabase* db = (_dbDatabase*) db_;
  return (dbChip*) db->_chip_tbl->getPtr(dbid_);
}

void dbChip::destroy(dbChip* chip_)
{
  _dbChip* chip = (_dbChip*) chip_;
  _dbDatabase* db = chip->getDatabase();
  dbProperty::destroyProperties(chip);
  db->_chip_tbl->destroy(chip);
  db->_chip = 0;
}
// User Code End dbChipPublicMethods
}  // namespace odb
// Generator Code End Cpp