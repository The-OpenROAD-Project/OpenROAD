#include <set>
#include <string>

#include "gtest/gtest.h"
#include "odb/db.h"
#include "sta/Liberty.hh"
#include "tst/fixture.h"

namespace odb {

class TestDbNet : public tst::Fixture
{
 protected:
  struct ComplexHierarchy
  {
    dbNet* the_net = nullptr;
    std::set<dbModNet*> expected_modnets;
  };

  void SetUp() override
  {
    library_ = readLiberty("./Nangate45/Nangate45_typ.lib");
    loadTechAndLib("tech", "Nangate45.lef", "./Nangate45/Nangate45.lef");

    dbChip* chip = dbChip::create(db_.get(), db_->getTech());
    block_ = dbBlock::create(chip, "top");
  }

  ComplexHierarchy SetUpComplexHierarchy(const std::string& net_name,
                                         const std::string& top_mod_net_name)
  {
    ComplexHierarchy hierarchy;

    // ARRANGE
    // Create a hierarchy:
    // top_mod
    //  |-- l1_inst_A (l1_mod_A)
    //  |    |-- l2_inst_A1 (l2_mod_A1)
    //  |    |    +-- leaf_inst_1 (AND2_X1)
    //  |    +-- l2_inst_A2 (l2_mod_A2)
    //  |         +-- leaf_inst_2 (OR2_X1)
    //  |-- l1_inst_B (l1_mod_B)
    //  |    +-- leaf_inst_3 (INV_X1)
    //  +-- leaf_inst_4 (AND2_X1)

    auto* top_mod = block_->getTopModule();
    auto* l1_mod_A = dbModule::create(block_, "l1_mod_A");
    auto* l1_mod_B = dbModule::create(block_, "l1_mod_B");
    auto* l2_mod_A1 = dbModule::create(block_, "l2_mod_A1");
    auto* l2_mod_A2 = dbModule::create(block_, "l2_mod_A2");

    auto* l1_inst_A = dbModInst::create(top_mod, l1_mod_A, "l1_inst_A");
    auto* l1_inst_B = dbModInst::create(top_mod, l1_mod_B, "l1_inst_B");
    auto* l2_inst_A1 = dbModInst::create(l1_mod_A, l2_mod_A1, "l2_inst_A1");
    auto* l2_inst_A2 = dbModInst::create(l1_mod_A, l2_mod_A2, "l2_inst_A2");

    auto* and2_master = db_->findMaster("AND2_X1");
    auto* or2_master = db_->findMaster("OR2_X1");
    auto* inv1_master = db_->findMaster("INV_X1");

    auto* leaf_inst_1
        = dbInst::create(block_, and2_master, "leaf_inst_1", false, l2_mod_A1);
    auto* leaf_inst_2
        = dbInst::create(block_, or2_master, "leaf_inst_2", false, l2_mod_A2);
    auto* leaf_inst_3
        = dbInst::create(block_, inv1_master, "leaf_inst_3", false, l1_mod_B);
    auto* leaf_inst_4
        = dbInst::create(block_, and2_master, "leaf_inst_4", false, top_mod);

    hierarchy.the_net = dbNet::create(block_, net_name.c_str());

    auto* in_bterm = dbBTerm::create(hierarchy.the_net, "in_port");
    in_bterm->setIoType(dbIoType::INPUT);
    auto* out_bterm = dbBTerm::create(hierarchy.the_net, "out_port");
    out_bterm->setIoType(dbIoType::OUTPUT);

    auto* top_mod_net = dbModNet::create(top_mod, top_mod_net_name.c_str());
    hierarchy.expected_modnets.insert(top_mod_net);

    auto* l1_mod_A_net = dbModNet::create(l1_mod_A, "l1_mod_A_net");
    hierarchy.expected_modnets.insert(l1_mod_A_net);
    auto* l1_mod_B_net = dbModNet::create(l1_mod_B, "l1_mod_B_net");
    hierarchy.expected_modnets.insert(l1_mod_B_net);

    auto* l2_mod_A1_net = dbModNet::create(l2_mod_A1, "l2_mod_A1_net");
    hierarchy.expected_modnets.insert(l2_mod_A1_net);
    auto* l2_mod_A2_net = dbModNet::create(l2_mod_A2, "l2_mod_A2_net");
    hierarchy.expected_modnets.insert(l2_mod_A2_net);

    in_bterm->connect(top_mod_net);
    out_bterm->connect(top_mod_net);

    leaf_inst_1->findITerm("A1")->connect(hierarchy.the_net, l2_mod_A1_net);
    leaf_inst_2->findITerm("A2")->connect(hierarchy.the_net, l2_mod_A2_net);
    leaf_inst_3->findITerm("A")->connect(hierarchy.the_net, l1_mod_B_net);
    leaf_inst_4->findITerm("A1")->connect(hierarchy.the_net, top_mod_net);

    auto* l2_mod_A1_bterm = dbModBTerm::create(l2_mod_A1, "p0");
    l2_mod_A1_bterm->connect(l2_mod_A1_net);
    auto* l2_inst_A1_iterm
        = dbModITerm::create(l2_inst_A1, "p0", l2_mod_A1_bterm);
    l2_inst_A1_iterm->connect(l1_mod_A_net);

    auto* l2_mod_A2_bterm = dbModBTerm::create(l2_mod_A2, "p0");
    l2_mod_A2_bterm->connect(l2_mod_A2_net);
    auto* l2_inst_A2_iterm
        = dbModITerm::create(l2_inst_A2, "p0", l2_mod_A2_bterm);
    l2_inst_A2_iterm->connect(l1_mod_A_net);

    auto* l1_mod_A_bterm = dbModBTerm::create(l1_mod_A, "p0");
    l1_mod_A_bterm->connect(l1_mod_A_net);
    auto* l1_inst_A_iterm = dbModITerm::create(l1_inst_A, "p0", l1_mod_A_bterm);
    l1_inst_A_iterm->connect(top_mod_net);

    auto* l1_mod_B_bterm = dbModBTerm::create(l1_mod_B, "p0");
    l1_mod_B_bterm->connect(l1_mod_B_net);
    auto* l1_inst_B_iterm = dbModITerm::create(l1_inst_B, "p0", l1_mod_B_bterm);
    l1_inst_B_iterm->connect(top_mod_net);

    return hierarchy;
  }

