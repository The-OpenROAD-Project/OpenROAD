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
#include <QThread>
#include <QTimer>
#include <QVBoxLayout>
#include <mutex>

#include "gui/gui.h"
#include "ord/OpenRoad.hh"
#include "spdlog/formatter.h"
#include "spdlog/sinks/base_sink.h"

namespace gui {

ScriptWidget::ScriptWidget(QWidget* parent)
    : QDockWidget("Scripting", parent),
      output_(new QPlainTextEdit(this)),
      input_(new TclCmdInputWidget(this)),
      pauser_(new QPushButton("Idle", this)),
      pause_timer_(std::make_unique<QTimer>()),
      interp_(nullptr),
      history_(),
      history_buffer_last_(),
      historyPosition_(0),
      paused_(false),
      logger_(nullptr),
      buffer_outputs_(false),
      is_interactive_(true),
      sink_(nullptr)
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

  QWidget* container = new QWidget(this);
  container->setLayout(layout);

  connect(input_,
          SIGNAL(completeCommand(const QString&)),
          this,
          SLOT(executeCommand(const QString&)));
  connect(
      this, SIGNAL(commandExecuted(int)), input_, SLOT(commandExecuted(int)));
  connect(input_, SIGNAL(historyGoBack()), this, SLOT(goBackHistory()));
  connect(input_, SIGNAL(historyGoForward()), this, SLOT(goForwardHistory()));
  connect(input_, SIGNAL(textChanged()), this, SLOT(outputChanged()));
  connect(output_, SIGNAL(textChanged()), this, SLOT(outputChanged()));
  connect(pauser_, SIGNAL(pressed()), this, SLOT(pauserClicked()));
  connect(pause_timer_.get(), SIGNAL(timeout()), this, SLOT(unpause()));

  connect(this,
          SIGNAL(addToOutput(const QString&, const QColor&)),
          this,
          SLOT(addTextToOutput(const QString&, const QColor&)),
          Qt::QueuedConnection);

  setWidget(container);
}

ScriptWidget::~ScriptWidget()
{
  if (logger_ != nullptr) {
    // make sure to remove the Gui sink from logger
    logger_->removeSink(sink_);
  }

  // restore old exit
  Tcl_DeleteCommand(interp_, "exit");
  Tcl_Eval(interp_, "rename ::tcl::openroad::exit exit");
}

int ScriptWidget::tclExitHandler(ClientData instance_data,
                                 Tcl_Interp* interp,
                                 int argc,
                                 const char** argv)
{
  ScriptWidget* widget = (ScriptWidget*) instance_data;

  // exit was called, so ensure continue after close is cleared
  Gui::get()->clearContinueAfterClose();

  // announces exit to Qt
  emit widget->tclExiting();

  return TCL_OK;
}

void ScriptWidget::setupTcl(Tcl_Interp* interp,
                            bool interactive,
                            bool do_init_openroad,
                            const std::function<void(void)>& post_or_init)
{
  is_interactive_ = interactive;
  interp_ = interp;

  // Overwrite exit to allow Qt to handle exit
  Tcl_Eval(interp_, "rename exit ::tcl::openroad::exit");
  Tcl_CreateCommand(
      interp_, "exit", ScriptWidget::tclExitHandler, this, nullptr);

  if (do_init_openroad) {
    // OpenRoad is not initialized
    pauser_->setText("Running");
    pauser_->setStyleSheet("background-color: red");
    int setup_tcl_result = ord::tclAppInit(interp_);
    post_or_init();
    pauser_->setText("Idle");
    pauser_->setStyleSheet("");

    addTclResultToOutput(setup_tcl_result);
  } else {
    post_or_init();
    Gui::get()->load_design();
  }

  input_->init(interp_);
}

