#include <stdio.h>
#include "utl/Logger.h"

#include "db.h"
using namespace odb;

dbMaster* createMaster2X1(dbLib*      lib,
                          const char* name,
                          uint        width,
                          uint        height,
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
  dbDatabase*  db   = dbDatabase::create();
  db->setLogger(logger);
  dbTech*      tech = dbTech::create(db);
  dbTechLayer::create(tech, "L1", dbTechLayerType::MASTERSLICE);
  dbLib*    lib   = dbLib::create(db, "lib1", ',');
  dbChip*   chip  = dbChip::create(db);
  dbBlock::create(chip, "simple_block");
  createMaster2X1(lib, "and2", 1000, 1000, "a", "b", "o");
  createMaster2X1(lib, "or2", 500, 500, "a", "b", "o");
  return db;
}

// #     (n1)   +-----
// #    --------|a    \    (n5)
// #     (n2)   | (i1)o|-----------+
// #    --------|b    /            |       +-------
// #            +-----             +--------\a     \    (n7)
// #                                         ) (i3)o|---------------
// #     (n3)   +-----             +--------/b     /
// #    --------|a    \    (n6)    |       +-------
// #     (n4)   | (i2)o|-----------+
// #    --------|b    /
// #            +-----
dbDatabase* create2LevetDbNoBTerms()
{
  dbDatabase* db    = createSimpleDB();
  dbLib*      lib   = db->findLib("lib1");
  dbChip*     chip  = db->getChip();
  dbBlock*    block = chip->getBlock();
  auto        and2  = lib->findMaster("and2");
  auto        or2   = lib->findMaster("or2");
  dbInst*     i1    = dbInst::create(block, and2, "i1");
  dbInst*     i2    = dbInst::create(block, and2, "i2");
  dbInst*     i3    = dbInst::create(block, or2, "i3");
  dbNet*      n1    = dbNet::create(block, "n1");
  dbNet*      n2    = dbNet::create(block, "n2");
  dbNet*      n3    = dbNet::create(block, "n3");
  dbNet*      n4    = dbNet::create(block, "n4");
  dbNet*      n5    = dbNet::create(block, "n5");
  dbNet*      n6    = dbNet::create(block, "n6");
  dbNet*      n7    = dbNet::create(block, "n7");
  dbITerm::connect(i1->findITerm("a"), n1);
  dbITerm::connect(i1->findITerm("b"), n2);
  dbITerm::connect(i2->findITerm("a"), n3);
  dbITerm::connect(i2->findITerm("b"), n4);
  dbITerm::connect(i3->findITerm("a"), n5);
  dbITerm::connect(i3->findITerm("b"), n6);
  dbITerm::connect(i1->findITerm("o"), n5);
  dbITerm::connect(i2->findITerm("o"), n6);
  dbITerm::connect(i3->findITerm("o"), n7);
  return db;
}
dbDatabase* create2LevetDbWithBTerms()
{
  dbDatabase* db    = create2LevetDbNoBTerms();
  dbBlock*    block = db->getChip()->getBlock();
  auto        n1    = block->findNet("n1");
  auto        n2    = block->findNet("n2");
  auto        n7    = block->findNet("n7");
  dbBTerm*    IN1   = dbBTerm::create(n1, "IN1");
  IN1->setIoType(dbIoType::INPUT);
  dbBTerm* IN2 = dbBTerm::create(n2, "IN2");
  IN2->setIoType(dbIoType::INPUT);
  dbBTerm* OUT = dbBTerm::create(n7, "IN3");
  OUT->setIoType(dbIoType::OUTPUT);
  return db;
}
