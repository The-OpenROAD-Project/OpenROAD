// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#include "browserWidget.h"

#include <QColor>
#include <QColorDialog>
#include <QEvent>
#include <QHeaderView>
#include <QLocale>
#include <QMenu>
#include <QMouseEvent>
#include <QPushButton>
#include <QSettings>
#include <QString>
#include <QWidget>
#include <algorithm>
#include <any>
#include <cstdint>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "dbDescriptors.h"
#include "db_sta/dbSta.hh"
#include "displayControls.h"
#include "gui/gui.h"
#include "layoutViewer.h"
#include "odb/db.h"
#include "utl/Logger.h"

Q_DECLARE_METATYPE(odb::dbInst*);
Q_DECLARE_METATYPE(odb::dbModule*);
Q_DECLARE_METATYPE(QStandardItem*);

namespace gui {

const int BrowserWidget::kSortRole = Qt::UserRole + 2;

struct BrowserWidget::ModuleStats
{
  int64_t area = 0;
  int macros = 0;
  int insts = 0;
  int modules = 0;

  int hier_macros = 0;
  int hier_insts = 0;
  int hier_modules = 0;

  void incrementInstances()
  {
    insts++;
    hier_insts++;
  }

  void resetInstances() { hier_insts = 0; }

  void incrementMacros()
  {
    macros++;
    hier_macros++;
  }

  void resetMacros() { hier_macros = 0; }

  void incrementModules()
  {
    modules++;
    hier_modules++;
  }

  void resetModules() { hier_modules = 0; }

  ModuleStats& operator+=(const ModuleStats& other)
  {
    area += other.area;
    macros += other.macros;
    insts += other.insts;
    modules += other.modules;

    hier_macros += other.hier_macros;
    hier_insts += other.hier_insts;
    hier_modules += other.hier_modules;

    return *this;
  }

