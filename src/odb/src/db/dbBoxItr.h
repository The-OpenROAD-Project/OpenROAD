// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include "dbCore.h"
#include "odb/dbId.h"
#include "odb/dbIterator.h"
#include "odb/odb.h"

namespace odb {

class _dbBox;
class _dbPolygon;

template <uint page_size>
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
  uint sequential() const override;
  uint size(dbObject* parent) const override;
  uint begin(dbObject* parent) const override;
  uint end(dbObject* parent) const override;
  uint next(uint id, ...) const override;
  dbObject* getObject(uint id, ...) override;

 private:
  dbTable<_dbBox, page_size>* box_tbl_;
  dbTable<_dbPolygon, page_size>* pbox_tbl_;
  bool include_polygons_;
};

}  // namespace odb
