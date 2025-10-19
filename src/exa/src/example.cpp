// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include "exa/example.h"

#include <memory>
#include <utility>

#include "observer.h"
#include "odb/db.h"
#include "utl/Logger.h"

namespace exa {

// These need to be defined in the cpp due to the smart pointer in
// example.h to Observer which is only declared there.
Example::Example(odb::dbDatabase* db, utl::Logger* logger)
    : db_(db), logger_(logger)
{
}

Example::~Example() = default;

// Checks that a block exists and errors if not.  error() will throw an
// exception with the message.
odb::dbBlock* Example::getBlock()
{
  odb::dbChip* chip = db_->getChip();
  if (!chip) {
    logger_->error(utl::EXA, 2, "No chip exists.");
  }

  odb::dbBlock* block = chip->getBlock();
  if (!block) {
    logger_->error(utl::EXA, 3, "No block exists.");
  }

  return block;
}

// The example operation.
void Example::makeInstance(const char* name)
{
  logger_->info(utl::EXA, 1, "Making an example instance named {}", name);

  odb::dbBlock* block = getBlock();

  // Find an arbitrary master to instantiate
  odb::dbMaster* master = nullptr;
  for (odb::dbLib* lib : db_->getLibs()) {
    if (!lib->getMasters().empty()) {
      master = *lib->getMasters().begin();
      break;
    }
  }

  if (!master) {
    logger_->error(utl::EXA, 4, "No master found.");
  }

  // Make an instance and mark the instance as placed.  The default
  // is at (0, 0) with R0 orientation.
  odb::dbInst* inst = odb::dbInst::create(block, master, name);
  inst->setPlacementStatus(odb::dbPlacementStatus::PLACED);

  // Notify the observer that something interesting has happened.
  if (debug_observer_) {
    debug_observer_->makeInstance(inst);
  }
}

void Example::setDebug(std::unique_ptr<Observer>& observer)
{
  debug_observer_ = std::move(observer);
}

}  // namespace exa
