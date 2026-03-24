// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#include "cmdInputWidget.h"

#include <QFile>
#include <QMimeData>
#include <QSettings>
#include <QString>
#include <QTextStream>
#include <QWidget>
#include <algorithm>

namespace gui {

CmdInputWidget::CmdInputWidget(const QString& interp_name, QWidget* parent)
    : QPlainTextEdit(parent),
      line_height_(0),
      document_margins_(0),
      max_height_(QWIDGETSIZE_MAX),
      historyPosition_(0)
{
  setObjectName("interperter_scripting");  // for settings
  setPlaceholderText(interp_name + " commands");
  setAcceptDrops(true);

  determineLineHeight();

  // precompute size for updating text box size
  document_margins_ = 2 * (document()->documentMargin() + 3);

  connect(
      this, &CmdInputWidget::textChanged, this, &CmdInputWidget::updateSize);
  updateSize();

  connect(this,
          &CmdInputWidget::commandFinishedExecuting,
          this,
          &CmdInputWidget::commandFinished);
}

CmdInputWidget::~CmdInputWidget() = default;

void CmdInputWidget::setWidgetFont(const QFont& font)
{
  setFont(font);

  determineLineHeight();
}

void CmdInputWidget::determineLineHeight()
{
  const QFontMetrics font_metrics = fontMetrics();
  line_height_ = font_metrics.lineSpacing();

  double tab_indent_width = 2 * font_metrics.averageCharWidth();
#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
  // setTabStopWidth deprecated in 5.10
  setTabStopDistance(tab_indent_width);
#else
  setTabStopWidth(tab_indent_width);
#endif
}

bool CmdInputWidget::handleHistoryKeyPress(QKeyEvent* event)
{
  const int key = event->key();

  // handle regular command typing
  const bool has_control = event->modifiers().testFlag(Qt::ControlModifier);

  if (key == Qt::Key_Down) {
    // Handle down through history
    // control+down immediate
    if ((!textCursor().hasSelection()
         && !textCursor().movePosition(QTextCursor::Down))
        || has_control) {
      goForwardHistory();
      return true;
    }
  } else if (key == Qt::Key_Up) {
    // Handle up through history
    // control+up immediate
    if ((!textCursor().hasSelection()
         && !textCursor().movePosition(QTextCursor::Up))
        || has_control) {
      goBackHistory();
      return true;
    }
  }

  return false;
}

bool CmdInputWidget::handleEnterKeyPress(QKeyEvent* event)
{
  const int key = event->key();

  // handle regular command typing
  const bool has_control = event->modifiers().testFlag(Qt::ControlModifier);
  if (key == Qt::Key_Enter || key == Qt::Key_Return) {
    // Handle enter
    if (has_control) {
      // don't execute just insert the newline
      // does not get inserted by Qt, so manually inserting
      insertPlainText("\n");
      return true;
    }
    // Check if command complete and attempt to execute, otherwise do nothing
    if (isCommandComplete(toPlainText().simplified().toStdString())) {
      // execute command
      executeCommand(text());
      return true;
    }
  }

  return false;
}

// Update the size of the widget to match text
void CmdInputWidget::updateSize()
{
  int height = document()->size().toSize().height();
  height = std::max(height, 1);

  // in px
  int desired_height = height * line_height_ + document_margins_;

  desired_height = std::min(desired_height, max_height_);

  setFixedHeight(desired_height);

  ensureCursorVisible();
}

// Handle dragged and drop script files
void CmdInputWidget::dragEnterEvent(QDragEnterEvent* event)
{
  if (event->mimeData()->text().startsWith("file://")) {
    event->accept();
  }
}

void CmdInputWidget::dropEvent(QDropEvent* event)
{
  if (event->mimeData()->text().startsWith("file://")) {
    event->accept();

    // replace the content in the text area with the file
    QFile drop_file(event->mimeData()->text().remove(0, 7).simplified());
    if (drop_file.open(QIODevice::ReadOnly)) {
      QTextStream file_data(&drop_file);
      setText(file_data.readAll());
      drop_file.close();
    }
  }
}

// replicate QLineEdit function
QString CmdInputWidget::text() const
{
  return toPlainText();
}

// replicate QLineEdit function
void CmdInputWidget::setText(const QString& text)
{
  setPlainText(text);
  emit textChanged();
}

void CmdInputWidget::setMaxHeight(int height)
{
  const int min_height = line_height_ + document_margins_;  // atleast one line
  height = std::max(height, min_height);

  // save max height, since it's overwritten by setFixedHeight
  max_height_ = height;
  setMaximumHeight(height);

  updateSize();
}

void CmdInputWidget::readSettings(QSettings* settings)
{
  settings->beginGroup(objectName());
  history_ = settings->value("history").toStringList();
  historyPosition_ = history_.size();
  settings->endGroup();
}

void CmdInputWidget::writeSettings(QSettings* settings)
{
  settings->beginGroup(objectName());
  settings->setValue("history", history_);
  settings->endGroup();
}

void CmdInputWidget::goForwardHistory()
{
  if (historyPosition_ < history_.size() - 1) {
    ++historyPosition_;
    setText(history_[historyPosition_]);
  } else if (historyPosition_ == history_.size() - 1) {
    ++historyPosition_;
    setText(history_buffer_last_);
  }
}

void CmdInputWidget::goBackHistory()
{
  if (historyPosition_ > 0) {
    if (historyPosition_ == history_.size()) {
      // whats in the buffer is the last thing the user was editing
      history_buffer_last_ = text();
    }
    --historyPosition_;
    setText(history_[historyPosition_]);
  }
}

void CmdInputWidget::addCommandToHistory(const QString& command)
{
  // Update history; ignore repeated commands and keep last 100
  const int history_limit = 100;
  if (history_.empty() || command != history_.last()) {
    if (history_.size() == history_limit) {
      history_.pop_front();
    }
    history_.append(command);
  }
  historyPosition_ = history_.size();
}

void CmdInputWidget::commandFinished(bool is_ok)
{
  if (is_ok) {
    clear();
  }
}

}  // namespace gui
