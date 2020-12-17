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

using namespace odb

    //
    // dump the database to stdout
    //
    void
    print_lib(dbLib* lib)
{
  dbString name = lib->getName();

  printf("\nBEGIN LIB %s\n", name.c_str());

  dbSet<dbMaster>           masters = lib->getMasters();
  dbSet<dbMaster>::iterator master_itr;

  for (master_itr = masters.begin(); master_itr != masters.end();
       ++master_itr) {
    dbMaster*    master = *master_itr;
    dbString     mname  = master->getName();
    dbMasterType type   = master->getType();
    printf("  MASTER %s TYPE %s\n", mname.c_str(), type.getString());
  }

  printf("END LIB\n");
}

void print_instances(dbBlock* block)
{
  printf("  BEGIN INSTANCES\n");

  dbSet<dbInst>           insts = block->getInsts();
  dbSet<dbInst>::iterator inst_itr;

  for (inst_itr = insts.begin(); inst_itr != insts.end(); ++inst_itr) {
    dbInst*   inst   = *inst_itr;
    dbString  iname  = inst->getName();
    dbMaster* master = inst->getMaster();
    dbString  mname  = master->getName();
    printf("    INST %s %s\n", mname.c_str(), iname.c_str());
  }

  printf("  END INSTANCES\n");
}

void print_nets(dbBlock* block)
{
  printf("  BEGIN NETS\n");

  dbSet<dbNet>           nets = block->getNets();
  dbSet<dbNet>::iterator net_itr;

  for (net_itr = nets.begin(); net_itr != nets.end(); ++net_itr) {
    dbNet*   net  = *net_itr;
    dbString name = net->getName();
    printf("    NET %s ", name.c_str());

    dbSet<dbITerm>           iterms = net->getITerms();
    dbSet<dbITerm>::iterator iterm_itr;

    for (iterm_itr = iterms.begin(); iterm_itr != iterms.end(); ++iterm_itr) {
      dbITerm* iterm = *iterm_itr;
      dbInst*  inst  = iterm->getInst();
      dbMTerm* mterm = iterm->getMTerm();
      dbString iname = inst->getName();
      dbString mname = mterm->getName();
      printf("(%s %s) ", iname.c_str(), mname.c_str());
    }

    printf("\n");
  }

  printf("  END NETS\n");
}

void print_bterms(dbBlock* block)
{
  printf("  BEGIN BTERMS\n");
  dbSet<dbBTerm>           bterms = block->getBTerms();
  dbSet<dbBTerm>::iterator bterm_itr;

  for (bterm_itr = bterms.begin(); bterm_itr != bterms.end(); ++bterm_itr) {
    dbBTerm* bterm = *bterm_itr;
    dbNet*   net   = bterm->getNet();
    dbString bname = bterm->getName();
    dbString nname = net->getName();
    printf("    BTERM %s %s\n", bname.c_str(), nname.c_str());
  }
  printf("  END BTERMS\n");
}

void print_chip(dbChip* chip)
{
  dbString chip_name = chip->getName();

  printf("\nBEGIN CHIP %s\n", chip_name.c_str());

  print_instances(chip->getBlock());
  print_nets(chip->getBlock());
  print_bterms(chip->getBlock());

  printf("END CHIP\n");
}

void print_db(dbDatabase* db)
{
  dbSet<dbLib>           libs = db->getLibs();
  dbSet<dbLib>::iterator lib_itr;

  for (lib_itr = libs.begin(); lib_itr != libs.end(); ++lib_itr) {
    dbLib* lib = *lib_itr;
    print_lib(lib);
  }

  dbSet<dbChip>           chips = db->getChips();
  dbSet<dbChip>::iterator chip_itr;

  for (chip_itr = chips.begin(); chip_itr != chips.end(); ++chip_itr) {
    dbChip* chip = *chip_itr;
    print_chip(chip);
  }
}
