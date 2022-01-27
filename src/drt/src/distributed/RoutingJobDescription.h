
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
  RoutingJobDescription() {}
  void setWorkerPath(const std::string& path) { path_ = path; }
  void setGlobalsPath(const std::string& path) { globals_path_ = path; }
  void setSharedDir(const std::string& path) { shared_dir_ = path; }
  void setDesignPath(const std::string& path) { design_path_ = path; }
  const std::string& getWorkerPath() const { return path_; }
  const std::string& getGlobalsPath() const { return globals_path_; }
  const std::string& getSharedDir() const { return shared_dir_; }
  const std::string& getDesignPath() const { return design_path_; }

 private:
  std::string path_;
  std::string globals_path_;
  std::string design_path_;
  std::string shared_dir_;
  template <class Archive>
  void serialize(Archive& ar, const unsigned int version)
  {
    (ar) & boost::serialization::base_object<dst::JobDescription>(*this);
    (ar) & path_;
    (ar) & globals_path_;
    (ar) & design_path_;
    (ar) & shared_dir_;
  }
  friend class boost::serialization::access;
};
}  // namespace fr