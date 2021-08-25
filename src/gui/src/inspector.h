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
#include <QItemDelegate>
#include <QStandardItemModel>
#include <QTimer>
#include <QTreeView>
#include <QVBoxLayout>

#include <memory>
#include <vector>

#include "gui/gui.h"

namespace gui {

class SelectedItemModel : public QStandardItemModel
{
  Q_OBJECT

public:
  SelectedItemModel(
      const QColor& selectable,
      const QColor& editable,
      QObject* parent = nullptr)
  : QStandardItemModel(0, 2, parent),
    selectable_item_(selectable),
    editable_item_(editable) {}

  QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

  const QColor& getSelectableColor() { return selectable_item_; }
  const QColor& getEditableColor() { return editable_item_; }

private:
  const QColor selectable_item_;
  const QColor editable_item_;
};

class EditorItemDelegate : public QItemDelegate
{
  Q_OBJECT

public:
  // positions in ->data() where data is located
  static const int editor_        = Qt::UserRole;
  static const int editor_name_   = Qt::UserRole+1;
  static const int editor_type_   = Qt::UserRole+2;
  static const int editor_select_ = Qt::UserRole+3;
  static const int selected_      = Qt::UserRole+4;

  enum EditType {
    NUMBER,
    STRING,
    BOOL,
    LIST
  };

  EditorItemDelegate(SelectedItemModel* model, QObject* parent = nullptr);

  QWidget* createEditor(QWidget* parent,
                        const QStyleOptionViewItem& option,
                        const QModelIndex& index) const override;

  void setEditorData(QWidget* editor,
                     const QModelIndex& index) const override;
  void setModelData(QWidget* editor,
                    QAbstractItemModel* model,
                    const QModelIndex& index) const override;

private:
  SelectedItemModel* model_;
  const QColor background_;
};

// The inspector is to allow a single object to have it properties displayed.
// It is generic and builds on the Selected and Descriptor classes.
// The inspector knows how to handle SelectionSet and Selected objects
// and will create links for them in the tree widget.  Simple properties,
// like strings, are handled as well.
class Inspector : public QDockWidget
{
  Q_OBJECT

 public:
  Inspector(const SelectionSet& selected, QWidget* parent = nullptr);

 signals:
  void selected(const Selected& selected, bool showConnectivity = false);
  void selectedItemChanged(const Selected& selected);

 public slots:
  void inspect(const Selected& object);
  void clicked(const QModelIndex& index);
  void update();

  void indexClicked(const QModelIndex& index);
  void indexDoubleClicked(const QModelIndex& index);

 private:
  void handleAction(QWidget* action);
  QStandardItem* makeItem(const Selected& selected);
  QStandardItem* makeItem(const QString& name);
  QStandardItem* makeItem(const std::any& item);

  template<typename Iterator>
  QStandardItem* makeItem(QStandardItem* name_item, const Iterator& begin, const Iterator& end);

  void makeItemEditor(const std::string& name,
                      QStandardItem* item,
                      const Selected& selected,
                      const EditorItemDelegate::EditType type,
                      const Descriptor::Editor& editor);

  // The columns in the tree view
  enum Column
  {
    Name,
    Value
  };

  QTreeView* view_;
  SelectedItemModel* model_;
  QVBoxLayout* layout_;
  const SelectionSet& selected_;
  Selected selection_;
  std::unique_ptr<QTimer> mouse_timer_;

  std::map<QWidget*, Descriptor::ActionCallback> actions_;

  // used to finetune the double click interval
  static constexpr double mouse_double_click_scale_ = 0.75;
};

}  // namespace gui
