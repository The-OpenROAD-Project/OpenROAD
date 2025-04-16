// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#pragma once

#include <utility>
#include <vector>

#include "abc_library_factory.h"
#include "base/abc/abc.h"
#include "db_sta/dbNetwork.hh"
#include "rmp/unique_name.h"
#include "sta/GraphClass.hh"
#include "sta/NetworkClass.hh"
#include "utl/Logger.h"
#include "utl/deleter.h"

namespace rmp {
class LogicCut
{
 public:
  LogicCut(std::vector<sta::Net*>& primary_inputs,
           std::vector<sta::Net*>& primary_outputs,
           sta::InstanceSet& cut_instances)
      : primary_inputs_(std::move(primary_inputs)),
        primary_outputs_(std::move(primary_outputs)),
        cut_instances_(std::move(cut_instances))
  {
  }
  ~LogicCut() = default;

  const std::vector<sta::Net*>& primary_inputs() const
  {
    return primary_inputs_;
  }
  const std::vector<sta::Net*>& primary_outputs() const
  {
    return primary_outputs_;
  }
  const sta::InstanceSet& cut_instances() const { return cut_instances_; }

  bool IsEmpty() const
  {
    return primary_inputs_.empty() && primary_outputs_.empty()
           && cut_instances_.empty();
  }

  utl::UniquePtrWithDeleter<abc::Abc_Ntk_t> BuildMappedAbcNetwork(
      AbcLibrary& abc_library,
      sta::dbNetwork* network,
      utl::Logger* logger);

  void InsertMappedAbcNetwork(abc::Abc_Ntk_t* abc_network,
                              AbcLibrary& abc_library,
                              sta::dbNetwork* network,
                              UniqueName& unique_name,
                              utl::Logger* logger);

 private:
  std::vector<sta::Net*> primary_inputs_;
  std::vector<sta::Net*> primary_outputs_;
  sta::InstanceSet cut_instances_;
};
}  // namespace rmp
