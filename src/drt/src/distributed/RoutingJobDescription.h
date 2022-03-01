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
#include <boost/serialization/base_object.hpp>
#include <string>

#include "dst/JobMessage.h"
namespace boost::serialization {
class access;
}
namespace fr {

class RoutingJobDescription : public dst::JobDescription
{
 public:
  RoutingJobDescription() : reply_port_(0), idx_in_batch_(0)  {}
  void setWorkerStr(const std::string& worker) { worker_str_ = worker; }
  void setGlobalsPath(const std::string& path) { globals_path_ = path; }
  void setSharedDir(const std::string& path) { shared_dir_ = path; }
  void setDesignPath(const std::string& path) { design_path_ = path; }
  void setReplyIp(const std::string& ip) { reply_ip_ = ip; }
  void setReplyPort(unsigned short port) { reply_port_ = port; }
  void setIdxInBatch(int idx) { idx_in_batch_ = idx; }
  void setWorkers(const std::vector<std::string>& workers) { workers_ = workers; }
  const std::string& getWorkerStr() const { return worker_str_; }
  const std::string& getGlobalsPath() const { return globals_path_; }
  const std::string& getSharedDir() const { return shared_dir_; }
  const std::string& getDesignPath() const { return design_path_; }
  unsigned short getReplyPort() const { return reply_port_; }
  const std::string& getReplyIp() const { return reply_ip_; }
  int getIdxInBatch() const { return idx_in_batch_; }
  const std::vector<std::string>& getWorkers() { return workers_; }  

 private:
  std::string worker_str_;
  std::string globals_path_;
  std::string design_path_;
  std::string shared_dir_;
  std::string reply_ip_;
  unsigned short reply_port_;
  int idx_in_batch_;
  std::vector<std::string> workers_;
  template <class Archive>
  void serialize(Archive& ar, const unsigned int version)
  {
    (ar) & boost::serialization::base_object<dst::JobDescription>(*this);
    (ar) & globals_path_;
    (ar) & design_path_;
    (ar) & shared_dir_;
    (ar) & reply_ip_;
    (ar) & reply_port_;
    (ar) & idx_in_batch_;
    (ar) & workers_;
  }
  friend class boost::serialization::access;
};
}  // namespace fr
