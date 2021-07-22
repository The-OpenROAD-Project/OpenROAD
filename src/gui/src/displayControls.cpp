///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2019, The Regents of the University of California
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

#include <QDebug>
#include <QHeaderView>
#include <QKeyEvent>
#include <QLineEdit>
#include <QPainter>
#include <QSettings>
#include <QVBoxLayout>
#include <vector>

#include "db.h"
#include "displayControls.h"
#include "ord/InitOpenRoad.hh"

Q_DECLARE_METATYPE(odb::dbTechLayer*);

namespace gui {

using namespace odb;

PatternButton::PatternButton(Qt::BrushStyle pattern, QWidget* parent)
    : QRadioButton(parent), pattern_(pattern)
{
  setFixedWidth(100);
}

void PatternButton::paintEvent(QPaintEvent* event)
{
  QRadioButton::paintEvent(event);
  auto qp = QPainter(this);
  auto brush = QBrush(QColor("black"), pattern_);
  qp.setBrush(brush);
  auto button_rect = rect();
  button_rect.adjust(18, 2, -1, -1);
  qp.drawRect(button_rect);
  qp.end();
}

DisplayColorDialog::DisplayColorDialog(QColor color,
                                       Qt::BrushStyle pattern,
                                       QWidget* parent)
    : QDialog(parent), color_(color), pattern_(pattern)
{
  buildUI();
}

void DisplayColorDialog::buildUI()
{
  color_dialog_ = new QColorDialog(this);
  color_dialog_->setOptions(QColorDialog::DontUseNativeDialog);
  color_dialog_->setOption(QColorDialog::ShowAlphaChannel);
  color_dialog_->setWindowFlags(Qt::Widget);
  color_dialog_->setCurrentColor(color_);
  pattern_group_box_ = new QGroupBox("Layer Pattern");
  grid_layout_ = new QGridLayout();

  grid_layout_->setColumnStretch(2, 4);

  int row_index = 0;
  for (auto& pattern_group : DisplayColorDialog::brush_patterns_) {
    int col_index = 0;
    for (auto pattern : pattern_group) {
      PatternButton* pattern_button = new PatternButton(pattern);
      pattern_buttons_.push_back(pattern_button);
      if (pattern == pattern_)
        pattern_button->setChecked(true);
      else
        pattern_button->setChecked(false);
      grid_layout_->addWidget(pattern_button, row_index, col_index);
      ++col_index;
    }
    ++row_index;
  }
  pattern_group_box_->setLayout(grid_layout_);
  connect(color_dialog_, SIGNAL(accepted()), this, SLOT(acceptDialog()));
  connect(color_dialog_, SIGNAL(rejected()), this, SLOT(rejectDialog()));

  main_layout_ = new QVBoxLayout();
  main_layout_->addWidget(pattern_group_box_);
  main_layout_->addWidget(color_dialog_);

  setLayout(main_layout_);
  setWindowTitle("Layer Config");
  setFixedSize(600, width() - 20);
}

DisplayColorDialog::~DisplayColorDialog()
{
}

Qt::BrushStyle DisplayColorDialog::getSelectedPattern() const
{
  for (auto pattern_button : pattern_buttons_) {
    if (pattern_button->isChecked())
      return pattern_button->pattern();
  }
  return Qt::SolidPattern;
}

void DisplayColorDialog::acceptDialog()
{
  color_ = color_dialog_->selectedColor();
  accept();
}

void DisplayColorDialog::rejectDialog()
{
  reject();
}

DisplayControls::DisplayControls(QWidget* parent)
    : QDockWidget("Display Control", parent),
      view_(new QTreeView(parent)),
      model_(new QStandardItemModel(0, 4, parent)),
      ignore_callback_(false),
      tech_inited_(false)
{
  setObjectName("layers");  // for settings
  model_->setHorizontalHeaderLabels({"", "C", "V", "S"});
  view_->setModel(model_);

  QHeaderView* header = view_->header();
  header->setSectionResizeMode(Name, QHeaderView::Stretch);
  header->setSectionResizeMode(Swatch, QHeaderView::ResizeToContents);
  header->setSectionResizeMode(Visible, QHeaderView::ResizeToContents);
  header->setSectionResizeMode(Selectable, QHeaderView::ResizeToContents);

  auto layers = makeItem(
      layers_group_,
      "Layers",
      model_,
      Qt::Checked,
      [this](bool visible) {
        toggleAllChildren(visible, layers_group_.name, Visible);
      },
      [this](bool selectable) {
        toggleAllChildren(selectable, layers_group_.name, Selectable);
      });
  view_->expand(layers->index());

  // Nets group
  auto nets_parent = makeItem(
      nets_group_,
      "Nets", model_,
      Qt::Checked,
      [this](bool visible) {
        toggleAllChildren(visible, nets_group_.name, Visible);
      },
      [this](bool selectable) {
        toggleAllChildren(selectable, nets_group_.name, Selectable);
      });

  // make net items, non-null last argument to create checkbox
  makeItem(nets_.signal, "Signal", nets_parent, Qt::Checked, std::function<void(bool)>(), [](bool) {});
  makeItem(nets_.power, "Power", nets_parent, Qt::Checked, std::function<void(bool)>(), [](bool) {});
  makeItem(nets_.ground, "Ground", nets_parent, Qt::Checked, std::function<void(bool)>(), [](bool) {});
  makeItem(nets_.clock, "Clock", nets_parent, Qt::Checked, std::function<void(bool)>(), [](bool) {});

  // Instance group
  auto instances_parent = makeItem(
      instance_group_,
      "Instances",
      model_,
      Qt::Checked,
      [this](bool visible) {
        toggleAllChildren(visible, instance_group_.name, Visible);
      },
      [this](bool selectable) {
        toggleAllChildren(selectable, instance_group_.name, Selectable);
      });

  // make instance items, non-null last argument to create checkbox
  makeItem(instances_.core, "StdCells", instances_parent, Qt::Checked, std::function<void(bool)>(), [](bool){});
  makeItem(instances_.blocks, "Macros", instances_parent, Qt::Checked, std::function<void(bool)>(), [](bool){});
  makeItem(instances_.fill, "Fill", instances_parent, Qt::Checked, std::function<void(bool)>(), [](bool){});
  makeItem(instances_.endcap, "Endcap", instances_parent, Qt::Checked, std::function<void(bool)>(), [](bool){});
  makeItem(instances_.pads, "Pads", instances_parent, Qt::Checked, std::function<void(bool)>(), [](bool){});
  makeItem(instances_.cover, "Cover", instances_parent, Qt::Checked, std::function<void(bool)>(), [](bool){});

  // Rows
  makeItem(rows_, "Rows", model_, Qt::Unchecked);

  // Rows
  makeItem(congestion_map_, "Congestion Map", model_, Qt::Unchecked);
  makeItem(pin_markers_, "Pin Markers", model_, Qt::Checked);

  // Track patterns group
  auto tracks = makeItem(
      tracks_group_, "Tracks", model_, Qt::Unchecked, [this](bool visible) {
        toggleAllChildren(visible, tracks_group_.name, Visible);
      });

  makeItem(tracks_.pref, "Pref", tracks, Qt::Unchecked);
  makeItem(tracks_.non_pref, "Non Pref", tracks, Qt::Unchecked);

  // Misc group
  auto misc = makeItem(
      misc_group_, "Misc", model_, Qt::Unchecked, [this](bool visible) {
        toggleAllChildren(visible, misc_group_.name, Visible);
      });

  makeItem(misc_.fills, "Fills", misc, Qt::Unchecked);

  setWidget(view_);
  connect(model_,
          SIGNAL(itemChanged(QStandardItem*)),
          this,
          SLOT(itemChanged(QStandardItem*)));

  connect(view_,
          SIGNAL(doubleClicked(const QModelIndex&)),
          this,
          SLOT(displayItemDblClicked(const QModelIndex&)));
  setMinimumWidth(375);
  congestion_dialog_ = new CongestionSetupDialog(this);

  connect(congestion_dialog_,
          SIGNAL(applyCongestionRequested()),
          this,
          SIGNAL(changed()));
  connect(congestion_dialog_,
          SIGNAL(congestionSetupChanged()),
          this,
          SIGNAL(changed()));
}

void DisplayControls::readSettings(QSettings* settings)
{
  auto getChecked = [](QSettings* settings, const char* name) {
    return settings->value(name).toBool() ? Qt::Checked : Qt::Unchecked;
  };

  settings->beginGroup("display_controls");

  settings->beginGroup("nets");
  nets_.signal.visible->setCheckState(getChecked(settings, "signal_visible"));
  nets_.signal.selectable->setCheckState(getChecked(settings, "signal_selectable"));
  nets_.power.visible->setCheckState(getChecked(settings, "power_visible"));
  nets_.power.selectable->setCheckState(getChecked(settings, "power_selectable"));
  nets_.ground.visible->setCheckState(getChecked(settings, "ground_visible"));
  nets_.ground.selectable->setCheckState(getChecked(settings, "ground_selectable"));
  nets_.clock.visible->setCheckState(getChecked(settings, "clock_visible"));
  nets_.clock.selectable->setCheckState(getChecked(settings, "clock_selectable"));
  settings->endGroup();  // nets

  settings->beginGroup("instances");
  instances_.core.visible->setCheckState(getChecked(settings, "stdcell_visible"));
  instances_.core.selectable->setCheckState(getChecked(settings, "stdcell_selectable"));
  instances_.blocks.visible->setCheckState(getChecked(settings, "blocks_visible"));
  instances_.blocks.selectable->setCheckState(getChecked(settings, "blocks_selectable"));
  instances_.fill.visible->setCheckState(getChecked(settings, "fill_visible"));
  instances_.fill.selectable->setCheckState(getChecked(settings, "fill_selectable"));
  instances_.endcap.visible->setCheckState(getChecked(settings, "endcap_visible"));
  instances_.endcap.selectable->setCheckState(getChecked(settings, "endcap_selectable"));
  instances_.pads.visible->setCheckState(getChecked(settings, "pads_visible"));
  instances_.pads.selectable->setCheckState(getChecked(settings, "pads_selectable"));
  instances_.cover.visible->setCheckState(getChecked(settings, "cover_visible"));
  instances_.cover.selectable->setCheckState(getChecked(settings, "cover_selectable"));
  settings->endGroup();  // nets

  rows_.visible->setCheckState(getChecked(settings, "rows_visible"));
  congestion_map_.visible->setCheckState(
      getChecked(settings, "congestion_map_visible"));
  pin_markers_.visible->setCheckState(
      getChecked(settings, "pin_markers_visible"));

  settings->beginGroup("tracks");
  tracks_.pref.visible->setCheckState(getChecked(settings, "pref_visible"));
  tracks_.non_pref.visible->setCheckState(
      getChecked(settings, "non_pref_visible"));
  settings->endGroup();  // tracks

  settings->beginGroup("misc");
  misc_.fills.visible->setCheckState(getChecked(settings, "fills_visible"));
  settings->endGroup();  // misc

  settings->endGroup();
}

void DisplayControls::writeSettings(QSettings* settings)
{
  auto asBool
      = [](QStandardItem* item) { return item->checkState() == Qt::Checked; };

  settings->beginGroup("display_controls");

  settings->beginGroup("nets");
  settings->setValue("signal_visible", asBool(nets_.signal.visible));
  settings->setValue("signal_selectable", asBool(nets_.signal.selectable));
  settings->setValue("power_visible", asBool(nets_.power.visible));
  settings->setValue("power_selectable", asBool(nets_.power.selectable));
  settings->setValue("ground_visible", asBool(nets_.ground.visible));
  settings->setValue("ground_selectable", asBool(nets_.ground.selectable));
  settings->setValue("clock_visible", asBool(nets_.clock.visible));
  settings->setValue("clock_selectable", asBool(nets_.clock.selectable));
  settings->endGroup();  // nets

  settings->beginGroup("instances");
  settings->setValue("stdcell_visible", asBool(instances_.core.visible));
  settings->setValue("stdcell_selectable", asBool(instances_.core.selectable));
  settings->setValue("blocks_visible", asBool(instances_.blocks.visible));
  settings->setValue("blocks_selectable", asBool(instances_.blocks.selectable));
  settings->setValue("fill_visible", asBool(instances_.fill.visible));
  settings->setValue("fill_selectable", asBool(instances_.fill.selectable));
  settings->setValue("endcap_visible", asBool(instances_.endcap.visible));
  settings->setValue("endcap_selectable", asBool(instances_.endcap.selectable));
  settings->setValue("pads_visible", asBool(instances_.pads.visible));
  settings->setValue("pads_selectable", asBool(instances_.pads.selectable));
  settings->setValue("cover_visible", asBool(instances_.cover.visible));
  settings->setValue("cover_selectable", asBool(instances_.cover.selectable));
  settings->endGroup();  // instances

  settings->setValue("rows_visible", asBool(rows_.visible));
  settings->setValue("congestion_map_visible", asBool(congestion_map_.visible));
  settings->setValue("pin_markers_visible", asBool(pin_markers_.visible));

  settings->beginGroup("tracks");
  settings->setValue("pref_visible", asBool(tracks_.pref.visible));
  settings->setValue("non_pref_visible", asBool(tracks_.non_pref.visible));
  settings->endGroup();  // tracks

  settings->beginGroup("misc");
  settings->setValue("fills_visible", asBool(misc_.fills.visible));
  settings->endGroup();  // misc

  settings->endGroup();
}

void DisplayControls::toggleAllChildren(bool checked,
                                        QStandardItem* parent,
                                        Column column)
{
  Qt::CheckState state = checked ? Qt::Checked : Qt::Unchecked;
  for (int row = 0; row < parent->rowCount(); ++row) {
    auto child = parent->child(row, column);
    child->setCheckState(state);
  }
  emit changed();
}

void DisplayControls::toggleParent(const QStandardItem* parent,
                                   QStandardItem* parent_flag,
                                   int column)
{
  if (parent == nullptr || parent_flag == nullptr) {
    return;
  }

  bool at_least_one_checked = false;
  bool all_checked = true;

  for (int row = 0; row < parent->rowCount(); ++row) {
    bool checked = parent->child(row, column)->checkState() == Qt::Checked;
    at_least_one_checked |= checked;
    all_checked &= checked;
  }

  ignore_callback_ = true;
  Qt::CheckState new_state;
  if (all_checked) {
    new_state = Qt::Checked;
  }
  else if (at_least_one_checked) {
    new_state = Qt::PartiallyChecked;
  }
  else {
    new_state = Qt::Unchecked;
  }
  parent_flag->setCheckState(new_state);
  ignore_callback_ = false;
}

void DisplayControls::itemChanged(QStandardItem* item)
{
  if (item->isCheckable() == false) {
    emit changed();
    return;
  }
  bool checked = item->checkState() == Qt::Checked;
  Callback callback = item->data().value<Callback>();
  if (callback.action && !ignore_callback_) {
    callback.action(checked);
  }
  QModelIndex item_index = item->index();
  QModelIndex parent_index = item_index.parent();
  if (parent_index.isValid()) {
    toggleParent(model_->item(parent_index.row(), 0), // parent row
        model_->item(parent_index.row(), item_index.column()), // selected column
        item_index.column());
  }
  // disable selectable column if visible is unchecked
  if (item_index.column() == Visible) {
    QStandardItem* selectable = nullptr;
    if (!parent_index.isValid()) {
      selectable = model_->item(item_index.row(), Selectable);
    } else {
      if (item->parent() != nullptr) {
        selectable = item->parent()->child(item_index.row(), Selectable);
      }
    }

    if (selectable != nullptr) {
      selectable->setEnabled(item->checkState() != Qt::Unchecked);
    }
  }
  emit changed();
}

void DisplayControls::displayItemDblClicked(const QModelIndex& index)
{
  if (index.column() == 1) {
    auto color_item = model_->itemFromIndex(index);
    QVariant tech_layer_data = color_item->data(Qt::UserRole);
    if (!tech_layer_data.isValid())
      return;
    auto tech_layer
        = static_cast<odb::dbTechLayer*>(tech_layer_data.value<void*>());
    if (tech_layer == nullptr)
      return;
    QColor color_val = color(tech_layer);
    Qt::BrushStyle pattern_val = pattern(tech_layer);
    DisplayColorDialog display_dialog(color_val, pattern_val);
    display_dialog.exec();
    QColor chosen_color = display_dialog.getSelectedColor();
    if (chosen_color.isValid()) {
      QPixmap swatch(20, 20);
      swatch.fill(chosen_color);
      color_item->setIcon(QIcon(swatch));
      auto cut_layer_index
          = model_->sibling(index.row() + 1, index.column(), index);
      if (cut_layer_index.isValid()) {
        auto cut_color_item = model_->itemFromIndex(cut_layer_index);
        cut_color_item->setIcon(QIcon(swatch));
      }
      if (chosen_color != color_val
          || layer_pattern_[tech_layer]
                 != display_dialog.getSelectedPattern()) {
        layer_color_[tech_layer] = chosen_color;
        layer_pattern_[tech_layer] = display_dialog.getSelectedPattern();
        view_->repaint();
        emit changed();
      }
    }
  }
}

void DisplayControls::setDb(odb::dbDatabase* db)
{
  db_ = db;
  if (!db) {
    return;
  }

  dbTech* tech = db->getTech();
  if (!tech) {
    return;
  }

  techInit();

  for (dbTechLayer* layer : tech->getLayers()) {
    dbTechLayerType type = layer->getType();
    if (type == dbTechLayerType::ROUTING || type == dbTechLayerType::CUT) {
      makeItem(
          layer_controls_[layer],
          QString::fromStdString(layer->getName()),
          layers_group_.name,
          Qt::Checked,
          std::function<void(bool)>(),
          [this](bool selectable) {},  // non-null to create checkbox
          color(layer),
          type == dbTechLayerType::CUT ? NULL : layer);
    }
  }

  for (int i = 0; i < 4; i++)
    view_->resizeColumnToContents(i);
  emit changed();
}

template <typename T>
QStandardItem* DisplayControls::makeItem(
    ModelRow& row,
    const QString& text,
    T* parent,
    Qt::CheckState checked,
    const std::function<void(bool)>& visibility_action,
    const std::function<void(bool)>& select_action,
    const QColor& color,
    odb::dbTechLayer* tech_layer)
{
  row.name = new QStandardItem(text);
  row.name->setEditable(false);

  QPixmap swatch(20, 20);
  swatch.fill(color);
  row.swatch = new QStandardItem(QIcon(swatch), "");
  row.swatch->setEditable(false);
  row.swatch->setCheckable(false);
  if (tech_layer != nullptr) {
    QVariant tech_layer_data(
        QVariant::fromValue(static_cast<void*>(tech_layer)));
    row.swatch->setData(tech_layer_data, Qt::UserRole);
  }

  row.visible = new QStandardItem("");
  row.visible->setCheckable(true);
  row.visible->setEditable(false);
  row.visible->setCheckState(checked);
  row.visible->setData(QVariant::fromValue(Callback({visibility_action})));

  if (select_action) {
    row.selectable = new QStandardItem("");
    row.selectable->setCheckable(true);
    row.selectable->setEditable(false);
    row.selectable->setCheckState(checked);
    row.selectable->setData(QVariant::fromValue(Callback({select_action})));
  }

  parent->appendRow({row.name, row.swatch, row.visible, row.selectable});
  return row.name;
}

QColor DisplayControls::color(const odb::dbTechLayer* layer)
{
  return layer_color_.at(layer);
}

Qt::BrushStyle DisplayControls::pattern(const odb::dbTechLayer* layer)
{
  return layer_pattern_.at(layer);
}

bool DisplayControls::isVisible(const odb::dbTechLayer* layer)
{
  auto it = layer_controls_.find(layer);
  if (it != layer_controls_.end()) {
    return it->second.visible->checkState() == Qt::Checked;
  }
  return false;
}

bool DisplayControls::isInstanceVisible(odb::dbInst* inst)
{
  dbMaster* master = inst->getMaster();
  if (master->isEndCap()) {
    return instances_.endcap.visible->checkState() == Qt::Checked;
  } else if (master->isFiller()) {
    return instances_.fill.visible->checkState() == Qt::Checked;
  } else if (master->isCore()) {
    return instances_.core.visible->checkState() == Qt::Checked;
  } else if (master->isBlock()) {
    return instances_.blocks.visible->checkState() == Qt::Checked;
  } else if (master->isPad()) {
    return instances_.pads.visible->checkState() == Qt::Checked;
  } else if (master->getType() == dbMasterType::COVER || master->getType() == dbMasterType::COVER_BUMP) {
    return instances_.cover.visible->checkState() == Qt::Checked;
  } else {
    return true;
  }
}

bool DisplayControls::isInstanceSelectable(odb::dbInst* inst)
{
  dbMaster* master = inst->getMaster();
  if (master->isEndCap()) {
    return instances_.endcap.selectable->checkState() == Qt::Checked;
  } else if (master->isFiller()) {
    return instances_.fill.selectable->checkState() == Qt::Checked;
  } else if (master->isCore()) {
    return instances_.core.selectable->checkState() == Qt::Checked;
  } else if (master->isBlock()) {
    return instances_.blocks.selectable->checkState() == Qt::Checked;
  } else if (master->isPad()) {
    return instances_.pads.selectable->checkState() == Qt::Checked;
  } else if (master->getType() == dbMasterType::COVER || master->getType() == dbMasterType::COVER_BUMP) {
    return instances_.cover.selectable->checkState() == Qt::Checked;
  } else {
    return true;
  }
}

bool DisplayControls::isNetVisible(odb::dbNet* net)
{
  switch (net->getSigType()) {
    case dbSigType::SIGNAL:
      return nets_.signal.visible->checkState() == Qt::Checked;
    case dbSigType::POWER:
      return nets_.power.visible->checkState() == Qt::Checked;
    case dbSigType::GROUND:
      return nets_.ground.visible->checkState() == Qt::Checked;
    case dbSigType::CLOCK:
      return nets_.clock.visible->checkState() == Qt::Checked;
    default:
      return true;
  }
}

bool DisplayControls::isNetSelectable(odb::dbNet* net)
{
  switch (net->getSigType()) {
    case dbSigType::SIGNAL:
      return nets_.signal.selectable->checkState() == Qt::Checked;
    case dbSigType::POWER:
      return nets_.power.selectable->checkState() == Qt::Checked;
    case dbSigType::GROUND:
      return nets_.ground.selectable->checkState() == Qt::Checked;
    case dbSigType::CLOCK:
      return nets_.clock.selectable->checkState() == Qt::Checked;
    default:
      return true;
  }
}

bool DisplayControls::isSelectable(const odb::dbTechLayer* layer)
{
  auto it = layer_controls_.find(layer);
  if (it != layer_controls_.end()) {
    return it->second.selectable->checkState() == Qt::Checked;
  }
  return false;
}

bool DisplayControls::areFillsVisible()
{
  return misc_.fills.visible->checkState() == Qt::Checked;
}

bool DisplayControls::areRowsVisible()
{
  return rows_.visible->checkState() == Qt::Checked;
}

bool DisplayControls::arePrefTracksVisible()
{
  return tracks_.pref.visible->checkState() == Qt::Checked;
}

bool DisplayControls::areNonPrefTracksVisible()
{
  return tracks_.non_pref.visible->checkState() == Qt::Checked;
}

bool DisplayControls::isCongestionVisible() const
{
  return congestion_map_.visible->checkState() == Qt::Checked;
}

bool DisplayControls::arePinMarkersVisible() const
{
  return pin_markers_.visible->checkState() == Qt::Checked;
}

void DisplayControls::addCustomVisibilityControl(const std::string& name,
                                                 bool initially_visible)
{
  auto q_name = QString::fromStdString(name);
  auto checked = initially_visible ? Qt::Checked : Qt::Unchecked;
  makeItem(custom_controls_[name],
           q_name,
           model_,
           checked,
           [this, name](bool visible) {
             custom_controls_[name].visible->setCheckState(
                 visible ? Qt::Checked : Qt::Unchecked);
           });
}

bool DisplayControls::checkCustomVisibilityControl(const std::string& name)
{
  return custom_controls_[name].visible->checkState() == Qt::Checked;
}

bool DisplayControls::showHorizontalCongestion() const
{
  return congestion_dialog_->showHorizontalCongestion()
         || !congestion_dialog_->showVerticalCongestion();
}

bool DisplayControls::showVerticalCongestion() const
{
  return congestion_dialog_->showVerticalCongestion()
         || !congestion_dialog_->showHorizontalCongestion();
}

float DisplayControls::getMinCongestionToShow() const
{
  return congestion_dialog_->getMinCongestionValue();
}

float DisplayControls::getMaxCongestionToShow() const
{
  return congestion_dialog_->getMaxCongestionValue();
}

QColor DisplayControls::getCongestionColor(float congestion) const
{
  return congestion_dialog_->getCongestionColorForPercentage(congestion);
}

void DisplayControls::showCongestionSetup()
{
  return congestion_dialog_->show();
}

void DisplayControls::techInit()
{
  if (tech_inited_ || !db_) {
    return;
  }

  dbTech* tech = db_->getTech();
  if (!tech) {
    return;
  }

  // Default colors
  // From http://vrl.cs.brown.edu/color seeded with #00F, #F00, #0D0
  const QColor colors[] = {QColor(0, 0, 254),
                           QColor(254, 0, 0),
                           QColor(9, 221, 0),
                           QColor(190, 244, 81),
                           QColor(159, 24, 69),
                           QColor(32, 216, 253),
                           QColor(253, 108, 160),
                           QColor(117, 63, 194),
                           QColor(128, 155, 49),
                           QColor(234, 63, 252),
                           QColor(9, 96, 19),
                           QColor(214, 120, 239),
                           QColor(192, 222, 164),
                           QColor(110, 68, 107)};
  const int num_colors = sizeof(colors) / sizeof(QColor);
  int metal = 0;
  int via = 0;

  // Iterate through the layers and set default colors
  for (dbTechLayer* layer : tech->getLayers()) {
    dbTechLayerType type = layer->getType();
    QColor color;
    if (type == dbTechLayerType::ROUTING) {
      if (metal < num_colors) {
        color = colors[metal++];
      } else {
        // pick a random color as we exceeded the built-in palette size
        color = QColor(50 + rand() % 200, 50 + rand() % 200, 50 + rand() % 200);
      }
    } else if (type == dbTechLayerType::CUT) {
      if (via < num_colors) {
        color = colors[via++];
      } else {
        // pick a random color as we exceeded the built-in palette size
        color = QColor(50 + rand() % 200, 50 + rand() % 200, 50 + rand() % 200);
      }
    } else {
      continue;
    }
    color.setAlpha(180);
    layer_color_[layer] = color;
    layer_pattern_[layer] = Qt::SolidPattern;  // Default pattern is fill solid
  }

  tech_inited_ = true;
}

void DisplayControls::designLoaded(odb::dbBlock* block)
{
  setDb(block->getDb());
}

}  // namespace gui