  dbBlock* block_;
  sta::LibertyLibrary* library_;
};

// Test a multi-level hierarchy where a dbNet is connected to multiple
// bterms and iterms across the hierarchy.
// We expect to find all the modnets connected up through the hierarchy.
TEST_F(TestDbNet, FindRelatedModNetsComplex)
{
  // ARRANGE
  const auto hierarchy = SetUpComplexHierarchy("the_net", "top_mod_net");

  // ACTION
  std::set<dbModNet*> related_modnets;
  hierarchy.the_net->findRelatedModNets(related_modnets);

  // ASSERT
  ASSERT_EQ(related_modnets.size(), hierarchy.expected_modnets.size());
  for (auto* expected_net : hierarchy.expected_modnets) {
    EXPECT_NE(related_modnets.find(expected_net), related_modnets.end())
        << "Missing " << expected_net->getName();
  }
}

// Test that calling the function on a net with no modnet connections
// returns an empty set.
TEST_F(TestDbNet, FindRelatedModNetsNone)
{
  // ARRANGE
  auto* net = dbNet::create(block_, "net_no_modnet");
  auto* inst = dbInst::create(block_, db_->findMaster("AND2_X1"), "i1");
  inst->findITerm("A1")->connect(net);

  // ACTION
  std::set<dbModNet*> related_modnets;
  net->findRelatedModNets(related_modnets);

  // ASSERT
  EXPECT_TRUE(related_modnets.empty());
}

// Test that calling the function with a pre-filled set clears it first.
TEST_F(TestDbNet, FindRelatedModNetsClearsSet)
{
  // ARRANGE
  auto* net = dbNet::create(block_, "some_net");
  auto* top_mod = block_->getTopModule();
  auto* dummy_mod_net = dbModNet::create(top_mod, "dummy");

  std::set<dbModNet*> related_modnets;
  related_modnets.insert(dummy_mod_net);

  // ACTION
  net->findRelatedModNets(related_modnets);

  // ASSERT
  EXPECT_TRUE(related_modnets.empty());
}

// Test that the net is renamed to the name of the highest-level modnet
TEST_F(TestDbNet, RenameWithModNetInHighestHier)
{
  // ARRANGE
  const auto hierarchy
      = SetUpComplexHierarchy("original_name", "top_level_name");

  // ACTION
  hierarchy.the_net->renameWithModNetInHighestHier();

  // ASSERT
  EXPECT_EQ(hierarchy.the_net->getName(), "top_level_name");
}

// Test that the net name is unchanged if there are no hierarchical connections
TEST_F(TestDbNet, RenameWithModNetNoHier)
{
  // ARRANGE
  auto* net = dbNet::create(block_, "original_name");
  auto* inst = dbInst::create(block_, db_->findMaster("AND2_X1"), "i1");
  inst->findITerm("A1")->connect(net);

  // ACTION
  net->renameWithModNetInHighestHier();

  // ASSERT
  EXPECT_EQ(net->getName(), "original_name");
}

}  // namespace odb
