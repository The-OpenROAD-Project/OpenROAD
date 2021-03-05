///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2019, Nefelus Inc
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#include "defin.h"
#include "db.h"
#include "definReader.h"

namespace odb {

defin::defin(dbDatabase* db,utl::Logger* logger, MODE mode)
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

void defin::namesAreDBIDs()
{
  _reader->namesAreDBIDs();
}

void defin::setAssemblyMode()
{
  _reader->setAssemblyMode();
}

void defin::useBlockName(const char* name)
{
  _reader->useBlockName(name);
}

dbChip* defin::createChip(std::vector<dbLib*>& libs, const char* def_file)
{
  if (libs.size() == 0)
    return NULL;

  return _reader->createChip(libs, def_file);
}

dbBlock* defin::createBlock(dbBlock*             parent,
                            std::vector<dbLib*>& libs,
                            const char*          def_file)
{
  if (libs.size() == 0)
    return NULL;

  return _reader->createBlock(parent, libs, def_file);
}

bool defin::replaceWires(dbBlock* block, const char* def_file)
{
  return _reader->replaceWires(block, def_file);
}

}  // namespace odb
