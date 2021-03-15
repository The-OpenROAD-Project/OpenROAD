///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2021, OpenROAD
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

#include <QDialog>
#include <QModelIndex>
#include <QString>
#include <vector>

#include "opendb/db.h"
#include "staGui.h"
#include "timingReportDialog.h"
#include "ui_timingDebug.h"

namespace ord {
class OpenRoad;
}

namespace gui {
class TimingDebugDialog : public QDialog, public Ui::TimingDialog
{
  Q_OBJECT
 public:
  TimingDebugDialog(QWidget* parent = nullptr);
  ~TimingDebugDialog();

  TimingPathRenderer* getTimingRenderer() { return path_renderer_; }

 signals:
  void highlightTimingPath(TimingPath* timing_path);

 public slots:
  void accept();
  void reject();
  bool populateTimingPaths(odb::dbBlock* block);

  void showPathDetails(const QModelIndex& index);
  void highlightPathStage(const QModelIndex& index);
  void timingPathsViewCustomSort(int col_index);
  void findNodeInPathDetails();

  void showPathIndex(int pathId);
  void showTimingReportDialog();
  void selectedRowChanged(const QItemSelection& prev_index,
                          const QItemSelection& curr_index);
  void selectedDetailRowChanged(const QItemSelection& prev_index,
                                const QItemSelection& curr_index);

  void handleDbChange(QString change_type, std::vector<odb::dbObject*> objects);

 private:
  TimingPathsModel* timing_paths_model_;
  TimingPathDetailModel* path_details_model_;
  TimingPathRenderer* path_renderer_;
  GuiDBChangeListener* dbchange_listener_;
  TimingReportDialog* timing_report_dlg_;
};
}  // namespace gui