///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2022, The Regents of the University of California
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

#pragma once

#include <QPlainTextEdit>
#include <QSettings>
#include <QStringList>

namespace gui {

class CmdInputWidget : public QPlainTextEdit
{
  Q_OBJECT

 public:
  CmdInputWidget(const QString& interp_name, QWidget* parent = nullptr);
  virtual ~CmdInputWidget();

  void setWidgetFont(const QFont& font);

  QString text() const;
  void setText(const QString& text);

  void setMaxHeight(int height);

  void readSettings(QSettings* settings);
  void writeSettings(QSettings* settings);

 signals:
  void exiting();

  void commandAboutToExecute();
  void commandFinishedExecuting(bool is_ok);

  void addResultToOutput(const QString& result, bool is_ok);
  void addCommandToOutput(const QString& cmd);

 public slots:
  virtual void executeCommand(const QString& cmd,
                              bool echo = true,
                              bool silent = false)
      = 0;

 private slots:
  void updateSize();
  void commandFinished(bool is_ok);

 protected:
  void dragEnterEvent(QDragEnterEvent* event) override;
  void dropEvent(QDropEvent* event) override;
  bool handleHistoryKeyPress(QKeyEvent* event);
  bool handleEnterKeyPress(QKeyEvent* event);

  void addCommandToHistory(const QString& command);

  virtual bool isCommandComplete(const std::string& cmd) const = 0;

 private:
  // back in history
  void goBackHistory();
  // forward in history
  void goForwardHistory();

  void determineLineHeight();

  int line_height_;
  int document_margins_;

  int max_height_;

  QStringList history_;
  QString history_buffer_last_;
  int historyPosition_;
};

}  // namespace gui
