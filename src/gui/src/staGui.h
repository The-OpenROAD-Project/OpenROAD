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

#include <map>
#include <memory>
#include <set>
#include <vector>
#include <QAbstractTableModel>
#include <QCheckBox>
#include <QColor>
#include <QComboBox>
#include <QDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QSpinBox>

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
  void populateModel(bool get_max,
                     int path_count,
                     const std::set<sta::Pin*>& from,
                     const std::vector<std::set<sta::Pin*>>& thru,
                     const std::set<sta::Pin*>& to,
                     bool unconstrainted);

 public slots:
  void sort(int col_index, Qt::SortOrder sort_order) override;

 private:
  bool populatePaths(bool get_max,
                     int path_count,
                     const std::set<sta::Pin*>& from,
                     const std::vector<std::set<sta::Pin*>>& thru,
                     const std::set<sta::Pin*>& to,
                     bool unconstrainted);

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
                 bool is_clock = false,
                 bool is_rising = false,
                 bool is_sink = false,
                 bool has_values = false,
                 float arrival = 0.0,
                 float delay = 0.0,
                 float slew = 0.0,
                 float load = 0.0)
      : pin_(pin),
        is_clock_(is_clock),
        is_rising_(is_rising),
        is_sink_(is_sink),
        has_values_(has_values),
        arrival_(arrival),
        delay_(delay),
        slew_(slew),
        load_(load),
        path_slack_(0.0),
        paired_nodes_({}),
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
  odb::dbITerm* getPinAsITerm() const;
  odb::dbBTerm* getPinAsBTerm() const;
  const odb::Rect getPinBBox() const;
  const odb::Rect getPinLargestBox() const;

  bool isClock() const { return is_clock_; }
  bool isRisingEdge() const { return is_rising_; }
  bool isSink() const { return is_sink_; }
  bool isSource() const { return !is_sink_; }

  float getArrival() const { return arrival_; }
  float getDelay() const { return delay_; }
  float getSlew() const { return slew_; }
  float getLoad() const { return load_; }

  void setPathSlack(float value) { path_slack_ = value; }
  float getPathSlack() const { return path_slack_; }

  bool hasValues() const { return has_values_; }

  void addPairedNode(const TimingPathNode* node) { paired_nodes_.insert(node); }
  void clearPairedNodes() { paired_nodes_.clear(); }
  const std::set<const TimingPathNode*>& getPairedNodes() const { return paired_nodes_; }
  void setInstanceNode(const TimingPathNode* node) { instance_node_ = node; }
  const TimingPathNode* getInstanceNode() const { return instance_node_; }

  void copyData(TimingPathNode* other) const;

 private:
  odb::dbObject* pin_;
  bool is_clock_;
  bool is_rising_;
  bool is_sink_;
  bool has_values_;
  float arrival_;
  float delay_;
  float slew_;
  float load_;
  float path_slack_;

  std::set<const TimingPathNode*> paired_nodes_;
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

  static void buildPaths(sta::dbSta* sta,
                         bool get_max,
                         bool include_unconstrained,
                         int path_count,
                         const std::set<sta::Pin*>& from,
                         const std::vector<std::set<sta::Pin*>>& thrus,
                         const std::set<sta::Pin*>& to,
                         bool include_capture,
                         std::vector<std::unique_ptr<TimingPath>>& paths);

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
  void setSlackOnPathNodes();

  int getClkPathEndIndex() const { return clk_path_end_index_; }
  int getClkCaptureEndIndex() const { return clk_capture_end_index_; }

  TimingNodeList* getPathNodes() { return &path_nodes_; }
  TimingNodeList* getCaptureNodes() { return &capture_nodes_; }

  std::string getStartStageName() const;
  std::string getEndStageName() const;

  void populatePath(sta::Path* path, sta::dbSta* sta, sta::DcalcAnalysisPt* dcalc_ap, bool clock_expanded);
  void populateCapturePath(sta::Path* path, sta::dbSta* sta, sta::DcalcAnalysisPt* dcalc_ap, float offset, bool clock_expanded);

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

  void populateNodeList(sta::Path* path, sta::dbSta* sta, sta::DcalcAnalysisPt* dcalc_ap, float offset, bool clock_expanded, TimingNodeList& list);

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
  Qt::ItemFlags flags(const QModelIndex &index) const Q_DECL_OVERRIDE;

  TimingPath* getPath() const { return path_; }
  TimingPath::TimingNodeList* getNodes() const { return nodes_; }
  bool hasNodes() const { return nodes_ != nullptr && !nodes_->empty(); }
  int getClockEndIndex() const { return is_capture_ ? path_->getClkCaptureEndIndex() : path_->getClkPathEndIndex(); }

  const TimingPathNode* getNodeAt(const QModelIndex& index) const;
  void setExpandClock(bool state) { expand_clock_ = state; }
  bool shouldHide(const QModelIndex& index) const;

  bool isClockSummaryRow(const QModelIndex& index) const { return index.row() == clock_summary_row_; }

  void populateModel(TimingPath* path, TimingPath::TimingNodeList* nodes);

 private:
  sta::dbSta* sta_;
  bool is_capture_;
  bool expand_clock_;

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

