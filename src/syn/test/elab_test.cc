// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <cstdint>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

#include "driver.h"
#include "elab_testcases.h"
#include "equivalence_check.h"
#include "gtest/gtest.h"
#include "syn/ir/Bundle.h"
#include "syn/ir/Graph.h"
#include "syn/ir/Instance.h"
#include "syn/ir/TritModel.h"

int main(int argc, char** argv)
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

// Regression test for unique case (1'b1) with non-constant case items.
TEST(ElabTest, VariableIndexArrayWrite)
{
  auto result = syn::elaborateText(R"(
    module top (
        input  logic       clk,
        input  logic       rst_n,
        input  logic       wr_en,
        input  logic [1:0] wr_idx,
        input  logic [7:0] wr_data,
        input  logic [1:0] rd_idx,
        output logic [7:0] rd_data
    );
        typedef struct packed {
            logic       valid;
            logic [7:0] data;
        } entry_t;

        entry_t [3:0] mem_q, mem_n;

        always_comb begin
            mem_n = mem_q;
            if (wr_en) begin
                mem_n[wr_idx] = '{valid: 1'b1, data: wr_data};
            end
        end

        always_ff @(posedge clk or negedge rst_n) begin
            if (!rst_n)
                mem_q <= '0;
            else
                mem_q <= mem_n;
        end

        assign rd_data = mem_q[rd_idx].data;
    endmodule
  )",
                                   {"--top", "top"});
  ASSERT_TRUE(result) << "Elaboration failed";

  syn::Graph& g = *result;
  g.normalize();

  // SAT-check the write semantics: when wr_en=1, after one clock the
  // entry at index wr_idx must hold {valid:1, data:wr_data}.  We probe
  // the next state by reading it through rd_data with rd_idx==wr_idx;
  // this is a one-cycle unroll built from two TritModels of the same
  // graph, linked by m2's Dff outputs (= "mem_q on cycle 2") to m1's
  // Dff data inputs (= "mem_n on cycle 1").
  //
  // Regression: before the polarity fix, wr_data was routed to the
  // *non-matching* entries, so rd_data of mem_q[wr_idx] would not
  // observe wr_data and the SAT query found a counter-example.

  auto inputs_map = g.collectInputs();
  auto outputs_map = g.collectOutputs();
  ASSERT_TRUE(inputs_map.contains("wr_en"));
  ASSERT_TRUE(inputs_map.contains("rst_n"));
  ASSERT_TRUE(inputs_map.contains("wr_idx"));
  ASSERT_TRUE(inputs_map.contains("wr_data"));
  ASSERT_TRUE(inputs_map.contains("rd_idx"));
  ASSERT_TRUE(outputs_map.contains("rd_data"));

  syn::BundleView wr_en = inputs_map.at("wr_en");
  syn::BundleView rst_n = inputs_map.at("rst_n");
  syn::BundleView wr_idx = inputs_map.at("wr_idx");
  syn::BundleView wr_data = inputs_map.at("wr_data");
  syn::BundleView rd_idx = inputs_map.at("rd_idx");
  syn::BundleView rd_data = outputs_map.at("rd_data");

  // Aggregate all Dff data inputs and outputs in matching order, so
  // dff_in[i] is the data bit that captures into dff_out[i].
  syn::Bundle dff_in, dff_out;
  g.forEachInstance([&](const syn::Instance* inst) {
    if (auto* dff = inst->try_as<syn::Dff>()) {
      dff_in.append(dff->data());
      dff_out.append(g.output(inst));
    }
  });
  ASSERT_GT(dff_in.width(), 0u);
  ASSERT_EQ(dff_in.width(), dff_out.width());

  syn::Bundle all_inputs;
  for (auto& [name, bv] : inputs_map) {
    all_inputs.append(bv);
  }
  syn::Bundle support;
  support.append(all_inputs);
  support.append(dff_out);

  syn::Solver solver;
  syn::TritModel m1(solver, g), m2(solver, g);
  m1.encodeCone(support, dff_in);
  m2.encodeCone(support, rd_data);

  auto eq = [&](int v1, int v2) {
    solver.addClause({v1, -v2});
    solver.addClause({-v1, v2});
  };

  // Link m2.dff_out[i] (current state on cycle 2) to m1.dff_in[i]
  // (next state computed on cycle 1).
  for (uint32_t i = 0; i < dff_in.width(); i++) {
    eq(m1.valVar(dff_in[i]), m2.valVar(dff_out[i]));
    eq(m1.defVar(dff_in[i]), m2.defVar(dff_out[i]));
  }

  // Inputs defined in both cycles.
  for (uint32_t i = 0; i < all_inputs.width(); i++) {
    solver.addClause({m1.defVar(all_inputs[i])});
    solver.addClause({m2.defVar(all_inputs[i])});
  }

  // Cycle 1: wr_en=1, rst_n=1 (no async reset).
  solver.addClause({m1.valVar(wr_en.asNet())});
  solver.addClause({m1.valVar(rst_n.asNet())});

  // Cycle 2 reads the entry that cycle 1 wrote.
  for (uint32_t i = 0; i < wr_idx.width(); i++) {
    eq(m1.valVar(wr_idx[i]), m2.valVar(rd_idx[i]));
  }

  // Look for a counter-example: rd_data either undefined or != wr_data.
  int diff = 0;
  for (uint32_t i = 0; i < rd_data.width(); i++) {
    int x = solver.encodeXor(m2.valVar(rd_data[i]), m1.valVar(wr_data[i]));
    int bit = solver.encodeOr(-m2.defVar(rd_data[i]), x);
    diff = (i == 0) ? bit : solver.encodeOr(diff, bit);
  }
  solver.addClause({diff});

  EXPECT_EQ(solver.solve(), syn::Solver::Unsat)
      << "wr_en=1 should make mem[wr_idx].data observable as wr_data on the "
         "next cycle";
}

