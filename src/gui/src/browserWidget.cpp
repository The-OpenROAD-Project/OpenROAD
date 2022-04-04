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

#include "browserWidget.h"

#include <QHeaderView>

#include "utl/Logger.h"

Q_DECLARE_METATYPE(odb::dbInst*);
Q_DECLARE_METATYPE(odb::dbModule*);

namespace gui {

///////

BrowserWidget::BrowserWidget(QWidget* parent)
    : QDockWidget("Hierarchy Browser", parent),
      block_(nullptr),
      view_(new QTreeView(this)),
      model_(new QStandardItemModel(this))
{
  setObjectName("hierarchy_viewer");  // for settings

  model_->setHorizontalHeaderLabels({"Instance", "Master", "Area"});
  view_->setModel(model_);

  QHeaderView* header = view_->header();
  header->setStretchLastSection(false);
  header->setSectionResizeMode(0, QHeaderView::Stretch);
  header->setSectionResizeMode(1, QHeaderView::ResizeToContents);
  header->setSectionResizeMode(2, QHeaderView::ResizeToContents);

  setWidget(view_);

  connect(view_,
          SIGNAL(clicked(const QModelIndex&)),
          this,
          SLOT(clicked(const QModelIndex&)));
  connect(view_->selectionModel(),
          SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
          this,
          SLOT(selectionChanged(const QItemSelection&, const QItemSelection&)));
}

void BrowserWidget::selectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
  auto indexes = selected.indexes();
  if (indexes.isEmpty()) {
    return;
  }

  emit clicked(indexes.first());
}

void BrowserWidget::clicked(const QModelIndex& index)
{
  QStandardItem* item = model_->itemFromIndex(index);
  QVariant data = item->data();
  if (data.isValid()) {
    auto* gui = Gui::get();
    auto* inst = data.value<odb::dbInst*>();
    if (inst != nullptr) {
      emit select(gui->makeSelected(inst));
    } else {
      auto* module = data.value<odb::dbModule*>();
      if (module != nullptr) {
        emit select(gui->makeSelected(module));
      }
    }
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
  updateModel();
}

void BrowserWidget::hideEvent(QHideEvent* event)
{
  removeOwner();
  clearModel();
}

void BrowserWidget::updateModel()
{
  clearModel();

  if (block_ == nullptr) {
    return;
  }

  auto* root = model_->invisibleRootItem();
  populateModule(block_->getTopModule(), root);

  QStandardItem* orphans = new QStandardItem("Physical only");
  orphans->setEditable(false);
  orphans->setSelectable(false);
  double area = 0;
  for (auto* inst : block_->getInsts()) {
    if (inst->getModule() != nullptr) {
      continue;
    }

    area += addInstanceItem(inst, orphans);
  }
  root->appendRow({orphans, new QStandardItem, makeSizeItem(area)});
}

void BrowserWidget::clearModel()
{
  model_->removeRows(0, model_->rowCount());
}

int64_t BrowserWidget::populateModule(odb::dbModule* module, QStandardItem* parent)
{
  int64_t leaf_area = 0;

  auto* leafs = new QStandardItem("Leaf instances");
  leafs->setEditable(false);
  leafs->setSelectable(false);
  for (auto* inst : module->getInsts()) {
    leaf_area += addInstanceItem(inst, leafs);
  }

  int64_t area = 0;
  for (auto* child : module->getChildren()) {
    area += addModuleItem(child->getMaster(), parent);
  }

  parent->appendRow({leafs, new QStandardItem, makeSizeItem(leaf_area)});

  return leaf_area + area;
}

int64_t BrowserWidget::addInstanceItem(odb::dbInst* inst, QStandardItem* parent)
{
  QStandardItem* item = new QStandardItem(inst->getConstName());
  item->setEditable(false);
  item->setSelectable(true);
  item->setData(QVariant::fromValue(inst));

  auto* box = inst->getBBox();
  int64_t area = box->getDX() * box->getDY();

  parent->appendRow({item, makeMasterItem(inst->getMaster()->getConstName()), makeSizeItem(area)});

  return area;
}

int64_t BrowserWidget::addModuleItem(odb::dbModule* module, QStandardItem* parent)
{
  QStandardItem* item = new QStandardItem(QString::fromStdString(module->getHierarchicalName()));
  item->setEditable(false);
  item->setSelectable(true);
  item->setData(QVariant::fromValue(module));

  int64_t area = populateModule(module, item);

  parent->appendRow({item, makeMasterItem(module->getName()), makeSizeItem(area)});

  return area;
}

QStandardItem* BrowserWidget::makeSizeItem(int64_t area) const
{
  double scale_to_um = block_->getDbUnitsPerMicron() * block_->getDbUnitsPerMicron();

  std::string units = "\u03BC"; // mu
  double disp_area = area / scale_to_um;
  if (disp_area > 10e6) {
    disp_area /= (1e3 * 1e3);
    units = "m";
  }

  auto text = fmt::format("{:.3f}", disp_area) + " " + units + "m\u00B2"; // m2

  QStandardItem* size = new QStandardItem(QString::fromStdString(text));
  size->setEditable(false);
  size->setData(Qt::AlignRight, Qt::TextAlignmentRole);

  return size;
}

QStandardItem* BrowserWidget::makeMasterItem(const std::string& name) const
{
  QStandardItem* master = new QStandardItem(QString::fromStdString(name));
  master->setEditable(false);

  return master;
}

void BrowserWidget::inDbInstCreate(odb::dbInst*)
{
  updateModel();
}

void BrowserWidget::inDbInstCreate(odb::dbInst*, odb::dbRegion*)
{
  updateModel();
}

void BrowserWidget::inDbInstDestroy(odb::dbInst*)
{
  updateModel();
}

void BrowserWidget::inDbInstSwapMasterAfter(odb::dbInst*)
{
  updateModel();
}

}  // namespace gui