class TimingConeRenderer : public gui::Renderer
{
 public:
  TimingConeRenderer();
  void setSTA(sta::dbSta* sta) { sta_ = sta; }
  void setITerm(odb::dbITerm* term, bool fanin, bool fanout);
  void setBTerm(odb::dbBTerm* term, bool fanin, bool fanout);
  void setPin(sta::Pin* pin, bool fanin, bool fanout);

  virtual void drawObjects(gui::Painter& painter) override;

 private:
  using PinList = std::vector<std::unique_ptr<TimingPathNode>>;
  using DepthMap = std::map<int, PinList>;

  sta::dbSta* sta_;
  sta::Pin* term_;
  bool fanin_;
  bool fanout_;
  DepthMap map_;
  int min_map_index_;
  int max_map_index_;
  float min_timing_;
  float max_timing_;
  SpectrumGenerator color_generator_;

  using DepthMapSet = std::map<int, std::set<odb::dbObject*>>;
  void getFaninCone(sta::Pin* pin, DepthMapSet& depth_map);
  void getFanoutCone(sta::Pin* pin, DepthMapSet& depth_map);
  void getCone(sta::Pin* pin, sta::PinSet* pin_set, DepthMapSet& depth_map, bool is_fanin);
  void buildConnectivity();
  void annotateTiming(sta::Pin* pin);
  bool isSupplyPin(sta::Pin* pin) const;
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

class PinSetWidget : public QWidget
{
 Q_OBJECT
 public:
  PinSetWidget(bool add_remove_button, QWidget* parent = nullptr);

  void setSTA(sta::dbSta* sta) { sta_ = sta; }

  void updatePins();

  void setPins(const std::set<sta::Pin*>& pins);

  const std::set<sta::Pin*> getPins() const;

  bool isAddMode() const { return add_mode_; }
  bool isRemoveMode() const { return !isAddMode(); }
  void setAddMode();
  void setRemoveMode();

 signals:
  void addRemoveTriggered(PinSetWidget*);
  void inspect(const Selected& selected);

 public slots:
  void clearPins() { setPins({}); }

 protected:
  void keyPressEvent(QKeyEvent* event) override;

 private slots:
  void findPin();
  void showMenu(const QPoint& point);

 private:
  sta::dbSta* sta_;
  std::vector<sta::Pin*> pins_;

  QListWidget* box_;
  QLineEdit* find_pin_;
  QPushButton* clear_;
  QPushButton* add_remove_;

  bool add_mode_;

  void addPin(sta::Pin* pin);
  void removePin(sta::Pin* pin);
  void removeSelectedPins();
};

class TimingControlsDialog : public QDialog
{
 Q_OBJECT
 public:
  TimingControlsDialog(QWidget* parent = nullptr);

  void setSTA(sta::dbSta* sta);

  void setPathCount(int path_count) { path_count_spin_box_->setValue(path_count); }
  int getPathCount() const { return path_count_spin_box_->value(); }

  void setUnconstrained(bool uncontrained);
  bool getUnconstrained() const;

  void setExpandClock(bool expand);
  bool getExpandClock() const;

  void setFromPin(const std::set<sta::Pin*>& pins) { from_->setPins(pins); }
  void setThruPin(const std::vector<std::set<sta::Pin*>>& pins);
  void setToPin(const std::set<sta::Pin*>& pins) { to_->setPins(pins); }

  const std::set<sta::Pin*> getFromPins() const { return from_->getPins(); }
  const std::vector<std::set<sta::Pin*>> getThruPins() const;
  const std::set<sta::Pin*> getToPins() const { return to_->getPins(); }

  sta::Pin* convertTerm(Gui::odbTerm term) const;

 signals:
  void inspect(const Selected& selected);
  void expandClock(bool expand);

 public slots:
  void populate();

 private slots:
  void addRemoveThru(PinSetWidget* row);

 private:
  sta::dbSta* sta_;

  QFormLayout* layout_;

  QSpinBox* path_count_spin_box_;
  QComboBox* corner_box_;

  QCheckBox* uncontrained_;
  QCheckBox* expand_clk_;

  PinSetWidget* from_;
  std::vector<PinSetWidget*> thru_;
  PinSetWidget* to_;

  static constexpr int thru_start_row_ = 3;

  void setPinSelections();

  void addThruRow(const std::set<sta::Pin*>& pins);
  void setupPinRow(const QString& label, PinSetWidget* row, int row_index = -1);
};

}  // namespace gui
