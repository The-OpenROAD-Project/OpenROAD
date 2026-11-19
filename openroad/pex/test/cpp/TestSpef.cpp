// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <cctype>
#include <cstddef>
#include <cstdlib>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "fsv/Itf.h"
#include "fsv/Stack.h"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "pex/Extractor.h"
#include "pex/Model.h"
#include "pex/SpefWriter.h"
#include "tst/db_fixture.h"

namespace pex {
namespace {

// One lateral and end table per routing layer, with no covers: enough to
// extract, and the SPEF does not depend on the values.
Model openContexts()
{
  Model model;
  for (const char* layer : {"li1", "met1", "met2", "met3", "met4", "met5"}) {
    const LateralKey key{.layer = layer, .below = "", .above = ""};
    LateralTable& t
        = model.addLateral(key, LateralTable({0.14, 1.6}, {0.14, 1.6}));
    for (int w = 0; w < 2; w++) {
      for (int l = 0; l < 3; l++) {
        for (int r = 0; r < 3; r++) {
          t.set(w,
                l,
                r,
                {
                    .ground = 0.03,
                    .left = l == 2 ? 0.0 : 0.02,
                    .right = r == 2 ? 0.0 : 0.02,
                    .below = 0,
                    .above = 0,
                });
        }
      }
    }
    EndTable& e = model.addEnd(key, EndTable({0.14, 1.6}, {}, {}));
    for (int w = 0; w < 2; w++) {
      e.set(w,
            0,
            0,
            {.total = 0.01, .below = 0, .above = 0, .lateral = 0, .facing = 0});
    }
  }
  return model;
}

// What a SPEF says of each net.
struct SpefNet
{
  double total = 0;
  double sum = 0;  // of the *CAP lines
  int resistors = 0;
  int couplings = 0;
};

// A SPEF name as ODB has it: SPEF's escapes dropped, ODB's own (before a
// bracket) kept.
std::string unescaped(const std::string& name)
{
  std::string result;
  for (size_t i = 0; i < name.size(); i++) {
    if (name[i] == '\\' && i + 1 < name.size() && name[i + 1] != '['
        && name[i + 1] != ']') {
      continue;
    }
    result += name[i];
  }
  return result;
}

std::map<std::string, SpefNet> parse(const std::string& text)
{
  std::map<std::string, std::string> names;
  std::map<std::string, SpefNet> nets;
  std::istringstream in(text);
  std::string line;
  std::string section;
  SpefNet* net = nullptr;
  while (std::getline(in, line)) {
    std::istringstream words(line);
    std::vector<std::string> w;
    for (std::string s; words >> s;) {
      w.push_back(s);
    }
    if (w.empty()) {
      continue;
    }
    if (w[0][0] == '*' && !std::isdigit(static_cast<unsigned char>(w[0][1]))) {
      section = w[0];
      if (section == "*D_NET") {
        net = &nets[names.at(w[1])];
        net->total = std::stod(w[2]);
      }
      continue;
    }
    if (section == "*NAME_MAP") {
      names[w[0]] = unescaped(w[1]);
    } else if (section == "*CAP") {
      net->sum += std::stod(w.back());
      net->couplings += w.size() == 4 ? 1 : 0;
    } else if (section == "*RES") {
      net->resistors++;
    }
  }
  return nets;
}

class SpefTest : public tst::DbFixture
{
 protected:
  void SetUp() override
  {
    odb::dbTech* tech
        = loadTechLef("sky130", "_main/openroad/pex/test/sky130hd/sky130hd.tlef");
    loadLibLef(tech,
               "cells",
               "_main/openroad/pex/test/sky130hd/sky130_fd_sc_hd_merged.lef");
    block_ = readDef("_main/openroad/fsv/test/field_solver_gcd.def");
    stack_ = fsv::readItfFile(getFilePath("_main/openroad/fsv/test/sky130.itf"));
    model_ = openContexts();
  }

