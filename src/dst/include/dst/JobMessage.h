/* Authors: Osama */
/*
 * Copyright (c) 2021, The Regents of the University of California
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the University nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once
#include <memory>
#include <string>
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