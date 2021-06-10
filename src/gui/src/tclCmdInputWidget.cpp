///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2019, OpenROAD
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

#include "tclCmdInputWidget.h"

#include <tcl.h>

namespace gui {

TclCmdInputWidget::TclCmdInputWidget(QWidget* parent) :
    QPlainTextEdit(parent)
{
  setObjectName("tcl_scripting");  // for settings
  setPlaceholderText("TCL commands");

  max_height_  = QWIDGETSIZE_MAX;
  determineLineHeight();

  // precompute size for updating text box size
  document_margins_ = 2 * document()->documentMargin();

  connect(this, SIGNAL(textChanged()), this, SLOT(updateSize()));
  updateSize();
}

TclCmdInputWidget::~TclCmdInputWidget()
{
}

void TclCmdInputWidget::setFont(const QFont& font)
{
  QPlainTextEdit::setFont(font);

  determineLineHeight();
}

void TclCmdInputWidget::determineLineHeight()
{
  QFontMetrics font_metrics = fontMetrics();
  line_height_ = font_metrics.lineSpacing();

  double tab_indent_width = 2 * font_metrics.averageCharWidth();
#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
  // setTabStopWidth deprecated in 5.10
  setTabStopDistance(tab_indent_width);
#else
  setTabStopWidth(tab_indent_width);
#endif
}

void TclCmdInputWidget::keyPressEvent(QKeyEvent* e)
{
  bool forward_key_press = true;

  bool has_control = e->modifiers().testFlag(Qt::ControlModifier);
  int key = e->key();
  if (key == Qt::Key_Enter || key == Qt::Key_Return) {
    // Handle enter
    if (has_control) {
      // don't execute just insert the newline
      // does not get inserted by Qt, so manually inserting
      insertPlainText("\n");
    } else {
      // Check if command complete and attempt to execute, otherwise do nothing
      if (isCommandComplete(toPlainText().simplified().toStdString())) {
        emit completeCommand();
        if (toPlainText().isEmpty()) {
          // if successful, contents was cleared
          forward_key_press = false;
        }
      }
    }
  } else if (key == Qt::Key_Down) {
    // Handle down through history
    // control+down immediate
    if ((!textCursor().hasSelection() && !textCursor().movePosition(QTextCursor::Down))
        || has_control) {
      emit historyGoForward();
    }
  } else if (key == Qt::Key_Up) {
    // Handle up through history
    // control+up immediate
    if ((!textCursor().hasSelection() && !textCursor().movePosition(QTextCursor::Up))
        || has_control) {
      emit historyGoBack();
    }
  }

  if (forward_key_press) {
    QPlainTextEdit::keyPressEvent(e);
  }
}

void TclCmdInputWidget::keyReleaseEvent(QKeyEvent* e)
{
  int key = e->key();
  if (key == Qt::Key_Enter || key == Qt::Key_Return) {
    // announce that text might have changed due to enter key
    // to ensure resizing is processed
    emit textChanged();
  }

  QPlainTextEdit::keyReleaseEvent(e);
}

bool TclCmdInputWidget::isCommandComplete(const std::string& cmd)
{
  if (cmd.empty()) {
    return false;
  }

  // check if last character is \, then assume it is multiline
  if (cmd.at(cmd.size() - 1) == '\\') {
    return false;
  }

  return Tcl_CommandComplete(cmd.c_str());
}

// Slot to announce command executed
void TclCmdInputWidget::commandExecuted(int return_code)
{
  if (return_code == TCL_OK) {
    clear();
  }
}

// Update the size of the widget to match text
void TclCmdInputWidget::updateSize()
{
  int height = document()->size().toSize().height();
  if (height < 1) {
    height = 1; // ensure minimum is 1 line
  }

  // in px
  int desired_height = height * line_height_ + document_margins_;

  if (desired_height > max_height_) {
    desired_height = max_height_; // ensure maximum from Qt suggestion
  }

  setFixedHeight(desired_height);

  ensureCursorVisible();
}

// replicate QLineEdit function
QString TclCmdInputWidget::text()
{
  return toPlainText();
}

// replicate QLineEdit function
void TclCmdInputWidget::setText(const QString& text)
{
  setPlainText(text);
  emit textChanged();
}

void TclCmdInputWidget::setMaximumHeight(int height)
{
  int min_height = line_height_ + document_margins_; // atleast one line
  if (height < min_height) {
    height = min_height;
  }

  // save max height, since it's overwritten by setFixedHeight
  max_height_ = height;
  QPlainTextEdit::setMaximumHeight(height);

  updateSize();
}

}  // namespace gui
