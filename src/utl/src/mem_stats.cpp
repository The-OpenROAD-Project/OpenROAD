// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

/*
 * Author:  David Robert Nadeau
 * Site:    http://NadeauSoftware.com/
 * License: Creative Commons Attribution 3.0 Unported License
 *          http://creativecommons.org/licenses/by/3.0/deed.en_US
 */

#include "utl/mem_stats.h"

#include <cstddef>

#ifdef _WIN32
#include <psapi.h>
#include <windows.h>

#elif defined(__unix__) || defined(__unix) || defined(unix) \
    || (defined(__APPLE__) && defined(__MACH__))
#include <sys/resource.h>
#include <unistd.h>

#if defined(__APPLE__) && defined(__MACH__)
#include <mach/mach.h>

#elif (defined(_AIX) || defined(__TOS__AIX__)) \
    || (defined(__sun__) || defined(__sun)     \
        || defined(sun) && (defined(__SVR4) || defined(__svr4__)))
#include <fcntl.h>
#include <procfs.h>

#elif defined(__linux__) || defined(__linux) || defined(linux) \
    || defined(__gnu_linux__)
#include <fstream>
#endif

#else
#error "Cannot define getPeakRSS( ) or getCurrentRSS( ) for an unknown OS."
#endif

namespace utl {

/**
 * Returns the peak (maximum so far) resident set size (physical
 * memory use) measured in bytes, or zero if the value cannot be
 * determined on this OS.
 */
size_t getPeakRSS()
{
#ifdef _WIN32
  /* Windows -------------------------------------------------- */
  PROCESS_MEMORY_COUNTERS info;
  GetProcessMemoryInfo(GetCurrentProcess(), &info, sizeof(info));
  return static_cast<size_t>(info.PeakWorkingSetSize);

#elif (defined(_AIX) || defined(__TOS__AIX__)) \
    || (defined(__sun__) || defined(__sun)     \
        || defined(sun) && (defined(__SVR4) || defined(__svr4__)))
  /* AIX and Solaris ------------------------------------------ */
  struct psinfo psinfo;
  int fd = -1;
  if ((fd = open("/proc/self/psinfo", O_RDONLY)) == -1) {
    return 0; /* Can't open? */
  }
  if (read(fd, &psinfo, sizeof(psinfo)) != sizeof(psinfo)) {
    close(fd);
    return 0; /* Can't read? */
  }
  close(fd);
  return static_cast<size_t>(psinfo.pr_rssize * 1024L);

#elif defined(__unix__) || defined(__unix) || defined(unix) \
    || (defined(__APPLE__) && defined(__MACH__))
  /* BSD, Linux, and OSX -------------------------------------- */
  struct rusage rusage;
  getrusage(RUSAGE_SELF, &rusage);
#if defined(__APPLE__) && defined(__MACH__)
  return static_cast<size_t>(rusage.ru_maxrss);
#else
  return static_cast<size_t>(rusage.ru_maxrss * 1024L);
#endif

#else
  /* Unknown OS ----------------------------------------------- */
  return 0; /* Unsupported. */
#endif
}

/**
 * Returns the current resident set size (physical memory use) measured
 * in bytes, or zero if the value cannot be determined on this OS.
 */
size_t getCurrentRSS()
{
#ifdef _WIN32
  /* Windows -------------------------------------------------- */
  PROCESS_MEMORY_COUNTERS info;
  GetProcessMemoryInfo(GetCurrentProcess(), &info, sizeof(info));
  return static_cast<size_t>(info.WorkingSetSize);

#elif defined(__APPLE__) && defined(__MACH__)
  /* OSX ------------------------------------------------------ */
  struct mach_task_basic_info info;
  mach_msg_type_number_t infoCount = MACH_TASK_BASIC_INFO_COUNT;
  if (task_info(mach_task_self(),
                MACH_TASK_BASIC_INFO,
                reinterpret_cast<task_info_t>(&info),
                &infoCount)
      != KERN_SUCCESS) {
    return 0; /* Can't access? */
  }
  return static_cast<size_t>(info.resident_size);

#elif defined(__linux__) || defined(__linux) || defined(linux) \
    || defined(__gnu_linux__)
  /* Linux ---------------------------------------------------- */
  /* statm fields are in pages: total program size, then resident size. */
  std::ifstream statm("/proc/self/statm");
  size_t size = 0;
  size_t rss = 0;
  if (!(statm >> size >> rss)) {
    return 0; /* Can't open or read? */
  }
  return rss * static_cast<size_t>(sysconf(_SC_PAGESIZE));

#else
  /* AIX, BSD, Solaris, and Unknown OS ------------------------ */
  return 0; /* Unsupported. */
#endif
}

}  // namespace utl
