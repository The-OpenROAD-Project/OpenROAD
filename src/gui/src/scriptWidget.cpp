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

#include "scriptWidget.h"

#include <errno.h>
#include <unistd.h>

#include <QCoreApplication>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLineEdit>
#include <QTimer>
#include <QVBoxLayout>

#include "ord/OpenRoad.hh"
#include "spdlog/formatter.h"
#include "spdlog/sinks/base_sink.h"
#include "utl/Logger.h"

namespace gui {

ScriptWidget::ScriptWidget(QWidget* parent)
    : QDockWidget("Scripting", parent),
      input_(new QLineEdit),
      output_(new QTextEdit),
      pauser_(new QPushButton("Idle")),
      historyPosition_(0)
{
  setObjectName("scripting");  // for settings
  output_->setFont(QFont("Monospace"));

  output_->setReadOnly(true);
  pauser_->setEnabled(false);

  QHBoxLayout* inner_layout = new QHBoxLayout;
  inner_layout->addWidget(pauser_);
  inner_layout->addWidget(input_, /* stretch */ 1);

  QVBoxLayout* layout = new QVBoxLayout;
  layout->addWidget(output_, /* stretch */ 1);
  layout->addLayout(inner_layout);

  QWidget* container = new QWidget;
  container->setLayout(layout);

  QTimer::singleShot(200, this, &ScriptWidget::setupTcl);

  connect(input_, SIGNAL(returnPressed()), this, SLOT(executeCommand()));
  connect(pauser_, SIGNAL(pressed()), this, SLOT(pauserClicked()));

  setWidget(container);
}

int channelClose(ClientData instance_data, Tcl_Interp* interp)
{
  // This channel should never be closed
  return EINVAL;
}

int ScriptWidget::channelOutput(ClientData instance_data,
                                const char* buf,
                                int to_write,
                                int* error_code)
{
  // Buffer up the output
  ScriptWidget* widget = (ScriptWidget*) instance_data;
  widget->logger_->report(std::string(buf, to_write));
  return to_write;
}

void channelWatch(ClientData instance_data, int mask)
{
  // watch is not supported inside OpenROAD GUI
}

Tcl_ChannelType ScriptWidget::stdout_channel_type_ = {
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

int ScriptWidget::tclExitHandler(ClientData instance_data,
                                 Tcl_Interp *interp,
                                 int argc,
                                 const char **argv) {
  ScriptWidget* widget = (ScriptWidget*) instance_data;
  // announces exit to Qt
  emit widget->tclExiting();
  // does not matter from here on, since GUI is getting ready exit
  return TCL_OK;
}

void ScriptWidget::setupTcl()
{
  interp_ = Tcl_CreateInterp();

  Tcl_Channel stdout_channel = Tcl_CreateChannel(
      &stdout_channel_type_, "stdout", (ClientData) this, TCL_WRITABLE);
  if (stdout_channel) {
    Tcl_SetChannelOption(nullptr, stdout_channel, "-translation", "lf");
    Tcl_SetChannelOption(nullptr, stdout_channel, "-buffering", "none");
    Tcl_RegisterChannel(interp_, stdout_channel);  // per man page: some tcl bug
    Tcl_SetStdChannel(stdout_channel, TCL_STDOUT);
  }

  // Overwrite exit to allow Qt to handle exit
  Tcl_CreateCommand(interp_, "exit", ScriptWidget::tclExitHandler, this, nullptr);

  // Ensures no newlines are present in stdout stream when using logger, but normal behavior in file writing
  Tcl_Eval(interp_, "rename puts ::tcl::openroad::puts");
  Tcl_Eval(interp_, "proc puts { args } { if {[llength $args] == 1} { ::tcl::openroad::puts -nonewline {*}$args } else { ::tcl::openroad::puts {*}$args } }");

  pauser_->setText("Running");
  pauser_->setStyleSheet("background-color: red");
  ord::tclAppInit(interp_);
  pauser_->setText("Idle");
  pauser_->setStyleSheet("");

  // TODO: tclAppInit should return the status which we could
  // pass to updateOutput
  updateOutput(TCL_OK, /* command_finished */ true);
}

void ScriptWidget::executeCommand()
{
  pauser_->setText("Running");
  pauser_->setStyleSheet("background-color: red");
  QString command = input_->text();
  input_->clear();

  // Show the command that we executed
  output_->setTextColor(Qt::black);
  output_->append("> " + command);

  // Make changes visible while command runs
  QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

  int return_code = Tcl_Eval(interp_, command.toLatin1().data());

  // Show its output
  updateOutput(return_code, /* command_finished */ true);

  // Update history; ignore repeated commands and keep last 100
  const int history_limit = 100;
  if (history_.empty() || command != history_.last()) {
    if (history_.size() == history_limit) {
      history_.pop_front();
    }

    history_.append(command);
  }
  historyPosition_ = history_.size();
  pauser_->setText("Idle");
  pauser_->setStyleSheet("");
  emit commandExecuted();
}

void ScriptWidget::updateOutput(int return_code, bool command_finished)
{
  // Show whatever we captured from the output channel in grey
  output_->setTextColor(QColor(0x30, 0x30, 0x30));
  for (auto& out : outputBuffer_) {
    if (!out.isEmpty()) {
      output_->append(out);
    }
  }
  outputBuffer_.clear();

  if (command_finished) {
    // Show the return value color-coded by ok/err.
    const char* result = Tcl_GetString(Tcl_GetObjResult(interp_));
    if (result[0] != '\0') {
      output_->setTextColor((return_code == TCL_OK) ? Qt::blue : Qt::red);
      output_->append(result);
    }
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
  history_ = settings->value("history").toStringList();
  historyPosition_ = history_.size();
  settings->endGroup();
}

void ScriptWidget::writeSettings(QSettings* settings)
{
  settings->beginGroup("scripting");
  settings->setValue("history", history_);
  settings->endGroup();
}

void ScriptWidget::pause()
{
  QString prior_text = pauser_->text();
  bool prior_enable = pauser_->isEnabled();
  QString prior_style = pauser_->styleSheet();
  pauser_->setText("Continue");
  pauser_->setStyleSheet("background-color: yellow");
  pauser_->setEnabled(true);
  paused_ = true;

  // Keep processing events until the user continues
  while (paused_) {
    QCoreApplication::processEvents(QEventLoop::WaitForMoreEvents);
  }

  pauser_->setText(prior_text);
  pauser_->setStyleSheet(prior_style);
  pauser_->setEnabled(prior_enable);

  // Make changes visible while command runs
  QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
}

void ScriptWidget::pauserClicked()
{
  paused_ = false;
}

// This class is an spdlog sink that writes the messages into the output
// area.
template <typename Mutex>
class ScriptWidget::GuiSink : public spdlog::sinks::base_sink<Mutex>
{
 public:
  GuiSink(ScriptWidget* widget) : widget_(widget) {}

 protected:
  void sink_it_(const spdlog::details::log_msg& msg) override
  {
    // Convert the msg into a formatted string
    spdlog::memory_buf_t formatted;
    this->formatter_->format(msg, formatted);
    // -1 is to drop the final '\n' character
    auto str
        = QString::fromLatin1(formatted.data(), (int) formatted.size() - 1);
    widget_->outputBuffer_.append(str);

    // Make it appear now
    widget_->updateOutput(0, /* command_finished */ false);
    widget_->output_->update();
    QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
  }

  void flush_() override {}

 private:
  ScriptWidget* widget_;
};

void ScriptWidget::setLogger(utl::Logger* logger)
{
  logger_ = logger;
  logger->addSink(std::make_shared<GuiSink<std::mutex>>(this));
}

}  // namespace gui
