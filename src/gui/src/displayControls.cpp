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

#include <QHeaderView>
#include <QKeyEvent>
#include <QLineEdit>
#include <QSettings>
#include <QVBoxLayout>

#include "db.h"
#include "displayControls.h"
#include "openroad/InitOpenRoad.hh"

namespace gui {

using namespace odb;

DisplayControls::DisplayControls(QWidget* parent)
    : QDockWidget("Display Control", parent),
      view_(new QTreeView(parent)),
      model_(new QStandardItemModel(0, 4, parent)),
      tech_inited_(false),
      tracks_visible_pref_(false),
      tracks_visible_non_pref_(false)
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

  setWidget(view_);
  connect(model_,
          SIGNAL(itemChanged(QStandardItem*)),
          this,
          SLOT(itemChanged(QStandardItem*)));
}

void DisplayControls::toggleAllChildren(bool           checked,
                                        QStandardItem* parent,
                                        Column         column)
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
  bool     checked  = item->checkState() == Qt::Checked;
  Callback callback = item->data().value<Callback>();
  callback.action(checked);
  emit changed();
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
          color(layer));
    }
  }
  emit changed();
}

template <typename T>
QStandardItem* DisplayControls::makeItem(
    const QString&                   text,
    T*                               parent,
    Qt::CheckState                   checked,
    const std::function<void(bool)>& visibility_action,
    const std::function<void(bool)>& select_action,
    const QColor&                    color)
{
  QStandardItem* nameItem = new QStandardItem(text);

  QPixmap swatch(20, 20);
  swatch.fill(color);
  QStandardItem* colorItem = new QStandardItem(QIcon(swatch), "");

  QStandardItem* visibilityItem = new QStandardItem("");
  visibilityItem->setCheckable(true);
  visibilityItem->setCheckState(checked);
  visibilityItem->setData(QVariant::fromValue(Callback({visibility_action})));

  QStandardItem* selectItem = nullptr;
  if (select_action) {
    selectItem = new QStandardItem("");
    selectItem->setCheckable(true);
    selectItem->setCheckState(checked);
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

bool DisplayControls::isSelectable(const odb::dbTechLayer* layer)
{
  return layer_selectable_.at(layer);
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
  const QColor colors[]  = {QColor(0, 0, 254),
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
  const int    numColors = sizeof(colors) / sizeof(QColor);
  int          metal     = 0;
  int          via       = 0;

  // Iterate through the layers and set default colors
  for (dbTechLayer* layer : tech->getLayers()) {
    dbTechLayerType type = layer->getType();
    QColor          color;
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
    layer_color_[layer]   = color;
    layer_visible_[layer] = true;
  }

  tech_inited_ = true;
}

void DisplayControls::designLoaded(odb::dbBlock* block)
{
  setDb(block->getDb());
}

}  // namespace gui
