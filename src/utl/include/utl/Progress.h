// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#pragma once

#include <signal.h>  // NOLINT(modernize-deprecated-headers): for sigaction

#include <atomic>
#include <csignal>
#include <ctime>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <string>
#include <vector>

namespace utl {

class Logger;
class ProgressReporter;

class Progress
{
 public:
  struct ProgressHalt
  {
    Progress* progress = nullptr;
    Logger* logger = nullptr;

    struct sigaction orig_sigaction;

    std::queue<std::time_t> interrupts;
    constexpr static int max_idle_interrupts = 2;
    constexpr static int max_in_progress_interrupts = 3;
    constexpr static int max_prune_delay = 10;
  };

  Progress(Logger* logger);
  virtual ~Progress();

  // Start an activity with progress based on number of iterations
  std::shared_ptr<ProgressReporter> startIterationReporting(
      const std::string& name,
      std::optional<int> total_work,
      std::optional<int> interval);

  // Start an activity with progress based on percentage of work
  std::shared_ptr<ProgressReporter> startPercentageReporting(
      const std::string& name,
      int total_work,
      std::optional<int> interval);

  // interrupt all reporters
  void interrupt();

  // returns true is reporters are active
  int countReporters();
  bool inProgress() { return countReporters() > 0; }

  // get all current reporters
  std::vector<std::shared_ptr<ProgressReporter>> getReporters();

  // In batch mode no ctrl-c handler is installed.
  static void setBatchMode(bool value) { batch_mode_ = value; }

 protected:
  // called at the start of a new reporter
  virtual void start(std::shared_ptr<ProgressReporter>& reporter) = 0;
  // called when a reporter is updated
  virtual void update(ProgressReporter* reporter) = 0;
  // called at the end of a reporter
  virtual void end(ProgressReporter* reporter) = 0;
  // called when a reporter is deleted
  virtual void deleted(ProgressReporter* reporter) = 0;

  Logger* logger_;

  friend class ProgressReporter;

 private:
  void addReporter(std::shared_ptr<ProgressReporter>& reporter);
  bool removeReporter(ProgressReporter* reporter);

  ProgressHalt signal_halt_;
  ProgressHalt* prev_signal_halt_ = nullptr;

  std::mutex reporters_lock_;
  std::vector<std::weak_ptr<ProgressReporter>> reporters_;

  static bool batch_mode_;
};

class ProgressReporter
{
 public:
  enum class ReportType
  {
    PERCENTAGE,
    ITERATION,
    NONE
  };

  ProgressReporter(Progress* progress,
                   ReportType type,
                   Logger* logger,
                   const std::string& name);
  ~ProgressReporter();

  std::string getName() const { return name_; }
  int getValue() const { return value_; }

  ReportType getType() const { return type_; }

  void setReportInterval(int interval) { report_interval_ = interval; }
  std::optional<int> getReportInterval() const { return report_interval_; }

  void setTotalWork(int total_work) { total_work_ = total_work; }
  std::optional<int> getTotalWork() const { return total_work_; }

  void setUseLogger(bool use_logger) { use_logger_ = use_logger; }
  bool usingLogger() const { return use_logger_; }

  // report progress and returns true is an interrupt has been requested
  bool reportProgress(int progress);
  // increment progress and returns true is an interrupt has been requested
  bool incrementProgress(int increment = 1);
  // returns true is an interrupt has been requested and allows for some
  // progressing
  bool hasInterrupt();
  // returns true is an interrupt has been requested
  bool checkInterrupt() const { return interrupt_; };

  // call at the end of reporting, if db_modified is true generates a warning if
  // an interrupt was requested, returns true is an interrupt has been requested
  bool end(bool db_modified);

  // interrupt this reporter
  void interrupt() { interrupt_ = true; }

  // returns a message to use for the current state of the reporter
  std::optional<std::string> getMessage();
  std::optional<std::string> getEndMessage();

  Progress* getProgressor() const { return progress_; }

 private:
  void report();

  Progress* progress_;
  Logger* logger_;

  std::string name_;
  const ReportType type_;

  bool use_logger_ = false;

  std::optional<int> total_work_;
  std::optional<int> report_interval_;

  // these can change during calls
  std::atomic<bool> interrupt_ = false;
  std::atomic<int> value_ = 0;
  std::atomic<int> next_percent_report_ = 0;
};

}  // namespace utl
