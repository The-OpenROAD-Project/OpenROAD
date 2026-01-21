// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include "utl/Progress.h"

#include <signal.h>  // NOLINT(modernize-deprecated-headers): for sigaction

#include <algorithm>
#include <csignal>
#include <ctime>
#include <memory>
#include <mutex>
#include <optional>
#include <vector>

#include "utl/Logger.h"

namespace utl {

bool Progress::batch_mode_{false};

static Progress::ProgressHalt* signal_halt = nullptr;

static void controlC_handler(int sig)
{
  if (signal_halt == nullptr) {
    return;
  }

  const auto now = std::time(nullptr);
  signal_halt->interrupts.push(now);

  // Remove old interrupts
  while (std::difftime(now, signal_halt->interrupts.front())
         >= Progress::ProgressHalt::max_prune_delay) {
    signal_halt->interrupts.pop();
  }

  const int max_interrupts
      = signal_halt->progress->inProgress()
            ? Progress::ProgressHalt::max_in_progress_interrupts
            : Progress::ProgressHalt::max_idle_interrupts;

  if (signal_halt->interrupts.size() >= max_interrupts) {
    // halt the application
    signal(sig, SIG_DFL);
    raise(sig);
  } else if (signal_halt->interrupts.size() + 1 >= max_interrupts) {
    // let user know one more will terminate openroad
    signal_halt->logger->warn(
        utl::UTL, 14, "OpenROAD will terminate after the next control+C");
  }

  if (signal_halt->progress == nullptr) {
    return;
  }

  // request interrupt
  signal_halt->progress->interrupt();
}

Progress::Progress(Logger* logger) : logger_(logger)
{
  signal_halt_.progress = this;
  signal_halt_.logger = logger_;

  prev_signal_halt_ = signal_halt;
  signal_halt = &signal_halt_;

  if (!batch_mode_) {
    struct sigaction act;
    act.sa_handler = controlC_handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGINT, &act, &signal_halt_.orig_sigaction);
  }
}

Progress::~Progress()
{
  if (!batch_mode_) {
    sigaction(SIGINT, &signal_halt_.orig_sigaction, nullptr);
  }
  signal_halt = prev_signal_halt_;
}

std::shared_ptr<ProgressReporter> Progress::startIterationReporting(
    const std::string& name,
    std::optional<int> total_work,
    std::optional<int> interval)
{
  auto reporter = std::make_shared<ProgressReporter>(
      this, ProgressReporter::ReportType::ITERATION, logger_, name);
  if (total_work.has_value()) {
    reporter->setTotalWork(total_work.value());
  }
  if (interval.has_value()) {
    reporter->setReportInterval(interval.value());
  }

  addReporter(reporter);

  start(reporter);

  return reporter;
}

std::shared_ptr<ProgressReporter> Progress::startPercentageReporting(
    const std::string& name,
    int total_work,
    std::optional<int> interval)
{
  auto reporter = std::make_shared<ProgressReporter>(
      this, ProgressReporter::ReportType::PERCENTAGE, logger_, name);
  reporter->setTotalWork(total_work);
  if (interval.has_value()) {
    reporter->setReportInterval(interval.value());
  }

  addReporter(reporter);

  start(reporter);

  return reporter;
}

void Progress::addReporter(std::shared_ptr<ProgressReporter>& reporter)
{
  std::unique_lock<std::mutex> lock(reporters_lock_);
  reporters_.push_back(reporter);
}

bool Progress::removeReporter(ProgressReporter* reporter)
{
  std::unique_lock<std::mutex> lock(reporters_lock_);

  if (reporters_.empty()) {
    return false;
  }

  bool found_reporter = false;
  std::erase_if(reporters_, [&](const auto& other) {
    const auto other_ptr = other.lock();
    if (!other_ptr) {
      return true;
    }

    bool found = other_ptr.get() == reporter;
    found_reporter |= found;
    return found;
  });
  return found_reporter;
}

void Progress::interrupt()
{
  std::unique_lock<std::mutex> lock(reporters_lock_);
  for (const auto& reporter : reporters_) {
    if (auto report = reporter.lock()) {
      report->interrupt();
    }
  }
}

std::vector<std::shared_ptr<ProgressReporter>> Progress::getReporters()
{
  std::unique_lock<std::mutex> lock(reporters_lock_);

  std::vector<std::shared_ptr<ProgressReporter>> reporters;

  for (const auto& reporter : reporters_) {
    if (auto rep = reporter.lock()) {
      reporters.push_back(rep);
    }
  }

  return reporters;
}

int Progress::countReporters()
{
  std::unique_lock<std::mutex> lock(reporters_lock_);

  int count = 0;

  std::vector<std::shared_ptr<ProgressReporter>> reporters;

  for (const auto& reporter : reporters_) {
    if (!reporter.expired()) {
      count++;
    }
  }

  return count;
}

///////////////////////////////////////////////////////////////////

ProgressReporter::ProgressReporter(Progress* progress,
                                   ReportType type,
                                   Logger* logger,
                                   const std::string& name)
    : progress_(progress), logger_(logger), name_(name), type_(type)
{
}

ProgressReporter::~ProgressReporter()
{
  progress_->deleted(this);
}

bool ProgressReporter::reportProgress(int progress)
{
  value_ = progress;

  report();

  return hasInterrupt();
}

bool ProgressReporter::incrementProgress(int increment)
{
  value_ += increment;

  report();

  return hasInterrupt();
}

void ProgressReporter::report()
{
  if (usingLogger()) {
    const auto msg = getMessage();
    if (msg) {
      logger_->report(msg.value());
    }
  }
}

bool ProgressReporter::end(bool db_modified)
{
  progress_->removeReporter(this);

  if (checkInterrupt() && db_modified) {
    logger_->error(utl::UTL,
                   13,
                   "{} was interupted and may have modified the database.",
                   name_);
  }

  if (usingLogger()) {
    const auto msg = getEndMessage();
    if (msg) {
      logger_->report(msg.value());
    }
  }

  progress_->end(this);

  return checkInterrupt();
}

bool ProgressReporter::hasInterrupt()
{
  progress_->update(this);

  return interrupt_;
}

std::optional<std::string> ProgressReporter::getMessage()
{
  switch (type_) {
    case ReportType::NONE:
      return {};
    case ReportType::ITERATION: {
      const int interval = report_interval_.value_or(1);
      if (value_ % interval == 0) {
        return fmt::format("{} iteration{}: {}",
                           name_,
                           interrupt_ ? "*" : "",
                           static_cast<int>(value_));
      }
      break;
    }
    case ReportType::PERCENTAGE: {
      const float perc
          = 100 * static_cast<float>(value_) / total_work_.value_or(100);
      if (perc >= next_percent_report_) {
        const int interval = report_interval_.value_or(10);
        while (next_percent_report_ <= perc) {
          next_percent_report_ += interval;
        }

        return fmt::format(
            "{}{}: {:3.1f}%", name_, interrupt_ ? "*" : "", perc);
      }
      break;
    }
  }

  return {};
}

std::optional<std::string> ProgressReporter::getEndMessage()
{
  if (checkInterrupt()) {
    return fmt::format("{} finishied with interrupt", name_);
  }

  switch (type_) {
    case ReportType::NONE:
      return {};
    case ReportType::ITERATION: {
      return fmt::format("{} done", name_);
    }
    case ReportType::PERCENTAGE: {
      return fmt::format("{}: {:3.1f}%", name_, 100.0);
    }
  }

  return {};
}

}  // namespace utl
