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

#pragma once

#include <array>
#include <cstdlib>
#include <iomanip>
#include <limits>
#include <map>
#include <sstream>
#include <stack>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include "Metrics.h"
#include "spdlog/fmt/fmt.h"
#include "spdlog/fmt/ostr.h"
#include "spdlog/spdlog.h"

namespace utl {

// Keep this sorted
#define FOREACH_TOOL(X) \
  X(ANT)                \
  X(CTS)                \
  X(DFT)                \
  X(DPL)                \
  X(DPO)                \
  X(DRT)                \
  X(DST)                \
  X(FIN)                \
  X(FLW)                \
  X(GPL)                \
  X(GRT)                \
  X(GUI)                \
  X(IFP)                \
  X(MPL)                \
  X(ODB)                \
  X(ORD)                \
  X(PAD)                \
  X(PAR)                \
  X(PDN)                \
  X(PDR)                \
  X(PPL)                \
  X(PSM)                \
  X(PSN)                \
  X(RCX)                \
  X(RMP)                \
  X(RSZ)                \
  X(STA)                \
  X(STT)                \
  X(TAP)                \
  X(UKN)                \
  X(UPF)                \
  X(UTL)

#define GENERATE_ENUM(ENUM) ENUM,
#define GENERATE_STRING(STRING) #STRING,

// backward compatibility with fmt versions older than 8
#if FMT_VERSION >= 80000
#define FMT_RUNTIME(format_string) fmt::runtime(format_string)
#else
#define FMT_RUNTIME(format_string) format_string
#endif

enum ToolId
{
  FOREACH_TOOL(GENERATE_ENUM)
      SIZE  // the number of tools, do not put anything after this
};

class Logger
{
 public:
  // Use nullptr if messages or metrics are not logged to a file.
  Logger(const char* filename = nullptr,
         const char* metrics_filename = nullptr);
  ~Logger();
  static ToolId findToolId(const char* tool_name);

  template <typename... Args>
  inline void report(const std::string& message, const Args&... args)
  {
    logger_->log(spdlog::level::level_enum::off, FMT_RUNTIME(message), args...);
  }

  // Do NOT call this directly, use the debugPrint macro  instead (defined
  // below)
  template <typename... Args>
  inline void debug(ToolId tool,
                    const std::string& group,
                    const std::string& message,
                    const Args&... args)
  {
    // Message counters do NOT apply to debug messages.
    logger_->log(spdlog::level::level_enum::debug,
                 FMT_RUNTIME("[{} {}-{}] " + message),
                 level_names[spdlog::level::level_enum::debug],
                 tool_names_[tool],
                 group,
                 args...);
    logger_->flush();
  }

  template <typename... Args>
  inline void info(ToolId tool,
                   int id,
                   const std::string& message,
                   const Args&... args)
  {
    log(tool, spdlog::level::level_enum::info, id, message, args...);
  }

  template <typename... Args>
  inline void warn(ToolId tool,
                   int id,
                   const std::string& message,
                   const Args&... args)
  {
    log(tool, spdlog::level::level_enum::warn, id, message, args...);
  }

  template <typename... Args>
  __attribute__((noreturn)) inline void error(ToolId tool,
                                              int id,
                                              const std::string& message,
                                              const Args&... args)
  {
    log(tool, spdlog::level::err, id, message, args...);
    char tool_id[32];
    sprintf(tool_id, "%s-%04d", tool_names_[tool], id);
    std::runtime_error except(tool_id);
    // Exception should be caught by swig error handler.
    throw except;
  }

  template <typename... Args>
  __attribute__((noreturn)) void critical(ToolId tool,
                                          int id,
                                          const std::string& message,
                                          const Args&... args)
  {
    log(tool, spdlog::level::level_enum::critical, id, message, args...);
    exit(EXIT_FAILURE);
  }

  // For logging to the metrics file.  This is a much more restricted
  // API as we are writing JSON not user messages.
  // Note: these methods do no escaping so avoid special characters.
  template <typename T,
            typename U = std::enable_if_t<std::is_arithmetic<T>::value>>
  inline void metric(const std::string_view metric_name, T value)
  {
    const std::string name = std::string(metric_name);
    if (std::isinf(value)) {
      if (value < 0) {
        metric(name, "-Infinity");
      } else {
        metric(name, "Infinity");
      }
    } else if (std::isnan(value)) {
      metric(name, "NaN");
    } else {
      std::ostringstream oss;
      oss << std::defaultfloat << std::setprecision(6) << value;
      log_metric(name, oss.str());
    }
  }

