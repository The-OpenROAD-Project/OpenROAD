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

#pragma once

#include <string>
#include <vector>
#include <array>
#include <map>
#include <string_view>
#include <cstdlib>
#include <type_traits>

#include "spdlog/spdlog.h"
#include "spdlog/fmt/ostr.h"

namespace utl {

#define FOREACH_TOOL(X) \
    X(ANT) \
    X(CTS) \
    X(DPL) \
    X(DRT) \
    X(FIN) \
    X(GPL) \
    X(GRT) \
    X(GUI) \
    X(PAD) \
    X(IFP) \
    X(MPL) \
    X(ODB) \
    X(ORD) \
    X(PAR) \
    X(PDN) \
    X(PPL) \
    X(PSM) \
    X(PSN) \
    X(RCX) \
    X(RSZ) \
    X(STA) \
    X(STT) \
    X(TAP) \
    X(UKN) \

#define GENERATE_ENUM(ENUM) ENUM,
#define GENERATE_STRING(STRING) #STRING,

enum ToolId
{
 FOREACH_TOOL(GENERATE_ENUM)
 SIZE // the number of tools, do not put anything after this
};

enum LogMode {
  FULL,
  QUIET,
  SILENT,
};

class Logger
{
 public:
  // Use nullptr if messages or metrics are not logged to a file.
  // Passing in true for quiet_logs will set the log level of stdout to warning
  // and above.
  //
  // Passing in true for silent_logs will disable stdout logging.
  Logger(const char* filename = nullptr, const char* metrics_filename = nullptr,
         const bool quiet_logs = false, const bool silent_logs = false);
  ~Logger();
  static ToolId findToolId(const char *tool_name);

  template <typename... Args>
    inline void report(const std::string& message,
                       const Args&... args)
    {
      spdlog::level::level_enum report_level = spdlog::level::level_enum::off;

      if (log_mode_ == LogMode::QUIET || log_mode_ == LogMode::SILENT) {
        report_level = spdlog::level::level_enum::info;
      }

      logger_->log(report_level, message, args...);
    }

  // Do NOT call this directly, use the debugPrint macro  instead (defined below)
  template <typename... Args>
    inline void debug(ToolId tool,
                      int level,
                      const std::string& message,
                      const Args&... args)
    {
      log(tool, spdlog::level::level_enum::debug, /*id*/ level, message, args...);
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
    __attribute__((noreturn))
    inline void error(ToolId tool,
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
    __attribute__((noreturn))
    void critical(ToolId tool,
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
            typename = std::enable_if_t<std::is_arithmetic_v<T>>>
  inline void metric(ToolId tool,
                     const std::string_view metric,
                     T value)
  {
    log_metric(tool, metric, value);
  }

  inline void metric(ToolId tool,
                     const std::string_view metric,
                     const std::string& value)
  {
    log_metric(tool, metric, '"' +  value + '"');
  }

  void setDebugLevel(ToolId tool, const char* group, int level);

  bool debugCheck(ToolId tool, const char* group, int level) const {
      if (!debug_on_) {
        return false;
      }
      auto& groups = debug_group_level_[tool];
      auto it = groups.find(group);
      return (it != groups.end() && level <= it->second);
  }

  void addSink(spdlog::sink_ptr sink);

 private:
  template <typename... Args>
    inline void log(ToolId tool,
                    spdlog::level::level_enum level,
                    int id,
                    const std::string& message,
                    const Args&... args)
    {
      assert(id >= 0 && id <= 9999);
      logger_->log(level,
                   "[{} {}-{:04d}] " + message,
                   level_names[level],
                   tool_names_[tool],
                   id,
                   args...);
    }

  template <typename Value>
    inline void log_metric(ToolId tool,
                           const std::string_view metric,
                           const Value& value)
    {
      metrics_logger_->info("  {}\"{}::{}\" : {}",
                            first_metric_ ? "  " : ", ",
                            tool_names_[tool],
                            metric,
                            value);
      first_metric_ = false;
    }

  // Allows for lookup by a compatible key (ie string_view)
  // to avoid constructing a key (string) just for lookup
  struct StringViewCmp {
    using is_transparent = std::true_type; // enabler
    bool operator()(const std::string_view a, const std::string_view b) const {
      return a < b;
    }
  };
  using DebugGroups = std::map<std::string, int, StringViewCmp>;

  std::vector<spdlog::sink_ptr> sinks_;
  std::shared_ptr<spdlog::logger> logger_;
  std::shared_ptr<spdlog::logger> metrics_logger_;
  LogMode log_mode_;

  std::array<DebugGroups, ToolId::SIZE> debug_group_level_;
  bool debug_on_;
  bool first_metric_;
  static constexpr const char *level_names[] = {"TRACE",
                                                "DEBUG",
                                                "INFO",
                                                "WARNING",
                                                "ERROR",
                                                "CRITICAL",
                                                "OFF"};
  static constexpr const char *pattern_ = "%v";
  static constexpr const char* tool_names_[] = { FOREACH_TOOL(GENERATE_STRING) };
};

// Use this macro for any debug messages.  It avoids evaluating message and varargs
// when no message is issued.
#define debugPrint(logger, tool, group, level, message, ...)   \
  if (logger->debugCheck(tool, group, level)) {                \
    logger->debug(tool, level, message, ##__VA_ARGS__); \
  }

#undef FOREACH_TOOL
#undef GENERATE_ENUM
#undef GENERATE_STRING

}  // namespace utl
