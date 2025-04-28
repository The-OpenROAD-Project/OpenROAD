// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#pragma once

#include <string>
#include <vector>
namespace dpl {
class Node;

class DetailedMgr;
class DetailedGenerator
{
 public:
  explicit DetailedGenerator(const char* name) : name_(name) {}
  virtual ~DetailedGenerator() = default;

  virtual const std::string& getName() const { return name_; }

  // Different methods for generating moves.  We _must_ overload these.  The
  // generated move should be stored in the manager.
  virtual bool generate(DetailedMgr* mgr, std::vector<Node*>& candiates) = 0;

  virtual void stats() = 0;

  virtual void init(DetailedMgr* mgr) = 0;

 private:
  const std::string name_;
};

}  // namespace dpl
