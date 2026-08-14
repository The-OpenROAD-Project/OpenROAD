// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbAlignmentMarkerRule.h"

#include <cstdint>
#include <vector>

#include "dbCore.h"
#include "dbDatabase.h"
#include "dbLib.h"
#include "dbMaster.h"
#include "dbTable.h"
#include "odb/db.h"
// User Code Begin Includes
#include <algorithm>
#include <cstddef>

#include "odb/dbTransform.h"
#include "odb/dbTypes.h"
// User Code End Includes
namespace odb {
template class dbTable<_dbAlignmentMarkerRule>;

bool _dbAlignmentMarkerRule::operator==(const _dbAlignmentMarkerRule& rhs) const
{
  // NOLINTBEGIN(readability-simplify-boolean-expr)
  if (lib_a_ != rhs.lib_a_) {
    return false;
  }
  if (master_a_ != rhs.master_a_) {
    return false;
  }
  if (lib_b_ != rhs.lib_b_) {
    return false;
  }
  if (master_b_ != rhs.master_b_) {
    return false;
  }
  if (tolerance_ != rhs.tolerance_) {
    return false;
  }

  return true;
  // NOLINTEND(readability-simplify-boolean-expr)
}

bool _dbAlignmentMarkerRule::operator<(const _dbAlignmentMarkerRule& rhs) const
{
  return true;
}

_dbAlignmentMarkerRule::_dbAlignmentMarkerRule(_dbDatabase* db)
{
  tolerance_ = 0;
}

dbIStream& operator>>(dbIStream& stream, _dbAlignmentMarkerRule& obj)
{
  stream >> obj.lib_a_;
  stream >> obj.master_a_;
  stream >> obj.lib_b_;
  stream >> obj.master_b_;
  stream >> obj.rel_orients_;
  stream >> obj.tolerance_;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbAlignmentMarkerRule& obj)
{
  stream << obj.lib_a_;
  stream << obj.master_a_;
  stream << obj.lib_b_;
  stream << obj.master_b_;
  stream << obj.rel_orients_;
  stream << obj.tolerance_;
  return stream;
}

void _dbAlignmentMarkerRule::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  info.children["rel_orients"].add(rel_orients_);
}

////////////////////////////////////////////////////////////////////
//
// dbAlignmentMarkerRule - Methods
//
////////////////////////////////////////////////////////////////////

void dbAlignmentMarkerRule::setTolerance(int tolerance)
{
  _dbAlignmentMarkerRule* obj = (_dbAlignmentMarkerRule*) this;

  obj->tolerance_ = tolerance;
}

int dbAlignmentMarkerRule::getTolerance() const
{
  _dbAlignmentMarkerRule* obj = (_dbAlignmentMarkerRule*) this;
  return obj->tolerance_;
}

// User Code Begin dbAlignmentMarkerRulePublicMethods

dbMaster* dbAlignmentMarkerRule::getMasterA() const
{
  _dbAlignmentMarkerRule* obj = (_dbAlignmentMarkerRule*) this;
  if (!obj->master_a_.isValid() || !obj->lib_a_.isValid()) {
    return nullptr;
  }
  _dbDatabase* db = obj->getDatabase();
  auto* lib = (_dbLib*) db->lib_tbl_->getPtr(obj->lib_a_);
  return (dbMaster*) lib->master_tbl_->getPtr(obj->master_a_);
}

dbMaster* dbAlignmentMarkerRule::getMasterB() const
{
  _dbAlignmentMarkerRule* obj = (_dbAlignmentMarkerRule*) this;
  if (!obj->master_b_.isValid() || !obj->lib_b_.isValid()) {
    return nullptr;
  }
  _dbDatabase* db = obj->getDatabase();
  auto* lib = (_dbLib*) db->lib_tbl_->getPtr(obj->lib_b_);
  return (dbMaster*) lib->master_tbl_->getPtr(obj->master_b_);
}

std::vector<dbOrientType> dbAlignmentMarkerRule::getRelativeOrientations() const
{
  _dbAlignmentMarkerRule* obj = (_dbAlignmentMarkerRule*) this;
  std::vector<dbOrientType> orients;
  orients.reserve(obj->rel_orients_.size());
  for (const auto rel_orient : obj->rel_orients_) {
    orients.emplace_back(static_cast<dbOrientType::Value>(rel_orient));
  }
  // When masterA == masterB the rule cannot tell which marker plays "A" vs
  // "B", so the relative orientation `A^-1 * B` is interchangeable with its
  // inverse `B^-1 * A`. Close the allowed set under inversion so the check
  // is symmetric regardless of which chip's marker happens to be iterated
  // first.
  if (obj->lib_a_ == obj->lib_b_ && obj->master_a_ == obj->master_b_) {
    const std::size_t original_size = orients.size();
    for (std::size_t i = 0; i < original_size; ++i) {
      dbTransform xform(orients[i]);
      xform.invert();
      const dbOrientType inverse = xform.getOrient();
      if (std::ranges::find(orients, inverse) == orients.end()) {
        orients.push_back(inverse);
      }
    }
  }
  return orients;
}

void dbAlignmentMarkerRule::setRelativeOrientations(
    const std::vector<dbOrientType>& relative_orientations)
{
  _dbAlignmentMarkerRule* obj = (_dbAlignmentMarkerRule*) this;
  obj->rel_orients_.clear();
  obj->rel_orients_.reserve(relative_orientations.size());
  for (const auto& relative_orient : relative_orientations) {
    obj->rel_orients_.push_back(
        static_cast<uint8_t>(relative_orient.getValue()));
  }
}

void dbAlignmentMarkerRule::addRelativeOrientation(
    dbOrientType relative_orientation)
{
  _dbAlignmentMarkerRule* obj = (_dbAlignmentMarkerRule*) this;
  obj->rel_orients_.push_back(
      static_cast<uint8_t>(relative_orientation.getValue()));
}

dbAlignmentMarkerRule* dbAlignmentMarkerRule::create(dbMaster* master_a,
                                                     dbMaster* master_b)
{
  if (master_a == nullptr || master_b == nullptr) {
    return nullptr;
  }
  _dbDatabase* _db = master_a->getImpl()->getDatabase();
  dbLib* lib_a = master_a->getLib();
  dbLib* lib_b = master_b->getLib();

  _dbAlignmentMarkerRule* rule = _db->alignment_marker_rule_tbl_->create();
  rule->lib_a_ = ((_dbLib*) lib_a)->getOID();
  rule->master_a_ = ((_dbMaster*) master_a)->getOID();
  rule->lib_b_ = ((_dbLib*) lib_b)->getOID();
  rule->master_b_ = ((_dbMaster*) master_b)->getOID();
  return (dbAlignmentMarkerRule*) rule;
}

void dbAlignmentMarkerRule::destroy(
    dbAlignmentMarkerRule* alignment_marker_rule)
{
  auto* obj = (_dbAlignmentMarkerRule*) alignment_marker_rule;
  _dbDatabase* _db = obj->getDatabase();
  _db->alignment_marker_rule_tbl_->destroy(obj);
}

// User Code End dbAlignmentMarkerRulePublicMethods
}  // namespace odb
// Generator Code End Cpp