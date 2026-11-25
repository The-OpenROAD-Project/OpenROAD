// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <functional>
#include <map>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "sta/FuncExpr.hh"
#include "sta/Liberty.hh"
#include "sta/Sequential.hh"
#include "tst/fixture.h"
#include "utl/Logger.h"

namespace ret {

// Fixture for the SequentialUnmapper unit tests.
//
// Tests load their own hand-made library pair(s) with setUpChipForTest,
// build a tiny netlist with the tst::Fixture builders, run the unmapper,
// then check the result two ways:
//   - explicit structural asserts (cell counts, which pin a port sits on);
//   - expectSameFunction: a truth-table comparison of the logic cone feeding
//     the base flop's D pin against the original cell's liberty next_state.
//     This checks what unmapping promises (same function) without depending
//     on which gates were used to build it.
class SequentialUnmapperFixture : public tst::Fixture
{
 public:
  using LibPair = std::pair<const char*, const char*>;  // (lef, lib)
  using LeafNets = std::map<std::string, odb::dbNet*>;  // liberty pin -> net
  // odb deletes std::less on dbObject*, hence the unordered map.
  using NetValues = std::unordered_map<odb::dbNet*, int>;
  using LeafValue = std::function<int(const sta::LibertyPort*)>;
  using ExpectedFn = std::function<int(const LeafValue&)>;

  // Library pairs shared by the test files; the first LEF loaded creates
  // the tech, later LEFs are macros-only.
  static constexpr LibPair kBasic{"sequnmap_basic.lef", "sequnmap_basic.lib"};
  static constexpr LibPair kTieMulti{"sequnmap_tie_multi.lef",
                                     "sequnmap_tie_multi.lib"};
  static constexpr LibPair kTieSized{"sequnmap_tie_sized.lef",
                                     "sequnmap_tie_sized.lib"};
  static constexpr LibPair kNegedge{"sequnmap_seq_active_low.lef",
                                    "sequnmap_seq_active_low.lib"};
  // No plain flip-flop; self-contained, so it is loaded alone.
  static constexpr LibPair kAsyncOnly{"sequnmap_async_only.lef",
                                      "sequnmap_async_only.lib"};

 protected:
  // Load one or more (lef, lib) pairs, then create the chip and block.
  // Call as the first line of a test, wrapped in ASSERT_NO_FATAL_FAILURE.
  void setUpChipForTest(const std::vector<LibPair>& libs)
  {
    ASSERT_FALSE(libs.empty()) << "need at least one (lef, lib) pair";
    const std::string dir = "_main/openroad/ret/test/";
    odb::dbTech* tech = nullptr;
    for (const LibPair& lef_lib : libs) {
      const std::string lef = lef_lib.first;
      const std::string lib = lef_lib.second;
      const std::string name = lef.substr(0, lef.rfind('.'));
      odb::dbLib* db_lib = nullptr;
      if (tech == nullptr) {
        db_lib = loadTechAndLib("tech", name.c_str(), getFilePath(dir + lef));
        ASSERT_NE(db_lib, nullptr) << lef << " did not load (tech LEF)";
        tech = getDb()->getTech();
      } else {
        db_lib = loadLibaryLef(tech, name.c_str(), getFilePath(dir + lef));
        ASSERT_NE(db_lib, nullptr) << lef << " did not load (macros-only LEF)";
      }
      ASSERT_NE(readLiberty(getFilePath(dir + lib)), nullptr)
          << lib << " did not load";
    }
    chip_ = odb::dbChip::create(getDb(), tech);
    ASSERT_NE(chip_, nullptr);
    block_ = odb::dbBlock::create(chip_, "top");
    // Tell dbSta which block it owns so the netlist checks below work.
    getSta()->postReadDef(block_);
  }

  // Primary INPUT ports for the given net names.
  void makeInputPorts(const std::vector<const char*>& names)
  {
    for (const char* name : names) {
      makeBTerm(block_, name);  // BTermOptions default: INPUT, SIGNAL
    }
  }

  void makeOutputPort(const char* name)
  {
    tst::BTermOptions options;
    options.io_type = odb::dbIoType::OUTPUT;
    makeBTerm(block_, name, options);
  }

  // ---- netlist queries -----------------------------------------------------

  int countMaster(const char* master_name) const
  {
    int n = 0;
    for (odb::dbInst* inst : block_->getInsts()) {
      if (inst->getMaster()->getName() == master_name) {
        n++;
      }
    }
    return n;
  }

  // First instance of the given master, or nullptr.
  odb::dbInst* getInstByMasterName(const std::string& master_name) const
  {
    for (odb::dbInst* inst : block_->getInsts()) {
      if (inst->getMaster()->getName() == master_name) {
        return inst;
      }
    }
    return nullptr;
  }

  // The gate driving a net (nullptr if undriven or driven by a port).
  static odb::dbInst* driverOf(odb::dbNet* net)
  {
    odb::dbITerm* out = net ? net->getFirstOutput() : nullptr;
    return out ? out->getInst() : nullptr;
  }

  // Nets on a gate's INPUT pins, in pin order.
  static std::vector<odb::dbNet*> inputNets(odb::dbInst* inst)
  {
    std::vector<odb::dbNet*> nets;
    for (odb::dbITerm* iterm : inst->getITerms()) {
      if (iterm->getIoType() == odb::dbIoType::INPUT) {
        nets.push_back(iterm->getNet());
      }
    }
    return nets;
  }

