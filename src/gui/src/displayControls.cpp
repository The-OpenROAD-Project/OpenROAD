// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#include "displayControls.h"

#include <QApplication>
#include <QColor>
#include <QDialog>
#include <QFontDialog>
#include <QHeaderView>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMenu>
#include <QPainter>
#include <QVariant>
#include <QWidget>
#include <algorithm>
#include <array>
#include <map>
#include <memory>
#include <optional>
#include <ranges>
#include <set>
#include <utility>
#include <variant>
#include <vector>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QRegularExpression>
#else
#include <QRegExp>
#endif
#include <QSettings>
#include <QVBoxLayout>
#include <functional>
#include <random>
#include <string>

#include "dbDescriptors.h"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "gui/gui.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "utl/Logger.h"

Q_DECLARE_METATYPE(odb::dbTechLayer*);
Q_DECLARE_METATYPE(odb::dbSite*);
Q_DECLARE_METATYPE(std::function<void()>);
Q_DECLARE_METATYPE(gui::DisplayControls::ModelRow*);

namespace gui {

using odb::dbTechLayer;
using odb::dbTechLayerType;

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

DisplayColorDialog::DisplayColorDialog(const QColor& color,
                                       Qt::BrushStyle pattern,
                                       QWidget* parent)
    : QDialog(parent), color_(color), pattern_(pattern), show_brush_(true)
{
  buildUI();
}

DisplayColorDialog::DisplayColorDialog(const QColor& color, QWidget* parent)
    : QDialog(parent),
      color_(color),
      pattern_(Qt::SolidPattern),
      show_brush_(false)
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
    for (auto& pattern_group : DisplayColorDialog::kBrushPatterns) {
      int col_index = 0;
      for (auto pattern : pattern_group) {
        PatternButton* pattern_button = new PatternButton(pattern, this);
        pattern_buttons_.push_back(pattern_button);
        if (pattern == pattern_) {
          pattern_button->setChecked(true);
        } else {
          pattern_button->setChecked(false);
        }
        grid_layout_->addWidget(pattern_button, row_index, col_index);
        ++col_index;
      }
      ++row_index;
    }
    pattern_group_box_->setLayout(grid_layout_);
    main_layout_->addWidget(pattern_group_box_);
  }

  main_layout_->addWidget(color_dialog_);

  connect(color_dialog_,
          &QColorDialog::accepted,
          this,
          &DisplayColorDialog::acceptDialog);
  connect(color_dialog_,
          &QColorDialog::rejected,
          this,
          &DisplayColorDialog::rejectDialog);

  setLayout(main_layout_);
  setWindowTitle("Layer Config");
}

DisplayColorDialog::~DisplayColorDialog() = default;

