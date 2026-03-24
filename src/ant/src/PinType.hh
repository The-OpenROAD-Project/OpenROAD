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
  bool isITerm;
  std::string name;
  union
  {
    odb::dbITerm* iterm;
    odb::dbBTerm* bterm;
  };
  PinType(std::string name_, odb::dbITerm* iterm_)
  {
    name = std::move(name_);
    iterm = iterm_;
    isITerm = true;
  }
  PinType(std::string name_, odb::dbBTerm* bterm_)
  {
    name = std::move(name_);
    bterm = bterm_;
    isITerm = false;
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
