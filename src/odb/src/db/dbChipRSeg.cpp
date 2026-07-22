// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbChipRSeg.h"

#include "dbChip.h"
#include "dbChipCapNode.h"
#include "dbChipNet.h"
#include "dbCore.h"
#include "dbDatabase.h"
#include "dbTable.h"
#include "odb/db.h"
#include "utl/Logger.h"
namespace odb {
template class dbTable<_dbChipRSeg>;

bool _dbChipRSeg::operator==(const _dbChipRSeg& rhs) const
{
  // NOLINTBEGIN(readability-simplify-boolean-expr)
  if (next_chip_r_seg_ != rhs.next_chip_r_seg_) {
    return false;
  }
  if (source_cap_node_ != rhs.source_cap_node_) {
    return false;
  }
  if (target_cap_node_ != rhs.target_cap_node_) {
    return false;
  }
  if (resistance_ != rhs.resistance_) {
    return false;
  }

  return true;
  // NOLINTEND(readability-simplify-boolean-expr)
}

bool _dbChipRSeg::operator<(const _dbChipRSeg& rhs) const
{
  return true;
}

_dbChipRSeg::_dbChipRSeg(_dbDatabase* db)
{
  resistance_ = 0.0;
}

dbIStream& operator>>(dbIStream& stream, _dbChipRSeg& obj)
{
  stream >> obj.next_chip_r_seg_;
  stream >> obj.source_cap_node_;
  stream >> obj.target_cap_node_;
  stream >> obj.resistance_;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbChipRSeg& obj)
{
  stream << obj.next_chip_r_seg_;
  stream << obj.source_cap_node_;
  stream << obj.target_cap_node_;
  stream << obj.resistance_;
  return stream;
}

void _dbChipRSeg::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);
}

////////////////////////////////////////////////////////////////////
//
// dbChipRSeg - Methods
//
////////////////////////////////////////////////////////////////////

void dbChipRSeg::setResistance(float resistance)
{
  _dbChipRSeg* obj = (_dbChipRSeg*) this;

  obj->resistance_ = resistance;
}

float dbChipRSeg::getResistance() const
{
  _dbChipRSeg* obj = (_dbChipRSeg*) this;
  return obj->resistance_;
}

// User Code Begin dbChipRSegPublicMethods
dbChipRSeg* dbChipRSeg::create(dbChipNet* chip_net_,
                               dbChipCapNode* source_cap_node_,
                               dbChipCapNode* target_cap_node_)
{
  _dbChipNet* chip_net = (_dbChipNet*) chip_net_;
  _dbChip* chip = (_dbChip*) chip_net_->getChip();
  utl::Logger* logger = chip->getLogger();

  if (!source_cap_node_ || !target_cap_node_) {
    logger->error(utl::ODB,
                  535,
                  "Cannot create chip r-seg on net {} with a null cap node.",
                  chip_net_->getName());
  }

  if (source_cap_node_ == target_cap_node_) {
    logger->error(
        utl::ODB,
        536,
        "Cannot create chip r-seg on net {} between a cap node and itself.",
        chip_net_->getName());
  }

  if (source_cap_node_->getChipNet() != chip_net_
      || target_cap_node_->getChipNet() != chip_net_) {
    logger->error(utl::ODB,
                  537,
                  "Cannot create chip r-seg on net {}: its source and target "
                  "cap nodes must belong to that net.",
                  chip_net_->getName());
  }

  _dbChipRSeg* r_seg = chip->chip_r_seg_tbl_->create();
  r_seg->source_cap_node_ = source_cap_node_->getImpl()->getOID();
  r_seg->target_cap_node_ = target_cap_node_->getImpl()->getOID();

  // Add to the chip net's r-seg list
  r_seg->next_chip_r_seg_ = chip_net->first_r_seg_;
  chip_net->first_r_seg_ = r_seg->getOID();

  return (dbChipRSeg*) r_seg;
}

void dbChipRSeg::destroy(dbChipRSeg* r_seg_)
{
  _dbChipRSeg* r_seg = (_dbChipRSeg*) r_seg_;
  _dbChip* chip = (_dbChip*) r_seg->getOwner();
  _dbDatabase* db = chip->getDatabase();
  _dbChipCapNode* target_cap_node
      = chip->chip_cap_node_tbl_->getPtr(r_seg->target_cap_node_);
  _dbChipNet* chip_net = db->chip_net_tbl_->getPtr(target_cap_node->chip_net_);

  // Remove the deleted r-seg from the chip net's r-seg list
  if (chip_net->first_r_seg_ == r_seg->getOID()) {
    chip_net->first_r_seg_ = r_seg->next_chip_r_seg_;
  } else {
    _dbChipRSeg* previous_r_seg = nullptr;
    _dbChipRSeg* current_r_seg
        = chip->chip_r_seg_tbl_->getPtr(chip_net->first_r_seg_);
    while (current_r_seg && current_r_seg != r_seg) {
      previous_r_seg = current_r_seg;
      current_r_seg
          = chip->chip_r_seg_tbl_->getPtr(current_r_seg->next_chip_r_seg_);
    }
    if (current_r_seg && previous_r_seg) {
      previous_r_seg->next_chip_r_seg_ = current_r_seg->next_chip_r_seg_;
    }
  }

  chip->chip_r_seg_tbl_->destroy(r_seg);
}

dbChipCapNode* dbChipRSeg::getSourceCapNode() const
{
  _dbChipRSeg* r_seg = (_dbChipRSeg*) this;
  _dbChip* chip = (_dbChip*) r_seg->getOwner();
  _dbChipCapNode* source_cap_node
      = chip->chip_cap_node_tbl_->getPtr(r_seg->source_cap_node_);
  return (dbChipCapNode*) source_cap_node;
}

dbChipCapNode* dbChipRSeg::getTargetCapNode() const
{
  _dbChipRSeg* r_seg = (_dbChipRSeg*) this;
  _dbChip* chip = (_dbChip*) r_seg->getOwner();
  _dbChipCapNode* target_cap_node
      = chip->chip_cap_node_tbl_->getPtr(r_seg->target_cap_node_);
  return (dbChipCapNode*) target_cap_node;
}

dbChipNet* dbChipRSeg::getChipNet() const
{
  return getTargetCapNode()->getChipNet();
}
// User Code End dbChipRSegPublicMethods
}  // namespace odb
// Generator Code End Cpp