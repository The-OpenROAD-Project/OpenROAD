///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2023, Google LLC
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

#include "helper.h"

#include "odb/db.h"
#include "utl/Logger.h"

namespace odb {

dbMaster* createMaster2X1(dbLib* lib,
                          const char* name,
                          uint width,
                          uint height,
                          const char* in1,
                          const char* in2,
                          const char* out)
{
  dbMaster* master = dbMaster::create(lib, name);
  master->setWidth(width);
  master->setHeight(height);
  master->setType(dbMasterType::CORE);
  dbMTerm::create(master, in1, dbIoType::INPUT, dbSigType::SIGNAL);
  dbMTerm::create(master, in2, dbIoType::INPUT, dbSigType::SIGNAL);
  dbMTerm::create(master, out, dbIoType::OUTPUT, dbSigType::SIGNAL);
  master->setFrozen();
  return master;
}

dbDatabase* createSimpleDB()
{
  utl::Logger* logger = new utl::Logger();
  dbDatabase* db = dbDatabase::create();
  db->setLogger(logger);
  dbTech* tech = dbTech::create(db, "tech");
  dbTechLayer::create(tech, "L1", dbTechLayerType::MASTERSLICE);
  dbLib* lib = dbLib::create(db, "lib1", tech, ',');
  dbChip* chip = dbChip::create(db);
  dbBlock::create(chip, "simple_block");
  createMaster2X1(lib, "and2", 1000, 1000, "a", "b", "o");
  createMaster2X1(lib, "or2", 500, 500, "a", "b", "o");
  return db;
}

dbDatabase* create2LevetDbNoBTerms()
{
  dbDatabase* db = createSimpleDB();
  dbLib* lib = db->findLib("lib1");
  dbChip* chip = db->getChip();
  dbBlock* block = chip->getBlock();
  auto and2 = lib->findMaster("and2");
  auto or2 = lib->findMaster("or2");
  dbInst* i1 = dbInst::create(block, and2, "i1");
  dbInst* i2 = dbInst::create(block, and2, "i2");
  dbInst* i3 = dbInst::create(block, or2, "i3");
  dbNet* n1 = dbNet::create(block, "n1");
  dbNet* n2 = dbNet::create(block, "n2");
  dbNet* n3 = dbNet::create(block, "n3");
  dbNet* n4 = dbNet::create(block, "n4");
  dbNet* n5 = dbNet::create(block, "n5");
  dbNet* n6 = dbNet::create(block, "n6");
  dbNet* n7 = dbNet::create(block, "n7");
  i1->findITerm("a")->connect(n1);
  i1->findITerm("b")->connect(n2);
  i2->findITerm("a")->connect(n3);
  i2->findITerm("b")->connect(n4);
  i3->findITerm("a")->connect(n5);
  i3->findITerm("b")->connect(n6);
  i1->findITerm("o")->connect(n5);
  i2->findITerm("o")->connect(n6);
  i3->findITerm("o")->connect(n7);
  return db;
}

dbDatabase* create2LevetDbWithBTerms()
{
  dbDatabase* db = create2LevetDbNoBTerms();
  dbBlock* block = db->getChip()->getBlock();
  auto n1 = block->findNet("n1");
  auto n2 = block->findNet("n2");
  auto n7 = block->findNet("n7");
  dbBTerm* IN1 = dbBTerm::create(n1, "IN1");
  IN1->setIoType(dbIoType::INPUT);
  dbBTerm* IN2 = dbBTerm::create(n2, "IN2");
  IN2->setIoType(dbIoType::INPUT);
  dbBTerm* OUT = dbBTerm::create(n7, "IN3");
  OUT->setIoType(dbIoType::OUTPUT);
  return db;
}

}  // namespace odb
