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

#include <memory>
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
class dbObject;
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
namespace sta {
class dbSta;
class DcalcAnalysisPt;
class Instance;
class Net;
class Path;
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
  TimingPathsModel(sta::dbSta* sta, QObject* parent = nullptr);

  int rowCount(const QModelIndex& parent = QModelIndex()) const Q_DECL_OVERRIDE;
  int columnCount(const QModelIndex& parent
                  = QModelIndex()) const Q_DECL_OVERRIDE;

  QVariant data(const QModelIndex& index,
                int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
  QVariant headerData(int section,
                      Qt::Orientation orientation,
                      int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;

  TimingPath* getPathAt(const QModelIndex& index) const;

  void resetModel();
  void populateModel(bool get_max, int path_count);

 public slots:
  void sort(int col_index, Qt::SortOrder sort_order) override;

 private:
  bool populatePaths(bool get_max, int path_count, bool clockExpanded = false);

  sta::dbSta* sta_;
  std::vector<std::unique_ptr<TimingPath>> timing_paths_;

  enum Column {
    Clock,
    Required,
    Arrival,
    Slack,
    Start,
    End
  };
};

class TimingPathNode
{
 public:
  TimingPathNode(odb::dbObject* pin,
                 bool is_clock,
                 bool is_rising,
                 bool is_sink,
                 float arrival,
                 float required,
                 float delay,
                 float slack,
                 float slew,
                 float load)
      : pin_(pin),
        is_clock_(is_clock),
        is_rising_(is_rising),
        is_sink_(is_sink),
        arrival_(arrival),
        required_(required),
        delay_(delay),
        slack_(slack),
        slew_(slew),
        load_(load),
        sink_node_(nullptr),
        instance_node_(nullptr)
  {
  }

  std::string getNodeName(bool include_master = false) const;
  std::string getNetName() const;

  odb::dbNet* getNet() const;
  odb::dbInst* getInstance() const;
  bool hasInstance() const { return getInstance() != nullptr; }

  bool isPinITerm() const { return pin_->getObjectType() == odb::dbObjectType::dbITermObj; }
  bool isPinBTerm() const { return pin_->getObjectType() == odb::dbObjectType::dbBTermObj; }

  odb::dbObject* getPin() const { return pin_; }
  odb::dbITerm* getPinAsITerm() const { return static_cast<odb::dbITerm*>(pin_); }
  odb::dbBTerm* getPinAsBTerm() const { return static_cast<odb::dbBTerm*>(pin_); }

  bool isClock() const { return is_clock_; }
  bool isRisingEdge() const { return is_rising_; }
  bool isSink() const { return is_sink_; }
  bool isSource() const { return !is_sink_; }

  float getArrival() const { return arrival_; }
  float getRequired() const { return required_; }
  float getDelay() const { return delay_; }
  float getSlack() const { return slack_; }
  float getSlew() const { return slew_; }
  float getLoad() const { return load_; }

  void setSinkNode(const TimingPathNode* node) { sink_node_ = node; }
  const TimingPathNode* getSinkNode() const { return sink_node_; }
  void setInstanceNode(const TimingPathNode* node) { instance_node_ = node; }
  const TimingPathNode* getInstanceNode() const { return instance_node_; }

 private:
  odb::dbObject* pin_;
  bool is_clock_;
  bool is_rising_;
  bool is_sink_;
  float arrival_;
  float required_;
  float delay_;
  float slack_;
  float slew_;
  float load_;

  const TimingPathNode* sink_node_;
  const TimingPathNode* instance_node_;
};

class TimingPath
{
 public:
  TimingPath()
      : path_nodes_(),
        capture_nodes_(),
        start_clk_(),
        end_clk_(),
        slack_(0),
        path_delay_(0),
        arr_time_(0),
        req_time_(0),
        clk_path_end_index_(0),
        clk_capture_end_index_(0)
  {
  }

  using TimingNodeList = std::vector<std::unique_ptr<TimingPathNode>>;

  void appendNode(TimingPathNode* node) { path_nodes_.push_back(std::unique_ptr<TimingPathNode>(node)); }

  void setStartClock(const char* name) { start_clk_ = name; }
  const std::string& getStartClock() const { return start_clk_; }
  void setEndClock(const char* name) { end_clk_ = name; }
  const std::string& getEndClock() const { return end_clk_; }

  float getPathArrivalTime() const { return arr_time_; }
  void setPathArrivalTime(float arr) { arr_time_ = arr; }
  float getPathRequiredTime() const { return req_time_; }
  void setPathRequiredTime(float req) { req_time_ = req; }
  float getSlack() const { return slack_; }
  void setSlack(float slack) { slack_ = slack; }
  float getPathDelay() const { return path_delay_; }
  void setPathDelay(float del) { path_delay_ = del; }

  void computeClkEndIndex();

  int getClkPathEndIndex() const { return clk_path_end_index_; }
  int getClkCaptureEndIndex() const { return clk_capture_end_index_; }

  TimingNodeList* getPathNodes() { return &path_nodes_; }
  TimingNodeList* getCaptureNodes() { return &capture_nodes_; }

  std::string getStartStageName() const;
  std::string getEndStageName() const;

  void populatePath(sta::Path* path, sta::dbSta* sta, sta::DcalcAnalysisPt* dcalc_ap, bool clock_expanded, bool first_path);
  void populateCapturePath(sta::Path* path, sta::dbSta* sta, sta::DcalcAnalysisPt* dcalc_ap, float offset, bool clock_expanded, bool first_path);

 private:
  TimingNodeList path_nodes_;
  TimingNodeList capture_nodes_;
  std::string start_clk_;
  std::string end_clk_;
  float slack_;
  float path_delay_;
  float arr_time_;
  float req_time_;
  int clk_path_end_index_;
  int clk_capture_end_index_;

  void populateNodeList(sta::Path* path, sta::dbSta* sta, sta::DcalcAnalysisPt* dcalc_ap, float offset, bool clock_expanded, bool first_path, TimingNodeList& list);

  void computeClkEndIndex(TimingNodeList& nodes, int& index);
};

class TimingPathDetailModel : public QAbstractTableModel
{
 public:
  TimingPathDetailModel(bool is_hold, sta::dbSta* sta, QObject* parent = nullptr);

  int rowCount(const QModelIndex& parent = QModelIndex()) const Q_DECL_OVERRIDE;
  int columnCount(const QModelIndex& parent
                  = QModelIndex()) const Q_DECL_OVERRIDE;

  QVariant data(const QModelIndex& index,
                int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
  QVariant headerData(int section,
                      Qt::Orientation orientation,
                      int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;

  TimingPath* getPath() const { return path_; }
  TimingPath::TimingNodeList* getNodes() const { return nodes_; }
  bool hasNodes() const { return nodes_ != nullptr && !nodes_->empty(); }
  int getClockEndIndex() const { return is_capture_ ? path_->getClkCaptureEndIndex() : path_->getClkPathEndIndex(); }

  const TimingPathNode* getNodeAt(const QModelIndex& index) const;
  bool shouldHide(const QModelIndex& index, bool expand_clock) const;

  bool isClockSummaryRow(const QModelIndex& index) const { return index.row() == clock_summary_row_; }

  void populateModel(TimingPath* path, TimingPath::TimingNodeList* nodes);

 private:
  sta::dbSta* sta_;
  bool is_capture_;

  TimingPath* path_;
  TimingPath::TimingNodeList* nodes_;

  // Unicode symbols
  static constexpr char up_down_arrows_[] = "\u21C5";
  static constexpr char up_arrow_[] = "\u2191";
  static constexpr char down_arrow_[] = "\u2193";
  enum Column {
    Pin,
    RiseFall,
    Time,
    Delay,
    Slew,
    Load
  };
  static constexpr int clock_summary_row_ = 1;
};

class TimingPathRenderer : public gui::Renderer
{
 public:
  TimingPathRenderer();
  void highlight(TimingPath* path);

  void highlightNode(const TimingPathNode* node);
  void clearHighlightNodes() { highlight_stage_.clear(); }

  virtual void drawObjects(gui::Painter& /* painter */) override;

  TimingPath* getPathToRender() { return path_; }

 private:
  void highlightStage(gui::Painter& painter,
                      const gui::Descriptor* net_descriptor,
                      const gui::Descriptor* inst_descriptor);

  void drawNodesList(TimingPath::TimingNodeList* nodes,
                     gui::Painter& painter,
                     const gui::Descriptor* net_descriptor,
                     const gui::Descriptor* inst_descriptor,
                     const gui::Descriptor* bterm_descriptor,
                     const Painter::Color& clock_color);

  // Expanded path is owned by PathRenderer.
  TimingPath* path_;

  struct HighlightStage {
    odb::dbNet* net;
    odb::dbInst* inst;
    odb::dbObject* sink;
  };
  std::vector<std::unique_ptr<HighlightStage>> highlight_stage_;

  static const gui::Painter::Color inst_highlight_color_;
  static const gui::Painter::Color path_inst_color_;
  static const gui::Painter::Color term_color_;
  static const gui::Painter::Color signal_color_;
  static const gui::Painter::Color clock_color_;
  static const gui::Painter::Color capture_clock_color_;
};

class GuiDBChangeListener : public QObject, public odb::dbBlockCallBackObj
{
  Q_OBJECT
 public:
  GuiDBChangeListener(QObject* parent = nullptr) : QObject(parent), is_modified_(false) {}

  void inDbInstCreate(odb::dbInst* /* inst */) override
  {
    callback();
  }
  void inDbInstDestroy(odb::dbInst* /* inst */) override
  {
    callback();
  }
  void inDbInstSwapMasterBefore(odb::dbInst* /* inst */,
                                odb::dbMaster* /* master */) override
  {
    callback();
  }
  void inDbInstSwapMasterAfter(odb::dbInst* /* inst */) override
  {
    callback();
  }
  void inDbNetCreate(odb::dbNet* /* net */) override
  {
    callback();
  }
  void inDbNetDestroy(odb::dbNet* /* net */) override
  {
    callback();
  }
  void inDbITermPostConnect(odb::dbITerm* /* iterm */) override
  {
    callback();
  }
  void inDbITermPreDisconnect(odb::dbITerm* /* iterm */) override
  {
    callback();
  }
  void inDbITermDestroy(odb::dbITerm* /* iterm */) override
  {
    callback();
  }
  void inDbBTermPostConnect(odb::dbBTerm* /* bterm */) override
  {
    callback();
  }
  void inDbBTermPreDisconnect(odb::dbBTerm* /* bterm */) override
  {
    callback();
  }
  void inDbBTermDestroy(odb::dbBTerm* /* bterm */) override
  {
    callback();
  }
  void inDbWireCreate(odb::dbWire* /* wire */) override
  {
    callback();
  }
  void inDbWireDestroy(odb::dbWire* /* wire */) override
  {
    callback();
  }
  void inDbFillCreate(odb::dbFill* /* fill */) override
  {
    callback();
  }

 signals:
  void dbUpdated();

 public slots:
  void reset()
  {
    is_modified_ = false;
    // call reset after gui refresh
  }

 private:
  void callback()
  {
    if (!is_modified_) {
      emit dbUpdated();
      is_modified_ = true;
    }
  }

  bool is_modified_;
};

}  // namespace gui
