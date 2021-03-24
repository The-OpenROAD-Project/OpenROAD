///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2019, OpenROAD
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

#include "displayControls.h"

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
#include "openroad/InitOpenRoad.hh"

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
      tech_inited_(false),
      fills_visible_(false),
      tracks_visible_pref_(false),
      tracks_visible_non_pref_(false),
      rows_visible_(false),
      nets_signal_visible_(true),
      nets_special_visible_(true),
      nets_power_visible_(true),
      nets_ground_visible_(true),
      nets_clock_visible_(true),
      congestion_visible_(false),
      pin_markers_visible_(true)

{
  setObjectName("layers");  // for settings
  model_->setHorizontalHeaderLabels({"", "C", "V", "S"});
  view_->setModel(model_);

  QHeaderView* header = view_->header();
  header->setSectionResizeMode(Name, QHeaderView::Stretch);
  header->setSectionResizeMode(Swatch, QHeaderView::ResizeToContents);
  header->setSectionResizeMode(Visible, QHeaderView::ResizeToContents);
  header->setSectionResizeMode(Selectable, QHeaderView::ResizeToContents);

  layers_ = makeItem(
      "Layers",
      model_,
      Qt::Checked,
      [this](bool visible) { toggleAllChildren(visible, layers_, Visible); },
      [this](bool selectable) {
        toggleAllChildren(selectable, layers_, Selectable);
      });
  view_->expand(layers_->index());

  // nets patterns
  nets_ = makeItem("Nets", model_, Qt::Checked, [this](bool visible) {
    toggleAllChildren(visible, nets_, Visible);
  });

  nets_signal_ = makeItem("Signal", nets_, Qt::Checked, [this](bool visible) {
    nets_signal_visible_ = visible;
  });

  nets_special_ = makeItem("Special", nets_, Qt::Checked, [this](bool visible) {
    nets_special_visible_ = visible;
  });

  nets_power_ = makeItem("Power", nets_, Qt::Checked, [this](bool visible) {
    nets_power_visible_ = visible;
  });

  nets_ground_ = makeItem("Ground", nets_, Qt::Checked, [this](bool visible) {
    nets_ground_visible_ = visible;
  });

  nets_clock_ = makeItem("Clock", nets_, Qt::Checked, [this](bool visible) {
    nets_clock_visible_ = visible;
  });

  // Rows
  rows_ = makeItem("Rows", model_, Qt::Unchecked, [this](bool visible) {
    rows_visible_ = visible;
  });

  // Rows
  congestion_map_
      = makeItem("Congestion Map", model_, Qt::Unchecked, [this](bool visible) {
          congestion_visible_ = visible;
        });
  pin_markers_
      = makeItem("Pin Markers", model_, Qt::Checked, [this](bool visible) {
          pin_markers_visible_ = visible;
        });

  // Track patterns
  tracks_ = makeItem("Tracks", model_, Qt::Unchecked, [this](bool visible) {
    toggleAllChildren(visible, tracks_, Visible);
  });

  tracks_pref_ = makeItem("Pref", tracks_, Qt::Unchecked, [this](bool visible) {
    tracks_visible_pref_ = visible;
  });

  tracks_non_pref_
      = makeItem("Non Pref", tracks_, Qt::Unchecked, [this](bool visible) {
          tracks_visible_non_pref_ = visible;
        });

  // Track patterns
  misc_ = makeItem("Misc", model_, Qt::Unchecked, [this](bool visible) {
    toggleAllChildren(visible, misc_, Visible);
  });

  fills_ = makeItem("Fills", misc_, Qt::Unchecked, [this](bool visible) {
    fills_visible_ = visible;
  });

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

void DisplayControls::itemChanged(QStandardItem* item)
{
  if (item->isCheckable() == false) {
    emit changed();
    return;
  }
  bool checked = item->checkState() == Qt::Checked;
  Callback callback = item->data().value<Callback>();
  callback.action(checked);
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
          QString::fromStdString(layer->getName()),
          layers_,
          Qt::Checked,
          [this, layer](bool visible) { layer_visible_[layer] = visible; },
          [this, layer](bool select) { layer_selectable_[layer] = select; },
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
    const QString& text,
    T* parent,
    Qt::CheckState checked,
    const std::function<void(bool)>& visibility_action,
    const std::function<void(bool)>& select_action,
    const QColor& color,
    odb::dbTechLayer* tech_layer)
{
  QStandardItem* name_item = new QStandardItem(text);
  name_item->setEditable(false);

  QPixmap swatch(20, 20);
  swatch.fill(color);
  QStandardItem* color_item = new QStandardItem(QIcon(swatch), "");
  color_item->setEditable(false);
  color_item->setCheckable(false);
  if (tech_layer != nullptr) {
    QVariant tech_layer_data(
        QVariant::fromValue(static_cast<void*>(tech_layer)));
    color_item->setData(tech_layer_data, Qt::UserRole);
  }

  QStandardItem* visibility_item = new QStandardItem("");
  visibility_item->setCheckable(true);
  visibility_item->setEditable(false);
  visibility_item->setCheckState(checked);
  visibility_item->setData(QVariant::fromValue(Callback({visibility_action})));

  QStandardItem* select_item = nullptr;
  if (select_action) {
    select_item = new QStandardItem("");
    select_item->setCheckable(true);
    select_item->setCheckState(checked);
    select_item->setEditable(false);
    select_item->setData(QVariant::fromValue(Callback({select_action})));
  }

  parent->appendRow({name_item, color_item, visibility_item, select_item});
  return name_item;
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
  auto it = layer_visible_.find(layer);
  if (it != layer_visible_.end()) {
    return it->second;
  }
  return false;
}

bool DisplayControls::isNetVisible(odb::dbNet* net)
{
  switch (net->getSigType()) {
    case dbSigType::SIGNAL:
      return nets_signal_visible_;
    case dbSigType::POWER:
      return nets_power_visible_;
    case dbSigType::GROUND:
      return nets_ground_visible_;
    case dbSigType::CLOCK:
      return nets_clock_visible_;
    default:
      return true;
  }
}

bool DisplayControls::isSelectable(const odb::dbTechLayer* layer)
{
  auto it = layer_selectable_.find(layer);
  if (it != layer_selectable_.end()) {
    return it->second;
  }
  return false;
}

bool DisplayControls::areFillsVisible()
{
  return fills_visible_;
}

bool DisplayControls::areRowsVisible()
{
  return rows_visible_;
}

bool DisplayControls::arePrefTracksVisible()
{
  return tracks_visible_pref_;
}

bool DisplayControls::areNonPrefTracksVisible()
{
  return tracks_visible_non_pref_;
}

bool DisplayControls::isCongestionVisible() const
{
  return congestion_visible_;
}

bool DisplayControls::arePinMarkersVisible() const
{
  return pin_markers_visible_;
}

void DisplayControls::addCustomVisibilityControl(const std::string& name,
                                                 bool initially_visible)
{
  auto q_name = QString::fromStdString(name);
  auto checked = initially_visible ? Qt::Checked : Qt::Unchecked;
  auto item = makeItem(q_name, model_, checked, [this, name](bool visible) {
    custom_visibility_[name] = visible;
  });
  custom_visibility_[name] = initially_visible;
}

bool DisplayControls::checkCustomVisibilityControl(const std::string& name)
{
  return custom_visibility_[name];
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
  return congestion_dialog_->getMinCongestionToShow();
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
    layer_visible_[layer] = true;
    layer_selectable_[layer] = true;
  }

  tech_inited_ = true;
}

void DisplayControls::designLoaded(odb::dbBlock* block)
{
  setDb(block->getDb());
}

}  // namespace gui
