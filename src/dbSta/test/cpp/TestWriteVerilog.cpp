// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025, The OpenROAD Authors

// Regression test: write_verilog -remove_cells must not drop hierarchical
// module instances.
//
// Background:
//   CellSet uses CellIdLess which compares cells by network_->id(cell).
//   Before the fix, ConcreteCell used raw sequential id_ (0,1,2,...) while
//   dbModule used (db_id << 4) | 0x8 (24,40,56,...).  These ID spaces
//   overlapped, so CellSet::contains could treat a dbModule as equal to a
//   ConcreteCell, causing writeChild to silently drop hierarchical instances.
//
//   The fix tags ConcreteCell IDs with CONCRETE_OBJECT_ID (0xF) so they
//   never collide with dbModule IDs (tag 0x8).

#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>

#include "gtest/gtest.h"
#include "sta/Network.hh"
#include "sta/NetworkClass.hh"
#include "sta/PatternMatch.hh"
#include "sta/VerilogWriter.hh"
#include "tst/IntegratedFixture.h"

namespace sta {

class TestWriteVerilog : public tst::IntegratedFixture
{
 public:
  TestWriteVerilog()
      : tst::IntegratedFixture(tst::IntegratedFixture::Technology::Nangate45,
                               "_main/src/dbSta/test/")
  {
  }
};

// Verifies that write_verilog -remove_cells does not drop hierarchical
// instances when a liberty cell is removed.
//
// Puts a liberty cell (INV_X1) in remove_cells, then verifies that the
// hierarchical instance (sub_inst) is still present in the output.
// Before the fix, this could fail when the ConcreteCell's raw id_
// collided with the dbModule's computed id.
TEST_F(TestWriteVerilog, RemoveCellsIdCollision)
{
  const std::string test_name = "TestWriteVerilog_RemoveCellsIdCollision";

  // Read a simple hierarchical design: top -> sub_mod -> INV_X1
  readVerilogAndSetup(test_name + "_pre.v", /*init_default_sdc=*/false);

  Network* network = sta_->network();
  Instance* top_inst = network->topInstance();

  // Find the hierarchical instance and its Cell* (which is a dbModule*)
  Cell* hier_cell = nullptr;
  std::string hier_inst_name;
  InstanceChildIterator* child_iter = network->childIterator(top_inst);
  while (child_iter->hasNext()) {
    Instance* child = child_iter->next();
    if (network->isHierarchical(child)) {
      hier_cell = network->cell(child);
      hier_inst_name = network->name(child);
      break;
    }
  }
  delete child_iter;
  ASSERT_NE(hier_cell, nullptr) << "No hierarchical instance found in design";

  ObjectId hier_id = network->id(hier_cell);

  // Verify no ConcreteCell shares the same id as the hier module.
  // After the fix, ConcreteCell IDs use tag 0xF while dbModule IDs
  // use tag 0x8, so no collision is possible.
  PatternMatch match_all("*",
                         /*is_regexp=*/false,
                         /*nocase=*/false,
                         sta_->tclInterp());

  LibraryIterator* lib_iter = network->libraryIterator();
  while (lib_iter->hasNext()) {
    Library* lib = lib_iter->next();
    CellSeq lib_cells = network->findCellsMatching(lib, &match_all);
    for (Cell* cell : lib_cells) {
      if (cell != hier_cell) {
        EXPECT_NE(network->id(cell), hier_id)
            << "ObjectId collision between ConcreteCell '"
            << network->name(cell) << "' and dbModule '"
            << network->name(hier_cell) << "' — tagging should prevent this";
      }
    }
  }
  delete lib_iter;

  // Find INV_X1 liberty cell to use in remove_cells
  Cell* inv_cell = nullptr;
  LibraryIterator* lib_iter2 = network->libraryIterator();
  while (lib_iter2->hasNext()) {
    Library* lib = lib_iter2->next();
    PatternMatch inv_match("INV_X1",
                           /*is_regexp=*/false,
                           /*nocase=*/false,
                           sta_->tclInterp());
    CellSeq cells = network->findCellsMatching(lib, &inv_match);
    if (!cells.empty()) {
      inv_cell = cells[0];
      break;
    }
  }
  delete lib_iter2;
  ASSERT_NE(inv_cell, nullptr) << "INV_X1 not found in libraries";

  // Put INV_X1 in remove_cells and write verilog
  CellSeq remove_cells;
  remove_cells.push_back(inv_cell);

  const std::string output_file = test_name + "_out.v";
  writeVerilog(output_file.c_str(), false, &remove_cells, network);

  // Read the output
  std::ifstream ifs(output_file);
  ASSERT_TRUE(ifs.is_open()) << "Failed to open output file: " << output_file;
  std::string content((std::istreambuf_iterator<char>(ifs)),
                      std::istreambuf_iterator<char>());
  ifs.close();

  // The hierarchical instance must be present in the output.
  // Only leaf INV_X1 instances should be removed, not hierarchical modules.
  EXPECT_NE(content.find(hier_inst_name), std::string::npos)
      << "Hierarchical instance '" << hier_inst_name
      << "' was incorrectly removed from write_verilog output.\n"
      << "  write_verilog -remove_cells should never drop hierarchical "
         "instances.";

  // Clean up
  std::remove(output_file.c_str());
}

}  // namespace sta
