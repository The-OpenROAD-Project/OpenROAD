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
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QFrame>
#include <QLabel>
#include <QClipboard>
#include <QDebug>
#include <QMessageBox>
#include <QSortFilterProxyModel>
#include <QHeaderView>

#include "db_sta/dbSta.hh"

namespace gui {

TimingWidget::TimingWidget(QWidget* parent)
    : QDockWidget("Timing Report", parent),
      setup_timing_table_view_(new QTableView(this)),
      hold_timing_table_view_(new QTableView(this)),
      path_details_table_view_(new QTableView(this)),
      capture_details_table_view_(new QTableView(this)),
      find_object_edit_(new QLineEdit(this)),
      path_index_spin_box_(new QSpinBox(this)),
      update_button_(new QPushButton("Update", this)),
      settings_button_(new QPushButton("Settings", this)),
      expand_clk_(new QCheckBox("Expand clock", this)),
      settings_(new TimingControlsDialog(this)),
      setup_timing_paths_model_(nullptr),
      hold_timing_paths_model_(nullptr),
      path_details_model_(nullptr),
      capture_details_model_(nullptr),
      path_renderer_(std::make_unique<TimingPathRenderer>()),
      cone_renderer_(std::make_unique<TimingConeRenderer>()),
      dbchange_listener_(new GuiDBChangeListener(this)),
      delay_widget_(new QTabWidget(this)),
      detail_widget_(new QTabWidget(this)),
      focus_view_(nullptr)
{
  setObjectName("timing_report"); // for settings

  QWidget* container = new QWidget(this);
  QGridLayout* layout = new QGridLayout;

  QFrame* control_frame = new QFrame(this);
  control_frame->setFrameShape(QFrame::StyledPanel);
  control_frame->setFrameShadow(QFrame::Raised);

  QHBoxLayout* controls_layout = new QHBoxLayout;
  controls_layout->addWidget(settings_button_);
  controls_layout->addWidget(update_button_);
  controls_layout->insertStretch(1);
  control_frame->setLayout(controls_layout);
  layout->addWidget(control_frame, 0, 0);

  // top half
  delay_widget_->addTab(setup_timing_table_view_, "Setup");
  delay_widget_->addTab(hold_timing_table_view_, "Hold");
  layout->addWidget(delay_widget_, 1, 0);

  // bottom half
  QTabWidget* path_widget = new QTabWidget(this);
  QWidget* bottom_widget = new QWidget(this);
  path_widget->addTab(bottom_widget, "Path Details");
  QGridLayout* bottom_widg_layout = new QGridLayout;
  bottom_widget->setLayout(bottom_widg_layout);
  QFrame* frame = new QFrame(this);
  frame->setFrameShape(QFrame::StyledPanel);
  frame->setFrameShadow(QFrame::Raised);
  bottom_widg_layout->addWidget(frame);

  QHBoxLayout* frame_layout = new QHBoxLayout;
  frame->setLayout(frame_layout);
  frame_layout->addWidget(new QLabel("Find", this));
  frame_layout->addWidget(find_object_edit_);
  find_object_edit_->setFocusPolicy(Qt::ClickFocus);
  find_object_edit_->setPlaceholderText("Pin or Net");

  frame_layout->addWidget(new QLabel("Path:", this));
  frame_layout->addWidget(path_index_spin_box_);
  frame_layout->addWidget(expand_clk_);
  expand_clk_->setCheckState(Qt::Checked);
  expand_clk_->setTristate(false);

  detail_widget_->addTab(path_details_table_view_, "Data");
  detail_widget_->addTab(capture_details_table_view_, "Capture");
  bottom_widg_layout->addWidget(detail_widget_);

  layout->addWidget(path_widget, 2, 0);

  container->setLayout(layout);
  setWidget(container);

  connect(find_object_edit_,
          SIGNAL(returnPressed()),
          this,
          SLOT(findNodeInPathDetails()));

  connect(
      path_index_spin_box_, SIGNAL(valueChanged(int)), this, SLOT(showPathIndex(int)));

  connect(dbchange_listener_,
          SIGNAL(dbUpdated()),
          this,
          SLOT(handleDbChange()));
  connect(
      update_button_, SIGNAL(clicked()), this, SLOT(populatePaths()));
  connect(
      update_button_, SIGNAL(clicked()), dbchange_listener_, SLOT(reset()));

  connect(settings_button_,
          SIGNAL(clicked()),
          this,
          SLOT(showSettings()));

  connect(settings_,
          SIGNAL(inspect(const Selected&)),
          this,
          SIGNAL(inspect(const Selected&)));

  connect(expand_clk_, SIGNAL(stateChanged(int)), this, SLOT(updateClockRows()));

  path_index_spin_box_->setRange(0, 0);
}

TimingWidget::~TimingWidget()
{
  dbchange_listener_->removeOwner();
}

void TimingWidget::init(sta::dbSta* sta)
{
  cone_renderer_->setSTA(sta);
  settings_->setSTA(sta);

  setup_timing_paths_model_ = new TimingPathsModel(sta, this);
  hold_timing_paths_model_ = new TimingPathsModel(sta, this);
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
  setup_timing_table_view_->horizontalHeader()->setSortIndicator(3, Qt::AscendingOrder);
  hold_timing_table_view_->setSortingEnabled(true);
  hold_timing_table_view_->horizontalHeader()->setSortIndicator(3, Qt::AscendingOrder);

  connect(setup_timing_paths_model_, SIGNAL(modelReset()), this, SLOT(modelWasReset()));
  connect(hold_timing_paths_model_, SIGNAL(modelReset()), this, SLOT(modelWasReset()));

  connect(setup_timing_table_view_->horizontalHeader(),
          SIGNAL(sortIndicatorChanged(int, Qt::SortOrder)),
          setup_timing_table_view_->model(),
          SLOT(sort(int, Qt::SortOrder)));
  connect(hold_timing_table_view_->horizontalHeader(),
          SIGNAL(sortIndicatorChanged(int, Qt::SortOrder)),
          hold_timing_table_view_->model(),
          SLOT(sort(int, Qt::SortOrder)));

  connect(
      setup_timing_table_view_->selectionModel(),
      SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
      this,
      SLOT(selectedRowChanged(const QItemSelection&, const QItemSelection&)));

  connect(
      hold_timing_table_view_->selectionModel(),
      SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
      this,
      SLOT(selectedRowChanged(const QItemSelection&, const QItemSelection&)));

  connect(
      path_details_table_view_->selectionModel(),
      SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
      this,
      SLOT(selectedDetailRowChanged(const QItemSelection&,
                                    const QItemSelection&)));
  connect(
      capture_details_table_view_->selectionModel(),
      SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
      this,
      SLOT(selectedCaptureRowChanged(const QItemSelection&,
                                     const QItemSelection&)));

  clearPathDetails();
}

void TimingWidget::updatePaths()
{
  update_button_->click();
}

void TimingWidget::readSettings(QSettings* settings)
{
  settings->beginGroup(objectName());

  settings_->setPathCount(settings->value("path_count", settings_->getPathCount()).toInt());
  expand_clk_->setCheckState(settings->value("expand_clk", expand_clk_->checkState()).value<Qt::CheckState>());

  settings->endGroup();
}

void TimingWidget::writeSettings(QSettings* settings)
{
  settings->beginGroup(objectName());

  settings->setValue("path_count", settings_->getPathCount());
  settings->setValue("expand_clk", expand_clk_->checkState());

  settings->endGroup();
}

void TimingWidget::keyPressEvent(QKeyEvent* key_event)
{
  if (key_event->matches(QKeySequence::Copy)) {
    copy();
    key_event->accept();
  }
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

  TimingPathsModel* focus_model = static_cast<TimingPathsModel*>(focus_view_->model());

  auto* path = focus_model->getPathAt(index);

  path_details_model_->populateModel(path, path->getPathNodes());
  capture_details_model_->populateModel(path, path->getCaptureNodes());

  path_details_table_view_->resizeColumnsToContents();
  path_details_table_view_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);

