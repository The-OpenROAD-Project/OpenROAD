// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

//
// syn IR backend for slang-elab
//
// BackendGraphBuilder implementation targeting syn::Graph.
//
#pragma once

#include <cassert>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "ir.h"
#include "slang/ast/expressions/Operator.h"
#include "slang_frontend.h"
#include "syn/ir/Bundle.h"
#include "syn/ir/Graph.h"

namespace utl {
class Logger;
}

namespace slang_frontend {

// Wraps `logger->error(utl::SYN, code, "{}", message)`. Defined in error.cc
// so its TU is the only one that pulls utl/Logger.h (which transitively
// brings spdlog's bundled fmt). slang headers pull a different fmt version,
// and mixing the two in one TU causes inline-namespace collisions.
[[noreturn]] void reportError(utl::Logger* logger,
                              int code,
                              std::string_view message);

class BackendGraphBuilder : public BackendGraphBuilderBase
{
 public:
  BackendGraphBuilder();

  std::unique_ptr<BackendGraphBuilder> start_new_graph(
      std::string_view graph_name);
  void finalize();

  // Access the underlying graph. The constructor populates graph_, and it
  // is only emptied by the final move-out in driver.cc after which graph()
  // is no longer called.
  syn::Graph& graph()
  {
    assert(graph_);
    return *graph_;
  }

  // Helpers
  std::string new_id(const std::string& base = std::string());
  syn::Bundle toBundle(const ir::Value& v);
  ir::Value fromBundle(const syn::Bundle& b);

  // BackendGraphBuilderBase interface
  ir::Value Unop(ast::UnaryOperator op,
                 ir::Value a,
                 bool a_signed,
                 uint64_t y_width) override;
  ir::Value Biop(ast::BinaryOperator op,
                 ir::Value a,
                 ir::Value b,
                 bool a_signed,
                 bool b_signed,
                 uint64_t y_width) override;
  ir::Value Demux(ir::Value a, ir::Value s) override;
  ir::Value Bwmux(ir::Value a, ir::Value b, ir::Value s) override;
  ir::Value Bmux(ir::Value a, ir::Value s) override;
  ir::Value Shift(ir::Value a,
                  ir::Value s,
                  bool s_signed,
                  uint64_t result_width) override;
  ir::Value Shiftx(ir::Value a,
                   ir::Value s,
                   bool s_signed,
                   uint64_t result_width) override;
  ir::Value Mux(ir::Value a, ir::Value b, ir::Net s) override;
  ir::Value add_placeholder_signal(uint64_t width,
                                   std::string_view name_suggestion,
                                   bool public_name) override;
  void add_input(std::string_view name, ir::Value signal) override;
  void add_output(std::string_view name, ir::Value signal) override;
  void add_instance(std::string_view cell_type,
                    std::vector<PortConnection> ports) override;
  void connect(ir::Value target, ir::Value source) override;
  void set_initialization(ir::Value signal, ir::Const init_value) override;
  void add_memory_init(std::string_view name,
                       uint64_t bit_offset,
                       bool big_endian,
                       ir::Const data) override;
  void add_dual_edge_aldff(const std::string& base_name,
                           ir::Value clk,
                           ir::Value aload,
                           ir::Value d,
                           ir::Value q,
                           ir::Value ad,
                           bool aload_polarity) override;
  void add_dff(std::string_view name,
               const ir::Value& clk,
               const ir::Value& d,
               const ir::Value& q,
               bool clk_polarity) override;
  void add_dffe(std::string_view name,
                const ir::Value& clk,
                const ir::Value& en,
                const ir::Value& d,
                const ir::Value& q,
                bool clk_polarity,
                bool en_polarity) override;
  void add_aldff(std::string_view name,
                 const ir::Value& clk,
                 const ir::Value& aload,
                 const ir::Value& d,
                 const ir::Value& q,
                 const ir::Value& ad,
                 bool clk_polarity,
                 bool aload_polarity) override;

  std::unique_ptr<syn::Graph> graph_;
  unsigned next_id = 0;
};

// No-op attribute guard for syn backend. The syn IR does not track
// HDL attributes and source locations, so we silently drop them.
class AttributeGuard
{
 public:
  AttributeGuard(GraphBuilder&) {}

  template <typename IdT, typename ValT>
  void set(IdT, ValT)
  {
  }
};

}  // namespace slang_frontend
