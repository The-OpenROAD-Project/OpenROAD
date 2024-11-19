// Copyright 2024 Google LLC
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#pragma once

#include <unordered_set>
#include <utility>
#include <vector>

#include "abc_library_factory.h"
#include "base/abc/abc.h"
#include "db_sta/dbNetwork.hh"
#include "sta/GraphClass.hh"
#include "sta/NetworkClass.hh"
#include "unique_name.h"
#include "utl/Logger.h"
#include "utl/deleter.h"

namespace rmp {
class LogicCut
{
 public:
  LogicCut(std::vector<sta::Net*>& primary_inputs,
           std::vector<sta::Net*>& primary_outputs,
           std::unordered_set<sta::Instance*>& cut_instances)
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
  const std::unordered_set<sta::Instance*>& cut_instances() const
  {
    return cut_instances_;
  }

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
                              sta::dbNetwork* network,
                              UniqueName& unique_name,
                              utl::Logger* logger);

 private:
  std::vector<sta::Net*> primary_inputs_;
  std::vector<sta::Net*> primary_outputs_;
  std::unordered_set<sta::Instance*> cut_instances_;
};
}  // namespace rmp
