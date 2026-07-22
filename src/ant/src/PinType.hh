// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#pragma once

#include <cstddef>
#include <string>
#include <utility>

#include "odb/db.h"

namespace ant {

struct PinType
{
  bool is_iterm;
  std::string name;
  union
  {
    odb::dbITerm* iterm;
    odb::dbBTerm* bterm;
  };
  PinType(std::string name, odb::dbITerm* iterm)
      : is_iterm(true), name(std::move(name)), iterm(iterm)
  {
  }
  PinType(std::string name, odb::dbBTerm* bterm)
      : is_iterm(false), name(std::move(name)), bterm(bterm)
  {
  }
  bool operator==(const PinType& t) const { return (this->name == t.name); }
};

class PinTypeCmp
{
 public:
  size_t operator()(const PinType& t1, const PinType& t2) const
  {
    return t1.name < t2.name;
  }
};

}  // namespace ant
