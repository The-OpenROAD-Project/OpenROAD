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
#include <QFontDialog>
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

#include "utl/Logger.h"

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
  // adjust by height of radio button + 4 pixels
  button_rect.adjust(button_rect.height() + 4, 2, -1, -2);
  qp.drawRect(button_rect);
  qp.end();
}

DisplayColorDialog::DisplayColorDialog(QColor color,
                                       Qt::BrushStyle pattern,
                                       QWidget* parent)
    : QDialog(parent), color_(color), pattern_(pattern), show_brush_(true)
{
  buildUI();
}

DisplayColorDialog::DisplayColorDialog(QColor color,
                                       QWidget* parent)
    : QDialog(parent), color_(color), pattern_(Qt::SolidPattern), show_brush_(false)
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

  main_layout_ = new QVBoxLayout();

  if (show_brush_) {
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
    main_layout_->addWidget(pattern_group_box_);
  }

  main_layout_->addWidget(color_dialog_);

  connect(color_dialog_, SIGNAL(accepted()), this, SLOT(acceptDialog()));
  connect(color_dialog_, SIGNAL(rejected()), this, SLOT(rejectDialog()));

  setLayout(main_layout_);
  setWindowTitle("Layer Config");
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
      db_(nullptr),
      logger_(nullptr),
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
  // QTreeView defaults stretchLastSection to true, overriding setSectionResizeMode
  header->setStretchLastSection(false);

  auto layers = makeParentItem(
      layers_group_,
      "Layers",
      model_,
      Qt::Checked,
      true);
  view_->expand(layers->index());

  // Nets group
  auto nets_parent = makeParentItem(
      nets_group_,
      "Nets",
      model_,
      Qt::Checked,
      true);

  // make net items, non-null last argument to create checkbox
  makeLeafItem(nets_.signal, "Signal", nets_parent, Qt::Checked, true);
  makeLeafItem(nets_.power, "Power", nets_parent, Qt::Checked, true);
  makeLeafItem(nets_.ground, "Ground", nets_parent, Qt::Checked, true);
  makeLeafItem(nets_.clock, "Clock", nets_parent, Qt::Checked, true);
  toggleParent(nets_group_);

  // Instance group
  auto instances_parent = makeParentItem(
      instance_group_,
      "Instances",
      model_,
      Qt::Checked,
      true);

  // make instance items, non-null last argument to create checkbox
  makeLeafItem(instances_.core, "StdCells", instances_parent, Qt::Checked, true);
  makeLeafItem(instances_.blocks, "Macros", instances_parent, Qt::Checked, true);
  makeLeafItem(instances_.fill, "Fill", instances_parent, Qt::Checked, true);
  makeLeafItem(instances_.endcap, "Endcap", instances_parent, Qt::Checked, true);
  makeLeafItem(instances_.pads, "Pads", instances_parent, Qt::Checked, true);
  makeLeafItem(instances_.cover, "Cover", instances_parent, Qt::Checked, true);
  toggleParent(instance_group_);

  // Blockages group
  auto blockages = makeParentItem(
      blockage_group_, "Blockages", model_, Qt::Checked, true);
  placement_blockage_color_ = Qt::darkGray;
  placement_blockage_pattern_ = Qt::BDiagPattern;

  makeLeafItem(blockages_.blockages, "Placement", blockages, Qt::Checked, true, placement_blockage_color_);
  makeLeafItem(blockages_.obstructions, "Routing", blockages, Qt::Checked, true);
  toggleParent(blockage_group_);

  // Rows
  row_color_ = QColor(0, 0xff, 0, 0x70);
  makeParentItem(rows_, "Rows", model_, Qt::Unchecked, false, row_color_);

  // Rows
  makeParentItem(congestion_map_, "Congestion Map", model_, Qt::Unchecked);
  makeParentItem(pin_markers_, "Pin Markers", model_, Qt::Checked);

  // Track patterns group
  auto tracks = makeParentItem(
      tracks_group_, "Tracks", model_, Qt::Unchecked);

  makeLeafItem(tracks_.pref, "Pref", tracks, Qt::Unchecked);
  makeLeafItem(tracks_.non_pref, "Non Pref", tracks, Qt::Unchecked);
  toggleParent(tracks_group_);

  // Misc group
  auto misc = makeParentItem(
      misc_group_, "Misc", model_, Qt::Unchecked);

  instance_name_color_ = Qt::yellow;

  makeLeafItem(misc_.instance_names, "Instance names", misc, Qt::Checked, false, instance_name_color_);
  instance_name_font_ = QFont(); // use default font
  instance_name_font_.setPointSize(12);
  makeLeafItem(misc_.scale_bar, "Scale bar", misc, Qt::Checked);
  makeLeafItem(misc_.fills, "Fills", misc, Qt::Unchecked);
  toggleParent(misc_group_);

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

