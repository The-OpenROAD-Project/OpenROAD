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

#if defined(__unix__) || defined(__unix) || defined(unix) \
    || (defined(__APPLE__) && defined(__MACH__))
#include <sys/resource.h>
#include <unistd.h>

#if defined(__APPLE__) && defined(__MACH__)
#include <mach/mach.h>

#elif defined(__linux) || defined(linux) || defined(__gnu_linux__)
#include <cstdint>
#include <cstdio>
#endif

#endif

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
      logger_(logger),
      cpu_start_(clock())
{
}

size_t ScopedStatistics::getStartRSZ()
{
#if defined(__APPLE) && defined(__MACH__)
  struct mach_task_basic_info info;
  mach_msg_type_number_t info_count = MACH_TASK_BASIC_INFO_COUNT;
  if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO,
                (task_info_t) &info, &info_count) {
    return (size_t) 0L;
  }
  return (size_t) info.resident_size / 1024;
#elif defined(__linux) || defined(linux) || defined(__gnu_linux__)
  int64_t rss = 0L;
  FILE* fp = fopen("/proc/self/statm", "r");
  if (fp == nullptr) {
    return (size_t) 0L;
  }
  if (fscanf(fp, "%*s%ld", &rss) != 1) {
    fclose(fp);
    return (size_t) 0L;
  }
  fclose(fp);
  return static_cast<size_t>(rss) * static_cast<size_t>(sysconf(_SC_PAGESIZE))
         / (1024 * 1024);
#else
  return (size_t) 0L; /* Unsupported. */
#endif
}

size_t ScopedStatistics::getPeakRSZ()
{
#if defined(__unix__) || defined(__unix) || defined(unix) \
    || (defined(__APPLE__) && defined(__MACH__))
  struct rusage rsg;
  if (getrusage(RUSAGE_SELF, &rsg) != 0) {
    return (size_t) 0L;
  }
#if defined(__APPLE__) && defined(__MACH__)
  return ((size_t) rsg.ru_maxrss) / (1024 * 1024);
#else
  return (size_t) rsg.ru_maxrss / 1024;
#endif
#endif
}

ScopedStatistics::~ScopedStatistics()
{
  auto ts_elapsed = Timer::elapsed();
  int hour = ts_elapsed / 3600;
  int min = (((int) ts_elapsed % 3600) / 60);
  int sec = (int) ts_elapsed % 60;

  auto ts_cpu = (clock() - cpu_start_) / CLOCKS_PER_SEC;
  int chour = ts_cpu / 3600;
  int cmin = (ts_cpu % 3600) / 60;
  int csec = ts_cpu % 60;

  logger_->report(msg_ + ": cpu time = {:02}:{:02}:{:02}, elapsed time = {:02}:{:02}:{:02}, memory = {} MB, peak = {} MB",
      chour, cmin, csec, hour, min, sec, start_rsz_, getPeakRSZ());
}

}  // namespace utl
