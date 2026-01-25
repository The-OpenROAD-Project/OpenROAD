// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbChip.h"

#include <cstdint>
#include <cstdlib>
#include <string>
#include <unordered_map>

#include "dbBlock.h"
#include "dbBlockItr.h"
#include "dbChipConn.h"
#include "dbChipConnItr.h"
#include "dbChipInst.h"
#include "dbChipInstItr.h"
#include "dbChipNet.h"
#include "dbChipNetItr.h"
#include "dbChipRegion.h"
#include "dbCommon.h"
#include "dbCore.h"
#include "dbDatabase.h"
#include "dbMarkerCategory.h"
#include "dbNameCache.h"
#include "dbProperty.h"
#include "dbPropertyItr.h"
#include "dbTable.h"
#include "dbTech.h"
#include "odb/db.h"
#include "odb/dbChipCallBackObj.h"
#include "odb/dbObject.h"
#include "odb/dbSet.h"
#include "odb/geom.h"
// User Code Begin Includes
#include <list>
// User Code End Includes
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
  if (top_ != rhs.top_) {
    return false;
  }
  if (chipinsts_ != rhs.chipinsts_) {
    return false;
  }
  if (conns_ != rhs.conns_) {
    return false;
  }
  if (nets_ != rhs.nets_) {
    return false;
  }
  if (tech_ != rhs.tech_) {
    return false;
  }
  if (*prop_tbl_ != *rhs.prop_tbl_) {
    return false;
  }
  if (*chip_region_tbl_ != *rhs.chip_region_tbl_) {
    return false;
  }
  if (*marker_categories_tbl_ != *rhs.marker_categories_tbl_) {
    return false;
  }
  if (next_entry_ != rhs.next_entry_) {
    return false;
  }

  // User Code Begin ==
  if (*block_tbl_ != *rhs.block_tbl_) {
    return false;
  }
  if (*name_cache_ != *rhs.name_cache_) {
    return false;
  }
  // User Code End ==
  return true;
}

bool _dbChip::operator<(const _dbChip& rhs) const
{
  if (top_ >= rhs.top_) {
    return false;
  }

  return true;
}

_dbChip::_dbChip(_dbDatabase* db)
{
  name_ = nullptr;
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
  prop_tbl_ = new dbTable<_dbProperty>(
      db, this, (GetObjTbl_t) &_dbChip::getObjectTable, dbPropertyObj);
  chip_region_tbl_ = new dbTable<_dbChipRegion>(
      db, this, (GetObjTbl_t) &_dbChip::getObjectTable, dbChipRegionObj);
  marker_categories_tbl_ = new dbTable<_dbMarkerCategory>(
      db, this, (GetObjTbl_t) &_dbChip::getObjectTable, dbMarkerCategoryObj);
  // User Code Begin Constructor
  block_tbl_ = new dbTable<_dbBlock>(
      db, this, (GetObjTbl_t) &_dbChip::getObjectTable, dbBlockObj);
  name_cache_
      = new _dbNameCache(db, this, (GetObjTbl_t) &_dbChip::getObjectTable);

  block_itr_ = new dbBlockItr(block_tbl_);

  prop_itr_ = new dbPropertyItr(prop_tbl_);
  // User Code End Constructor
}