void DisplayControls::writeSettingsForRow(QSettings* settings, const ModelRow& row)
{
  auto asBool
      = [](QStandardItem* item) { return item->checkState() == Qt::Checked; };

  settings->beginGroup(row.name->text());
  settings->setValue("visible", asBool(row.visible));
  if (row.selectable != nullptr) {
    settings->setValue("selectable", asBool(row.selectable));
  }
  settings->endGroup();
}

void DisplayControls::readSettingsForRow(QSettings* settings, const ModelRow& row)
{
  auto getChecked = [](QSettings* settings, QString name, bool default_value) {
    return settings->value(name, default_value).toBool() ? Qt::Checked : Qt::Unchecked;
  };

  settings->beginGroup(row.name->text());
  row.visible->setCheckState(getChecked(settings, "visible", row.visible->checkState() == Qt::Checked));
  if (row.selectable != nullptr) {
    row.selectable->setCheckState(getChecked(settings, "selectable", row.selectable->checkState() == Qt::Checked));
  }
  settings->endGroup();
}

void DisplayControls::readSettings(QSettings* settings)
{
  auto getColor = [this, settings](QStandardItem* item, QColor& color, const char* key) {
    color = settings->value(key, color).value<QColor>();
    item->setIcon(makeSwatchIcon(color));
  };
  settings->beginGroup("display_controls");

  settings->beginGroup("nets");
  readSettingsForRow(settings, nets_.signal);
  readSettingsForRow(settings, nets_.power);
  readSettingsForRow(settings, nets_.ground);
  readSettingsForRow(settings, nets_.clock);
  settings->endGroup();

  // instances
  settings->beginGroup("instances");
  readSettingsForRow(settings, instances_.core);
  readSettingsForRow(settings, instances_.blocks);
  readSettingsForRow(settings, instances_.fill);
  readSettingsForRow(settings, instances_.endcap);
  readSettingsForRow(settings, instances_.pads);
  readSettingsForRow(settings, instances_.cover);
  settings->endGroup();

  // blockages
  settings->beginGroup("blockages");
  readSettingsForRow(settings, blockages_.blockages);
  readSettingsForRow(settings, blockages_.obstructions);
  getColor(blockages_.blockages.swatch, placement_blockage_color_, "placement_color");
  // pattern saved as int
  placement_blockage_pattern_ =
      static_cast<Qt::BrushStyle>(settings->value("placement_pattern",
                                  static_cast<int>(placement_blockage_pattern_)).toInt());
  settings->endGroup();

  // rows
  readSettingsForRow(settings, rows_);
  getColor(rows_.swatch, row_color_, "row_color");
  // congestion map
  readSettingsForRow(settings, congestion_map_);
  // pin markers
  readSettingsForRow(settings, pin_markers_);

  // tracks
  settings->beginGroup("tracks");
  readSettingsForRow(settings, tracks_.pref);
  readSettingsForRow(settings, tracks_.non_pref);
  settings->endGroup();

  // misc
  settings->beginGroup("misc");
  readSettingsForRow(settings, misc_.instance_names);
  readSettingsForRow(settings, misc_.scale_bar);
  readSettingsForRow(settings, misc_.fills);
  getColor(misc_.instance_names.swatch, instance_name_color_, "instance_name_color");
  instance_name_font_ = settings->value("instance_name_font", instance_name_font_).value<QFont>();
  settings->endGroup();

  settings->endGroup();
}