Qt::BrushStyle DisplayColorDialog::getSelectedPattern() const
{
  for (auto pattern_button : pattern_buttons_) {
    if (pattern_button->isChecked()) {
      return pattern_button->pattern();
    }
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

DisplayControlModel::DisplayControlModel(int user_data_item_idx,
                                         QWidget* parent)
    : QStandardItemModel(0, 4, parent), user_data_item_idx_(user_data_item_idx)
{
}

QVariant DisplayControlModel::data(const QModelIndex& index, int role) const
{
  if (role == Qt::ToolTipRole) {
    QStandardItem* item = itemFromIndex(index);
    QVariant data = item->data(user_data_item_idx_);
    if (data.isValid()) {
      dbTechLayer* layer = data.value<dbTechLayer*>();
      if (layer != nullptr) {
        auto selected = Gui::get()->makeSelected(layer);
        if (selected) {
          auto props = selected.getProperties();

          // provide tooltip with layer information
          QString information;

          auto add_prop
              = [&props](const std::string& prop, QString& info) -> bool {
            auto prop_find = std::ranges::find_if(
                props, [prop](const auto& p) { return p.name == prop; });
            if (prop_find == props.end()) {
              return false;
            }
            info += "\n" + QString::fromStdString(prop) + ": ";
            info += QString::fromStdString(prop_find->toString());
            return true;
          };

          // type
          add_prop("Layer type", information);

          // direction
          add_prop("Direction", information);

          // min path width
          add_prop("Default width", information);

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
      }
      if (section == 2) {
        return QIcon(":/visible.png");
      }
      if (section == 3) {
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
      model_(new DisplayControlModel(kUserDataItemIdx, this)),
      routing_layers_menu_(new QMenu(this)),
      layers_menu_(new QMenu(this)),
      layers_menu_layer_(nullptr),
      ignore_callback_(false),
      ignore_selection_(false),
      default_site_color_(QColor(0, 0xff, 0, 0x70)),
      logger_(nullptr),
      sta_(nullptr),
      inst_descriptor_(nullptr)
{
  setObjectName("layers");  // for settings
  view_->setModel(model_);
  view_->setContextMenuPolicy(Qt::CustomContextMenu);

  view_->viewport()->installEventFilter(this);

  QHeaderView* header = view_->header();
  header->setMinimumSectionSize(25);
  header->setSectionResizeMode(kName, QHeaderView::Stretch);
  header->setSectionResizeMode(kSwatch, QHeaderView::ResizeToContents);
  header->setSectionResizeMode(kVisible, QHeaderView::ResizeToContents);
  header->setSectionResizeMode(kSelectable, QHeaderView::ResizeToContents);
  // QTreeView defaults stretchLastSection to true, overriding
  // setSectionResizeMode
  header->setStretchLastSection(false);

  createLayerMenu();

  auto* root = model_->invisibleRootItem();

  custom_controls_start_ = -1;

  auto layers
      = makeParentItem(layers_group_, "Layers", root, Qt::Checked, true);
  view_->expand(layers->index());
  auto implant_layer
      = makeParentItem(layers_.implant, "Implant", layers, Qt::Checked, true);
  auto other_layer
      = makeParentItem(layers_.other, "Other", layers, Qt::Unchecked, true);
  // hide initially
  view_->setRowHidden(implant_layer->row(), layers->index(), true);
  view_->setRowHidden(other_layer->row(), layers->index(), true);

  // Nets group
  auto nets_parent
      = makeParentItem(nets_group_, "Nets", root, Qt::Checked, true);

  // make net items, non-null last argument to create checkbox
  makeLeafItem(nets_.signal, "Signal", nets_parent, Qt::Checked, true);
  makeLeafItem(nets_.power, "Power", nets_parent, Qt::Checked, true);
  makeLeafItem(nets_.ground, "Ground", nets_parent, Qt::Checked, true);
  makeLeafItem(nets_.clock, "Clock", nets_parent, Qt::Checked, true);
  makeLeafItem(nets_.reset, "Reset", nets_parent, Qt::Checked, true);
  makeLeafItem(nets_.tieoff, "Tie off", nets_parent, Qt::Checked, true);
  makeLeafItem(nets_.scan, "Scan", nets_parent, Qt::Checked, true);
  makeLeafItem(nets_.analog, "Analog", nets_parent, Qt::Checked, true);
  toggleParent(nets_group_);

  // Instance group
  auto instances_parent
      = makeParentItem(instance_group_, "Instances", root, Qt::Checked, true);

  // make instance items, non-null last argument to create checkbox
  // stdcell instances
  auto stdcell_parent = makeParentItem(
      instances_.stdcells, "StdCells", instance_group_.name, Qt::Checked, true);

  auto bufinv_parent = makeParentItem(stdcell_instances_.bufinv,
                                      "Buffers/Inverters",
                                      stdcell_parent,
                                      Qt::Checked,
                                      true);
  makeLeafItem(bufinv_instances_.timing,
               "Timing opt.",
               bufinv_parent,
               Qt::Checked,
               true);
  makeLeafItem(
      bufinv_instances_.other, "Netlist", bufinv_parent, Qt::Checked, true);
  toggleParent(stdcell_instances_.bufinv);

  makeLeafItem(stdcell_instances_.combinational,
               "Combinational",
               stdcell_parent,
               Qt::Checked,
               true);
  makeLeafItem(stdcell_instances_.sequential,
               "Sequential",
               stdcell_parent,
               Qt::Checked,
               true);
  auto clock_tree_parent = makeParentItem(stdcell_instances_.clock_tree,
                                          "Clock tree",
                                          stdcell_parent,
                                          Qt::Checked,
                                          true);
  makeLeafItem(clock_tree_instances_.bufinv,
               "Buffer/Inverter",
               clock_tree_parent,
               Qt::Checked,
               true);
  makeLeafItem(clock_tree_instances_.clock_gates,
               "Clock gate",
               clock_tree_parent,
               Qt::Checked,
               true);
  toggleParent(stdcell_instances_.clock_tree);
  makeLeafItem(stdcell_instances_.level_shiters,
               "Level shifter",
               stdcell_parent,
               Qt::Checked,
               true);
  toggleParent(instances_.stdcells);

  makeLeafItem(instances_.blocks, "Macro", instances_parent, Qt::Checked, true);

  auto pad_parent = makeParentItem(
      instances_.pads, "Pads", instances_parent, Qt::Checked, true);
  makeLeafItem(pad_instances_.input, "Input", pad_parent, Qt::Checked, true);
  makeLeafItem(pad_instances_.output, "Output", pad_parent, Qt::Checked, true);
  makeLeafItem(pad_instances_.inout, "Inout", pad_parent, Qt::Checked, true);
  makeLeafItem(pad_instances_.power, "Power", pad_parent, Qt::Checked, true);
  makeLeafItem(pad_instances_.spacer, "Spacer", pad_parent, Qt::Checked, true);
  makeLeafItem(pad_instances_.areaio, "Area IO", pad_parent, Qt::Checked, true);
  makeLeafItem(pad_instances_.other, "Other", pad_parent, Qt::Checked, true);
  toggleParent(instances_.pads);

  auto phys_parent = makeParentItem(
      instances_.physical, "Physical", instances_parent, Qt::Checked, true);
  makeLeafItem(
      physical_instances_.fill, "Fill cell", phys_parent, Qt::Checked, true);
  makeLeafItem(
      physical_instances_.endcap, "Endcap", phys_parent, Qt::Checked, true);
  makeLeafItem(
      physical_instances_.tap, "Welltap", phys_parent, Qt::Checked, true);
  makeLeafItem(
      physical_instances_.tie, "Tie high/low", phys_parent, Qt::Checked, true);
  makeLeafItem(
      physical_instances_.antenna, "Antenna", phys_parent, Qt::Checked, true);
  makeLeafItem(
      physical_instances_.cover, "Cover", phys_parent, Qt::Checked, true);
  makeLeafItem(
      physical_instances_.bump, "Bump", phys_parent, Qt::Checked, true);
  makeLeafItem(
      physical_instances_.other, "Other", phys_parent, Qt::Checked, true);
  toggleParent(instances_.physical);
  toggleParent(instance_group_);

  // Blockages group
  auto blockages
      = makeParentItem(blockage_group_, "Blockages", root, Qt::Checked, true);
  placement_blockage_color_ = Qt::darkGray;
  placement_blockage_pattern_ = Qt::BDiagPattern;

  makeLeafItem(blockages_.blockages,
               "Placement",
               blockages,
               Qt::Checked,
               true,
               placement_blockage_color_);
  makeLeafItem(
      blockages_.obstructions, "Routing", blockages, Qt::Checked, true);
  toggleParent(blockage_group_);

  // Rulers
  ruler_font_ = QApplication::font();  // use default font
  ruler_color_ = Qt::cyan;
  makeParentItem(rulers_, "Rulers", root, Qt::Checked, true, ruler_color_);
  setNameItemDoubleClickAction(rulers_, [this]() {
    ruler_font_
        = QFontDialog::getFont(nullptr, ruler_font_, this, "Ruler font");
  });

  // Rows / sites
  makeParentItem(site_group_, "Rows", root, Qt::Unchecked, true);

  // Track patterns group
  auto tracks = makeParentItem(tracks_group_, "Tracks", root, Qt::Unchecked);

  makeLeafItem(tracks_.pref, "Pref", tracks, Qt::Unchecked);
  makeLeafItem(tracks_.non_pref, "Non Pref", tracks, Qt::Unchecked);
  toggleParent(tracks_group_);

  // Shape type group
  auto shape_types
      = makeParentItem(shape_type_group_, "Shape Types", root, Qt::Checked);
  auto shape_types_routing = makeParentItem(
      shape_types_.routing_group, "Routing", shape_types, Qt::Checked);
  makeLeafItem(shape_types_.routing.segments,
               "Segments",
               shape_types_routing,
               Qt::Checked);
  makeLeafItem(
      shape_types_.routing.vias, "Vias", shape_types_routing, Qt::Checked);
  auto shape_types_srouting = makeParentItem(shape_types_.special_routing_group,
                                             "Special Routing",
                                             shape_types,
                                             Qt::Checked);
  makeLeafItem(shape_types_.special_routing.segments,
               "Segments",
               shape_types_srouting,
               Qt::Checked);
  makeLeafItem(shape_types_.special_routing.vias,
               "Vias",
               shape_types_srouting,
               Qt::Checked);
  makeLeafItem(shape_types_.pins, "Pins", shape_types, Qt::Checked, true);
  makeLeafItem(shape_types_.pin_names, "Pin Names", shape_types, Qt::Checked);
  shape_types_.pins.visible->setData(
      QVariant::fromValue(&shape_types_.pin_names), kDisableRowItemIdx);
  pin_markers_font_ = QApplication::font();  // use default font
  setNameItemDoubleClickAction(shape_types_.pin_names, [this]() {
    pin_markers_font_ = QFontDialog::getFont(
        nullptr, pin_markers_font_, this, "Pin marker font");
  });
  makeLeafItem(shape_types_.fill, "Fills", shape_types, Qt::Unchecked);
  toggleParent(shape_type_group_);

  // Misc group
  auto misc = makeParentItem(misc_group_, "Misc", root, Qt::Unchecked, true);

  instance_name_font_ = QApplication::font();  // use default font
  instance_name_color_ = Qt::yellow;

  iterm_label_font_ = QApplication::font();  // use default font
  iterm_label_color_ = Qt::yellow;

  label_font_ = QApplication::font();  // use default font

  auto instance_shape
      = makeParentItem(misc_.instances, "Instances", misc, Qt::Checked, true);
  makeLeafItem(instance_shapes_.names,
               "Names",
               instance_shape,
               Qt::Checked,
               false,
               instance_name_color_);
  makeLeafItem(
      instance_shapes_.pins, "Pins", instance_shape, Qt::Checked, true);
  makeLeafItem(instance_shapes_.iterm_labels,
               "Pin Names",
               instance_shape,
               Qt::Unchecked,
               false,
               iterm_label_color_);
  instance_shapes_.pins.visible->setData(
      QVariant::fromValue(&instance_shapes_.iterm_labels), kDisableRowItemIdx);
  makeLeafItem(
      instance_shapes_.blockages, "Blockages", instance_shape, Qt::Checked);
  toggleParent(misc_.instances);
  setNameItemDoubleClickAction(instance_shapes_.names, [this]() {
    instance_name_font_ = QFontDialog::getFont(
        nullptr, instance_name_font_, this, "Instance name font");
  });
  setNameItemDoubleClickAction(instance_shapes_.iterm_labels, [this]() {
    iterm_label_font_ = QFontDialog::getFont(
        nullptr, iterm_label_font_, this, "Instance pin name font");
  });

  region_color_ = QColor(0x70, 0x70, 0x70, 0x70);  // semi-transparent mid-gray
  region_pattern_ = Qt::SolidPattern;
  background_color_ = Qt::black;
  makeLeafItem(misc_.scale_bar, "Scale bar", misc, Qt::Checked);
  makeLeafItem(misc_.access_points, "Access points", misc, Qt::Unchecked);
  makeLeafItem(
      misc_.regions, "Regions", misc, Qt::Checked, true, region_color_);
  makeLeafItem(misc_.detailed, "Detailed view", misc, Qt::Unchecked);
  makeLeafItem(misc_.selected, "Highlight selected", misc, Qt::Checked);
  makeLeafItem(misc_.module, "Module view", misc, Qt::Unchecked);
  makeLeafItem(
      misc_.manufacturing_grid, "Manufacturing grid", misc, Qt::Unchecked);
  makeLeafItem(misc_.gcell_grid, "GCell grid", misc, Qt::Unchecked);
  makeLeafItem(misc_.flywires_only, "Flywires only", misc, Qt::Unchecked);
  makeLeafItem(
      misc_.focused_nets_guides, "Focused nets guides", misc, Qt::Unchecked);
  makeLeafItem(misc_.labels, "Labels", misc, Qt::Checked, true);
  setNameItemDoubleClickAction(misc_.labels, [this]() {
    label_font_
        = QFontDialog::getFont(nullptr, label_font_, this, "User label font");
  });
  makeLeafItem(
      misc_.background, "Background", misc, {}, false, background_color_);
  toggleParent(misc_group_);

  checkLiberty();

  setWidget(view_);
  connect(model_,
          &DisplayControlModel::itemChanged,
          this,
          &DisplayControls::itemChanged);

  connect(view_->selectionModel(),
          &QItemSelectionModel::selectionChanged,
          this,
          &DisplayControls::displayItemSelected);
  connect(view_,
          &QTreeView::doubleClicked,
          this,
          &DisplayControls::displayItemDblClicked);

  connect(view_,
          &QTreeView::customContextMenuRequested,
          this,
          &DisplayControls::itemContextMenu);

  connect(
      this, &DisplayControls::colorChanged, this, &DisplayControls::changed);

  custom_controls_start_ = root->rowCount();

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
          [this]() { layerShowOnlySelectedNeighbors(0, 0); });

  connect(routing_layers_menu_->addAction("Show only selected"),
          &QAction::triggered,
          [this]() { layerShowOnlySelectedNeighbors(0, 0); });

  const QString show_range = "Show layer range ";
  const QString updown_arrow = "↕";
  const QString down_arrow = "↓";
  const QString up_arrow = "↑";
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

    connect(routing_layers_menu_->addAction(show_range + arrows),
            &QAction::triggered,
            [this, up, down]() { layerShowOnlySelectedNeighbors(down, up); });
  };

  add_range_action(1, 1);  // 1 layer above / below
  add_range_action(2, 2);  // 2 layers above / below
  add_range_action(0, 1);  // 1 layer below
  add_range_action(1, 0);  // 1 layer above
}

void DisplayControls::writeSettingsForRow(QSettings* settings,
                                          const ModelRow& row,
                                          bool include_children)
{
  writeSettingsForRow(
      settings, row.name, row.visible, row.selectable, include_children);
}

void DisplayControls::writeSettingsForRow(QSettings* settings,
                                          const QStandardItem* name,
                                          const QStandardItem* visible,
                                          const QStandardItem* selectable,
                                          bool include_children)
{
  auto as_bool = [](const QStandardItem* item) {
    return item->checkState() != Qt::Unchecked;
  };

  settings->beginGroup(name->text());
  if (name->hasChildren() && include_children) {
    for (int r = 0; r < name->rowCount(); r++) {
      writeSettingsForRow(settings,
                          name->child(r, kName),
                          name->child(r, kVisible),
                          name->child(r, kSelectable));
    }
  } else {
    if (visible != nullptr) {
      settings->setValue("visible", as_bool(visible));
    }
    if (selectable != nullptr) {
      settings->setValue("selectable", as_bool(selectable));
    }
  }
  settings->endGroup();
}

void DisplayControls::readSettingsForRow(QSettings* settings,
                                         const ModelRow& row,
                                         bool include_children)
{
  readSettingsForRow(
      settings, row.name, row.visible, row.selectable, include_children);
}

void DisplayControls::readSettingsForRow(QSettings* settings,
                                         const QStandardItem* name,
                                         QStandardItem* visible,
                                         QStandardItem* selectable,
                                         bool include_children)
{
  auto get_checked = [](QSettings* settings,
                        const QString& name,
                        const QStandardItem* item) {
    return settings->value(name, item->checkState() != Qt::Unchecked).toBool()
               ? Qt::Checked
               : Qt::Unchecked;
  };

  settings->beginGroup(name->text());
  if (name->hasChildren() && include_children) {
    for (int r = 0; r < name->rowCount(); r++) {
      readSettingsForRow(settings,
                         name->child(r, kName),
                         name->child(r, kVisible),
                         name->child(r, kSelectable));
    }
  } else {
    if (visible != nullptr) {
      visible->setCheckState(get_checked(settings, "visible", visible));
    }
    if (selectable != nullptr) {
      selectable->setCheckState(
          get_checked(settings, "selectable", selectable));
    }
  }
  settings->endGroup();
}

void DisplayControls::readSettings(QSettings* settings)
{
  auto get_color
      = [this, settings](const ModelRow& row, QColor& color, const char* key) {
          color = settings->value(key, color).value<QColor>();
          row.swatch->setIcon(makeSwatchIcon(color));
        };
  auto get_pattern = [settings](Qt::BrushStyle& style, const char* key) {
    style = static_cast<Qt::BrushStyle>(
        settings->value(key, static_cast<int>(style)).toInt());
  };
  auto get_font = [settings](QFont& font, const char* key) {
    font = settings->value(key, font).value<QFont>();
  };

  settings->beginGroup("display_controls");

  readSettingsForRow(settings, nets_group_);
  readSettingsForRow(settings, instance_group_);
  readSettingsForRow(settings, blockage_group_);
  readSettingsForRow(settings, rulers_);
  readSettingsForRow(settings, tracks_group_);
  readSettingsForRow(settings, shape_type_group_);
  readSettingsForRow(settings, misc_group_);

  readSettingsForRow(settings, site_group_, false);

  settings->beginGroup("other");
  settings->beginGroup("color");
  get_color(misc_.background, background_color_, "background");
  get_color(
      blockages_.blockages, placement_blockage_color_, "blockages_placement");
  get_color(rulers_, ruler_color_, "ruler");
  get_color(instance_shapes_.names, instance_name_color_, "instance_name");
  get_color(instance_shapes_.iterm_labels, iterm_label_color_, "iterm_label");
  get_color(misc_.regions, region_color_, "region");
  settings->endGroup();
  settings->beginGroup("pattern");
  get_pattern(placement_blockage_pattern_, "blockages_placement");
  get_pattern(region_pattern_, "region");
  settings->endGroup();
  settings->beginGroup("font");
  get_font(pin_markers_font_, "pin_markers");
  get_font(ruler_font_, "ruler");
  get_font(label_font_, "label");
  get_font(instance_name_font_, "instance_name");
  get_font(iterm_label_font_, "iterm_label");
  settings->endGroup();
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
        renderer_settings[key_group.toStdString()]
            = value.toString().toStdString();
      } else {
        // this can get called before logger has been created
        if (logger_ != nullptr) {
          logger_->warn(utl::GUI,
                        57,
                        "Unknown data type \"{}\" for \"{}\".",
                        type.toStdString(),
                        key_group.toStdString());
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

  writeSettingsForRow(settings, nets_group_);
  writeSettingsForRow(settings, instance_group_);
  writeSettingsForRow(settings, blockage_group_);
  writeSettingsForRow(settings, rulers_);
  writeSettingsForRow(settings, tracks_group_);
  writeSettingsForRow(settings, shape_type_group_);
  writeSettingsForRow(settings, misc_group_);
  writeSettingsForRow(settings, site_group_, false);

  settings->beginGroup("other");
  settings->beginGroup("color");
  settings->setValue("background", background_color_);
  settings->setValue("blockages_placement", placement_blockage_color_);
  settings->setValue("ruler", ruler_color_);
  settings->setValue("instance_name", instance_name_color_);
  settings->setValue("iterm_label", iterm_label_color_);
  settings->setValue("region", region_color_);
  settings->endGroup();
  settings->beginGroup("pattern");
  // save pattern as int
  settings->setValue("blockages_placement",
                     static_cast<int>(placement_blockage_pattern_));
  settings->setValue("region", static_cast<int>(region_pattern_));
  settings->endGroup();
  settings->beginGroup("font");
  settings->setValue("pin_markers", pin_markers_font_);
  settings->setValue("ruler", ruler_font_);
  settings->setValue("label", label_font_);
  settings->setValue("instance_name", instance_name_font_);
  settings->setValue("iterm_label", iterm_label_font_);
  settings->endGroup();
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
        if (const auto* v = std::get_if<bool>(&value)) {
          type = "bool";
          data = *v;
        } else if (const auto* v = std::get_if<int>(&value)) {
          type = "int";
          data = *v;
        } else if (const auto* v = std::get_if<double>(&value)) {
          type = "double";
          data = *v;
        } else if (const auto* v = std::get_if<std::string>(&value)) {
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
    if (child) {
      child->setCheckState(state);
    }
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
    if (view_->isRowHidden(row, parent->index())) {
      continue;
    }
    auto child = parent->child(row, column);
    if (child) {
      bool checked = child->checkState() == Qt::Checked;
      at_least_one_checked |= checked;
      all_checked &= checked;
    }
  }

  ignore_callback_ = true;
  Qt::CheckState new_state;
  if (all_checked) {
    new_state = Qt::Checked;
  } else if (at_least_one_checked) {
    new_state = Qt::PartiallyChecked;
  } else {
    new_state = Qt::Unchecked;
  }
  parent_flag->setCheckState(new_state);
  ignore_callback_ = false;
}

void DisplayControls::toggleParent(ModelRow& row)
{
  if (row.visible != nullptr) {
    toggleParent(row.name, row.visible, kVisible);
  }
  if (row.selectable != nullptr) {
    toggleParent(row.name, row.selectable, kSelectable);
  }
}

void DisplayControls::itemChanged(QStandardItem* item)
{
  if (item->isCheckable() == false) {
    emit changed();
    return;
  }
  bool checked = item->checkState() == Qt::Checked;
  Callback callback = item->data(kCallbackItemIdx).value<Callback>();
  if (callback.action && !ignore_callback_) {
    callback.action(checked);
  }
  QModelIndex item_index = item->index();
  QModelIndex parent_index = item_index.parent();
  if (parent_index.isValid()) {
    const QModelIndex toggle_parent_index
        = model_->index(parent_index.row(), kName, parent_index.parent());
    const QModelIndex toggle_index = model_->index(
        parent_index.row(), item_index.column(), parent_index.parent());
    toggleParent(model_->itemFromIndex(toggle_parent_index),  // parent row
                 model_->itemFromIndex(toggle_index),         // selected column
                 item_index.column());
  }
  // disable selectable column if visible is unchecked
  if (item_index.column() == kVisible) {
    QStandardItem* selectable = nullptr;
    if (!parent_index.isValid()) {
      selectable = model_->item(item_index.row(), kSelectable);
    } else {
      if (item->parent() != nullptr) {
        selectable = item->parent()->child(item_index.row(), kSelectable);
      }
    }

    if (selectable != nullptr) {
      selectable->setEnabled(item->checkState() != Qt::Unchecked);
    }
  }

  // check if item has exclusivity set
  auto exclusive = item->data(kExclusivityItemIdx);
  if (exclusive.isValid() && checked) {
    QStandardItem* parent = model_->itemFromIndex(item_index.parent());

    QSet<QStandardItem*> items_to_check;
    for (const auto& exclusion : exclusive.value<QSet<QString>>()) {
      // "" means everything is exclusive
      bool exclude_all = exclusion == "";

      for (int r = 0; r < parent->rowCount(); r++) {
        const QModelIndex row_name = model_->index(r, kName, parent->index());
        const QModelIndex toggle_col
            = model_->index(r, item_index.column(), parent->index());
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

  // check disabled pair
  auto disabled_row_pair = item->data(kDisableRowItemIdx);
  if (disabled_row_pair.isValid()) {
    const ModelRow* row = disabled_row_pair.value<ModelRow*>();
    row->name->setEnabled(checked);
    row->swatch->setEnabled(checked);
    if (row->visible) {
      row->visible->setEnabled(checked);
    }
    if (row->selectable) {
      row->selectable->setEnabled(checked);
    }
  }

  emit changed();
}

void DisplayControls::displayItemSelected(const QItemSelection& selection)
{
  if (ignore_selection_) {
    return;
  }

  for (const auto& index : selection.indexes()) {
    const QModelIndex name_index
        = model_->index(index.row(), kName, index.parent());
    auto* name_item = model_->itemFromIndex(name_index);
    QVariant user_data = name_item->data(kUserDataItemIdx);
    if (!user_data.isValid()) {
      continue;
    }

    if (auto* tech_layer = user_data.value<dbTechLayer*>()) {
      emit selected(Gui::get()->makeSelected(tech_layer));
    } else if (auto* site = user_data.value<odb::dbSite*>()) {
      emit selected(Gui::get()->makeSelected(site));
    } else {
      continue;
    }
    return;
  }
}

std::pair<QColor*, Qt::BrushStyle*> DisplayControls::lookupColor(
    QStandardItem* item,
    const QModelIndex* index)
{
  if (item == misc_.background.swatch) {
    return {&background_color_, nullptr};
  }
  if (item == blockages_.blockages.swatch) {
    return {&placement_blockage_color_, &placement_blockage_pattern_};
  }
  if (item == misc_.regions.swatch) {
    return {&region_color_, &region_pattern_};
  }
  if (item == instance_shapes_.names.swatch) {
    return {&instance_name_color_, nullptr};
  }
  if (item == instance_shapes_.iterm_labels.swatch) {
    return {&iterm_label_color_, nullptr};
  }
  if (item == rulers_.swatch) {
    return {&ruler_color_, nullptr};
  }
  QVariant tech_layer_data = item->data(kUserDataItemIdx);
  if (!tech_layer_data.isValid()) {
    return {nullptr, nullptr};
  }
  auto tech_layer = tech_layer_data.value<dbTechLayer*>();
  auto site = tech_layer_data.value<odb::dbSite*>();
  if (tech_layer != nullptr) {
    QColor* item_color = &layer_color_[tech_layer];
    Qt::BrushStyle* item_pattern = &layer_pattern_[tech_layer];
    return {item_color, item_pattern};
  }
  if (site != nullptr) {
    return {&site_color_[site], nullptr};
  }

  return {nullptr, nullptr};
}

void DisplayControls::displayItemDblClicked(const QModelIndex& index)
{
  if (!model_->itemFromIndex(index)->isEnabled()) {
    // ignore disabled items
    return;
  }

  if (index.column() == 0) {
    auto name_item = model_->itemFromIndex(index);

    auto data = name_item->data(kDoubleclickItemIdx);
    if (data.isValid()) {
      auto callback = data.value<std::function<void()>>();
      callback();
      emit changed();
    }
  } else if (index.column() == 1) {  // handle color changes
    auto color_item = model_->itemFromIndex(index);

    QColor* item_color = nullptr;
    Qt::BrushStyle* item_pattern = nullptr;

    const auto lookup = lookupColor(color_item, &index);
    item_color = std::get<0>(lookup);
    item_pattern = std::get<1>(lookup);

    if (item_color == nullptr) {
      return;
    }

    std::unique_ptr<DisplayColorDialog> display_dialog;
    if (item_pattern != nullptr) {
      display_dialog
          = std::make_unique<DisplayColorDialog>(*item_color, *item_pattern);
    } else {
      display_dialog = std::make_unique<DisplayColorDialog>(*item_color);
    }
    display_dialog->exec();
    QColor chosen_color = display_dialog->getSelectedColor();
    if (chosen_color.isValid()) {
      color_item->setIcon(makeSwatchIcon(chosen_color));
      *item_color = std::move(chosen_color);
      if (item_pattern != nullptr) {
        *item_pattern = display_dialog->getSelectedPattern();
      }
      view_->repaint();
      emit colorChanged();
    }
  }
}

// path is separated by "/", so setting Standard Cells, would be
// Instances/StdCells
void DisplayControls::setControlByPath(const std::string& path,
                                       bool is_visible,
                                       Qt::CheckState value)
{
  std::vector<QStandardItem*> items;
  findControlsInItems(path, is_visible ? kVisible : kSelectable, items);

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

// path is separated by "/", so setting Standard Cells, would be
// Instances/StdCells
void DisplayControls::setControlByPath(const std::string& path,
                                       const QColor& color)
{
  std::vector<QStandardItem*> items;
  findControlsInItems(path, kSwatch, items);

  if (items.empty()) {
    logger_->error(utl::GUI, 40, "Unable to find {} display control", path);
  } else {
    for (auto* item : items) {
      const auto& [item_color, item_style] = lookupColor(item);
      if (item_color == nullptr) {
        continue;
      }
      *item_color = color;
      item->setIcon(makeSwatchIcon(color));
    }
  }
  if (!items.empty()) {
    emit colorChanged();
  }
}

// path is separated by "/", so setting Standard Cells, would be
// Instances/StdCells
bool DisplayControls::checkControlByPath(const std::string& path,
                                         bool is_visible)
{
  std::vector<QStandardItem*> items;
  findControlsInItems(path, is_visible ? kVisible : kSelectable, items);

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
  }
  logger_->warn(utl::GUI,
                34,
                "Found {} controls matching {} at {}.",
                items.size(),
                path,
                is_visible ? "visible" : "select");
  return false;
}

void DisplayControls::collectControls(
    const QStandardItem* parent,
    Column column,
    std::map<std::string, QStandardItem*>& items,
    const std::string& prefix)
{
  for (int i = 0; i < parent->rowCount(); i++) {
    auto child = parent->child(i, kName);
    if (child != nullptr) {
      if (child->hasChildren()) {
        collectControls(
            child, column, items, prefix + child->text().toStdString() + "/");
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

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  QString regexPattern = QRegularExpression::wildcardToRegularExpression(
      QString::fromStdString(path));  // Defaults to exact match.

  // Create the QRegularExpression object with the case-insensitive option.
  const QRegularExpression path_compare(
      regexPattern, QRegularExpression::CaseInsensitiveOption);

  // Iterate and check for an exact match.
  for (auto& [item_path, item] : controls) {
    QRegularExpressionMatch match
        = path_compare.match(QString::fromStdString(item_path));
    if (match.hasMatch()) {
      items.push_back(item);
    }
  }
#else
  const QRegExp path_compare(
      QString::fromStdString(path), Qt::CaseInsensitive, QRegExp::Wildcard);
  for (auto& [item_path, item] : controls) {
    if (path_compare.exactMatch(QString::fromStdString(item_path))) {
      items.push_back(item);
    }
  }
#endif
}

void DisplayControls::save()
{
  saved_state_.clear();

  std::map<std::string, QStandardItem*> controls_visible;
  std::map<std::string, QStandardItem*> controls_selectable;
  collectControls(model_->invisibleRootItem(), kVisible, controls_visible);
  collectControls(
      model_->invisibleRootItem(), kSelectable, controls_selectable);

  for (auto& [control_name, control] : controls_visible) {
    saved_state_[control] = control->checkState();
  }
  for (auto& [control_name, control] : controls_selectable) {
    saved_state_[control] = control->checkState();
  }
}

void DisplayControls::restore()
{
  // Collect current controls in case some were removed after save was called.
  std::map<std::string, QStandardItem*> all_controls;
  collectControls(model_->invisibleRootItem(), kVisible, all_controls);
  collectControls(model_->invisibleRootItem(), kSelectable, all_controls);
  std::set<QStandardItem*> controls;

  for (auto& [control_name, control] : all_controls) {
    controls.insert(control);
  }

  for (auto& [control, state] : saved_state_) {
    if (controls.find(control) != controls.end()) {
      control->setCheckState(state);
    }
  }
}

void DisplayControls::addTech(odb::dbTech* tech)
{
  if (!tech) {
    return;
  }

  if (techs_.find(tech) != techs_.end()) {
    return;
  }

  techInit(tech);
  libInit(tech->getDb());

  for (dbTechLayer* layer : tech->getLayers()) {
    auto& row = layer_controls_[layer];
    QStandardItem* parent;
    Qt::CheckState checked;
    switch (layer->getType()) {
      case dbTechLayerType::ROUTING:
      case dbTechLayerType::CUT:
        parent = layers_group_.name;
        checked = Qt::Checked;
        break;
      case dbTechLayerType::IMPLANT:
        parent = layers_.implant.name;
        checked = Qt::Checked;
        break;
      default:
        parent = layers_.other.name;
        checked = Qt::Unchecked;
        break;
    }
    makeLeafItem(row,
                 QString::fromStdString(layer->getName()),
                 parent,
                 checked,
                 true,
                 color(layer),
                 QVariant::fromValue(layer));
  }

  if (layers_.implant.name->hasChildren()) {
    view_->setRowHidden(layers_.implant.name->row(),
                        layers_group_.name->index(),
                        !layers_.implant.name->hasChildren());
    toggleParent(layers_.implant);
  }
  if (layers_.other.name->hasChildren()) {
    view_->setRowHidden(layers_.other.name->row(),
                        layers_group_.name->index(),
                        !layers_.other.name->hasChildren());
    toggleParent(layers_.other);
  }

  toggleParent(layers_group_);

  for (int i = 0; i < 4; i++) {
    view_->resizeColumnToContents(i);
  }
  emit changed();
}

void DisplayControls::setLogger(utl::Logger* logger)
{
  logger_ = logger;
}

void DisplayControls::setSTA(sta::dbSta* sta)
{
  sta_ = sta;
  sta_->getDbNetwork()->addObserver(this);

  checkLiberty();
}

void DisplayControls::setDBInstDescriptor(DbInstDescriptor* desciptor)
{
  inst_descriptor_ = desciptor;
}

QStandardItem* DisplayControls::makeParentItem(ModelRow& row,
                                               const QString& text,
                                               QStandardItem* parent,
                                               Qt::CheckState checked,
                                               bool add_selectable,
                                               const QColor& color)
{
  makeLeafItem(row, text, parent, checked, add_selectable, color);

  row.visible->setData(QVariant::fromValue(Callback({[this, row](bool visible) {
                         toggleAllChildren(visible, row.name, kVisible);
                       }})),
                       kCallbackItemIdx);
  if (add_selectable) {
    row.selectable->setData(
        QVariant::fromValue(Callback({[this, row](bool selectable) {
          toggleAllChildren(selectable, row.name, kSelectable);
        }})),
        kCallbackItemIdx);
  }

  return row.name;
}

void DisplayControls::makeLeafItem(ModelRow& row,
                                   const QString& text,
                                   QStandardItem* parent,
                                   std::optional<Qt::CheckState> checked,
                                   bool add_selectable,
                                   const QColor& color,
                                   const QVariant& user_data)
{
  row.name = new QStandardItem(text);
  row.name->setEditable(false);
  if (user_data.isValid()) {
    row.name->setData(user_data, kUserDataItemIdx);
  }

  row.swatch = new QStandardItem(makeSwatchIcon(color), "");
  row.swatch->setEditable(false);
  row.swatch->setCheckable(false);
  if (user_data.isValid()) {
    row.swatch->setData(user_data, kUserDataItemIdx);
  }

  if (checked.has_value()) {
    row.visible = new QStandardItem("");
    row.visible->setCheckable(true);
    row.visible->setEditable(false);
    row.visible->setCheckState(checked.value());
  }

  if (add_selectable) {
    row.selectable = new QStandardItem("");
    row.selectable->setCheckable(true);
    row.selectable->setEditable(false);
    row.selectable->setCheckState(checked.value_or(Qt::Unchecked));
  }

  parent->appendRow({row.name, row.swatch, row.visible, row.selectable});
}

void DisplayControls::setNameItemDoubleClickAction(
    ModelRow& row,
    const std::function<void()>& callback)
{
  row.name->setData(QVariant::fromValue(callback), kDoubleclickItemIdx);

  QFont current_font = row.name->data(Qt::FontRole).value<QFont>();
  current_font.setUnderline(true);
  row.name->setData(current_font, Qt::FontRole);
}

void DisplayControls::setItemExclusivity(
    ModelRow& row,
    const std::set<std::string>& exclusivity)
{
  QSet<QString> names;
  for (const auto& name : exclusivity) {
    names.insert(QString::fromStdString(name));
  }
  row.visible->setData(QVariant::fromValue(names), kExclusivityItemIdx);
}

QIcon DisplayControls::makeSwatchIcon(const QColor& color)
{
  QPixmap swatch(20, 20);
  swatch.fill(color);

  return QIcon(swatch);
}

QColor DisplayControls::background()
{
  return background_color_;
}

QColor DisplayControls::color(const dbTechLayer* layer)
{
  auto it = layer_color_.find(layer);
  if (it != layer_color_.end()) {
    return it->second;
  }
  return QColor();
}

Qt::BrushStyle DisplayControls::pattern(const dbTechLayer* layer)
{
  auto it = layer_pattern_.find(layer);
  if (it != layer_pattern_.end()) {
    return it->second;
  }
  return Qt::NoBrush;
}

QColor DisplayControls::placementBlockageColor()
{
  return placement_blockage_color_;
}

Qt::BrushStyle DisplayControls::placementBlockagePattern()
{
  return placement_blockage_pattern_;
}

QColor DisplayControls::regionColor()
{
  return region_color_;
}

Qt::BrushStyle DisplayControls::regionPattern()
{
  return region_pattern_;
}

QColor DisplayControls::instanceNameColor()
{
  return instance_name_color_;
}

QFont DisplayControls::instanceNameFont()
{
  return instance_name_font_;
}

QColor DisplayControls::itermLabelColor()
{
  return iterm_label_color_;
}

QFont DisplayControls::itermLabelFont()
{
  return iterm_label_font_;
}

bool DisplayControls::isModelRowVisible(
    const DisplayControls::ModelRow* row) const
{
  if (row == nullptr) {
    return true;
  }
  return row->visible->checkState() != Qt::Unchecked;
}

bool DisplayControls::isModelRowSelectable(
    const DisplayControls::ModelRow* row) const
{
  if (row == nullptr) {
    return true;
  }
  return row->selectable->checkState() != Qt::Unchecked;
}

const DisplayControls::ModelRow* DisplayControls::getLayerRow(
    const dbTechLayer* layer) const
{
  auto it = layer_controls_.find(layer);
  if (it != layer_controls_.end()) {
    return &it->second;
  }
  return nullptr;
}

const DisplayControls::ModelRow* DisplayControls::getSiteRow(
    odb::dbSite* site) const
{
  auto it = site_controls_.find(site);
  if (it != site_controls_.end()) {
    return &it->second;
  }
  return nullptr;
}

bool DisplayControls::isVisible(const dbTechLayer* layer)
{
  auto* row = getLayerRow(layer);
  if (row == nullptr) {
    return false;
  }

  return isModelRowVisible(row);
}

bool DisplayControls::isSelectable(const dbTechLayer* layer)
{
  auto* row = getLayerRow(layer);
  if (row == nullptr) {
    return false;
  }

  return isModelRowSelectable(row);
}

bool DisplayControls::isInstanceVisible(odb::dbInst* inst)
{
  return isModelRowVisible(getInstRow(inst));
}

const DisplayControls::ModelRow* DisplayControls::getInstRow(
    odb::dbInst* inst) const
{
  switch (sta_->getInstanceType(inst)) {
    case sta::dbSta::InstType::BLOCK:
      return &instances_.blocks;
    case sta::dbSta::InstType::PAD:
      return &pad_instances_.other;
    case sta::dbSta::InstType::PAD_INPUT:
      return &pad_instances_.input;
    case sta::dbSta::InstType::PAD_OUTPUT:
      return &pad_instances_.output;
    case sta::dbSta::InstType::PAD_INOUT:
      return &pad_instances_.inout;
    case sta::dbSta::InstType::PAD_POWER:
      return &pad_instances_.power;
    case sta::dbSta::InstType::PAD_SPACER:
      return &pad_instances_.spacer;
    case sta::dbSta::InstType::PAD_AREAIO:
      return &pad_instances_.areaio;
    case sta::dbSta::InstType::ENDCAP:
      return &physical_instances_.endcap;
    case sta::dbSta::InstType::FILL:
      return &physical_instances_.fill;
    case sta::dbSta::InstType::TAPCELL:
      return &physical_instances_.tap;
    case sta::dbSta::InstType::BUMP:
      return &physical_instances_.bump;
    case sta::dbSta::InstType::COVER:
      return &physical_instances_.cover;
    case sta::dbSta::InstType::ANTENNA:
      return &physical_instances_.antenna;
    case sta::dbSta::InstType::TIE:
      return &physical_instances_.tie;
    case sta::dbSta::InstType::LEF_OTHER:
      return &physical_instances_.other;
    case sta::dbSta::InstType::STD_CELL:
      return &instances_.stdcells;
    case sta::dbSta::InstType::STD_INV: /* fallthru */
    case sta::dbSta::InstType::STD_BUF:
      return &bufinv_instances_.other;
    case sta::dbSta::InstType::STD_BUF_CLK_TREE: /* fallthru */
    case sta::dbSta::InstType::STD_INV_CLK_TREE:
      return &clock_tree_instances_.bufinv;
    case sta::dbSta::InstType::STD_BUF_TIMING_REPAIR: /* fallthru */
    case sta::dbSta::InstType::STD_INV_TIMING_REPAIR:
      return &bufinv_instances_.timing;
    case sta::dbSta::InstType::STD_CLOCK_GATE:
      return &clock_tree_instances_.clock_gates;
    case sta::dbSta::InstType::STD_LEVEL_SHIFT:
      return &stdcell_instances_.level_shiters;
    case sta::dbSta::InstType::STD_SEQUENTIAL:
      return &stdcell_instances_.sequential;
    case sta::dbSta::InstType::STD_PHYSICAL:
      return &instances_.physical;
    case sta::dbSta::InstType::STD_COMBINATIONAL:
      return &stdcell_instances_.combinational;
    case sta::dbSta::InstType::STD_OTHER:
      return &instance_group_;
  }

  return nullptr;
}

bool DisplayControls::isInstanceSelectable(odb::dbInst* inst)
{
  return isModelRowSelectable(getInstRow(inst));
}

const DisplayControls::ModelRow* DisplayControls::getNetRow(
    odb::dbNet* net) const
{
  switch (net->getSigType().getValue()) {
    case odb::dbSigType::SIGNAL:
      return &nets_.signal;
    case odb::dbSigType::POWER:
      return &nets_.power;
    case odb::dbSigType::GROUND:
      return &nets_.ground;
    case odb::dbSigType::CLOCK:
      return &nets_.clock;
    case odb::dbSigType::RESET:
      return &nets_.reset;
    case odb::dbSigType::TIEOFF:
      return &nets_.tieoff;
    case odb::dbSigType::SCAN:
      return &nets_.scan;
    case odb::dbSigType::ANALOG:
      return &nets_.analog;
  }

  return nullptr;
}

bool DisplayControls::isNetVisible(odb::dbNet* net)
{
  return isModelRowVisible(getNetRow(net));
}

bool DisplayControls::isNetSelectable(odb::dbNet* net)
{
  return isModelRowSelectable(getNetRow(net));
}

bool DisplayControls::areInstanceNamesVisible()
{
  return isModelRowVisible(&instance_shapes_.names);
}

bool DisplayControls::areInstancePinsVisible()
{
  return isModelRowVisible(&instance_shapes_.pins);
}

bool DisplayControls::areInstancePinsSelectable()
{
  return isModelRowSelectable(&instance_shapes_.pins);
}

bool DisplayControls::areInstancePinNamesVisible()
{
  return isModelRowVisible(&instance_shapes_.iterm_labels);
}

bool DisplayControls::areInstanceBlockagesVisible()
{
  return isModelRowVisible(&instance_shapes_.blockages);
}

bool DisplayControls::areRulersVisible()
{
  return isModelRowVisible(&rulers_);
}

bool DisplayControls::areRulersSelectable()
{
  return isModelRowSelectable(&rulers_);
}

QColor DisplayControls::rulerColor()
{
  return ruler_color_;
}

QFont DisplayControls::rulerFont()
{
  return ruler_font_;
}

bool DisplayControls::areLabelsVisible()
{
  return isModelRowVisible(&misc_.labels);
}

bool DisplayControls::areLabelsSelectable()
{
  return isModelRowSelectable(&misc_.labels);
}

QFont DisplayControls::labelFont()
{
  return label_font_;
}

bool DisplayControls::areBlockagesVisible()
{
  return isModelRowVisible(&blockages_.blockages);
}

bool DisplayControls::areBlockagesSelectable()
{
  return isModelRowSelectable(&blockages_.blockages);
}

bool DisplayControls::areObstructionsVisible()
{
  return isModelRowVisible(&blockages_.obstructions);
}

bool DisplayControls::areObstructionsSelectable()
{
  return isModelRowSelectable(&blockages_.obstructions);
}

bool DisplayControls::areRegionsSelectable() const
{
  return isModelRowSelectable(&misc_.regions);
}

bool DisplayControls::areSitesVisible()
{
  return isModelRowVisible(&site_group_);
}

bool DisplayControls::areSitesSelectable()
{
  return isModelRowSelectable(&site_group_);
}

bool DisplayControls::isSiteVisible(odb::dbSite* site)
{
  return isModelRowVisible(getSiteRow(site));
}

bool DisplayControls::isSiteSelectable(odb::dbSite* site)
{
  return isModelRowSelectable(getSiteRow(site));
}

QColor DisplayControls::siteColor(odb::dbSite* site)
{
  return site_color_[site];
}

bool DisplayControls::areSelectedVisible()
{
  return isModelRowVisible(&misc_.selected);
}

bool DisplayControls::isDetailedVisibility()
{
  return isModelRowVisible(&misc_.detailed);
}

bool DisplayControls::arePrefTracksVisible()
{
  return isModelRowVisible(&tracks_.pref);
}

bool DisplayControls::areNonPrefTracksVisible()
{
  return isModelRowVisible(&tracks_.non_pref);
}

bool DisplayControls::isScaleBarVisible() const
{
  return isModelRowVisible(&misc_.scale_bar);
}

bool DisplayControls::areAccessPointsVisible() const
{
  return isModelRowVisible(&misc_.access_points);
}

bool DisplayControls::areRegionsVisible() const
{
  return isModelRowVisible(&misc_.regions);
}

bool DisplayControls::isManufacturingGridVisible() const
{
  return isModelRowVisible(&misc_.manufacturing_grid);
}

bool DisplayControls::isModuleView() const
{
  return isModelRowVisible(&misc_.module);
}

bool DisplayControls::isGCellGridVisible() const
{
  return isModelRowVisible(&misc_.gcell_grid);
}

bool DisplayControls::isFlywireHighlightOnly() const
{
  return isModelRowVisible(&misc_.flywires_only);
}

bool DisplayControls::areFocusedNetsGuidesVisible() const
{
  return isModelRowVisible(&misc_.focused_nets_guides);
}

bool DisplayControls::areIOPinsVisible() const
{
  return isModelRowVisible(&shape_types_.pins);
}

bool DisplayControls::areIOPinsSelectable() const
{
  return isModelRowSelectable(&shape_types_.pins);
}

bool DisplayControls::areIOPinNamesVisible() const
{
  return isModelRowVisible(&shape_types_.pin_names);
}

bool DisplayControls::areRoutingSegmentsVisible() const
{
  return isModelRowVisible(&shape_types_.routing.segments);
}

bool DisplayControls::areRoutingViasVisible() const
{
  return isModelRowVisible(&shape_types_.routing.vias);
}

bool DisplayControls::areSpecialRoutingSegmentsVisible() const
{
  return isModelRowVisible(&shape_types_.special_routing.segments);
}

bool DisplayControls::areSpecialRoutingViasVisible() const
{
  return isModelRowVisible(&shape_types_.special_routing.vias);
}

bool DisplayControls::areFillsVisible() const
{
  return isModelRowVisible(&shape_types_.fill);
}

QFont DisplayControls::ioPinMarkersFont() const
{
  return pin_markers_font_;
}

void DisplayControls::registerRenderer(Renderer* renderer)
{
  if (custom_controls_.contains(renderer)) {
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
                       model_->invisibleRootItem(),
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
            parent_row.name = model_->item(parent_idx.row(), kName);
            parent_row.swatch = model_->item(parent_idx.row(), kSwatch);
            parent_row.visible = model_->item(parent_idx.row(), kVisible);
            parent_row.selectable = model_->item(parent_idx.row(), kSelectable);
            break;
          }
        }
      }
      if (parent_row.name == nullptr) {
        makeParentItem(parent_row,
                       parent_item_name,
                       model_->invisibleRootItem(),
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

  // Sort custom_controls
  std::vector<QList<QStandardItem*>> custom_controls;
  while (model_->invisibleRootItem()->rowCount() > custom_controls_start_) {
    custom_controls.push_back(
        model_->takeRow(model_->invisibleRootItem()->rowCount() - 1));
  }
  std::ranges::stable_sort(custom_controls,
                           [](const QList<QStandardItem*>& list0,
                              const QList<QStandardItem*>& list1) {
                             return list0[kName]->text() < list1[kName]->text();
                           });
  for (const auto& row : custom_controls) {
    model_->appendRow(row);
  }
}

void DisplayControls::unregisterRenderer(Renderer* renderer)
{
  saveRendererState(renderer);

  if (!custom_controls_.contains(renderer)) {
    return;
  }

  const auto& rows = custom_controls_[renderer];

  const QModelIndex& parent_idx = rows[0].name->index().parent();
  for (const auto& row : std::ranges::reverse_view(rows)) {
    // remove from Display controls
    auto index = model_->indexFromItem(row.name);
    model_->removeRow(index.row(), index.parent());
  }
  if (!model_->hasChildren(parent_idx)) {
    model_->removeRow(parent_idx.row(), parent_idx.parent());
  }

  custom_controls_.erase(renderer);
}

void DisplayControls::inDbRowCreate(odb::dbRow* row)
{
  libInit(row->getDb());
}

void DisplayControls::libInit(odb::dbDatabase* db)
{
  for (auto* lib : db->getLibs()) {
    for (auto* site : lib->getSites()) {
      if (site_controls_.find(site) == site_controls_.end()) {
        makeLeafItem(site_controls_[site],
                     QString::fromStdString(site->getName()),
                     site_group_.name,
                     site_group_.visible->checkState() == Qt::Checked
                         ? Qt::Checked
                         : Qt::Unchecked,
                     true,
                     default_site_color_,
                     QVariant::fromValue(site));
        site_color_[site] = default_site_color_;
      }
    }
  }

  toggleParent(site_group_);
}

void DisplayControls::techInit(odb::dbTech* tech)
{
  // disable if grid is not present
  misc_.manufacturing_grid.name->setEnabled(tech->hasManufacturingGrid());
  misc_.manufacturing_grid.visible->setEnabled(tech->hasManufacturingGrid());

  // Default colors
  // From http://vrl.cs.brown.edu/color seeded with #00F, #F00, #0D0
  const std::array<QColor, 14> default_metal_colors
      = {QColor(0, 0, 254),
         QColor(254, 0, 0),
         QColor(9, 221, 0),
         QColor(190, 244, 81),
         QColor(222, 33, 96),  // Metal 5
         QColor(32, 216, 253),
         QColor(253, 108, 160),
         QColor(117, 63, 194),
         QColor(128, 155, 49),
         QColor(234, 63, 252),  // Metal 10
         QColor(9, 96, 19),
         QColor(214, 120, 239),
         QColor(192, 222, 164),
         QColor(110, 68, 107)};  // Metal 14
  const std::array<QColor, 14> default_cut_colors
      = {QColor(126, 126, 255),
         QColor(255, 126, 126),
         QColor(4, 110, 0),
         QColor(95, 122, 40),
         QColor(111, 17, 48),  // Metal 5
         QColor(16, 108, 126),
         QColor(126, 54, 80),
         QColor(58, 32, 97),
         QColor(225, 255, 136),
         QColor(117, 32, 126),  // Metal 10
         QColor(18, 192, 38),
         QColor(107, 60, 119),
         QColor(96, 111, 82),
         QColor(220, 136, 214)};  // Metal 14
  int metal = 0;
  int via = 0;

  // ensure if random colors are used they are consistent
  std::mt19937 gen_color(1);

  auto generate_next_color = [&gen_color]() -> QColor {
    return QColor(
        50 + gen_color() % 200, 50 + gen_color() % 200, 50 + gen_color() % 200);
  };

  // Iterate through the layers and set default colors
  for (dbTechLayer* layer : tech->getLayers()) {
    dbTechLayerType type = layer->getType();
    QColor color;
    if (type == dbTechLayerType::ROUTING) {
      if (metal < default_metal_colors.size()) {
        color = default_metal_colors[metal++];
      } else {
        // pick a random color as we exceeded the built-in palette size
        color = generate_next_color();
      }
    } else if (type == dbTechLayerType::CUT) {
      if (via < default_cut_colors.size()) {
        if (metal != 0) {
          color = default_cut_colors[via++];
        } else {
          // via came first, so pick random color
          color = generate_next_color();
        }
      } else {
        // pick a random color as we exceeded the built-in palette size
        color = generate_next_color();
      }
    } else {
      // Do not draw from the existing palette so the metal layers can claim
      // those colors.
      color = generate_next_color();
    }
    color.setAlpha(180);
    layer_color_[layer] = std::move(color);
    layer_pattern_[layer] = Qt::SolidPattern;  // Default pattern is fill solid
  }
  techs_.insert(tech);
}

void DisplayControls::blockLoaded(odb::dbBlock* block)
{
  addTech(block->getTech());
}

void DisplayControls::setCurrentChip(odb::dbChip* chip)
{
  if (!chip) {
    return;
  }

  std::set<odb::dbTech*> visible_techs;

  std::function<void(odb::dbChip*)> collect_techs = [&](odb::dbChip* chip) {
    auto tech = chip->getTech();
    if (tech) {
      addTech(tech);
      visible_techs.insert(tech);
    }

    odb::dbBlock* block = chip->getBlock();
    if (block) {
      for (auto child : block->getChildren()) {
        visible_techs.insert(child->getTech());
      }
    }

    for (auto* inst : chip->getChipInsts()) {
      collect_techs(inst->getMasterChip());
    }
  };

  collect_techs(chip);

  for (auto& [layer, row] : layer_controls_) {
    const bool visible
        = visible_techs.find(layer->getTech()) != visible_techs.end();
    QModelIndex idx = model_->indexFromItem(row.name);
    view_->setRowHidden(idx.row(), idx.parent(), !visible);
  }
}

void DisplayControls::restoreTclCommands(std::vector<std::string>& cmds)
{
  buildRestoreTclCommands(cmds, model_->invisibleRootItem());
}

void DisplayControls::buildRestoreTclCommands(std::vector<std::string>& cmds,
                                              const QStandardItem* parent,
                                              const std::string& prefix)
{
  const std::string visible_restore
      = "gui::set_display_controls \"{}\" visible {}";
  const std::string selectable_restore
      = "gui::set_display_controls \"{}\" selectable {}";

  // loop over settings and save
  for (int r = 0; r < parent->rowCount(); r++) {
    QStandardItem* item = parent->child(r, 0);
    const std::string name = prefix + item->text().toStdString();

    if (item->hasChildren()) {
      buildRestoreTclCommands(cmds, item, name + "/");
    } else {
      auto* visible = parent->child(r, kVisible);
      if (visible) {
        bool vis = visible->checkState() == Qt::Checked;
        cmds.push_back(fmt::format(FMT_RUNTIME(visible_restore), name, vis));
      }
      auto* selectable = parent->child(r, kSelectable);
      if (selectable != nullptr) {
        bool select = selectable->checkState() == Qt::Checked;
        cmds.push_back(
            fmt::format(FMT_RUNTIME(selectable_restore), name, select));
      }
    }
  }
}

void DisplayControls::itemContextMenu(const QPoint& point)
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

  const QModelIndex name_index = model_->index(index.row(), kName, parent);
  auto* name_item = model_->itemFromIndex(name_index);
  layers_menu_layer_ = name_item->data(kUserDataItemIdx).value<dbTechLayer*>();

  if (layers_menu_layer_ != nullptr) {
    switch (layers_menu_layer_->getType()) {
      case dbTechLayerType::CUT:
      case dbTechLayerType::ROUTING:
        routing_layers_menu_->popup(view_->viewport()->mapToGlobal(point));
        break;
      default:
        layers_menu_->popup(view_->viewport()->mapToGlobal(point));
        break;
    }
  }
}

void DisplayControls::layerShowOnlySelectedNeighbors(int lower, int upper)
{
  if (layers_menu_layer_ == nullptr) {
    return;
  }

  std::set<const dbTechLayer*> layers;
  collectNeighboringLayers(layers_menu_layer_, lower, upper, layers);
  setOnlyVisibleLayers(layers);

  layers_menu_layer_ = nullptr;
}

void DisplayControls::collectNeighboringLayers(
    dbTechLayer* layer,
    int lower,
    int upper,
    std::set<const dbTechLayer*>& layers)
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

void DisplayControls::setOnlyVisibleLayers(
    const std::set<const dbTechLayer*>& layers)
{
  for (auto& [layer, row] : layer_controls_) {
    row.visible->setCheckState(Qt::Unchecked);
  }

  for (auto* layer : layers) {
    if (layer_controls_.contains(layer)) {
      layer_controls_[layer].visible->setCheckState(Qt::Checked);
    }
  }
}

void DisplayControls::postReadLiberty()
{
  checkLiberty(true);
}

void DisplayControls::postReadDb()
{
  emit changed();
}

void DisplayControls::checkLiberty(bool assume_loaded)
{
  bool enable = true;

  if (sta_ == nullptr) {
    enable = false;
  } else {
    if (!assume_loaded) {
      auto* network = sta_->getDbNetwork();
      if (network->defaultLibertyLibrary() == nullptr) {
        enable = false;
      }
    }
  }

  std::vector<ModelRow*> liberty_dependent_rows{
      &stdcell_instances_.bufinv,
      &stdcell_instances_.clock_tree,
      &stdcell_instances_.combinational,
      &stdcell_instances_.level_shiters,
      &stdcell_instances_.sequential,
      &bufinv_instances_.timing,
      &bufinv_instances_.other,
      &clock_tree_instances_.bufinv,
      &clock_tree_instances_.clock_gates};

  for (auto* row : liberty_dependent_rows) {
    auto* name = row->name;
    auto* visible = row->visible;
    auto* selectable = row->selectable;

    name->setEnabled(enable);
    visible->setEnabled(enable);
    if (selectable != nullptr) {
      selectable->setEnabled(enable);
    }
  }
}

bool DisplayControls::eventFilter(QObject* obj, QEvent* event)
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

}  // namespace gui
