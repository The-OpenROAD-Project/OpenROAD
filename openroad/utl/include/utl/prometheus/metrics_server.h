// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include "utl/prometheus/registry.h"
#include "utl/prometheus/text_serializer.h"

namespace utl {
class Logger;
}

namespace utl {
class PrometheusMetricsServer
{
 public:
  PrometheusMetricsServer(std::shared_ptr<PrometheusRegistry>& registry_,
                          utl::Logger* logger,
                          uint16_t port)
  {
    SetRegistry(registry_);
    port_ = port;
    logger_ = logger;
    worker_thread_
        = std::thread(&PrometheusMetricsServer::WorkerFunction, this);
  }
  ~PrometheusMetricsServer();

  bool is_ready() { return is_ready_; }
  bool has_startup_failed() { return startup_failed_; }
  uint16_t port() { return port_; }

  void SetRegistry(std::shared_ptr<PrometheusRegistry>& new_registry_ptr)
  {
    registry_ptr_ = new_registry_ptr;
  }

 private:
  std::thread worker_thread_;
  std::shared_ptr<PrometheusRegistry> registry_ptr_{nullptr};
  // The worker thread publishes port_ (the OS-chosen port when the caller
  // asked for 0), is_ready_ and startup_failed_; the owning thread reads them
  // through the accessors above and in the destructor. shutdown_ travels the
  // other way, plus the worker sets it on an unrecoverable startup failure.
  // The default sequentially consistent ordering is what makes port_ visible
  // to a reader that has observed is_ready_.
  std::atomic<uint16_t> port_;
  std::atomic<utl::Logger*> logger_;
  std::atomic<bool> shutdown_ = false;
  std::atomic<bool> is_ready_ = false;
  std::atomic<bool> startup_failed_ = false;

  void RunServer();
  void WorkerFunction();
};
}  // namespace utl
