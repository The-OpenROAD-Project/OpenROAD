// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#include "helper.h"

#include "odb/db.h"

namespace odb {

dbMaster* SimpleDbFixture::createMaster2X1(dbLib* lib,
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

dbMaster* SimpleDbFixture::createMaster1X1(dbLib* lib,
                                           const char* name,
                                           uint width,
                                           uint height,
                                           const char* in1,
                                           const char* out)
{
  dbMaster* master = dbMaster::create(lib, name);
  master->setWidth(width);
  master->setHeight(height);
  master->setType(dbMasterType::CORE);
  dbMTerm::create(master, in1, dbIoType::INPUT, dbSigType::SIGNAL);
  dbMTerm::create(master, out, dbIoType::OUTPUT, dbSigType::SIGNAL);
  master->setFrozen();
  return master;
}

void SimpleDbFixture::createSimpleDB()
{
  dbDatabase* db = db_.get();
  dbTech* tech = dbTech::create(db, "tech");
  dbTechLayer::create(tech, "L1", dbTechLayerType::MASTERSLICE);
  dbLib* lib = dbLib::create(db, "lib1", tech, ',');
  dbChip* chip = dbChip::create(db, tech);
  dbBlock::create(chip, "simple_block");
  createMaster2X1(lib, "and2", 1000, 1000, "a", "b", "o");
  createMaster2X1(lib, "or2", 500, 500, "a", "b", "o");
  createMaster1X1(lib, "inv1", 500, 500, "ip0", "op0");
}

void SimpleDbFixture::create2LevetDbNoBTerms()
{
  createSimpleDB();
  dbLib* lib = db_->findLib("lib1");
  dbChip* chip = db_->getChip();
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
}

void SimpleDbFixture::create2LevetDbWithBTerms()
{
  create2LevetDbNoBTerms();
  dbBlock* block = db_->getChip()->getBlock();
  auto n1 = block->findNet("n1");
  auto n2 = block->findNet("n2");
  auto n7 = block->findNet("n7");
  dbBTerm* IN1 = dbBTerm::create(n1, "IN1");
  IN1->setIoType(dbIoType::INPUT);
  dbBTerm* IN2 = dbBTerm::create(n2, "IN2");
  IN2->setIoType(dbIoType::INPUT);
  dbBTerm* OUT = dbBTerm::create(n7, "IN3");
  OUT->setIoType(dbIoType::OUTPUT);
}

}  // namespace odb