  inline void metric(const std::string_view metric, const std::string& value)
  {
    log_metric(std::string(metric), '"' + value + '"');
  }

  void setDebugLevel(ToolId tool, const char* group, int level);

  bool debugCheck(ToolId tool, const char* group, int level) const
  {
    if (!debug_on_) {
      return false;
    }
    auto& groups = debug_group_level_[tool];
    auto it = groups.find(group);
    return (it != groups.end() && level <= it->second);
  }

  void suppressMessage(ToolId tool, int id);

  void addSink(spdlog::sink_ptr sink);
  void removeSink(spdlog::sink_ptr sink);
  void addMetricsSink(const char* metrics_filename);
  void removeMetricsSink(const char* metrics_filename);

  void setMetricsStage(std::string_view format);
  void clearMetricsStage();
  void pushMetricsStage(std::string_view format);
  std::string popMetricsStage();

 private:
  std::vector<std::string> metrics_sinks_;
  std::list<MetricsEntry> metrics_entries_;
  std::vector<MetricsPolicy> metrics_policies_;

  template <typename... Args>
  inline void log(ToolId tool,
                  spdlog::level::level_enum level,
                  int id,
                  const std::string& message,
                  const Args&... args)
  {
    assert(id >= 0 && id <= max_message_id);
    auto& counter = message_counters_[tool][id];
    auto count = counter++;
    if (count < max_message_print) {
      logger_->log(level,
                   FMT_RUNTIME("[{} {}-{:04d}] " + message),
                   level_names[level],
                   tool_names_[tool],
                   id,
                   args...);
      return;
    }

    if (count == max_message_print) {
      logger_->log(level,
                   "[{} {}-{:04d}] message limit reached, "
                   "this message will no longer print",
                   level_names[level],
                   tool_names_[tool],
                   id);
    } else {
      counter--;  // to avoid counter overflow
    }
  }

  inline void log_metric(const std::string metric, const std::string value)
  {
    std::string key;
    if (metrics_stages_.empty())
      key = metric;
    else
      key = fmt::format(FMT_RUNTIME(metrics_stages_.top()), metric);
    metrics_entries_.push_back({key, value});
  }

  void flushMetrics();
  void finalizeMetrics();

  // Allows for lookup by a compatible key (ie string_view)
  // to avoid constructing a key (string) just for lookup
  struct StringViewCmp
  {
    using is_transparent = std::true_type;  // enabler
    bool operator()(const std::string_view a, const std::string_view b) const
    {
      return a < b;
    }
  };
  using DebugGroups = std::map<std::string, int, StringViewCmp>;

  static constexpr int max_message_id = 9999;

  // Stop issuing messages of a given tool/id when this limit is hit.
  static int max_message_print;

  std::vector<spdlog::sink_ptr> sinks_;
  std::shared_ptr<spdlog::logger> logger_;
  std::stack<std::string> metrics_stages_;

  // This matrix is pre-allocated so it can be safely updated
  // from multiple threads without locks.
  using MessageCounter = std::array<short, max_message_id + 1>;
  std::array<MessageCounter, ToolId::SIZE> message_counters_;
  std::array<DebugGroups, ToolId::SIZE> debug_group_level_;
  bool debug_on_;
  static constexpr const char* level_names[]
      = {"TRACE", "DEBUG", "INFO", "WARNING", "ERROR", "CRITICAL", "OFF"};
  static constexpr const char* pattern_ = "%v";
  static constexpr const char* tool_names_[] = {FOREACH_TOOL(GENERATE_STRING)};
};

// Use this macro for any debug messages.  It avoids evaluating message and
// varargs when no message is issued.
#define debugPrint(logger, tool, group, level, ...) \
  if (logger->debugCheck(tool, group, level)) {     \
    logger->debug(tool, group, ##__VA_ARGS__);      \
  }

#undef FOREACH_TOOL
#undef GENERATE_ENUM
#undef GENERATE_STRING

}  // namespace utl
