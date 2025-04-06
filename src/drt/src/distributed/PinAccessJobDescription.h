// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#pragma once
#include <boost/serialization/base_object.hpp>
#include <string>
#include <vector>

#include "dst/JobMessage.h"
#include "paUpdate.h"
namespace boost::serialization {
class access;
}
namespace drt {

class PinAccessJobDescription : public dst::JobDescription
{
 public:
  enum JobType
  {
    UPDATE_PA,
    UPDATE_PATTERNS,
    INIT_PA,
    INST_ROWS
  };
  void setPath(const std::string& path) { path_ = path; }
  void setType(JobType in) { type_ = in; }
  JobType getType() const { return type_; }
  const std::string getPath() const { return path_; }

 private:
  std::string path_;
  JobType type_{UPDATE_PA};

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version)
  {
    (ar) & boost::serialization::base_object<dst::JobDescription>(*this);
    (ar) & path_;
    (ar) & type_;
  }
  friend class boost::serialization::access;
};

}  // namespace drt
