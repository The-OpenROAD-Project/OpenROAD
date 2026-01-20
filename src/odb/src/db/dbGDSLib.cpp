// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#include "dbGDSLib.h"

#include <utility>

#include "dbCore.h"
#include "dbDatabase.h"
#include "dbTable.h"
#include "odb/db.h"
#include "odb/dbObject.h"
#include "odb/dbSet.h"
#include "odb/dbTypes.h"

namespace odb {
template class dbTable<_dbGDSLib>;

bool _dbGDSLib::operator==(const _dbGDSLib& rhs) const
{
  if (lib_name_ != rhs.lib_name_) {
    return false;
  }
  if (uu_per_dbu_ != rhs.uu_per_dbu_) {
    return false;
  }
  if (dbu_per_meter_ != rhs.dbu_per_meter_) {
    return false;
  }
  if (*gdsstructure_tbl_ != *rhs.gdsstructure_tbl_) {
    return false;
  }
  if (gdsstructure_hash_ != rhs.gdsstructure_hash_) {
    return false;
  }
  return true;
}

////////////////////////////////////////////////////////////////////
//
// dbGDSLib - Methods
//
////////////////////////////////////////////////////////////////////

dbObjectTable* _dbGDSLib::getObjectTable(dbObjectType type)
{
  switch (type) {
    case dbGDSStructureObj:
      return gdsstructure_tbl_;
    default:
      break;
  }
  return getTable()->getObjectTable(type);
}

_dbGDSLib::_dbGDSLib(_dbDatabase* db)
{
  uu_per_dbu_ = 1.0;
  dbu_per_meter_ = 1e9;

  gdsstructure_tbl_ = new dbTable<_dbGDSStructure>(
      db, this, (GetObjTbl_t) &_dbGDSLib::getObjectTable, dbGDSStructureObj);

  gdsstructure_hash_.setTable(gdsstructure_tbl_);
}

_dbGDSLib::_dbGDSLib(_dbDatabase* db, const _dbGDSLib& r)
    : lib_name_(r.lib_name_),
      uu_per_dbu_(r.uu_per_dbu_),
      dbu_per_meter_(r.dbu_per_meter_),
      gdsstructure_hash_(r.gdsstructure_hash_),
      gdsstructure_tbl_(r.gdsstructure_tbl_)
{
}

_dbGDSLib::~_dbGDSLib()
{
  delete gdsstructure_tbl_;
}

dbIStream& operator>>(dbIStream& stream, _dbGDSLib& obj)
{
  stream >> obj.lib_name_;
  stream >> obj.uu_per_dbu_;
  stream >> obj.dbu_per_meter_;
  stream >> *obj.gdsstructure_tbl_;
  stream >> obj.gdsstructure_hash_;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbGDSLib& obj)
{
  stream << obj.lib_name_;
  stream << obj.uu_per_dbu_;
  stream << obj.dbu_per_meter_;
  stream << NamedTable("_structure_tbl", obj.gdsstructure_tbl_);
  stream << obj.gdsstructure_hash_;
  return stream;
}

_dbGDSStructure* _dbGDSLib::findStructure(const char* name)
{
  return gdsstructure_hash_.find(name);
}

////////////////////////////////////////////////////////////////////
//
// dbLib - Methods
//
////////////////////////////////////////////////////////////////////

void dbGDSLib::setLibname(std::string libname)
{
  _dbGDSLib* obj = (_dbGDSLib*) this;

  obj->lib_name_ = std::move(libname);
}

std::string dbGDSLib::getLibname() const
{
  _dbGDSLib* obj = (_dbGDSLib*) this;
  return obj->lib_name_;
}

void dbGDSLib::setUnits(double uu_per_dbu, double dbu_per_meter)
{
  _dbGDSLib* obj = (_dbGDSLib*) this;

  obj->uu_per_dbu_ = uu_per_dbu;
  obj->dbu_per_meter_ = dbu_per_meter;
}

std::pair<double, double> dbGDSLib::getUnits() const
{
  _dbGDSLib* obj = (_dbGDSLib*) this;
  return std::make_pair(obj->uu_per_dbu_, obj->dbu_per_meter_);
}

dbGDSStructure* dbGDSLib::findGDSStructure(const char* name) const
{
  _dbGDSLib* obj = (_dbGDSLib*) this;
  return (dbGDSStructure*) obj->gdsstructure_hash_.find(name);
}

dbSet<dbGDSStructure> dbGDSLib::getGDSStructures()
{
  _dbGDSLib* obj = (_dbGDSLib*) this;
  return dbSet<dbGDSStructure>(obj, obj->gdsstructure_tbl_);
}

dbGDSLib* dbGDSLib::create(dbDatabase* db, const std::string& name)
{
  auto* obj = (_dbDatabase*) db;
  auto lib = (dbGDSLib*) obj->gds_lib_tbl_->create();
  lib->setLibname(name);
  return lib;
}

void dbGDSLib::destroy(dbGDSLib* lib)
{
  auto* obj = (_dbGDSLib*) lib;
  auto* db = (_dbDatabase*) obj->getOwner();
  db->gds_lib_tbl_->destroy(obj);
}

void _dbGDSLib::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  info.children["libname"].add(lib_name_);
  info.children["structure_hash"].add(gdsstructure_hash_);
  gdsstructure_tbl_->collectMemInfo(info.children["structure"]);
}

}  // namespace odb
