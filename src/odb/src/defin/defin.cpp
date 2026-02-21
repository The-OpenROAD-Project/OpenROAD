// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "odb/defin.h"

#include <vector>

#include "absl/base/attributes.h"
#include "absl/base/const_init.h"
#include "absl/synchronization/mutex.h"
#include "definReader.h"
#include "odb/db.h"

namespace odb {

// Protects the DefParser namespace that has static variables
ABSL_CONST_INIT absl::Mutex defin::def_mutex_(absl::kConstInit);

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
  absl::MutexLock lock(&def_mutex_);
  reader_->readChip(libs, def_file, chip, issue_callback);
}

}  // namespace odb
