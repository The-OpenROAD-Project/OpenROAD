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
  uint16_t port() { return port_; }

  void SetRegistry(std::shared_ptr<PrometheusRegistry>& new_registry_ptr)
  {
    registry_ptr_ = new_registry_ptr;
  }

 private:
  std::thread worker_thread_;
  std::shared_ptr<PrometheusRegistry> registry_ptr_{nullptr};
  uint16_t port_;
  std::atomic<utl::Logger*> logger_;
  bool shutdown_ = false;
  bool is_ready_ = false;

  void RunServer();
  void WorkerFunction();
};
}  // namespace utl
