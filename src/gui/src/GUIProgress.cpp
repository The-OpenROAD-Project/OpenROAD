// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include "GUIProgress.h"

#include <QApplication>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QPushButton>
#include <QStatusBar>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>
#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "mainWindow.h"
#include "utl/Logger.h"
#include "utl/Progress.h"

namespace gui {

GUIProgress::GUIProgress(utl::Logger* logger, MainWindow* mw)
    : utl::Progress(logger),
      combined_progress_(new CombinedProgressWidget(this, mw))
{
  mw->statusBar()->addPermanentWidget(combined_progress_);
}

void GUIProgress::start(std::shared_ptr<utl::ProgressReporter>& reporter)
{
  // Create reporting widget
  ProgressReporterWidget* widget = new ProgressReporterWidget(reporter);
  widgets_[reporter.get()] = widget;

  configureProgressUI(reporter.get(), widget);
  combined_progress_->addWidget(widget);

  updateCombined();

  updateGUI();
}

void GUIProgress::update(utl::ProgressReporter* reporter)
{
  updateGUI();

  configureProgressUI(reporter, widgets_[reporter]);

  updateCombined();

  updateGUI();
}

void GUIProgress::end(utl::ProgressReporter* reporter)
{
  auto* widget = widgets_[reporter];

  // remove widget
  widgets_.erase(reporter);

  updateCombined();

  // delete
  widget->deleteLater();
  combined_progress_->removeWidget(widget);

  updateGUI();
}

void GUIProgress::deleted(utl::ProgressReporter* reporter)
{
  if (widgets_.find(reporter) == widgets_.end()) {
    return;
  }

  auto* widget = widgets_[reporter];

  // remove widget
  widgets_.erase(reporter);

  updateCombined();

  // delete
  widget->deleteLater();
  combined_progress_->removeWidget(widget);

  updateGUI();
}

void GUIProgress::updateGUI() const
{
  QApplication::processEvents();
}

void GUIProgress::configureProgressUI(utl::ProgressReporter* reporter,
                                      ProgressWidget* widget) const
{
  if (reporter->checkInterrupt()) {
    widget->setFormat(fmt::format("{}*: %v / %m", reporter->getName()));
  } else {
    widget->setFormat(fmt::format("{}: %v / %m", reporter->getName()));
  }

  switch (reporter->getType()) {
    case utl::ProgressReporter::ReportType::PERCENTAGE:
      widget->setRange(0, reporter->getTotalWork().value_or(100));
      widget->setProgress(reporter->getValue());
      break;
    case utl::ProgressReporter::ReportType::ITERATION:
      widget->setRange(
          0, reporter->getTotalWork().value_or(reporter->getValue() + 1));
      widget->setProgress(reporter->getValue());
      break;
    case utl::ProgressReporter::ReportType::NONE:
      widget->setRange(0, 0);
      widget->setFormat(reporter->getName());
      widget->setProgress(0);
      break;
  }
}

void GUIProgress::updateCombined()
{
  const auto reporters = getReporters();
  if (reporters.empty()) {
    combined_progress_->reset();
  } else if (reporters.size() == 1) {
    configureProgressUI(reporters[0].get(), combined_progress_);
  } else {
    bool has_interrupt = false;
    std::vector<std::pair<int, int>> reporter_progress;

    std::string names;

    // interate and combine information
    for (const auto& reporter : reporters) {
      has_interrupt |= reporter->checkInterrupt();

      if (!names.empty()) {
        names += ", ";
      }
      names += reporter->getName();

      switch (reporter->getType()) {
        case utl::ProgressReporter::ReportType::PERCENTAGE:
          reporter_progress.emplace_back(
              reporter->getValue(), reporter->getTotalWork().value_or(100));
          break;
        case utl::ProgressReporter::ReportType::ITERATION:
          reporter_progress.emplace_back(
              reporter->getValue(),
              reporter->getTotalWork().value_or(reporter->getValue() + 1));
          break;
        case utl::ProgressReporter::ReportType::NONE:
          break;
      }
    }

    int max_work = 0;
    for (const auto& [progress, work] : reporter_progress) {
      max_work = std::max(max_work, work);
    }
    const int total_work = max_work * reporter_progress.size();
    int total_progress = 0;
    for (const auto& [progress, work] : reporter_progress) {
      if (work < 2) {
        continue;
      }
      const float scale = static_cast<float>(max_work) / (work - 1);
      total_progress += progress * scale;
    }

    if (names.size() > kMaxCombinedNameLength && reporter_progress.size() > 2) {
      names = "several";
    }
    if (names.size() > kMaxCombinedNameLength) {
      names = names.substr(0, kMaxCombinedNameLength - 3);
      names += "...";
    }

    if (has_interrupt) {
      combined_progress_->setFormat(fmt::format("{}*: %p%", names));
    } else {
      combined_progress_->setFormat(fmt::format("{}: %p%", names));
    }
    combined_progress_->setRange(0, total_work);
    combined_progress_->setProgress(total_progress);
  }
}

////////////////////////////////////////

ProgressWidget::ProgressWidget(QWidget* parent)
    : QWidget(parent), progress_bar_(new QProgressBar(this))
{
  // connect via queued connnection for threaded reporting
  connect(this,
          &ProgressWidget::updateProgress,
          progress_bar_,
          &QProgressBar::setValue,
          Qt::QueuedConnection);
  connect(this,
          &ProgressWidget::updateRange,
          progress_bar_,
          &QProgressBar::setRange,
          Qt::QueuedConnection);

  QHBoxLayout* layout = new QHBoxLayout(this);
  layout->addWidget(progress_bar_, 1);

  setLayout(layout);

  setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
  setMinimumWidth(100);
  setMaximumWidth(400);
}

void ProgressWidget::setProgress(int progress)
{
  emit updateProgress(progress);
}

void ProgressWidget::setRange(int min, int max)
{
  progress_bar_->setTextVisible(true);
  emit updateRange(min, max);
}

void ProgressWidget::reset()
{
  progress_bar_->setRange(0, 1);
  progress_bar_->setValue(0);

  progress_bar_->reset();
  progress_bar_->resetFormat();
  progress_bar_->setTextVisible(false);
}

void ProgressWidget::setFormat(const std::string& format)
{
  progress_bar_->setFormat(QString::fromStdString(format));
}

bool ProgressWidget::checkInterrupt(const std::string& name)
{
  return ProgressWidget::checkInterrupt(name, this);
}

bool ProgressWidget::checkInterrupt(const std::string& name, QWidget* parent)
{
  const std::string message
      = fmt::format("Are you sure you want to interrupt {}?", name);
  return QMessageBox::critical(parent,
                               "Interrupt?",
                               QString::fromStdString(message),
                               QMessageBox::Yes,
                               QMessageBox::No)
         == QMessageBox::Yes;
}

////////////////////////////////////////

ProgressReporterWidget::ProgressReporterWidget(
    std::shared_ptr<utl::ProgressReporter>& reporter,
    QWidget* parent)
    : ProgressWidget(parent), reporter_(reporter)
{
}

////////////////////////////////////////

CombinedProgressWidget::CombinedProgressWidget(utl::Progress* progress,
                                               QWidget* parent)
    : ProgressWidget(parent),
      progress_(progress),
      details_(new QWidget(this)),
      interrupt_(new QPushButton(this)),
      expand_(new QPushButton(this))
{
  connect(interrupt_,
          &QPushButton::clicked,
          this,
          &CombinedProgressWidget::interrupt);

  interrupt_->setIcon(QIcon(":/cancel.png"));

  layout()->addWidget(interrupt_);
  layout()->addWidget(expand_);

  // Minimize margins to to avoid the status bar growing
  layout()->setContentsMargins(0, 0, 0, 0);

  connect(expand_,
          &QPushButton::clicked,
          this,
          &CombinedProgressWidget::toggleProgress);

  QVBoxLayout* details_layout = new QVBoxLayout();
  details_->setLayout(details_layout);
  details_->setWindowFlags(Qt::Window);
  details_->setMinimumWidth(400);
  details_->setWindowTitle("Progress");

  hideProgress();
  expand_->setEnabled(false);
}

void CombinedProgressWidget::interrupt()
{
  if (progress_->inProgress() && checkInterrupt("all")) {
    progress_->interrupt();
  }
}

void CombinedProgressWidget::toggleProgress()
{
  if (details_->isVisible()) {
    hideProgress();
  } else {
    showProgress();
  }
}

void CombinedProgressWidget::showProgress()
{
  expand_->setIcon(QIcon(":/expand_more.png"));
  details_->show();
}

void CombinedProgressWidget::hideProgress()
{
  expand_->setIcon(QIcon(":/expand_less.png"));
  details_->hide();
}

void CombinedProgressWidget::addWidget(ProgressReporterWidget* widget)
{
  // Create framed object
  QFrame* frame = new QFrame(this);
  frame->setFrameStyle(QFrame::Box);
  frame->setLineWidth(1);

  QVBoxLayout* layout = new QVBoxLayout;
  layout->addWidget(widget);
  frame->setLayout(layout);

  details_->layout()->addWidget(frame);
  widgets_[widget] = frame;

  expand_->setEnabled(true);
}

void CombinedProgressWidget::removeWidget(ProgressReporterWidget* widget)
{
  if (widgets_.find(widget) == widgets_.end()) {
    return;
  }

  details_->layout()->removeWidget(widgets_[widget]);
  widgets_.erase(widget);

  if (widgets_.empty()) {
    hideProgress();
    expand_->setEnabled(false);
  }
}

}  // namespace gui
