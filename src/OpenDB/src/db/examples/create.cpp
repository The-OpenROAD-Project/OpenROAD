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

#include <stdio.h>
#include "db.h"
#include "dbMap.h"

using namespace odb

    extern void
    print_db(dbDatabase* db);

// The following code implements this two-level circuit using the database API.
//
// The values in ()'s are the object names used in the database.
//
//     (n1)   +-----
// IN1--------|a    \    (n5)
//     (n2)   | (i1)o|-----------+
// IN2--------|b    /            |       +-------
//            +-----             +--------\a     \    (n7)
//                                         ) (i3)o|---------------OUT
//     (n3)   +-----             +--------/b     /
// IN3--------|a    \    (n6)    |       +-------
//     (n4)   | (i2)o|-----------+
// IN4--------|b    /
//            +-----
//

void create_db()
{
  // Create a database
  dbDatabase* db = dbDatabase::open("chip.db", dbCreate);

  dbTech* tech = dbTech::create(db, "tech");

  // Create a library
  dbLib* lib = dbLib::create(db, "lib");
  lib->setTech(tech);

  //  Create 2-input "and" gate
  dbMaster* and2 = dbMaster::create(lib, "and2");
  and2->setType(dbMasterType(dbMasterType::CORE));
  and2->setWidth(1000);   // 1um
  and2->setHeight(1000);  // 1um
  dbMTerm::create(and2, "a", dbIoType(dbIoType::INPUT));
  dbMTerm::create(and2, "b", dbIoType(dbIoType::INPUT));
  dbMTerm::create(and2, "o", dbIoType(dbIoType::OUTPUT));
  and2->setFrozen();

  //  Create 2-input "or" gate
  dbMaster* or2 = dbMaster::create(lib, "or2");
  or2->setType(dbMasterType(dbMasterType::CORE));
  or2->setWidth(1000);   // 1um
  or2->setHeight(1000);  // 1um
  dbMTerm::create(or2, "a", dbIoType(dbIoType::INPUT));
  dbMTerm::create(or2, "b", dbIoType(dbIoType::INPUT));
  dbMTerm::create(or2, "o", dbIoType(dbIoType::OUTPUT));
  or2->setFrozen();

  // Create the chip object
  dbChip* chip = dbChip::create(db, tech, "and_or_circuit");

  // Create the top-level block (there is only one block in this example)
  dbBlock* block = dbBlock::create(chip, "top");

  // Create the gate instances
  dbInst* i1 = dbInst::create(block, and2, "i1");
  dbInst* i2 = dbInst::create(block, and2, "i2");
  dbInst* i3 = dbInst::create(block, or2, "i3");

  // Create the nets
  dbNet* n1 = dbNet::create(block, "n1");
  dbNet* n2 = dbNet::create(block, "n2");
  dbNet* n3 = dbNet::create(block, "n3");
  dbNet* n4 = dbNet::create(block, "n4");
  dbNet* n5 = dbNet::create(block, "n5");
  dbNet* n6 = dbNet::create(block, "n6");
  dbNet* n7 = dbNet::create(block, "n7");

  // Create the block-terms
  dbBTerm::create(n1, "IN1", dbIoType(dbIoType::INPUT));
  dbBTerm::create(n2, "IN2", dbIoType(dbIoType::INPUT));
  dbBTerm::create(n3, "IN3", dbIoType(dbIoType::INPUT));
  dbBTerm::create(n4, "IN4", dbIoType(dbIoType::INPUT));
  dbBTerm::create(n7, "OUT", dbIoType(dbIoType::OUTPUT));

  // Create the instance-terminals
  dbITerm::connect(i1, n1, i1->getMaster()->findMTerm("a"));
  dbITerm::connect(i1, n2, i1->getMaster()->findMTerm("b"));
  dbITerm::connect(i1, n5, i1->getMaster()->findMTerm("o"));
  dbITerm::connect(i2, n3, i2->getMaster()->findMTerm("a"));
  dbITerm::connect(i2, n4, i2->getMaster()->findMTerm("b"));
  dbITerm::connect(i2, n6, i2->getMaster()->findMTerm("o"));
  dbITerm::connect(i3, n5, i3->getMaster()->findMTerm("a"));
  dbITerm::connect(i3, n6, i3->getMaster()->findMTerm("b"));
  dbITerm::connect(i3, n7, i3->getMaster()->findMTerm("o"));

  dbSet<dbNet>      nets = block->getNets();
  dbMap<dbNet, int> weights(nets);

  int                    i = 0;
  dbSet<dbNet>::iterator itr;

  for (itr = nets.begin(); itr != nets.end(); ++itr) {
    dbNet* net   = *itr;
    weights[net] = 100 + i++;
  }

  for (itr = nets.begin(); itr != nets.end(); ++itr) {
    dbNet* net = *itr;
    int    w   = weights[net];
    printf("net(%d) w = %d\n", net->getId(), w);
  }

  db->save();

  print_db(db);

  dbDatabase::close(db);
}

int main()
{
  create_db();
  return 0;
}
