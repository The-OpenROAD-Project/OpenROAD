///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2019, Nefelus Inc
// All rights reserved.
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

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <fstream>
#include <iostream>
#if sun == 1
#include <fcntl.h>
#include <procfs.h>
#endif

#include <assert.h>
#include <stdlib.h>

#include "utl/Logger.h"

using namespace std;

int Ath__double2int(double v)
{
  int iv = (int) v;
  if (iv > 0) {
    if (v > (double) iv + 0.5)
      iv++;
  } else if (iv < 0) {
    if ((double) iv > v + 0.5)
      iv--;
  }
  return iv;
}
namespace odb {

unsigned int AthHashFunction(char* key, unsigned int len, unsigned int prime)
{
  unsigned int hash, i;
  for (hash = len, i = 0; i < len; ++i)
    hash = (hash << 4) ^ (hash >> 28) ^ key[i];
  return (hash % prime);
}

int AthGetProcessMem(uint64* size, uint64* res)
{
#if unix == 1
#if sun == 1

#define page_size 1024
  // for Solaris, read /proc/pid/psinfo and get the right fields
  psinfo_t psi;
  char buff[1024];
  int pid = getpid();
  int fd = -1;
  int sz;

  sprintf(buff, "/proc/%d/psinfo", pid);
  fd = open(buff, O_RDONLY);
  if (fd < 0) {
    return -1;
  }
  sz = read(fd, &psi, sizeof(psinfo_t));

  close(fd);
  if (sz != sizeof(psinfo_t)) {
    return -1;
  }

  *size = psi.pr_size * page_size;
  *res = psi.pr_rssize * page_size;

  return 0;

#elif linux == 1
#define page_size 4096
  // for Linux, parse the first two fields from /proc/pid/statm
  char buff[1024];
  int pid = getpid();
  sprintf(buff, "/proc/%d/statm", pid);

  FILE* f = fopen(buff, "r");
  int cnt = fscanf(f, "%llu %llu", size, res);
  fclose(f);

  if (2 != cnt) {
    return -1;
  }

  *size = *size * page_size;
  *res = *res * page_size;

  return 0;

#endif
#endif
  return 0;
}

uint64 max_res = 0;
uint64 max_size = 0;

void AthSignalInstaller(int signo, void (*signal_handler)(int))
{
  struct sigaction act;
  act.sa_handler = signal_handler;
  sigemptyset(&act.sa_mask);
  act.sa_flags = SA_RESTART;
  if (sigaction(signo, &act, NULL) == -1) {
    perror("signal:");
  }
  // printf("Installed signal handler!\n");
}

void AthMaxMem(uint64 size, uint64 res)
{
  if (size > max_size)
    max_size = size;

  if (res > max_res)
    max_res = res;
}
void AthMemCounter(int /* unused: signo */)
{
  uint64 size;
  uint64 res;
  AthGetProcessMem(&size, &res);
  AthMaxMem(size, res);
}

// for scaling mem output
#define MS (1024 * 1024)
#define GS (1024 * 1024 * 1024)
void AthMemCounterp(int /* unused: signo */)
{
  uint64 size;
  uint64 res;
  AthGetProcessMem(&size, &res);
  AthMaxMem(size, res);

  double psize = ((double) size) / MS;
  double pres = ((double) res) / MS;
  if (psize > 1024.0) {
    fprintf(stdout,
            "AthMemCounter (size/res): %.2f GB / %.2f GB\n",
            psize / 1024,
            pres / 1024);
  } else {
    fprintf(
        stdout, "AthMemCounter (size/res): %.2f MB / %.2f MB\n", psize, pres);
  }
}

int AthResourceLog(utl::Logger* logger, const char* title, int ss)
{
  static struct tms ctp;
  static time_t wtp = 0;
  static double mmp = 0.0;

  // mallinfo does not work on 64 bit.
  // tttt_use_mallinfo is set 0.
  // change it to 1 and rebuild before trying mallinfo on 32 bit.

  if (ss == -2)
    return 0;

  uint64 res;
  uint64 size;

  double mmn;
#ifdef OLD_MALLOC
  static struct mallinfo mminfo;
  mminfo = mallinfo();
  mmn = (double) ((uint) mminfo.arena) + (double) ((uint) mminfo.hblkhd);
#else
  if (AthGetProcessMem(&size, &res) != 0) {
    return 0;
  }

  AthMaxMem(size, res);

  mmn = (double) size;
#endif

  if (ss == -1) {
    if (mmn != mmp)
      return 1;
    return 0;
  }

  struct tms ctn;
  time_t wtn, wtd;
  time(&wtn);
  times(&ctn);
  int ticks = sysconf(_SC_CLK_TCK);
  if (wtp) {
    if (title && title[0])
      logger->warn(utl::ODB, 700, "%s:  ", title);

    wtd = wtn - wtp;
    logger->warn(utl::ODB, 701, "ELAPSE %.8s  ", asctime(gmtime(&wtd)) + 11);

    if (ss) {
      logger->warn(utl::ODB, 702,
             "CPU %ld cs  ",
             (ctn.tms_utime + ctn.tms_stime) - (ctp.tms_utime + ctp.tms_stime));
      logger->warn(utl::ODB, 703, "MEM %d B (%d B)", (int) (mmn), (int) (mmn - mmp));
      logger->warn(utl::ODB, 704, " MAX %d M", (int) max_size);
      logger->warn(utl::ODB, 705, "  %s\n", ctime(&wtn));
    } else {
      int imeg = 1024 * 1024;
      logger->warn(utl::ODB, 706,
             "CPU %ld sec  ",
             ((ctn.tms_utime + ctn.tms_stime) - (ctp.tms_utime + ctp.tms_stime))
                 / ticks);
      logger->warn(utl::ODB, 707, "MEM %d M (%d M)", (int) (mmn / imeg), (int) ((mmn - mmp) / imeg));
      logger->warn(utl::ODB, 708, " MAX %d M", (int) ((double) max_size / imeg));
      logger->warn(utl::ODB, 709, "  %s\n", ctime(&wtn));
    }
  }
  wtp = wtn;
  ctp = ctn;
  mmp = mmn;

  // reset maximums after they were printed
  max_size = size;
  max_res = res;

  return 1;
}

int Ath__double2int(double v)
{
  int iv = (int) v;
  if (iv > 0) {
    if (v > (double) iv + 0.5)
      iv++;
  } else if (iv < 0) {
    if ((double) iv > v + 0.5)
      iv--;
  }
  return iv;
}

}  // namespace odb
