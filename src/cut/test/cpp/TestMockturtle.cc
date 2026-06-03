// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#include <string.h>  // NOLINT(modernize-deprecated-headers): for strdup()
#include <unistd.h>

#include <array>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "base/abc/abc.h"
#include "base/main/abcapis.h"
#include "cut/abc_library_factory.h"
#include "cut/logic_cut.h"
#include "cut/logic_extractor.h"
#include "db_sta/dbReadVerilog.hh"
#include "db_sta/dbSta.hh"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "helper.h"
#include "lorina/common.hpp"
#include "map/mio/mio.h"
#include "map/scl/sclLib.h"
#include "misc/vec/vecPtr.h"
#include "mockturtle/algorithms/emap.hpp"
#include "mockturtle/io/genlib_reader.hpp"
#include "mockturtle/networks/aig.hpp"
#include "mockturtle/networks/block.hpp"
#include "mockturtle/utils/name_utils.hpp"
#include "mockturtle/utils/tech_library.hpp"
#include "mockturtle/views/names_view.hpp"
#include "mockturtle/views/topo_view.hpp"
#include "odb/db.h"
#include "odb/dbSet.h"
#include "odb/lefin.h"
#include "sta/Graph.hh"
#include "sta/Liberty.hh"
#include "sta/NetworkClass.hh"
#include "sta/SdcClass.hh"
#include "sta/Sta.hh"
#include "sta/Units.hh"
#include "sta/VerilogReader.hh"
#include "tst/fixture.h"
#include "utl/deleter.h"
#include "utl/unique_name.h"

namespace abc {
Vec_Str_t* Abc_SclProduceGenlibStr(SC_Lib* p,
                                   float Slew,
                                   float Gain,
                                   int nGatesMin,
                                   int fUseAll,
                                   int* pnCellCount);
}  // namespace abc

