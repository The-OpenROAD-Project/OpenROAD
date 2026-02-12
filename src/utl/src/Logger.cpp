// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#include "utl/Logger.h"

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <memory>
#include <ostream>
#include <sstream>
#include <stack>
#include <string>
#include <string_view>
#include <utility>

#include "CommandLineProgress.h"
#include "utl/Metrics.h"
#if SPDLOG_VERSION < 10601
#include "spdlog/details/pattern_formatter.h"
#else
#include "spdlog/pattern_formatter.h"
#endif
#include "spdlog/common.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/ostream_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"
#include "utl/Progress.h"
#include "utl/prometheus/metrics_server.h"
#include "utl/prometheus/registry.h"

namespace utl {

Logger::Logger(const char* log_filename, const char* metrics_filename)
{
  progress_ = std::make_unique<CommandLineProgress>(this);

  sinks_.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
  if (log_filename) {
    sinks_.push_back(
        std::make_shared<spdlog::sinks::basic_file_sink_mt>(log_filename));
  }

  logger_ = std::make_shared<spdlog::logger>(
      "logger", sinks_.begin(), sinks_.end());
  setFormatter();
  logger_->set_level(spdlog::level::level_enum::debug);

  if (metrics_filename) {
    addMetricsSink(metrics_filename);
  }

  metrics_policies_ = MetricsPolicy::makeDefaultPolicies();

  for (auto& counters : message_counters_) {
    for (auto& counter : counters) {
      counter = 0;
    }
  }

  for (auto& levels : message_levels_) {
    for (auto& level : levels) {
      level.store(spdlog::level::off, std::memory_order_relaxed);
    }
  }

  prometheus_registry_ = std::make_shared<PrometheusRegistry>();
}

Logger::~Logger()
{
  finalizeMetrics();
}

void Logger::addMetricsSink(const char* metrics_filename)
{
  metrics_sinks_.emplace_back(metrics_filename);
}

void Logger::removeMetricsSink(const char* metrics_filename)
{
  auto metrics_file = std::ranges::find(metrics_sinks_, metrics_filename);
  if (metrics_file == metrics_sinks_.end()) {
    this->error(UTL, 11, "{} is not a metrics file", metrics_filename);
  }
  flushMetrics();

  metrics_sinks_.erase(metrics_file);
}

ToolId Logger::findToolId(const char* tool_name)
{
  int tool_id = 0;
  for (const char* tool : tool_names_) {
    if (strcmp(tool_name, tool) == 0) {
      return static_cast<ToolId>(tool_id);
    }
    tool_id++;
  }
  return UKN;
}

void Logger::setDebugLevel(ToolId tool, const char* group, int level)
{
  if (level == 0) {
    auto& groups = debug_group_level_[tool];
    auto it = groups.find(group);
    if (it != groups.end()) {
      groups.erase(it);
      debug_on_
          = std::ranges::any_of(debug_group_level_,

                                [](auto& group) { return !group.empty(); });
    }
  } else {
    debug_on_ = true;
    debug_group_level_.at(tool)[group] = level;
  }
}

void Logger::addSink(spdlog::sink_ptr sink)
{
  sinks_.push_back(sink);
  logger_->sinks().emplace_back(std::move(sink));
  setFormatter();  // updates the new sink
}

void Logger::removeSink(const spdlog::sink_ptr& sink)
{
  // remove from local list of sinks_
  auto sinks_find = std::ranges::find(sinks_, sink);
  if (sinks_find != sinks_.end()) {
    sinks_.erase(sinks_find);
  }
  // remove from spdlog list of sinks
  auto& logger_sinks = logger_->sinks();
  auto logger_find = std::ranges::find(logger_sinks, sink);
  if (logger_find != logger_sinks.end()) {
    logger_sinks.erase(logger_find);
  }
}

void Logger::setMetricsStage(std::string_view format)
{
  if (metrics_stages_.empty()) {
    metrics_stages_.emplace(format);
  } else {
    metrics_stages_.top() = format;
  }
}

void Logger::clearMetricsStage()
{
  std::stack<std::string> new_stack;
  metrics_stages_.swap(new_stack);
}

void Logger::pushMetricsStage(std::string_view format)
{
  metrics_stages_.emplace(format);
}

std::string Logger::popMetricsStage()
{
  if (!metrics_stages_.empty()) {
    std::string stage = metrics_stages_.top();
    metrics_stages_.pop();
    return stage;
  }
  return "";
}

void Logger::flushMetrics()
{
  const std::string json = MetricsEntry::assembleJSON(metrics_entries_);

  for (const std::string& sink_path : metrics_sinks_) {
    std::ofstream sink_file(sink_path);
    if (sink_file) {
      sink_file << json;
    } else {
      this->warn(UTL, 10, "Unable to open {} to write metrics", sink_path);
    }
  }
}

void Logger::addWarningMetrics()
{
  // Add metrics for non-zero warnings
  int warning_type_cnt = 0;
  for (int i = 0; i < ToolId::SIZE; ++i) {
    for (int j = 0; j <= max_message_id; ++j) {
      if (message_counters_[i][j] > 0
          && message_levels_[i][j] == spdlog::level::warn) {
        warning_type_cnt++;
        log_metric(
            // NOLINTNEXTLINE(misc-include-cleaner)
            fmt::format("flow__warnings__count:{}-{:04}", tool_names_[i], j),
            std::to_string(message_counters_[i][j]));
      }
    }
  }

  // Add a metric to report the number of unique warning types
  log_metric("flow__warnings__type_count", std::to_string(warning_type_cnt));
}

void Logger::finalizeMetrics()
{
  log_metric("flow__warnings__count", std::to_string(warning_count_));
  log_metric("flow__errors__count", std::to_string(error_count_));

  addWarningMetrics();

  for (MetricsPolicy policy : metrics_policies_) {
    policy.applyPolicy(metrics_entries_);
  }

  flushMetrics();
}

void Logger::suppressMessage(ToolId tool, int id)
{
  message_counters_[tool][id] = max_message_print + 1;
}

void Logger::unsuppressMessage(ToolId tool, int id)
{
  message_counters_[tool][id] = 0;
}

void Logger::redirectFileBegin(const std::string& filename)
{
  assertNoRedirect();

  file_redirect_ = std::make_unique<std::ofstream>(filename);
  setRedirectSink(*file_redirect_);
}

void Logger::redirectFileAppendBegin(const std::string& filename)
{
  assertNoRedirect();

  file_redirect_
      = std::make_unique<std::ofstream>(filename, std::ofstream::app);
  setRedirectSink(*file_redirect_);
}

void Logger::redirectFileEnd()
{
  if (file_redirect_ == nullptr) {
    return;
  }

  restoreFromRedirect();

  file_redirect_->close();
  file_redirect_ = nullptr;
}

void Logger::teeFileBegin(const std::string& filename)
{
  assertNoRedirect();

  file_redirect_ = std::make_unique<std::ofstream>(filename);
  setRedirectSink(*file_redirect_, true);
}

void Logger::teeFileAppendBegin(const std::string& filename)
{
  assertNoRedirect();

  file_redirect_
      = std::make_unique<std::ofstream>(filename, std::ofstream::app);
  setRedirectSink(*file_redirect_, true);
}

void Logger::teeFileEnd()
{
  redirectFileEnd();
}

void Logger::redirectStringBegin()
{
  assertNoRedirect();

  string_redirect_ = std::make_unique<std::ostringstream>();
  setRedirectSink(*string_redirect_);
}

std::string Logger::redirectStringEnd()
{
  if (string_redirect_ == nullptr) {
    return "";
  }

  restoreFromRedirect();

  std::string string = string_redirect_->str();
  string_redirect_ = nullptr;

  return string;
}

void Logger::teeStringBegin()
{
  assertNoRedirect();

  string_redirect_ = std::make_unique<std::ostringstream>();
  setRedirectSink(*string_redirect_, true);
}

std::string Logger::teeStringEnd()
{
  return redirectStringEnd();
}

Logger* Logger::defaultLogger()
{
  static Logger default_logger;
  return &default_logger;
}

void Logger::assertNoRedirect()
{
  if (string_redirect_ != nullptr || file_redirect_ != nullptr) {
    this->error(
        UTL, 102, "Unable to start new log redirect while another is active.");
  }
}

void Logger::setRedirectSink(std::ostream& sink_stream, bool keep_sinks)
{
  if (!keep_sinks) {
    logger_->sinks().clear();
  }

  logger_->sinks().push_back(
      std::make_shared<spdlog::sinks::ostream_sink_mt>(sink_stream, true));
  setFormatter();
}

void Logger::restoreFromRedirect()
{
  logger_->sinks().clear();
  logger_->sinks().insert(
      logger_->sinks().begin(), sinks_.begin(), sinks_.end());
}

void Logger::startPrometheusEndpoint(uint16_t port)
{
  if (prometheus_metrics_) {
    return;
  }

  prometheus_metrics_ = std::make_unique<PrometheusMetricsServer>(
      prometheus_registry_, this, port);
}

std::shared_ptr<PrometheusRegistry> Logger::getRegistry()
{
  return prometheus_registry_;
}

bool Logger::isPrometheusServerReadyToServe()
{
  if (!prometheus_metrics_) {
    return false;
  }

  return prometheus_metrics_->is_ready() && prometheus_metrics_->port() != 0;
}

uint16_t Logger::getPrometheusPort()
{
  if (!prometheus_metrics_) {
    return 0;
  }

  return prometheus_metrics_->port();
}

void Logger::setFormatter()
{
  // create formatter without a newline
  std::unique_ptr<spdlog::formatter> formatter
      = std::make_unique<spdlog::pattern_formatter>(
          pattern_, spdlog::pattern_time_type::local, "");
  logger_->set_formatter(std::move(formatter));
}

std::unique_ptr<Progress> Logger::swapProgress(Progress* progress)
{
  std::unique_ptr<Progress> current_progress = std::move(progress_);
  progress_.reset(progress);

  return current_progress;
}

}  // namespace utl
