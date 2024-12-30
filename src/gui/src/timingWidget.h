///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2021, The Regents of the University of California
// All rights reserved.
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
#include <QTableView>
#include <memory>

#include "gui/gui.h"
#include "odb/db.h"

namespace sta {
class Pin;
class dbSta;
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
    CLOSEST_MATCH,
    FROM_START_TO_END
  };

  TimingWidget(QWidget* parent = nullptr);
  ~TimingWidget();

  void init(sta::dbSta* sta);

  TimingPathRenderer* getTimingRenderer() { return path_renderer_.get(); }
  TimingConeRenderer* getConeRenderer() { return cone_renderer_.get(); }

  void readSettings(QSettings* settings);
  void writeSettings(QSettings* settings);

  TimingControlsDialog* getSettings() { return settings_; }

  void updatePaths();
  void reportSlackHistogramPaths(const std::set<const sta::Pin*>& report_pins,
                                 const std::string& path_group_name);

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
                             const std::string& path_group_name);
  void setInitialColumnsVisibility(const QVariant& columns_visibility);
  QVariantList getColumnsVisibility() const;

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
