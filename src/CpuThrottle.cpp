// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "ord/CpuThrottle.h"

#ifdef ENABLE_THROTTLE

#ifdef __linux__
#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#endif

#include <algorithm>
#include <chrono>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "utl/Logger.h"

namespace ord {

CpuThreadThrottle::CpuThreadThrottle(utl::Logger* logger) : logger_(logger)
{
#ifndef __linux__
  return;
#else
  total_cpus_ = std::thread::hardware_concurrency();
  if (total_cpus_ == 0) {
    logger_->warn(
        utl::ORD, 40, "Throttle: cannot detect CPU count, skipping throttle.");
    return;
  }

  ensureDirectory();

  // Acquire 1 slot for the main thread.
  resize(1);
#endif
}

CpuThreadThrottle::~CpuThreadThrottle()
{
  if (contention_count_ > 0 && logger_) {
    logger_->info(utl::ORD,
                  82,
                  "Throttle: summary - {} resizes, {} contentions, {}ms "
                  "total wait.",
                  resize_count_,
                  contention_count_,
                  total_wait_ms_);
  }
  for (int fd : held_fds_) {
    close(fd);
  }
  held_fds_.clear();
}

CpuThreadThrottle::CpuThreadThrottle(CpuThreadThrottle&& other) noexcept
    : held_fds_(std::move(other.held_fds_)),
      total_cpus_(other.total_cpus_),
      logger_(other.logger_),
      total_wait_ms_(other.total_wait_ms_),
      resize_count_(other.resize_count_),
      contention_count_(other.contention_count_)
{
  other.total_cpus_ = 0;
  other.logger_ = nullptr;
  other.contention_count_ = 0;
}

CpuThreadThrottle& CpuThreadThrottle::operator=(
    CpuThreadThrottle&& other) noexcept
{
  if (this != &other) {
    for (int fd : held_fds_) {
      close(fd);
    }
    held_fds_ = std::move(other.held_fds_);
    total_cpus_ = other.total_cpus_;
    logger_ = other.logger_;
    total_wait_ms_ = other.total_wait_ms_;
    resize_count_ = other.resize_count_;
    contention_count_ = other.contention_count_;
    other.total_cpus_ = 0;
    other.logger_ = nullptr;
    other.contention_count_ = 0;
  }
  return *this;
}

void CpuThreadThrottle::resize(int num_threads)
{
#ifndef __linux__
  return;
#else
  if (total_cpus_ == 0) {
    return;
  }

  num_threads = std::max(1, std::min(num_threads, total_cpus_));
  const int current = heldCount();

  if (num_threads == current) {
    return;
  }

  ++resize_count_;

  if (num_threads < current) {
    // Release excess slots from the back.
    while (heldCount() > num_threads) {
      close(held_fds_.back());
      held_fds_.pop_back();
    }
    logger_->info(utl::ORD,
                  78,
                  "Throttle: resized to {} CPU slot(s) (released {}).",
                  heldCount(),
                  current - heldCount());
    return;
  }

  // Need more slots.
  const int additional = num_threads - current;
  logger_->info(utl::ORD,
                58,
                "Throttle: requesting {} additional CPU slot(s) "
                "(currently holding {}, target {}).",
                additional,
                current,
                num_threads);

  bool logged_waiting = false;
  int retries = 0;
  while (!tryAcquireAdditional(additional)) {
    if (!logged_waiting) {
      logger_->info(
          utl::ORD,
          77,
          "Throttle: waiting for {} CPU slot(s) to become available...",
          additional);
      logged_waiting = true;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(kRetryDelayMs));
    if (++retries >= kMaxRetries) {
      logger_->warn(utl::ORD,
                    81,
                    "Throttle: timed out after {}s waiting for CPU slots. "
                    "Proceeding with {} slot(s).",
                    kMaxRetries * kRetryDelayMs / 1000,
                    heldCount());
      return;
    }
  }

  if (logged_waiting) {
    const int wait_ms = retries * kRetryDelayMs;
    total_wait_ms_ += wait_ms;
    ++contention_count_;
    logger_->info(utl::ORD,
                  64,
                  "Throttle: acquired {} slot(s) after waiting {}ms.",
                  heldCount(),
                  wait_ms);
  } else {
    logger_->info(
        utl::ORD, 79, "Throttle: now holding {} CPU slot(s).", heldCount());
  }
#endif
}

#ifdef __linux__

void CpuThreadThrottle::ensureDirectory()
{
  if (mkdir(kSemDir, 0777) == 0) {
    // Ensure world-writable regardless of umask.
    chmod(kSemDir, 0777);
  }
  // Ignore EEXIST — directory may already exist from another process.
}

bool CpuThreadThrottle::tryAcquireAdditional(int additional)
{
  // Open and lock the coordinator to serialize allocation attempts.
  int coord_fd = open(kCoordinatorLock, O_CREAT | O_RDONLY | O_CLOEXEC, 0666);
  if (coord_fd < 0) {
    if (errno == EACCES || errno == ENOSPC || errno == EROFS) {
      logger_->error(utl::ORD,
                     80,
                     "Throttle: cannot open coordinator lock: {}",
                     std::strerror(errno));
    }
    return false;
  }
  if (flock(coord_fd, LOCK_EX) != 0) {
    close(coord_fd);
    return false;
  }

  std::vector<int> acquired;
  acquired.reserve(additional);

  for (int i = 0;
       i < total_cpus_ && static_cast<int>(acquired.size()) < additional;
       ++i) {
    std::string path = std::string(kSemDir) + "/cpu." + std::to_string(i);
    int fd = open(path.c_str(), O_CREAT | O_RDONLY | O_CLOEXEC, 0666);
    if (fd < 0) {
      continue;
    }
    if (flock(fd, LOCK_EX | LOCK_NB) == 0) {
      acquired.push_back(fd);
    } else {
      close(fd);
    }
  }

  // Release coordinator lock.
  flock(coord_fd, LOCK_UN);
  close(coord_fd);

  if (static_cast<int>(acquired.size()) == additional) {
    // Success — append newly acquired fds to held set.
    held_fds_.insert(held_fds_.end(), acquired.begin(), acquired.end());
    return true;
  }

  // Not enough slots — release everything we just acquired.
  for (int fd : acquired) {
    close(fd);
  }
  return false;
}

#endif  // __linux__

// --- CpuThreadGuard ---

CpuThreadGuard::CpuThreadGuard(CpuThreadThrottle* throttle, int num_threads)
    : throttle_(throttle)
{
  if (throttle_) {
    throttle_->resize(num_threads);
  }
}

CpuThreadGuard::~CpuThreadGuard()
{
  if (throttle_) {
    throttle_->resize(1);
  }
}

CpuThreadGuard::CpuThreadGuard(CpuThreadGuard&& other) noexcept
    : throttle_(other.throttle_)
{
  other.throttle_ = nullptr;
}

CpuThreadGuard& CpuThreadGuard::operator=(CpuThreadGuard&& other) noexcept
{
  if (this != &other) {
    if (throttle_) {
      throttle_->resize(1);
    }
    throttle_ = other.throttle_;
    other.throttle_ = nullptr;
  }
  return *this;
}

// --- Global throttle ---

static CpuThreadThrottle* g_cpu_throttle = nullptr;
static int g_target_threads = 1;

CpuThreadThrottle* globalCpuThrottle()
{
  return g_cpu_throttle;
}

void setGlobalCpuThrottle(CpuThreadThrottle* throttle)
{
  g_cpu_throttle = throttle;
}

void setGlobalCpuTargetThreads(int threads)
{
  g_target_threads = threads;
}

int globalCpuTargetThreads()
{
  return g_target_threads;
}

CpuThreadGuard acquireGlobalCpuThreads()
{
  return CpuThreadGuard(g_cpu_throttle, g_target_threads);
}

CpuThreadGuard acquireGlobalCpuThreads(int num_threads)
{
  return CpuThreadGuard(g_cpu_throttle, num_threads);
}

}  // namespace ord

