// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#pragma once

#include <array>
#include <atomic>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <ios>
#include <list>
#include <map>
#include <memory>
#include <ostream>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "spdlog/details/os.h"
#include "spdlog/fmt/fmt.h"
#include "spdlog/fmt/ostr.h"
#include "utl/Metrics.h"
#if FMT_VERSION >= 110000
#include "spdlog/fmt/ranges.h"
#endif

#include "spdlog/spdlog.h"

namespace utl {

class PrometheusMetricsServer;
class PrometheusRegistry;

class Progress;

// Keep this sorted
#define FOREACH_TOOL(X) \
  X(ANT)                \
  X(CGT)                \
  X(CHK)                \
  X(CTS)                \
  X(CUT)                \
  X(DFT)                \
  X(DPL)                \
  X(DRT)                \
  X(DST)                \
  X(EST)                \
  X(EXA)                \
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
  X(PPL)                \
  X(PSM)                \
  X(RAM)                \
  X(RCX)                \
  X(RMP)                \
  X(RSZ)                \
  X(STA)                \
  X(STT)                \
  X(TAP)                \
  X(TST)                \
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
  Logger(const Logger& logger) = delete;
  ~Logger();
  static ToolId findToolId(const char* tool_name);

  template <typename... Args>
  void report(const std::string& message, const Args&... args)
  {
    logger_->log(spdlog::level::level_enum::off,
                 FMT_RUNTIME(message + spdlog::details::os::default_eol),
                 args...);
  }

  // Reports a string literal with no interpolation or newline.
  template <typename... Args>
  void reportLiteral(const std::string& message)
  {
    logger_->log(spdlog::level::level_enum::off, "{}", message);
  }

  // Do NOT call this directly, use the debugPrint macro  instead (defined
  // below)
  template <typename... Args>
  void debug(ToolId tool,
             const char* group,
             const std::string& message,
             const Args&... args)
  {
    // Message counters do NOT apply to debug messages.
    logger_->log(
        spdlog::level::level_enum::debug,
        FMT_RUNTIME("[{} {}-{}] " + message + spdlog::details::os::default_eol),
        level_names[spdlog::level::level_enum::debug],
        tool_names_[tool],
        group,
        args...);
    logger_->flush();
  }

  template <typename... Args>
  void info(ToolId tool,
            int id,
            const std::string& message,
            const Args&... args)
  {
    log(tool, spdlog::level::level_enum::info, id, message, args...);
  }

  template <typename... Args>
  void warn(ToolId tool,
            int id,
            const std::string& message,
            const Args&... args)
  {
    warning_count_++;
    log(tool, spdlog::level::level_enum::warn, id, message, args...);
  }

