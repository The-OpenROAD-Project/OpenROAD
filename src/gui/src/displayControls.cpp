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

#include <QApplication>
#include <QDebug>
#include <QFontDialog>
#include <QHeaderView>
#include <QKeyEvent>
#include <QLineEdit>
#include <QPainter>
#include <QRegExp>
#include <QSettings>
#include <QVBoxLayout>
#include <random>
#include <vector>

#include "db.h"
#include "displayControls.h"

#include "utl/Logger.h"

Q_DECLARE_METATYPE(odb::dbTechLayer*);
Q_DECLARE_METATYPE(std::function<void(void)>);

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

  main_layout_ = new QVBoxLayout;

  if (show_brush_) {
    pattern_group_box_ = new QGroupBox("Layer Pattern", this);
    grid_layout_ = new QGridLayout;

    grid_layout_->setColumnStretch(2, 4);

    int row_index = 0;
    for (auto& pattern_group : DisplayColorDialog::brush_patterns_) {
      int col_index = 0;
      for (auto pattern : pattern_group) {
        PatternButton* pattern_button = new PatternButton(pattern, this);
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

DisplayControlModel::DisplayControlModel(int user_data_item_idx, QWidget* parent) :
  QStandardItemModel(0, 4, parent),
  user_data_item_idx_(user_data_item_idx)
{
}

QVariant DisplayControlModel::data(const QModelIndex& index, int role) const
{
  if (role == Qt::ToolTipRole) {
    QStandardItem* item = itemFromIndex(index);
    QVariant data = item->data(user_data_item_idx_);
    if (data.isValid()) {
      odb::dbTechLayer* layer = data.value<odb::dbTechLayer*>();
      auto selected = Gui::get()->makeSelected(layer);
      if (selected) {
        auto props = selected.getProperties();

        // provide tooltip with layer information
        QString information;

        auto add_prop = [props](const std::string& prop, QString& info) -> bool {
          auto prop_find = std::find_if(props.begin(), props.end(), [prop](const auto& p) {
            return p.name == prop;
          });
          if (prop_find == props.end()) {
            return false;
          }
          info += "\n" + QString::fromStdString(prop) + ": ";
          info += QString::fromStdString(prop_find->toString());
          return true;
        };

        // direction
        add_prop("Direction", information);

        // min width
        add_prop("Minimum width", information);

        // min spacing
        add_prop("Minimum spacing", information);

        // resistance
        add_prop("Resistance", information);

        // capacitance
        add_prop("Capacitance", information);

        if (!information.isEmpty()) {
          return information.remove(0, 1);
        }
      }
    }
  }
  return QStandardItemModel::data(index, role);
}

QVariant DisplayControlModel::headerData(int section,
                                         Qt::Orientation orientation,
                                         int role) const
{
  if (orientation == Qt::Horizontal) {
    if (role == Qt::DisplayRole) {
      if (section == 0) {
        return "";
      }
    } else if (role == Qt::DecorationRole) {
      if (section == 1) {
        return QIcon(":/palette.png");
      } else if (section == 2) {
        return QIcon(":/visible.png");
      } else if (section == 3) {
        return QIcon(":/select.png");
      }
    }
  }

  return QVariant();
}

///////////

DisplayControls::DisplayControls(QWidget* parent)
    : QDockWidget("Display Control", parent),
      view_(new QTreeView(this)),
      model_(new DisplayControlModel(user_data_item_idx_, this)),
      layers_menu_(new QMenu(this)),
      layers_menu_layer_(nullptr),
      ignore_callback_(false),
      db_(nullptr),
      logger_(nullptr),
      tech_inited_(false)
{
  setObjectName("layers");  // for settings
  view_->setModel(model_);
  view_->setContextMenuPolicy(Qt::CustomContextMenu);

  QHeaderView* header = view_->header();
  header->setSectionResizeMode(Name, QHeaderView::Stretch);
  header->setSectionResizeMode(Swatch, QHeaderView::ResizeToContents);
  header->setSectionResizeMode(Visible, QHeaderView::ResizeToContents);
  header->setSectionResizeMode(Selectable, QHeaderView::ResizeToContents);
  // QTreeView defaults stretchLastSection to true, overriding setSectionResizeMode
  header->setStretchLastSection(false);

  createLayerMenu();

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
  makeLeafItem(instances_.welltap, "Welltap", instances_parent, Qt::Checked, true);
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

  // Rulers
  ruler_font_ = QApplication::font(); // use default font
  ruler_color_ = Qt::cyan;
  makeParentItem(rulers_, "Rulers", model_, Qt::Checked, true, row_color_);
  setNameItemDoubleClickAction(rulers_, [this]() {
    ruler_font_ = QFontDialog::getFont(nullptr, ruler_font_, this, "Ruler font");
  });

  // Rows
  row_color_ = QColor(0, 0xff, 0, 0x70);
  makeParentItem(rows_, "Rows", model_, Qt::Unchecked, false, row_color_);

  // Rows
  makeParentItem(pin_markers_, "Pin Markers", model_, Qt::Checked);
  pin_markers_font_ = QApplication::font(); // use default font
  setNameItemDoubleClickAction(pin_markers_, [this]() {
    pin_markers_font_ = QFontDialog::getFont(nullptr, pin_markers_font_, this, "Pin marker font");
  });


  // Track patterns group
  auto tracks = makeParentItem(
      tracks_group_, "Tracks", model_, Qt::Unchecked);

  makeLeafItem(tracks_.pref, "Pref", tracks, Qt::Unchecked);
  makeLeafItem(tracks_.non_pref, "Non Pref", tracks, Qt::Unchecked);
  toggleParent(tracks_group_);

  // Misc group
  auto misc = makeParentItem(
      misc_group_, "Misc", model_, Qt::Unchecked);

  instance_name_font_ = QApplication::font(); // use default font
  instance_name_color_ = Qt::yellow;

  makeLeafItem(misc_.instance_names, "Instance names", misc, Qt::Checked, false, instance_name_color_);
  makeLeafItem(misc_.scale_bar, "Scale bar", misc, Qt::Checked);
  makeLeafItem(misc_.fills, "Fills", misc, Qt::Unchecked);
  makeLeafItem(misc_.access_points, "Access Points", misc, Qt::Unchecked);
  makeLeafItem(misc_.detailed, "Detailed view", misc, Qt::Unchecked);
  makeLeafItem(misc_.selected, "Highlight selected", misc, Qt::Checked);
  toggleParent(misc_group_);
  setNameItemDoubleClickAction(misc_.instance_names, [this]() {
    instance_name_font_ = QFontDialog::getFont(nullptr, instance_name_font_, this, "Instance name font");
  });

  setWidget(view_);
  connect(model_,
          SIGNAL(itemChanged(QStandardItem*)),
          this,
          SLOT(itemChanged(QStandardItem*)));

  connect(view_->selectionModel(),
          SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
          this,
          SLOT(displayItemSelected(const QItemSelection&)));
  connect(view_,
          SIGNAL(doubleClicked(const QModelIndex&)),
          this,
          SLOT(displayItemDblClicked(const QModelIndex&)));

  connect(view_,
          SIGNAL(customContextMenuRequested(const QPoint &)),
          this,
          SLOT(itemContextMenu(const QPoint &)));

  // register renderers
  if (gui::Gui::get() != nullptr) {
    for (auto renderer : gui::Gui::get()->renderers()) {
      registerRenderer(renderer);
    }
  }
}

DisplayControls::~DisplayControls()
{
  custom_controls_.clear();
}

void DisplayControls::createLayerMenu()
{
  connect(layers_menu_->addAction("Show only selected"),
          &QAction::triggered,
          [this]() {
            layerShowOnlySelectedNeighbors(0, 0);
          });


  const QString show_range = "Show layer range ";
  const QString updown_arrow = "\u2195";
  const QString down_arrow = "\u2193";
  const QString up_arrow = "\u2191";
  auto add_range_action = [&](int up, int down) {
    QString arrows;
    for (int n = 1; n < down; n++) {
      arrows += up_arrow;
    }
    if (up > 0 && down > 0) {
      arrows += updown_arrow;
    } else if (down > 0) {
      arrows += up_arrow;
    } else if (up > 0) {
      arrows += down_arrow;
    }
    for (int n = 1; n < up; n++) {
      arrows += down_arrow;
    }

    connect(layers_menu_->addAction(show_range + arrows),
            &QAction::triggered,
            [this, up, down]() {
              layerShowOnlySelectedNeighbors(down, up);
            });
  };

  add_range_action(1, 1); // 1 layer above / below
  add_range_action(2, 2); // 2 layers above / below
  add_range_action(0, 1); // 1 layer below
  add_range_action(1, 0); // 1 layer above
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
  readSettingsForRow(settings, instances_.welltap);
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
  // pin markers
  readSettingsForRow(settings, pin_markers_);
  pin_markers_font_ = settings->value("pin_markers_font", pin_markers_font_).value<QFont>();

  // rulers
  readSettingsForRow(settings, rulers_);
  getColor(rulers_.swatch, ruler_color_, "ruler_color");
  ruler_font_ = settings->value("ruler_font", ruler_font_).value<QFont>();

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
  readSettingsForRow(settings, misc_.access_points);
  readSettingsForRow(settings, misc_.detailed);
  readSettingsForRow(settings, misc_.selected);
  getColor(misc_.instance_names.swatch, instance_name_color_, "instance_name_color");
  instance_name_font_ = settings->value("instance_name_font", instance_name_font_).value<QFont>();
  settings->endGroup();

  // custom renderers
  settings->beginGroup("custom");
  custom_controls_settings_.clear();
  for (const auto& group : settings->childGroups()) {
    auto& renderer_settings = custom_controls_settings_[group.toStdString()];

    settings->beginGroup(group);
    for (const auto& key_group : settings->childGroups()) {
      settings->beginGroup(key_group);
      const QVariant value = settings->value("data");
      const QString type = settings->value("type").value<QString>();
      if (type == "bool") {
        renderer_settings[key_group.toStdString()] = value.toBool();
      } else if (type == "int") {
        renderer_settings[key_group.toStdString()] = value.toInt();
      } else if (type == "double") {
        renderer_settings[key_group.toStdString()] = value.toDouble();
      } else if (type == "string") {
        renderer_settings[key_group.toStdString()] = value.toString().toStdString();
      } else {
        // this can get called before logger has been created
        if (logger_ != nullptr) {
          logger_->warn(utl::GUI, 57, "Unknown data type \"{}\" for \"{}\".", type.toStdString(), key_group.toStdString());
        }
      }
      settings->endGroup();
    }
    settings->endGroup();
  }
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
  writeSettingsForRow(settings, instances_.welltap);
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
  // pin markers
  writeSettingsForRow(settings, pin_markers_);
  settings->setValue("pin_markers_font", pin_markers_font_);

  // rulers
  writeSettingsForRow(settings, rulers_);
  settings->setValue("ruler_color", ruler_color_);
  settings->setValue("ruler_font", ruler_font_);

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
  writeSettingsForRow(settings, misc_.access_points);
  writeSettingsForRow(settings, misc_.detailed);
  writeSettingsForRow(settings, misc_.selected);
  settings->setValue("instance_name_color", instance_name_color_);
  settings->setValue("instance_name_font", instance_name_font_);
  settings->endGroup();

  // custom renderers
  settings->beginGroup("custom");
  for (auto renderer : Gui::get()->renderers()) {
    saveRendererState(renderer);
  }
  for (const auto& [group, renderer_settings] : custom_controls_settings_) {
    const QString group_name = QString::fromStdString(group);
    if (!group_name.isEmpty()) {
      settings->beginGroup(group_name);
      for (const auto& [name, value] : renderer_settings) {
        const QString setting_name = QString::fromStdString(name);
        settings->beginGroup(setting_name);
        QVariant data;
        QVariant type;
        if(const auto* v = std::get_if<bool>(&value)) {
          type = "bool";
          data = *v;
        } else if(const auto* v = std::get_if<int>(&value)) {
          type = "int";
          data = *v;
        } else if(const auto* v = std::get_if<double>(&value)) {
          type = "double";
          data = *v;
        } else if(const auto* v = std::get_if<std::string>(&value)) {
          type = "string";
          data = QString::fromStdString(*v);
        } else {
          logger_->warn(utl::GUI, 54, "Unknown data type for \"{}\".", name);
        }
        settings->setValue("data", data);
        settings->setValue("type", type);
        settings->endGroup();
      }
      settings->endGroup();
    }
  }
  settings->endGroup();

  settings->endGroup();
}

void DisplayControls::saveRendererState(Renderer* renderer)
{
  const std::string& group_name = renderer->getSettingsGroupName();
  if (group_name.empty()) {
    return;
  }

  custom_controls_settings_[group_name] = renderer->getSettings();
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
  Callback callback = item->data(callback_item_idx_).value<Callback>();
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

  // check if item has exclusivity set
  auto exclusive = item->data(exclusivity_item_idx_);
  if (exclusive.isValid() && checked) {
    QStandardItem* parent = model_->itemFromIndex(item_index.parent());

    QSet<QStandardItem*> items_to_check;
    for (const auto& exclusion : exclusive.value<QSet<QString>>()) {
      // "" means everything is exclusive
      bool exclude_all = exclusion == "";

      for (int r = 0; r < parent->rowCount(); r++) {
        const QModelIndex row_name = model_->index(r, Name, parent->index());
        const QModelIndex toggle_col = model_->index(r, item_index.column(), parent->index());
        if (exclude_all) {
          items_to_check.insert(model_->itemFromIndex(toggle_col));
        } else {
          auto* name_item = model_->itemFromIndex(row_name);
          if (name_item->text() == exclusion) {
            items_to_check.insert(model_->itemFromIndex(toggle_col));
          }
        }
      }
    }

    // toggle mutually exclusive items
    for (auto& check_item : items_to_check) {
      if (check_item == nullptr || check_item == item) {
        continue;
      }
      if (check_item->checkState() == Qt::Checked) {
        check_item->setCheckState(Qt::Unchecked);
      }
    }
  }

  emit changed();
}

void DisplayControls::displayItemSelected(const QItemSelection& selection)
{
  for (const auto& index : selection.indexes()) {
    const QModelIndex name_index = model_->index(index.row(), Name, index.parent());
    auto* name_item = model_->itemFromIndex(name_index);
    QVariant tech_layer_data = name_item->data(user_data_item_idx_);
    if (!tech_layer_data.isValid()) {
      continue;
    }
    auto* tech_layer = tech_layer_data.value<odb::dbTechLayer*>();
    if (tech_layer == nullptr) {
      continue;
    }
    emit selected(Gui::get()->makeSelected(tech_layer));
    return;
  }
}

void DisplayControls::displayItemDblClicked(const QModelIndex& index)
{
  if (index.column() == 0) {
    auto name_item = model_->itemFromIndex(index);

    auto data = name_item->data(doubleclick_item_idx_);
    if (data.isValid()) {
      auto callback = data.value<std::function<void(void)>>();
      callback();
      emit changed();
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
    } else if (color_item == rulers_.swatch) {
      item_color = &ruler_color_;
    } else {
      QVariant tech_layer_data = color_item->data(user_data_item_idx_);
      if (!tech_layer_data.isValid()) {
        return;
      }
      auto tech_layer = tech_layer_data.value<odb::dbTechLayer*>();
      if (tech_layer == nullptr) {
        return;
      }
      item_color = &layer_color_[tech_layer];
      item_pattern = &layer_pattern_[tech_layer];
      if (tech_layer->getType() != dbTechLayerType::ROUTING) {
        if (index.row() != 0) {
          // ensure if a via is the first layer, it can still be modified
          return;
        }
      } else {
        has_sibling = true;
      }
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
  std::vector<QStandardItem*> items;
  findControlsInItems(path,
                      is_visible ? Visible : Selectable,
                      items);

  if (items.empty()) {
    logger_->error(utl::GUI,
                   13,
                   "Unable to find {} display control at {}.",
                   is_visible ? "visible" : "select",
                   path);
  } else {
    for (auto* item : items) {
      item->setCheckState(value);
    }
  }
}

// path is separated by "/", so setting Standard Cells, would be Instances/StdCells
bool DisplayControls::checkControlByPath(const std::string& path,
                                         bool is_visible)
{
  std::vector<QStandardItem*> items;
  findControlsInItems(path,
                      is_visible ? Visible : Selectable,
                      items);

  if (items.empty()) {
    logger_->warn(utl::GUI,
                  14,
                  "Unable to find {} display control at {}.",
                  is_visible ? "visible" : "select",
                  path);
    // assume false when item cannot be found.
    return false;
  }

  if (items.size() == 1) {
    return items[0]->checkState() == Qt::Checked;
  } else {
    logger_->warn(utl::GUI,
                  34,
                  "Found {} controls matching {} at {}.",
                  items.size(),
                  path,
                  is_visible ? "visible" : "select");
    return false;
  }
}

void DisplayControls::collectControls(const QStandardItem* parent,
                                      Column column,
                                      std::map<std::string, QStandardItem*>& items,
                                      const std::string& prefix)
{
  for (int i = 0; i < parent->rowCount(); i++) {
    auto child = parent->child(i, Name);
    if (child != nullptr) {
      if (child->hasChildren()) {
        collectControls(child, column, items, prefix + child->text().toStdString() + "/");
      } else {
        auto* item = parent->child(i, column);
        if (item != nullptr) {
          items[prefix + child->text().toStdString()] = item;
        }
      }
    }
  }
}

void DisplayControls::findControlsInItems(const std::string& path,
                                          Column column,
                                          std::vector<QStandardItem*>& items)
{
  std::map<std::string, QStandardItem*> controls;
  collectControls(model_->invisibleRootItem(), column, controls);

  const QRegExp path_compare(QString::fromStdString(path), Qt::CaseInsensitive, QRegExp::Wildcard);
  for (auto& [item_path, item] : controls) {
    if (path_compare.exactMatch(QString::fromStdString(item_path))) {
      items.push_back(item);
    }
  }
}

void DisplayControls::save()
{
  saved_state_.clear();

  std::map<std::string, QStandardItem*> controls_visible;
  std::map<std::string, QStandardItem*> controls_selectable;
  collectControls(model_->invisibleRootItem(), Visible, controls_visible);
  collectControls(model_->invisibleRootItem(), Selectable, controls_selectable);

  for (auto& [control_name, control] : controls_visible) {
    saved_state_[control] = control->checkState();
  }
  for (auto& [control_name, control] : controls_selectable) {
    saved_state_[control] = control->checkState();
  }
}

void DisplayControls::restore()
{
  for (auto& [control, state] : saved_state_) {
    control->setCheckState(state);
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

  if (tech_inited_) {
    return;
  }

  techInit();

  for (dbTechLayer* layer : tech->getLayers()) {
    dbTechLayerType type = layer->getType();
    if (type == dbTechLayerType::ROUTING || type == dbTechLayerType::CUT) {
      auto& row = layer_controls_[layer];
      makeLeafItem(
          row,
          QString::fromStdString(layer->getName()),
          layers_group_.name,
          Qt::Checked,
          true,
          color(layer),
          QVariant::fromValue(layer));
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
  })), callback_item_idx_);
  if (add_selectable) {
    row.selectable->setData(QVariant::fromValue(Callback({
      [this, row](bool selectable) {
        toggleAllChildren(selectable, row.name, Selectable);
      }
    })), callback_item_idx_);
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
  if (user_data.isValid()) {
    row.name->setData(user_data, user_data_item_idx_);
  }

  row.swatch = new QStandardItem(makeSwatchIcon(color), "");
  row.swatch->setEditable(false);
  row.swatch->setCheckable(false);
  if (user_data.isValid()) {
    row.swatch->setData(user_data, user_data_item_idx_);
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

void DisplayControls::setNameItemDoubleClickAction(ModelRow& row, const std::function<void(void)>& callback)
{
  row.name->setData(QVariant::fromValue(callback), doubleclick_item_idx_);

  QFont current_font = row.name->data(Qt::FontRole).value<QFont>();
  current_font.setUnderline(true);
  row.name->setData(current_font, Qt::FontRole);
}

void DisplayControls::setItemExclusivity(ModelRow& row, const std::set<std::string>& exclusivity)
{
  QSet<QString> names;
  for (const auto& name : exclusivity) {
    names.insert(QString::fromStdString(name));
  }
  row.visible->setData(QVariant::fromValue(names), exclusivity_item_idx_);
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
    if (master->getType() == dbMasterType::CORE_WELLTAP) {
      return instances_.welltap.visible->checkState() == Qt::Checked;
    } else {
      return instances_.core.visible->checkState() == Qt::Checked;
    }
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
    if (master->getType() == dbMasterType::CORE_WELLTAP) {
      return instances_.welltap.selectable->checkState() == Qt::Checked;
    } else {
      return instances_.core.selectable->checkState() == Qt::Checked;
    }
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

bool DisplayControls::areRulersVisible()
{
  return rulers_.visible->checkState() == Qt::Checked;
}

bool DisplayControls::areRulersSelectable()
{
  return rulers_.selectable->checkState() == Qt::Checked;
}

QColor DisplayControls::rulerColor()
{
  return ruler_color_;
}

QFont DisplayControls::rulerFont()
{
  return ruler_font_;
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

bool DisplayControls::areSelectedVisible()
{
  return misc_.selected.visible->checkState() == Qt::Checked;
}

bool DisplayControls::isDetailedVisibility()
{
  return misc_.detailed.visible->checkState() == Qt::Checked;
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

bool DisplayControls::arePinMarkersVisible() const
{
  return pin_markers_.visible->checkState() == Qt::Checked;
}

bool DisplayControls::areAccessPointsVisible() const
{
  return misc_.access_points.visible->checkState() == Qt::Checked;
}

QFont DisplayControls::pinMarkersFont()
{
  return pin_markers_font_;
}

void DisplayControls::registerRenderer(Renderer* renderer)
{
  if (custom_controls_.count(renderer) != 0) {
    // already registered
    return;
  }

  const std::string& group_name = renderer->getDisplayControlGroupName();
  const auto& items = renderer->getDisplayControls();

  if (!items.empty()) {
    // build controls
    std::vector<ModelRow> rows;
    if (group_name.empty()) {
      for (const auto& [name, control] : items) {
        ModelRow row;
        makeParentItem(row,
                       QString::fromStdString(name),
                       model_,
                       control.visibility ? Qt::Checked : Qt::Unchecked);
        if (control.interactive_setup) {
          setNameItemDoubleClickAction(row, control.interactive_setup);
        }
        if (!control.mutual_exclusivity.empty()) {
          setItemExclusivity(row, control.mutual_exclusivity);
        }
        rows.push_back(row);
      }
    } else {
      const QString parent_item_name = QString::fromStdString(group_name);
      ModelRow parent_row;
      // check if parent has already been created
      for (const auto& [renderer, controls] : custom_controls_) {
        const QModelIndex& parent_idx = controls[0].name->index().parent();
        if (parent_idx.isValid()) {
          QStandardItem* check_item = model_->itemFromIndex(parent_idx);
          if (check_item->text() == parent_item_name) {
            parent_row.name = model_->item(parent_idx.row(), Name);
            parent_row.swatch = model_->item(parent_idx.row(), Swatch);
            parent_row.visible = model_->item(parent_idx.row(), Visible);
            parent_row.selectable = model_->item(parent_idx.row(), Selectable);
            break;
          }
        }
      }
      if (parent_row.name == nullptr) {
        makeParentItem(parent_row,
                       parent_item_name,
                       model_,
                       Qt::Checked);
      }
      for (const auto& [name, control] : items) {
        ModelRow row;
        makeLeafItem(row,
                     QString::fromStdString(name),
                     parent_row.name,
                     control.visibility ? Qt::Checked : Qt::Unchecked);
        if (control.interactive_setup) {
          setNameItemDoubleClickAction(row, control.interactive_setup);
        }
        if (!control.mutual_exclusivity.empty()) {
          setItemExclusivity(row, control.mutual_exclusivity);
        }
        rows.push_back(row);
      }
      toggleParent(parent_row);
    }

    auto& add_rows = custom_controls_[renderer];
    add_rows.insert(add_rows.begin(), rows.begin(), rows.end());
  }

  // check if there are settings to recover
  const std::string settings_name = renderer->getSettingsGroupName();
  if (!settings_name.empty()) {
    auto setting = custom_controls_settings_.find(settings_name);
    if (setting != custom_controls_settings_.end()) {
      renderer->setSettings(setting->second);
    }
  }
}

void DisplayControls::unregisterRenderer(Renderer* renderer)
{
  saveRendererState(renderer);

  if (custom_controls_.count(renderer) == 0) {
    return;
  }

  const auto& rows = custom_controls_[renderer];

  const QModelIndex& parent_idx = rows[0].name->index().parent();
  for (auto itr = rows.rbegin(); itr != rows.rend(); itr++) {
    // remove from Display controls
    auto index = model_->indexFromItem(itr->name);
    model_->removeRow(index.row(), index.parent());
  }
  if (!model_->hasChildren(parent_idx)) {
    model_->removeRow(parent_idx.row(), parent_idx.parent());
  }

  custom_controls_.erase(renderer);
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

  // ensure if random colors are used they are consistent
  std::mt19937 gen_color(1);

  // Iterate through the layers and set default colors
  for (dbTechLayer* layer : tech->getLayers()) {
    dbTechLayerType type = layer->getType();
    QColor color;
    if (type == dbTechLayerType::ROUTING) {
      if (metal < num_colors) {
        color = colors[metal++];
      } else {
        // pick a random color as we exceeded the built-in palette size
        color = QColor(50 + gen_color() % 200, 50 + gen_color() % 200, 50 + gen_color() % 200);
      }
    } else if (type == dbTechLayerType::CUT) {
      if (via < num_colors) {
        if (metal != 0) {
          color = colors[via++];
        } else {
          // via came first, so pick random color
          color = QColor(50 + gen_color() % 200, 50 + gen_color() % 200, 50 + gen_color() % 200);
        }
      } else {
        // pick a random color as we exceeded the built-in palette size
        color = QColor(50 + gen_color() % 200, 50 + gen_color() % 200, 50 + gen_color() % 200);
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

void DisplayControls::restoreTclCommands(std::vector<std::string>& cmds)
{
  buildRestoreTclCommands(cmds, model_->invisibleRootItem());
}

void DisplayControls::buildRestoreTclCommands(std::vector<std::string>& cmds, const QStandardItem* parent, const std::string& prefix)
{
  const std::string visible_restore = "gui::set_display_controls \"{}\" visible {}";
  const std::string selectable_restore = "gui::set_display_controls \"{}\" selectable {}";

  // loop over settings and save
  for (int r = 0; r < parent->rowCount(); r++) {
    QStandardItem* item = parent->child(r, 0);
    const std::string name = prefix + item->text().toStdString();

    if (item->hasChildren()) {
      buildRestoreTclCommands(cmds, item, name + "/");
    } else {
      bool visible = parent->child(r, Visible)->checkState() == Qt::Checked;
      cmds.push_back(fmt::format(visible_restore, name, visible));
      auto* selectable = parent->child(r, Selectable);
      if (selectable != nullptr) {
        bool select = selectable->checkState() == Qt::Checked;
        cmds.push_back(fmt::format(selectable_restore, name, select));
      }
    }
  }
}

void DisplayControls::itemContextMenu(const QPoint &point)
{
  const QModelIndex index = view_->indexAt(point);

  if (!index.isValid()) {
    return;
  }

  // check if index is a layer
  const QModelIndex parent = index.parent();
  if (!parent.isValid()) {
    return;
  }

  auto* parent_item = model_->itemFromIndex(parent);
  if (parent_item != layers_group_.name) {
    // not a member of the layers
    return;
  }

  const QModelIndex name_index = model_->index(index.row(), Name, parent);
  auto* name_item = model_->itemFromIndex(name_index);
  layers_menu_layer_ = name_item->data(user_data_item_idx_).value<odb::dbTechLayer*>();

  layers_menu_->popup(view_->viewport()->mapToGlobal(point));
}

void DisplayControls::layerShowOnlySelectedNeighbors(int lower, int upper)
{
  if (layers_menu_layer_ == nullptr) {
    return;
  }

  std::set<const odb::dbTechLayer*> layers;
  collectNeighboringLayers(layers_menu_layer_, lower, upper, layers);
  setOnlyVisibleLayers(layers);

  layers_menu_layer_ = nullptr;
}

void DisplayControls::collectNeighboringLayers(odb::dbTechLayer* layer,
                                               int lower,
                                               int upper,
                                               std::set<const odb::dbTechLayer*>& layers)
{
  if (layer == nullptr) {
    return;
  }

  layers.insert(layer);
  if (lower > 0) {
    collectNeighboringLayers(layer->getLowerLayer(), lower - 1, 0, layers);
  }

  if (upper > 0) {
    collectNeighboringLayers(layer->getUpperLayer(), 0, upper - 1, layers);
  }
}

void DisplayControls::setOnlyVisibleLayers(const std::set<const odb::dbTechLayer*> layers)
{
  for (auto& [layer, row] : layer_controls_) {
    row.visible->setCheckState(Qt::Unchecked);
  }

  for (auto* layer : layers) {
    if (layer_controls_.count(layer) != 0) {
      layer_controls_[layer].visible->setCheckState(Qt::Checked);
    }
  }
}

}  // namespace gui
