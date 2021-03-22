/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2020, OpenROAD
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

#include "utility/Logger.h"

#include <mutex>
#include <atomic>

#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

namespace utl {

Logger::Logger(const char* log_filename, const char *metrics_filename)
  : debug_on_(false),
    first_metric_(true)
{
  // This ensures it is safe to update the message counters
  // without using locks.
  static_assert(std::atomic<MessageCounter::value_type>::is_always_lock_free,
                "message counter should be atomic");

 sinks_.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
  if (log_filename)
    sinks_.push_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(log_filename));
  
  logger_ = std::make_shared<spdlog::logger>("logger", sinks_.begin(), sinks_.end());
  logger_->set_pattern(pattern_);
  logger_->set_level(spdlog::level::level_enum::debug);

  metrics_logger_ = std::make_shared<spdlog::logger>("metrics");
  if (metrics_filename) {
    auto metrics_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(metrics_filename);
    metrics_logger_->sinks().push_back(metrics_sink);
    metrics_logger_->set_pattern("%v");
    metrics_logger_->info("{"); // start json object
  }

  for (auto& counters : message_counters_) {
    counters.fill(0);
  }
}

Logger::~Logger()
{
  // Terminate the json object before we disappear
  metrics_logger_->info("}");
}

ToolId
Logger::findToolId(const char *tool_name)
{
  int tool_id = 0;
  for (const char *tool : tool_names_) {
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
                              [](auto& group) { return !group.empty(); }
                              );
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
  logger_->set_pattern(pattern_); // updates the new sink
}

}  // namespace
