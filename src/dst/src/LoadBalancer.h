// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <queue>
#include <string>
#include <vector>

#include "BalancerConnection.h"
#include "absl/synchronization/mutex.h"
#include "boost/asio.hpp"
#include "boost/asio/ip/address.hpp"
#include "boost/asio/thread_pool.hpp"
#include "boost/thread/thread.hpp"

namespace utl {
class Logger;
}

namespace dst {

namespace ip = asio::ip;

// time in seconds between retrying to find new workers on the network
inline constexpr int kWorkersDiscoveryPeriod = 15;

class Distributed;
class LoadBalancer
{
 public:
  // constructor for accepting connection from client
  LoadBalancer(Distributed* dist,
               asio::io_context& service,
               utl::Logger* logger,
               const char* ip,
               const char* workers_domain,
               uint16_t port = 1234);
  ~LoadBalancer();
  bool addWorker(const std::string& ip, uint16_t port);
  void updateWorker(const ip::address& ip, uint16_t port);
  void getNextWorker(ip::address& ip, uint16_t& port);
  void removeWorker(const ip::address& ip, uint16_t port, bool lock = true);
  void punishWorker(const ip::address& ip, uint16_t port);

 private:
  struct Worker
  {
    ip::address ip;
    uint16_t port;
    uint16_t priority;
    Worker(const ip::address& ip, uint16_t port, uint16_t priority)
        : ip(ip), port(port), priority(priority)
    {
    }
    bool operator==(const Worker& rhs) const
    {
      return (ip == rhs.ip && port == rhs.port && priority == rhs.priority);
    }
  };
  struct CompareWorker
  {
    bool operator()(Worker const& w1, Worker const& w2)
    {
      return w1.priority > w2.priority;
    }
  };

  Distributed* dist_;
  tcp::acceptor acceptor_;
  asio::io_context* service_;
  utl::Logger* logger_;
  std::priority_queue<Worker, std::vector<Worker>, CompareWorker> workers_;
  absl::Mutex workers_mutex_;
  std::unique_ptr<asio::thread_pool> pool_;
  absl::Mutex pool_mutex_;
  uint32_t jobs_;
  std::atomic<bool> alive_ = true;
  boost::thread workers_lookup_thread_;
  std::vector<std::string> broadcastData_;

  void start_accept();
  void handle_accept(const BalancerConnection::Pointer& connection,
                     const boost::system::error_code& err);
  void lookUpWorkers(const char* domain, uint16_t port);
  friend class dst::BalancerConnection;
};
}  // namespace dst
