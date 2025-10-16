// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include <memory>

#include "odb/db.h"
#include "utl/Logger.h"

namespace exa {

class Observer;

// A simple example tool with a single operation for demonstration.

class Example
{
 public:
  Example(odb::dbDatabase* db, utl::Logger* logger);
  ~Example();

  // The example operation that makes an instance.
  void makeInstance(const char* name);

  // Register an observer for debug graphics
  void setDebug(std::unique_ptr<Observer>& observer);

 private:
  // Simple helper method to get the block from the db_.
  odb::dbBlock* getBlock();

  odb::dbDatabase* db_ = nullptr;
  utl::Logger* logger_ = nullptr;

  std::unique_ptr<Observer> debug_observer_;
};

}  // namespace exa
