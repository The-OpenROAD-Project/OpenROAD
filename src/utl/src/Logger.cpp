/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2020, The Regents of the University of California
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

#include "utl/Logger.h"

#include <atomic>
#include <fstream>
#include <mutex>

#include "spdlog/pattern_formatter.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/ostream_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

namespace utl {

Logger::Logger(const char* log_filename, const char* metrics_filename)
    : string_redirect_(nullptr),
      file_redirect_(nullptr),
      debug_on_(false),
      warning_count_(0),
      error_count_(0)
{
  sinks_.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
  if (log_filename)
    sinks_.push_back(
        std::make_shared<spdlog::sinks::basic_file_sink_mt>(log_filename));

  logger_ = std::make_shared<spdlog::logger>(
      "logger", sinks_.begin(), sinks_.end());
  setFormatter();
  logger_->set_level(spdlog::level::level_enum::debug);

  if (metrics_filename)
    addMetricsSink(metrics_filename);

  metrics_policies_ = MetricsPolicy::makeDefaultPolicies();

  for (auto& counters : message_counters_) {
    for (auto& counter : counters) {
      counter = 0;
    }
  }
}

Logger::~Logger()
{
  finalizeMetrics();
}

void Logger::addMetricsSink(const char* metrics_filename)
{
  metrics_sinks_.push_back(metrics_filename);
}

void Logger::removeMetricsSink(const char* metrics_filename)
{
  auto metrics_file = std::find(
      metrics_sinks_.begin(), metrics_sinks_.end(), metrics_filename);
  if (metrics_file == metrics_sinks_.end()) {
    error(UTL, 2, "{} is not a metrics file", metrics_filename);
  }
  flushMetrics();

  metrics_sinks_.erase(metrics_file);
}

ToolId Logger::findToolId(const char* tool_name)
{
  int tool_id = 0;
  for (const char* tool : tool_names_) {
    if (strcmp(tool_name, tool) == 0)
      return static_cast<ToolId>(tool_id);
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
      debug_on_ = std::any_of(debug_group_level_.begin(),
                              debug_group_level_.end(),
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
  logger_->sinks().push_back(sink);
  setFormatter();  // updates the new sink
}

void Logger::removeSink(spdlog::sink_ptr sink)
{
  // remove from local list of sinks_
  auto sinks_find = std::find(sinks_.begin(), sinks_.end(), sink);
  if (sinks_find != sinks_.end()) {
    sinks_.erase(sinks_find);
  }
  // remove from spdlog list of sinks
  auto& logger_sinks = logger_->sinks();
  auto logger_find = std::find(logger_sinks.begin(), logger_sinks.end(), sink);
  if (logger_find != logger_sinks.end()) {
    logger_sinks.erase(logger_find);
  }
}

void Logger::setMetricsStage(std::string_view format)
{
  if (metrics_stages_.empty())
    metrics_stages_.push(std::string(format));
  else
    metrics_stages_.top() = format;
}

void Logger::clearMetricsStage()
{
  std::stack<std::string> new_stack;
  metrics_stages_.swap(new_stack);
}

void Logger::pushMetricsStage(std::string_view format)
{
  metrics_stages_.push(std::string(format));
}

std::string Logger::popMetricsStage()
{
  if (!metrics_stages_.empty()) {
    std::string stage = metrics_stages_.top();
    metrics_stages_.pop();
    return stage;
  } else {
    return "";
  }
}

void Logger::flushMetrics()
{
  const std::string json = MetricsEntry::assembleJSON(metrics_entries_);

  for (std::string sink_path : metrics_sinks_) {
    std::ofstream sink_file(sink_path);
    if (sink_file) {
      sink_file << json;
    } else {
      warn(UTL, 1, "Unable to open {} to write metrics", sink_path);
    }
  }
}

void Logger::finalizeMetrics()
{
  log_metric("flow__warnings__count", std::to_string(warning_count_));
  log_metric("flow__errors__count", std::to_string(error_count_));

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

void Logger::assertNoRedirect()
{
  if (string_redirect_ != nullptr || file_redirect_ != nullptr) {
    error(
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

void Logger::setFormatter()
{
  // create formatter without a newline
  std::unique_ptr<spdlog::formatter> formatter
      = std::make_unique<spdlog::pattern_formatter>(
          pattern_, spdlog::pattern_time_type::local, "");
  logger_->set_formatter(std::move(formatter));
}

}  // namespace utl
