// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "odb/defin.h"

#include <vector>

#include "definReader.h"
#include "odb/db.h"

namespace odb {

// Protects the DefParser namespace that has static variables
std::mutex defin::_def_mutex;

defin::defin(dbDatabase* db, utl::Logger* logger, MODE mode)
{
  _reader = new definReader(db, logger, mode);
}

defin::~defin()
{
  delete _reader;
}

void defin::skipConnections()
{
  _reader->skipConnections();
}

void defin::skipWires()
{
  _reader->skipWires();
}

void defin::skipSpecialWires()
{
  _reader->skipSpecialWires();
}

void defin::skipShields()
{
  _reader->skipShields();
}

void defin::skipBlockWires()
{
  _reader->skipBlockWires();
}

void defin::skipFillWires()
{
  _reader->skipFillWires();
}

void defin::continueOnErrors()
{
  _reader->continueOnErrors();
}

void defin::useBlockName(const char* name)
{
  _reader->useBlockName(name);
}

dbChip* defin::createChip(std::vector<dbLib*>& libs,
                          const char* def_file,
                          dbTech* tech)
{
  std::lock_guard<std::mutex> lock(_def_mutex);
  return _reader->createChip(libs, def_file, tech);
}

dbBlock* defin::createBlock(dbBlock* parent,
                            std::vector<dbLib*>& libs,
                            const char* def_file,
                            dbTech* tech)
{
  std::lock_guard<std::mutex> lock(_def_mutex);
  return _reader->createBlock(parent, libs, def_file, tech);
}

}  // namespace odb
