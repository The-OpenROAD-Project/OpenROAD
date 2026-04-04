// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "CpuThrottle.h"

#ifdef __linux__
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#endif

#include <string>
#include <thread>

#include "utl/Logger.h"

namespace ord {

CpuThreadThrottle::CpuThreadThrottle(int num_threads, utl::Logger* logger)
    : logger_(logger)
{
#ifndef __linux__
  // flock-based throttling is Linux-only; no-op on other platforms.
  return;
#endif
  const int total_cpus = std::thread::hardware_concurrency();
  if (total_cpus == 0) {
    logger_->warn(
        utl::ORD, 40, "Throttle: cannot detect CPU count, skipping throttle.");
    return;
  }

  if (num_threads > total_cpus) {
    num_threads = total_cpus;
  }

  ensureDirectory();

  logger_->info(utl::ORD,
                41,
                "Throttle: requesting {} CPU slot(s) out of {} total.",
                num_threads,
                total_cpus);

  bool logged_waiting = false;
  while (!tryAcquire(num_threads, total_cpus)) {
    if (!logged_waiting) {
      logger_->info(
          utl::ORD,
          42,
          "Throttle: waiting for {} CPU slot(s) to become available...",
          num_threads);
      logged_waiting = true;
    }
    usleep(100000);  // 100ms
  }

  logger_->info(
      utl::ORD, 43, "Throttle: acquired {} CPU slot(s).", num_threads);
}

CpuThreadThrottle::~CpuThreadThrottle()
{
  for (int fd : held_fds_) {
    close(fd);
  }
  held_fds_.clear();
  if (coordinator_fd_ >= 0) {
    close(coordinator_fd_);
    coordinator_fd_ = -1;
  }
}

CpuThreadThrottle::CpuThreadThrottle(CpuThreadThrottle&& other) noexcept
    : held_fds_(std::move(other.held_fds_)),
      coordinator_fd_(other.coordinator_fd_),
      logger_(other.logger_)
{
  other.coordinator_fd_ = -1;
  other.logger_ = nullptr;
}

CpuThreadThrottle& CpuThreadThrottle::operator=(
    CpuThreadThrottle&& other) noexcept
{
  if (this != &other) {
    for (int fd : held_fds_) {
      close(fd);
    }
    if (coordinator_fd_ >= 0) {
      close(coordinator_fd_);
    }
    held_fds_ = std::move(other.held_fds_);
    coordinator_fd_ = other.coordinator_fd_;
    logger_ = other.logger_;
    other.coordinator_fd_ = -1;
    other.logger_ = nullptr;
  }
  return *this;
}

#ifdef __linux__

void CpuThreadThrottle::ensureDirectory()
{
  mkdir(kSemDir, 0777);
  // Ignore EEXIST — directory may already exist from another process.
}

bool CpuThreadThrottle::tryAcquire(int num_threads, int total_cpus)
{
  // Open and lock the coordinator to serialize allocation attempts.
  // This prevents deadlock: only one process tries to grab CPU slots at a time.
  int coord_fd = open(kCoordinatorLock, O_CREAT | O_RDWR, 0666);
  if (coord_fd < 0) {
    logger_->warn(utl::ORD,
                  44,
                  "Throttle: cannot open coordinator lock: {}",
                  std::strerror(errno));
    return false;
  }
  if (flock(coord_fd, LOCK_EX) != 0) {
    close(coord_fd);
    return false;
  }

  std::vector<int> acquired;
  acquired.reserve(num_threads);

  for (int i = 0;
       i < total_cpus && static_cast<int>(acquired.size()) < num_threads;
       ++i) {
    std::string path = std::string(kSemDir) + "/cpu." + std::to_string(i);
    int fd = open(path.c_str(), O_CREAT | O_RDWR, 0666);
    if (fd < 0) {
      continue;
    }
    if (flock(fd, LOCK_EX | LOCK_NB) == 0) {
      acquired.push_back(fd);
    } else {
      close(fd);
    }
  }

  if (static_cast<int>(acquired.size()) == num_threads) {
    // Success — release coordinator but keep CPU locks held.
    flock(coord_fd, LOCK_UN);
    close(coord_fd);
    held_fds_ = std::move(acquired);
    return true;
  }

  // Not enough slots — release everything and retry later.
  for (int fd : acquired) {
    close(fd);
  }
  flock(coord_fd, LOCK_UN);
  close(coord_fd);
  return false;
}

#endif  // __linux__

}  // namespace ord
