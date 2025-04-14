// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#pragma once

#include <QFrame>
#include <QProgressBar>
#include <QPushButton>
#include <QWidget>
#include <csignal>
#include <map>
#include <memory>
#include <string>

#include "utl/Progress.h"

namespace utl {

class Logger;

}

namespace gui {

class MainWindow;
class ProgressWidget;
class ProgressReporterWidget;
class CombinedProgressWidget;

class GUIProgress : public utl::Progress
{
 public:
  GUIProgress(utl::Logger* logger, MainWindow* mw);
  ~GUIProgress() override = default;

  void start(std::shared_ptr<utl::ProgressReporter>& reporter) override;
  void update(utl::ProgressReporter* reporter) override;
  void end(utl::ProgressReporter* reporter) override;
  void deleted(utl::ProgressReporter* reporter) override;

 private:
  void updateCombined();
  void configureProgressUI(utl::ProgressReporter* reporter,
                           ProgressWidget* widget) const;

  CombinedProgressWidget* combined_progress_;

  std::map<utl::ProgressReporter*, ProgressReporterWidget*> widgets_;

  constexpr static int max_combined_name_length_ = 30;

  void updateGUI() const;
};

class ProgressWidget : public QWidget
{
  Q_OBJECT

 public:
  ProgressWidget(QWidget* parent = nullptr);

  void setProgress(int progress);
  void setRange(int min, int max);

  void reset();

  void setFormat(const std::string& format);

  bool checkInterrupt(const std::string& name);
  static bool checkInterrupt(const std::string& name, QWidget* parent);

 signals:
  void updateProgress(int progress);
  void updateRange(int min, int max);

 protected:
  QProgressBar* progress_bar_;
};

class ProgressReporterWidget : public ProgressWidget
{
  Q_OBJECT

 public:
  ProgressReporterWidget(std::shared_ptr<utl::ProgressReporter>& reporter,
                         QWidget* parent = nullptr);

 private:
  std::weak_ptr<utl::ProgressReporter> reporter_;
};

class CombinedProgressWidget : public ProgressWidget
{
  Q_OBJECT

 public:
  CombinedProgressWidget(utl::Progress* progress, QWidget* parent = nullptr);

 public slots:
  void interrupt();

  void toggleProgress();
  void showProgress();
  void hideProgress();

  void addWidget(ProgressReporterWidget* widget);
  void removeWidget(ProgressReporterWidget* widget);

 private:
  utl::Progress* progress_;

  QWidget* details_;

  QPushButton* interrupt_;
  QPushButton* expand_;

  std::map<ProgressReporterWidget*, QFrame*> widgets_;
};

}  // namespace gui
