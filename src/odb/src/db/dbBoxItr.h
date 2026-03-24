// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cstdint>

#include "dbCore.h"
#include "odb/dbId.h"
#include "odb/dbIterator.h"

namespace odb {

class _dbBox;
class _dbPolygon;

template <uint32_t page_size>
class dbBoxItr : public dbIterator
{
 public:
  dbBoxItr(dbTable<_dbBox, page_size>* box_tbl,
           dbTable<_dbPolygon, page_size>* pbox_tbl,
           bool include_polygons)
  {
    box_tbl_ = box_tbl;
    pbox_tbl_ = pbox_tbl;
    include_polygons_ = include_polygons;
  }

  bool reversible() const override;
  bool orderReversed() const override;
  void reverse(dbObject* parent) override;
  uint32_t sequential() const override;
  uint32_t size(dbObject* parent) const override;
  uint32_t begin(dbObject* parent) const override;
  uint32_t end(dbObject* parent) const override;
  uint32_t next(uint32_t id, ...) const override;
  dbObject* getObject(uint32_t id, ...) override;

 private:
  dbTable<_dbBox, page_size>* box_tbl_;
  dbTable<_dbPolygon, page_size>* pbox_tbl_;
  bool include_polygons_;
};

}  // namespace odb
