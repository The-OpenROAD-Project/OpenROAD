// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#pragma once

#include <QColor>
#include <QDockWidget>
#include <QLabel>
#include <QMenu>
#include <QPushButton>
#include <QStandardItemModel>
#include <QString>
#include <QStyledItemDelegate>
#include <QTimer>
#include <QTreeView>
#include <QVBoxLayout>
#include <QVariant>
#include <QWidget>
#include <any>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "gui/gui.h"

namespace gui {
class SelectedItemModel;

class EditorItemDelegate : public QStyledItemDelegate
{
  Q_OBJECT

 public:
  // positions in ->data() where data is located
  static const int kEditor = Qt::UserRole;
  static const int kEditorName = Qt::UserRole + 1;
  static const int kEditorType = Qt::UserRole + 2;
  static const int kEditorSelect = Qt::UserRole + 3;
  static const int kSelected = Qt::UserRole + 4;
  static const int kHtml = Qt::UserRole + 5;

  enum EditType
  {
    kNumber,
    kString,
    kBool,
    kList
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

 protected:
  void paint(QPainter* painter,
             const QStyleOptionViewItem& option,
             const QModelIndex& index) const override;
  QSize sizeHint(const QStyleOptionViewItem& option,
                 const QModelIndex& index) const override;

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
  QStandardItem* makeItem(const std::any& item_param, bool short_name = false);

  template <typename Iterator>
  QStandardItem* makeList(QStandardItem* name_item,
                          const Iterator& begin,
                          const Iterator& end);
  template <typename Iterator>
  QStandardItem* makePropertyList(QStandardItem* name_item,
                                  const Iterator& begin,
                                  const Iterator& end);
  QStandardItem* makePropertyTable(QStandardItem* name_item,
                                   const PropertyTable& table);

  void makeItemEditor(const std::string& name,
                      QStandardItem* item,
                      const Selected& selected,
                      EditorItemDelegate::EditType type,
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
  ~ActionLayout() override;

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
  void selected(const Selected& selected, bool show_connectivity = false);
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

  int selectNext();
  int selectPrevious();

  void updateSelectedFields(const QModelIndex& index);

  void setReadOnly();
  void unsetReadOnly();

  void reload();
  void loadActions();

 private slots:
  void focusIndex(const QModelIndex& index);
  void defocus();

  void indexClicked();
  void indexDoubleClicked(const QModelIndex& index);

  void showCommandsMenu(const QPoint& pos);

 private:
  void handleAction(QWidget* action);

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
    kName,
    kValue
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
  bool readonly_;

  const HighlightSet& highlighted_;

  std::vector<Selected> navigation_history_;

  std::map<QWidget*, Descriptor::ActionCallback> actions_;
  Descriptor::ActionCallback deselect_action_;

  // used to finetune the double click interval
  static constexpr double kMouseDoubleClickScale = 0.75;
};

}  // namespace gui
