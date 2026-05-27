// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <cstdint>
#include <sstream>

#include "gtest/gtest.h"
#include "syn/ir/Bundle.h"
#include "syn/ir/Graph.h"
#include "syn/ir/Instance.h"
#include "syn/ir/Net.h"
#include "syn/ir/TritModel.h"

namespace syn {

// SAT-based equivalence check between two graphs.
//
// g1 is the reference ("gold") side, g2 is the candidate ("gate") side.
// allowRefinement: if true, g2 may be defined where g1 is X (one-sided
//   refinement). Otherwise require bit-for-bit equality of the trit values.
// inputsDefined: if true, constrain inputs to be defined (drop X on inputs).
inline bool equivalenceCheck(Graph& g1,
                             Graph& g2,
                             bool inputsDefined,
                             bool allowRefinement)
{
  Bundle allInputs1, allInputs2;
  {
    auto i1 = g1.collectInputs(), i2 = g2.collectInputs();
    EXPECT_EQ(i1.size(), i2.size())
        << "number of inputs should match between gate/gold";
    auto it1 = i1.begin(), it2 = i2.begin();
    for (; it1 != i1.end() && it2 != i2.end(); it1++, it2++) {
      EXPECT_TRUE(i2.contains(it1->first));
      EXPECT_TRUE(i1.contains(it2->first));
      EXPECT_EQ(it1->second.width(), it2->second.width())
          << "input " << it1->first
          << " needs matching width between gate/gold";
      allInputs1.append(it1->second);
      allInputs2.append(it2->second);
    }
  }

  Bundle allOutputs1, allOutputs2;
  {
    auto o1 = g1.collectOutputs(), o2 = g2.collectOutputs();
    EXPECT_EQ(o1.size(), o2.size())
        << "number of outputs should match between gate/gold";
    auto it1 = o1.begin(), it2 = o2.begin();
    for (; it1 != o1.end() && it2 != o2.end(); it1++, it2++) {
      EXPECT_TRUE(o2.contains(it1->first));
      EXPECT_TRUE(o1.contains(it2->first));
      EXPECT_EQ(it1->second.width(), it2->second.width())
          << "output " << it1->first
          << " needs matching width between gate/gold";
      allOutputs1.append(it1->second);
      allOutputs2.append(it2->second);
    }
  }

  Solver solver;
  TritModel m1(solver, g1), m2(solver, g2);
  m1.encodeCone(allInputs1, allOutputs1);
  m2.encodeCone(allInputs2, allOutputs2);
  for (uint32_t i = 0; i < allInputs1.width() && i < allInputs2.width(); i++) {
    Net net1 = allInputs1[i], net2 = allInputs2[i];
    solver.addClause({m1.valVar(net1), -m2.valVar(net2)});
    solver.addClause({-m1.valVar(net1), m2.valVar(net2)});
    solver.addClause({m1.defVar(net1), -m2.defVar(net2)});
    solver.addClause({-m1.defVar(net1), m2.defVar(net2)});
    if (inputsDefined) {
      solver.addClause({m1.defVar(net1)});
      solver.addClause({m2.defVar(net2)});
    }
  }

  if (allOutputs1.empty() || allOutputs2.empty()) {
    ADD_FAILURE() << "no outputs to compare";
    return false;
  }
  int flag = 0;
  for (uint32_t i = 0; i < allOutputs1.width() && i < allOutputs2.width();
       i++) {
    Net net1 = allOutputs1[i], net2 = allOutputs2[i];
    int differs;
    if (allowRefinement) {
      // Gold (m1) defined AND (gate (m2) undefined OR values disagree).
      differs = solver.encodeAnd(
          m1.defVar(net1),
          solver.encodeOr(-m2.defVar(net2),
                          solver.encodeXor(m1.valVar(net1), m2.valVar(net2))));
    } else {
      // def disagrees OR (both defined AND values disagree).
      differs = solver.encodeOr(
          solver.encodeXor(m1.defVar(net1), m2.defVar(net2)),
          solver.encodeAnd(m1.defVar(net1),
                           solver.encodeXor(m1.valVar(net1), m2.valVar(net2))));
    }

    if (i == 0) {
      flag = differs;
    } else {
      flag = solver.encodeOr(flag, differs);
    }
  }
  solver.addClause({flag});

  if (solver.solve() != Solver::Unsat) {
    std::stringstream ss;
    ss << "; SAT verification failed: inputsDefined=" << inputsDefined
       << " allowRefinement=" << allowRefinement << "\n";
    auto dumpSide = [&](Graph& g, TritModel& m, const char* label) {
      ss << "; " << label << ":\n";
      g.forEachInstance([&](const Instance* inst) {
        if (inst->is<Output>()) {
          g.dumpInstance(ss, inst);
          ss << "\n";
        }
      });
      m.dumpCone(ss);
    };
    dumpSide(g1, m1, "gold side");
    dumpSide(g2, m2, "gate side");
    ADD_FAILURE() << ss.str();
    return false;
  }

  return true;
}

}  // namespace syn