void ScriptWidget::executeCommand(const QString& command, bool echo)
{
  if (echo) {
    // Show the command that we executed
    addCommandToOutput(command);
  }

  int return_code = executeTclCommand(command);

  // Show its output
  addTclResultToOutput(return_code);

  if (return_code == TCL_OK) {
    if (echo) {
      // record the successful command to tcl history command
      Tcl_RecordAndEval(interp_, command.toLatin1().data(), TCL_NO_EVAL);
    }

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

  emit commandExecuted(return_code);
}

void ScriptWidget::executeSilentCommand(const QString& command)
{
  int return_code = executeTclCommand(command);

  if (return_code != TCL_OK) {
    // Show its error output
    addTclResultToOutput(return_code);
  }

  emit commandExecuted(return_code);
}

int ScriptWidget::executeTclCommand(const QString& command)
{
  pauser_->setText("Running");
  pauser_->setStyleSheet("background-color: red");

  emit commandAboutToExecute();
  QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

  int return_code = Tcl_Eval(interp_, command.toLatin1().data());

  pauser_->setText("Idle");
  pauser_->setStyleSheet("");

  return return_code;
}

void ScriptWidget::addCommandToOutput(const QString& cmd)
{
  const QString first_line_prefix = ">>> ";
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
    if (return_code == TCL_OK) {
      addToOutput(result, tcl_ok_msg_);
    } else {
      try {
        logger_->error(utl::GUI, 70, result);
      } catch (const std::runtime_error& e) {
        if (!is_interactive_) {
          // rethrow error
          throw e;
        }
      }
    }
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

void ScriptWidget::addTextToOutput(const QString& text, const QColor& color)
{
  // make sure cursor is at the end of the document
  output_->moveCursor(QTextCursor::End);

  QString output_text = text;
  if (text.endsWith('\n')) {
    // remove last new line
    output_text.chop(1);
  }

  QStringList output;
  for (QString& text_line : output_text.split('\n')) {
    // check for line length limits
    if (text_line.size() > max_output_line_length_) {
      text_line = text_line.left(max_output_line_length_ - 3);
      text_line += "...";
    }

    output.append(text_line.toHtmlEscaped());
  }

  // output new text
  // set new text color
  QString html = "<p style=\"color:" + color.name() + ";white-space: pre;\">";
  html += output.join("<br>");
  html += "</p>";
  output_->appendHtml(html);
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

void ScriptWidget::pause(int timeout)
{
  QString prior_text = pauser_->text();
  bool prior_enable = pauser_->isEnabled();
  QString prior_style = pauser_->styleSheet();
  pauser_->setText("Continue");
  pauser_->setStyleSheet("background-color: yellow");
  pauser_->setEnabled(true);
  paused_ = true;

  emit executionPaused();

  input_->setReadOnly(true);

  triggerPauseCountDown(timeout);

  // Keep processing events until the user continues
  while (paused_) {
    QCoreApplication::processEvents(QEventLoop::WaitForMoreEvents);
  }

  pauser_->setText(prior_text);
  pauser_->setStyleSheet(prior_style);
  pauser_->setEnabled(prior_enable);

  input_->setReadOnly(false);

  emit commandAboutToExecute();

  // Make changes visible while command runs
  QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
}

void ScriptWidget::unpause()
{
  paused_ = false;
}

void ScriptWidget::triggerPauseCountDown(int timeout)
{
  if (timeout == 0) {
    return;
  }

  pause_timer_->setInterval(timeout);
  pause_timer_->start();
  QTimer::singleShot(timeout, this, SLOT(updatePauseTimeout()));
  updatePauseTimeout();
}

void ScriptWidget::updatePauseTimeout()
{
  if (!paused_) {
    // already unpaused
    return;
  }

  const int one_second = 1000;

  int seconds = pause_timer_->remainingTime() / one_second;
  pauser_->setText("Continue (" + QString::number(seconds) + "s)");

  QTimer::singleShot(one_second, this, SLOT(updatePauseTimeout()));
}

void ScriptWidget::pauserClicked()
{
  pause_timer_->stop();
  paused_ = false;
}

void ScriptWidget::outputChanged()
{
  // ensure the new output is visible
  output_->ensureCursorVisible();
  // Make changes visible
  if (!buffer_outputs_) {
    QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
  }
}

void ScriptWidget::bufferOutputs(bool state)
{
  buffer_outputs_ = state;
}

void ScriptWidget::resizeEvent(QResizeEvent* event)
{
  input_->setMaximumHeight(event->size().height()
                           - output_->sizeHint().height());
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

    mutex_.lock();  // formatter checks and caches some information
    this->formatter_->format(msg, formatted);
    mutex_.unlock();
    const QString formatted_msg = QString::fromStdString(
        std::string(formatted.data(), formatted.size()));

    if (msg.level == spdlog::level::level_enum::off) {
      // this comes from a ->report
      widget_->addReportToOutput(formatted_msg);
    } else {
      // select error message color if message level is error or above.
      const QColor& msg_color = msg.level >= spdlog::level::level_enum::err
                                    ? widget_->tcl_error_msg_
                                    : widget_->buffer_msg_;

      widget_->addLogToOutput(formatted_msg, msg_color);
    }

    // process widget event queue, if main thread will process new text,
    // otherwise there is nothing to process from this thread.
    if (QThread::currentThread() == widget_->thread()) {
      QCoreApplication::sendPostedEvents(widget_);
    }
  }

  void flush_() override {}

 private:
  ScriptWidget* widget_;
  std::mutex mutex_;
};

void ScriptWidget::setLogger(utl::Logger* logger)
{
  // use spdlog::details::null_mutex instead of std::mutex, Qt will handle the
  // thread transfers to the output viewer null_mutex prevents deadlock (by not
  // locking) when the logging causes a redraw, which then causes another
  // logging event.
  sink_ = std::make_shared<GuiSink<spdlog::details::null_mutex>>(this);
  logger_ = logger;
  logger->addSink(sink_);
}

}  // namespace gui
