// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#pragma once
#include <boost/serialization/access.hpp>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "db/obj/frAccess.h"
namespace drt {
class frInst;
class frInstTerm;
class frDesign;
class paUpdate
{
 public:
  paUpdate() = default;

  void addPinAccess(frPin* pin, std::vector<std::unique_ptr<frPinAccess>> pa)
  {
    pin_access_.emplace_back(pin, std::move(pa));
  }
  void setInstRows(const std::vector<std::vector<frInst*>>& in)
  {
    inst_rows_ = in;
  }
  void setGroupResults(
      const std::vector<std::pair<frInstTerm*, std::vector<frAccessPoint*>>>&
          in)
  {
    group_results_ = in;
  }
  void addGroupResult(
      const std::pair<frInstTerm*, std::vector<frAccessPoint*>>& in)
  {
    group_results_.push_back(in);
  }
  std::vector<std::pair<frPin*, std::vector<std::unique_ptr<frPinAccess>>>>&
  getPinAccess()
  {
    return pin_access_;
  }
  const std::vector<std::vector<frInst*>>& getInstRows() const
  {
    return inst_rows_;
  }
  const std::vector<std::pair<frInstTerm*, std::vector<frAccessPoint*>>>&
  getGroupResults() const
  {
    return group_results_;
  }
  static void serialize(const paUpdate& update, const std::string& path);
  static void deserialize(frDesign* design,
                          paUpdate& update,
                          const std::string& path);

 private:
  std::vector<std::pair<frPin*, std::vector<std::unique_ptr<frPinAccess>>>>
      pin_access_;
  std::vector<std::vector<frInst*>> inst_rows_;
  std::vector<std::pair<frInstTerm*, std::vector<frAccessPoint*>>>
      group_results_;

  template <class Archive>
  void serialize(Archive& ar, unsigned int version);

  friend class boost::serialization::access;
};
}  // namespace drt
