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
#include <boost/property_tree/json_parser.hpp>
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
      file_line_(file_line),
      viewed_(false),
      visible_(true)
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
  const int min_box = 20.0 / painter.getPixelsPerDBU();

  const odb::Rect& box = getBBox();
  if (box.maxDXDY() < min_box) {
    // box is too small to be useful, so draw X instead
    odb::Point center(box.xMin() + box.dx() / 2, box.yMin() + box.dy() / 2);
    painter.drawX(center.x(), center.y(), min_box);
  } else {
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
  return vio->getType();
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
  Properties props;

  auto gui = Gui::get();

  auto layer = vio->getLayer();
  if (layer != nullptr) {
    props.push_back({"Layer", gui->makeSelected(layer)});
  }

  auto srcs = vio->getSources();
  if (!srcs.empty()) {
    SelectionSet sources;
    for (auto& src : srcs) {
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
      load_(new QPushButton("Load...", this)),
      renderer_(std::make_unique<DRCRenderer>(violations_))
{
  setObjectName("drc_viewer");  // for settings

  model_->setHorizontalHeaderLabels({"Type", "Violation"});
  view_->setModel(model_);

  QHeaderView* header = view_->header();
  header->setSectionResizeMode(0, QHeaderView::ResizeToContents);
  header->setSectionResizeMode(1, QHeaderView::Stretch);

  QWidget* container = new QWidget(this);
  QVBoxLayout* layout = new QVBoxLayout;
  layout->addWidget(view_);
  layout->addWidget(load_);

  container->setLayout(layout);
  setWidget(container);

  connect(view_, &ObjectTree::clicked, this, &DRCWidget::clicked);
  connect(view_->selectionModel(),
          &QItemSelectionModel::selectionChanged,
          this,
          &DRCWidget::selectionChanged);
  connect(load_, &QPushButton::released, this, &DRCWidget::selectReport);

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

  violations_.clear();

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
  std::ifstream report(filename.toStdString());
  if (!report.is_open()) {
    logger_->error(utl::GUI,
                   30,
                   "Unable to open TritonRoute DRC report: {}",
                   filename.toStdString());
  }

  std::regex violation_type("\\s*violation type: (.*)");
  std::regex srcs("\\s*srcs: (.*)");
  std::regex congestion_line("\\s*congestion information: (.*)");
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
      logger_->error(utl::GUI,
                     45,
                     "Unable to parse line as violation type (line: {}): {}",
                     line_number,
                     line);
    }

    // sources of violation
    line_number++;
    int source_line_number = line_number;
    std::getline(report, line);
    std::string sources;
    if (std::regex_match(line, base_match, srcs)) {
      sources = base_match[1].str();
    } else {
      logger_->error(utl::GUI,
                     46,
                     "Unable to parse line as violation source (line: {}): {}",
                     line_number,
                     line);
    }

    line_number++;
    std::getline(report, line);
    std::string congestion_information;

    // congestion information (optional)
    if (std::regex_match(line, base_match, congestion_line)) {
      congestion_information = base_match[1].str();
      line_number++;
      std::getline(report, line);
    }

    // bounding box and layer
    if (!std::regex_match(line, base_match, bbox_layer)) {
      logger_->error(
          utl::GUI,
          47,
          "Unable to parse line as violation location (line: {}): {}",
          line_number,
          line);
    }

    std::string bbox = base_match[1].str();
    odb::dbTechLayer* layer = tech->findLayer(base_match[2].str().c_str());
    if (layer == nullptr && base_match[2].str() != "-") {
      logger_->warn(utl::GUI,
                    40,
                    "Unable to find tech layer (line: {}): {}",
                    line_number,
                    base_match[2].str());
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
        logger_->error(utl::GUI,
                       48,
                       "Unable to parse bounding box (line: {}): {}",
                       line_number,
                       bbox);
      } catch (std::out_of_range&) {
        logger_->error(utl::GUI,
                       49,
                       "Unable to parse bounding box (line: {}): {}",
                       line_number,
                       bbox);
      }
    } else {
      logger_->error(utl::GUI,
                     50,
                     "Unable to parse bounding box (line: {}): {}",
                     line_number,
                     bbox);
    }

    std::vector<std::any> srcs_list;
    std::stringstream srcs_stream(sources);
    std::string single_source;
    std::string comment;

    // split sources list
    while (getline(srcs_stream, single_source, ' ')) {
      if (single_source.empty()) {
        continue;
      }

      auto ident = single_source.find(':');
      std::string item_type = single_source.substr(0, ident);
      std::string item_name = single_source.substr(ident + 1);

      std::any item = nullptr;

      if (item_type == "net") {
        odb::dbNet* net = block_->findNet(item_name.c_str());
        if (net != nullptr) {
          item = net;
        } else {
          logger_->warn(utl::GUI,
                        44,
                        "Unable to find net (line: {}): {}",
                        source_line_number,
                        item_name);
        }
      } else if (item_type == "inst") {
        odb::dbInst* inst = block_->findInst(item_name.c_str());
        if (inst != nullptr) {
          item = inst;
        } else {
          logger_->warn(utl::GUI,
                        43,
                        "Unable to find instance (line: {}): {}",
                        source_line_number,
                        item_name);
        }
      } else if (item_type == "iterm") {
        odb::dbITerm* iterm = block_->findITerm(item_name.c_str());
        if (iterm != nullptr) {
          item = iterm;
        } else {
          logger_->warn(utl::GUI,
                        42,
                        "Unable to find iterm (line: {}): {}",
                        source_line_number,
                        item_name);
        }
      } else if (item_type == "bterm") {
        odb::dbBTerm* bterm = block_->findBTerm(item_name.c_str());
        if (bterm != nullptr) {
          item = bterm;
        } else {
          logger_->warn(utl::GUI,
                        41,
                        "Unable to find bterm (line: {}): {}",
                        source_line_number,
                        item_name);
        }
      } else if (item_type == "obstruction") {
        bool found = false;
        if (layer != nullptr) {
          for (const auto obs : block_->getObstructions()) {
            auto obs_bbox = obs->getBBox();
            if (obs_bbox->getTechLayer() == layer) {
              odb::Rect obs_rect = obs_bbox->getBox();
              if (obs_rect.intersects(rect)) {
                srcs_list.emplace_back(obs);
                found = true;
              }
            }
          }
        }
        if (!found) {
          logger_->warn(utl::GUI,
                        52,
                        "Unable to find obstruction (line: {})",
                        source_line_number);
        }
      } else {
        logger_->warn(utl::GUI,
                      51,
                      "Unknown source type (line: {}): {}",
                      source_line_number,
                      item_type);
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

    comment += congestion_information;

    std::vector<DRCViolation::DRCShape> shapes({rect});
    violations_.push_back(std::make_unique<DRCViolation>(
        name, type, srcs_list, shapes, layer, comment, violation_line_number));
  }

  report.close();
}

void DRCWidget::loadJSONReport(const QString& filename)
{
  boost::property_tree::ptree tree;
  try {
    boost::property_tree::json_parser::read_json(filename.toStdString(), tree);
  } catch (const boost::property_tree::json_parser_error& e1) {
    logger_->error(utl::GUI,
                   55,
                   "Unable to parse JSON file {}: {}",
                   filename.toStdString(),
                   e1.what());
  }
  auto tech = block_->getDataBase()->getTech();
  int dbUnits = block_->getDataBase()->getTech()->getDbUnitsPerMicron();
  // check if DRC key exists
  if (tree.find("DRC") == tree.not_found()) {
    logger_->error(utl::GUI,
                   87,
                   "Unable to find the DRC key in JSON file {}",
                   filename.toStdString());
  }
  for (const auto& rule : tree.get_child("DRC")) {
    auto& drc_rule = rule.second;

    const std::string violation_type = drc_rule.get<std::string>("name", "");
    const std::string violation_text
        = drc_rule.get<std::string>("description", "");
    int i = 0;
    auto violations_arr = drc_rule.get_child_optional("violations");
    if (!violations_arr) {
      logger_->error(utl::GUI,
                     86,
                     "Unable to find the violations key in JSON file {}",
                     filename.toStdString());
    }
    for (const auto& [_, violation] : violations_arr.value()) {
      std::string layer_str = violation.get<std::string>("layer", "-");
      const std::string shape_type = violation.get<std::string>("type", "-");
      odb::dbTechLayer* layer = nullptr;
      if (!layer_str.empty()) {
        layer = tech->findLayer(layer_str.c_str());
        if (layer == nullptr && layer_str != "-") {
          logger_->warn(
              utl::GUI, 79, "Unable to find tech layer: {}", layer_str);
        }
      }

      std::vector<odb::Point> shape_points;
      auto shape = violation.get_child_optional("shape");
      if (shape) {
        for (const auto& [_, pt] : shape.value()) {
          double x = pt.get<double>("x", 0.0);
          double y = pt.get<double>("y", 0.0);
          shape_points.emplace_back(x * dbUnits, y * dbUnits);
        }
      } else {
        logger_->warn(utl::GUI, 99, "Unable to find shape of violation");
      }

      std::vector<DRCViolation::DRCShape> shapes;
      if (shape_type == "box") {
        shapes.emplace_back(
            DRCViolation::DRCRect(shape_points[0], shape_points[1]));
      } else if (shape_type == "edge") {
        shapes.emplace_back(
            DRCViolation::DRCLine(shape_points[0], shape_points[1]));
      } else if (shape_type == "polygon") {
        shapes.emplace_back(DRCViolation::DRCPoly(shape_points));
      } else {
        logger_->error(
            utl::GUI, 58, "Unable to parse violation shape: {}", shape_type);
      }
      std::vector<std::any> srcs_list;
      auto sources_arr = violation.get_child_optional("sources");
      if (sources_arr) {
        for (const auto& [_, src] : sources_arr.value()) {
          std::string src_type = src.get<std::string>("type", "-");
          std::string src_name = src.get<std::string>("name", "-");

          std::any item = nullptr;
          if (src_type == "net") {
            odb::dbNet* net = block_->findNet(src_name.c_str());
            if (net != nullptr) {
              item = net;
            } else {
              logger_->warn(utl::GUI, 85, "Unable to find net: {}", src_name);
            }
          } else if (src_type == "inst") {
            odb::dbInst* inst = block_->findInst(src_name.c_str());
            if (inst != nullptr) {
              item = inst;
            } else {
              logger_->warn(
                  utl::GUI, 84, "Unable to find instance: {}", src_name);
            }
          } else if (src_type == "iterm") {
            odb::dbITerm* iterm = block_->findITerm(src_name.c_str());
            if (iterm != nullptr) {
              item = iterm;
            } else {
              logger_->warn(utl::GUI, 83, "Unable to find iterm: {}", src_name);
            }
          } else if (src_type == "bterm") {
            odb::dbBTerm* bterm = block_->findBTerm(src_name.c_str());
            if (bterm != nullptr) {
              item = bterm;
            } else {
              logger_->warn(utl::GUI, 82, "Unable to find bterm: {}", src_name);
            }
          } else if (src_type == "obstruction") {
            if (layer != nullptr) {
              for (const auto obs : block_->getObstructions()) {
                auto obs_bbox = obs->getBBox();
                if (obs_bbox->getTechLayer() == layer) {
                  // TODO: check if obs_rect is inside shape
                  srcs_list.emplace_back(obs);
                }
              }
            }
          } else {
            logger_->warn(utl::GUI, 81, "Unknown source type: {}", src_type);
          }

          if (item.has_value()) {
            srcs_list.push_back(item);
          } else {
            if (!src_name.empty()) {
              logger_->warn(
                  utl::GUI, 80, "Failed to add source item: {}", src_name);
            }
          }
        }
      }

      std::string name = violation_type + " - " + std::to_string(++i);
      violations_.push_back(std::make_unique<DRCViolation>(
          name, violation_type, srcs_list, shapes, layer, violation_text, 0));
    }
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
