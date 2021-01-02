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

#include "selectHighlightWindow.h"

#include <QDebug>
#include <QHBoxLayout>
#include <QPushButton>
#include <QString>
#include <QToolButton>
#include <QVBoxLayout>
#include <string>

#include "gui/gui.h"

namespace gui {

SelHltModel::SelHltModel(const SelectionSet& objs) : objs_(objs)
{
}

SelHltModel::~SelHltModel()
{
}

void SelHltModel::populateModel()
{
  beginResetModel();
  int objIdx = 0;
  tableData_.clear();
  tableData_.reserve(objs_.size());
  for (auto& obj : objs_) {
    tableData_.push_back(&obj);
  }
  endResetModel();
}

int SelHltModel::rowCount(const QModelIndex& parent) const
{
  Q_UNUSED(parent);
  return objs_.size();
}

int SelHltModel::columnCount(const QModelIndex& parent) const
{
  Q_UNUSED(parent);
  return 3;
}

QVariant SelHltModel::data(const QModelIndex& index, int role) const
{
  if (!index.isValid() || role != Qt::DisplayRole) {
    return QVariant();
  }
  int rowIndex = index.row();
  if (rowIndex > tableData_.size())
    return QVariant();
  std::string objName = tableData_[rowIndex]->getName();
  std::string objType("");
  if (objName.rfind("Net: ", 0) == 0) {
    objName = objName.substr(5);
    objType = "Net";
  } else if (objName.rfind("Inst: ", 0) == 0) {
    objName = objName.substr(6);
    objType = "Instance";
  }
  if (index.column() == 0) {
    return QString::fromStdString(objName);
  } else if (index.column() == 1) {
    return QString::fromStdString(objType);
  } else if (index.column() == 2) {
    return QString("(0, 0)");
  }
  return QVariant();
}

QVariant SelHltModel::headerData(int section,
                                 Qt::Orientation orientation,
                                 int role) const
{
  if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
    if (section == 0) {
      return QString("Object");
    } else if (section == 1) {
      return QString("Type");
    } else if (section == 2) {
      return QString("Loc");
    }
  }
  return QVariant();
}

SelectHighlightWindow::SelectHighlightWindow(const SelectionSet& selSet,
                                             const SelectionSet& hltSet,
                                             QWidget* parent)
    : QDockWidget(parent),
      ui(new Ui::SelectHighlightWidget),
      selModel_(selSet),
      hltModel_(hltSet)
{
  ui->setupUi(this);
  ui->selTableView->setModel(&selModel_);
  ui->hltTableView->setModel(&hltModel_);
}

SelectHighlightWindow::~SelectHighlightWindow()
{
}

void SelectHighlightWindow::updateSelectionModel()
{
  selModel_.populateModel();
}

void SelectHighlightWindow::updateHighlightModel()
{
  hltModel_.populateModel();
}

}  // namespace gui