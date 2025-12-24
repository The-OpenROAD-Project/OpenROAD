// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cstdint>

#include "dbCore.h"
#include "odb/dbIterator.h"

namespace odb {

class _dbITerm;

class dbNetITermItr : public dbIterator
{
 public:
  dbNetITermItr(dbTable<_dbITerm, 1024>* iterm_tbl) { iterm_tbl_ = iterm_tbl; }

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
  dbTable<_dbITerm, 1024>* iterm_tbl_;
};

class dbInstITermItr : public dbIterator
{
  dbTable<_dbITerm, 1024>* _iterm_tbl;

 public:
  dbInstITermItr(dbTable<_dbITerm, 1024>* iterm_tbl) { _iterm_tbl = iterm_tbl; }

  bool reversible() const override;
  bool orderReversed() const override;
  void reverse(dbObject* parent) override;
  uint32_t sequential() const override;
  uint32_t size(dbObject* parent) const override;
  uint32_t begin(dbObject* parent) const override;
  uint32_t end(dbObject* parent) const override;
  uint32_t next(uint32_t id, ...) const override;
  dbObject* getObject(uint32_t id, ...) override;
};

}  // namespace odb
