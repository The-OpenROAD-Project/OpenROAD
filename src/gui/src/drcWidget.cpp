// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#include "drcWidget.h"

#include <QApplication>
#include <QComboBox>
#include <QFileDialog>
#include <QHeaderView>
#include <QPushButton>
#include <QVBoxLayout>
#include <QVariant>
#include <QWidget>
#include <algorithm>
#include <any>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "dbDescriptors.h"
#include "gui/gui.h"
#include "inspector.h"
#include "odb/db.h"
#include "odb/geom.h"
#include "utl/Logger.h"

Q_DECLARE_METATYPE(odb::dbMarker*);

namespace gui {

QVariant DRCItemModel::data(const QModelIndex& index, int role) const
{
  if (role == Qt::FontRole) {
    auto item_data = itemFromIndex(index)->data();
    if (item_data.isValid()) {
      odb::dbMarker* item = item_data.value<odb::dbMarker*>();
      QFont font = QApplication::font();
      font.setBold(!item->isVisited());
      return font;
    }
  }

  return QStandardItemModel::data(index, role);
}

///////

DRCWidget::DRCWidget(QWidget* parent)
    : QDockWidget("DRC Viewer", parent),
      logger_(nullptr),
      view_(new ObjectTree(this)),
      model_(new DRCItemModel(this)),
      chip_(nullptr),
      categories_(new QComboBox(this)),
      load_(new QPushButton("Load...", this)),
      renderer_(std::make_unique<DRCRenderer>())
{
  setObjectName("drc_viewer");  // for settings

  model_->setHorizontalHeaderLabels({"Type", "Violation"});
  view_->setModel(model_);

  QHeaderView* header = view_->header();
  header->setSectionResizeMode(0, QHeaderView::ResizeToContents);
  header->setSectionResizeMode(1, QHeaderView::Stretch);

  QHBoxLayout* selection_layout = new QHBoxLayout;
  selection_layout->addWidget(categories_);
  selection_layout->addWidget(load_);

  QWidget* container = new QWidget(this);
  QVBoxLayout* layout = new QVBoxLayout;
  layout->addLayout(selection_layout);
  layout->addWidget(view_);

  container->setLayout(layout);
  setWidget(container);

  connect(view_, &ObjectTree::clicked, this, &DRCWidget::clicked);
  connect(view_, &ObjectTree::doubleClicked, this, &DRCWidget::doubleClicked);
  connect(view_->selectionModel(),
          &QItemSelectionModel::selectionChanged,
          this,
          &DRCWidget::selectionChanged);
  connect(load_, &QPushButton::released, this, &DRCWidget::selectReport);
  connect(categories_,
          qOverload<int>(&QComboBox::currentIndexChanged),
          this,
          &DRCWidget::updateModel);

  view_->setMouseTracking(true);
  connect(view_, &ObjectTree::entered, this, &DRCWidget::focusIndex);

  connect(view_, &ObjectTree::viewportEntered, this, &DRCWidget::defocus);
  connect(view_, &ObjectTree::mouseExited, this, &DRCWidget::defocus);
}

void DRCWidget::focusIndex(const QModelIndex& focus_index)
{
  defocus();

  QStandardItem* item = model_->itemFromIndex(focus_index);
  QVariant data = item->data();
  if (data.isValid()) {
    odb::dbMarker* marker = data.value<odb::dbMarker*>();
    emit focus(Gui::get()->makeSelected(marker));
  }
}

void DRCWidget::defocus()
{
  emit focus(Selected());
}

void DRCWidget::setLogger(utl::Logger* logger)
{
  logger_ = logger;
}

void DRCWidget::selectReport()
{
  if (!chip_) {
    logger_->error(utl::GUI, 104, "No database has been loaded");
  }

  // OpenLane uses .drc and OpenROAD-flow-scripts uses .rpt
  QString filename = QFileDialog::getOpenFileName(
      this,
      tr("DRC Report"),
      QString(),
      tr("DRC Report (*.rpt *.drc *.json);;TritonRoute Report (*.rpt "
         "*.drc);;JSON (*.json);;All (*)"));
  if (!filename.isEmpty()) {
    loadReport(filename);
  }
}

void DRCWidget::selectionChanged(const QItemSelection& selected,
                                 const QItemSelection& deselected)
{
  auto indexes = selected.indexes();
  if (indexes.isEmpty()) {
    return;
  }

  emit clicked(indexes.first());
}

void DRCWidget::toggleParent(QStandardItem* child)
{
  QStandardItem* parent = child->parent();
  std::vector<bool> states;
  for (int r = 0; r < parent->rowCount(); r++) {
    auto* pchild = parent->child(r, 0);
    states.push_back(pchild->checkState() == Qt::Checked);
  }

  const bool all_on = std::ranges::all_of(states, [](bool v) { return v; });
  const bool any_on = std::ranges::any_of(states, [](bool v) { return v; });

  if (all_on) {
    parent->setCheckState(Qt::Checked);
  } else if (any_on) {
    parent->setCheckState(Qt::PartiallyChecked);
  } else {
    parent->setCheckState(Qt::Unchecked);
  }
}

bool DRCWidget::setVisibleDRC(QStandardItem* item,
                              bool visible,
                              bool announce_parent)
{
  QVariant data = item->data();
  if (data.isValid()) {
    odb::dbMarker* marker = data.value<odb::dbMarker*>();
    if (item->isCheckable() && marker->isVisible() != visible) {
      item->setCheckState(visible ? Qt::Checked : Qt::Unchecked);
      marker->setVisible(visible);
      renderer_->redraw();
      if (announce_parent) {
        toggleParent(item);
      }
      return true;
    }
  }

  return false;
}

void DRCWidget::showMarker(const QModelIndex& index, bool open_inspector)
{
  QStandardItem* item = model_->itemFromIndex(index);
  QVariant data = item->data();
  if (data.isValid()) {
    odb::dbMarker* marker = data.value<odb::dbMarker*>();
    const bool is_visible = item->checkState() == Qt::Checked;
    if (setVisibleDRC(item, is_visible, true)) {
      // do nothing, since its handled in function
    } else if (qGuiApp->keyboardModifiers() & Qt::ControlModifier) {
      marker->setVisited(false);
    } else {
      Selected t = Gui::get()->makeSelected(marker);
      emit selectDRC(t, open_inspector);
      focusIndex(index);
    }
  } else {
    if (item->hasChildren()) {
      const bool state = item->checkState() == Qt::Checked;
      for (int r = 0; r < item->rowCount(); r++) {
        auto* child = item->child(r, 0);
        setVisibleDRC(child, state, false);
      }
    }
  }
}

void DRCWidget::clicked(const QModelIndex& index)
{
  showMarker(index, false);
}

void DRCWidget::doubleClicked(const QModelIndex& index)
{
  showMarker(index, true);
}

void DRCWidget::setChip(odb::dbChip* chip)
{
  chip_ = chip;

  addOwner(chip_);
  updateMarkerGroups();
}

void DRCWidget::showEvent(QShowEvent* event)
{
  if (chip_) {
    addOwner(chip_);
  }

  updateMarkerGroups();
  toggleRenderer(true);
}

void DRCWidget::hideEvent(QHideEvent* event)
{
  removeOwner();

  toggleRenderer(false);
}

void DRCWidget::toggleRenderer(bool visible)
{
  if (!Gui::enabled()) {
    return;
  }

  auto gui = Gui::get();
  if (visible) {
    gui->registerRenderer(renderer_.get());
  } else {
    gui->unregisterRenderer(renderer_.get());
  }
}

void DRCWidget::updateModel()
{
  const std::string name = categories_->currentText().toStdString();
  odb::dbMarkerCategory* category = chip_->findMarkerCategory(name.c_str());

  model_->removeRows(0, model_->rowCount());

  if (category != nullptr) {
    for (odb::dbMarkerCategory* subcategory : category->getMarkerCategories()) {
      populateCategory(subcategory, model_->invisibleRootItem());
    }
  }

  toggleRenderer(!this->isHidden());
  renderer_->setCategory(category);
}

void DRCWidget::populateCategory(odb::dbMarkerCategory* category,
                                 QStandardItem* model)
{
  if (category == nullptr) {
    return;
  }

  auto make_item = [](const QString& text) {
    QStandardItem* item = new QStandardItem(text);
    item->setEditable(false);
    item->setSelectable(false);
    return item;
  };

  QStandardItem* type_group = make_item(category->getName());
  type_group->setCheckable(true);
  type_group->setCheckState(Qt::Checked);

  for (odb::dbMarkerCategory* subcategory : category->getMarkerCategories()) {
    populateCategory(subcategory, type_group);
  }

  int violation_idx = 1;
  QStandardItem* marker_item_child = nullptr;
  for (odb::dbMarker* marker : category->getMarkers()) {
    QStandardItem* marker_item
        = make_item(QString::fromStdString(marker->getName()));
    marker_item_child = marker_item;
    marker_item->setSelectable(true);
    marker_item->setData(QVariant::fromValue(marker));
    QStandardItem* marker_index = make_item(QString::number(violation_idx++));
    marker_index->setData(QVariant::fromValue(marker));
    marker_index->setCheckable(true);
    marker_index->setCheckState(marker->isVisible() ? Qt::Checked
                                                    : Qt::Unchecked);

    type_group->appendRow({marker_index, marker_item});
  }

  if (marker_item_child != nullptr) {
    toggleParent(marker_item_child);
  }

  model->appendRow(
      {type_group,
       make_item(QString::number(category->getMarkerCount()) + " markers")});
}

void DRCWidget::updateSelection(const Selected& selection)
{
  const std::any& object = selection.getObject();
  if (auto s = std::any_cast<odb::dbMarker*>(&object)) {
    (*s)->setVisited(true);
    emit update();
  }
}

void DRCWidget::selectCategory(odb::dbMarkerCategory* category)
{
  if (category == nullptr) {
    categories_->setCurrentText("");
  } else {
    category = category->getTopCategory();
    categories_->setCurrentText(category->getName());
  }

  updateModel();
  show();
  raise();
}

void DRCWidget::loadReport(const QString& filename)
{
  Gui::get()->removeSelected<odb::dbMarker*>();
  Gui::get()->removeSelected<odb::dbMarkerCategory*>();

  odb::dbMarkerCategory* category = nullptr;
  try {
    // OpenLane uses .drc and OpenROAD-flow-scripts uses .rpt
    if (filename.endsWith(".rpt") || filename.endsWith(".drc")) {
      category = loadTRReport(filename);
    } else if (filename.endsWith(".json")) {
      category = loadJSONReport(filename);
    } else {
      logger_->error(utl::GUI,
                     32,
                     "Unable to determine type of {}",
                     filename.toStdString());
    }
  } catch (std::runtime_error& e) {
    logger_->warn(utl::GUI, 110, "Failed to load: {}", e.what());
  }  // catch errors

  if (category != nullptr) {
    selectCategory(category);
  }

  updateModel();
  show();
  raise();
}

odb::dbMarkerCategory* DRCWidget::loadTRReport(const QString& filename)
{
  const std::string file = filename.toStdString();
  return odb::dbMarkerCategory::fromTR(chip_, "DRC", file);
}

odb::dbMarkerCategory* DRCWidget::loadJSONReport(const QString& filename)
{
  const std::string file = filename.toStdString();
  const auto categories = odb::dbMarkerCategory::fromJSON(chip_, file);

  if (categories.size() > 1) {
    logger_->warn(utl::GUI,
                  30,
                  "Multiple marker categories loaded, only the first one will "
                  "be selected.");
  }

  if (categories.empty()) {
    return nullptr;
  }

  return *categories.begin();
}

void DRCWidget::updateMarkerGroups()
{
  updateMarkerGroupsWithIgnore(nullptr);
}

void DRCWidget::updateMarkerGroupsWithIgnore(odb::dbMarkerCategory* ignore)
{
  if (chip_ == nullptr) {
    return;
  }

  const std::string current_text = categories_->currentText().toStdString();
  const bool remove_current
      = ignore != nullptr && ignore->getName() == current_text;

  categories_->clear();

  categories_->addItem("");
  for (auto* category : chip_->getMarkerCategories()) {
    if (ignore == category) {
      continue;
    }
    categories_->addItem(QString::fromStdString(category->getName()));
  }

  if (remove_current) {
    categories_->setCurrentText("");
  } else {
    categories_->setCurrentText(QString::fromStdString(current_text));
  }
}

void DRCWidget::inDbMarkerCategoryCreate(odb::dbMarkerCategory* category)
{
  updateMarkerGroups();
}

void DRCWidget::inDbMarkerCategoryDestroy(odb::dbMarkerCategory* category)
{
  updateMarkerGroupsWithIgnore(category);
}

void DRCWidget::inDbMarkerCreate(odb::dbMarker* marker)
{
  const std::string name = categories_->currentText().toStdString();
  odb::dbMarkerCategory* category = chip_->findMarkerCategory(name.c_str());

  if (marker->getCategory()->getTopCategory() == category) {
    updateModel();
  }
}

void DRCWidget::inDbMarkerDestroy(odb::dbMarker* marker)
{
  const std::string name = categories_->currentText().toStdString();
  odb::dbMarkerCategory* category = chip_->findMarkerCategory(name.c_str());

  if (marker->getCategory()->getTopCategory() == category) {
    updateModel();
  }
}

////////

DRCRenderer::DRCRenderer() : category_(nullptr)
{
}

void DRCRenderer::drawObjects(Painter& painter)
{
  if (category_ == nullptr) {
    return;
  }

  DbMarkerDescriptor* desc
      = (DbMarkerDescriptor*) Gui::get()->getDescriptor<odb::dbMarker*>();

  Painter::Color pen_color = Painter::kWhite;
  Painter::Color brush_color = pen_color;
  brush_color.a = 50;

  painter.setPen(pen_color, true, 0);
  painter.setBrush(brush_color, Painter::Brush::kDiagonal);
  for (odb::dbMarker* marker : category_->getAllMarkers()) {
    if (!marker->isVisible()) {
      continue;
    }
    desc->paintMarker(marker, painter);
  }
}

SelectionSet DRCRenderer::select(odb::dbTechLayer* layer,
                                 const odb::Rect& region)
{
  if (category_ == nullptr) {
    return SelectionSet();
  }

  if (layer != nullptr) {
    return SelectionSet();
  }

  auto gui = Gui::get();

  SelectionSet selections;
  for (odb::dbMarker* marker : category_->getAllMarkers()) {
    if (!marker->isVisible()) {
      continue;
    }
    if (marker->getBBox().intersects(region)) {
      selections.insert(gui->makeSelected(marker));
    }
  }
  return selections;
}

void DRCRenderer::setCategory(odb::dbMarkerCategory* category)
{
  category_ = category;
  redraw();
}

}  // namespace gui
