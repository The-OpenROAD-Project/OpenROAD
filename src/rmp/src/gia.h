// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2026, The OpenROAD Authors

#pragma once

#include <cstddef>
#include <functional>
#include <vector>

#include "aig/gia/gia.h"
#include "cut/abc_library_factory.h"
#include "utl/unique_name.h"

namespace sta {
class dbSta;
class Vertex;
}  // namespace sta

namespace utl {
class Logger;
}  // namespace utl

namespace rmp {

struct GiaOp final
{
  using AigManager = utl::UniquePtrWithDeleter<abc::Gia_Man_t>;
  using OpExecutor = std::function<void(AigManager&)>;

  size_t id;
  OpExecutor op;
  bool operator==(const GiaOp& other) const { return id == other.id; }
  bool operator!=(const GiaOp& other) const { return !(*this == other); }

  template <typename H>
  friend H AbslHashValue(H h, const GiaOp& gia_op)
  {
    return H::combine(std::move(h), gia_op.id);
  }
};

std::vector<GiaOp> GiaOps(utl::Logger* logger);

void RunGia(sta::dbSta* sta,
            const std::vector<sta::Vertex*>& candidate_vertices,
            cut::AbcLibrary& abc_library,
            const std::vector<GiaOp>& gia_ops,
            size_t resize_iters,
            utl::UniqueName& name_generator,
            utl::Logger* logger);
}  // namespace rmp
