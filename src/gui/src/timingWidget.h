// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#pragma once

#include <QCheckBox>
#include <QDockWidget>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMenu>
#include <QModelIndex>
#include <QPushButton>
#include <QSettings>
#include <QSpinBox>
#include <QSplitter>
#include <QString>
#include <QTableView>
#include <QVariant>
#include <QWidget>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "gui/gui.h"
#include "odb/db.h"
#include "sta/Clock.hh"
#include "sta/SdcClass.hh"
#include "staGuiInterface.h"

namespace sta {
class Pin;
class dbSta;
class Clock;
}  // namespace sta

namespace gui {

class TimingPath;
class TimingPathRenderer;
class TimingConeRenderer;
class TimingControlsDialog;
class TimingPathDetailModel;
class TimingPathsModel;
class GuiDBChangeListener;

class TimingWidget : public QDockWidget
{
  Q_OBJECT
 public:
  enum CommandType
  {
    kExact,
    kNoBuffering,
    kFromStartToEnd
  };

  TimingWidget(QWidget* parent = nullptr);
  ~TimingWidget() override;

  void init(sta::dbSta* sta);
  void setLogger(utl::Logger* logger);

  TimingPathRenderer* getTimingRenderer() { return path_renderer_.get(); }
  TimingConeRenderer* getConeRenderer() { return cone_renderer_.get(); }

  void readSettings(QSettings* settings);
  void writeSettings(QSettings* settings);

  TimingControlsDialog* getSettings() { return settings_; }

  void updatePaths();
  void reportSlackHistogramPaths(const std::set<const sta::Pin*>& report_pins,
                                 const std::string& path_group_name);
  void showWorstTimingPath(bool setup);
  void clearSelection();

 signals:
  void highlightTimingPath(TimingPath* timing_path);
  void inspect(const Selected& selection);
  void setCommand(const QString& command);

 public slots:
  void showPathDetails(const QModelIndex& index);
  void clearPathDetails();
  void highlightPathStage(TimingPathDetailModel* model,
                          const QModelIndex& index);

  void toggleRenderer(bool enable);

  void populatePaths();
  void modelWasReset();

  void selectedRowChanged(const QItemSelection& prev_index,
                          const QItemSelection& curr_index);
  void selectedDetailRowChanged(const QItemSelection& prev_index,
                                const QItemSelection& curr_index);
  void selectedCaptureRowChanged(const QItemSelection& prev_index,
                                 const QItemSelection& curr_index);

  void detailRowDoubleClicked(const QModelIndex& index);

  void handleDbChange();
  void setBlock(odb::dbBlock* block);

  void updateClockRows();

  void showSettings();

  void writePathReportCommand(const QModelIndex& selected_index,
                              const CommandType& type);
  void writePathDef(const QModelIndex& selected_index, const CommandType& type);
  void focusNets(const QModelIndex& selected_index,
                 const TimingPath::PathSection& path_section);
  void showCommandsMenu(const QPoint& pos);

 private slots:
  void hideColumn(int index, bool checked);

 protected:
  void keyPressEvent(QKeyEvent* key_event) override;
  void showEvent(QShowEvent* event) override;
  void hideEvent(QHideEvent* event) override;

 private:
  void copy();
  void setColumnDisplayMenu();
  void addCommandsMenuActions();
  void populateAndSortModels(const std::set<const sta::Pin*>& from,
                             const std::vector<std::set<const sta::Pin*>>& thru,
                             const std::set<const sta::Pin*>& to,
                             const std::string& path_group_name,
                             const sta::ClockSet* clks = nullptr);
  void setInitialColumnsVisibility(const QVariant& columns_visibility);
  QVariantList getColumnsVisibility() const;

  // Auxiliary for generating report_checks commands for Script Widget.
  QString generateFromStartToEndString(TimingPath* path);
  QString generateClosestMatchString(CommandType type, TimingPath* path);

  utl::Logger* logger_{nullptr};
  QMenu* commands_menu_;

  QModelIndex timing_paths_table_index_;

  QTableView* setup_timing_table_view_;
  QTableView* hold_timing_table_view_;
  QTableView* path_details_table_view_;
  QTableView* capture_details_table_view_;

  QPushButton* update_button_;
  QPushButton* columns_control_container_;
  QMenu* columns_control_;
  QPushButton* settings_button_;

  TimingControlsDialog* settings_;

  TimingPathsModel* setup_timing_paths_model_;
  TimingPathsModel* hold_timing_paths_model_;
  TimingPathDetailModel* path_details_model_;
  TimingPathDetailModel* capture_details_model_;
  std::unique_ptr<TimingPathRenderer> path_renderer_;
  std::unique_ptr<TimingConeRenderer> cone_renderer_;
  GuiDBChangeListener* dbchange_listener_;

  QSplitter* delay_detail_splitter_;
  QTabWidget* delay_widget_;
  QTabWidget* detail_widget_;

  QTableView* focus_view_;

  QVector<bool> initial_columns_visibility_;  // from settings
};
}  // namespace gui