namespace cut {

using cut::LogicCut;
using cut::LogicExtractorFactory;
using ::testing::Contains;
using ::testing::ElementsAre;
using ::testing::Pair;

namespace {

std::vector<std::pair<uint64_t, uint64_t>> GetAigFanins(
    const mockturtle::aig_network& ntk,
    mockturtle::aig_network::node node)
{
  std::vector<std::pair<uint64_t, uint64_t>> fanins;
  ntk.foreach_fanin(node, [&](const auto& signal) {
    fanins.emplace_back(signal.index, signal.complement);
  });

  return fanins;
}

}  // namespace

class MockturtleTest : public CutFixture
{
 protected:
  auto CreateTechLib()
  {
    cut::AbcLibraryFactory factory(&logger_);
    factory.AddDbSta(sta_.get());
    auto abc_library = factory.BuildScl();
    auto lib = abc_library.get();
    int cell_count = 0;
    auto* genlib_vec = abc::Abc_SclProduceGenlibStr(
        lib, abc::Abc_SclComputeAverageSlew(lib), 200.0f, 0, true, &cell_count);
    // ABC ends the file with '.end', but mockturtle doesn't like that
    for (int i = 0; i < sizeof(".end\n\0"); i++) {
      abc::Vec_StrPop(genlib_vec);
    }
    abc::Vec_StrPush(genlib_vec, '\0');
    auto* genlib_str = abc::Vec_StrArray(genlib_vec);
    std::istringstream genlib(genlib_str);
    abc::Vec_StrFree(genlib_vec);

    std::vector<mockturtle::gate> gates;
    EXPECT_EQ(lorina::read_genlib(genlib, mockturtle::genlib_reader(gates)),
              lorina::return_code::success);

    return mockturtle::tech_library<9u>(gates);
  }
};

TEST_F(MockturtleTest, ExtractsAndGateCorrectly)
{
  auto tech_lib = CreateTechLib();
  LoadVerilog(kPrefix + "simple_and_gate_extract.v");

  sta::Network* network = sta_->getDbNetwork();
  sta::Vertex* flop_input_vertex = nullptr;
  for (sta::Vertex* vertex : sta_->endpoints()) {
    if (std::string(vertex->name(network)) == "output_flop/D") {
      flop_input_vertex = vertex;
    }
  }
  EXPECT_NE(flop_input_vertex, nullptr);

  LogicExtractorFactory logic_extractor(sta_.get(), &logger_);
  logic_extractor.AppendEndpoint(flop_input_vertex);
  LogicCut cut = logic_extractor.BuildLogicCut(tech_lib);

  EXPECT_EQ(cut.cut_instances().size(), 1);
  EXPECT_EQ(std::string(network->name(*cut.cut_instances().begin())), "_403_");
}

TEST_F(MockturtleTest, ExtractSideOutputsCorrectly)
{
  auto tech_lib = CreateTechLib();

  LoadVerilog(kPrefix + "side_outputs_extract.v");

  sta::Network* network = sta_->getDbNetwork();
  sta::Vertex* flop_input_vertex = nullptr;
  for (sta::Vertex* vertex : sta_->endpoints()) {
    if (std::string(vertex->name(network)) == "output_flop/D") {
      flop_input_vertex = vertex;
    }
  }
  EXPECT_NE(flop_input_vertex, nullptr);

  LogicExtractorFactory logic_extractor(sta_.get(), &logger_);
  logic_extractor.AppendEndpoint(flop_input_vertex);
  LogicCut cut = logic_extractor.BuildLogicCut(tech_lib);

  std::unordered_set<std::string> primary_output_names;
  for (sta::Net* net : cut.primary_outputs()) {
    primary_output_names.insert(network->name(net));
  }

  // Since a single net feeds both of these outputs should expect just 1 output
  EXPECT_EQ(cut.primary_outputs().size(), 1);
  EXPECT_THAT(primary_output_names, Contains("flop_net"));
}

TEST(MockturtleAigTest, CreateIteBuildsDeterministicIntermediateSignals)
{
  mockturtle::aig_network ntk;

  const auto cond = ntk.create_pi();
  const auto f_then = ntk.create_pi();
  const auto f_else = ntk.create_pi();

  const auto ite = ntk.create_ite(cond, f_then, f_else);

  EXPECT_EQ(ntk.num_gates(), 3);
  EXPECT_EQ(ite.index, 6);
  EXPECT_EQ(ite.complement, 1);

  EXPECT_THAT(GetAigFanins(ntk, ntk.index_to_node(4)),
              ElementsAre(Pair(1, 1), Pair(3, 0)));
  EXPECT_THAT(GetAigFanins(ntk, ntk.index_to_node(5)),
              ElementsAre(Pair(1, 0), Pair(2, 0)));
  EXPECT_THAT(GetAigFanins(ntk, ntk.index_to_node(6)),
              ElementsAre(Pair(4, 1), Pair(5, 1)));
}

TEST(MockturtleAigTest, CreateMajBuildsDeterministicIntermediateSignals)
{
  mockturtle::aig_network ntk;

  const auto a = ntk.create_pi();
  const auto b = ntk.create_pi();
  const auto c = ntk.create_pi();

  const auto maj = ntk.create_maj(a, b, c);

  EXPECT_EQ(ntk.num_gates(), 4);
  EXPECT_EQ(maj.index, 7);
  EXPECT_EQ(maj.complement, 1);

  EXPECT_THAT(GetAigFanins(ntk, ntk.index_to_node(4)),
              ElementsAre(Pair(1, 0), Pair(2, 0)));
  EXPECT_THAT(GetAigFanins(ntk, ntk.index_to_node(5)),
              ElementsAre(Pair(1, 1), Pair(2, 1)));
  EXPECT_THAT(GetAigFanins(ntk, ntk.index_to_node(6)),
              ElementsAre(Pair(3, 0), Pair(5, 1)));
  EXPECT_THAT(GetAigFanins(ntk, ntk.index_to_node(7)),
              ElementsAre(Pair(4, 1), Pair(6, 1)));
}

}  // namespace cut
