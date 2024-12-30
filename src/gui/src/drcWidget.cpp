/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2020, The Regents of the University of California
// All rights reserved.
//
// BSD 3-Clause License
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
//
///////////////////////////////////////////////////////////////////////////////

#include "drcWidget.h"

#include <QApplication>
#include <QFileDialog>
#include <QHeaderView>
#include <QVBoxLayout>
#include <array>
#include <iomanip>
#include <map>

#include "dbDescriptors.h"
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
      block_(nullptr),
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
  if (!block_) {
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

  const bool all_on
      = std::all_of(states.begin(), states.end(), [](bool v) { return v; });
  const bool any_on
      = std::any_of(states.begin(), states.end(), [](bool v) { return v; });

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

void DRCWidget::clicked(const QModelIndex& index)
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
      emit selectDRC(t);
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

void DRCWidget::setBlock(odb::dbBlock* block)
{
  block_ = block;

  addOwner(block_);
  updateMarkerGroups();
}

void DRCWidget::showEvent(QShowEvent* event)
{
  addOwner(block_);

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
  odb::dbMarkerCategory* category = block_->findMarkerCategory(name.c_str());

  model_->removeRows(0, model_->rowCount());

  if (category != nullptr) {
    for (odb::dbMarkerCategory* subcategory : category->getMarkerCategorys()) {
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

  auto makeItem = [](const QString& text) {
    QStandardItem* item = new QStandardItem(text);
    item->setEditable(false);
    item->setSelectable(false);
    return item;
  };

  QStandardItem* type_group = makeItem(category->getName());
  type_group->setCheckable(true);
  type_group->setCheckState(Qt::Checked);

  for (odb::dbMarkerCategory* subcategory : category->getMarkerCategorys()) {
    populateCategory(subcategory, type_group);
  }

  int violation_idx = 1;
  QStandardItem* marker_item_child = nullptr;
  for (odb::dbMarker* marker : category->getMarkers()) {
    QStandardItem* marker_item
        = makeItem(QString::fromStdString(marker->getName()));
    marker_item_child = marker_item;
    marker_item->setSelectable(true);
    marker_item->setData(QVariant::fromValue(marker));
    QStandardItem* marker_index = makeItem(QString::number(violation_idx++));
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
       makeItem(QString::number(category->getMarkerCount()) + " markers")});
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
  } catch (std::runtime_error&) {
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
  return odb::dbMarkerCategory::fromTR(block_, "DRC", file);
}

odb::dbMarkerCategory* DRCWidget::loadJSONReport(const QString& filename)
{
  const std::string file = filename.toStdString();
  const auto categories = odb::dbMarkerCategory::fromJSON(block_, file);

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
  if (block_ == nullptr) {
    return;
  }

  const std::string currentText = categories_->currentText().toStdString();
  const bool remove_current
      = ignore != nullptr && ignore->getName() == currentText;

  categories_->clear();

  categories_->addItem("");
  for (auto* category : block_->getMarkerCategories()) {
    if (ignore == category) {
      continue;
    }
    categories_->addItem(QString::fromStdString(category->getName()));
  }

  if (remove_current) {
    categories_->setCurrentText("");
  } else {
    categories_->setCurrentText(QString::fromStdString(currentText));
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
  odb::dbMarkerCategory* category = block_->findMarkerCategory(name.c_str());

  if (marker->getCategory()->getTopCategory() == category) {
    updateModel();
  }
}

void DRCWidget::inDbMarkerDestroy(odb::dbMarker* marker)
{
  const std::string name = categories_->currentText().toStdString();
  odb::dbMarkerCategory* category = block_->findMarkerCategory(name.c_str());

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

  Painter::Color pen_color = Painter::white;
  Painter::Color brush_color = pen_color;
  brush_color.a = 50;

  painter.setPen(pen_color, true, 0);
  painter.setBrush(brush_color, Painter::Brush::DIAGONAL);
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