dbIStream& operator>>(dbIStream& stream, _dbChip& obj)
{
  if (obj.getDatabase()->isSchema(kSchemaChipExtended)) {
    stream >> obj.name_;
  }
  if (obj.getDatabase()->isSchema(kSchemaChipExtended)) {
    stream >> obj.type_;
  }
  if (obj.getDatabase()->isSchema(kSchemaChipExtended)) {
    stream >> obj.offset_;
  }
  if (obj.getDatabase()->isSchema(kSchemaChipExtended)) {
    stream >> obj.width_;
  }
  if (obj.getDatabase()->isSchema(kSchemaChipExtended)) {
    stream >> obj.height_;
  }
  if (obj.getDatabase()->isSchema(kSchemaChipExtended)) {
    stream >> obj.thickness_;
  }
  if (obj.getDatabase()->isSchema(kSchemaChipExtended)) {
    stream >> obj.shrink_;
  }
  if (obj.getDatabase()->isSchema(kSchemaChipExtended)) {
    stream >> obj.seal_ring_east_;
  }
  if (obj.getDatabase()->isSchema(kSchemaChipExtended)) {
    stream >> obj.seal_ring_west_;
  }
  if (obj.getDatabase()->isSchema(kSchemaChipExtended)) {
    stream >> obj.seal_ring_north_;
  }
  if (obj.getDatabase()->isSchema(kSchemaChipExtended)) {
    stream >> obj.seal_ring_south_;
  }
  if (obj.getDatabase()->isSchema(kSchemaChipExtended)) {
    stream >> obj.scribe_line_east_;
  }
  if (obj.getDatabase()->isSchema(kSchemaChipExtended)) {
    stream >> obj.scribe_line_west_;
  }
  if (obj.getDatabase()->isSchema(kSchemaChipExtended)) {
    stream >> obj.scribe_line_north_;
  }
  if (obj.getDatabase()->isSchema(kSchemaChipExtended)) {
    stream >> obj.scribe_line_south_;
  }
  if (obj.getDatabase()->isSchema(kSchemaChipExtended)) {
    stream >> obj.tsv_;
  }
  stream >> obj.top_;
  if (obj.getDatabase()->isSchema(kSchemaChipInst)) {
    stream >> obj.chipinsts_;
  }
  if (obj.getDatabase()->isSchema(kSchemaChipRegion)) {
    stream >> obj.conns_;
  }
  if (obj.getDatabase()->isSchema(kSchemaChipBump)) {
    stream >> obj.nets_;
  }
  if (obj.getDatabase()->isSchema(kSchemaChipTech)) {
    stream >> obj.tech_;
  }
  if (obj.getDatabase()->isSchema(kSchemaChipRegion)) {
    stream >> *obj.chip_region_tbl_;
  }
  if (obj.getDatabase()->isSchema(kSchemaChipMarkerCategories)) {
    stream >> *obj.marker_categories_tbl_;
  }
  // User Code Begin >>
  stream >> *obj.block_tbl_;
  stream >> *obj.prop_tbl_;
  stream >> *obj.name_cache_;
  if (obj.getDatabase()->isSchema(kSchemaChipHashTable)) {
    stream >> obj.next_entry_;
  }
  auto chip = (dbChip*) &obj;
  for (const auto& chip_region : chip->getChipRegions()) {
    obj.chip_region_map_[chip_region->getName()] = chip_region->getId();
  }
  for (const auto& marker_category : ((dbChip*) &obj)->getMarkerCategories()) {
    obj.marker_categories_map_[marker_category->getName()]
        = marker_category->getId();
  }
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
  stream << obj.top_;
  stream << obj.chipinsts_;
  stream << obj.conns_;
  stream << obj.nets_;
  stream << obj.tech_;
  stream << *obj.chip_region_tbl_;
  stream << *obj.marker_categories_tbl_;
  // User Code Begin <<
  stream << *obj.block_tbl_;
  stream << NamedTable("prop_tbl", obj.prop_tbl_);
  stream << *obj.name_cache_;
  stream << obj.next_entry_;
  // User Code End <<
  return stream;
}

