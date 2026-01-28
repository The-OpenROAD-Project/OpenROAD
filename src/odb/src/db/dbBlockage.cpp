// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbBlockage.h"

#include <cstdint>
#include <cstring>

#include "dbBlock.h"
#include "dbBox.h"
#include "dbCore.h"
#include "dbDatabase.h"
#include "dbInst.h"
#include "dbTable.h"
#include "odb/db.h"
#include "odb/dbBlockCallBackObj.h"
#include "odb/dbSet.h"
#include "odb/dbTypes.h"
#include "utl/Logger.h"

namespace odb {

template class dbTable<_dbBlockage>;

_dbBox* _dbBlockage::getBBox() const
{
  _dbBlock* block = (_dbBlock*) getOwner();
  return block->box_tbl_->getPtr(bbox_);
}

_dbInst* _dbBlockage::getInst()
{
  if (inst_ == 0) {
    return nullptr;
  }

  _dbBlock* block = (_dbBlock*) getOwner();
  return block->inst_tbl_->getPtr(inst_);
}

bool _dbBlockage::operator==(const _dbBlockage& rhs) const
{
  if (flags_.pushed_down != rhs.flags_.pushed_down) {
    return false;
  }

  if (flags_.soft != rhs.flags_.soft) {
    return false;
  }

  if (inst_ != rhs.inst_) {
    return false;
  }

  if (bbox_ != rhs.bbox_) {
    return false;
  }

  if (max_density_ != rhs.max_density_) {
    return false;
  }

  return true;
}

bool _dbBlockage::operator<(const _dbBlockage& rhs) const
{
  _dbBox* b0 = getBBox();
  _dbBox* b1 = rhs.getBBox();

  if (*b0 < *b1) {
    return true;
  }

  if (b0->equal(*b1)) {
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

    if (flags_.pushed_down < rhs.flags_.pushed_down) {
      return true;
    }

    if (flags_.pushed_down > rhs.flags_.pushed_down) {
      return false;
    }

    if (flags_.soft < rhs.flags_.soft) {
      return true;
    }

    if (flags_.soft > rhs.flags_.soft) {
      return false;
    }

    if (max_density_ < rhs.max_density_) {
      return true;
    }

    if (max_density_ > rhs.max_density_) {
      return false;
    }
  }

  return false;
}

////////////////////////////////////////////////////////////////////
//
// dbBlockage - Methods
//
////////////////////////////////////////////////////////////////////

dbBox* dbBlockage::getBBox()
{
  _dbBlockage* bkg = (_dbBlockage*) this;
  return (dbBox*) bkg->getBBox();
}

dbInst* dbBlockage::getInstance()
{
  _dbBlockage* bkg = (_dbBlockage*) this;
  return (dbInst*) bkg->getInst();
}

void dbBlockage::setPushedDown()
{
  _dbBlockage* bkg = (_dbBlockage*) this;
  bkg->flags_.pushed_down = 1;
}

bool dbBlockage::isPushedDown()
{
  _dbBlockage* bkg = (_dbBlockage*) this;
  return bkg->flags_.pushed_down == 1;
}

void dbBlockage::setSoft()
{
  _dbBlockage* bkg = (_dbBlockage*) this;
  bkg->flags_.soft = 1;
}

bool dbBlockage::isSoft()
{
  _dbBlockage* bkg = (_dbBlockage*) this;
  return bkg->flags_.soft == 1;
}

void dbBlockage::setMaxDensity(float max_density)
{
  _dbBlockage* bkg = (_dbBlockage*) this;
  bkg->max_density_ = max_density;
}

float dbBlockage::getMaxDensity()
{
  _dbBlockage* bkg = (_dbBlockage*) this;
  return bkg->max_density_;
}

dbBlock* dbBlockage::getBlock()
{
  return (dbBlock*) getImpl()->getOwner();
}

bool dbBlockage::isSystemReserved()
{
  _dbBlockage* bkg = (_dbBlockage*) this;
  return bkg->flags_.is_system_reserved;
}

void dbBlockage::setIsSystemReserved(bool is_system_reserved)
{
  _dbBlockage* bkg = (_dbBlockage*) this;
  bkg->flags_.is_system_reserved = is_system_reserved;
}

dbBlockage* dbBlockage::create(dbBlock* block_,
                               int x1,
                               int y1,
                               int x2,
                               int y2,
                               dbInst* inst_)
{
  _dbBlock* block = (_dbBlock*) block_;
  _dbInst* inst = (_dbInst*) inst_;

  _dbBlockage* bkg = block->blockage_tbl_->create();

  if (inst) {
    bkg->inst_ = inst->getOID();
  }

  _dbBox* box = block->box_tbl_->create();
  box->shape_.rect.init(x1, y1, x2, y2);
  box->flags_.owner_type = dbBoxOwner::BLOCKAGE;
  box->owner_ = bkg->getOID();
  bkg->bbox_ = box->getOID();

  // Update bounding box of block
  _dbBox* bbox = (_dbBox*) block->box_tbl_->getPtr(block->bbox_);
  bbox->shape_.rect.merge(box->shape_.rect);
  for (auto callback : block->callbacks_) {
    callback->inDbBlockageCreate((dbBlockage*) bkg);
  }
  return (dbBlockage*) bkg;
}

void dbBlockage::destroy(dbBlockage* blockage)
{
  _dbBlockage* bkg = (_dbBlockage*) blockage;
  _dbBlock* block = (_dbBlock*) blockage->getBlock();

  if (blockage->isSystemReserved()) {
    utl::Logger* logger = block->getLogger();
    logger->error(
        utl::ODB,
        1112,
        "You cannot delete a system created blockage (isSystemReserved).");
  }

  for (auto callback : block->callbacks_) {
    callback->inDbBlockageDestroy(blockage);
  }

  block->box_tbl_->destroy(block->box_tbl_->getPtr(bkg->bbox_));

  block->blockage_tbl_->destroy(bkg);
}

dbBlockage* dbBlockage::getBlockage(dbBlock* block_, uint32_t dbid_)
{
  _dbBlock* block = (_dbBlock*) block_;
  return (dbBlockage*) block->blockage_tbl_->getPtr(dbid_);
}

void _dbBlockage::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);
}

dbSet<dbBlockage>::iterator dbBlockage::destroy(
    dbSet<dbBlockage>::iterator& itr)
{
  dbBlockage* bt = *itr;
  dbSet<dbBlockage>::iterator next = ++itr;
  destroy(bt);
  return next;
}

}  // namespace odb
