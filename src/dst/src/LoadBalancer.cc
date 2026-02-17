// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#include "LoadBalancer.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <exception>
#include <limits>
#include <mutex>
#include <queue>
#include <string>
#include <vector>

#include "BalancerConnection.h"
#include "boost/asio.hpp"
#include "boost/bind/bind.hpp"
#include "boost/thread/thread.hpp"
#include "utl/Logger.h"

using boost::asio::ip::udp;

namespace dst {

void LoadBalancer::start_accept()
{
  if (jobs_ != 0 && jobs_ % 100 == 0) {
    logger_->info(utl::DST, 7, "Processed {} jobs", jobs_);
    auto copy = workers_;
    while (!copy.empty()) {
      auto worker = copy.top();
      logger_->report("Worker {}/{} handled {} jobs",
                      worker.ip,
                      worker.port,
                      worker.priority);
      copy.pop();
    }
  }
  jobs_++;
  BalancerConnection::Pointer connection
      = BalancerConnection::create(*service_, this, logger_);
  acceptor_.async_accept(connection->socket(),
                         boost::bind(&LoadBalancer::handle_accept,
                                     this,
                                     connection,
                                     asio::placeholders::error));
}

LoadBalancer::LoadBalancer(Distributed* dist,
                           asio::io_context& service,
                           utl::Logger* logger,
                           const char* ip,
                           const char* workers_domain,
                           unsigned short port)
    : dist_(dist),
      acceptor_(service, tcp::endpoint(ip::make_address(ip), port)),
      logger_(logger),
      jobs_(0)
{
  // pool_ = std::make_unique<asio::thread_pool>();
  service_ = &service;
  start_accept();
  if (std::strcmp(workers_domain, "") != 0) {
    workers_lookup_thread_ = boost::thread(
        boost::bind(&LoadBalancer::lookUpWorkers, this, workers_domain, port));
  }
}

LoadBalancer::~LoadBalancer()
{
  alive_ = false;
  if (workers_lookup_thread_.joinable()) {
    workers_lookup_thread_.join();
  }
}

bool LoadBalancer::addWorker(const std::string& ip, unsigned short port)
{
  std::lock_guard<std::mutex> lock(workers_mutex_);
  bool valid_worker_state = true;
  if (!broadcastData_.empty()) {
    for (auto data : broadcastData_) {
      try {
        asio::io_context service;
        tcp::socket socket(service);
        socket.connect(tcp::endpoint(ip::make_address(ip), port));
        asio::write(socket, asio::buffer(data));
        asio::streambuf receive_buffer;
        asio::read(socket, receive_buffer, asio::transfer_all());
      } catch (std::exception const& ex) {
        if (std::string(ex.what()).find("read: End of file")
            == std::string::npos) {
          // Since asio::transfer_all() used with a stream buffer it
          // always reach an eof file exception!
          valid_worker_state = false;
          break;
        }
      }
    }
  }
  if (valid_worker_state) {
    workers_.emplace(ip::make_address(ip), port, 0);
  }
  return valid_worker_state;
}
void LoadBalancer::updateWorker(const ip::address& ip, unsigned short port)
{
  std::lock_guard<std::mutex> lock(workers_mutex_);
  std::priority_queue<Worker, std::vector<Worker>, CompareWorker> new_queue;
  while (!workers_.empty()) {
    auto worker = workers_.top();
    workers_.pop();
    if (worker.ip == ip && worker.port == port) {
      worker.priority--;
    }
    new_queue.push(worker);
  }
  workers_.swap(new_queue);
}
void LoadBalancer::getNextWorker(ip::address& ip, uint16_t& port)
{
  std::lock_guard<std::mutex> lock(workers_mutex_);
  if (!workers_.empty()) {
    Worker w = workers_.top();
    workers_.pop();
    ip = w.ip;
    port = w.port;
    if (w.priority != std::numeric_limits<uint16_t>::max()) {
      w.priority++;
    }
    workers_.push(w);
  }
}

void LoadBalancer::punishWorker(const ip::address& ip, uint16_t port)
{
  std::lock_guard<std::mutex> lock(workers_mutex_);
  std::priority_queue<Worker, std::vector<Worker>, CompareWorker> new_queue;
  while (!workers_.empty()) {
    auto worker = workers_.top();
    workers_.pop();
    if (worker.ip == ip && worker.port == port) {
      worker.priority = worker.priority == 0 ? 2 : worker.priority * 2;
    }
    new_queue.push(worker);
  }
  workers_.swap(new_queue);
}

void LoadBalancer::removeWorker(const ip::address& ip, uint16_t port, bool lock)
{
  if (lock) {
    workers_mutex_.lock();
  }
  std::priority_queue<Worker, std::vector<Worker>, CompareWorker> new_queue;
  while (!workers_.empty()) {
    auto worker = workers_.top();
    workers_.pop();
    if (worker.ip == ip && worker.port == port) {
      continue;
    }
    new_queue.push(worker);
  }
  workers_.swap(new_queue);
  if (lock) {
    workers_mutex_.unlock();
  }
}

void LoadBalancer::lookUpWorkers(const char* domain, uint16_t port)
{
  asio::io_context ios;
  std::vector<Worker> workers_set;
  udp::resolver resolver(ios);
  while (alive_) {
    std::vector<Worker> new_workers;
    boost::system::error_code ec;
    udp::resolver::results_type results
        = resolver.resolve(domain, std::to_string(port), ec);
    if (ec) {
      logger_->warn(utl::DST,
                    203,
                    "Workers domain resolution failed with error code = {}. "
                    "Message = {}.",
                    ec.value(),
                    ec.message());
    }
    int new_workers_count = 0;
    for (const auto& entry : results) {
      auto discovered_worker = Worker(entry.endpoint().address(), port, 0);
      if (std::ranges::find(workers_set, discovered_worker)
          == workers_set.end()) {
        workers_set.push_back(discovered_worker);
        new_workers.push_back(discovered_worker);
        new_workers_count += 1;
      }
    }

    if (new_workers_count == 0) {
      debugPrint(
          logger_,
          utl::DST,
          "load_balancer",
          1,
          "Discovered 0 new workers with the given domain. Total workers = {}.",
          workers_set.size());
    } else {
      debugPrint(logger_,
                 utl::DST,
                 "load_balancer",
                 1,
                 "Discovered {} new workers with the given domain. Total "
                 "workers = {}.",
                 new_workers_count,
                 workers_set.size());
    }

    for (const auto& worker : new_workers) {
      addWorker(worker.ip.to_string(), worker.port);
    }

    boost::this_thread::sleep(
        boost::posix_time::milliseconds(kWorkersDiscoveryPeriod * 1000));
  }
}

void LoadBalancer::handle_accept(const BalancerConnection::Pointer& connection,
                                 const boost::system::error_code& err)
{
  if (!err) {
    connection->start();
  }
  start_accept();
}
}  // namespace dst

#if !SWIG && FMT_VERSION >= 100000
namespace boost::asio::ip {

static auto format_as(const boost::asio::ip::address& f)
{
  return fmt::streamed(f);
}

}  // namespace boost::asio::ip
#endif