dbObjectTable* _dbChip::getObjectTable(dbObjectType type)
{
  switch (type) {
    case dbPropertyObj:
      return prop_tbl_;
    case dbChipRegionObj:
      return chip_region_tbl_;
    case dbMarkerCategoryObj:
      return marker_categories_tbl_;
      // User Code Begin getObjectTable
    case dbBlockObj:
      return block_tbl_;
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

  prop_tbl_->collectMemInfo(info.children["prop_tbl_"]);

  chip_region_tbl_->collectMemInfo(info.children["chip_region_tbl_"]);

  marker_categories_tbl_->collectMemInfo(
      info.children["marker_categories_tbl_"]);

  // User Code Begin collectMemInfo
  block_tbl_->collectMemInfo(info.children["block"]);
  name_cache_->collectMemInfo(info.children["name_cache"]);
  // User Code End collectMemInfo
}

_dbChip::~_dbChip()
{
  if (name_) {
    free((void*) name_);
  }
  delete prop_tbl_;
  delete chip_region_tbl_;
  delete marker_categories_tbl_;
  // User Code Begin Destructor
  delete block_tbl_;
  delete name_cache_;
  delete block_itr_;
  delete prop_itr_;

  while (!callbacks_.empty()) {
    auto _cbitr = callbacks_.begin();
    (*_cbitr)->removeOwner();
  }
  // User Code End Destructor
}

////////////////////////////////////////////////////////////////////
//
// dbChip - Methods
//
////////////////////////////////////////////////////////////////////

const char* dbChip::getName() const
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

dbSet<dbChipRegion> dbChip::getChipRegions() const
{
  _dbChip* obj = (_dbChip*) this;
  return dbSet<dbChipRegion>(obj, obj->chip_region_tbl_);
}

dbSet<dbMarkerCategory> dbChip::getMarkerCategories() const
{
  _dbChip* obj = (_dbChip*) this;
  return dbSet<dbMarkerCategory>(obj, obj->marker_categories_tbl_);
}

// User Code Begin dbChipPublicMethods

dbChip::ChipType dbChip::getChipType() const
{
  _dbChip* obj = (_dbChip*) this;
  return (dbChip::ChipType) obj->type_;
}

dbBlock* dbChip::getBlock()
{
  _dbChip* chip = (_dbChip*) this;

  if (chip->top_ == 0) {
    return nullptr;
  }

  return (dbBlock*) chip->block_tbl_->getPtr(chip->top_);
}

dbSet<dbChipInst> dbChip::getChipInsts() const
{
  _dbChip* chip = (_dbChip*) this;
  _dbDatabase* db = (_dbDatabase*) chip->getOwner();
  return dbSet<dbChipInst>(chip, db->chip_inst_itr_);
}

dbSet<dbChipConn> dbChip::getChipConns() const
{
  _dbChip* chip = (_dbChip*) this;
  _dbDatabase* db = (_dbDatabase*) chip->getOwner();
  return dbSet<dbChipConn>(chip, db->chip_conn_itr_);
}

dbSet<dbChipNet> dbChip::getChipNets() const
{
  _dbChip* chip = (_dbChip*) this;
  _dbDatabase* db = (_dbDatabase*) chip->getOwner();
  return dbSet<dbChipNet>(chip, db->chip_net_itr_);
}

dbChipInst* dbChip::findChipInst(const std::string& name) const
{
  _dbChip* chip = (_dbChip*) this;
  auto it = chip->chipinsts_map_.find(name);
  if (it != chip->chipinsts_map_.end()) {
    auto db = (_dbDatabase*) chip->getOwner();
    return (dbChipInst*) db->chip_inst_tbl_->getPtr((*it).second);
  }
  return nullptr;
}

dbChipRegion* dbChip::findChipRegion(const std::string& name) const
{
  _dbChip* chip = (_dbChip*) this;
  auto it = chip->chip_region_map_.find(name);
  if (it != chip->chip_region_map_.end()) {
    return (dbChipRegion*) chip->chip_region_tbl_->getPtr((*it).second);
  }
  return nullptr;
}

dbTech* dbChip::getTech() const
{
  _dbChip* chip = (_dbChip*) this;
  if (!chip->tech_.isValid()) {
    return nullptr;
  }
  _dbDatabase* db = (_dbDatabase*) chip->getOwner();
  return (dbTech*) db->tech_tbl_->getPtr(chip->tech_);
}

Rect dbChip::getBBox() const
{
  _dbChip* _chip = (_dbChip*) this;
  const int dx = _chip->width_ + _chip->scribe_line_east_
                 + _chip->seal_ring_east_ + _chip->scribe_line_west_
                 + _chip->seal_ring_west_;
  const int dy = _chip->height_ + _chip->scribe_line_north_
                 + _chip->seal_ring_north_ + _chip->scribe_line_south_
                 + _chip->seal_ring_south_;
  Rect box(0, 0, dx, dy);
  return box;
}

Cuboid dbChip::getCuboid() const
{
  _dbChip* _chip = (_dbChip*) this;
  Rect box = getBBox();
  return Cuboid(
      box.xMin(), box.yMin(), 0, box.xMax(), box.yMax(), _chip->thickness_);
}

dbMarkerCategory* dbChip::findMarkerCategory(const char* name) const
{
  _dbChip* obj = (_dbChip*) this;
  auto it = obj->marker_categories_map_.find(name);
  if (it != obj->marker_categories_map_.end()) {
    return (dbMarkerCategory*) obj->marker_categories_tbl_->getPtr(it->second);
  }
  return nullptr;
}

dbChip* dbChip::create(dbDatabase* db_,
                       dbTech* tech,
                       const std::string& name,
                       ChipType type)
{
  _dbDatabase* db = (_dbDatabase*) db_;
  if (db->chip_hash_.hasMember(name.c_str())) {
    db->getLogger()->error(utl::ODB, 385, "Chip {} already exists", name);
  }
  _dbChip* chip = db->chip_tbl_->create();
  chip->name_ = safe_strdup(name.c_str());
  chip->type_ = (uint32_t) type;
  if (db->chip_ == 0) {
    db->chip_ = chip->getOID();
  }
  db->chip_hash_.insert(chip);
  if (tech) {
    chip->tech_ = tech->getId();
  } else if (type == ChipType::DIE) {
    chip->getLogger()->error(
        utl::ODB, 422, "Cannot create DIE chip without technology");
  }
  return (dbChip*) chip;
}

dbChip* dbChip::getChip(dbDatabase* db_, uint32_t dbid_)
{
  _dbDatabase* db = (_dbDatabase*) db_;
  return (dbChip*) db->chip_tbl_->getPtr(dbid_);
}

void dbChip::destroy(dbChip* chip_)
{
  _dbChip* chip = (_dbChip*) chip_;
  // Destroy chip connections
  auto chip_conns = chip_->getChipConns();
  auto chip_conns_itr = chip_conns.begin();
  while (chip_conns_itr != chip_conns.end()) {
    auto chipConn = *chip_conns_itr++;
    dbChipConn::destroy(chipConn);
  }
  // destroy chip insts
  auto chip_insts = chip_->getChipInsts();
  auto chip_insts_itr = chip_insts.begin();
  while (chip_insts_itr != chip_insts.end()) {
    auto chipInst = *chip_insts_itr++;
    dbChipInst::destroy(chipInst);
  }
  // TODO: destroy instances of the current chip
  // Destroy chip
  _dbDatabase* db = chip->getDatabase();
  if (db->chip_ == chip->getOID()) {
    db->chip_ = 0;
  }
  dbProperty::destroyProperties(chip);
  db->chip_hash_.remove(chip);
  db->chip_tbl_->destroy(chip);
}
// User Code End dbChipPublicMethods
}  // namespace odb
// Generator Code End Cpp
