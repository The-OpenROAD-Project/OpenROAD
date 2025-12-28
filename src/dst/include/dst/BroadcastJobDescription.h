// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#pragma once
#include "boost/serialization/base_object.hpp"
#include "dst/JobMessage.h"
namespace boost::serialization {
class access;
}
namespace dst {

class BroadcastJobDescription : public JobDescription
{
 public:
  void setWorkersCount(unsigned short count) { workers_count_ = count; }
  unsigned short getWorkersCount() const { return workers_count_; }

 private:
  unsigned short workers_count_{0};

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version)
  {
    (ar) & boost::serialization::base_object<dst::JobDescription>(*this);
    (ar) & workers_count_;
  }
  friend class boost::serialization::access;
};
}  // namespace dst
