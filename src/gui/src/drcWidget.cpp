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

#include "utl/Logger.h"

Q_DECLARE_METATYPE(gui::DRCViolation*);

namespace gui {

///////

DRCViolation::DRCViolation(const std::string& name, odb::dbMarker* marker)
    : name_(name), marker_(marker), viewed_(false), visible_(true)
{
  bbox_ = marker_->getBBox();
}

void DRCViolation::paint(Painter& painter)
{
  const int min_box = 20.0 / painter.getPixelsPerDBU();

  const odb::Rect& box = getBBox();
  if (box.maxDXDY() < min_box) {
    // box is too small to be useful, so draw X instead
    odb::Point center(box.xMin() + box.dx() / 2, box.yMin() + box.dy() / 2);
    painter.drawX(center.x(), center.y(), min_box);
  } else {
    for (const auto& shape : marker_->getShapes()) {
      if (std::holds_alternative<odb::Point>(shape)) {
        const odb::Point pt = std::get<odb::Point>(shape);
        painter.drawX(pt.x(), pt.y(), min_box);
      } else if (std::holds_alternative<odb::Line>(shape)) {
        const odb::Line line = std::get<odb::Line>(shape);
        painter.drawLine(line.pt0(), line.pt1());
      } else if (std::holds_alternative<odb::Rect>(shape)) {
        painter.drawRect(std::get<odb::Rect>(shape));
      } else {
        painter.drawPolygon(std::get<odb::Polygon>(shape));
        ;
      }
    }
  }
}

///////

DRCDescriptor::DRCDescriptor(
    const std::vector<std::unique_ptr<DRCViolation>>& violations)
    : violations_(violations)
{
}

std::string DRCDescriptor::getName(std::any object) const
{
  auto vio = std::any_cast<DRCViolation*>(object);
  return vio->getMarker()->getCategory()->getName();
}

std::string DRCDescriptor::getTypeName() const
{
  return "DRC";
}

bool DRCDescriptor::getBBox(std::any object, odb::Rect& bbox) const
{
  auto vio = std::any_cast<DRCViolation*>(object);
  bbox = vio->getBBox();
  return true;
}

void DRCDescriptor::highlight(std::any object, Painter& painter) const
{
  auto vio = std::any_cast<DRCViolation*>(object);
  vio->paint(painter);
}

Descriptor::Properties DRCDescriptor::getProperties(std::any object) const
{
  auto vio = std::any_cast<DRCViolation*>(object);
  odb::dbMarker* marker = vio->getMarker();
  Properties props;

  auto gui = Gui::get();

  auto layer = marker->getTechLayer();
  if (layer != nullptr) {
    props.push_back({"Layer", gui->makeSelected(layer)});
  }

  SelectionSet sources;
  for (auto* net : marker->getNets()) {
    auto select = gui->makeSelected(net);
    if (select) {
      sources.insert(select);
    }
  }
  for (auto* inst : marker->getInsts()) {
    auto select = gui->makeSelected(inst);
    if (select) {
      sources.insert(select);
    }
  }
  for (auto* iterm : marker->getITerms()) {
    auto select = gui->makeSelected(iterm);
    if (select) {
      sources.insert(select);
    }
  }
  for (auto* bterm : marker->getBTerms()) {
    auto select = gui->makeSelected(bterm);
    if (select) {
      sources.insert(select);
    }
  }
  for (auto* obs : marker->getObstructions()) {
    auto select = gui->makeSelected(obs);
    if (select) {
      sources.insert(select);
    }
  }
  if (!sources.empty()) {
    props.push_back({"Sources", sources});
  }

  const auto& comment = marker->getComment();
  if (!comment.empty()) {
    props.push_back({"Comment", comment});
  }

  int line_number = marker->getLineNumber();
  if (line_number > 0) {
    props.push_back({"Line number:", line_number});
  }

  return props;
}

Selected DRCDescriptor::makeSelected(std::any object) const
{
  if (auto vio = std::any_cast<DRCViolation*>(&object)) {
    return Selected(*vio, this);
  }
  return Selected();
}

bool DRCDescriptor::lessThan(std::any l, std::any r) const
{
  auto l_drc = std::any_cast<DRCViolation*>(l);
  auto r_drc = std::any_cast<DRCViolation*>(r);
  return l_drc < r_drc;
}

bool DRCDescriptor::getAllObjects(SelectionSet& objects) const
{
  for (auto& violation : violations_) {
    objects.insert(makeSelected(violation.get()));
  }
  return true;
}

///////

QVariant DRCItemModel::data(const QModelIndex& index, int role) const
{
  if (role == Qt::FontRole) {
    auto item_data = itemFromIndex(index)->data();
    if (item_data.isValid()) {
      DRCViolation* item = item_data.value<DRCViolation*>();
      QFont font = QApplication::font();
      font.setBold(!item->isViewed());
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
      marker_groups_(new QComboBox(this)),
      update_(new QPushButton("Update...", this)),
      load_(new QPushButton("Load...", this)),
      renderer_(std::make_unique<DRCRenderer>(violations_))
{
  setObjectName("drc_viewer");  // for settings

  model_->setHorizontalHeaderLabels({"Type", "Violation"});
  view_->setModel(model_);

  QHeaderView* header = view_->header();
  header->setSectionResizeMode(0, QHeaderView::ResizeToContents);
  header->setSectionResizeMode(1, QHeaderView::Stretch);

  QHBoxLayout* selection_layout = new QHBoxLayout;
  selection_layout->addWidget(marker_groups_);
  selection_layout->addWidget(load_);
  selection_layout->addWidget(update_);

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
  connect(
      update_, &QPushButton::released, this, &DRCWidget::updateMarkerGroups);
  connect(marker_groups_,
          qOverload<int>(&QComboBox::currentIndexChanged),
          this,
          &DRCWidget::updateModel);

  view_->setMouseTracking(true);
  connect(view_, &ObjectTree::entered, this, &DRCWidget::focusIndex);

  connect(view_, &ObjectTree::viewportEntered, this, &DRCWidget::defocus);
  connect(view_, &ObjectTree::mouseExited, this, &DRCWidget::defocus);

  Gui::get()->registerDescriptor<DRCViolation*>(new DRCDescriptor(violations_));
}

void DRCWidget::focusIndex(const QModelIndex& focus_index)
{
  defocus();

  QStandardItem* item = model_->itemFromIndex(focus_index);
  QVariant data = item->data();
  if (data.isValid()) {
    DRCViolation* violation = data.value<DRCViolation*>();
    emit focus(Gui::get()->makeSelected(violation));
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
    auto violation = data.value<DRCViolation*>();
    if (item->isCheckable() && violation->isVisible() != visible) {
      item->setCheckState(visible ? Qt::Checked : Qt::Unchecked);
      violation->setIsVisible(visible);
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
    auto violation = data.value<DRCViolation*>();
    const bool is_visible = item->checkState() == Qt::Checked;
    if (setVisibleDRC(item, is_visible, true)) {
      // do nothing, since its handled in function
    } else if (qGuiApp->keyboardModifiers() & Qt::ControlModifier) {
      violation->clearViewed();
    } else {
      Selected t = Gui::get()->makeSelected(violation);
      emit selectDRC(t);
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
  updateMarkerGroups();
}

void DRCWidget::showEvent(QShowEvent* event)
{
  updateMarkerGroups();
  toggleRenderer(true);
}

void DRCWidget::hideEvent(QHideEvent* event)
{
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
  violations_.clear();

  if (marker_groups_->currentIndex() > 0) {
    const std::string group_name = marker_groups_->currentText().toStdString();
    odb::dbMarkerGroup* group = block_->findMarkerGroup(group_name.c_str());
    for (auto* category : group->getMarkerCategorys()) {
      for (auto* marker : category->getMarkers()) {
        violations_.push_back(
            std::make_unique<DRCViolation>(category->getName(), marker));
      }
    }
  }

  auto makeItem = [](const QString& text) {
    QStandardItem* item = new QStandardItem(text);
    item->setEditable(false);
    item->setSelectable(false);
    return item;
  };

  model_->removeRows(0, model_->rowCount());

  std::map<std::string, std::vector<DRCViolation*>> violation_by_type;
  for (const auto& violation : violations_) {
    violation_by_type[violation->getMarker()->getCategory()->getName()]
        .push_back(violation.get());
  }

  for (const auto& [type, violation_list] : violation_by_type) {
    QStandardItem* type_group = makeItem(QString::fromStdString(type));
    type_group->setCheckable(true);
    type_group->setCheckState(Qt::Checked);

    int violation_idx = 1;
    for (const auto& violation : violation_list) {
      QStandardItem* violation_item
          = makeItem(QString::fromStdString(violation->getName()));
      violation_item->setSelectable(true);
      violation_item->setData(QVariant::fromValue(violation));
      QStandardItem* violation_index
          = makeItem(QString::number(violation_idx++));
      violation_index->setData(QVariant::fromValue(violation));
      violation_index->setCheckable(true);
      violation_index->setCheckState(violation->isVisible() ? Qt::Checked
                                                            : Qt::Unchecked);

      type_group->appendRow({violation_index, violation_item});
    }

    model_->appendRow(
        {type_group,
         makeItem(QString::number(violation_list.size()) + " violations")});
  }

  toggleRenderer(!this->isHidden());
  renderer_->redraw();
}

void DRCWidget::updateSelection(const Selected& selection)
{
  const std::any& object = selection.getObject();
  if (auto s = std::any_cast<DRCViolation*>(&object)) {
    (*s)->setViewed();
    emit update();
  }
}

void DRCWidget::loadReport(const QString& filename)
{
  Gui::get()->removeSelected<DRCViolation*>();

  try {
    // OpenLane uses .drc and OpenROAD-flow-scripts uses .rpt
    if (filename.endsWith(".rpt") || filename.endsWith(".drc")) {
      loadTRReport(filename);
    } else if (filename.endsWith(".json")) {
      loadJSONReport(filename);
    } else {
      logger_->error(utl::GUI,
                     32,
                     "Unable to determine type of {}",
                     filename.toStdString());
    }
  } catch (std::runtime_error&) {
  }  // catch errors

  updateModel();
  show();
  raise();
}

void DRCWidget::loadTRReport(const QString& filename)
{
  const std::string file = filename.toStdString();
  odb::dbMarkerGroup::fromTR(block_, "DRC", file.c_str());
}

void DRCWidget::loadJSONReport(const QString& filename)
{
  const std::string file = filename.toStdString();
  odb::dbMarkerGroup::fromJSON(block_, file.c_str());
}

void DRCWidget::updateMarkerGroups()
{
  if (block_ == nullptr) {
    return;
  }

  marker_groups_->clear();

  marker_groups_->addItem("");
  for (auto* group : block_->getMarkerGroups()) {
    marker_groups_->addItem(QString::fromStdString(group->getName()));
  }
}

////////

DRCRenderer::DRCRenderer(
    const std::vector<std::unique_ptr<DRCViolation>>& violations)
    : violations_(violations)
{
}

void DRCRenderer::drawObjects(Painter& painter)
{
  Painter::Color pen_color = Painter::white;
  Painter::Color brush_color = pen_color;
  brush_color.a = 50;

  painter.setPen(pen_color, true, 0);
  painter.setBrush(brush_color, Painter::Brush::DIAGONAL);
  for (const auto& violation : violations_) {
    if (!violation->isVisible()) {
      continue;
    }
    violation->paint(painter);
  }
}

SelectionSet DRCRenderer::select(odb::dbTechLayer* layer,
                                 const odb::Rect& region)
{
  if (layer != nullptr) {
    return SelectionSet();
  }

  auto gui = Gui::get();

  SelectionSet selections;
  for (const auto& violation : violations_) {
    if (!violation->isVisible()) {
      continue;
    }
    if (violation->getBBox().intersects(region)) {
      selections.insert(gui->makeSelected(violation.get()));
    }
  }
  return selections;
}

}  // namespace gui
