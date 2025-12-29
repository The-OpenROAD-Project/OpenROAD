// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#pragma once
#include <string>

#include "boost/serialization/base_object.hpp"
#include "dst/JobMessage.h"
namespace boost::serialization {
class access;
}
namespace dst {

class BalancerJobDescription : public JobDescription
{
 public:
  void setWorkerIP(const std::string& ip) { worker_ip_ = ip; }
  void setWorkerPort(unsigned short port) { worker_port_ = port; }
  std::string getWorkerIP() const { return worker_ip_; }
  unsigned short getWorkerPort() const { return worker_port_; }

 private:
  std::string worker_ip_;
  unsigned short worker_port_{0};

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version)
  {
    (ar) & boost::serialization::base_object<dst::JobDescription>(*this);
    (ar) & worker_ip_;
    (ar) & worker_port_;
  }
  friend class boost::serialization::access;
};
}  // namespace dst