  template <typename... Args>
  __attribute__((noreturn)) void error(ToolId tool,
                                       int id,
                                       const std::string& message,
                                       const Args&... args)
  {
    error_count_++;
    log(tool, spdlog::level::err, id, message, args...);
    char tool_id[32];
    sprintf(tool_id, "%s-%04d", tool_names_[tool], id);
    // Exception should be caught by swig error handler.
    throw std::runtime_error(tool_id);
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
  template <typename T, typename U = std::enable_if_t<std::is_arithmetic_v<T>>>
  void metric(const std::string_view metric_name, T value)
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

  void metric(const std::string_view metric, const std::string& value)
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

  int getWarningCount() const { return warning_count_; }

  void startPrometheusEndpoint(uint16_t port);
  std::shared_ptr<PrometheusRegistry> getRegistry();
  bool isPrometheusServerReadyToServe();
  uint16_t getPrometheusPort();

  void suppressMessage(ToolId tool, int id);
  void unsuppressMessage(ToolId tool, int id);

  void addSink(spdlog::sink_ptr sink);
  void removeSink(const spdlog::sink_ptr& sink);
  void addMetricsSink(const char* metrics_filename);
  void removeMetricsSink(const char* metrics_filename);

  void setMetricsStage(std::string_view format);
  void clearMetricsStage();
  void pushMetricsStage(std::string_view format);
  std::string popMetricsStage();

  // interface from sta::Report
  // Redirect output to filename until redirectFileEnd is called.
  void redirectFileBegin(const std::string& filename);
  // Redirect append output to filename until redirectFileEnd is called.
  void redirectFileAppendBegin(const std::string& filename);
  void redirectFileEnd();
  // Redirect output to a string until redirectStringEnd is called.
  void redirectStringBegin();
  std::string redirectStringEnd();
  // Tee output to filename until teeFileEnd is called.
  void teeFileBegin(const std::string& filename);
  // Tee append output to filename until teeFileEnd is called.
  void teeFileAppendBegin(const std::string& filename);
  void teeFileEnd();
  // Redirect output to a string until teeStringEnd is called.
  void teeStringBegin();
  std::string teeStringEnd();

  static Logger* defaultLogger();

  // Progress interface
  Progress* progress() const { return progress_.get(); }
  std::unique_ptr<Progress> swapProgress(Progress* progress);

 private:
  std::vector<std::string> metrics_sinks_;
  std::list<MetricsEntry> metrics_entries_;
  std::vector<MetricsPolicy> metrics_policies_;

  std::unique_ptr<Progress> progress_;

  template <typename... Args>
  void log(ToolId tool,
           spdlog::level::level_enum level,
           int id,
           const std::string& message,
           const Args&... args)
  {
    assert(id >= 0 && id <= max_message_id);
    message_levels_[tool][id].store(level, std::memory_order_relaxed);
    auto& counter = message_counters_[tool][id];
    auto count = counter++;
    if (count < max_message_print) {
      logger_->log(level,
                   FMT_RUNTIME("[{} {}-{:04d}] " + message
                               + spdlog::details::os::default_eol),
                   level_names[level],
                   tool_names_[tool],
                   id,
                   args...);
      return;
    }

    if (count == max_message_print) {
      logger_->log(level,
                   "[{} {}-{:04d}] message limit ({})"
                   " reached. This message will no longer print.{}",
                   level_names[level],
                   tool_names_[tool],
                   id,
                   max_message_print,
                   spdlog::details::os::default_eol);
    } else {
      counter--;  // to avoid counter overflow
    }
  }

  void log_metric(const std::string& metric, const std::string& value)
  {
    std::string key;
    if (metrics_stages_.empty()) {
      key = metric;
    } else {
      key = fmt::format(FMT_RUNTIME(metrics_stages_.top()), metric);
    }
    metrics_entries_.push_back({std::move(key), value});
  }

  void flushMetrics();
  void finalizeMetrics();
  // Add new metrics for non-zero warnings. It also counts the number of
  // unique warning types.
  void addWarningMetrics();

  void setRedirectSink(std::ostream& sink_stream, bool keep_sinks = false);
  void restoreFromRedirect();
  void assertNoRedirect();

  void setFormatter();

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
  static constexpr int max_message_print = 1000;

  std::vector<spdlog::sink_ptr> sinks_;
  std::shared_ptr<spdlog::logger> logger_;
  std::stack<std::string> metrics_stages_;

  // interface to handle string and file redirections
  std::unique_ptr<std::ostringstream> string_redirect_;
  std::unique_ptr<std::ofstream> file_redirect_;

  // Prometheus server metrics collection
  std::shared_ptr<PrometheusRegistry> prometheus_registry_;
  std::unique_ptr<PrometheusMetricsServer> prometheus_metrics_;

  // This matrix is pre-allocated so it can be safely updated
  // from multiple threads without locks.
  using MessageCounter = std::array<std::atomic_int16_t, max_message_id + 1>;
  std::array<MessageCounter, ToolId::SIZE> message_counters_;
  using MessageLevel
      = std::array<std::atomic<spdlog::level::level_enum>, max_message_id + 1>;
  std::array<MessageLevel, ToolId::SIZE> message_levels_;
  std::array<DebugGroups, ToolId::SIZE> debug_group_level_;
  bool debug_on_{false};
  std::atomic_int warning_count_{0};
  std::atomic_int error_count_{0};
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

// Define stream_as for fmt > v10
#if !SWIG && FMT_VERSION >= 100000

namespace utl {

struct test_ostream
{
 public:
  template <class T>
  static auto test(int) -> decltype(std::declval<std::ostream>()
                                        << std::declval<T>(),
                                    std::true_type());

  template <class>
  static auto test(...) -> std::false_type;
};

template <class T,
          class = std::enable_if_t<decltype(test_ostream::test<T>(0))::value>>
auto format_as(const T& t)
{
  return fmt::streamed(t);
}

}  // namespace utl

#else
namespace utl {

// Uncallable class
template <class T>
class format_as
{
};

}  // namespace utl
#endif
