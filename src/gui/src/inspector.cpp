// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#include "inspector.h"

#include <QApplication>
#include <QColor>
#include <QComboBox>
#include <QDebug>
#include <QHeaderView>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QPushButton>
#include <QString>
#include <QStyleOptionViewItem>
#include <QTextDocument>
#include <QVariant>
#include <QWidget>
#include <algorithm>
#include <any>
#include <cmath>
#include <iterator>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "gui/gui.h"
#include "gui_utils.h"

Q_DECLARE_METATYPE(gui::Selected);
Q_DECLARE_METATYPE(gui::Descriptor::Editor);
Q_DECLARE_METATYPE(gui::EditorItemDelegate::EditType);
Q_DECLARE_METATYPE(std::any);
Q_DECLARE_METATYPE(std::string);

namespace gui {

SelectedItemModel::SelectedItemModel(const Selected& object,
                                     const QColor& selectable,
                                     const QColor& editable,
                                     QObject* parent)
    : QStandardItemModel(0, 2, parent),
      selectable_item_(selectable),
      editable_item_(editable),
      object_(object)
{
  setHorizontalHeaderLabels({"Name", "Value"});
}

QVariant SelectedItemModel::data(const QModelIndex& index, int role) const
{
  if (role == Qt::ForegroundRole) {
    const bool has_selected
        = itemFromIndex(index)->data(EditorItemDelegate::kSelected).isValid();

    if (has_selected) {
      return QBrush(selectable_item_);
    }
  } else {
    if (index.column() == 1) {
      if (role == Qt::BackgroundRole) {
        const bool has_editor
            = itemFromIndex(index)->data(EditorItemDelegate::kEditor).isValid();

        if (has_editor) {
          return QBrush(editable_item_);
        }
      }
    }
  }
  return QStandardItemModel::data(index, role);
}

void SelectedItemModel::updateObject()
{
  beginResetModel();

  removeRows(0, rowCount());

  if (!object_) {
    endResetModel();
    return;
  }

  auto editors = object_.getEditors();

  try {
    for (const auto& prop : object_.getProperties()) {
      QStandardItem* name_item = nullptr;
      QStandardItem* value_item = nullptr;

      makePropertyItem(prop, name_item, value_item);

      appendRow({name_item, value_item});

      // make editor if found
      auto editor_found = editors.find(prop.name);
      if (editor_found != editors.end()) {
        auto& editor = (*editor_found).second;
        makeItemEditor(prop.name,
                       value_item,
                       object_,
                       EditorItemDelegate::getEditorType(prop.value),
                       editor);
      }
    }
  } catch (const std::runtime_error& error) {
    QMessageBox::critical(qobject_cast<QWidget*>(parent()),
                          error.what(),
                          "Failed to populate properties.");
    appendRow({makeItem(QString::fromStdString("error")),
               makeItem(QString(error.what()))});
  }

  endResetModel();
}

void SelectedItemModel::makePropertyItem(const Descriptor::Property& property,
                                         QStandardItem*& name_item,
                                         QStandardItem*& value_item)
{
  const std::string& name = property.name;
  const std::any& value = property.value;

  if (name_item == nullptr) {
    name_item = makeItem(QString::fromStdString(name));
  }

  value_item = nullptr;

  // For a SelectionSet a row is created with the set items
  // as children rows
  if (auto sel_set = std::any_cast<Descriptor::PropertyList>(&value)) {
    value_item = makePropertyList(name_item, sel_set->begin(), sel_set->end());
  } else if (auto sel_set = std::any_cast<PropertyTable>(&value)) {
    value_item = makePropertyTable(name_item, *sel_set);
  } else if (auto sel_set = std::any_cast<SelectionSet>(&value)) {
    value_item = makeList(name_item, sel_set->begin(), sel_set->end());
  } else if (auto v_list = std::any_cast<std::vector<std::any>>(&value)) {
    value_item = makeList(name_item, v_list->begin(), v_list->end());
  } else if (auto v_set = std::any_cast<std::set<std::any>>(&value)) {
    value_item = makeList(name_item, v_set->begin(), v_set->end());
  } else {
    value_item = makeItem(value);
  }
}

QStandardItem* SelectedItemModel::makeItem(const QString& name)
{
  auto item = new QStandardItem(name);
  item->setEditable(false);
  item->setSelectable(false);
  item->setData(Qt::AlignTop, Qt::TextAlignmentRole);
  if (name.contains('\n')) {
    QFont font("Monospace");
    font.setStyleHint(QFont::Monospace);
    item->setData(font, Qt::FontRole);
  }
  return item;
}

QStandardItem* SelectedItemModel::makeItem(const std::any& item_param,
                                           bool short_name)
{
  if (auto selected = std::any_cast<Selected>(&item_param)) {
    QStandardItem* item = nullptr;
    if (short_name) {
      item = makeItem(QString::fromStdString(selected->getShortName()));
    } else {
      item = makeItem(QString::fromStdString(selected->getName()));
    }
    item->setData(QVariant::fromValue(*selected),
                  EditorItemDelegate::kSelected);
    return item;
  }
  return makeItem(
      QString::fromStdString(Descriptor::Property::toString(item_param)));
}

template <typename Iterator>
QStandardItem* SelectedItemModel::makeList(QStandardItem* name_item,
                                           const Iterator& begin,
                                           const Iterator& end)
{
  int index = 0;
  for (Iterator use_itr = begin; use_itr != end; ++use_itr) {
    auto index_item = makeItem(QString::number(++index));
    auto selected_item = makeItem(*use_itr);
    name_item->appendRow({index_item, selected_item});
  }

  return makeItem(QString(QString::number(index) + " items"));
}

template <typename Iterator>
QStandardItem* SelectedItemModel::makePropertyList(QStandardItem* name_item,
                                                   const Iterator& begin,
                                                   const Iterator& end)
{
  for (Iterator use_itr = begin; use_itr != end; ++use_itr) {
    auto& [name, value] = *use_itr;
    name_item->appendRow({makeItem(name, true), makeItem(value)});
  }

  return makeItem(QString(QString::number(name_item->rowCount()) + " items"));
}

QStandardItem* SelectedItemModel::makePropertyTable(QStandardItem* name_item,
                                                    const PropertyTable& table)
{
  const int rows = table.getData().size();
  const int columns = table.getColumnHeaders().size();

  QStandardItem* table_item = makeItem(QString());

  QString html;
  html += "<style>th, td {padding: 2px; border: 1px solid black; text-align: center; vertical-align: middle;}</style>";
  html += "<table>";
  // setup column headers
  html += "<tr><th />";
  for (int col = 0; col < columns; col++) {
    html += "<th>";
    html += QString::fromStdString(table.getColumnHeaders().at(col))
                .replace("\n", "<br>");
    html += "</th>";
  }
  html += "</tr>";
  // add rows
  for (int row = 0; row < rows; row++) {
    html += "<tr>";
    html += "<th>";
    html += QString::fromStdString(table.getRowHeaders().at(row))
                .replace("\n", "<br>");
    html += "</th>";
    for (int col = 0; col < columns; col++) {
      html += "<td>";
      html += QString::fromStdString(table.getData().at(row).at(col))
                  .replace("\n", "<br>");
      html += "</td>";
    }
    html += "</tr>";
  }
  html += "</table>";
  table_item->setData(html, EditorItemDelegate::kHtml);

  name_item->appendRow({nullptr, table_item});

  return makeItem(QString(QString::number(rows * columns) + " items"));
}

void SelectedItemModel::makeItemEditor(const std::string& name,
                                       QStandardItem* item,
                                       const Selected& selected,
                                       const EditorItemDelegate::EditType type,
                                       const Descriptor::Editor& editor)
{
  item->setData(QVariant::fromValue(selected),
                EditorItemDelegate::kEditorSelect);
  item->setData(QVariant::fromValue(name), EditorItemDelegate::kEditorName);

  Descriptor::Editor used_editor = editor;
  if (type == EditorItemDelegate::kBool) {
    // for BOOL, replace options with true/false options
    used_editor.options = {{"True", true}, {"False", false}};
  }

  item->setData(QVariant::fromValue(used_editor), EditorItemDelegate::kEditor);
  if (used_editor.options.empty()) {
    // options are empty so use selected type
    item->setData(QVariant::fromValue(type), EditorItemDelegate::kEditorType);
  } else {
    // options are not empty so use list type
    item->setData(QVariant::fromValue(EditorItemDelegate::kList),
                  EditorItemDelegate::kEditorType);
  }
}

/////////

EditorItemDelegate::EditorItemDelegate(SelectedItemModel* model,
                                       QObject* parent)
    : QStyledItemDelegate(parent),
      model_(model),
      background_(model->getEditableColor())
{
}

QWidget* EditorItemDelegate::createEditor(
    QWidget* parent,
    const QStyleOptionViewItem& /* option */,
    const QModelIndex& index) const
{
  auto type = index.model()
                  ->data(index, kEditorType)
                  .value<EditorItemDelegate::EditType>();
  QWidget* editor;
  if (type == kList) {
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
  auto type = index.model()
                  ->data(index, kEditorType)
                  .value<EditorItemDelegate::EditType>();
  auto [callback, values]
      = index.model()->data(index, kEditor).value<Descriptor::Editor>();
  QString value = index.model()->data(index, Qt::EditRole).toString();

  if (type != kList) {
    QLineEdit* line_edit = static_cast<QLineEdit*>(editor);
    line_edit->setText(value);
  } else {
    QComboBox* combo_box = static_cast<QComboBox*>(editor);
    // disconnect to stop callback to setModelData
    combo_box->disconnect();
    combo_box->clear();
    for (const auto& [name, option_value] : values) {
      combo_box->addItem(QString::fromStdString(name),
                         QVariant::fromValue(option_value));
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
  auto type
      = model->data(index, kEditorType).value<EditorItemDelegate::EditType>();
  auto [callback, values]
      = model->data(index, kEditor).value<Descriptor::Editor>();

  const QString old_value = index.model()->data(index, Qt::EditRole).toString();

  bool accepted = false;
  QString value;
  std::any callback_value;
  bool value_valid = false;
  if (type != kList) {
    QLineEdit* line_edit = static_cast<QLineEdit*>(editor);
    value = line_edit->text();
    if (type == kNumber) {
      callback_value = value.toDouble(&value_valid);
    } else if (type == kString) {
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
    try {
      accepted = callback(callback_value);
    } catch (const std::runtime_error&) {
      // failed due to an error thrown by openroad
      accepted = false;
    }
  }

  QString edit_save = old_value;  // default to set to old value
  if (accepted) {
    // retrieve property again
    auto selected = model->data(index, kEditorSelect).value<Selected>();
    auto item_name = model->data(index, kEditorName).value<std::string>();

    auto new_property = selected.getProperty(item_name);
    if (model->data(index, kSelected).isValid()) {
      auto new_selected = std::any_cast<Selected>(new_property);
      model->setData(index, QVariant::fromValue(new_selected), kSelected);
    }
    edit_save
        = QString::fromStdString(Descriptor::Property::toString(new_property));
    model_->selectedItemChanged(index);
  }
  model->setData(index, edit_save, Qt::EditRole);

  // disable editing as we are done editing
  model_->itemFromIndex(index)->setEditable(false);
}

EditorItemDelegate::EditType EditorItemDelegate::getEditorType(
    const std::any& value)
{
  if (std::any_cast<const char*>(&value)) {
    return EditorItemDelegate::kString;
  }
  if (std::any_cast<const std::string>(&value)) {
    return EditorItemDelegate::kString;
  }
  if (std::any_cast<int>(&value)) {
    return EditorItemDelegate::kNumber;
  }
  if (std::any_cast<unsigned int>(&value)) {
    return EditorItemDelegate::kNumber;
  }
  if (std::any_cast<double>(&value)) {
    return EditorItemDelegate::kNumber;
  }
  if (std::any_cast<float>(&value)) {
    return EditorItemDelegate::kNumber;
  }
  if (std::any_cast<bool>(&value)) {
    return EditorItemDelegate::kBool;
  }
  return EditorItemDelegate::kString;
}

void EditorItemDelegate::paint(QPainter* painter,
                               const QStyleOptionViewItem& option,
                               const QModelIndex& index) const
{
  if (index.model()->data(index, kHtml).toString().isEmpty()) {
    QStyledItemDelegate::paint(painter, option, index);
    return;
  }

  QStyleOptionViewItem options(option);
  initStyleOption(&options, index);

  painter->save();

  QTextDocument doc;
  doc.setHtml(index.model()->data(index, kHtml).toString());

  /* Call this to get the focus rect and selection background. */
  options.text = "";
  options.widget->style()->drawControl(
      QStyle::CE_ItemViewItem, &options, painter);

  /* Draw using text document. */
  painter->translate(options.rect.left(), options.rect.top());
  const QRectF clip(0, 0, options.rect.width(), options.rect.height());
  doc.drawContents(painter, clip);

  painter->restore();
}

QSize EditorItemDelegate::sizeHint(const QStyleOptionViewItem& option,
                                   const QModelIndex& index) const
{
  if (index.model()->data(index, kHtml).toString().isEmpty()) {
    return QStyledItemDelegate::sizeHint(option, index);
  }
  QStyleOptionViewItem options(option);
  initStyleOption(&options, index);

  QTextDocument doc;
  doc.setHtml(index.model()->data(index, kHtml).toString());
  doc.setTextWidth(options.rect.width());
  return QSize(doc.idealWidth(), doc.size().height());
}

////////

ActionLayout::ActionLayout(QWidget* parent) : QLayout(parent)
{
}

ActionLayout::~ActionLayout()
{
  clear();
}

void ActionLayout::clear()
{
  for (auto& item : actions_) {
    removeItem(item);
  }
}

int ActionLayout::count() const
{
  return actions_.size();
}

void ActionLayout::addItem(QLayoutItem* item)
{
  actions_.append(item);

  item->widget()->setMinimumHeight(rowHeight());
}

QLayoutItem* ActionLayout::itemAt(int index) const
{
  return actions_.value(index);
}

QLayoutItem* ActionLayout::takeAt(int index)
{
  if (index >= 0 && index < actions_.size()) {
    return actions_.takeAt(index);
  }

  return nullptr;
}

QSize ActionLayout::sizeHint() const
{
  return minimumSize();
}

QSize ActionLayout::minimumSize() const
{
  if (actions_.empty()) {
    return QSize(0, 0);
  }

  QSize size;
  for (const auto& item : actions_) {
    size = size.expandedTo(item->minimumSize());
  }

  const QMargins margins = contentsMargins();
  size += QSize(margins.left() + margins.right(),
                margins.top() + margins.bottom());
  return size;
}

int ActionLayout::rowHeight() const
{
  QPushButton button;
  return button.sizeHint().height();
}

int ActionLayout::rowSpacing() const
{
  QPushButton button;
  return button.style()->layoutSpacing(
      QSizePolicy::PushButton, QSizePolicy::PushButton, Qt::Vertical);
}

int ActionLayout::buttonSpacing() const
{
  QPushButton button;
  return button.style()->layoutSpacing(
      QSizePolicy::PushButton, QSizePolicy::PushButton, Qt::Horizontal);
}

int ActionLayout::requiredRows(int width) const
{
  std::vector<ItemList> rows;
  organizeItemsToRows(width, rows);
  return rows.size();
}

int ActionLayout::itemWidth(QLayoutItem* item) const
{
  const QWidget* wid = item->widget();
  return wid->sizeHint().width();
}

void ActionLayout::setGeometry(const QRect& rect)
{
  QLayout::setGeometry(rect);

  std::vector<ItemList> rows;
  organizeItemsToRows(rect.width(), rows);
  if (rows.empty()) {
    return;
  }

  int left, top, right, bottom;
  getContentsMargins(&left, &top, &right, &bottom);
  QRect effective_rect = rect.adjusted(left, top, -right, -bottom);

  const int button_height = rowHeight();

  int y = effective_rect.y();
  for (ItemList& items : rows) {
    int x = effective_rect.x();
    int empty_space = effective_rect.right() - (x + rowWidth(items));
    const int size_adder
        = std::ceil(empty_space / static_cast<double>(items.size()));

    for (auto& item : items) {
      QSize size = item->sizeHint();

      if (empty_space != 0) {
        if (size_adder > empty_space) {
          size.setWidth(size.width() + empty_space);
          empty_space = 0;
        } else {
          size.setWidth(size.width() + size_adder);
          empty_space -= size_adder;
        }
      }
      size.setHeight(button_height);

      item->setGeometry(QRect(QPoint(x, y), size));
      x += size.width() + buttonSpacing();
    }

    y += rowHeight() + rowSpacing();
  }
}

bool ActionLayout::hasHeightForWidth() const
{
  return true;
}

int ActionLayout::heightForWidth(int width) const
{
  const int rows = requiredRows(width);
  int height = rows * rowHeight();
  if (rows > 1) {
    height += (rows - 1) * rowSpacing();
  }
  return height;
}

void ActionLayout::organizeItemsToRows(int width,
                                       std::vector<ItemList>& rows) const
{
  if (actions_.empty()) {
    return;
  }

  int left, top, right, bottom;
  getContentsMargins(&left, &top, &right, &bottom);
  const int effective_width = width + left + right;

  const int button_spacing = buttonSpacing();

  int x = 0;
  ItemList row;
  for (const auto& item : actions_) {
    const int item_width = itemWidth(item);

    int next_x = x + item_width;
    if (next_x >= effective_width) {
      x = item_width;
      rows.emplace_back(row.begin(), row.end());
      row.clear();
    } else {
      x = next_x;
    }
    row.push_back(item);

    x += button_spacing;
  }
  rows.emplace_back(row.begin(), row.end());

  if (rows.size() == 1) {
    return;
  }
  // make rows approximately even
  int total_width = 0;
  for (auto& row : rows) {
    total_width += rowWidth(row);
  }
  const int target_width = total_width / static_cast<double>(rows.size());
  for (int row = 0; row < rows.size() - 1; row++) {
    ItemList& source = rows[row];
    ItemList& destination = rows[row + 1];

    while (rowWidth(source) > target_width && source.size() > 1) {
      auto last_item = source.end() - 1;
      destination.insert(destination.begin(), *last_item);
      source.erase(last_item);
    }
  }
}

int ActionLayout::rowWidth(ItemList& row) const
{
  if (row.empty()) {
    return 0;
  }

  const int button_spacing = buttonSpacing();
  int width = -button_spacing;
  for (auto& item : row) {
    width += itemWidth(item) + buttonSpacing();
  }
  return width;
}

////////

Inspector::Inspector(const SelectionSet& selected,
                     const HighlightSet& highlighted,
                     QWidget* parent)
    : QDockWidget("Inspector", parent),
      view_(new ObjectTree(this)),
      model_(new SelectedItemModel(selection_,
                                   Qt::blue,
                                   QColor(0xc6, 0xff, 0xc4) /* pale green */,
                                   this)),
      layout_(new QVBoxLayout),
      action_layout_(new ActionLayout),
      selected_(selected),
      selected_itr_(selected.begin()),
      button_frame_(new QFrame(this)),
      button_next_(new QPushButton("Next →", this)),
      button_prev_(new QPushButton("← Previous", this)),
      selected_itr_label_(new QLabel(this)),
      commands_menu_(new QMenu("Commands Menu", this)),
      readonly_(false),
      highlighted_(highlighted)
{
  setObjectName("inspector");  // for settings
  view_->setModel(model_);
  view_->setItemDelegate(new EditorItemDelegate(model_, this));

  QHeaderView* header = view_->header();
  header->setSectionResizeMode(kName, QHeaderView::Interactive);

  QWidget* container = new QWidget(this);

  layout_->addWidget(view_, /* stretch */ 1);
  layout_->addLayout(action_layout_);

  button_frame_->setFrameShape(QFrame::StyledPanel);
  button_frame_->setFrameShadow(QFrame::Raised);
  QHBoxLayout* button_layout = new QHBoxLayout;
  button_layout->addWidget(button_prev_, 1);
  button_layout->addWidget(selected_itr_label_, 1, Qt::AlignHCenter);
  button_layout->addWidget(button_next_, 1);
  button_frame_->setLayout(button_layout);
  layout_->addWidget(button_frame_);
  button_frame_->setVisible(false);

  container->setLayout(layout_);

  setWidget(container);

  // connect so announcements can be made about changes
  connect(model_, &SelectedItemModel::selectedItemChanged, [this]() {
    emit selectedItemChanged(selection_);
  });

  connect(model_,
          &SelectedItemModel::selectedItemChanged,
          this,
          &Inspector::updateSelectedFields);

  connect(view_, &ObjectTree::clicked, this, &Inspector::clicked);
  connect(view_, &ObjectTree::doubleClicked, this, &Inspector::doubleClicked);

  connect(
      button_prev_, &QPushButton::pressed, this, &Inspector::selectPrevious);

  connect(button_next_, &QPushButton::pressed, this, &Inspector::selectNext);

  view_->setMouseTracking(true);
  view_->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(view_, &ObjectTree::entered, this, &Inspector::focusIndex);

  connect(view_, &ObjectTree::viewportEntered, this, &Inspector::defocus);
  connect(view_, &ObjectTree::mouseExited, this, &Inspector::defocus);

  mouse_timer_.setInterval(kMouseDoubleClickScale
                           * QApplication::doubleClickInterval());
  mouse_timer_.setSingleShot(true);
  connect(&mouse_timer_, &QTimer::timeout, this, &Inspector::indexClicked);

  setCommandsMenu();
}

void Inspector::setCommandsMenu()
{
  connect(view_,
          &ObjectTree::customContextMenuRequested,
          this,
          &Inspector::showCommandsMenu);

  connect(commands_menu_->addAction("Report Path"),
          &QAction::triggered,
          [this] { writePathReportCommand(); });
}

void Inspector::showCommandsMenu(const QPoint& pos)
{
  clicked_index_ = view_->indexAt(pos);
  QStandardItem* item = model_->itemFromIndex(clicked_index_);

  if (!item) {
    return;
  }

  Selected selected
      = item->data(EditorItemDelegate::kSelected).value<Selected>();

  if (selected) {
    if (selected.getTypeName() == "ITerm") {
      report_text_ = selected.getName();
      commands_menu_->popup(view_->viewport()->mapToGlobal(pos));
    }
  }
}

void Inspector::writePathReportCommand()
{
  QString command = "report_checks -through ";
  command += Utils::wrapInCurly(QString::fromStdString(report_text_));

  emit setCommand(command);
}

void Inspector::adjustHeaders()
{
  if (selection_) {
    // resize to fit the contents
    view_->resizeColumnToContents(kName);
    view_->resizeColumnToContents(kValue);

    const auto margins = view_->contentsMargins();
    const int width = view_->size().width() - margins.left() - margins.right();
    const int name_width = view_->columnWidth(kName);
    const int value_width = view_->columnWidth(kValue);

    const int max_name_width = width - value_width;

    // check if name column is wider than the widest available
    if (name_width > max_name_width) {
      const int min_name_width = 0.5 * name_width;
      // check if using a smaller name column will be useful,
      // otherwise keep full width column.
      if (min_name_width <= max_name_width) {
        view_->setColumnWidth(kName, max_name_width);
      }
    }
  }
}

int Inspector::selectNext()
{
  if (selected_.empty()) {
    selected_itr_ = selected_.begin();

    return 0;
  }

  selected_itr_++;  // go to next
  if (selected_itr_ == selected_.end()) {
    selected_itr_ = selected_.begin();
  }
  emit inspect(*selected_itr_);

  return getSelectedIteratorPosition();
}

int Inspector::selectPrevious()
{
  if (selected_.empty()) {
    selected_itr_ = selected_.begin();

    return 0;
  }

  if (selected_itr_ == selected_.begin()) {
    selected_itr_ = selected_.end();
  }
  selected_itr_--;
  emit inspect(*selected_itr_);

  return getSelectedIteratorPosition();
}

int Inspector::getSelectedIteratorPosition()
{
  return std::distance(selected_.begin(), selected_itr_);
}

void Inspector::inspect(const Selected& object)
{
  QApplication::setOverrideCursor(Qt::WaitCursor);

  if (deselect_action_) {
    deselect_action_();
  }

  // check if object is part of history, otherwise delete history
  if (std::ranges::find(navigation_history_, object)
      == navigation_history_.end()) {
    navigation_history_.clear();
  }

  if (object) {
    qDebug() << "Inspector change selection to"
             << QString::fromStdString(object.getName());
  } else {
    qDebug() << "Inspector change selection to nothing";
  }
  selection_ = object;
  emit selection(object);

  reload();

  // update iterator
  selected_itr_ = std::ranges::find(selected_, selection_);
  selected_itr_label_->setText(
      QString::number(getSelectedIteratorPosition() + 1) + "/"
      + QString::number(selected_.size()));

  // Auto open small lists
  for (int row = 0; row < model_->rowCount(); row++) {
    const QModelIndex index = model_->index(row, 0);
    if (model_->hasChildren(index) && model_->rowCount(index) < 10) {
      view_->expand(index);
    }
  }

  adjustHeaders();

  QApplication::restoreOverrideCursor();
}

void Inspector::reload()
{
  loadActions();
  model_->updateObject();
}

void Inspector::setReadOnly()
{
  readonly_ = true;
  reload();
}

void Inspector::unsetReadOnly()
{
  readonly_ = false;
  reload();
}

void Inspector::loadActions()
{
  // remove action buttons and ensure delete
  action_layout_->clear();
  while (!actions_.empty()) {
    auto it = actions_.begin();
    auto widget = it->first;
    actions_.erase(it);
    delete widget;  // no longer in the map so it's safe to delete
  }

  deselect_action_ = Descriptor::ActionCallback();

  if (!selection_) {
    return;
  }

  // add action buttons
  for (const auto& action : selection_.getActions()) {
    if (action.name == Descriptor::kDeselectAction) {
      deselect_action_ = action.callback;
    } else {
      makeAction(action);
    }
  }
  if (isHighlighted(selection_)) {
    makeAction({"Remove from highlight", [this]() -> Selected {
                  emit removeHighlight({&selection_});
                  return selection_;
                }});
  } else {
    makeAction({"Add to highlight", [this]() -> Selected {
                  emit addHighlight({selection_});
                  return selection_;
                }});
  }

  if (!navigation_history_.empty()) {
    makeAction({"Navigate back", [this]() -> Selected {
                  navigateBack();
                  return selection_;
                }});
  }
}

void Inspector::makeAction(const Descriptor::Action& action)
{
  std::vector<std::pair<std::string, QString>> button_replacements{
      {"Delete", ":/delete.png"},
      {"Zoom to", ":/zoom_to.png"},
      {"Remove from highlight", ":/highlight_off.png"},
      {"Add to highlight", ":/highlight_on.png"},
      {"Focus", ":/focus.png"},
      {"De-focus", ":/defocus.png"},
      {"Navigate back", ":/undo.png"},
      {"Insert Buffer", ":/buffer.png"}};
  std::vector<std::pair<std::string, QString>> symbol_replacements{
      {"Fanin Cone", "▷"}, {"Fanout Cone", "◁"}};

  const std::string& name = action.name;

  QPushButton* button = nullptr;
  for (const auto& [label, icon] : button_replacements) {
    if (name == label) {
      button = new QPushButton(QIcon(icon), "", this);
      button->setToolTip(
          QString::fromStdString(name));  // set tool since this is an icon
      break;
    }
  }
  if (button == nullptr) {
    for (const auto& [label, new_text] : symbol_replacements) {
      if (name == label) {
        button = new QPushButton(new_text, this);
        button->setToolTip(
            QString::fromStdString(name));  // set tool since this is a symbol
        break;
      }
    }
  }

  if (button == nullptr) {
    button = new QPushButton(QString::fromStdString(name), this);
  }
  connect(button, &QPushButton::released, [this, button]() {
    handleAction(button);
  });
  action_layout_->addWidget(button);
  actions_[button] = action.callback;

  if (readonly_ && name == "Delete") {
    button->setEnabled(false);
  }
}

void Inspector::clicked(const QModelIndex& index)
{
  // QT sends both single and double clicks, so they need to be handled with a
  // timer to be able to tell the difference
  if (!mouse_timer_.isActive()) {
    clicked_index_ = index;

    // Bypass the timer for those items for which there's
    // no double click handling
    QStandardItem* item = model_->itemFromIndex(index);
    QVariant edit_data = item->data(EditorItemDelegate::kEditor);
    if (!edit_data.isValid()) {
      indexClicked();
      return;
    }

    mouse_timer_.start();
  }
}

void Inspector::doubleClicked(const QModelIndex& index)
{
  mouse_timer_.stop();
  emit indexDoubleClicked(index);
}

void Inspector::indexClicked()
{
  // handle single click event
  QStandardItem* item = model_->itemFromIndex(clicked_index_);
  auto new_selected
      = item->data(EditorItemDelegate::kSelected).value<Selected>();
  if (new_selected) {
    if (navigation_history_.empty()) {
      // add starting object
      navigation_history_.push_back(selection_);
    }
    navigation_history_.push_back(new_selected);
    // If shift is help add to the list instead of replacing list
    if (qGuiApp->keyboardModifiers() & Qt::ShiftModifier) {
      emit addSelected(new_selected);
      inspect(new_selected);
    } else {
      emit selected(new_selected, false);
    }
  }
}

void Inspector::indexDoubleClicked(const QModelIndex& index)
{
  // handle single click event
  QStandardItem* item = model_->itemFromIndex(index);
  QVariant item_data = item->data(EditorItemDelegate::kEditor);

  if (!readonly_ && item_data.isValid()) {
    // set editable
    item->setEditable(true);
    // start editing
    view_->edit(index);
  }
}

void Inspector::focusIndex(const QModelIndex& focus_index)
{
  defocus();

  QStandardItem* item = model_->itemFromIndex(focus_index);
  QVariant item_data = item->data(EditorItemDelegate::kSelected);

  if (item_data.isValid()) {
    Selected sel = item_data.value<Selected>();
    if (!sel.isSlowHighlight()) {
      // emit the selected item as something to focus on
      emit focus(sel);
    }
  }
}

void Inspector::defocus()
{
  emit focus(Selected());
}

void Inspector::update(const Selected& object)
{
  if (selected_.empty()) {
    button_frame_->setVisible(false);
    inspect(Selected());
  } else {
    if (selected_.size() > 1) {
      button_frame_->setVisible(true);
    } else {
      button_frame_->setVisible(false);
    }
    selected_itr_ = selected_.begin();
    if (object) {
      inspect(object);
    } else {
      inspect(*selected_itr_);
    }
    raise();
  }
}

void Inspector::handleAction(QWidget* action)
{
  // Copy the callback as the action may be deleted from within the
  // callback.
  auto callback = actions_[action];
  Selected new_selection;
  try {
    new_selection = callback();
  } catch (const std::runtime_error&) {
    return;
  }

  if (new_selection && new_selection == selection_) {
    loadActions();
    return;
  }

  int itr_index = std::distance(selected_.begin(), selected_itr_);
  // remove the current selection
  emit removeSelected(selection_);
  if (new_selection) {
    // new selection as made, so add that and inspect it
    emit addSelected(new_selection);
    inspect(new_selection);
  } else {
    if (selected_.empty()) {
      // set is empty
      emit selected(Selected());
    } else {
      // determine new position in set
      itr_index = std::min(itr_index, static_cast<int>(selected_.size()) - 1);

      selected_itr_ = selected_.begin();
      std::advance(selected_itr_, itr_index);

      // inspect item at new this index
      inspect(*selected_itr_);
    }
  }
}

void Inspector::updateSelectedFields(const QModelIndex& index)
{
  // save expanded items
  std::map<QString, bool> expanded;
  for (int row = 0; row < model_->rowCount(); row++) {
    const QModelIndex index = model_->index(row, 0);
    const QString name = model_->itemFromIndex(index)->text();
    expanded[name] = view_->isExpanded(index);
  }

  reload();

  // restore expanded items
  for (int row = 0; row < model_->rowCount(); row++) {
    const QModelIndex row_index = model_->index(row, 0);
    const QString name = model_->itemFromIndex(row_index)->text();

    auto itr = expanded.find(name);
    if (itr != expanded.end()) {
      view_->setExpanded(row_index, (*itr).second);
    }
  }
}

bool Inspector::isHighlighted(const Selected& selected)
{
  for (const auto& highlight_set : highlighted_) {
    if (highlight_set.find(selected) != highlight_set.end()) {
      return true;
    }
  }

  return false;
}

void Inspector::navigateBack()
{
  if (navigation_history_.empty()) {
    return;
  }

  Selected next;
  if (navigation_history_.size() == 1) {
    next = navigation_history_.back();
    navigation_history_.clear();
  } else {
    next = *(navigation_history_.end() - 2);

    if (navigation_history_.size() == 2) {
      navigation_history_.clear();
    } else {
      navigation_history_.erase(navigation_history_.end() - 1);
    }
  }

  qDebug() << "Navigate to" << QString::fromStdString(next.getName());
  emit selected(next);
}

////////////

ObjectTree::ObjectTree(QWidget* parent) : QTreeView(parent)
{
}

void ObjectTree::leaveEvent(QEvent* event)
{
  emit mouseExited();
}

}  // namespace gui
