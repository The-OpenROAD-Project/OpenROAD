/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2023, The Regents of the University of California
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

#include "utl/timer.h"

#include <sys/resource.h>

#include <fstream>

namespace utl {

void Timer::reset()
{
  start_ = Clock::now();
}

double Timer::elapsed() const
{
  return std::chrono::duration<double>{Clock::now() - start_}.count();
}

std::ostream& operator<<(std::ostream& os, const Timer& t)
{
  os << t.elapsed() << " sec";
  return os;
}

//////////////////////////

DebugScopedTimer::DebugScopedTimer(utl::Logger* logger,
                                   ToolId tool,
                                   const std::string& group,
                                   int level,
                                   const std::string& msg)
    : Timer(),
      logger_(logger),
      msg_(msg),
      tool_(tool),
      group_(group),
      level_(level)
{
}

DebugScopedTimer::~DebugScopedTimer()
{
  debugPrint(logger_, tool_, group_.c_str(), level_, msg_, *this);
}

ScopedStatistics::ScopedStatistics(utl::Logger* logger, std::string msg)
    : Timer(),
      msg_(std::move(msg)),
      start_rsz_(getStartRSZ()),
      start_vsz_(getStartVSZ()),
      logger_(logger)
{
}

size_t ScopedStatistics::getMemoryUsage(const char* tag)
{
  FILE* file = fopen("/proc/self/status", "r");
  if (file == nullptr) {
    perror("Failed to open /proc/self/status");
    exit(EXIT_FAILURE);
  }

  size_t result = (size_t) -1;
  char line[128];
  size_t tagLength = strlen(tag);

  while (fgets(line, sizeof(line), file) != nullptr) {
    if (strncmp(line, tag, tagLength) == 0) {
      const char* p = line;
      while (*p < '0' || *p > '9') {
        p++;
      }
      result = (size_t) atoi(p);
      break;
    }
  }

  fclose(file);
  if (result == (size_t) -1) {
    fprintf(stderr, "%s not found in /proc/self/status\n", tag);
    exit(EXIT_FAILURE);
  }

  return result / 1024;
}

size_t ScopedStatistics::getStartVSZ()
{
  return getMemoryUsage("VmSize:");
}

size_t ScopedStatistics::getPeakVSZ()
{
  return getMemoryUsage("VmPeak:");
}

size_t ScopedStatistics::getStartRSZ()
{
  return getMemoryUsage("VmRSS:");
}

size_t ScopedStatistics::getPeakRSZ()
{
  return getMemoryUsage("VmHWM:");
}

ScopedStatistics::~ScopedStatistics()
{
  logger_->report(msg_ + ": runtime {} seconds, usage: rsz = {} MB, vsz = {} MB, peak: rsz = {} MB, vsz = {} MB", static_cast<int>(Timer::elapsed()), (getPeakRSZ() - start_rsz_),
    (getPeakVSZ() - start_vsz_), (getPeakRSZ()), (getPeakVSZ()));
}

}  // namespace utl
