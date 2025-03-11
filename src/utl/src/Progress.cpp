/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2025, The Regents of the University of California
// All rights reserved.
//
// BSD 3-Clause License
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
//
///////////////////////////////////////////////////////////////////////////////

#include "utl/Progress.h"

#include "utl/Logger.h"

namespace utl {

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

  if (signal_halt->interrupts.size()
      >= Progress::ProgressHalt::max_interrupts) {
    // halt the application
    signal(sig, SIG_DFL);
    raise(sig);
  } else if (signal_halt->interrupts.size() + 1
             >= Progress::ProgressHalt::max_interrupts) {
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

  signal_halt_.org_signal_handler = signal(SIGINT, controlC_handler);
}

Progress::~Progress()
{
  signal(SIGINT, signal_halt_.org_signal_handler);
  signal_halt = prev_signal_halt_;
}

std::shared_ptr<ProgressReporter> Progress::start(const std::string& name)
{
  auto reporter = std::make_shared<ProgressReporter>(
      this, ProgressReporter::ReportType::NONE, logger_, name);

  addReporter(reporter);

  start(reporter);

  return reporter;
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
  reporters_.erase(
      std::remove_if(reporters_.begin(),
                     reporters_.end(),
                     [reporter, &found_reporter](
                         const std::weak_ptr<ProgressReporter>& other) -> bool {
                       const auto other_ptr = other.lock();
                       if (!other_ptr) {
                         return true;
                       }

                       bool found = other_ptr.get() == reporter;
                       found_reporter |= found;
                       return found;
                     }),
      reporters_.end());
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
