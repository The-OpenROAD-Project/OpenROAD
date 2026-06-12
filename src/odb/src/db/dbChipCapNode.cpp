// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbChipCapNode.h"

#include "dbChip.h"
#include "dbChipBumpInst.h"
#include "dbChipNet.h"
#include "dbCore.h"
#include "dbDatabase.h"
#include "dbTable.h"
#include "odb/db.h"
namespace odb {
template class dbTable<_dbChipCapNode>;

bool _dbChipCapNode::operator==(const _dbChipCapNode& rhs) const
{
  // NOLINTBEGIN(readability-simplify-boolean-expr)
  if (chip_net_ != rhs.chip_net_) {
    return false;
  }
  if (next_chip_cap_node_ != rhs.next_chip_cap_node_) {
    return false;
  }
  if (chip_bump_inst_ != rhs.chip_bump_inst_) {
    return false;
  }
  if (capacitance_ != rhs.capacitance_) {
    return false;
  }

  return true;
  // NOLINTEND(readability-simplify-boolean-expr)
}

bool _dbChipCapNode::operator<(const _dbChipCapNode& rhs) const
{
  return true;
}

_dbChipCapNode::_dbChipCapNode(_dbDatabase* db)
{
  capacitance_ = 0.0;
}

dbIStream& operator>>(dbIStream& stream, _dbChipCapNode& obj)
{
  stream >> obj.chip_net_;
  stream >> obj.next_chip_cap_node_;
  stream >> obj.chip_bump_inst_;
  stream >> obj.capacitance_;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbChipCapNode& obj)
{
  stream << obj.chip_net_;
  stream << obj.next_chip_cap_node_;
  stream << obj.chip_bump_inst_;
  stream << obj.capacitance_;
  return stream;
}

void _dbChipCapNode::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);
}

////////////////////////////////////////////////////////////////////
//
// dbChipCapNode - Methods
//
////////////////////////////////////////////////////////////////////

void dbChipCapNode::setCapacitance(float capacitance)
{
  _dbChipCapNode* obj = (_dbChipCapNode*) this;

  obj->capacitance_ = capacitance;
}

float dbChipCapNode::getCapacitance() const
{
  _dbChipCapNode* obj = (_dbChipCapNode*) this;
  return obj->capacitance_;
}

// User Code Begin dbChipCapNodePublicMethods
dbChipCapNode* dbChipCapNode::create(dbChipNet* chip_net_)
{
  _dbChipNet* chip_net = (_dbChipNet*) chip_net_;
  _dbChip* chip = (_dbChip*) chip_net_->getChip();

  _dbChipCapNode* cap_node = chip->chip_cap_node_tbl_->create();
  cap_node->chip_net_ = chip_net->getOID();

  // Add to the chip net's cap node list
  cap_node->next_chip_cap_node_ = chip_net->first_cap_node_;
  chip_net->first_cap_node_ = cap_node->getOID();

  return (dbChipCapNode*) cap_node;
}

void dbChipCapNode::destroy(dbChipCapNode* chip_cap_node_)
{
  _dbChipCapNode* chip_cap_node = (_dbChipCapNode*) chip_cap_node_;
  _dbChip* chip = (_dbChip*) chip_cap_node->getOwner();
  _dbDatabase* db = chip->getDatabase();
  _dbChipNet* chip_net = db->chip_net_tbl_->getPtr(chip_cap_node->chip_net_);

  // Remove the deleted cap node from the chip net's cap node list
  if (chip_net->first_cap_node_ == chip_cap_node->getOID()) {
    chip_net->first_cap_node_ = chip_cap_node->next_chip_cap_node_;
  } else {
    _dbChipCapNode* previous_cap_node = nullptr;
    _dbChipCapNode* current_cap_node
        = chip->chip_cap_node_tbl_->getPtr(chip_net->first_cap_node_);
    while (current_cap_node && current_cap_node != chip_cap_node) {
      previous_cap_node = current_cap_node;
      current_cap_node = chip->chip_cap_node_tbl_->getPtr(
          current_cap_node->next_chip_cap_node_);
    }
    if (current_cap_node && previous_cap_node) {
      previous_cap_node->next_chip_cap_node_
          = current_cap_node->next_chip_cap_node_;
    }
  }

  chip->chip_cap_node_tbl_->destroy(chip_cap_node);
}

dbChipNet* dbChipCapNode::getChipNet() const
{
  _dbChipCapNode* chip_cap_node = (_dbChipCapNode*) this;
  _dbChip* chip = (_dbChip*) chip_cap_node->getOwner();
  _dbDatabase* db = chip->getDatabase();
  _dbChipNet* chip_net = db->chip_net_tbl_->getPtr(chip_cap_node->chip_net_);
  return (dbChipNet*) chip_net;
}

dbChipBumpInst* dbChipCapNode::getChipBumpInst() const
{
  _dbChipCapNode* chip_cap_node = (_dbChipCapNode*) this;

  if (chip_cap_node->chip_bump_inst_ == 0) {
    return nullptr;
  }

  _dbChip* chip = (_dbChip*) chip_cap_node->getOwner();
  _dbDatabase* db = chip->getDatabase();

  return (dbChipBumpInst*) db->chip_bump_inst_tbl_->getPtr(
      chip_cap_node->chip_bump_inst_);
}

void dbChipCapNode::setChipBumpInst(dbChipBumpInst* chip_bump_inst)
{
  _dbChipCapNode* chip_cap_node = (_dbChipCapNode*) this;
  chip_cap_node->chip_bump_inst_ = chip_bump_inst->getId();
}

dbBTerm* dbChipCapNode::getBTerm() const
{
  dbChipBumpInst* chip_bump_inst = getChipBumpInst();
  if (!chip_bump_inst) {
    return nullptr;
  }
  return chip_bump_inst->getChipBump()->getBTerm();
}

// User Code End dbChipCapNodePublicMethods
}  // namespace odb
// Generator Code End Cpp