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
#include <QDebug>
#include <QFileDialog>
#include <QHeaderView>
#include <QVBoxLayout>
#include <fstream>
#include <iomanip>
#include <map>
#include <regex>
#include <sstream>

#include "utl/Logger.h"

Q_DECLARE_METATYPE(gui::DRCViolation*);

namespace gui {

///////

DRCViolation::DRCViolation(const std::string& name,
                           const std::string& type,
                           const std::vector<std::any>& srcs,
                           const std::vector<DRCShape>& shapes,
                           odb::dbTechLayer* layer,
                           const std::string& comment,
                           int file_line)
    : name_(name),
      type_(type),
      srcs_(srcs),
      shapes_(shapes),
      layer_(layer),
      comment_(comment),
      viewed_(false),
      file_line_(file_line)
{
  computeBBox();
}

DRCViolation::DRCViolation(const std::string& name,
                           const std::string& type,
                           const std::vector<DRCShape>& shapes,
                           const std::string& comment,
                           int file_line)
    : DRCViolation(name, type, {}, shapes, nullptr, comment, file_line)
{
}

void DRCViolation::computeBBox()
{
  QPolygon outline;
  for (const auto& shape : shapes_) {
    if (auto s = std::get_if<DRCLine>(&shape)) {
      outline << QPoint((*s).first.x(), (*s).first.y());
      outline << QPoint((*s).second.x(), (*s).second.y());
    } else if (auto s = std::get_if<DRCRect>(&shape)) {
      outline = outline.united(
          QRect((*s).xMin(), (*s).yMin(), (*s).dx(), (*s).dy()));
    } else if (auto s = std::get_if<DRCPoly>(&shape)) {
      for (const auto& pt : *s) {
        outline << QPoint(pt.x(), pt.y());
      }
    }
  }

  QRect bounds = outline.boundingRect();
  bbox_
      = odb::Rect(bounds.left(), bounds.bottom(), bounds.right(), bounds.top());
}

void DRCViolation::paint(Painter& painter)
{
  for (const auto& shape : shapes_) {
    if (auto s = std::get_if<DRCLine>(&shape)) {
      const odb::Point& p1 = (*s).first;
      const odb::Point& p2 = (*s).second;
      painter.drawLine(p1.x(), p1.y(), p2.x(), p2.y());
    } else if (auto s = std::get_if<DRCRect>(&shape)) {
      painter.drawRect(*s);
    } else if (auto s = std::get_if<DRCPoly>(&shape)) {
      painter.drawPolygon(*s);
    }
  }
}

///////

std::string DRCDescriptor::getName(std::any object) const
{
  auto vio = std::any_cast<DRCViolation*>(object);
  return vio->getType();
}

std::string DRCDescriptor::getTypeName(std::any object) const
{
  return "DRC";
}

bool DRCDescriptor::getBBox(std::any object, odb::Rect& bbox) const
{
  auto vio = std::any_cast<DRCViolation*>(object);
  bbox = vio->getBBox();
  return true;
}

void DRCDescriptor::highlight(std::any object,
                              Painter& painter,
                              void* additional_data) const
{
  auto vio = std::any_cast<DRCViolation*>(object);
  vio->paint(painter);
}

Descriptor::Properties DRCDescriptor::getProperties(std::any object) const
{
  auto vio = std::any_cast<DRCViolation*>(object);
  Properties props;

  auto layer = vio->getLayer();
  if (layer != nullptr) {
    props.push_back({"Layer", layer->getName()});
  }

  auto srcs = vio->getSources();
  if (!srcs.empty()) {
    auto gui = Gui::get();
    SelectionSet sources;
    for (auto src : srcs) {
      auto select = gui->makeSelected(src);
      if (select) {
        sources.insert(select);
      }
    }
    props.push_back({"Sources", sources});
  }

  auto& comment = vio->getComment();
  if (!comment.empty()) {
    props.push_back({"Comment", vio->getComment()});
  }

  int line_number = vio->getFileLine();
  if (line_number != 0) {
    props.push_back({"Line number:", line_number});
  }

  return props;
}

Selected DRCDescriptor::makeSelected(std::any object,
                                     void* additional_data) const
{
  if (auto vio = std::any_cast<DRCViolation*>(&object)) {
    return Selected(*vio, this, additional_data);
  }
  return Selected();
}

bool DRCDescriptor::lessThan(std::any l, std::any r) const
{
  auto l_drc = std::any_cast<DRCViolation*>(l);
  auto r_drc = std::any_cast<DRCViolation*>(r);
  return l_drc < r_drc;
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
      view_(new QTreeView(this)),
      model_(new DRCItemModel(this)),
      block_(nullptr),
      load_(new QPushButton("Load..."))
{
  setObjectName("drc_viewer");  // for settings

  model_->setHorizontalHeaderLabels({"Type", "Violation"});
  view_->setModel(model_);

  QHeaderView* header = view_->header();
  header->setSectionResizeMode(0, QHeaderView::ResizeToContents);
  header->setSectionResizeMode(1, QHeaderView::Stretch);

  QWidget* container = new QWidget;
  QVBoxLayout* layout = new QVBoxLayout;
  layout->addWidget(view_);
  layout->addWidget(load_);

  container->setLayout(layout);
  setWidget(container);

  connect(view_,
          SIGNAL(clicked(const QModelIndex&)),
          this,
          SLOT(clicked(const QModelIndex&)));
  connect(view_->selectionModel(),
          SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
          this,
          SLOT(selectionChanged(const QItemSelection&, const QItemSelection&)));
  connect(load_, SIGNAL(released()), this, SLOT(selectReport()));
}

void DRCWidget::setLogger(utl::Logger* logger)
{
  logger_ = logger;
}

void DRCWidget::selectReport()
{
  QString filename = QFileDialog::getOpenFileName(
      this,
      tr("DRC Report"),
      QString(),
      tr("DRC Report (*.rpt *.ascii);;TritonRoute Report (*.rpt);;RVE (*.ascii);;All (*)"));
  if (!filename.isEmpty()) {
    loadReport(filename);
  }
}

void DRCWidget::selectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
  auto indexes = selected.indexes();
  if (indexes.isEmpty()) {
    return;
  }

  emit clicked(indexes.first());
}

void DRCWidget::clicked(const QModelIndex& index)
{
  QStandardItem* item = model_->itemFromIndex(index);
  QVariant data = item->data();
  if (data.isValid()) {
    auto violation = data.value<DRCViolation*>();
    if (qGuiApp->keyboardModifiers() & Qt::ControlModifier) {
      violation->clearViewed();
    } else {
      Selected t = Gui::get()->makeSelected(violation);
      emit selectDRC(t);
    }
  }
}

void DRCWidget::setBlock(odb::dbBlock* block)
{
  block_ = block;
}

void DRCWidget::showEvent(QShowEvent* event)
{
  toggleRenderer(true);
}

void DRCWidget::hideEvent(QHideEvent* event)
{
  toggleRenderer(false);
}

void DRCWidget::toggleRenderer(bool visible)
{
  auto gui = Gui::get();
  if (gui == nullptr) {
    return;
  }

  if (visible) {
    gui->registerRenderer(this);
  } else {
    gui->unregisterRenderer(this);
  }
}

void DRCWidget::updateModel()
{
  auto makeItem = [](const QString& text) {
    QStandardItem* item = new QStandardItem(text);
    item->setEditable(false);
    item->setSelectable(false);
    return item;
  };

  model_->removeRows(0, model_->rowCount());

  std::map<std::string, std::vector<DRCViolation*>> violation_by_type;
  for (const auto& violation : violations_) {
    violation_by_type[violation->getType()].push_back(violation.get());
  }

  for (const auto& [type, violation_list] : violation_by_type) {
    QStandardItem* type_group = makeItem(QString::fromStdString(type));

    int violation_idx = 1;
    for (const auto& violation : violation_list) {
      QStandardItem* violation_item
          = makeItem(QString::fromStdString(violation->getName()));
      violation_item->setSelectable(true);
      violation_item->setData(QVariant::fromValue(violation));
      QStandardItem* violation_index
          = makeItem(QString::number(violation_idx++));
      violation_index->setData(QVariant::fromValue(violation));

      type_group->appendRow({violation_index, violation_item});
    }

    model_->appendRow(
        {type_group,
         makeItem(QString::number(violation_list.size()) + " violations")});
  }

  toggleRenderer(!this->isHidden());
  setRepaintRequired();
}

void DRCWidget::drawObjects(Painter& painter)
{
  int min_box = 20.0 / painter.getPixelsPerDBU();
  Painter::Color pen_color = Painter::white;
  Painter::Color brush_color = pen_color;
  brush_color.a = 50;

  painter.setPen(pen_color, true, 0);
  painter.setHashedBrush(brush_color);
  for (const auto& violation : violations_) {
    const odb::Rect& box = violation->getBBox();
    if (std::max(box.dx(), box.dy()) < min_box) {
      // box is too small to be useful, so draw X instead
      odb::Point center(box.xMin() + box.dx() / 2, box.yMin() + box.dy() / 2);
      painter.drawLine({center.x() - min_box / 2, center.y() - min_box / 2},
                       {center.x() + min_box / 2, center.y() + min_box / 2});
      painter.drawLine({center.x() - min_box / 2, center.y() + min_box / 2},
                       {center.x() + min_box / 2, center.y() - min_box / 2});
    } else {
      violation->paint(painter);
    }
  }
}

SelectionSet DRCWidget::select(odb::dbTechLayer* layer, const odb::Point& point)
{
  if (layer != nullptr) {
    return SelectionSet();
  }

  auto gui = Gui::get();

  SelectionSet selections;
  for (const auto& violation : violations_) {
    if (violation->getBBox().intersects(point)) {
      selections.insert(gui->makeSelected(violation.get()));
    }
  }
  return selections;
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
  violations_.clear();

  try {
    if (filename.endsWith(".rpt")) {
      loadTRReport(filename);
    } else if (filename.endsWith(".ascii")) {
      loadASCIIReport(filename);
    } else {
      logger_->error(utl::GUI,
                     32,
                     "Unable to determine type of {}",
                     filename.toStdString());
    }
  } catch (std::runtime_error&) {
  }  // catch errors

  updateModel();
}

void DRCWidget::loadTRReport(const QString& filename)
{
  std::ifstream report(filename.toStdString());
  if (!report.is_open()) {
    logger_->error(utl::GUI,
                   30,
                   "Unable to open TritonRoute DRC report: {}",
                   filename.toStdString());
  }

  std::regex violation_type("\\s*violation type: (.*)");
  std::regex srcs("\\s*srcs: (.*)");
  std::regex bbox_layer("\\s*bbox = (.*) on Layer (.*)");
  std::regex bbox_corners(
      "\\s*\\(\\s*(.*),\\s*(.*)\\s*\\)\\s*-\\s*\\(\\s*(.*),\\s*(.*)\\s*\\)");

  int line_number = 0;
  auto tech = block_->getDataBase()->getTech();
  while (!report.eof()) {
    std::string line;
    std::smatch base_match;

    // type of violation
    line_number++;
    std::getline(report, line);
    if (line.empty()) {
      continue;
    }

    int violation_line_number = line_number;
    std::string type;
    if (std::regex_match(line, base_match, violation_type)) {
      type = base_match[1].str();
    } else {
      logger_->error(
          utl::GUI, 45, "Unable to parse line as violation type (line: {}): {}", line_number, line);
    }

    // sources of violation
    line_number++;
    int source_line_number = line_number;
    std::getline(report, line);
    std::string sources;
    if (std::regex_match(line, base_match, srcs)) {
      sources = base_match[1].str();
    } else {
      logger_->error(
          utl::GUI, 46, "Unable to parse line as violation source (line: {}): {}", line_number, line);
    }

    line_number++;
    std::getline(report, line);
    // bounding box and layer
    if (!std::regex_match(line, base_match, bbox_layer)) {
      logger_->error(
          utl::GUI, 47, "Unable to parse line as violation location (line: {}): {}", line_number, line);
    }

    std::string bbox = base_match[1].str();
    odb::dbTechLayer* layer = tech->findLayer(base_match[2].str().c_str());
    if (layer == nullptr) {
      logger_->warn(
          utl::GUI, 40, "Unable to find tech layer (line: {}): {}", line_number, base_match[2].str());
    }

    odb::Rect rect;
    if (std::regex_match(bbox, base_match, bbox_corners)) {
      try {
        rect.set_xlo(std::stod(base_match[1].str())
                     * block_->getDbUnitsPerMicron());
        rect.set_ylo(std::stod(base_match[2].str())
                     * block_->getDbUnitsPerMicron());
        rect.set_xhi(std::stod(base_match[3].str())
                     * block_->getDbUnitsPerMicron());
        rect.set_yhi(std::stod(base_match[4].str())
                     * block_->getDbUnitsPerMicron());
      } catch (std::invalid_argument&) {
        logger_->error(utl::GUI, 48, "Unable to parse bounding box (line: {}): {}", line_number, bbox);
      } catch (std::out_of_range&) {
        logger_->error(utl::GUI, 49, "Unable to parse bounding box (line: {}): {}", line_number, bbox);
      }
    } else {
      logger_->error(utl::GUI, 50, "Unable to parse bounding box (line: {}): {}", line_number, bbox);
    }

    std::vector<std::any> srcs_list;
    std::stringstream srcs_stream(sources);
    std::string single_source;
    std::string comment = "";

    // split sources list
    while (getline(srcs_stream, single_source, ' ')) {
      if (single_source.empty()) {
        continue;
      }

      auto ident = single_source.find(":");
      std::string item_type = single_source.substr(0, ident);
      std::string item_name = single_source.substr(ident + 1);

      std::any item;

      if (item_type == "net") {
        odb::dbNet* net = block_->findNet(item_name.c_str());
        if (net != nullptr) {
          item = net;
        } else {
          logger_->warn(utl::GUI, 44, "Unable to find net (line: {}): {}", source_line_number, item_name);
        }
      } else if (item_type == "inst") {
        odb::dbInst* inst = block_->findInst(item_name.c_str());
        if (inst != nullptr) {
          item = inst;
        } else {
          logger_->warn(utl::GUI, 43, "Unable to find instance (line: {}): {}", source_line_number, item_name);
        }
      } else if (item_type == "iterm") {
        odb::dbITerm* iterm = block_->findITerm(item_name.c_str());
        if (iterm != nullptr) {
          item = iterm;
        } else {
          logger_->warn(utl::GUI, 42, "Unable to find iterm (line: {}): {}", source_line_number, item_name);
        }
      } else if (item_type == "bterm") {
        odb::dbBTerm* bterm = block_->findBTerm(item_name.c_str());
        if (bterm != nullptr) {
          item = bterm;
        } else {
          logger_->warn(utl::GUI, 41, "Unable to find bterm (line: {}): {}", source_line_number, item_name);
        }
      } else if (item_type == "obstruction") {
        bool found = false;
        if (layer != nullptr) {
          for (const auto& obs : block_->getObstructions()) {
            auto obs_bbox = obs->getBBox();
            if (obs_bbox->getTechLayer() == layer) {
              odb::Rect obs_rect;
              obs_bbox->getBox(obs_rect);
              if (obs_rect.intersects(rect)) {
                srcs_list.push_back(obs);
                found = true;
              }
            }
          }
        }
        if (!found) {
          logger_->warn(utl::GUI, 52, "Unable to find obstruction (line: {})", source_line_number);
        }
      } else {
        logger_->warn(utl::GUI, 51, "Unknown source type (line: {}): {}", source_line_number, item_type);
      }

      if (item.has_value()) {
        srcs_list.push_back(item);
      } else {
        if (!item_name.empty()) {
          comment += single_source + " ";
        }
      }
    }

    std::string name = "Layer: ";
    if (layer != nullptr) {
      name += layer->getName();
    } else {
      name += "<unknown>";
    }
    name += ", Sources: " + sources;

    std::vector<DRCViolation::DRCShape> shapes({rect});
    violations_.push_back(std::make_unique<DRCViolation>(
        name, type, srcs_list, shapes, layer, comment, violation_line_number));
  }

  report.close();
}

void DRCWidget::loadASCIIReport(const QString& filename)
{
  logger_->error(utl::GUI, 31, "ASCII databases not supported.");
}

}  // namespace gui
