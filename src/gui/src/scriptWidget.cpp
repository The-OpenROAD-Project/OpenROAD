///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2019, The Regents of the University of California
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
#include <QTimer>
#include <QVBoxLayout>

#include "ord/OpenRoad.hh"
#include "utl/Logger.h"
#include "spdlog/formatter.h"
#include "spdlog/sinks/base_sink.h"

namespace gui {

ScriptWidget::ScriptWidget(QWidget* parent)
    : QDockWidget("Scripting", parent),
      output_(new QTextEdit),
      input_(new TclCmdInputWidget),
      pauser_(new QPushButton("Idle")),
      historyPosition_(0)
{
  setObjectName("scripting");  // for settings

  output_->setReadOnly(true);
  pauser_->setEnabled(false);
  pauser_->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);

  QHBoxLayout* inner_layout = new QHBoxLayout;
  inner_layout->addWidget(pauser_);
  inner_layout->addWidget(input_);

  QVBoxLayout* layout = new QVBoxLayout;
  layout->addWidget(output_, /* stretch */ 1);
  layout->addLayout(inner_layout);

  QWidget* container = new QWidget;
  container->setLayout(layout);

  QTimer::singleShot(200, this, &ScriptWidget::setupTcl);

  connect(input_, SIGNAL(completeCommand()), this, SLOT(executeCommand()));
  connect(this, SIGNAL(commandExecuted(int)), input_, SLOT(commandExecuted(int)));
  connect(input_, SIGNAL(historyGoBack()), this, SLOT(goBackHistory()));
  connect(input_, SIGNAL(historyGoForward()), this, SLOT(goForwardHistory()));
  connect(input_, SIGNAL(textChanged()), this, SLOT(outputChanged()));
  connect(output_, SIGNAL(textChanged()), this, SLOT(outputChanged()));
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
  addTclResultToOutput(TCL_OK);

  input_->init(interp_);
}

void ScriptWidget::executeCommand()
{
  pauser_->setText("Running");
  pauser_->setStyleSheet("background-color: red");
  QString command = input_->text();

  // Show the command that we executed
  addCommandToOutput(command);

  int return_code = Tcl_Eval(interp_, command.toLatin1().data());

  // Show its output
  addTclResultToOutput(return_code);

  if (return_code == TCL_OK) {
    // record the successful command to tcl history command
    Tcl_RecordAndEval(interp_, command.toLatin1().data(), TCL_NO_EVAL);

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

  pauser_->setText("Idle");
  pauser_->setStyleSheet("");

  emit commandExecuted(return_code);
}

void ScriptWidget::addCommandToOutput(const QString& cmd)
{
  const QString first_line_prefix    = ">>> ";
  const QString continue_line_prefix = "... ";

  QString command = first_line_prefix + cmd;
  command.replace("\n", "\n" + continue_line_prefix);

  addToOutput(command, cmd_msg_);
}

void ScriptWidget::addTclResultToOutput(int return_code)
{
  // Show the return value color-coded by ok/err.
  const char* result = Tcl_GetString(Tcl_GetObjResult(interp_));
  if (result[0] != '\0') {
    addToOutput(result, (return_code == TCL_OK) ? tcl_ok_msg_ : tcl_error_msg_);
  }
}

void ScriptWidget::addLogToOutput(const QString& text, const QColor& color)
{
  addToOutput(text, color);
}

void ScriptWidget::addReportToOutput(const QString& text)
{
  addToOutput(text, buffer_msg_);
}

void ScriptWidget::addToOutput(const QString& text, const QColor& color)
{
  // make sure cursor is at the end of the document
  output_->moveCursor(QTextCursor::End);

  QString output_text = text;
  if (text.endsWith('\n')) {
    // remove last new line
    output_text.chop(1);
  }

  // set new text color
  output_->setTextColor(color);

  QStringList output;
  for (QString& text_line : output_text.split('\n')) {
    // check for line length limits
    if (text_line.size() > max_output_line_length_) {
      text_line = text_line.left(max_output_line_length_-3);
      text_line += "...";
    }

    output.append(text_line);
  }
  // output new text
  output_->append(output.join("\n"));

  // ensure changes are updated
  QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
}

ScriptWidget::~ScriptWidget()
{
  // TODO: I am being lazy and not cleaning up the tcl interpreter.
  // We are likely exiting anyways
}

void ScriptWidget::goForwardHistory()
{
  if (historyPosition_ < history_.size() - 1) {
    ++historyPosition_;
    input_->setText(history_[historyPosition_]);
  } else if (historyPosition_ == history_.size() - 1) {
    ++historyPosition_;
    input_->setText(history_buffer_last_);
  }
}

void ScriptWidget::goBackHistory()
{
  if (historyPosition_ > 0) {
    if (historyPosition_ == history_.size()) {
      // whats in the buffer is the last thing the user was editing
      history_buffer_last_ = input_->text();
    }
    --historyPosition_;
    input_->setText(history_[historyPosition_]);
  }
}

void ScriptWidget::readSettings(QSettings* settings)
{
  settings->beginGroup("scripting");
  history_ = settings->value("history").toStringList();
  historyPosition_ = history_.size();

  input_->readSettings(settings);

  settings->endGroup();
}

void ScriptWidget::writeSettings(QSettings* settings)
{
  settings->beginGroup("scripting");
  settings->setValue("history", history_);

  input_->writeSettings(settings);

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

void ScriptWidget::outputChanged()
{
  // ensure the new output is visible
  output_->ensureCursorVisible();
  // Make changes visible
  QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
}

void ScriptWidget::resizeEvent(QResizeEvent* event)
{
  input_->setMaximumHeight(event->size().height() - output_->sizeHint().height());
  QDockWidget::resizeEvent(event);
}

void ScriptWidget::setFont(const QFont& font)
{
  QDockWidget::setFont(font);
  output_->setFont(font);
  input_->setFont(font);
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
    std::string formatted_msg = std::string(formatted.data(), formatted.size());

    if (msg.level == spdlog::level::level_enum::off) {
      // this comes from a ->report
      widget_->addReportToOutput(formatted_msg.c_str());
    }
    else {
      // select error message color if message level is error or above.
      const QColor& msg_color = msg.level >= spdlog::level::level_enum::err ? widget_->tcl_error_msg_ : widget_->buffer_msg_;

      widget_->addLogToOutput(formatted_msg.c_str(), msg_color);
    }
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
