// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "odb/defin.h"

#include <mutex>
#include <vector>

#include "definReader.h"
#include "odb/db.h"

namespace odb {

// Protects the DefParser namespace that has static variables
std::mutex defin::def_mutex_;

defin::defin(dbDatabase* db, utl::Logger* logger, MODE mode)
{
  reader_ = new definReader(db, logger, mode);
}

defin::~defin()
{
  delete reader_;
}

void defin::skipConnections()
{
  reader_->skipConnections();
}

void defin::skipWires()
{
  reader_->skipWires();
}

void defin::skipSpecialWires()
{
  reader_->skipSpecialWires();
}

void defin::skipShields()
{
  reader_->skipShields();
}

void defin::skipBlockWires()
{
  reader_->skipBlockWires();
}

void defin::skipFillWires()
{
  reader_->skipFillWires();
}

void defin::continueOnErrors()
{
  reader_->continueOnErrors();
}

void defin::useBlockName(const char* name)
{
  reader_->useBlockName(name);
}

void defin::readChip(std::vector<dbLib*>& libs,
                     const char* def_file,
                     dbChip* chip,
                     const bool issue_callback)
{
  std::lock_guard<std::mutex> lock(def_mutex_);
  reader_->readChip(libs, def_file, chip, issue_callback);
}

}  // namespace odb
