//////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2019, OpenROAD
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

#include "timingDebugDialog.h"

#include <QDebug>
#include <QMessageBox>
#include <QSortFilterProxyModel>

#include "db_sta/dbSta.hh"
#include "staGui.h"

namespace gui {
TimingDebugDialog::TimingDebugDialog(QWidget* parent)
    : QDialog(parent),
      timing_paths_model_(nullptr),
      path_details_model_(new TimingPathDetailModel()),
      path_renderer_(new TimingPathRenderer()),
      dbchange_listener_(new GuiDBChangeListener)
{
  setupUi(this);
  timingPathTableView->horizontalHeader()->setSectionResizeMode(
      QHeaderView::Stretch);
  pathDetailsTableView->horizontalHeader()->setSectionResizeMode(
      QHeaderView::Stretch);
  timingPathTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
  pathDetailsTableView->setSelectionBehavior(QAbstractItemView::SelectRows);

  connect(timingPathTableView,
          SIGNAL(clicked(const QModelIndex&)),
          this,
          SLOT(showPathDetails(const QModelIndex&)));
  connect(timingPathTableView->horizontalHeader(),
          SIGNAL(sectionClicked(int)),
          this,
          SLOT(timingPathsViewCustomSort(int)));
  connect(pathDetailsTableView,
          SIGNAL(clicked(const QModelIndex&)),
          this,
          SLOT(highlightPathStage(const QModelIndex&)));

  connect(findObjectEdit,
          SIGNAL(returnPressed()),
          this,
          SLOT(findNodeInPathDetails()));

  connect(nextPathBtn, SIGNAL(clicked()), this, SLOT(showNextPath()));
  connect(prevPathBtn, SIGNAL(clicked()), this, SLOT(showPrevPath()));
  connect(
      jumpToPathEdit, SIGNAL(returnPressed()), this, SLOT(showRequestedPath()));

  connect(dbchange_listener_,
          SIGNAL(dbUpdated(QString, std::vector<odb::dbObject*>)),
          this,
          SLOT(handleDbChange(QString, std::vector<odb::dbObject*>)));
  jumpToPathEdit->setFocusPolicy(Qt::StrongFocus);
}

TimingDebugDialog::~TimingDebugDialog()
{
  // TBD
}

void TimingDebugDialog::accept()
{
  QDialog::accept();
}

void TimingDebugDialog::reject()
{
  QDialog::reject();
  path_renderer_->highlight(nullptr);
  Gui::get()->unregisterRenderer(path_renderer_);
}

void TimingDebugDialog::populateTimingPaths(odb::dbBlock* block)
{
  if (timing_paths_model_ != nullptr)
    return;
  timing_paths_model_ = new TimingPathsModel();
  timingPathTableView->setModel(timing_paths_model_);
  jumpToPathEdit->setValidator(
      new QIntValidator(0, timing_paths_model_->rowCount()));
}

void TimingDebugDialog::showPathDetails(const QModelIndex& index)
{
  if (!index.isValid() || timing_paths_model_ == nullptr)
    return;
  auto path = timing_paths_model_->getPathAt(index.row());
  path_details_model_->populateModel(path);
  pathDetailsTableView->setModel(path_details_model_);
  path_renderer_->highlight(path);
  emit highlightTimingPath(path);
}

void TimingDebugDialog::highlightPathStage(const QModelIndex& index)
{
  if (!index.isValid() || timing_paths_model_ == nullptr)
    return;
  path_renderer_->highlightNode(index.row());
  emit highlightTimingPath(path_renderer_->getPathToRender());
}

void TimingDebugDialog::timingPathsViewCustomSort(int col_index)
{
  if (col_index != 1)
    return;
  timing_paths_model_->sort(col_index);
}

void TimingDebugDialog::findNodeInPathDetails()
{
  auto search_node = findObjectEdit->text();
  QSortFilterProxyModel proxy;
  proxy.setSourceModel(path_details_model_);
  proxy.setFilterKeyColumn(0);
  proxy.setFilterFixedString(search_node);
  QModelIndex match_index = proxy.mapToSource(proxy.index(0, 0));
  if (match_index.isValid()) {
    pathDetailsTableView->selectRow(match_index.row());
  } else {
    QMessageBox::information(
        this, "Node Search : ", search_node + " Match not found!");
  }
}

void TimingDebugDialog::showNextPath()
{
  QItemSelectionModel* selectionModel = timingPathTableView->selectionModel();
  int row = -1;
  if (selectionModel->hasSelection())
    row = selectionModel->selection().first().indexes().first().row();
  int rowcount = timingPathTableView->model()->rowCount();
  row = (row + 1) % rowcount;
  QModelIndex new_index = timingPathTableView->model()->index(row, 0);
  timingPathTableView->selectRow(new_index.row());
  showPathDetails(new_index);
}

void TimingDebugDialog::showPrevPath()
{
  QItemSelectionModel* selectionModel = timingPathTableView->selectionModel();
  int row = -1;
  if (selectionModel->hasSelection())
    row = selectionModel->selection().first().indexes().first().row();
  int rowcount = timingPathTableView->model()->rowCount();
  row = (row - 1) % rowcount;
  QModelIndex new_index = timingPathTableView->model()->index(row, 0);
  timingPathTableView->selectRow(new_index.row());
  showPathDetails(new_index);
}

void TimingDebugDialog::showPathIndex(int path_idx)
{
  QModelIndex new_index = timingPathTableView->model()->index(path_idx, 0);
  timingPathTableView->selectRow(new_index.row());
  showPathDetails(new_index);
}

void TimingDebugDialog::showRequestedPath()
{
  QString path_str = jumpToPathEdit->text();
  int path_idx = path_str.toInt();
  showPathIndex(path_idx - 1);
}

void TimingDebugDialog::handleDbChange(QString change_type,
                                       std::vector<odb::dbObject*> objects)
{
  path_details_model_->populateModel(nullptr);
  timing_paths_model_->resetModel();
}

}  // namespace gui