void DisplayControls::writeSettings(QSettings* settings)
{
  settings->beginGroup("display_controls");

  // nets
  settings->beginGroup("nets");
  writeSettingsForRow(settings, nets_.signal);
  writeSettingsForRow(settings, nets_.power);
  writeSettingsForRow(settings, nets_.ground);
  writeSettingsForRow(settings, nets_.clock);
  settings->endGroup();

  // instances
  settings->beginGroup("instances");
  writeSettingsForRow(settings, instances_.core);
  writeSettingsForRow(settings, instances_.blocks);
  writeSettingsForRow(settings, instances_.fill);
  writeSettingsForRow(settings, instances_.endcap);
  writeSettingsForRow(settings, instances_.pads);
  writeSettingsForRow(settings, instances_.cover);
  settings->endGroup();

  // blockages
  settings->beginGroup("blockages");
  writeSettingsForRow(settings, blockages_.blockages);
  writeSettingsForRow(settings, blockages_.obstructions);
  settings->setValue("placement_color", placement_blockage_color_);
  // save pattern as int
  settings->setValue("placement_pattern", static_cast<int>(placement_blockage_pattern_));
  settings->endGroup();

  // rows
  writeSettingsForRow(settings, rows_);
  settings->setValue("row_color", row_color_);
  // congestion map
  writeSettingsForRow(settings, congestion_map_);
  // pin markers
  writeSettingsForRow(settings, pin_markers_);

  // tracks
  settings->beginGroup("tracks");
  writeSettingsForRow(settings, tracks_.pref);
  writeSettingsForRow(settings, tracks_.non_pref);
  settings->endGroup();

  // misc
  settings->beginGroup("misc");
  writeSettingsForRow(settings, misc_.instance_names);
  writeSettingsForRow(settings, misc_.scale_bar);
  writeSettingsForRow(settings, misc_.fills);
  settings->setValue("instance_name_color", instance_name_color_);
  settings->setValue("instance_name_font", instance_name_font_);
  settings->endGroup();

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

void DisplayControls::toggleParent(ModelRow& row)
{
  toggleParent(row.name, row.visible, Visible);
  if (row.selectable != nullptr) {
    toggleParent(row.name, row.selectable, Selectable);
  }
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
  if (index.column() == 0) {
    auto name_item = model_->itemFromIndex(index);

    if (name_item == misc_.instance_names.name) {
      // handle font change
      instance_name_font_ = QFontDialog::getFont(nullptr, instance_name_font_, this, "Instance name font");
    }
  }
  else if (index.column() == 1) { // handle color changes
    auto color_item = model_->itemFromIndex(index);

    QColor* item_color = nullptr;
    Qt::BrushStyle* item_pattern = nullptr;
    bool has_sibling = false;

    // check if placement
    if (color_item == blockages_.blockages.swatch) {
      item_color = &placement_blockage_color_;
      item_pattern = &placement_blockage_pattern_;
    } else if (color_item == misc_.instance_names.swatch) {
      item_color = &instance_name_color_;
    } else if (color_item == rows_.swatch) {
      item_color = &row_color_;
    } else {
      QVariant tech_layer_data = color_item->data(Qt::UserRole);
      if (!tech_layer_data.isValid()) {
        return;
      }
      auto tech_layer
          = static_cast<odb::dbTechLayer*>(tech_layer_data.value<void*>());
      if (tech_layer == nullptr) {
        return;
      }
      item_color = &layer_color_[tech_layer];
      item_pattern = &layer_pattern_[tech_layer];
      has_sibling = true;
    }

    if (item_color == nullptr) {
      return;
    }


    std::unique_ptr<DisplayColorDialog> display_dialog;
    if (item_pattern != nullptr) {
      display_dialog = std::make_unique<DisplayColorDialog>(*item_color, *item_pattern);
    } else {
      display_dialog = std::make_unique<DisplayColorDialog>(*item_color);
    }
    display_dialog->exec();
    QColor chosen_color = display_dialog->getSelectedColor();
    if (chosen_color.isValid()) {
      color_item->setIcon(makeSwatchIcon(chosen_color));

      if (has_sibling) {
        auto cut_layer_index
            = model_->sibling(index.row() + 1, index.column(), index);
        if (cut_layer_index.isValid()) {
          auto cut_color_item = model_->itemFromIndex(cut_layer_index);
          cut_color_item->setIcon(makeSwatchIcon(chosen_color));
        }
      }
      *item_color = chosen_color;
      if (item_pattern != nullptr) {
        *item_pattern = display_dialog->getSelectedPattern();
      }
      view_->repaint();
      emit changed();
    }
  }
}

// path is separated by "/", so setting Standard Cells, would be Instances/StdCells
void DisplayControls::setControlByPath(const std::string& path,
                                       bool is_visible,
                                       Qt::CheckState value)
{
  QStandardItem* item = findControlInItem(model_->invisibleRootItem(),
                                          path,
                                          is_visible ? Visible : Selectable);

  if (item == nullptr) {
    logger_->error(utl::GUI,
                   13,
                   "Unable to find {} display control at {}.",
                   is_visible ? "visible" : "select",
                   path);
  } else {
    item->setCheckState(value);
  }
}

QStandardItem* DisplayControls::findControlInItem(const QStandardItem* parent,
                                                  const std::string& name,
                                                  Column column)
{
  auto next_level = name.find_first_of("/");
  bool at_end_of_path = next_level == std::string::npos;
  const QString item_name = name.substr(0, next_level).c_str();
  for (int i = 0; i < parent->rowCount(); i++) {
    auto child = parent->child(i, Name);
    if (child != nullptr && child->text().compare(item_name, Qt::CaseInsensitive) == 0) {
      if (at_end_of_path) {
        return parent->child(i, column);
      } else {
        return findControlInItem(child, name.substr(next_level+1), column);
      }
    }
  }
  return nullptr;
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
      makeLeafItem(
          layer_controls_[layer],
          QString::fromStdString(layer->getName()),
          layers_group_.name,
          Qt::Checked,
          true,
          color(layer),
          type == dbTechLayerType::CUT ? QVariant() : QVariant::fromValue(static_cast<void*>(layer)));
    }
  }

  toggleParent(layers_group_);

  for (int i = 0; i < 4; i++)
    view_->resizeColumnToContents(i);
  emit changed();
}