  static bool isMaster(odb::dbInst* inst, const char* name)
  {
    return inst != nullptr && inst->getMaster()->getName() == name;
  }

  const sta::Sequential* sequentialOf(const char* cell_name) const
  {
    sta::LibertyCell* cell
        = getSta()->getDbNetwork()->findLibertyCell(cell_name);
    return cell ? &cell->sequentials().front() : nullptr;
  }

  // ---- functional check ----------------------------------------------------

  // Evaluate a liberty function; `leaf` supplies the value of each port.
  // Returns -1 if any leaf is unknown.
  static int evalFunc(const sta::FuncExpr* func, const LeafValue& leaf)
  {
    if (func == nullptr) {
      return -1;
    }
    switch (func->op()) {
      case sta::FuncExpr::Op::port:
        return leaf(func->port());
      case sta::FuncExpr::Op::one:
        return 1;
      case sta::FuncExpr::Op::zero:
        return 0;
      case sta::FuncExpr::Op::not_: {
        const int a = evalFunc(func->left(), leaf);
        return a < 0 ? -1 : 1 - a;
      }
      case sta::FuncExpr::Op::and_:
      case sta::FuncExpr::Op::or_:
      case sta::FuncExpr::Op::xor_: {
        const int a = evalFunc(func->left(), leaf);
        const int b = evalFunc(func->right(), leaf);
        if (a < 0 || b < 0) {
          return -1;
        }
        if (func->op() == sta::FuncExpr::Op::and_) {
          return a & b;
        }
        if (func->op() == sta::FuncExpr::Op::or_) {
          return a | b;
        }
        return a ^ b;
      }
    }
    return -1;
  }

  // Value of a net under `values`. A net in `values` is a leaf; any other
  // net is computed from its driver's output-pin liberty function, recursing
  // through the driver's input nets.
  int evalNet(odb::dbNet* net, const NetValues& values) const
  {
    const auto leaf_it = values.find(net);
    if (leaf_it != values.end()) {
      return leaf_it->second;
    }
    odb::dbITerm* out = net ? net->getFirstOutput() : nullptr;
    if (out == nullptr) {
      ADD_FAILURE() << "undriven net " << (net ? net->getName() : "<null>");
      return -1;
    }
    const sta::dbNetwork* network = getSta()->getDbNetwork();
    sta::LibertyPort* port = network->libertyPort(network->dbToSta(out));
    const sta::FuncExpr* func = port ? port->function() : nullptr;
    if (func == nullptr) {
      ADD_FAILURE() << "no liberty function on " << out->getInst()->getName()
                    << "/" << out->getMTerm()->getName();
      return -1;
    }
    odb::dbInst* inst = out->getInst();
    return evalFunc(func, [&](const sta::LibertyPort* p) {
      odb::dbITerm* iterm = inst->findITerm(p->name().c_str());
      odb::dbNet* in = iterm ? iterm->getNet() : nullptr;
      if (in == nullptr) {
        ADD_FAILURE() << "floating input " << inst->getName() << "/"
                      << p->name();
        return -1;
      }
      return evalNet(in, values);
    });
  }

  // Truth-table check: `net` must compute expected(leaf) for every
  // assignment of the leaf nets and the state bit.
  void expectNetFunction(odb::dbNet* net,
                         const sta::Sequential* seq,
                         const LeafNets& leaf_nets,
                         odb::dbNet* state_net,
                         const ExpectedFn& expected) const
  {
    const std::vector<std::pair<std::string, odb::dbNet*>> leaves(
        leaf_nets.begin(), leaf_nets.end());
    const int n = static_cast<int>(leaves.size());
    for (int bits = 0; bits < (1 << (n + 1)); bits++) {
      NetValues values;
      std::string row;
      for (int i = 0; i < n; i++) {
        const int v = (bits >> i) & 1;
        values[leaves[i].second] = v;
        row += leaves[i].first + "=" + std::to_string(v) + " ";
      }
      const int state = (bits >> n) & 1;
      values[state_net] = state;
      row += seq->output()->name() + "=" + std::to_string(state);

      const LeafValue leaf = [&](const sta::LibertyPort* p) -> int {
        if (p == seq->output()) {
          return state;
        }
        if (p == seq->outputInv()) {
          return 1 - state;
        }
        const auto it = leaf_nets.find(p->name());
        if (it == leaf_nets.end()) {
          ADD_FAILURE() << "pin " << p->name() << " has no net in leaf_nets";
          return -1;
        }
        return values.at(it->second);
      };
      EXPECT_EQ(evalNet(net, values), expected(leaf))
          << "net " << net->getName() << " differs from expected at " << row;
    }
  }

  // The base flop's D cone must equal the cell's next_state.
  void expectSameFunction(odb::dbNet* d_net,
                          const sta::Sequential* seq,
                          const LeafNets& leaf_nets,
                          odb::dbNet* state_net) const
  {
    expectNetFunction(
        d_net, seq, leaf_nets, state_net, [&](const LeafValue& leaf) {
          return evalFunc(seq->data(), leaf);
        });
  }

  // Netlist and timing-graph consistency: both return the number of new
  // warnings, so 0 means clean.
  void expectSane() const
  {
    EXPECT_EQ(getSta()->getDbNetwork()->checkAxioms(), 0)
        << "dbNetwork axioms violated";
    EXPECT_EQ(getSta()->checkSanity(), 0) << "STA sanity check failed";
  }

  odb::dbChip* chip_ = nullptr;
  odb::dbBlock* block_ = nullptr;
};

}  // namespace ret