  friend ModuleStats operator+(ModuleStats lhs, const ModuleStats& other)
  {
    lhs += other;

    return lhs;
  }
};

///////

BrowserWidget::BrowserWidget(
    const std::map<odb::dbModule*, LayoutViewer::ModuleSettings>&
        modulesettings,
    DisplayControls* controls,
    QWidget* parent)
    : QDockWidget("Hierarchy Browser", parent),
      block_(nullptr),
      sta_(nullptr),
      inst_descriptor_(nullptr),
      display_controls_(controls),
      display_controls_warning_(
          new QPushButton("Module view is not enabled", this)),
      include_physical_cells_(new QCheckBox("Include physical", this)),
      modulesettings_(modulesettings),
      view_(new QTreeView(this)),
      model_(new QStandardItemModel(this)),
      model_modified_(false),
      initial_load_(true),
      ignore_selection_(false),
      menu_(new QMenu(this))
{
  setObjectName("hierarchy_viewer");  // for settings

  QHBoxLayout* setting_layout = new QHBoxLayout;
  setting_layout->addWidget(display_controls_warning_);
  setting_layout->addStretch();
  setting_layout->addWidget(include_physical_cells_);

  QWidget* widget = new QWidget(this);
  QVBoxLayout* layout = new QVBoxLayout;
  widget->setLayout(layout);
  layout->addLayout(setting_layout);
  layout->addWidget(view_);

  display_controls_warning_->setStyleSheet("color: red;");
  include_physical_cells_->setCheckState(Qt::Checked);

  model_->setHorizontalHeaderLabels({"Instance",
                                     "Module",
                                     "Instances",
                                     "Macros",
                                     "Modules",
                                     "Area",
                                     "Local Instances",
                                     "Local Macros",
                                     "Local Modules"});
  model_->setSortRole(kSortRole);
  view_->setModel(model_);
  view_->setContextMenuPolicy(Qt::CustomContextMenu);

  view_->viewport()->installEventFilter(this);

  makeMenu();

  QHeaderView* header = view_->header();
  header->setSectionsMovable(true);
  header->setStretchLastSection(false);
  header->setSectionResizeMode(kInstance, QHeaderView::Interactive);
  header->setSectionResizeMode(kMaster, QHeaderView::Interactive);
  header->setSectionResizeMode(kInstances, QHeaderView::Interactive);
  header->setSectionResizeMode(kMacros, QHeaderView::Interactive);
  header->setSectionResizeMode(kModules, QHeaderView::Interactive);
  header->setSectionResizeMode(kArea, QHeaderView::Interactive);

  setWidget(widget);

  connect(view_, &QTreeView::clicked, this, &BrowserWidget::clicked);

  connect(view_,
          &QTreeView::customContextMenuRequested,
          this,
          &BrowserWidget::itemContextMenu);

  connect(view_, &QTreeView::collapsed, this, &BrowserWidget::itemCollapsed);
  connect(view_, &QTreeView::expanded, this, &BrowserWidget::itemExpanded);

  connect(view_->selectionModel(),
          &QItemSelectionModel::selectionChanged,
          this,
          &BrowserWidget::selectionChanged);

  connect(model_,
          &QStandardItemModel::itemChanged,
          this,
          &BrowserWidget::itemChanged);

  connect(this,
          &BrowserWidget::updateModuleColor,
          this,
          &BrowserWidget::updateModuleColorIcon);

  connect(display_controls_warning_,
          &QPushButton::pressed,
          this,
          &BrowserWidget::enableModuleView);

  connect(include_physical_cells_,
          &QCheckBox::stateChanged,
          this,
          &BrowserWidget::markModelModified);
}

void BrowserWidget::displayControlsUpdated()
{
  const bool show_warning = !display_controls_->isModuleView();
  if (display_controls_warning_->isEnabled() != show_warning) {
    display_controls_warning_->setEnabled(show_warning);
    display_controls_warning_->setVisible(show_warning);
  }
}

void BrowserWidget::enableModuleView()
{
  display_controls_->setControlByPath("Misc/Module view", true, Qt::Checked);
}

void BrowserWidget::makeMenu()
{
  connect(menu_->addAction("Select"), &QAction::triggered, [&](bool) {
    emit select({menu_item_});
  });
  connect(menu_->addAction("Select children"), &QAction::triggered, [&](bool) {
    emit select(getMenuItemChildren());
  });
  connect(menu_->addAction("Select all"), &QAction::triggered, [&](bool) {
    auto children = getMenuItemChildren();
    children.insert(menu_item_);
    emit select(children);
  });
  connect(menu_->addAction("Remove from selected"),
          &QAction::triggered,
          [&](bool) { emit removeSelect(menu_item_); });

  menu_->addSeparator();

  connect(menu_->addAction("Highlight"), &QAction::triggered, [&](bool) {
    emit highlight({menu_item_});
  });
  connect(menu_->addAction("Highlight children"),
          &QAction::triggered,
          [&](bool) { emit highlight(getMenuItemChildren()); });
  connect(menu_->addAction("Highlight all"), &QAction::triggered, [&](bool) {
    auto children = getMenuItemChildren();
    children.insert(menu_item_);
    emit highlight(children);
  });
  connect(menu_->addAction("Remove from highlight"),
          &QAction::triggered,
          [&](bool) { emit removeHighlight(menu_item_); });

  menu_->addSeparator();

  connect(menu_->addAction("Change color"), &QAction::triggered, [&](bool) {
    auto* module = std::any_cast<odb::dbModule*>(menu_item_.getObject());
    if (module == nullptr) {
      return;
    }

    auto& setting = modulesettings_.at(module);
    QColor color = setting.color;

    color = QColorDialog::getColor(
        color, this, "Module color", QColorDialog::ShowAlphaChannel);
    if (color.isValid()) {
      emit updateModuleColor(module, color, true);
    }
  });
  connect(menu_->addAction("Reset color"), &QAction::triggered, [&](bool) {
    auto* module = std::any_cast<odb::dbModule*>(menu_item_.getObject());
    if (module == nullptr) {
      return;
    }

    auto& setting = modulesettings_.at(module);
    QColor color = setting.orig_color;

    emit updateModuleColor(module, color, true);
  });
}

void BrowserWidget::readSettings(QSettings* settings)
{
  settings->beginGroup(objectName());
  view_->header()->restoreState(
      settings->value("headers", view_->header()->saveState()).toByteArray());
  include_physical_cells_->setCheckState(
      settings->value("include_physical_cells").toBool() ? Qt::Checked
                                                         : Qt::Unchecked);
  settings->endGroup();
}

void BrowserWidget::writeSettings(QSettings* settings)
{
  settings->beginGroup(objectName());
  settings->setValue("headers", view_->header()->saveState());
  settings->setValue("include_physical_cells",
                     include_physical_cells_->isChecked());
  settings->endGroup();
}

void BrowserWidget::setDBInstDescriptor(DbInstDescriptor* desciptor)
{
  inst_descriptor_ = desciptor;
}

Selected BrowserWidget::getSelectedFromIndex(const QModelIndex& index)
{
  QStandardItem* item = model_->itemFromIndex(index);
  QVariant data = item->data();
  if (data.isValid()) {
    auto* ref = data.value<QStandardItem*>();
    if (ref != nullptr) {
      data = ref->data();
    }

    auto* gui = Gui::get();
    auto* inst = data.value<odb::dbInst*>();
    if (inst != nullptr) {
      return gui->makeSelected(inst);
    }
    auto* module = data.value<odb::dbModule*>();
    if (module != nullptr) {
      return gui->makeSelected(module);
    }
  }

  return Selected();
}

void BrowserWidget::selectionChanged(const QItemSelection& selected,
                                     const QItemSelection& deselected)
{
  auto indexes = selected.indexes();
  if (indexes.isEmpty()) {
    return;
  }

  clicked(indexes.first());
}

void BrowserWidget::clicked(const QModelIndex& index)
{
  if (ignore_selection_) {
    return;
  }

  Selected sel = getSelectedFromIndex(index);

  if (sel) {
    emit select({std::move(sel)});
  }
}

void BrowserWidget::setBlock(odb::dbBlock* block)
{
  block_ = block;
  addOwner(block_);
  updateModel();
}

void BrowserWidget::showEvent(QShowEvent* event)
{
  addOwner(block_);
  if (sta_ != nullptr) {
    sta_->getDbNetwork()->addObserver(this);
  }
  updateModel();
}

void BrowserWidget::hideEvent(QHideEvent* event)
{
  removeOwner();
  if (sta_ != nullptr) {
    sta_->getDbNetwork()->removeObserver(this);
  }
  clearModel();
}

void BrowserWidget::paintEvent(QPaintEvent* event)
{
  updateModel();
  QDockWidget::paintEvent(event);
}

void BrowserWidget::markModelModified()
{
  model_modified_ = true;
  emit update();
}

void BrowserWidget::updateModel()
{
  if (!model_modified_ && model_->rowCount() != 0) {
    return;
  }

  if (block_ == nullptr) {
    return;
  }

  setUpdatesEnabled(false);
  clearModel();

  auto* root = model_->invisibleRootItem();
  addModuleItem(block_->getTopModule(), root, true);

  std::vector<odb::dbInst*> insts;
  for (auto* inst : block_->getInsts()) {
    if (!inst->isPhysicalOnly()) {
      continue;
    }

    insts.push_back(inst);
  }
  addInstanceItems(insts, "Physical only", root);

  if (initial_load_) {
    view_->header()->resizeSections(QHeaderView::ResizeToContents);
    initial_load_ = false;
  }
  model_modified_ = false;
  setUpdatesEnabled(true);
  view_->setSortingEnabled(true);
}

void BrowserWidget::clearModel()
{
  model_->removeRows(0, model_->rowCount());
  modulesmap_.clear();
}

BrowserWidget::ModuleStats BrowserWidget::populateModule(odb::dbModule* module,
                                                         QStandardItem* parent)
{
  ModuleStats stats;
  for (auto* child : module->getChildren()) {
    stats += addModuleItem(child->getMaster(), parent, false);
  }
  stats.resetMacros();
  stats.resetInstances();

  std::vector<odb::dbInst*> insts;
  for (auto* inst : module->getInsts()) {
    if (!include_physical_cells_->isChecked()) {
      switch (sta_->getInstanceType(inst)) {
        case sta::dbSta::InstType::ENDCAP:
        case sta::dbSta::InstType::FILL:
        case sta::dbSta::InstType::TAPCELL:
        case sta::dbSta::InstType::STD_PHYSICAL:
        case sta::dbSta::InstType::BUMP:
        case sta::dbSta::InstType::COVER:
        case sta::dbSta::InstType::ANTENNA:
          continue;
        case sta::dbSta::InstType::BLOCK:
        case sta::dbSta::InstType::PAD:
        case sta::dbSta::InstType::PAD_INPUT:
        case sta::dbSta::InstType::PAD_OUTPUT:
        case sta::dbSta::InstType::PAD_INOUT:
        case sta::dbSta::InstType::PAD_POWER:
        case sta::dbSta::InstType::PAD_SPACER:
        case sta::dbSta::InstType::PAD_AREAIO:
        case sta::dbSta::InstType::TIE:
        case sta::dbSta::InstType::LEF_OTHER:
        case sta::dbSta::InstType::STD_CELL:
        case sta::dbSta::InstType::STD_INV:
        case sta::dbSta::InstType::STD_BUF:
        case sta::dbSta::InstType::STD_BUF_CLK_TREE:
        case sta::dbSta::InstType::STD_INV_CLK_TREE:
        case sta::dbSta::InstType::STD_BUF_TIMING_REPAIR:
        case sta::dbSta::InstType::STD_INV_TIMING_REPAIR:
        case sta::dbSta::InstType::STD_CLOCK_GATE:
        case sta::dbSta::InstType::STD_LEVEL_SHIFT:
        case sta::dbSta::InstType::STD_SEQUENTIAL:
        case sta::dbSta::InstType::STD_COMBINATIONAL:
        case sta::dbSta::InstType::STD_OTHER:
          break;
      }
    }
    insts.push_back(inst);
  }
  stats += addInstanceItems(insts, "Leaf instances", parent);

  return stats;
}

BrowserWidget::ModuleStats BrowserWidget::addInstanceItems(
    const std::vector<odb::dbInst*>& insts,
    const std::string& title,
    QStandardItem* parent)
{
  auto make_leaf_item = [](const std::string& title) -> QStandardItem* {
    QStandardItem* leaf = new QStandardItem(QString::fromStdString(title));
    leaf->setEditable(false);
    leaf->setSelectable(false);
    return leaf;
  };

  struct Leaf
  {
    QStandardItem* item = nullptr;
    ModuleStats stats;
  };
  std::map<sta::dbSta::InstType, Leaf> leaf_types;
  for (auto* inst : insts) {
    auto type = sta_->getInstanceType(inst);
    auto& leaf_parent = leaf_types[type];
    if (leaf_parent.item == nullptr) {
      leaf_parent.item = make_leaf_item(sta_->getInstanceTypeText(type));
    }
    const bool create_row = type == sta::dbSta::InstType::BLOCK;
    leaf_parent.stats += addInstanceItem(inst, leaf_parent.item, create_row);
  }

  ModuleStats total;
  if (!leaf_types.empty()) {
    auto* leafs = make_leaf_item(title);
    ModuleStats leaf_stats;
    for (const auto& [type, leaf] : leaf_types) {
      makeRowItems(leaf.item, "", leaf.stats, leafs, true);

      leaf_stats += leaf.stats;
    }
    makeRowItems(leafs, "", leaf_stats, parent, true);

    total += leaf_stats;
  }

  return total;
}

BrowserWidget::ModuleStats BrowserWidget::addInstanceItem(odb::dbInst* inst,
                                                          QStandardItem* parent,
                                                          bool create_row)
{
  auto* box = inst->getBBox();

  ModuleStats stats;
  stats.area = box->getDX() * (int64_t) box->getDY();

  if (inst->isBlock()) {
    stats.incrementMacros();
  } else {
    stats.incrementInstances();
  }

  if (create_row) {
    QStandardItem* item = new QStandardItem(inst->getConstName());
    item->setEditable(false);
    item->setSelectable(true);
    item->setData(QVariant::fromValue(inst));
    item->setData(inst->getConstName(), kSortRole);

    makeRowItems(item, inst->getMaster()->getConstName(), stats, parent, true);
  }

  return stats;
}

BrowserWidget::ModuleStats BrowserWidget::addModuleItem(odb::dbModule* module,
                                                        QStandardItem* parent,
                                                        bool expand)
{
  auto* inst = module->getModInst();
  QString item_name;
  if (inst == nullptr) {
    item_name = QString::fromStdString(module->getHierarchicalName());
  } else {
    item_name = QString::fromStdString(inst->getName());
  }
  QStandardItem* item = new QStandardItem(item_name);
  item->setEditable(false);
  item->setSelectable(true);
  item->setData(QVariant::fromValue(module));
  item->setData(item_name, kSortRole);

  item->setCheckable(true);
  auto& settings = modulesettings_.at(module);
  item->setCheckState(settings.visible ? Qt::Checked : Qt::Unchecked);
  item->setIcon(makeModuleIcon(settings.color));

  modulesmap_[module] = item;

  ModuleStats stats = populateModule(module, item);

  makeRowItems(item, module->getName(), stats, parent, false);
  stats.resetModules();

  stats.incrementModules();

  if (expand) {
    view_->expand(item->index());
  }

  return stats;
}

void BrowserWidget::makeRowItems(QStandardItem* item,
                                 const std::string& master,
                                 const BrowserWidget::ModuleStats& stats,
                                 QStandardItem* parent,
                                 bool is_leaf) const
{
  double scale_to_um
      = block_->getDbUnitsPerMicron() * block_->getDbUnitsPerMicron();

  QString units = "μ";
  double disp_area = stats.area / scale_to_um;
  if (disp_area > 1e6) {
    disp_area /= (1e3 * 1e3);
    units = "m";
  }

  QString text = QString::number(disp_area, 'f', 3) + " " + units + "m²";

  auto make_data_item
      = [item](const QString& text,
               std::optional<int64_t> sort_value) -> QStandardItem* {
    QStandardItem* data_item = new QStandardItem(text);
    data_item->setEditable(false);
    data_item->setData(QVariant::fromValue(item));
    if (sort_value) {
      data_item->setData(qint64(sort_value.value()), kSortRole);
      data_item->setData(Qt::AlignRight, Qt::TextAlignmentRole);
    } else {
      data_item->setData(text, kSortRole);
    }
    return data_item;
  };

  QStandardItem* master_item
      = make_data_item(QString::fromStdString(master), {});

  QStandardItem* area = make_data_item(text, stats.area);

  QStandardItem* local_insts
      = make_data_item(QString::number(stats.hier_insts), stats.hier_insts);
  QStandardItem* insts
      = make_data_item(QString::number(stats.insts), stats.insts);

  QStandardItem* local_macros
      = make_data_item(QString::number(stats.hier_macros), stats.hier_macros);
  QStandardItem* macros
      = make_data_item(QString::number(stats.macros), stats.macros);

  QStandardItem* modules
      = make_data_item(QString::number(stats.hier_modules), stats.hier_modules);
  QStandardItem* local_modules
      = make_data_item(QString::number(stats.modules), stats.modules);

  parent->appendRow({item,
                     master_item,
                     insts,
                     macros,
                     modules,
                     area,
                     local_insts,
                     local_macros,
                     local_modules});
}

void BrowserWidget::inDbInstCreate(odb::dbInst*)
{
  markModelModified();
}

void BrowserWidget::inDbInstCreate(odb::dbInst*, odb::dbRegion*)
{
  markModelModified();
}

void BrowserWidget::inDbInstDestroy(odb::dbInst*)
{
  markModelModified();
}

void BrowserWidget::inDbInstSwapMasterAfter(odb::dbInst*)
{
  markModelModified();
}

void BrowserWidget::itemContextMenu(const QPoint& point)
{
  const QModelIndex index = view_->indexAt(point);

  if (!index.isValid()) {
    return;
  }

  menu_item_ = getSelectedFromIndex(index);

  if (!menu_item_) {
    return;
  }

  menu_->popup(view_->viewport()->mapToGlobal(point));
}

SelectionSet BrowserWidget::getMenuItemChildren()
{
  if (!menu_item_) {
    return {};
  }

  auto* module = std::any_cast<odb::dbModule*>(menu_item_.getObject());
  if (module == nullptr) {
    return {};
  }

  auto* gui = Gui::get();
  SelectionSet children;
  for (auto* child : getChildren(module)) {
    children.insert(gui->makeSelected(child));
  }
  return children;
}

void BrowserWidget::itemChanged(QStandardItem* item)
{
  ignore_selection_ = true;
  const auto state = item->checkState();

  auto* module = item->data().value<odb::dbModule*>();

  if (module == nullptr) {
    return;
  }

  emit updateModuleVisibility(module, state == Qt::Checked);

  // toggle children
  if (state != Qt::PartiallyChecked) {
    for (int r = 0; r < item->rowCount(); r++) {
      QStandardItem* child = item->child(r, kInstance);
      if (child->isCheckable()) {
        child->setCheckState(state);
      }
    }
  }

  toggleParent(item);
}

void BrowserWidget::toggleParent(QStandardItem* item)
{
  auto* parent = item->parent();

  if (parent == nullptr) {
    return;
  }

  std::vector<Qt::CheckState> childstates;
  for (int r = 0; r < parent->rowCount(); r++) {
    QStandardItem* child = parent->child(r, kInstance);
    if (child->isCheckable()) {
      childstates.push_back(child->checkState());
    }
  }

  const bool all_on = std::ranges::all_of(
      childstates, [](Qt::CheckState state) { return state == Qt::Checked; });
  const bool all_off = std::ranges::all_of(
      childstates, [](Qt::CheckState state) { return state == Qt::Unchecked; });

  if (all_on) {
    parent->setCheckState(Qt::Checked);
  } else if (all_off) {
    parent->setCheckState(Qt::Unchecked);
  } else {
    parent->setCheckState(Qt::PartiallyChecked);
  }
}

bool BrowserWidget::eventFilter(QObject* obj, QEvent* event)
{
  if (obj == view_->viewport()) {
    if (event->type() == QEvent::MouseButtonPress) {
      QMouseEvent* mouse_event = static_cast<QMouseEvent*>(event);
      if (mouse_event->button() == Qt::RightButton) {
        ignore_selection_ = true;
      } else {
        ignore_selection_ = false;
      }
    } else if (event->type() == QEvent::MouseButtonRelease) {
      ignore_selection_ = false;
    } else if (event->type() == QEvent::ContextMenu) {
      // reset because the context menu has popped up.
      ignore_selection_ = false;
    }
  }

  return QDockWidget::eventFilter(obj, event);
}

QIcon BrowserWidget::makeModuleIcon(const QColor& color)
{
  QPixmap swatch(20, 20);
  swatch.fill(color);

  return QIcon(swatch);
}

void BrowserWidget::itemCollapsed(const QModelIndex& index)
{
  auto* item = model_->itemFromIndex(index);
  auto* module = item->data().value<odb::dbModule*>();

  if (module == nullptr) {
    return;
  }

  updateChildren(module, modulesettings_.at(module).color);
}

void BrowserWidget::updateChildren(odb::dbModule* module, const QColor& color)
{
  for (auto* child : getAllChildren(module)) {
    emit updateModuleColor(child, color, false);
  }
}

void BrowserWidget::itemExpanded(const QModelIndex& index)
{
  auto* item = model_->itemFromIndex(index);
  auto* module = item->data().value<odb::dbModule*>();

  if (module == nullptr) {
    return;
  }

  resetChildren(module);
}

void BrowserWidget::resetChildren(odb::dbModule* module)
{
  for (auto* child : getChildren(module)) {
    const QColor& base_color = modulesettings_.at(child).user_color;
    emit updateModuleColor(child, base_color, false);

    if (view_->isExpanded(modulesmap_[child]->index())) {
      resetChildren(child);
    } else {
      updateChildren(child, base_color);
    }
  }
}

void BrowserWidget::updateModuleColorIcon(odb::dbModule* module,
                                          const QColor& color)
{
  auto* item = modulesmap_[module];
  item->setIcon(makeModuleIcon(color));

  if (!view_->isExpanded(item->index())) {
    updateChildren(module, color);
  }
}

std::set<odb::dbModule*> BrowserWidget::getChildren(odb::dbModule* parent)
{
  std::set<odb::dbModule*> children;
  for (auto* child : parent->getChildren()) {
    children.insert(child->getMaster());
  }
  return children;
}

std::set<odb::dbModule*> BrowserWidget::getAllChildren(odb::dbModule* parent)
{
  std::set<odb::dbModule*> children;
  for (auto* child : getChildren(parent)) {
    children.insert(child);
    const auto next_children = getAllChildren(child);
    children.insert(next_children.begin(), next_children.end());
  }
  return children;
}

void BrowserWidget::setSTA(sta::dbSta* sta)
{
  sta_ = sta;
  sta_->getDbNetwork()->addObserver(this);
  markModelModified();
}

void BrowserWidget::postReadLiberty()
{
  markModelModified();
}

void BrowserWidget::postReadDb()
{
  markModelModified();
}

}  // namespace gui
