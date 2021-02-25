/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2020, OpenROAD
// All rights reserved.
//
// BSD 3-Clause License
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <QAbstractTableModel>

#include "opendb/db.h"

namespace ord {
class OpenRoad;
}
namespace sta {
class Instance;
class Net;
class Pin;
}  // namespace sta
namespace gui {
class TimingPathsModel;
class TimingPathNode;
class TimingPath;
class TimingPathDetailModel;
class TimingPathsModel : public QAbstractTableModel
{
 public:
  TimingPathsModel();

  int rowCount(const QModelIndex& parent = QModelIndex()) const Q_DECL_OVERRIDE;
  int columnCount(const QModelIndex& parent
                  = QModelIndex()) const Q_DECL_OVERRIDE;

  QVariant data(const QModelIndex& index,
                int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
  QVariant headerData(int section,
                      Qt::Orientation orientation,
                      int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;

  void findInstances(std::string pattern, std::vector<odb::dbInst*>& insts);
  void findNets(std::string pattern, std::vector<odb::dbNet*>& nets);
  void findPins(std::string pattern, std::vector<odb::dbObject*>& pins);

 private:
  void populateModel();
  bool populatePaths(bool get_max = true, int path_count = 1000);

  ord::OpenRoad* openroad_;
  std::vector<sta::Instance*> findInstancesNetwork(std::string pattern);
  std::vector<sta::Net*> findNetsNetwork(std::string pattern);
  std::vector<sta::Pin*> findPinsNetwork(std::string pattern);

  std::vector<TimingPath*> timing_paths_;
  const static inline std::vector<std::string> _path_columns
      = {"Id", "Clock", "Req", "Arrival", "Slack", "Start", "End"};
};

class TimingPathNode
{
 public:
  TimingPathNode(odb::dbObject* pin,
                 bool is_rising,
                 float arrival,
                 float required,
                 float slack,
                 float slew,
                 float load)
      : pin_(pin),
        is_rising_(is_rising),
        arrival_(arrival),
        required_(required),
        slack_(slack),
        slew_(slew),
        load_(load)
  {
  }

  odb::dbObject* pin_;
  bool is_rising_;
  float arrival_;
  float required_;
  float slack_;
  float slew_;
  float load_;
};

class TimingPath
{
 public:
  TimingPath() {}

  void appendNode(const TimingPathNode& node) { path_nodes_.push_back(node); }
  int levelsCount() const { return path_nodes_.size(); }
  void setStartClock(const char* name) { startClk = name; }
  std::string getStartClock() const { return startClk; }
  void setEndClock(const char* name) { endClk = name; }
  std::string getEndClock() const { return endClk; }

  // Time will be returned in in nano seconds
  double getPathArrivalTime() const;
  double getPathRequiredTime() const;
  double getSlack() const;

  std::string getStartStageName() const;
  std::string getEndStageName() const;

  void printPath(const std::string& file_name) const;

 private:
  std::vector<TimingPathNode> path_nodes_;
  std::string startClk;
  std::string endClk;
};

class TimingPathDetailModel : public QAbstractTableModel
{
 public:
  TimingPathDetailModel() {}
  ~TimingPathDetailModel() {}

  int rowCount(const QModelIndex& parent = QModelIndex()) const Q_DECL_OVERRIDE;
  int columnCount(const QModelIndex& parent
                  = QModelIndex()) const Q_DECL_OVERRIDE;

  QVariant data(const QModelIndex& index,
                int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
  QVariant headerData(int section,
                      Qt::Orientation orientation,
                      int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;

  void populateModel(TimingPath* path);

 private:
  TimingPath* path_;
  const static inline std::vector<std::string> _path_details_columns
      = {"Fanout", "Cap", "Slew", "Delay", "Time", "Description"};
};
}  // namespace gui
