// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

//Generator Code Begin Header
#pragma once

#include "odb/dbIterator.h"
#include "odb/odb.h"
// User Code Begin Includes
// User Code End Includes

namespace odb {
  class _{{itr.parentObject}};

  template <class T>
  class dbTable;

  //User Code Begin classes
  //User Code End classes

  class {{itr.name}} : public dbIterator
  {
  public:
    {{itr.name}}(dbTable<_{{itr.parentObject}}>* {{itr.tableName}}) { _{{itr.tableName}} = {{itr.tableName}}; }

    bool      reversible() override;
    bool      orderReversed() override;
    void      reverse(dbObject* parent) override;
    uint      sequential() override;
    uint      size(dbObject* parent) override;
    uint      begin(dbObject* parent) override;
    uint      end(dbObject* parent) override;
    uint      next(uint id, ...) override;
    dbObject* getObject(uint id, ...) override;
    // User Code Begin Methods
    // User Code End Methods
  private:
    dbTable<_{{itr.parentObject}}>* _{{itr.tableName}};
    // User Code Begin Fields
    // User Code End Fields
  };

  
}
//Generator Code End Header
