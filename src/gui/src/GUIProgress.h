/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2025, The Regents of the University of California
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

#pragma once

#include <QFrame>
#include <QProgressBar>
#include <QPushButton>
#include <QWidget>
#include <csignal>
#include <map>
#include <memory>

#include "mainWindow.h"
#include "utl/Progress.h"

namespace utl {

class Logger;

}

namespace gui {

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

  MainWindow* mw_;

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

 signals:
  void updateProgress(int progress);
  void updateRange(int min, int max);

 protected:
  bool checkInterrupt(const std::string& name);
  QProgressBar* progress_bar_;
  QPushButton* interrupt_;
};

class ProgressReporterWidget : public ProgressWidget
{
  Q_OBJECT

 public:
  ProgressReporterWidget(std::shared_ptr<utl::ProgressReporter>& reporter,
                         QWidget* parent = nullptr);

 public slots:
  void interrupt();

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

  QPushButton* expand_;

  std::map<ProgressReporterWidget*, QFrame*> widgets_;
};

}  // namespace gui
