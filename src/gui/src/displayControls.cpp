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

#include <QColorDialog>
#include <QHeaderView>
#include <QKeyEvent>
#include <QLineEdit>
#include <QSettings>
#include <QVBoxLayout>

#include "db.h"
#include "openroad/InitOpenRoad.hh"

Q_DECLARE_METATYPE(odb::dbTechLayer*);

namespace gui {

using namespace odb;

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
      nets_clock_visible_(true)
{
  setObjectName("layers");  // for settings
  model_->setHorizontalHeaderLabels({"", "", "V", "S"});
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
    auto colorItem = model_->itemFromIndex(index);
    QVariant colorVariant = colorItem->data(Qt::UserRole);
    QColor colorVal = QColor(colorVariant.toString());

    QColor chosenColor = QColorDialog::getColor(colorVal);
    if (chosenColor.isValid()) {
      colorItem->setData(QVariant(chosenColor), Qt::UserRole);
      QPixmap swatch(20, 20);
      swatch.fill(chosenColor);
      colorItem->setIcon(QIcon(swatch));
      QVariant techLayerData = colorItem->data(Qt::UserRole + 1);
      if (techLayerData.isValid()) {
        auto techLayer
            = static_cast<odb::dbTechLayer*>(techLayerData.value<void*>());
        if (techLayer != nullptr) {
          layer_color_[techLayer] = chosenColor;
          view_->repaint();
          emit changed();
        }
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
          layer);
    }
  }
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
    odb::dbTechLayer* techLayer)
{
  QStandardItem* nameItem = new QStandardItem(text);
  nameItem->setEditable(false);

  QPixmap swatch(20, 20);
  swatch.fill(color);
  QStandardItem* colorItem = new QStandardItem(QIcon(swatch), "");
  QString colorName = color.name(QColor::HexArgb);
  colorItem->setData(QVariant(colorName), Qt::UserRole);
  colorItem->setEditable(false);
  colorItem->setCheckable(false);
  if (techLayer != nullptr) {
    QVariant techLayerData(QVariant::fromValue(static_cast<void*>(techLayer)));
    colorItem->setData(techLayerData, Qt::UserRole + 1);
  }

  QStandardItem* visibilityItem = new QStandardItem("");
  visibilityItem->setCheckable(true);
  visibilityItem->setEditable(false);
  visibilityItem->setCheckState(checked);
  visibilityItem->setData(QVariant::fromValue(Callback({visibility_action})));

  QStandardItem* selectItem = nullptr;
  if (select_action) {
    selectItem = new QStandardItem("");
    selectItem->setCheckable(true);
    selectItem->setCheckState(checked);
    selectItem->setEditable(false);
    selectItem->setData(QVariant::fromValue(Callback({select_action})));
  }

  parent->appendRow({nameItem, colorItem, visibilityItem, selectItem});
  return nameItem;
}

QColor DisplayControls::color(const odb::dbTechLayer* layer)
{
  return layer_color_.at(layer);
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
  const int numColors = sizeof(colors) / sizeof(QColor);
  int metal = 0;
  int via = 0;

  // Iterate through the layers and set default colors
  for (dbTechLayer* layer : tech->getLayers()) {
    dbTechLayerType type = layer->getType();
    QColor color;
    if (type == dbTechLayerType::ROUTING) {
      if (metal < numColors) {
        color = colors[metal++];
      } else {
        // pick a random color as we exceeded the built-in palette size
        color = QColor(50 + rand() % 200, 50 + rand() % 200, 50 + rand() % 200);
      }
    } else if (type == dbTechLayerType::CUT) {
      if (via < numColors) {
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
