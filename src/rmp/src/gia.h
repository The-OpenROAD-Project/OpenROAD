// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#pragma once

#include <cstddef>

#include "cut/abc_library_factory.h"
#include "db_sta/dbSta.hh"
#include "utl/Logger.h"
#include "utl/unique_name.h"

namespace rmp {

struct GiaOp final
{
  size_t id;
  std::function<void(abc::Gia_Man_t*&)> op;
  bool operator==(const GiaOp& other) const { return id == other.id; }
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
