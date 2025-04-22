// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#pragma once

#include <QAbstractTableModel>
#include <QCheckBox>
#include <QColor>
#include <QComboBox>
#include <QDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QSpinBox>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "gui/gui.h"
#include "odb/db.h"
#include "odb/dbBlockCallBackObj.h"
#include "sta/PathExpanded.hh"
#include "sta/Sta.hh"
#include "staGuiInterface.h"

namespace sta {
class dbSta;
class Pin;
}  // namespace sta
namespace gui {
class TimingPathsModel;
class TimingPathDetailModel;
class GuiDBChangeListener;

class TimingPathsModel : public QAbstractTableModel
{
  Q_OBJECT

 private:
  enum Column
  {
    Clock,
    Required,
    Arrival,
    Slack,
    Skew,
    LogicDelay,
    LogicDepth,
    Fanout,
    Start,
    End
  };

 public:
  static const std::map<Column, const char*>& getColumnNames()
  {
    static const std::map<Column, const char*> column_names
        = {{Clock, "Capture Clock"},
           {Required, "Required"},
           {Arrival, "Arrival"},
           {Slack, "Slack"},
           {Skew, "Skew"},
           {LogicDelay, "Logic Delay"},
           {LogicDepth, "Logic Depth"},
           {Fanout, "Fanout"},
           {Start, "Start"},
           {End, "End"}};
    return column_names;
  }

  TimingPathsModel(bool is_setup,
                   STAGuiInterface* sta,
                   QObject* parent = nullptr);

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
  void populateModel(const std::set<const sta::Pin*>& from,
                     const std::vector<std::set<const sta::Pin*>>& thru,
                     const std::set<const sta::Pin*>& to,
                     const std::string& path_group_name);

 public slots:
  void sort(int col_index, Qt::SortOrder sort_order) override;

 private:
  bool populatePaths(const std::set<const sta::Pin*>& from,
                     const std::vector<std::set<const sta::Pin*>>& thru,
                     const std::set<const sta::Pin*>& to,
                     const std::string& path_group_name);

  STAGuiInterface* sta_;
  bool is_setup_;
  std::vector<std::unique_ptr<TimingPath>> timing_paths_;
};

class TimingPathDetailModel : public QAbstractTableModel
{
 private:
  enum Column
  {
    Pin,
    Fanout,
    RiseFall,
    Time,
    Delay,
    Slew,
    Load
  };

 public:
  static const std::map<Column, const char*>& getColumnNames()
  {
    static const std::map<Column, const char*> column_names
        = {{Pin, "Pin"},
           {Fanout, "Fanout"},
           {RiseFall, "RiseFall"},
           {Time, "Time"},
           {Delay, "Delay"},
           {Slew, "Slew"},
           {Load, "Load"}};
    return column_names;
  }

  TimingPathDetailModel(bool is_capture,
                        sta::dbSta* sta,
                        QObject* parent = nullptr);

  int rowCount(const QModelIndex& parent = QModelIndex()) const Q_DECL_OVERRIDE;
  int columnCount(const QModelIndex& parent
                  = QModelIndex()) const Q_DECL_OVERRIDE;

