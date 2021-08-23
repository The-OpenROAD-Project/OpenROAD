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

#include <QDockWidget>
#include <QKeyEvent>
#include <QModelIndex>
#include <QTableView>
#include <QLineEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QSettings>
#include <vector>

#include "odb/db.h"
#include "staGui.h"

namespace ord {
class OpenRoad;
}

namespace gui {
class TimingWidget : public QDockWidget
{
  Q_OBJECT
 public:
  TimingWidget(QWidget* parent = nullptr);

  TimingPathRenderer* getTimingRenderer() { return path_renderer_; }

  void readSettings(QSettings* settings);
  void writeSettings(QSettings* settings);

 signals:
  void highlightTimingPath(TimingPath* timing_path);

 public slots:
  void keyPressEvent(QKeyEvent* key_event);

  void showPathDetails(const QModelIndex& index);
  void clearPathDetails();
  void highlightPathStage(const QModelIndex& index);
  void findNodeInPathDetails();

  void toggleRenderer(bool enable);

  void populatePaths();
  void modelWasReset();

  void showPathIndex(int pathId);
  void selectedRowChanged(const QItemSelection& prev_index,
                          const QItemSelection& curr_index);
  void selectedDetailRowChanged(const QItemSelection& prev_index,
                                const QItemSelection& curr_index);

  void handleDbChange(QString change_type, std::vector<odb::dbObject*> objects);

 protected:
  void showEvent(QShowEvent* event) override;
  void hideEvent(QHideEvent* event) override;

 private:
  void copy();

  QTableView* setup_timing_table_view_;
  QTableView* hold_timing_table_view_;
  QTableView* path_details_table_view_;

  QLineEdit* find_object_edit_;
  QSpinBox* path_index_spin_box_;
  QSpinBox* path_count_spin_box_;
  QPushButton* update_button_;

  TimingPathsModel* setup_timing_paths_model_;
  TimingPathsModel* hold_timing_paths_model_;
  TimingPathDetailModel* path_details_model_;
  TimingPathRenderer* path_renderer_;
  GuiDBChangeListener* dbchange_listener_;
  QTabWidget* delay_widget_;

  QTableView* focus_view_;
};
}  // namespace gui
