/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2020, The Regents of the University of California
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

#include "gui/gui.h"
#include "odb/db.h"
#include "odb/dbBlockCallBackObj.h"
#include "sta/PathExpanded.hh"
#include "sta/PathRef.hh"
#include "sta/Sta.hh"
namespace odb {
class dbBlock;
class dbFill;
class dbInst;
class dbMaster;
class dbNet;
class dbITerm;
class dbWire;
class dbBTerm;
class dbBPin;
class dbBlockage;
class dbObstruction;
class dbRegion;
class dbRow;
class dbSWire;
}  // namespace odb
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
class GuiDBChangeListener;

class TimingPathsModel : public QAbstractTableModel
{
 Q_OBJECT

 public:
  TimingPathsModel(bool get_max = true, int path_count = 100);
  ~TimingPathsModel();

  int rowCount(const QModelIndex& parent = QModelIndex()) const Q_DECL_OVERRIDE;
  int columnCount(const QModelIndex& parent
                  = QModelIndex()) const Q_DECL_OVERRIDE;

  QVariant data(const QModelIndex& index,
                int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
  QVariant headerData(int section,
                      Qt::Orientation orientation,
                      int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;

  TimingPath* getPathAt(int index) const { return timing_paths_[index]; }

  void resetModel();
  void populateModel(bool get_max = true, int path_count = 100);

 public slots:
  void sort(int col_index, Qt::SortOrder sort_order) override;

 private:
  bool populatePaths(bool get_max = true, int path_count = 100, bool clockExpanded = false);

  ord::OpenRoad* openroad_;
  std::vector<TimingPath*> timing_paths_;

  static const std::vector<std::string> _path_columns;
  enum Column : int;
};

struct TimingPathNode
{
  TimingPathNode(odb::dbObject* pin,
                 bool is_rising,
                 float arrival,
                 float required,
                 float delay,
                 float slack,
                 float slew,
                 float load)
      : pin_(pin),
        is_rising_(is_rising),
        arrival_(arrival),
        required_(required),
        delay_(delay),
        slack_(slack),
        slew_(slew),
        load_(load)
  {
  }

  std::string getNodeName(bool include_master = false) const;
  std::string getNetName() const;

  odb::dbObject* pin_;
  bool is_rising_;
  float arrival_;
  float required_;
  float delay_;
  float slack_;
  float slew_;
  float load_;
};

class TimingPath
{
 public:
  TimingPath(int path_index)
      : path_nodes_(),
        start_clk_(),
        end_clk_(),
        slack_(0),
        path_delay_(0),
        arr_time_(0),
        req_time_(0),
        path_index_(path_index)
  {
  }

  void appendNode(const TimingPathNode& node) { path_nodes_.push_back(node); }
  int levelsCount() const { return path_nodes_.size(); }
  void setStartClock(const char* name) { start_clk_ = name; }
  std::string getStartClock() const { return start_clk_; }
  void setEndClock(const char* name) { end_clk_ = name; }
  std::string getEndClock() const { return end_clk_; }

  float getPathArrivalTime() const { return arr_time_; }
  void setPathArrivalTime(float arr) { arr_time_ = arr; }
  float getPathRequiredTime() const { return req_time_; }
  void setPathRequiredTime(float req) { req_time_ = req; }
  float getSlack() const { return slack_; }
  void setSlack(float slack) { slack_ = slack; }
  float getPathDelay() const { return path_delay_; }
  void setPathDelay(float del) { path_delay_ = del; }

  int getPathIndex() const { return path_index_; }

  TimingPathNode getNodeAt(int index) const { return path_nodes_[index]; }

  std::string getStartStageName() const;
  std::string getEndStageName() const;

 private:
  std::vector<TimingPathNode> path_nodes_;
  std::string start_clk_;
  std::string end_clk_;
  float slack_;
  float path_delay_;
  float arr_time_;
  float req_time_;
  int path_index_;
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
  // Unicode symbols
  static const char* up_down_arrows;
  static const char* up_arrow;
  static const char* down_arrow;
  static const std::vector<std::string> _path_details_columns;
  enum Column : int;
};

class TimingPathRenderer : public gui::Renderer
{
 public:
  TimingPathRenderer();
  ~TimingPathRenderer();
  void highlight(TimingPath* path);

  void highlightNode(int node_id);
  virtual void drawObjects(gui::Painter& /* painter */) override;

  TimingPath* getPathToRender() { return path_; }

 private:
  void highlightInst(odb::dbInst* inst,
                     gui::Painter& painter,
                     const gui::Painter::Color& color);
  void highlightStage(gui::Painter& painter);
  void highlightTerm(odb::dbBTerm* term, gui::Painter& painter);
  void highlightNet(odb::dbNet* net,
                    odb::dbObject* source_node,
                    odb::dbObject* sink_node,
                    gui::Painter& painter);

  // Expanded path is owned by PathRenderer.
  TimingPath* path_;

  int highlight_node_;

  static gui::Painter::Color inst_highlight_color_;
  static gui::Painter::Color path_inst_color_;
  static gui::Painter::Color term_color_;
  static gui::Painter::Color signal_color_;
  static gui::Painter::Color clock_color_;
};

class GuiDBChangeListener : public QObject, public odb::dbBlockCallBackObj
{
  Q_OBJECT
 public:
  GuiDBChangeListener() : isDirty_(false) {}

  void inDbInstCreate(odb::dbInst* inst) override
  {
    callback("inDbInstCreate", inst);
  }
  void inDbInstDestroy(odb::dbInst* inst) override
  {
    callback("inDbInstDestroy", inst);
  }
  void inDbInstSwapMasterBefore(odb::dbInst* inst,
                                odb::dbMaster* master) override
  {
    callback("inDbInstSwapMasterBefore", inst, master);
  }
  void inDbInstSwapMasterAfter(odb::dbInst* inst) override
  {
    callback("inDbInstSwapMasterAfter", inst);
  }
  void inDbNetCreate(odb::dbNet* net) override
  {
    callback("inDbNetCreate", net);
  }
  void inDbNetDestroy(odb::dbNet* net) override
  {
    callback("inDbNetDestroy", net);
  }
  void inDbITermPostConnect(odb::dbITerm* iterm) override
  {
    callback("inDbITermPostConnect", iterm);
  }
  void inDbITermPreDisconnect(odb::dbITerm* iterm) override
  {
    callback("inDbITermPreDisconnect", iterm);
  }
  void inDbITermDestroy(odb::dbITerm* iterm) override
  {
    callback("inDbITermDestroy", iterm);
  }
  void inDbBTermPostConnect(odb::dbBTerm* bterm) override
  {
    callback("inDbBTermPostConnect", bterm);
  }
  void inDbBTermPreDisconnect(odb::dbBTerm* bterm) override
  {
    callback("inDbBTermPreDisconnect", bterm);
  }
  void inDbBTermDestroy(odb::dbBTerm* bterm) override
  {
    callback("inDbBTermDestroy", bterm);
  }
  void inDbWireCreate(odb::dbWire* wire) override
  {
    callback("inDbWireCreate", wire);
  }
  void inDbWireDestroy(odb::dbWire* wire) override
  {
    callback("inDbWireDestroy", wire);
  }
  void inDbBlockageCreate(odb::dbBlockage* blockage) override
  {
    callback("inDbBlockageCreate", blockage);
  }
  void inDbObstructionCreate(odb::dbObstruction* obstr) override
  {
    callback("inDbObstructionCreate", obstr);
  }
  void inDbObstructionDestroy(odb::dbObstruction* obstr) override
  {
    callback("inDbObstructionDestroy", obstr);
  }
  void inDbBlockStreamOutAfter(odb::dbBlock* block) override
  {
    callback("inDbBlockStreamOutAfter", block);
  }
  void inDbFillCreate(odb::dbFill* fill) override
  {
    callback("inDbFillCreate", fill);
  }

 signals:
  void dbUpdated(const QString& update_type,
                 const std::vector<odb::dbObject*>& update_objects);
 public slots:
  void reset()
  {
    isDirty_ = false;
    // call reset after gui refresh
  }

 private:
  void callback(QString update_type,
                odb::dbObject* obj1 = nullptr,
                odb::dbObject* obj2 = nullptr)
  {
    if (isDirty_ == false) {
      // send signal if dirty was false
      std::vector<odb::dbObject*> objects{obj1, obj2};
      emit dbUpdated(update_type, objects);
      isDirty_ = true;
    }
  }
  bool isDirty_;
};

}  // namespace gui
