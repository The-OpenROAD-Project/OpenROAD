// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#pragma once

////////////////////////////////////////////////////////////////////////////////
// Includes.
////////////////////////////////////////////////////////////////////////////////
#include <string>
#include <vector>

#include "architecture.h"
#include "detailed_manager.h"
#include "detailed_segment.h"
#include "network.h"

namespace dpo {

////////////////////////////////////////////////////////////////////////////////
// Forward declarations.
////////////////////////////////////////////////////////////////////////////////
class Journal;

////////////////////////////////////////////////////////////////////////////////
// Classes.
////////////////////////////////////////////////////////////////////////////////
class DetailedObjective
{
 public:
  explicit DetailedObjective(const char* name = "objective") : name_(name) {}
  virtual ~DetailedObjective() = default;

  virtual const std::string& getName() const { return name_; }

  virtual double curr() = 0;

  virtual double delta(const Journal& journal) = 0;
  virtual void accept() {}
  virtual void reject() {}

 private:
  const std::string name_;
};

}  // namespace dpo