  QVariant data(const QModelIndex& index,
                int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
  QVariant headerData(int section,
                      Qt::Orientation orientation,
                      int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
  Qt::ItemFlags flags(const QModelIndex& index) const Q_DECL_OVERRIDE;

  TimingPath* getPath() const { return path_; }
  TimingNodeList* getNodes() const { return nodes_; }
  bool hasNodes() const { return nodes_ != nullptr && !nodes_->empty(); }
  int getClockEndIndex() const
  {
    return is_capture_ ? path_->getClkCaptureEndIndex()
                       : path_->getClkPathEndIndex();
  }

  const TimingPathNode* getNodeAt(const QModelIndex& index) const;
  void setExpandClock(bool state) { expand_clock_ = state; }
  bool shouldHide(const QModelIndex& index) const;

  bool isClockSummaryRow(const QModelIndex& index) const
  {
    return index.row() == clock_summary_row_;
  }

  void populateModel(TimingPath* path, TimingNodeList* nodes);

 private:
  sta::dbSta* sta_;
  bool is_capture_;
  bool expand_clock_;

  TimingPath* path_;
  TimingNodeList* nodes_;

  // Unicode symbols
  static constexpr char up_down_arrows_[] = "⇅";
  static constexpr char up_arrow_[] = "↑";
  static constexpr char down_arrow_[] = "↓";
  static constexpr int clock_summary_row_ = 1;
};

class TimingPathRenderer : public gui::Renderer
{
 public:
  TimingPathRenderer();
  void highlight(TimingPath* path);

  void highlightNode(const TimingPathNode* node);
  void clearHighlightNodes();

  void drawObjects(gui::Painter& /* painter */) override;
  const char* getDisplayControlGroupName() override { return "Timing Path"; }

  TimingPath* getPathToRender() { return path_; }

 private:
  void highlightStage(gui::Painter& painter,
                      const gui::Descriptor* net_descriptor,
                      const gui::Descriptor* inst_descriptor);

  void drawNodesList(TimingNodeList* nodes,
                     gui::Painter& painter,
                     const gui::Descriptor* net_descriptor,
                     const gui::Descriptor* inst_descriptor,
                     const gui::Descriptor* bterm_descriptor,
                     const Painter::Color& clock_color,
                     bool draw_clock,
                     bool draw_signal);

  // Expanded path is owned by PathRenderer.
  TimingPath* path_;

  struct HighlightStage
  {
    odb::dbNet* net;
    odb::dbInst* inst;
    odb::dbObject* sink;
  };
  std::vector<std::unique_ptr<HighlightStage>> highlight_stage_;
  std::mutex rendering_;

  static const gui::Painter::Color inst_highlight_color_;
  static const gui::Painter::Color path_inst_color_;
  static const gui::Painter::Color term_color_;
  static const gui::Painter::Color signal_color_;
  static const gui::Painter::Color clock_color_;
  static const gui::Painter::Color capture_clock_color_;

  static constexpr const char* data_path_label_ = "Data path";
  static constexpr const char* launch_clock_label_ = "Launch clock";
  static constexpr const char* capture_clock_label_ = "Capture clock";
};

class TimingConeRenderer : public gui::Renderer
{
 public:
  TimingConeRenderer();
  void setSTA(sta::dbSta* sta) { sta_ = sta; }
  void setITerm(odb::dbITerm* term, bool fanin, bool fanout);
  void setBTerm(odb::dbBTerm* term, bool fanin, bool fanout);
  void setPin(const sta::Pin* pin, bool fanin, bool fanout);

  void drawObjects(gui::Painter& painter) override;

 private:
  sta::dbSta* sta_;
  const sta::Pin* term_;
  bool fanin_;
  bool fanout_;
  ConeDepthMap map_;
  float min_timing_;
  float max_timing_;
  SpectrumGenerator color_generator_;

  bool isSupplyPin(const sta::Pin* pin) const;
};

class GuiDBChangeListener : public QObject, public odb::dbBlockCallBackObj
{
  Q_OBJECT
 public:
  GuiDBChangeListener(QObject* parent = nullptr)
      : QObject(parent), is_modified_(false)
  {
  }

  void inDbInstCreate(odb::dbInst* /* inst */) override { callback(); }
  void inDbInstDestroy(odb::dbInst* /* inst */) override { callback(); }
  void inDbInstSwapMasterBefore(odb::dbInst* /* inst */,
                                odb::dbMaster* /* master */) override
  {
    callback();
  }
  void inDbInstSwapMasterAfter(odb::dbInst* /* inst */) override { callback(); }
  void inDbNetCreate(odb::dbNet* /* net */) override { callback(); }
  void inDbNetDestroy(odb::dbNet* /* net */) override { callback(); }
  void inDbITermPostConnect(odb::dbITerm* /* iterm */) override { callback(); }
  void inDbITermPreDisconnect(odb::dbITerm* /* iterm */) override
  {
    callback();
  }
  void inDbITermDestroy(odb::dbITerm* /* iterm */) override { callback(); }
  void inDbBTermPostConnect(odb::dbBTerm* /* bterm */) override { callback(); }
  void inDbBTermPreDisconnect(odb::dbBTerm* /* bterm */) override
  {
    callback();
  }
  void inDbBTermDestroy(odb::dbBTerm* /* bterm */) override { callback(); }
  void inDbWireCreate(odb::dbWire* /* wire */) override { callback(); }
  void inDbWireDestroy(odb::dbWire* /* wire */) override { callback(); }
  void inDbFillCreate(odb::dbFill* /* fill */) override { callback(); }

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

  void setPins(const std::set<const sta::Pin*>& pins);

  const std::set<const sta::Pin*> getPins() const;

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
  std::vector<const sta::Pin*> pins_;

  QListWidget* box_;
  QLineEdit* find_pin_;
  QPushButton* clear_;
  QPushButton* add_remove_;

  bool add_mode_;

  void addPin(const sta::Pin* pin);
  void removePin(const sta::Pin* pin);
  void removeSelectedPins();
};

class TimingControlsDialog : public QDialog
{
  Q_OBJECT
 public:
  TimingControlsDialog(QWidget* parent = nullptr);

  void setSTA(sta::dbSta* sta);
  STAGuiInterface* getSTA() const { return sta_.get(); }

  void setPathCount(int path_count);
  int getPathCount() const { return sta_->getMaxPathCount(); }

  void setUnconstrained(bool uncontrained);
  bool getUnconstrained() const { return sta_->isIncludeUnconstrainedPaths(); }

  void setOnePathPerEndpoint(bool value);
  bool getOnePathPerEndpoint() const { return sta_->isOnePathPerEndpoint(); }

  void setExpandClock(bool expand);
  bool getExpandClock() const;

  void setFromPin(const std::set<const sta::Pin*>& pins)
  {
    from_->setPins(pins);
  }
  void setThruPin(const std::vector<std::set<const sta::Pin*>>& pins);
  void setToPin(const std::set<const sta::Pin*>& pins) { to_->setPins(pins); }

  const std::set<const sta::Pin*> getFromPins() const
  {
    return from_->getPins();
  }
  const std::vector<std::set<const sta::Pin*>> getThruPins() const;
  const std::set<const sta::Pin*> getToPins() const { return to_->getPins(); }

  const sta::Pin* convertTerm(Gui::odbTerm term) const;

  sta::Corner* getCorner() const { return sta_->getCorner(); }
  void setCorner(sta::Corner* corner) { sta_->setCorner(corner); }

 signals:
  void inspect(const Selected& selected);
  void expandClock(bool expand);

 public slots:
  void populate();

 private slots:
  void addRemoveThru(PinSetWidget* row);

 private:
  std::unique_ptr<STAGuiInterface> sta_;

  QFormLayout* layout_;

  QSpinBox* path_count_spin_box_;
  QComboBox* corner_box_;

  QCheckBox* unconstrained_;
  QCheckBox* one_path_per_endpoint_;
  QCheckBox* expand_clk_;

  PinSetWidget* from_;
  std::vector<PinSetWidget*> thru_;
  PinSetWidget* to_;

  static constexpr int thru_start_row_ = 3;

  void setPinSelections();

  void addThruRow(const std::set<const sta::Pin*>& pins);
  void setupPinRow(const QString& label, PinSetWidget* row, int row_index = -1);
};

}  // namespace gui
