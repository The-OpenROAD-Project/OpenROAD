// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#pragma once
#include <boost/serialization/base_object.hpp>
#include <string>
#include <utility>
#include <vector>

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
