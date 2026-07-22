// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbObstruction.h"

#include <cstdint>
#include <cstring>

#include "dbBlock.h"
#include "dbBox.h"
#include "dbCore.h"
#include "dbDatabase.h"
#include "dbInst.h"
#include "dbTable.h"
#include "dbTechLayer.h"
#include "odb/db.h"
#include "odb/dbBlockCallBackObj.h"
#include "odb/dbSet.h"
#include "odb/dbTypes.h"
#include "utl/Logger.h"

namespace odb {

template class dbTable<_dbObstruction>;

_dbObstruction::_dbObstruction(_dbDatabase*, const _dbObstruction& o)
    : flags_(o.flags_),
      inst_(o.inst_),
      bbox_(o.bbox_),
      min_spacing_(o.min_spacing_),
      effective_width_(o.effective_width_)
{
}

_dbObstruction::_dbObstruction(_dbDatabase*)
{
  flags_.slot_obs = 0;
  flags_.fill_obs = 0;
  flags_.except_pg_nets = 0;
  flags_.pushed_down = 0;
  flags_.has_min_spacing = 0;
  flags_.has_effective_width = 0;
  flags_.spare_bits = 0;
  flags_.is_system_reserved = 0;
  min_spacing_ = 0;
  effective_width_ = 0;
}

dbOStream& operator<<(dbOStream& stream, const _dbObstruction& obs)
{
  uint32_t* bit_field = (uint32_t*) &obs.flags_;
  stream << *bit_field;
  stream << obs.inst_;
  stream << obs.bbox_;
  stream << obs.min_spacing_;
  stream << obs.effective_width_;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, _dbObstruction& obs)
{
  uint32_t* bit_field = (uint32_t*) &obs.flags_;
  stream >> *bit_field;
  stream >> obs.inst_;
  stream >> obs.bbox_;
  stream >> obs.min_spacing_;
  stream >> obs.effective_width_;

  _dbDatabase* db = obs.getImpl()->getDatabase();
  if (!db->isSchema(kSchemaExceptPgNetsObstruction)) {
    // assume false for older databases
    obs.flags_.except_pg_nets = false;
  }

  if (!db->isSchema(kSchemaDieAreaIsPolygon)) {
    // assume false for older databases
    obs.flags_.is_system_reserved = false;
  }

  return stream;
}

bool _dbObstruction::operator==(const _dbObstruction& rhs) const
{
  if (flags_.slot_obs != rhs.flags_.slot_obs) {
    return false;
  }

  if (flags_.fill_obs != rhs.flags_.fill_obs) {
    return false;
  }

  if (flags_.except_pg_nets != rhs.flags_.except_pg_nets) {
    return false;
  }

  if (flags_.pushed_down != rhs.flags_.pushed_down) {
    return false;
  }

  if (flags_.has_min_spacing != rhs.flags_.has_min_spacing) {
    return false;
  }

  if (flags_.has_effective_width != rhs.flags_.has_effective_width) {
    return false;
  }

  if (inst_ != rhs.inst_) {
    return false;
  }

  if (bbox_ != rhs.bbox_) {
    return false;
  }

  return true;
}

bool _dbObstruction::operator<(const _dbObstruction& rhs) const
{
  _dbBlock* lhs_block = (_dbBlock*) getOwner();
  _dbBlock* rhs_block = (_dbBlock*) rhs.getOwner();
  _dbBox* lhs_box = lhs_block->box_tbl_->getPtr(bbox_);
  _dbBox* rhs_box = rhs_block->box_tbl_->getPtr(rhs.bbox_);

  if (*lhs_box < *rhs_box) {
    return true;
  }

  if (lhs_box->equal(*rhs_box)) {
    if (inst_ && rhs.inst_) {
      _dbBlock* lhs_blk = (_dbBlock*) getOwner();
      _dbBlock* rhs_blk = (_dbBlock*) rhs.getOwner();
      _dbInst* lhs_inst = lhs_blk->inst_tbl_->getPtr(inst_);
      _dbInst* rhs_inst = rhs_blk->inst_tbl_->getPtr(rhs.inst_);
      int r = strcmp(lhs_inst->name_, rhs_inst->name_);

      if (r < 0) {
        return true;
      }

      if (r > 0) {
        return false;
      }
    } else if (inst_) {
      return false;
    } else if (rhs.inst_) {
      return true;
    }

    if (flags_.slot_obs < rhs.flags_.slot_obs) {
      return true;
    }

    if (flags_.slot_obs > rhs.flags_.slot_obs) {
      return false;
    }

    if (flags_.fill_obs < rhs.flags_.fill_obs) {
      return true;
    }

    if (flags_.fill_obs > rhs.flags_.fill_obs) {
      return false;
    }

    if (flags_.except_pg_nets < rhs.flags_.except_pg_nets) {
      return true;
    }

    if (flags_.except_pg_nets > rhs.flags_.except_pg_nets) {
      return false;
    }

    if (flags_.pushed_down < rhs.flags_.pushed_down) {
      return true;
    }

    if (flags_.pushed_down > rhs.flags_.pushed_down) {
      return false;
    }

    if (flags_.has_min_spacing < rhs.flags_.has_min_spacing) {
      return true;
    }

    if (flags_.has_min_spacing > rhs.flags_.has_min_spacing) {
      return false;
    }

    if (flags_.has_effective_width < rhs.flags_.has_effective_width) {
      return true;
    }

    if (flags_.has_effective_width > rhs.flags_.has_effective_width) {
      return false;
    }

    if (min_spacing_ < rhs.min_spacing_) {
      return true;
    }

    if (min_spacing_ > rhs.min_spacing_) {
      return false;
    }

    if (effective_width_ < rhs.effective_width_) {
      return true;
    }

    if (effective_width_ > rhs.effective_width_) {
      return false;
    }
  }

  return false;
}

////////////////////////////////////////////////////////////////////
//
// dbObstruction - Methods
//
////////////////////////////////////////////////////////////////////

dbBox* dbObstruction::getBBox()
{
  _dbObstruction* obs = (_dbObstruction*) this;
  _dbBlock* block = (_dbBlock*) obs->getOwner();
  return (dbBox*) block->box_tbl_->getPtr(obs->bbox_);
}

dbInst* dbObstruction::getInstance()
{
  _dbObstruction* obs = (_dbObstruction*) this;

  if (obs->inst_ == 0) {
    return nullptr;
  }

  _dbBlock* block = (_dbBlock*) obs->getOwner();
  return (dbInst*) block->inst_tbl_->getPtr(obs->inst_);
}

void dbObstruction::setSlotObstruction()
{
  _dbObstruction* obs = (_dbObstruction*) this;
  obs->flags_.slot_obs = 1;
}

bool dbObstruction::isSlotObstruction()
{
  _dbObstruction* obs = (_dbObstruction*) this;
  return obs->flags_.slot_obs == 1;
}

void dbObstruction::setFillObstruction()
{
  _dbObstruction* obs = (_dbObstruction*) this;
  obs->flags_.fill_obs = 1;
}

bool dbObstruction::isFillObstruction()
{
  _dbObstruction* obs = (_dbObstruction*) this;
  return obs->flags_.fill_obs == 1;
}

void dbObstruction::setExceptPGNetsObstruction()
{
  _dbObstruction* obs = (_dbObstruction*) this;
  obs->flags_.except_pg_nets = 1;
}

bool dbObstruction::isExceptPGNetsObstruction()
{
  _dbObstruction* obs = (_dbObstruction*) this;
  return obs->flags_.except_pg_nets == 1;
}

void dbObstruction::setPushedDown()
{
  _dbObstruction* obs = (_dbObstruction*) this;
  obs->flags_.pushed_down = 1;
}

bool dbObstruction::isPushedDown()
{
  _dbObstruction* obs = (_dbObstruction*) this;
  return obs->flags_.pushed_down == 1;
}

bool dbObstruction::hasEffectiveWidth()
{
  _dbObstruction* obs = (_dbObstruction*) this;
  return obs->flags_.has_effective_width == 1U;
}

void dbObstruction::setEffectiveWidth(int w)
{
  _dbObstruction* obs = (_dbObstruction*) this;
  obs->flags_.has_effective_width = 1U;
  obs->effective_width_ = w;
}

int dbObstruction::getEffectiveWidth()
{
  _dbObstruction* obs = (_dbObstruction*) this;
  return obs->effective_width_;
}

bool dbObstruction::hasMinSpacing()
{
  _dbObstruction* obs = (_dbObstruction*) this;
  return obs->flags_.has_min_spacing == 1U;
}

void dbObstruction::setMinSpacing(int w)
{
  _dbObstruction* obs = (_dbObstruction*) this;
  obs->flags_.has_min_spacing = 1U;
  obs->min_spacing_ = w;
}

int dbObstruction::getMinSpacing()
{
  _dbObstruction* obs = (_dbObstruction*) this;
  return obs->min_spacing_;
}

dbBlock* dbObstruction::getBlock()
{
  return (dbBlock*) getImpl()->getOwner();
}

bool dbObstruction::isSystemReserved()
{
  _dbObstruction* obs = (_dbObstruction*) this;
  return obs->flags_.is_system_reserved;
}

void dbObstruction::setIsSystemReserved(bool is_system_reserved)
{
  _dbObstruction* obs = (_dbObstruction*) this;
  obs->flags_.is_system_reserved = is_system_reserved;
}

dbObstruction* dbObstruction::create(dbBlock* block_,
                                     dbTechLayer* layer_,
                                     int x1,
                                     int y1,
                                     int x2,
                                     int y2,
                                     dbInst* inst_)
{
  _dbBlock* block = (_dbBlock*) block_;
  _dbTechLayer* layer = (_dbTechLayer*) layer_;
  _dbInst* inst = (_dbInst*) inst_;

  _dbObstruction* obs = block->obstruction_tbl_->create();

  if (inst) {
    obs->inst_ = inst->getOID();
  }

  _dbBox* box = block->box_tbl_->create();
  box->shape_.rect.init(x1, y1, x2, y2);
  box->flags_.owner_type = dbBoxOwner::OBSTRUCTION;
  box->owner_ = obs->getOID();
  box->flags_.layer_id = layer->getOID();
  obs->bbox_ = box->getOID();

  // Update bounding box of block
  block->add_rect(box->shape_.rect);
  for (auto callback : block->callbacks_) {
    callback->inDbObstructionCreate((dbObstruction*) obs);
  }
  return (dbObstruction*) obs;
}

void dbObstruction::destroy(dbObstruction* obstruction)
{
  _dbObstruction* obs = (_dbObstruction*) obstruction;
  _dbBlock* block = (_dbBlock*) obs->getOwner();

  if (obstruction->isSystemReserved()) {
    utl::Logger* logger = block->getLogger();
    logger->error(
        utl::ODB,
        1111,
        "You cannot delete a system created obstruction (isSystemReserved).");
  }

  for (auto callback : block->callbacks_) {
    callback->inDbObstructionDestroy(obstruction);
  }
  dbProperty::destroyProperties(obs);
  block->obstruction_tbl_->destroy(obs);
}

dbSet<dbObstruction>::iterator dbObstruction::destroy(
    dbSet<dbObstruction>::iterator& itr)
{
  dbObstruction* bt = *itr;
  dbSet<dbObstruction>::iterator next = ++itr;
  destroy(bt);
  return next;
}

dbObstruction* dbObstruction::getObstruction(dbBlock* block_, uint32_t dbid_)
{
  _dbBlock* block = (_dbBlock*) block_;
  return (dbObstruction*) block->obstruction_tbl_->getPtr(dbid_);
}

void _dbObstruction::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);
}
}  // namespace odb
