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
#include <boost/serialization/vector.hpp>
#include <string>

#include "dst/JobMessage.h"
#include "rmp/Restructure.h"
namespace boost::serialization {
class access;
}
namespace rmp {

class RestructureJobDescription : public dst::JobDescription
{
 public:
  RestructureJobDescription()
      : num_instances_(-1), level_gain_(-1), delay_(-1), iterations_(1)
  {
  }
  void setBlifPath(const std::string& path) { blif_path_ = path; }
  void setWorkDirName(const std::string& path) { work_dir_name_ = path; }
  void setPostABCScript(const std::string& path) { post_abc_script_ = path; }
  void setHiCellPort(const std::string& cell, const std::string& port)
  {
    hicell_ = cell;
    hiport_ = port;
  }
  void setLoCellPort(const std::string& cell, const std::string& port)
  {
    locell_ = cell;
    loport_ = port;
  }
  void setNumInstances(int value) { num_instances_ = value; }
  void setIterations(int value) { iterations_ = value; }
  void setLevelGain(int value) { level_gain_ = value; }
  void setDelay(float value) { delay_ = value; }
  void setMode(Mode value) { mode_ = value; }
  void setLibFiles(const std::vector<std::string>& list) { lib_files_ = list; }

  const std::string& getLoCell() const { return locell_; }
  const std::string& getLoPort() const { return loport_; }
  const std::string& getHiCell() const { return hicell_; }
  const std::string& getHiPort() const { return hiport_; }
  Mode getMode() const { return mode_; }
  int getNumInstances() const { return num_instances_; }
  int getLevelGain() const { return level_gain_; }
  float getDelay() const { return delay_; }
  ushort getIterations() const { return iterations_; }
  const std::string& getBlifPath() const { return blif_path_; }
  const std::string& getWorkDirName() const { return work_dir_name_; }
  const std::string& getPostABCScript() const { return post_abc_script_; }
  const std::vector<std::string>& getLibFiles() const { return lib_files_; }

 private:
  std::string locell_;
  std::string loport_;
  std::string hicell_;
  std::string hiport_;
  Mode mode_;
  int num_instances_;
  int level_gain_;
  float delay_;
  ushort iterations_;
  std::string blif_path_;
  std::string work_dir_name_;
  std::string post_abc_script_;
  std::vector<std::string> lib_files_;

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version)
  {
    (ar) & boost::serialization::base_object<dst::JobDescription>(*this);
    (ar) & locell_;
    (ar) & loport_;
    (ar) & hicell_;
    (ar) & hiport_;
    (ar) & mode_;
    (ar) & num_instances_;
    (ar) & level_gain_;
    (ar) & iterations_;
    (ar) & delay_;
    (ar) & blif_path_;
    (ar) & work_dir_name_;
    (ar) & post_abc_script_;
    (ar) & lib_files_;
  }
  friend class boost::serialization::access;
};
}  // namespace rmp
