// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#include "scriptWidget.h"

#include <unistd.h>

#include <QColor>
#include <QCoreApplication>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QPushButton>
#include <QSettings>
#include <QThread>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>
#include <cerrno>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>

#include "absl/synchronization/mutex.h"
#include "gui/gui.h"
#include "spdlog/formatter.h"
#include "spdlog/sinks/base_sink.h"
#include "tcl.h"
#include "tclCmdInputWidget.h"
#include "utl/Logger.h"

namespace gui {

ScriptWidget::ScriptWidget(QWidget* parent)
    : QDockWidget("Scripting", parent),
      output_(new QPlainTextEdit(this)),
      input_(new TclCmdInputWidget(this)),
      pauser_(new QPushButton("Idle", this)),
      pause_timer_(std::make_unique<QTimer>()),
      report_timer_(std::make_unique<QTimer>()),
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
  pause_timer_->setSingleShot(true);

  QHBoxLayout* inner_layout = new QHBoxLayout;
  inner_layout->addWidget(pauser_);
  inner_layout->addWidget(input_);

  QVBoxLayout* layout = new QVBoxLayout;
  layout->addWidget(output_, /* stretch */ 1);
  layout->addLayout(inner_layout);

  QWidget* container = new QWidget(this);
  container->setLayout(layout);

  connect(input_,
          &TclCmdInputWidget::textChanged,
          this,
          &ScriptWidget::outputChanged);

  connect(input_, &TclCmdInputWidget::exiting, this, &ScriptWidget::exiting);
  connect(input_,
          &TclCmdInputWidget::commandAboutToExecute,
          this,
          &ScriptWidget::commandAboutToExecute);
  connect(input_,
          &TclCmdInputWidget::commandAboutToExecute,
          this,
          &ScriptWidget::setPauserToRunning);
  connect(input_,
          &TclCmdInputWidget::addCommandToOutput,
          this,
          &ScriptWidget::addCommandToOutput);
  connect(input_,
          &TclCmdInputWidget::addResultToOutput,
          this,
          &ScriptWidget::addResultToOutput);
  connect(input_,
          &TclCmdInputWidget::addTextToOutput,
          this,
          &ScriptWidget::addTextToOutput,
          Qt::QueuedConnection);
  connect(input_,
          &TclCmdInputWidget::commandFinishedExecuting,
          this,
          &ScriptWidget::resetPauser);
  connect(input_,
          &TclCmdInputWidget::commandFinishedExecuting,
          this,
          &ScriptWidget::commandExecuted);
  connect(input_,
          &TclCmdInputWidget::commandFinishedExecuting,
          this,
          &ScriptWidget::flushReportBufferToOutput);
  connect(output_,
          &QPlainTextEdit::textChanged,
          this,
          &ScriptWidget::outputChanged);
  connect(pauser_, &QPushButton::pressed, this, &ScriptWidget::pauserClicked);
  connect(pause_timer_.get(), &QTimer::timeout, this, &ScriptWidget::unpause);
  connect(report_timer_.get(),
          &QTimer::timeout,
          this,
          &ScriptWidget::flushReportBufferToOutput);

  connect(this,
          &ScriptWidget::addToOutput,
          this,
          &ScriptWidget::addTextToOutput,
          Qt::QueuedConnection);

  setWidget(container);
}

// When displaying text in Scripting, processing events in order to
// ensure the text is visible to the user can considerably slow down
// the logging, so, for reports, we use a buffer.
void ScriptWidget::flushReportBufferToOutput()
{
  if (!reporting_.TryLock()) {
    // failed to aquire lock
    // return and this will be called at some point later
    QTimer::singleShot(
        kReportDisplayInterval, this, &ScriptWidget::flushReportBufferToOutput);
    return;
  }

  if (!report_buffer_.isEmpty()) {
    // this comes from a ->report
    addTextToOutput(report_buffer_, buffer_msg_);
    report_buffer_.clear();
  }
  reporting_.Unlock();
}

ScriptWidget::~ScriptWidget()
{
  // When _input is destroyed it can trigger this connection resulting
  // in a crash.
  disconnect(input_,
             &TclCmdInputWidget::textChanged,
             this,
             &ScriptWidget::outputChanged);
  if (logger_ != nullptr) {
    // make sure to remove the Gui sink from logger
    logger_->removeSink(sink_);
  }
}

void ScriptWidget::setupTcl(Tcl_Interp* interp,
                            bool interactive,
                            bool do_init_openroad,
                            const std::function<void()>& post_or_init)
{
  is_interactive_ = interactive;
  input_->setTclInterp(interp, do_init_openroad, post_or_init);
}

void ScriptWidget::executeCommand(const QString& command, bool echo)
{
  input_->executeCommand(command, echo);
}

void ScriptWidget::executeSilentCommand(const QString& command)
{
  input_->executeCommand(command, false, true);
}

