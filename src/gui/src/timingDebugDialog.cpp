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

#include "staGui.h"

namespace gui {
TimingDebugDialog::TimingDebugDialog(QWidget* parent)
    : QDialog(parent),
      timing_paths_model_(nullptr),
      path_details_model_(new TimingPathDetailModel())
{
  setupUi(this);
  timingPathTableView->horizontalHeader()->setSectionResizeMode(
      QHeaderView::Stretch);
  pathDetailsTableView->horizontalHeader()->setSectionResizeMode(
      QHeaderView::Stretch);
  timingPathTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
  pathDetailsTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
  // pathDetailsTableView->setModel(path_details_model_);

  connect(timingPathTableView,
          SIGNAL(clicked(const QModelIndex&)),
          this,
          SLOT(showPathDetails(const QModelIndex&)));

  connect(timingPathTableView,
          SIGNAL(doubleClicked(const QModelIndex&)),
          this,
          SLOT(showPathInLayout(const QModelIndex&)));
}

void TimingDebugDialog::accept()
{
  QDialog::accept();
}

void TimingDebugDialog::reject()
{
  QDialog::reject();
}

void TimingDebugDialog::populateTimingPaths(odb::dbBlock* block)
{
  if (timing_paths_model_ != nullptr)
    return;
  timing_paths_model_ = new TimingPathsModel();
  timingPathTableView->setModel(timing_paths_model_);
}

void TimingDebugDialog::showPathDetails(const QModelIndex& index)
{
  if (!index.isValid() || timing_paths_model_ == nullptr)
    return;
  auto path = timing_paths_model_->getPathAt(index.row());
  path_details_model_->populateModel(path);
  pathDetailsTableView->setModel(path_details_model_);
}

void TimingDebugDialog::showPathInLayout(const QModelIndex& index)
{
  // TBD
  qDebug() << "Came to show Path In Layout of path " << index.row() + 1;
}

}  // namespace gui