void DisplayControls::setLogger(utl::Logger* logger)
{
  logger_ = logger;
}

QStandardItem* DisplayControls::makeParentItem(
    ModelRow& row,
    const QString& text,
    QStandardItemModel* parent,
    Qt::CheckState checked,
    bool add_selectable,
    const QColor& color)
{
  makeLeafItem(row, text, parent->invisibleRootItem(), checked, add_selectable, color);

  row.visible->setData(QVariant::fromValue(Callback({
    [this, row](bool visible) {
      toggleAllChildren(visible, row.name, Visible);
    }
  })));
  if (add_selectable) {
    row.selectable->setData(QVariant::fromValue(Callback({
      [this, row](bool selectable) {
        toggleAllChildren(selectable, row.name, Selectable);
      }
    })));
  }

  return row.name;
}

void DisplayControls::makeLeafItem(
    ModelRow& row,
    const QString& text,
    QStandardItem* parent,
    Qt::CheckState checked,
    bool add_selectable,
    const QColor& color,
    const QVariant& user_data)
{
  row.name = new QStandardItem(text);
  row.name->setEditable(false);

  row.swatch = new QStandardItem(makeSwatchIcon(color), "");
  row.swatch->setEditable(false);
  row.swatch->setCheckable(false);
  if (user_data.isValid()) {
    row.swatch->setData(user_data, Qt::UserRole);
  }

  row.visible = new QStandardItem("");
  row.visible->setCheckable(true);
  row.visible->setEditable(false);
  row.visible->setCheckState(checked);

  if (add_selectable) {
    row.selectable = new QStandardItem("");
    row.selectable->setCheckable(true);
    row.selectable->setEditable(false);
    row.selectable->setCheckState(checked);
  }

  parent->appendRow({row.name, row.swatch, row.visible, row.selectable});
}

const QIcon DisplayControls::makeSwatchIcon(const QColor& color)
{
  QPixmap swatch(20, 20);
  swatch.fill(color);

  return QIcon(swatch);
}

QColor DisplayControls::color(const odb::dbTechLayer* layer)
{
  return layer_color_.at(layer);
}

Qt::BrushStyle DisplayControls::pattern(const odb::dbTechLayer* layer)
{
  return layer_pattern_.at(layer);
}

QColor DisplayControls::placementBlockageColor()
{
  return placement_blockage_color_;
}

Qt::BrushStyle DisplayControls::placementBlockagePattern()
{
  return placement_blockage_pattern_;
}

QColor DisplayControls::instanceNameColor()
{
  return instance_name_color_;
}

QFont DisplayControls::instanceNameFont()
{
  return instance_name_font_;
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
  } else if (master->isCover()) {
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
  } else if (master->isCover()) {
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

bool DisplayControls::areInstanceNamesVisible()
{
  return misc_.instance_names.visible->checkState() == Qt::Checked;
}

bool DisplayControls::areFillsVisible()
{
  return misc_.fills.visible->checkState() == Qt::Checked;
}

bool DisplayControls::areBlockagesVisible()
{
  return blockages_.blockages.visible->checkState() == Qt::Checked;
}

bool DisplayControls::areBlockagesSelectable()
{
  return blockages_.blockages.selectable->checkState() == Qt::Checked;
}

bool DisplayControls::areObstructionsVisible()
{
  return blockages_.obstructions.visible->checkState() == Qt::Checked;
}

bool DisplayControls::areObstructionsSelectable()
{
  return blockages_.obstructions.selectable->checkState() == Qt::Checked;
}

bool DisplayControls::areRowsVisible()
{
  return rows_.visible->checkState() == Qt::Checked;
}

QColor DisplayControls::rowColor()
{
  return row_color_;
}

bool DisplayControls::arePrefTracksVisible()
{
  return tracks_.pref.visible->checkState() == Qt::Checked;
}

bool DisplayControls::areNonPrefTracksVisible()
{
  return tracks_.non_pref.visible->checkState() == Qt::Checked;
}

bool DisplayControls::isScaleBarVisible() const
{
  return misc_.scale_bar.visible->checkState() == Qt::Checked;
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
  makeParentItem(custom_controls_[name],
                 q_name,
                 model_,
                 checked);
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