  capture_details_table_view_->resizeColumnsToContents();
  capture_details_table_view_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);

  path_renderer_->highlight(path);
  emit highlightTimingPath(path);

  path_index_spin_box_->setRange(0, focus_model->rowCount()-1);
  path_index_spin_box_->setValue(index.row());

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

  const bool show = expand_clk_->checkState() == Qt::Checked;

  auto toggleModelView = [show](TimingPathDetailModel* model, QTableView* view) {
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

void TimingWidget::highlightPathStage(TimingPathDetailModel* model, const QModelIndex& index)
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

void TimingWidget::findNodeInPathDetails()
{
  auto search_node = find_object_edit_->text();
  QSortFilterProxyModel proxy;

  TimingPathDetailModel* model = path_details_model_;
  QTableView* view = path_details_table_view_;
  if (detail_widget_->currentWidget() == capture_details_table_view_) {
    model = capture_details_model_;
    view = capture_details_table_view_;
  }

  proxy.setSourceModel(model);
  proxy.setFilterKeyColumn(0);
  proxy.setFilterFixedString(search_node);
  QModelIndex match_index = proxy.mapToSource(proxy.index(0, 0));
  if (match_index.isValid()) {
    const int row = match_index.row();
    const bool show_clock = expand_clk_->checkState() == Qt::Checked;
    if (view->isRowHidden(row) && !show_clock) {
      // turn on expand clock so we can select that row
      expand_clk_->setCheckState(Qt::Checked);
    }

    if (!view->isRowHidden(row)) {
      // double check if row is hidden, since if th clock summary row matched, that is not a node.
      view->selectRow(row);
      return;
    }
  }

  QMessageBox::information(
      this, "Node Search: ", search_node + " Match not found!");
}

void TimingWidget::showPathIndex(int path_idx)
{
  if (focus_view_ == nullptr) {
    return;
  }

  QModelIndex new_index = focus_view_->model()->index(path_idx, 0);
  focus_view_->selectRow(new_index.row());

  showPathDetails(new_index);
}

void TimingWidget::populatePaths()
{
  clearPathDetails();

  const int count = settings_->getPathCount();
  const auto from = settings_->getFromPins();
  const auto thru = settings_->getThruPins();
  const auto to = settings_->getToPins();
  const bool unconstrained = settings_->getUnconstrained();

  setup_timing_paths_model_->populateModel(true, count, from, thru, to, unconstrained);
  hold_timing_paths_model_->populateModel(false, count, from, thru, to, unconstrained);

  // honor selected sort
  auto setup_header = setup_timing_table_view_->horizontalHeader();
  setup_timing_paths_model_->sort(setup_header->sortIndicatorSection(), setup_header->sortIndicatorOrder());
  auto hold_header = hold_timing_table_view_->horizontalHeader();
  hold_timing_paths_model_->sort(hold_header->sortIndicatorSection(), hold_header->sortIndicatorOrder());
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
  auto top_sel_index = sel_indices.first();
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
  auto top_sel_index = sel_indices.first();
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
  auto top_sel_index = sel_indices.first();
  highlightPathStage(capture_details_model_, top_sel_index);
}

void TimingWidget::copy()
{
  QTableView* focus_view = static_cast<QTableView*>(delay_widget_->currentWidget());

  QItemSelectionModel* selection = focus_view->selectionModel();
  QModelIndexList indexes = selection->selectedIndexes();

  if (indexes.size() < 1)
    return;
  auto sel_index = indexes.first();
  if (focus_view == setup_timing_table_view_ ||
      focus_view == hold_timing_table_view_) {
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

}  // namespace gui
