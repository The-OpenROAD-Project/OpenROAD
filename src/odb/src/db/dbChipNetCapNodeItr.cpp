// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbChipNetCapNodeItr.h"

#include <cstdint>

#include "dbChipCapNode.h"
#include "dbChipNet.h"
#include "dbTable.h"

namespace odb {

////////////////////////////////////////////////////////////////////
//
// dbChipNetCapNodeItr - Methods
//
////////////////////////////////////////////////////////////////////

bool dbChipNetCapNodeItr::reversible() const
{
  return true;
}

bool dbChipNetCapNodeItr::orderReversed() const
{
  return true;
}

void dbChipNetCapNodeItr::reverse(dbObject* parent)
{
  // User Code Begin reverse
  _dbChipNet* net = (_dbChipNet*) parent;
  uint32_t current_id = net->first_cap_node_;
  uint32_t new_head = 0;

  while (current_id != 0) {
    _dbChipCapNode* current_cap_node = chip_cap_node_tbl_->getPtr(current_id);
    const uint32_t next_id = current_cap_node->next_chip_cap_node_;

    current_cap_node->next_chip_cap_node_ = new_head;
    new_head = current_id;
    current_id = next_id;
  }

  net->first_cap_node_ = new_head;
  // User Code End reverse
}

uint32_t dbChipNetCapNodeItr::sequential() const
{
  return 0;
}

uint32_t dbChipNetCapNodeItr::size(dbObject* parent) const
{
  uint32_t id;
  uint32_t cnt = 0;

  for (id = dbChipNetCapNodeItr::begin(parent);
       id != dbChipNetCapNodeItr::end(parent);
       id = dbChipNetCapNodeItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint32_t dbChipNetCapNodeItr::begin(dbObject* parent) const
{
  // User Code Begin begin
  _dbChipNet* chip_net = (_dbChipNet*) parent;
  return (uint32_t) chip_net->first_cap_node_;
  // User Code End begin
}

uint32_t dbChipNetCapNodeItr::end(dbObject* /* unused: parent */) const
{
  return 0;
}

uint32_t dbChipNetCapNodeItr::next(uint32_t id, ...) const
{
  // User Code Begin next
  _dbChipCapNode* current_cap_node = chip_cap_node_tbl_->getPtr(id);
  return (uint32_t) current_cap_node->next_chip_cap_node_;
  // User Code End next
}

dbObject* dbChipNetCapNodeItr::getObject(uint32_t id, ...)
{
  return chip_cap_node_tbl_->getPtr(id);
}
}  // namespace odb
// Generator Code End Cpp