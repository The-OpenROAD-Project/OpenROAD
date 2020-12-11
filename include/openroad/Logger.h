/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2019, OpenROAD
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

#include "spdlog/spdlog.h"

namespace ord {

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
    X(END) \

#define GENERATE_ENUM(ENUM) ENUM,
#define GENERATE_STRING(STRING) #STRING,

enum ToolId
{
 FOREACH_TOOL(GENERATE_ENUM)
};

static const char* tool_name_tbl[] = {
 FOREACH_TOOL(GENERATE_STRING)
};

#undef FOREACH_TOOL
#undef GENERATE_ENUM
#undef GENERATE_STRING

constexpr const char *level_names[] = {"TRACE",
                                       "DEBUG",
                                       "INFO",
                                       "WARNING",
                                       "ERROR",
                                       "CRITICAL",
                                       "OFF"};

extern std::shared_ptr<spdlog::logger> logger;

// Use nullptr if messages are not logged to a file.
void initLogger(const char* filename);

template <typename... Args>
inline void report(const std::string& message,
                   const Args&... args)
{
  logger->log(spdlog::level::level_enum::off, message, args...);
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
inline void warn2(ToolId tool,
                  int id,
                  const std::string& message,
                  const Args&... args)
{
  log(tool, spdlog::level::level_enum::warn, id, message, args...);
}

template <typename... Args>
inline void error2(ToolId tool,
                   int id,
                   const std::string& message,
                   const Args&... args)
{
  log(tool, spdlog::level::err, id, message, args...);
  char tool_id[32];
  sprintf(tool_id, "%s-%04d", tool_name_tbl[tool], id);
  std::runtime_error except(tool_id);
  // Exception should be caught by swig error handler.
  throw except;
}

template <typename... Args>
void critical(ToolId tool,
              int id,
              const std::string& message,
              const Args&... args)
{
  log(tool, spdlog::level::level_enum::critical, id, message, args...);
}

template <typename... Args>
inline void log(ToolId tool,
                spdlog::level::level_enum level,
                int id,
                const std::string& message,
                const Args&... args)
{
  assert(id >= 0 && id <= 9999);
  logger->log(level,
              "[{} {}-{:04d}] " + message,
              level_names[level],
              tool_name_tbl[tool],
              id,
              args...);
}

}  // namespace ordlog
