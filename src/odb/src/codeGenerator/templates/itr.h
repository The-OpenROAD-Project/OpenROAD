{% import 'macros.jinja' as macros %}
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

//Generator Code Begin Header
#pragma once

#include "odb/dbIterator.h"
#include "odb/odb.h"
#include "dbCore.h"
// User Code Begin Includes
// User Code End Includes

namespace odb {
  class _{{itr.parentObject}};

  //User Code Begin classes
  //User Code End classes

  class {{itr.name}} : public dbIterator
  {
  public:
    {{itr.name}}({{macros.table_type(itr)}}* {{itr.tableName}}) { _{{itr.tableName}} = {{itr.tableName}}; }

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
    {{macros.table_type(itr)}}* _{{itr.tableName}};
    // User Code Begin Fields
    // User Code End Fields
  };

  
}
//Generator Code End Header
