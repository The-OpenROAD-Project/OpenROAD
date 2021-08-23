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

#include "inspector.h"

#include <QApplication>
#include <QComboBox>
#include <QDebug>
#include <QHeaderView>
#include <QPushButton>
#include <QLineEdit>

#include "gui/gui.h"

Q_DECLARE_METATYPE(gui::Selected);
Q_DECLARE_METATYPE(gui::Descriptor::Editor);
Q_DECLARE_METATYPE(gui::EditorItemDelegate::EditType);
Q_DECLARE_METATYPE(std::any);
Q_DECLARE_METATYPE(std::string);

namespace gui {

using namespace odb;

static QString convertAnyToQString(const std::any& item, EditorItemDelegate::EditType* type = nullptr)
{
  QString value;
  EditorItemDelegate::EditType value_type = EditorItemDelegate::STRING; // default to string

  if (auto selected = std::any_cast<Selected>(&item)) {
    value = QString::fromStdString(selected->getName());
  } else if (auto v = std::any_cast<const char*>(&item)) {
    value = QString(*v);
  } else if (auto v = std::any_cast<const std::string>(&item)) {
    value = QString::fromStdString(*v);
  } else if (auto v = std::any_cast<int>(&item)) {
    value_type = EditorItemDelegate::NUMBER;
    value = QString::number(*v);
  } else if (auto v = std::any_cast<unsigned int>(&item)) {
    value_type = EditorItemDelegate::NUMBER;
    value = QString::number(*v);
  } else if (auto v = std::any_cast<double>(&item)) {
    value_type = EditorItemDelegate::NUMBER;
    value = QString::number(*v);
  } else if (auto v = std::any_cast<float>(&item)) {
    value_type = EditorItemDelegate::NUMBER;
    value = QString::number(*v);
  } else if (auto v = std::any_cast<bool>(&item)) {
    value_type = EditorItemDelegate::BOOL;
    value = *v ? "True" : "False";
  } else {
    value = "<unknown>";
  }

  if (type != nullptr) {
    *type = value_type;
  }
  return value;
}

QVariant SelectedItemModel::data(const QModelIndex& index, int role) const
{
  if (index.column() == 1) {
    if (role == Qt::DisplayRole) {
      auto selected_data = itemFromIndex(index)->data(EditorItemDelegate::selected_).value<Selected>();
      if (selected_data) {
        // use selected name when available
        return QString::fromStdString(selected_data.getName());
      }
    } else if (role == Qt::BackgroundRole) {
      bool has_editor = itemFromIndex(index)->data(EditorItemDelegate::editor_).isValid();

      if (has_editor) {
        return QBrush(editable_item_);
      }
    } else if (role == Qt::ForegroundRole) {
      bool has_selected = itemFromIndex(index)->data(EditorItemDelegate::selected_).isValid();

      if (has_selected){
        return QBrush(selectable_item_);
      }
    }
  }
  return QStandardItemModel::data(index, role);
}

EditorItemDelegate::EditorItemDelegate(SelectedItemModel* model,
                                       QObject* parent) : QItemDelegate(parent),
                                       model_(model),
                                       background_(model->getEditableColor())
{
}

QWidget* EditorItemDelegate::createEditor(QWidget* parent,
                                          const QStyleOptionViewItem& /* option */,
                                          const QModelIndex& index) const
{
  auto type = index.model()->data(index, editor_type_).value<EditorItemDelegate::EditType>();
  QWidget* editor;
  if (type == LIST) {
    editor = new QComboBox(parent);
  } else {
    editor = new QLineEdit(parent);
  }
  editor->setStyleSheet("background: " + background_.name());
  return editor;
}

void EditorItemDelegate::setEditorData(QWidget* editor,
                                       const QModelIndex& index) const
{
  auto type = index.model()->data(index, editor_type_).value<EditorItemDelegate::EditType>();
  auto [callback, values] = index.model()->data(index, editor_).value<Descriptor::Editor>();
  QString value = index.model()->data(index, Qt::EditRole).toString();

  if (type != LIST) {
    QLineEdit* line_edit = static_cast<QLineEdit*>(editor);
    line_edit->setText(value);
  } else {
    QComboBox* combo_box = static_cast<QComboBox*>(editor);
    // disconnect to stop callback to setModelData
    combo_box->disconnect();
    combo_box->clear();
    for (const auto& [name, option_value] : values) {
      combo_box->addItem(QString::fromStdString(name), QVariant::fromValue(option_value));
    }
    combo_box->setCurrentText(value);
    // listen for changes and update immediately
    connect(combo_box,
            &QComboBox::currentTextChanged,
            [this, editor, index](const QString& text) {
              setModelData(editor, model_, index);
            });
  }
}

void EditorItemDelegate::setModelData(QWidget* editor,
                                      QAbstractItemModel* model,
                                      const QModelIndex& index) const
{
  auto type = model->data(index, editor_type_).value<EditorItemDelegate::EditType>();
  auto [callback, values] = model->data(index, editor_).value<Descriptor::Editor>();

  const QString old_value = index.model()->data(index, Qt::EditRole).toString();

  bool accepted = false;
  QString value;
  std::any callback_value;
  bool value_valid = false;
  if (type != LIST) {
    QLineEdit* line_edit = static_cast<QLineEdit*>(editor);
    value = line_edit->text();
    if (type == NUMBER) {
      callback_value = value.toDouble(&value_valid);
    } else if (type == STRING) {
      callback_value = value.toStdString();
      value_valid = true;
    }
  } else {
    QComboBox* combo_box = static_cast<QComboBox*>(editor);
    value = combo_box->currentText();
    callback_value = combo_box->currentData().value<std::any>();
    value_valid = true;
  }
  if (value_valid && value != old_value) {
    accepted = callback(callback_value);
  }

  QString edit_save = old_value; // default to set to old value
  if (accepted) {
    // retrieve property again
    auto selected = model->data(index, editor_select_).value<Selected>();
    auto item_name = model->data(index, editor_name_).value<std::string>();
    if (item_name == "Name") { // name and type are inserted in inspector, so handle differently
      edit_save = QString::fromStdString(selected.getName());
    } else if (item_name == "Type") {
      edit_save = QString::fromStdString(selected.getTypeName());
    } else {
      auto new_property = selected.getProperty(item_name);
      if (model->data(index, selected_).isValid()) {
        auto new_selected = std::any_cast<Selected>(new_property);
        model->setData(index, QVariant::fromValue(new_selected), selected_);
      }
      edit_save = convertAnyToQString(new_property);
    }
  }
  model->setData(index, edit_save, Qt::EditRole);

  // disable editing as we are done editing
  model_->itemFromIndex(index)->setEditable(false);
}

////////

Inspector::Inspector(const SelectionSet& selected, QWidget* parent)
    : QDockWidget("Inspector", parent),
      view_(new QTreeView()),
      model_(new SelectedItemModel(Qt::blue, QColor(0xc6, 0xff, 0xc4) /* pale green */)),
      layout_(new QVBoxLayout),
      selected_(selected),
      selection_(Selected()),
      mouse_timer_(nullptr)
{
  setObjectName("inspector");  // for settings
  model_->setHorizontalHeaderLabels({"Name", "Value"});
  view_->setModel(model_);
  view_->setItemDelegate(new EditorItemDelegate(model_, this));

  QHeaderView* header = view_->header();
  header->setSectionResizeMode(Name, QHeaderView::Stretch);
  header->setSectionResizeMode(Value, QHeaderView::ResizeToContents);
  // QTreeView defaults stretchLastSection to true, overriding setSectionResizeMode
  header->setStretchLastSection(false);

  QWidget* container = new QWidget;
  layout_->addWidget(view_, /* stretch */ 1);

  container->setLayout(layout_);

  setWidget(container);

  // connect so announcements can be made about changes
  connect(model_,
          &SelectedItemModel::itemChanged,
          [this]() { emit selectedItemChanged(selection_); });

  connect(view_,
          SIGNAL(clicked(const QModelIndex&)),
          this,
          SLOT(clicked(const QModelIndex&)));
}

void Inspector::inspect(const Selected& object)
{
  // disconnect so announcements can be will not be made about changes
  // changes right now are based on adding item to model
  blockSignals(true);

  model_->removeRows(0, model_->rowCount());
  // remove action buttons and ensure delete
  for (auto& [button, action] : actions_) {
    delete button;
  }
  actions_.clear();

  selection_ = object;

  if (!object) {
    return;
  }

  auto editors = object.getEditors();

  Descriptor::Properties all_properties;
  all_properties.push_back({"Type", object.getTypeName()});
  all_properties.push_back({"Name", object.getName()});

  Descriptor::Properties properties = object.getProperties();
  std::copy(properties.begin(), properties.end(), std::back_inserter(all_properties));

  for (auto& [name, value] : all_properties) {
    auto name_item = makeItem(QString::fromStdString(name));
    auto editor_found = editors.find(name);
    EditorItemDelegate::EditType editor_type = EditorItemDelegate::STRING; // default to string

    QStandardItem* value_item = nullptr;

    // For a SelectionSet a row is created with the set items
    // as children rows
    if (auto sel_set = std::any_cast<SelectionSet>(&value)) {
      value_item = makeItem(name_item, sel_set->begin(), sel_set->end());
    } else if (auto v_list = std::any_cast<std::vector<std::any>>(&value)) {
      value_item = makeItem(name_item, v_list->begin(), v_list->end());
    } else if (auto v_set = std::any_cast<std::set<std::any>>(&value)) {
      value_item = makeItem(name_item, v_set->begin(), v_set->end());
    } else if (auto selected = std::any_cast<Selected>(&value)) {
      value_item = makeItem(*selected);
    } else {
      value_item = makeItem(convertAnyToQString(value, &editor_type));
    }

    model_->appendRow({name_item, value_item});

    // Auto open small lists
    if (model_->hasChildren(name_item->index()) && name_item->rowCount() < 10) {
      view_->expand(name_item->index());
    }

    // make editor if found
    if (editor_found != editors.end()) {
      auto editor = (*editor_found).second;
      makeItemEditor(name, value_item, object, editor_type, editor);
    }
  }

  // add action buttons
  for (const auto [name, action] : object.getActions()) {
    QPushButton* button = new QPushButton(name.c_str(), this);
    connect(button, &QPushButton::released, [this, button]() {
      handleAction(button);
    });
    layout_->addWidget(button);
    actions_[button] = action;
  }

  blockSignals(false);
}

void Inspector::clicked(const QModelIndex& index)
{
  // QT sends both single and double clicks, so they need to be handled with a timer to
  // be able to tell the difference
  if (mouse_timer_ == nullptr) {
    mouse_timer_ = std::make_unique<QTimer>(this);
    mouse_timer_->setInterval(mouse_double_click_scale_ * QApplication::doubleClickInterval());
    mouse_timer_->setSingleShot(true);

    connect(mouse_timer_.get(), &QTimer::timeout, [this, index]() { emit indexClicked(index); });

    mouse_timer_->start();
  } else {
    mouse_timer_->stop();

    emit indexDoubleClicked(index);
  }

}

void Inspector::indexClicked(const QModelIndex& index)
{
  mouse_timer_ = nullptr;

  // handle single click event
  QStandardItem* item = model_->itemFromIndex(index);
  auto new_selected = item->data(EditorItemDelegate::selected_).value<Selected>();
  if (new_selected) {
    emit selected(new_selected, false);
  }
}

void Inspector::indexDoubleClicked(const QModelIndex& index)
{
  mouse_timer_ = nullptr;

  // handle single click event
  QStandardItem* item = model_->itemFromIndex(index);
  QVariant item_data = item->data(EditorItemDelegate::editor_);

  if (item_data.isValid()) {
    // set editable
    item->setEditable(true);
    // start editing
    view_->edit(index);
  }
}

void Inspector::update()
{
  if (selected_.empty()) {
    inspect(Selected());
  } else {
    inspect(*selected_.begin());
    raise();
  }
}

void Inspector::handleAction(QWidget* action)
{
  auto callback = actions_[action];
  auto new_selection = callback();
  emit selected(new_selection);
}

QStandardItem* Inspector::makeItem(const QString& name)
{
  auto item = new QStandardItem(name);
  item->setEditable(false);
  item->setSelectable(false);
  return item;
}

QStandardItem* Inspector::makeItem(const std::any& item)
{
  return makeItem(convertAnyToQString(item));
}

QStandardItem* Inspector::makeItem(const Selected& selected)
{
  auto item = makeItem(QString::fromStdString(selected.getName()));
  item->setData(QVariant::fromValue(selected), EditorItemDelegate::selected_);
  return item;
}

template<typename Iterator>
QStandardItem* Inspector::makeItem(QStandardItem* name_item, const Iterator& begin, const Iterator& end)
{
  int index = 0;
  for (Iterator use_itr = begin; use_itr != end; ++use_itr) {
    auto index_item = makeItem(QString::number(++index));
    auto selected_item = makeItem(*use_itr);
    name_item->appendRow({index_item, selected_item});
  }

  return makeItem(QString::number(index) + " items");
}

void Inspector::makeItemEditor(const std::string& name,
                               QStandardItem* item,
                               const Selected& selected,
                               const EditorItemDelegate::EditType type,
                               const Descriptor::Editor& editor)
{
  item->setData(QVariant::fromValue(selected), EditorItemDelegate::editor_select_);
  item->setData(QVariant::fromValue(name), EditorItemDelegate::editor_name_);

  Descriptor::Editor used_editor = editor;
  if (type == EditorItemDelegate::BOOL) {
    // for BOOL, replace options with true/false options
    used_editor.options = {{"True", true}, {"False", false}};
  }

  item->setData(QVariant::fromValue(used_editor), EditorItemDelegate::editor_);
  if (used_editor.options.empty()) {
    // options are empty so use selected type
    item->setData(QVariant::fromValue(type), EditorItemDelegate::editor_type_);
  } else {
    // options are not empty so use list type
    item->setData(QVariant::fromValue(EditorItemDelegate::LIST), EditorItemDelegate::editor_type_);
  }
}

}  // namespace gui
