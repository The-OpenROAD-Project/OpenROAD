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
namespace drt {

class RoutingJobDescription : public dst::JobDescription
{
 public:
  void setGlobalsPath(const std::string& path) { router_cfg_path_ = path; }
  void setSharedDir(const std::string& path) { shared_dir_ = path; }
  void setDesignPath(const std::string& path) { design_path_ = path; }
  void setGuidePath(const std::string& path) { guide_path_ = path; }
  void setWorkers(const std::vector<std::pair<int, std::string>>& workers)
  {
    workers_ = workers;
  }
  void setUpdates(const std::vector<std::string>& updates)
  {
    updates_ = updates;
  }
  void setSendEvery(int val) { send_every_ = val; }
  void setViaData(const std::string& val) { via_data_ = val; }
  void setDesignUpdate(const bool& value) { design_update_ = value; }
  const std::string& getGlobalsPath() const { return router_cfg_path_; }
  const std::string& getSharedDir() const { return shared_dir_; }
  const std::string& getDesignPath() const { return design_path_; }
  const std::string& getGuidePath() const { return guide_path_; }
  const std::vector<std::pair<int, std::string>>& getWorkers()
  {
    return workers_;
  }
  const std::vector<std::string>& getUpdates() { return updates_; }
  bool isDesignUpdate() const { return design_update_; }
  int getSendEvery() const { return send_every_; }
  const std::string& getViaData() const { return via_data_; }

 private:
  std::string router_cfg_path_;
  std::string design_path_;
  std::string shared_dir_;
  std::string guide_path_;
  std::vector<std::pair<int, std::string>> workers_;
  std::vector<std::string> updates_;
  std::string via_data_;
  bool design_update_{false};
  int send_every_{10};

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version)
  {
    (ar) & boost::serialization::base_object<dst::JobDescription>(*this);
    (ar) & router_cfg_path_;
    (ar) & design_path_;
    (ar) & shared_dir_;
    (ar) & guide_path_;
    (ar) & workers_;
    (ar) & updates_;
    (ar) & via_data_;
    (ar) & design_update_;
    (ar) & send_every_;
  }
  friend class boost::serialization::access;
};

}  // namespace drt
