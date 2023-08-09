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

#include "cmdInputWidget.h"

#include <QMimeData>
#include <QTextStream>

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
    } else {
      // Check if command complete and attempt to execute, otherwise do nothing
      if (isCommandComplete(toPlainText().simplified().toStdString())) {
        // execute command
        executeCommand(text());
        return true;
      }
    }
  }

  return false;
}

// Update the size of the widget to match text
void CmdInputWidget::updateSize()
{
  int height = document()->size().toSize().height();
  if (height < 1) {
    height = 1;  // ensure minimum is 1 line
  }

  // in px
  int desired_height = height * line_height_ + document_margins_;

  if (desired_height > max_height_) {
    desired_height = max_height_;  // ensure maximum from Qt suggestion
  }

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
  if (height < min_height) {
    height = min_height;
  }

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
