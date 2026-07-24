// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbChipNetRSegItr.h"

#include <cstdint>

#include "dbChipNet.h"
#include "dbChipRSeg.h"
#include "dbTable.h"

namespace odb {

////////////////////////////////////////////////////////////////////
//
// dbChipNetRSegItr - Methods
//
////////////////////////////////////////////////////////////////////

bool dbChipNetRSegItr::reversible() const
{
  return true;
}

bool dbChipNetRSegItr::orderReversed() const
{
  return true;
}

void dbChipNetRSegItr::reverse(dbObject* parent)
{
  // User Code Begin reverse
  _dbChipNet* net = (_dbChipNet*) parent;
  uint32_t current_id = net->first_r_seg_;
  uint32_t new_head = 0;

  while (current_id != 0) {
    _dbChipRSeg* current_r_seg = chip_r_seg_tbl_->getPtr(current_id);
    const uint32_t next_id = current_r_seg->next_chip_r_seg_;

    current_r_seg->next_chip_r_seg_ = new_head;
    new_head = current_id;
    current_id = next_id;
  }

  net->first_r_seg_ = new_head;
  // User Code End reverse
}

uint32_t dbChipNetRSegItr::sequential() const
{
  return 0;
}

uint32_t dbChipNetRSegItr::size(dbObject* parent) const
{
  uint32_t id;
  uint32_t cnt = 0;

  for (id = dbChipNetRSegItr::begin(parent);
       id != dbChipNetRSegItr::end(parent);
       id = dbChipNetRSegItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint32_t dbChipNetRSegItr::begin(dbObject* parent) const
{
  // User Code Begin begin
  _dbChipNet* chip_net = (_dbChipNet*) parent;
  return (uint32_t) chip_net->first_r_seg_;
  // User Code End begin
}

uint32_t dbChipNetRSegItr::end(dbObject* /* unused: parent */) const
{
  return 0;
}

uint32_t dbChipNetRSegItr::next(uint32_t id, ...) const
{
  // User Code Begin next
  _dbChipRSeg* current_r_seg = chip_r_seg_tbl_->getPtr(id);
  return (uint32_t) current_r_seg->next_chip_r_seg_;
  // User Code End next
}

dbObject* dbChipNetRSegItr::getObject(uint32_t id, ...)
{
  return chip_r_seg_tbl_->getPtr(id);
}
}  // namespace odb
// Generator Code End Cpp