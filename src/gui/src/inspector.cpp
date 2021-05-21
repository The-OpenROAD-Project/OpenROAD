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

#include "inspector.h"

#include <QDebug>
#include <QHeaderView>
#include <QKeyEvent>
#include <QLineEdit>
#include <QPainter>
#include <QSettings>
#include <QVBoxLayout>
#include <vector>

#include "gui/gui.h"

Q_DECLARE_METATYPE(gui::Selected);

namespace gui {

using namespace odb;

static QStandardItem* makeItem(const QString& name,
                               const Selected& selected = Selected())
{
  auto item = new QStandardItem(name);
  item->setEditable(false);
  item->setSelectable(false);
  if (selected) {
    item->setData(QVariant::fromValue(selected));
    item->setForeground(Qt::blue);
  }
  return item;
}

static QStandardItem* makeItem(const std::string& name,
                               const Selected& selected = Selected())
{
  return makeItem(QString::fromStdString(name), selected);
}

static QStandardItem* makeItem(const char* name,
                               const Selected& selected = Selected())
{
  return makeItem(QString(name), selected);
}

Inspector::Inspector(const SelectionSet& selected, QWidget* parent)
    : QDockWidget("Inspector", parent),
      view_(new QTreeView()),
      model_(new QStandardItemModel(0, 2)),
      selected_(selected)
{
  setObjectName("inspector");  // for settings
  model_->setHorizontalHeaderLabels({"Name", "Value"});
  view_->setModel(model_);

  QHeaderView* header = view_->header();
  header->setSectionResizeMode(Name, QHeaderView::Stretch);
  header->setSectionResizeMode(Value, QHeaderView::ResizeToContents);

  setWidget(view_);
  connect(view_,
          SIGNAL(clicked(const QModelIndex&)),
          this,
          SLOT(clicked(const QModelIndex&)));
}

void Inspector::inspect(const Selected& object)
{
  model_->removeRows(0, model_->rowCount());

  if (!object) {
    return;
  }

  model_->appendRow({makeItem("Type"), makeItem(object.getTypeName())});
  model_->appendRow({makeItem("Name"), makeItem(object.getName())});

  for (auto& [name, value] : object.getProperties()) {
    auto name_item = makeItem(name);

    // For a SelectionSet a row is created with the set items
    // as children rows
    if (auto sel_set = std::any_cast<SelectionSet>(&value)) {
      auto value_item = makeItem(QString::number(sel_set->size()) + " items");
      model_->appendRow({name_item, value_item});
      int index = 1;
      for (const auto& selected : *sel_set) {
        auto index_item = makeItem(QString::number(index++));
        auto selected_item = makeItem(selected.getName(), selected);
        name_item->appendRow({index_item, selected_item});
      }
      // Auto open small lists
      if (sel_set->size() < 10) {
        view_->expand(name_item->index());
      }
    } else if (auto selected = std::any_cast<Selected>(&value)) {
      auto value_item = makeItem(selected->getName(), *selected);
      model_->appendRow({name_item, value_item});
    } else if (auto v = std::any_cast<const char*>(&value)) {
      model_->appendRow({name_item, makeItem(QString(*v))});
    } else if (auto v = std::any_cast<const std::string>(&value)) {
      model_->appendRow({name_item, makeItem(*v)});
    } else if (auto v = std::any_cast<int>(&value)) {
      model_->appendRow({name_item, makeItem(QString::number(*v))});
    } else if (auto v = std::any_cast<unsigned int>(&value)) {
      model_->appendRow({name_item, makeItem(QString::number(*v))});
    } else if (auto v = std::any_cast<double>(&value)) {
      model_->appendRow({name_item, makeItem(QString::number(*v))});
    } else if (auto v = std::any_cast<float>(&value)) {
      model_->appendRow({name_item, makeItem(QString::number(*v))});
    } else if (auto v = std::any_cast<bool>(&value)) {
      model_->appendRow({name_item, makeItem(*v ? "True" : "False")});
    } else {
      auto value_item = makeItem(QString("<unknown>"));
      model_->appendRow({name_item, value_item});
    }
  }
}

void Inspector::clicked(const QModelIndex& index)
{
  QStandardItem* item = model_->itemFromIndex(index);
  auto new_selected = item->data().value<Selected>();
  if (new_selected) {
    emit selected(new_selected, false);
  }
}

void Inspector::update()
{
  if (selected_.empty()) {
    inspect(Selected());
  } else {
    inspect(*selected_.begin());
  }
}

}  // namespace gui
