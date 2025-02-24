// Copyright 2025 Google LLC
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#pragma once

#include <atomic>
#include <chrono>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include "registry.h"
#include "text_serializer.h"

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
