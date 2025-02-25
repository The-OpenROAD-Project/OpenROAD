//////////////////////////////////////////////////////////////////////////////
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

#include "timingWidget.h"

#include <QApplication>
#include <QClipboard>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QSortFilterProxyModel>
#include <QVBoxLayout>
#include <vector>

#include "db_sta/dbSta.hh"
#include "gui_utils.h"
#include "sta/Liberty.hh"
#include "staGui.h"

namespace gui {

TimingWidget::TimingWidget(QWidget* parent)
    : QDockWidget("Timing Report", parent),
      commands_menu_(new QMenu("Commands Menu", this)),
      setup_timing_table_view_(new QTableView(this)),
      hold_timing_table_view_(new QTableView(this)),
      path_details_table_view_(new QTableView(this)),
      capture_details_table_view_(new QTableView(this)),
      update_button_(new QPushButton("Update", this)),
      columns_control_container_(new QPushButton("Columns", this)),
      columns_control_(new QMenu("Columns", this)),
      settings_button_(new QPushButton("Settings", this)),
      settings_(new TimingControlsDialog(this)),
      setup_timing_paths_model_(nullptr),
      hold_timing_paths_model_(nullptr),
      path_details_model_(nullptr),
      capture_details_model_(nullptr),
      path_renderer_(std::make_unique<TimingPathRenderer>()),
      cone_renderer_(std::make_unique<TimingConeRenderer>()),
      dbchange_listener_(new GuiDBChangeListener(this)),
      delay_detail_splitter_(new QSplitter(Qt::Vertical, this)),
      delay_widget_(new QTabWidget(this)),
      detail_widget_(new QTabWidget(this)),
      focus_view_(nullptr)
{
  setObjectName("timing_report");  // for settings

  QWidget* container = new QWidget(this);
  QVBoxLayout* layout = new QVBoxLayout;

  QFrame* control_frame = new QFrame(this);
  control_frame->setFrameShape(QFrame::StyledPanel);
  control_frame->setFrameShadow(QFrame::Raised);

  QHBoxLayout* controls_layout = new QHBoxLayout;
  controls_layout->addWidget(settings_button_);
  controls_layout->addWidget(columns_control_container_);
  controls_layout->addWidget(update_button_);
  controls_layout->insertStretch(2);
  control_frame->setLayout(controls_layout);
  layout->addWidget(control_frame);

  // top half
  delay_widget_->addTab(setup_timing_table_view_, "Setup");
  delay_widget_->addTab(hold_timing_table_view_, "Hold");
  delay_detail_splitter_->addWidget(delay_widget_);

  // bottom half
  detail_widget_->addTab(path_details_table_view_, "Data Path Details");
  detail_widget_->addTab(capture_details_table_view_, "Capture Path Details");
  delay_detail_splitter_->addWidget(detail_widget_);

  layout->addWidget(delay_detail_splitter_);
  delay_detail_splitter_->setCollapsible(0, false);
  delay_detail_splitter_->setCollapsible(1, false);

  container->setLayout(layout);
  setWidget(container);

  addCommandsMenuActions();

  connect(dbchange_listener_,
          &GuiDBChangeListener::dbUpdated,
          this,
          &TimingWidget::handleDbChange);
  connect(update_button_,
          &QPushButton::clicked,
          this,
          &TimingWidget::populatePaths);
  connect(update_button_,
          &QPushButton::clicked,
          dbchange_listener_,
          &GuiDBChangeListener::reset);

  connect(settings_button_,
          &QPushButton::clicked,
          this,
          &TimingWidget::showSettings);

  connect(
      settings_, &TimingControlsDialog::inspect, this, &TimingWidget::inspect);

  connect(settings_,
          &TimingControlsDialog::expandClock,
          this,
          &TimingWidget::updateClockRows);
}

void TimingWidget::setColumnDisplayMenu()
{
  int column_index = 0;

  // Populate with all the available columns' actions.
  for (const auto& [column, name] : TimingPathsModel::getColumnNames()) {
    QAction* action = new QAction(name, this);
    action->setCheckable(true);
    action->setChecked(true);

    connect(action, &QAction::triggered, this, [=](bool checked) {
      hideColumn(column_index, checked);
    });

    columns_control_->addAction(action);

    // Uncheck boxes and hide columns based on settings.
    if (!initial_columns_visibility_.isEmpty()) {
      if (!initial_columns_visibility_[column_index]) {
        action->trigger();
      }
    }

    ++column_index;
  }

  columns_control_container_->setMenu(columns_control_);
}

void TimingWidget::hideColumn(const int index, const bool checked)
{
  setup_timing_table_view_->setColumnHidden(index, !checked);
  hold_timing_table_view_->setColumnHidden(index, !checked);
}

TimingWidget::~TimingWidget()
{
  dbchange_listener_->removeOwner();
}

void TimingWidget::init(sta::dbSta* sta)
{
  cone_renderer_->setSTA(sta);
  settings_->setSTA(sta);

  setup_timing_paths_model_
      = new TimingPathsModel(true, settings_->getSTA(), this);
  hold_timing_paths_model_
      = new TimingPathsModel(false, settings_->getSTA(), this);
  path_details_model_ = new TimingPathDetailModel(false, sta, this);
  capture_details_model_ = new TimingPathDetailModel(true, sta, this);

  auto setupTableView = [](QTableView* view, QAbstractTableModel* model) {
    view->setModel(model);
    view->setContextMenuPolicy(Qt::CustomContextMenu);
    view->setSelectionMode(QAbstractItemView::SingleSelection);
    view->setSelectionBehavior(QAbstractItemView::SelectRows);
    view->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    view->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
  };

  setupTableView(setup_timing_table_view_, setup_timing_paths_model_);
  setupTableView(hold_timing_table_view_, hold_timing_paths_model_);
  setupTableView(path_details_table_view_, path_details_model_);
  setupTableView(capture_details_table_view_, capture_details_model_);

  // default to sorting by slack
  setup_timing_table_view_->setSortingEnabled(true);
  setup_timing_table_view_->horizontalHeader()->setSortIndicator(
      3, Qt::AscendingOrder);
  hold_timing_table_view_->setSortingEnabled(true);
  hold_timing_table_view_->horizontalHeader()->setSortIndicator(
      3, Qt::AscendingOrder);

  setColumnDisplayMenu();

  connect(setup_timing_paths_model_,
          &TimingPathsModel::modelReset,
          this,
          &TimingWidget::modelWasReset);
  connect(hold_timing_paths_model_,
          &TimingPathsModel::modelReset,
          this,
          &TimingWidget::modelWasReset);

  connect(setup_timing_table_view_->horizontalHeader(),
          &QHeaderView::sortIndicatorChanged,
          setup_timing_table_view_->model(),
          &QAbstractItemModel::sort);
  connect(hold_timing_table_view_->horizontalHeader(),
          &QHeaderView::sortIndicatorChanged,
          hold_timing_table_view_->model(),
          &QAbstractItemModel::sort);

  connect(setup_timing_table_view_->selectionModel(),
          &QItemSelectionModel::selectionChanged,
          this,
          &TimingWidget::selectedRowChanged);

  connect(setup_timing_table_view_,
          &QTableView::customContextMenuRequested,
          this,
          &TimingWidget::showCommandsMenu);

  connect(hold_timing_table_view_->selectionModel(),
          &QItemSelectionModel::selectionChanged,
          this,
          &TimingWidget::selectedRowChanged);

  connect(hold_timing_table_view_,
          &QTableView::customContextMenuRequested,
          this,
          &TimingWidget::showCommandsMenu);

  connect(path_details_table_view_->selectionModel(),
          &QItemSelectionModel::selectionChanged,
          this,
          &TimingWidget::selectedDetailRowChanged);

  connect(capture_details_table_view_->selectionModel(),
          &QItemSelectionModel::selectionChanged,
          this,
          &TimingWidget::selectedCaptureRowChanged);

  connect(path_details_table_view_,
          &QTableView::doubleClicked,
          this,
          &TimingWidget::detailRowDoubleClicked);

  connect(capture_details_table_view_,
          &QTableView::doubleClicked,
          this,
          &TimingWidget::detailRowDoubleClicked);

  clearPathDetails();
}

void TimingWidget::updatePaths()
{
  update_button_->click();
}

void TimingWidget::readSettings(QSettings* settings)
{
  settings->beginGroup(objectName());

  settings_->setPathCount(
      settings->value("path_count", settings_->getPathCount()).toInt());
  settings_->setOnePathPerEndpoint(
      settings->value("one_path_per_endpoint").toBool());
  settings_->setExpandClock(
      settings->value("expand_clk", settings_->getExpandClock()).toBool());
  delay_detail_splitter_->restoreState(
      settings->value("splitter", delay_detail_splitter_->saveState())
          .toByteArray());

  setInitialColumnsVisibility(settings->value("columns_visibility"));

  settings->endGroup();
}

void TimingWidget::setInitialColumnsVisibility(
    const QVariant& columns_visibility)
{
  for (QVariant& index_visibility : columns_visibility.toList()) {
    initial_columns_visibility_.push_back(index_visibility.toBool());
  }
}

void TimingWidget::writeSettings(QSettings* settings)
{
  settings->beginGroup(objectName());

  settings->setValue("path_count", settings_->getPathCount());
  settings->setValue("one_path_per_endpoint",
                     settings_->getOnePathPerEndpoint());
  settings->setValue("expand_clk", settings_->getExpandClock());
  settings->setValue("splitter", delay_detail_splitter_->saveState());
  settings->setValue("columns_visibility", getColumnsVisibility());

  settings->endGroup();
}

QVariantList TimingWidget::getColumnsVisibility() const
{
  QVariantList column_visibility;

  for (int column_index = 0;
       column_index < setup_timing_paths_model_->columnCount();
       ++column_index) {
    // true -> visible
    column_visibility.push_back(
        !setup_timing_table_view_->isColumnHidden(column_index));
  }

  return column_visibility;
}

void TimingWidget::keyPressEvent(QKeyEvent* key_event)
{
  if (key_event->matches(QKeySequence::Copy)) {
    copy();
    key_event->accept();
  }
}

void TimingWidget::addCommandsMenuActions()
{
  QMenu* closest_match_menu = new QMenu("Closest Match", this);
  connect(closest_match_menu->addAction("Exact"), &QAction::triggered, [this] {
    writePathReportCommand(timing_paths_table_index_, EXACT);
  });
  connect(closest_match_menu->addAction("No Buffering"),
          &QAction::triggered,
          [this] {
            writePathReportCommand(timing_paths_table_index_, NO_BUFFERING);
          });
  commands_menu_->addMenu(closest_match_menu);

  connect(commands_menu_->addAction("From Start to End"),
          &QAction::triggered,
          [this] {
            writePathReportCommand(timing_paths_table_index_,
                                   FROM_START_TO_END);
          });
}

void TimingWidget::showCommandsMenu(const QPoint& pos)
{
  if (!focus_view_) {
    return;
  }

  timing_paths_table_index_ = focus_view_->indexAt(pos);

  commands_menu_->popup(focus_view_->viewport()->mapToGlobal(pos));
}

// The nodes must be written within curly braces to
// deal with characters like '$'
void TimingWidget::writePathReportCommand(const QModelIndex& selected_index,
                                          const CommandType& type)
{
  TimingPathsModel* focus_model
      = static_cast<TimingPathsModel*>(focus_view_->model());
  TimingPath* selected_path = focus_model->getPathAt(selected_index);

  QString command = "report_checks ";

  switch (type) {
    case FROM_START_TO_END: {
      command += generateFromStartToEndString(selected_path);
      break;
    }
    case NO_BUFFERING: {
      command += generateClosestMatchString(NO_BUFFERING, selected_path);
      break;
    }
    case EXACT: {
      command += generateClosestMatchString(EXACT, selected_path);
      break;
    }
  }

  emit setCommand(command);
}

QString TimingWidget::generateFromStartToEndString(TimingPath* path)
{
  QString start_node = QString::fromStdString(path->getStartStageName());
  QString end_node = QString::fromStdString(path->getEndStageName());

  return "-from " + Utils::wrapInCurly(start_node) + " -to "
         + Utils::wrapInCurly(end_node);
}

QString TimingWidget::generateClosestMatchString(CommandType type,
                                                 TimingPath* path)
{
  QString command;
  TimingNodeList* node_list = &path->getPathNodes();

  const int clock_end_idx = path->getClkPathEndIndex();
  int start_idx = clock_end_idx + 1;

  const bool only_clock_nodes = start_idx == node_list->size();
  if (only_clock_nodes) {  // Then we write the clock nodes
    start_idx = 0;

    // The first and last node must also be written with -through. The path
    // cannot be found otherwise.
    for (int i = start_idx; i < node_list->size(); i++) {
      command += (*node_list)[i]->isRisingEdge() ? " -rise_through "
                                                 : " -fall_through ";
      command += Utils::wrapInCurly(
          QString::fromStdString((*node_list)[i]->getNodeName()));
    }
  } else {
    command += (*node_list)[start_idx]->isRisingEdge() ? "-rise_from "
                                                       : "-fall_from ";
    command += Utils::wrapInCurly(
        QString::fromStdString(path->getStartStageName()));

    int prev_inst_fields_char_count = 0;
    sta::dbNetwork* network = settings_->getSTA()->getSTA()->getDbNetwork();
    bool skipped_inverter_pair = false;
    QString inst_fields;
    for (int i = (start_idx + 1); i < (node_list->size() - 1); i++) {
      TimingPathNode* curr_node = (*node_list)[i].get();
      odb::dbInst* curr_node_inst = curr_node->getInstance();
      if (type == NO_BUFFERING
          && network->libertyCell(curr_node_inst)->isBuffer()) {
        continue;
      }

      odb::dbInst* prev_node_inst = nullptr;
      if (curr_node != node_list->front().get()) {
        TimingPathNode* prev_node = (*node_list)[i - 1].get();
        prev_node_inst = prev_node->getInstance();
      }

      inst_fields
          += curr_node->isRisingEdge() ? " -rise_through " : " -fall_through ";
      inst_fields += Utils::wrapInCurly(
          QString::fromStdString(curr_node->getNodeName()));

      // We update the command string only at an
      // interface between two instances.
      if (curr_node_inst != prev_node_inst) {
        // Avoid messing up the command if there's another
        // inverter after an inverter pair.
        if (skipped_inverter_pair) {
          skipped_inverter_pair = false;
          prev_inst_fields_char_count = inst_fields.size();
          command += inst_fields;
          inst_fields.clear();
          continue;
        }

        if (type == NO_BUFFERING
            && network->libertyCell(curr_node_inst)->isInverter()
            && network->libertyCell(prev_node_inst)->isInverter()) {
          // Remove previously inserted inverter fields.
          command.truncate(command.size() - prev_inst_fields_char_count);
          skipped_inverter_pair = true;
          inst_fields.clear();
          continue;
        }

        prev_inst_fields_char_count = inst_fields.size();
        command += inst_fields;
        inst_fields.clear();
      }
    }

    command += node_list->back()->isRisingEdge() ? " -rise_to " : " -fall_to ";
    command
        += Utils::wrapInCurly(QString::fromStdString(path->getEndStageName()));
  }

  command += focus_view_ == setup_timing_table_view_ ? " -path_delay max"
                                                     : " -path_delay min";
  command += " -fields {capacitance slew input_pins nets fanout} -format "
            "full_clock_expanded";

  return command;
}

void TimingWidget::clearPathDetails()
{
  focus_view_ = nullptr;

  path_details_model_->populateModel(nullptr, nullptr);
  capture_details_model_->populateModel(nullptr, nullptr);

  path_details_table_view_->setEnabled(false);
  capture_details_table_view_->setEnabled(false);

  path_renderer_->highlight(nullptr);
  emit highlightTimingPath(nullptr);
}

void TimingWidget::showPathDetails(const QModelIndex& index)
{
  if (!index.isValid())
    return;

  if (index.model() == setup_timing_paths_model_) {
    hold_timing_table_view_->clearSelection();
    focus_view_ = setup_timing_table_view_;
  } else {
    setup_timing_table_view_->clearSelection();
    focus_view_ = hold_timing_table_view_;
  }

  TimingPathsModel* focus_model
      = static_cast<TimingPathsModel*>(focus_view_->model());

  auto* path = focus_model->getPathAt(index);

  path_details_model_->populateModel(path, &path->getPathNodes());
  capture_details_model_->populateModel(path, &path->getCaptureNodes());

  path_details_table_view_->resizeColumnsToContents();
  path_details_table_view_->horizontalHeader()->setSectionResizeMode(
      0, QHeaderView::Stretch);

  capture_details_table_view_->resizeColumnsToContents();
  capture_details_table_view_->horizontalHeader()->setSectionResizeMode(
      0, QHeaderView::Stretch);

  path_renderer_->highlight(path);
  emit highlightTimingPath(path);

  updateClockRows();

  path_details_table_view_->setEnabled(path_details_model_->hasNodes());
  capture_details_table_view_->setEnabled(capture_details_model_->hasNodes());
}

void TimingWidget::updateClockRows()
{
  if (path_details_model_ == nullptr) {
    return;
  }

  if (path_details_model_->getPath() == nullptr) {
    return;
  }

  const bool show = settings_->getExpandClock();

  auto toggleModelView
      = [show](TimingPathDetailModel* model, QTableView* view) {
          model->setExpandClock(show);

          for (int row = 0; row < model->rowCount(); row++) {
            if (model->shouldHide(model->index(row, 0))) {
              view->hideRow(row);
            } else {
              view->showRow(row);
            }
          }
        };

  toggleModelView(path_details_model_, path_details_table_view_);
  toggleModelView(capture_details_model_, capture_details_table_view_);
}

void TimingWidget::highlightPathStage(TimingPathDetailModel* model,
                                      const QModelIndex& index)
{
  if (!index.isValid()) {
    return;
  }
  path_renderer_->clearHighlightNodes();

  if (!model->hasNodes()) {
    return;
  }

  if (model->isClockSummaryRow(index)) {
    auto* nodes = model->getNodes();
    for (int i = 1; i < model->getClockEndIndex(); i++) {
      path_renderer_->highlightNode(nodes->at(i).get());
    }
  } else {
    auto* node = model->getNodeAt(index);
    path_renderer_->highlightNode(node);
  }

  emit highlightTimingPath(path_renderer_->getPathToRender());
}

void TimingWidget::populatePaths()
{
  clearPathDetails();

  const auto from = settings_->getFromPins();
  const auto thru = settings_->getThruPins();
  const auto to = settings_->getToPins();

  populateAndSortModels(from, thru, to, "" /* path group name */);
}

void TimingWidget::populateAndSortModels(
    const std::set<const sta::Pin*>& from,
    const std::vector<std::set<const sta::Pin*>>& thru,
    const std::set<const sta::Pin*>& to,
    const std::string& path_group_name)
{
  setup_timing_paths_model_->populateModel(from, thru, to, path_group_name);
  hold_timing_paths_model_->populateModel(from, thru, to, path_group_name);

  // honor selected sort
  auto setup_header = setup_timing_table_view_->horizontalHeader();
  setup_timing_paths_model_->sort(setup_header->sortIndicatorSection(),
                                  setup_header->sortIndicatorOrder());
  auto hold_header = hold_timing_table_view_->horizontalHeader();
  hold_timing_paths_model_->sort(hold_header->sortIndicatorSection(),
                                 hold_header->sortIndicatorOrder());
}

void TimingWidget::selectedRowChanged(const QItemSelection& selected_row,
                                      const QItemSelection& deselected_row)
{
  auto sel_indices = selected_row.indexes();
  if (sel_indices.isEmpty()) {
    if (!deselected_row.isEmpty()) {
      clearPathDetails();
    }

    return;
  }
  auto& top_sel_index = sel_indices.first();
  showPathDetails(top_sel_index);
}

void TimingWidget::selectedDetailRowChanged(
    const QItemSelection& selected_row,
    const QItemSelection& deselected_row)
{
  auto sel_indices = selected_row.indexes();
  if (sel_indices.isEmpty()) {
    return;
  }
  auto& top_sel_index = sel_indices.first();
  highlightPathStage(path_details_model_, top_sel_index);
}

void TimingWidget::selectedCaptureRowChanged(
    const QItemSelection& selected_row,
    const QItemSelection& deselected_row)
{
  auto sel_indices = selected_row.indexes();
  if (sel_indices.isEmpty()) {
    return;
  }
  auto& top_sel_index = sel_indices.first();
  highlightPathStage(capture_details_model_, top_sel_index);
}

void TimingWidget::detailRowDoubleClicked(const QModelIndex& index)
{
  auto model = static_cast<const TimingPathDetailModel*>(index.model());

  if (!index.isValid() || !model->hasNodes()
      || model->isClockSummaryRow(index)) {
    return;
  }

  auto* node = model->getNodeAt(index);
  auto* gui = Gui::get();

  if (auto iterm = node->getPinAsITerm()) {
    emit inspect(gui->makeSelected(iterm));
  } else if (auto bterm = node->getPinAsBTerm()) {
    emit inspect(gui->makeSelected(bterm));
  }
}

void TimingWidget::copy()
{
  QTableView* focus_view
      = static_cast<QTableView*>(delay_widget_->currentWidget());

  QItemSelectionModel* selection = focus_view->selectionModel();
  QModelIndexList indexes = selection->selectedIndexes();

  if (indexes.size() < 1)
    return;
  auto& sel_index = indexes.first();
  if (focus_view == setup_timing_table_view_
      || focus_view == hold_timing_table_view_) {
    auto src_index = sel_index.sibling(sel_index.row(), 5);
    auto sink_index = sel_index.sibling(sel_index.row(), 6);
    auto src_node
        = selection->model()->data(src_index, Qt::DisplayRole).toString();
    auto sink_node
        = selection->model()->data(sink_index, Qt::DisplayRole).toString();
    QString sel_text = src_node + " -> " + sink_node;
    QGuiApplication::clipboard()->setText(sel_text);
  } else {
    auto sibling_index = sel_index.sibling(sel_index.row(), 0);
    auto sel_text
        = selection->model()->data(sibling_index, Qt::DisplayRole).toString();
    QGuiApplication::clipboard()->setText(sel_text);
  }
}

void TimingWidget::handleDbChange()
{
  clearPathDetails();

  path_details_model_->populateModel(nullptr, nullptr);
  capture_details_model_->populateModel(nullptr, nullptr);

  setup_timing_paths_model_->resetModel();
  hold_timing_paths_model_->resetModel();
}

void TimingWidget::showEvent(QShowEvent* event)
{
  toggleRenderer(true);
}

void TimingWidget::hideEvent(QHideEvent* event)
{
  toggleRenderer(false);
}

void TimingWidget::modelWasReset()
{
  setup_timing_table_view_->resizeColumnsToContents();
  hold_timing_table_view_->resizeColumnsToContents();

  toggleRenderer(!this->isHidden());
}

void TimingWidget::toggleRenderer(bool visible)
{
  if (!Gui::enabled() || path_renderer_ == nullptr) {
    return;
  }

  auto gui = Gui::get();
  if (visible) {
    gui->registerRenderer(path_renderer_.get());
  } else {
    gui->unregisterRenderer(path_renderer_.get());
  }
}

void TimingWidget::setBlock(odb::dbBlock* block)
{
  dbchange_listener_->addOwner(block);
}

void TimingWidget::showSettings()
{
  settings_->populate();
  settings_->show();
}

void TimingWidget::reportSlackHistogramPaths(
    const std::set<const sta::Pin*>& report_pins,
    const std::string& path_group_name)
{
  clearPathDetails();
  populateAndSortModels({}, {report_pins}, {}, path_group_name);
}

}  // namespace gui
