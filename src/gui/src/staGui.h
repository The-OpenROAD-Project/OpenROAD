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

#include "gui/gui.h"
#include "opendb/db.h"
#include "sta/PathExpanded.hh"
#include "sta/PathRef.hh"
#include "sta/Sta.hh"
#include "opendb/dbBlockCallBackObj.h"
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
}
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

  void findInstances(std::string pattern, std::vector<odb::dbInst*>& insts);
  void findNets(std::string pattern, std::vector<odb::dbNet*>& nets);
  void findPins(std::string pattern, std::vector<odb::dbObject*>& pins);

 private:
  void populateModel();
  bool populatePaths(bool get_max = true, int path_count = 100);

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

  std::string getNodeName() const;
  std::string getNetName() const;

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
  TimingPath() : path_exp_(nullptr) {}

  void appendNode(const TimingPathNode& node) { path_nodes_.push_back(node); }
  int levelsCount() const { return path_nodes_.size(); }
  void setStartClock(const char* name) { startClk_ = name; }
  std::string getStartClock() const { return startClk_; }
  void setEndClock(const char* name) { endClk_ = name; }
  std::string getEndClock() const { return endClk_; }

  // Time will be returned in in nano seconds
  float getPathArrivalTime() const { return arrTime_; }
  void setArrTime(float arr) { arrTime_ = arr; }
  float getPathRequiredTime() const { return reqTime_; }
  void setReqTime(float req) { reqTime_ = req; }
  float getSlack() const { return slack_; }
  void setSlack(float slack) { slack_ = slack; }
  float getPathDelay() const { return pathDelay_; }
  void setPathDelay(float del) { pathDelay_ = del; }

  void setPathExp(sta::PathExpanded* path_exp) { path_exp_ = path_exp; }
  sta::PathExpanded* getPathExp() { return path_exp_; }

  TimingPathNode getNodeAt(int index) const { return path_nodes_[index]; }

  std::string getStartStageName() const;
  std::string getEndStageName() const;

  void printPath(const std::string& file_name) const;

 private:
  std::vector<TimingPathNode> path_nodes_;
  sta::PathExpanded* path_exp_;
  std::string startClk_;
  std::string endClk_;
  float slack_;
  float pathDelay_;
  float arrTime_;
  float reqTime_;
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
      = {"Node", "Transition", "Required", "Arrival", "Slack", "Slew", "Load"};
};

class TimingPathRenderer : public gui::Renderer
{
 public:
  TimingPathRenderer();
  ~TimingPathRenderer();
  void highlight(TimingPath* path);
  void highlightInstNode(TimingPathNode node);
  void drawObjectsNative(gui::Painter& /* painter */);
  virtual void drawObjects(gui::Painter& /* painter */) override;

 private:
  void highlightInst(odb::dbInst* inst,
                     gui::Painter& painter,
                     const gui::Painter::Color& color);
  void highlightTerm(odb::dbBTerm* term, gui::Painter& painter);
  void highlightNet(odb::dbNet* net,
                    odb::dbObject* source_node,
                    odb::dbObject* sink_node,
                    gui::Painter& painter);

  // Expanded path is owned by PathRenderer.
  TimingPath* path_;
  std::vector<odb::dbInst*> highlight_inst_nodes_;

  static gui::Painter::Color inst_highlight_color_;
  static gui::Painter::Color path_inst_color_;
  static gui::Painter::Color term_color_;
  static gui::Painter::Color signal_color_;
  static gui::Painter::Color clock_color_;
};

class guiCallbacks : public odb::dbBlockCallBackObj
{
public:
  guiCallbacks(): isDirty(false) { }
  void inDbInstCreate(odb::dbInst *inst) { cbk(); }
  void inDbInstDestroy(odb::dbInst *inst) { cbk(); }
  void inDbInstSwapMasterBefore(odb::dbInst *inst,
                                odb::dbMaster *master) { cbk(); }
  void inDbInstSwapMasterAfter(odb::dbInst *inst) { cbk(); }
  void inDbNetCreate(odb::dbNet* n) { cbk(); }
  void inDbNetDestroy(odb::dbNet *net) { cbk(); }
  void inDbITermPostConnect(odb::dbITerm *iterm) { cbk(); }
  void inDbITermPreDisconnect(odb::dbITerm *iterm) { cbk(); }
  void inDbITermDestroy(odb::dbITerm *iterm) { cbk(); }
  void inDbBTermPostConnect(odb::dbBTerm *bterm) { cbk(); }
  void inDbBTermPreDisconnect(odb::dbBTerm *bterm) { cbk(); }
  void inDbBTermDestroy(odb::dbBTerm *bterm) { cbk(); }
  void inDbWireCreate(odb::dbWire* w) { cbk(); }
  void inDbWireDestroy(odb::dbWire* w) { cbk(); }
  void inDbBlockageCreate(odb::dbBlockage* b) { cbk(); }
  void inDbObstructionCreate(odb::dbObstruction* o) { cbk(); }
  void inDbObstructionDestroy(odb::dbObstruction* o) { cbk(); }
  void inDbBlockStreamOutAfter(odb::dbBlock* b) { cbk(); }
  void inDbFillCreate(odb::dbFill* f) { cbk(); }
  void reset() {
    isDirty = false;
    // call reset after gui refresh
  }
private:
  void cbk() {
    if (isDirty == false) {
      // send signal if dirty was false
      isDirty = true;
    }

  }
  bool isDirty;
};

}  // namespace gui
