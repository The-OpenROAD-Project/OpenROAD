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

#include <QKeyEvent>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QTimer>

#include "openroad/OpenRoad.hh"
#include "scriptWidget.h"

namespace gui {

ScriptWidget::ScriptWidget(QWidget* parent)
    : QDockWidget("Scripting", parent),
      input_(new QLineEdit),
      output_(new QTextEdit),
      historyPosition_(0)
{
  setObjectName("scripting");  // for settings

  output_->setReadOnly(true);

  QVBoxLayout* layout = new QVBoxLayout;
  layout->addWidget(output_, /* stretch */ 1);
  layout->addWidget(input_);

  QWidget* container = new QWidget;
  container->setLayout(layout);

  QTimer::singleShot(200, this, &ScriptWidget::setupTcl);

  connect(input_, SIGNAL(returnPressed()), this, SLOT(executeCommand()));

  setWidget(container);
}

int channelClose(ClientData instanceData, Tcl_Interp* interp)
{
  // This channel should never be closed
  return EINVAL;
}

int ScriptWidget::channelOutput(ClientData  instanceData,
                                const char* buf,
                                int         toWrite,
                                int*        errorCodePtr)
{
  // Buffer up the output
  ScriptWidget* widget = (ScriptWidget*) instanceData;
  widget->outputBuffer_.append(QString::fromLatin1(buf, toWrite).trimmed());
  return toWrite;
}

void channelWatch(ClientData instanceData, int mask)
{
  // watch is not supported inside OpenROAD GUI
}

Tcl_ChannelType ScriptWidget::stdoutChannelType = {
    // Tcl stupidly defines this a non-cost char*
    ((char*) "stdout_channel"),  /* typeName */
    TCL_CHANNEL_VERSION_2,       /* version */
    channelClose,                /* closeProc */
    nullptr,                     /* inputProc */
    ScriptWidget::channelOutput, /* outputProc */
    nullptr,                     /* seekProc */
    nullptr,                     /* setOptionProc */
    nullptr,                     /* getOptionProc */
    channelWatch,                /* watchProc */
    nullptr,                     /* getHandleProc */
    nullptr,                     /* close2Proc */
    nullptr,                     /* blockModeProc */
    nullptr,                     /* flushProc */
    nullptr,                     /* handlerProc */
    nullptr,                     /* wideSeekProc */
    nullptr,                     /* threadActionProc */
    nullptr                      /* truncateProc */
};

void ScriptWidget::setupTcl()
{
  interp_ = Tcl_CreateInterp();

  Tcl_Channel stdoutChannel = Tcl_CreateChannel(
      &stdoutChannelType, "stdout", (ClientData) this, TCL_WRITABLE);
  if (stdoutChannel) {
    Tcl_SetChannelOption(nullptr, stdoutChannel, "-translation", "lf");
    Tcl_SetChannelOption(nullptr, stdoutChannel, "-buffering", "none");
    Tcl_RegisterChannel(interp_, stdoutChannel);  // per man page: some tcl bug
    Tcl_SetStdChannel(stdoutChannel, TCL_STDOUT);
  }

  ord::tclAppInit(interp_);
  // TODO: tclAppInit should return the status which we could
  // pass to updateOutput
  updateOutput(TCL_OK);
}

void ScriptWidget::executeCommand()
{
  QString command = input_->text();
  input_->clear();

  int return_code = Tcl_Eval(interp_, command.toLatin1().data());

  // Show the command that we executed
  output_->setTextColor(Qt::black);
  output_->append("> " + command);

  // Show its output
  updateOutput(return_code);

  // Update history; ignore repeated commands and keep last 100
  const int history_limit = 100;
  if (history_.empty() || command != history_.last()) {
    if (history_.size() == history_limit) {
      history_.pop_front();
    }

    history_.append(command);
  }
  historyPosition_ = history_.size();
  emit commandExecuted();
}

void ScriptWidget::updateOutput(int return_code)
{
  // Show whatever we captured from the output channel in grey
  output_->setTextColor(QColor(0x30, 0x30, 0x30));
  for (auto& out : outputBuffer_) {
    if (!out.isEmpty()) {
      output_->append(out);
    }
  }
  outputBuffer_.clear();

  // Show the return value color-coded by ok/err.
  const char* result = Tcl_GetString(Tcl_GetObjResult(interp_));
  if (result[0] != '\0') {
    output_->setTextColor((return_code == TCL_OK) ? Qt::blue : Qt::red);
    output_->append(result);
  }
}

ScriptWidget::~ScriptWidget()
{
  // TODO: I am being lazy and not cleaning up the tcl interpreter.
  // We are likely exiting anyways
}

void ScriptWidget::keyPressEvent(QKeyEvent* e)
{
  // Handle up/down through history
  int key = e->key();
  if (key == Qt::Key_Down) {
    if (historyPosition_ < history_.size() - 1) {
      ++historyPosition_;
      input_->setText(history_[historyPosition_]);
    } else if (historyPosition_ == history_.size() - 1) {
      ++historyPosition_;
      input_->clear();
    }
    return;
  } else if (key == Qt::Key_Up) {
    if (historyPosition_ > 0) {
      --historyPosition_;
      input_->setText(history_[historyPosition_]);
    }
    return;
  }
  QDockWidget::keyPressEvent(e);
}

void ScriptWidget::readSettings(QSettings* settings)
{
  settings->beginGroup("scripting");
  history_         = settings->value("history").toStringList();
  historyPosition_ = history_.size();
  settings->endGroup();
}

void ScriptWidget::writeSettings(QSettings* settings)
{
  settings->beginGroup("scripting");
  settings->setValue("history", history_);
  settings->endGroup();
}

}  // namespace gui