void ScriptWidget::setPauserToRunning()
{
  pauser_->setText("Running");
  pauser_->setStyleSheet("background-color: red");
}

void ScriptWidget::resetPauser()
{
  pauser_->setText("Idle");
  pauser_->setStyleSheet("");
}

void ScriptWidget::addCommandToOutput(const QString& cmd)
{
  const QString first_line_prefix = ">>> ";
  const QString continue_line_prefix = "... ";

  QString command = first_line_prefix + cmd;
  command.replace("\n", "\n" + continue_line_prefix);

  addToOutput(command, cmd_msg_);
}

void ScriptWidget::addResultToOutput(const QString& result, bool is_ok)
{
  if (result.isEmpty()) {
    return;
  }

  if (is_ok) {
    addToOutput(result, ok_msg_);
  } else {
    try {
      auto msg = result.toStdString();
      if (msg.find(TclCmdInputWidget::kExitString) == std::string::npos) {
        logger_->error(utl::GUI, 70, msg);
      }
    } catch (const std::runtime_error& e) {
      if (!is_interactive_) {
        throw;
      }
    }
  }
}

void ScriptWidget::addLogToOutput(const QString& text, const QColor& color)
{
  addToOutput(text, color);
}

void ScriptWidget::startReportTimer()
{
  report_timer_->start(kReportDisplayInterval);
}

void ScriptWidget::addMsgToReportBuffer(const QString& text)
{
  absl::MutexLock guard(&reporting_);
  report_buffer_ += text;
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

void ScriptWidget::readSettings(QSettings* settings)
{
  settings->beginGroup(objectName());
  input_->readSettings(settings);
  settings->endGroup();
}

void ScriptWidget::writeSettings(QSettings* settings)
{
  settings->beginGroup(objectName());
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

void ScriptWidget::setCommand(const QString& command)
{
  input_->setText(command);
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
  QTimer::singleShot(timeout, this, &ScriptWidget::updatePauseTimeout);
  updatePauseTimeout();
}

void ScriptWidget::updatePauseTimeout()
{
  if (!paused_) {
    // already unpaused
    return;
  }

  const int one_second = 1000;

  if (pause_timer_->isActive()) {
    const int seconds = pause_timer_->remainingTime() / one_second;
    pauser_->setText("Continue (" + QString::number(seconds) + "s)");

    QTimer::singleShot(one_second, this, &ScriptWidget::updatePauseTimeout);
  }
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
    // buffer outputs in case additional outputs are generated
    buffer_outputs_ = true;
    QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    buffer_outputs_ = false;
  }
}

void ScriptWidget::bufferOutputs(bool state)
{
  buffer_outputs_ = state;
}

void ScriptWidget::resizeEvent(QResizeEvent* event)
{
  input_->setMaxHeight(event->size().height() - output_->sizeHint().height());
  QDockWidget::resizeEvent(event);
}

void ScriptWidget::setWidgetFont(const QFont& font)
{
  QDockWidget::setFont(font);
  output_->setFont(font);
  input_->setWidgetFont(font);
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

    {
      // formatter checks and caches some information
      absl::MutexLock guard(&mutex_);
      this->formatter_->format(msg, formatted);
    }
    const QString formatted_msg = QString::fromStdString(
        std::string(formatted.data(), formatted.size()));

    if (msg.level == spdlog::level::level_enum::off) {
      widget_->addMsgToReportBuffer(formatted_msg);

      if (QThread::currentThread() == widget_->thread()) {
        if (!widget_->report_timer_->isActive()) {
          widget_->startReportTimer();
        }

        // the event loop is blocked so we need to manually process
        // events so that the timer keeps going
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
      }
    } else {
      // select error message color if message level is error or above.
      const QColor& msg_color = msg.level >= spdlog::level::level_enum::err
                                    ? widget_->error_msg_
                                    : widget_->buffer_msg_;

      widget_->flushReportBufferToOutput();
      widget_->addLogToOutput(formatted_msg, msg_color);
    }

    // process widget event queue, if main thread will process new text,
    // otherwise there is nothing to process from this thread.
    if (QThread::currentThread() == widget_->thread()) {
      if (!formatted_msg.contains("XcbConnection")) {
        // there is a slim chance of a deadlock if we update the GUI
        // from within a Qt error handler.
        QCoreApplication::sendPostedEvents(widget_);
      }
    }
  }

  void flush_() override {}

 private:
  ScriptWidget* widget_;
  absl::Mutex mutex_;
};

void ScriptWidget::setLogger(utl::Logger* logger)
{
  // use spdlog::details::null_mutex instead of absl::Mutex, Qt will handle the
  // thread transfers to the output viewer null_mutex prevents deadlock (by not
  // locking) when the logging causes a redraw, which then causes another
  // logging event.
  sink_ = std::make_shared<GuiSink<spdlog::details::null_mutex>>(this);
  logger_ = logger;
  logger->addSink(sink_);
}

}  // namespace gui