  std::string extractAndWrite(const int threads, const SpefOptions& spef = {})
  {
    ExtractOptions options;
    options.threads = threads;
    Extractor(block_, stack_, model_, getLogger(), options).extract();
    SpefOptions o = spef;
    o.date = "today";
    std::ostringstream out;
    writeSpef(block_, out, o);
    return out.str();
  }

  odb::dbBlock* block_ = nullptr;
  fsv::Stack stack_;
  Model model_;
};

// Each net's totals and resistors are ODB's; a coupling appears under both
// of its nets.
TEST_F(SpefTest, MatchesOdb)
{
  const std::map<std::string, SpefNet> nets = parse(extractAndWrite(0));
  int written = 0;
  int couplings = 0;
  for (odb::dbNet* net : block_->getNets()) {
    if (!net->isRCgraph()) {
      EXPECT_FALSE(nets.contains(net->getName())) << net->getName();
      continue;
    }
    written++;
    const SpefNet& s = nets.at(net->getName());
    double total = 0;
    int cc = 0;
    for (odb::dbCapNode* node : net->getCapNodes()) {
      total += node->getCapacitance(0);
      for (odb::dbCCSeg* seg : node->getCCSegs()) {
        total += seg->getCapacitance(0);
        cc++;
      }
    }
    EXPECT_NEAR(s.total, total, 1e-6 * total) << net->getName();
    EXPECT_NEAR(s.sum, total, 1e-6 * total) << net->getName();
    EXPECT_EQ(s.couplings, cc) << net->getName();
    EXPECT_EQ(s.resistors, net->getRSegs().size()) << net->getName();
    couplings += cc;
  }
  EXPECT_EQ(written, nets.size());
  EXPECT_GT(written, 100);
  EXPECT_GT(couplings, 0);
}

TEST_F(SpefTest, Threads)
{
  EXPECT_EQ(extractAndWrite(0), extractAndWrite(4));
}

// Characters SPEF reserves are escaped once; bus brackets are not. A net
// with no terminal on its wire (some in this excerpt) has no *CONN
// section, which may not be empty.
TEST_F(SpefTest, Names)
{
  const std::string text = extractAndWrite(0);
  EXPECT_EQ(text.find("*CONN\n*CAP"), std::string::npos);
  EXPECT_NE(text.find("*D_NET"), std::string::npos);
  EXPECT_NE(text.find(R"(dpath\.a_reg\.out\[0\]\$_DFFE_PP_)"),
            std::string::npos);
  EXPECT_EQ(text.find(R"(\\)"), std::string::npos);
}

// A subset of nets, each written once however often given; a net without
// an RC network and a corner ODB does not hold refused.
TEST_F(SpefTest, Nets)
{
  extractAndWrite(0);
  odb::dbNet* net = nullptr;
  odb::dbNet* bare = nullptr;
  for (odb::dbNet* n : block_->getNets()) {
    if (n->isRCgraph() && net == nullptr) {
      net = n;
    } else if (!n->isRCgraph() && bare == nullptr) {
      bare = n;
    }
  }
  ASSERT_NE(net, nullptr);
  ASSERT_NE(bare, nullptr);
  SpefOptions options;
  options.nets = {net, net};
  std::ostringstream out;
  writeSpef(block_, out, options);
  const std::string text = out.str();
  const std::map<std::string, SpefNet> nets = parse(text);
  ASSERT_EQ(nets.size(), 1);
  EXPECT_TRUE(nets.contains(net->getName()));
  size_t written = 0;
  for (size_t at = text.find("*D_NET"); at != std::string::npos;
       at = text.find("*D_NET", at + 1)) {
    written++;
  }
  EXPECT_EQ(written, 1);
  options.nets = {bare};
  EXPECT_THROW(writeSpef(block_, out, options), std::invalid_argument);
  options.nets = {};
  options.corner = 1;
  EXPECT_THROW(writeSpef(block_, out, options), std::invalid_argument);
}

}  // namespace
}  // namespace pex
