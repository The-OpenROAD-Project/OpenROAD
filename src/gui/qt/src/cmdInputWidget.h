// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#pragma once

#include <QColor>
#include <QPlainTextEdit>
#include <QSettings>
#include <QString>
#include <QStringList>
#include <QWidget>
#include <string>

namespace gui {

class CmdInputWidget : public QPlainTextEdit
{
  Q_OBJECT

 public:
  CmdInputWidget(const QString& interp_name, QWidget* parent = nullptr);
  ~CmdInputWidget() override;

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
  void addTextToOutput(const QString& text, const QColor& color);
  void addCommandToOutput(const QString& cmd);

 public slots:
  virtual void executeCommand(const QString& cmd, bool echo, bool silent) = 0;

  void executeCommand(const QString& cmd, bool echo)
  {
    executeCommand(cmd, echo, false);
  }
  void executeCommand(const QString& cmd) { executeCommand(cmd, true); }

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