#else  // !ENABLE_THROTTLE

namespace ord {

// No-op implementations when throttle is disabled (CMake builds).

CpuThreadThrottle::CpuThreadThrottle(utl::Logger* /*logger*/)
{
}
CpuThreadThrottle::~CpuThreadThrottle() = default;
CpuThreadThrottle::CpuThreadThrottle(CpuThreadThrottle&&) noexcept = default;
CpuThreadThrottle& CpuThreadThrottle::operator=(CpuThreadThrottle&&) noexcept
    = default;
void CpuThreadThrottle::resize(int /*num_threads*/)
{
}
void CpuThreadThrottle::ensureDirectory()
{
}
bool CpuThreadThrottle::tryAcquireAdditional(int /*additional*/)
{
  return false;
}

CpuThreadGuard::CpuThreadGuard(CpuThreadThrottle* /*throttle*/,
                               int /*num_threads*/)
{
}
CpuThreadGuard::~CpuThreadGuard() = default;
CpuThreadGuard::CpuThreadGuard(CpuThreadGuard&&) noexcept = default;
CpuThreadGuard& CpuThreadGuard::operator=(CpuThreadGuard&&) noexcept = default;

CpuThreadThrottle* globalCpuThrottle()
{
  return nullptr;
}
void setGlobalCpuThrottle(CpuThreadThrottle* /*throttle*/)
{
}
void setGlobalCpuTargetThreads(int /*threads*/)
{
}
int globalCpuTargetThreads()
{
  return 1;
}
CpuThreadGuard acquireGlobalCpuThreads()
{
  return CpuThreadGuard(nullptr, 0);
}
CpuThreadGuard acquireGlobalCpuThreads(int /*num_threads*/)
{
  return CpuThreadGuard(nullptr, 0);
}

}  // namespace ord

#endif  // ENABLE_THROTTLE