// Regression test for unique case (1'b1) with non-constant case items.
// The SwitchHelper::branch() skip optimization must not fire when case
// item patterns contain non-constant nets.
TEST(ElabTest, UniqueCaseOneHot)
{
  auto result
      = syn::elaborateText(R"(
    module inner(
        input  logic clk,
        input  logic cs,
        input  logic we,
        output logic rd
    );
        assign rd = cs & we;
    endmodule

    module top (
        input  logic clk,
        input  logic sel_a,
        input  logic sel_b,
        input  logic val_a,
        input  logic val_b,
        output logic [1:0] rd
    );
        logic [1:0] cs;
        logic [1:0] we;

        always_comb begin
            unique case (1'b1)
                sel_a: begin
                    cs = 2'b01;
                    we = {1'b0, val_a};
                end
                sel_b: begin
                    cs = 2'b10;
                    we = {val_b, 1'b0};
                end
                default: begin
                    cs = 2'b00;
                    we = 2'b00;
                end
            endcase
        end

        for (genvar g = 0; g < 2; g++) begin : gen_inner
            inner u_inner (
                .clk (clk),
                .cs  (cs[g]),
                .we  (we[g]),
                .rd  (rd[g])
            );
        end
    endmodule
  )",
                           {"--top", "top", "--blackboxed-module", "inner"});
  ASSERT_TRUE(result) << "Elaboration failed";

  syn::Graph& g = *result;
  g.normalize();

  // After normalization, the graph must contain Mux instances for the
  // case branches.  Before the fix, the branch-skip optimization in
  // SwitchHelper::branch() would fire incorrectly for case(1'b1) with
  // non-constant items (sel_a, sel_b), producing constant-zero cs/we.
  EXPECT_GE(g.count<syn::Mux>(), 2)
      << "Expected Mux instances for unique case (1'b1) branches";

  // Verify the Other (blackboxed inner) instances receive non-constant
  // inputs on cs and we ports.
  int inner_count = 0;
  g.forEachInstance([&](const syn::Instance* inst) {
    auto* other = inst->try_as<syn::Other>();
    if (!other || other->cellType() != "inner") {
      return;
    }
    inner_count++;
    for (auto& port : other->ports()) {
      if (port.direction != syn::Other::Port::kInput || port.name == "clk") {
        continue;
      }
      bool all_const = true;
      for (uint32_t i = 0; i < port.value.width(); i++) {
        if (!port.value[i].isConst()) {
          all_const = false;
        }
      }
      EXPECT_FALSE(all_const)
          << "inner port " << port.name << " should not be all-constant";
    }
  });
  EXPECT_EQ(inner_count, 2) << "Expected two inner instances";
}

// Parameterized SAT-equivalence check: slang frontend vs golden IR.
namespace syn {

class ElabSlangTest : public ::testing::TestWithParam<
                          std::pair<std::string_view, std::string_view>>
{
};

TEST_P(ElabSlangTest, MatchesGolden)
{
  const auto& [verilog, golden_ir] = GetParam();

  auto slang_g = elaborateText(std::string(verilog), {});
  ASSERT_TRUE(slang_g) << "Slang elaboration failed";
  slang_g->normalize();

  std::istringstream is{std::string(golden_ir)};
  std::unique_ptr<Graph> golden = Graph::parse(is);
  golden->normalize();

  // If the golden IR is blank (or parsed empty), dump the gate-side IR so
  // the test author can paste it in as the starting golden.
  if (golden->collectOutputs().empty()) {
    std::stringstream ss;
    slang_g->dump(ss);
    ADD_FAILURE() << "; empty gold side\n; gate side:\n" << ss.str();
    return;
  }

  equivalenceCheck(
      *golden, *slang_g, /*inputsDefined=*/true, /*allowRefinement=*/false);
}

INSTANTIATE_TEST_SUITE_P(Elab,
                         ElabSlangTest,
                         ::testing::ValuesIn(elabTestcases));

}  // namespace syn
