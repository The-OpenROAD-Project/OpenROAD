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
#include <QLabel>
#include <QMenu>
#include <QPushButton>
#include <QStandardItemModel>
#include <QTimer>
#include <QTreeView>
#include <QVBoxLayout>
#include <memory>
#include <vector>

#include "gui/gui.h"

namespace gui {
class SelectedItemModel;

class EditorItemDelegate : public QItemDelegate
{
  Q_OBJECT

 public:
  // positions in ->data() where data is located
  static const int editor_ = Qt::UserRole;
  static const int editor_name_ = Qt::UserRole + 1;
  static const int editor_type_ = Qt::UserRole + 2;
  static const int editor_select_ = Qt::UserRole + 3;
  static const int selected_ = Qt::UserRole + 4;

  enum EditType
  {
    NUMBER,
    STRING,
    BOOL,
    LIST
  };

  EditorItemDelegate(SelectedItemModel* model, QObject* parent = nullptr);

  QWidget* createEditor(QWidget* parent,
                        const QStyleOptionViewItem& option,
                        const QModelIndex& index) const override;

  void setEditorData(QWidget* editor, const QModelIndex& index) const override;
  void setModelData(QWidget* editor,
                    QAbstractItemModel* model,
                    const QModelIndex& index) const override;

  static EditType getEditorType(const std::any& value);

 private:
  SelectedItemModel* model_;
  const QColor background_;
};

class SelectedItemModel : public QStandardItemModel
{
  Q_OBJECT

 public:
  SelectedItemModel(const Selected& object,
                    const QColor& selectable,
                    const QColor& editable,
                    QObject* parent = nullptr);

  QVariant data(const QModelIndex& index,
                int role = Qt::DisplayRole) const override;

  const QColor& getSelectableColor() { return selectable_item_; }
  const QColor& getEditableColor() { return editable_item_; }

 signals:
  void selectedItemChanged(const QModelIndex& index);

 public slots:
  void updateObject();

 private:
  void makePropertyItem(const Descriptor::Property& property,
                        QStandardItem*& name_item,
                        QStandardItem*& value_item);
  QStandardItem* makeItem(const QString& name);
  QStandardItem* makeItem(const std::any& item, bool short_name = false);

  template <typename Iterator>
  QStandardItem* makeList(QStandardItem* name_item,
                          const Iterator& begin,
                          const Iterator& end);
  template <typename Iterator>
  QStandardItem* makePropertyList(QStandardItem* name_item,
                                  const Iterator& begin,
                                  const Iterator& end);

  void makeItemEditor(const std::string& name,
                      QStandardItem* item,
                      const Selected& selected,
                      const EditorItemDelegate::EditType type,
                      const Descriptor::Editor& editor);

  const QColor selectable_item_;
  const QColor editable_item_;
  const Selected& object_;
};

class ActionLayout : public QLayout
{
  // modified from:
  // https://doc.qt.io/qt-5/qtwidgets-layouts-flowlayout-example.html
  Q_OBJECT

 public:
  ActionLayout(QWidget* parent = nullptr);
  ~ActionLayout();

  void clear();

  void addItem(QLayoutItem* item) override;
  QSize sizeHint() const override;
  QSize minimumSize() const override;
  void setGeometry(const QRect& rect) override;
  QLayoutItem* itemAt(int index) const override;
  QLayoutItem* takeAt(int index) override;
  int count() const override;

  bool hasHeightForWidth() const override;
  int heightForWidth(int width) const override;

 private:
  int rowHeight() const;
  int rowSpacing() const;
  int buttonSpacing() const;
  int requiredRows(int width) const;

  int itemWidth(QLayoutItem* item) const;

  using ItemList = std::vector<QLayoutItem*>;
  void organizeItemsToRows(int width, std::vector<ItemList>& rows) const;
  int rowWidth(ItemList& row) const;

  QList<QLayoutItem*> actions_;
};

class ObjectTree : public QTreeView
{
  Q_OBJECT

 public:
  ObjectTree(QWidget* parent = nullptr);

 signals:
  void mouseExited();

 protected:
  void leaveEvent(QEvent* event) override;
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
  Inspector(const SelectionSet& selected,
            const HighlightSet& highlighted,
            QWidget* parent = nullptr);

  const Selected& getSelection() { return selection_; }

 signals:
  void addSelected(const Selected& selected);
  void removeSelected(const Selected& selected);
  void selected(const Selected& selected, bool showConnectivity = false);
  void selectedItemChanged(const Selected& selected);
  void selection(const Selected& selected);
  void focus(const Selected& selected);

  void addHighlight(const SelectionSet& selection);
  void removeHighlight(const QList<const Selected*>& selected);

  void setCommand(const QString& command);

 public slots:
  void inspect(const Selected& object);
  void clicked(const QModelIndex& index);
  void doubleClicked(const QModelIndex& index);
  void update(const Selected& object = Selected());
  void highlightChanged();
  void focusNetsChanged();

  int selectNext();
  int selectPrevious();

  void updateSelectedFields(const QModelIndex& index);

  void reload();

 private slots:
  void focusIndex(const QModelIndex& index);
  void defocus();

  void indexClicked();
  void indexDoubleClicked(const QModelIndex& index);

  void showCommandsMenu(const QPoint& pos);

 private:
  void handleAction(QWidget* action);
  void loadActions();

  void adjustHeaders();

  int getSelectedIteratorPosition();

  bool isHighlighted(const Selected& selected);

  void makeAction(const Descriptor::Action& action);

  void navigateBack();

  void setCommandsMenu();
  void writePathReportCommand();

  // The columns in the tree view
  enum Column
  {
    Name,
    Value
  };

  ObjectTree* view_;
  SelectedItemModel* model_;
  QVBoxLayout* layout_;
  ActionLayout* action_layout_;
  const SelectionSet& selected_;
  SelectionSet::iterator selected_itr_;
  Selected selection_;
  QFrame* button_frame_;
  QPushButton* button_next_;
  QPushButton* button_prev_;
  QLabel* selected_itr_label_;
  QTimer mouse_timer_;
  QModelIndex clicked_index_;
  QMenu* commands_menu_;

  std::string report_text_;

  const HighlightSet& highlighted_;

  std::vector<Selected> navigation_history_;

  std::map<QWidget*, Descriptor::ActionCallback> actions_;
  Descriptor::ActionCallback deselect_action_;

  // used to finetune the double click interval
  static constexpr double mouse_double_click_scale_ = 0.75;
};

}  // namespace gui
