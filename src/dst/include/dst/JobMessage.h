// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#pragma once
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace boost::serialization {
class access;
}
namespace dst {
class Distributed;
class WorkerConnection;
class BalancerConnection;

class JobDescription
{
 public:
  JobDescription() {}
  virtual ~JobDescription() {}

 private:
  template <class Archive>
  void serialize(Archive& ar, const unsigned int version)
  {
  }
  friend class boost::serialization::access;
};

class JobMessage
{
 public:
  enum JobType : int8_t
  {
    ROUTING,
    UPDATE_DESIGN,
    BALANCER,
    PIN_ACCESS,
    GRDR_INIT,
    SUCCESS,
    ERROR,
    NONE
  };
  enum MessageType : int8_t
  {
    UNICAST,
    BROADCAST
  };
  JobMessage(JobType job_type = NONE, MessageType msg_type = UNICAST)
      : msg_type_(msg_type), job_type_(job_type)
  {
  }
  void setJobDescription(std::unique_ptr<JobDescription> in)
  {
    desc_ = std::move(in);
  }
  std::unique_ptr<JobDescription>& getJobDescriptionRef() { return desc_; }
  void addJobDescription(std::unique_ptr<JobDescription> in)
  {
    descs_.push_back(std::move(in));
    // desc_ = std::move(in);
  }
  const std::vector<std::unique_ptr<JobDescription>>& getAllJobDescriptions()
  {
    return descs_;
  }
  void setJobType(JobType in) { job_type_ = in; }
  JobDescription* getJobDescription() { return desc_.get(); }
  JobType getJobType() const { return job_type_; }
  MessageType getMessageType() const { return msg_type_; }

 private:
  MessageType msg_type_;
  JobType job_type_;
  std::unique_ptr<JobDescription> desc_;
  std::vector<std::unique_ptr<JobDescription>> descs_;

  static constexpr const char* EOP
      = "\r\nENDOFPACKET\r\n";  // ENDOFPACKET SEQUENCE

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version);

  friend class boost::serialization::access;

  enum SerializeType
  {
    READ,
    WRITE
  };
  static bool serializeMsg(SerializeType type,
                           JobMessage& msg,
                           std::string& str);
  friend class dst::Distributed;
  friend class dst::WorkerConnection;
  friend class dst::BalancerConnection;
};

}  // namespace dst
