/* Authors: Osama */
/*
 * Copyright (c) 2021, The Regents of the University of California
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the University nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "LoadBalancer.h"

#include <boost/bind/bind.hpp>
#include <boost/thread/thread.hpp>

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
  BalancerConnection::pointer connection
      = BalancerConnection::create(*service, this, logger_);
  acceptor_.async_accept(connection->socket(),
                         boost::bind(&LoadBalancer::handle_accept,
                                     this,
                                     connection,
                                     asio::placeholders::error));
}

LoadBalancer::LoadBalancer(Distributed* dist,
                           asio::io_service& io_service,
                           utl::Logger* logger,
                           const char* ip,
                           const char* workers_domain,
                           unsigned short port)
    : dist_(dist),
      acceptor_(io_service, tcp::endpoint(ip::address::from_string(ip), port)),
      logger_(logger),
      jobs_(0)
{
  // pool_ = std::make_unique<asio::thread_pool>();
  service = &io_service;
  start_accept();
  if (std::strcmp(workers_domain, "") != 0) {
    workers_lookup_thread = boost::thread(
        boost::bind(&LoadBalancer::lookUpWorkers, this, workers_domain, port));
  }
}

LoadBalancer::~LoadBalancer()
{
  alive = false;
  if (workers_lookup_thread.joinable()) {
    workers_lookup_thread.join();
  }
}

bool LoadBalancer::addWorker(const std::string& ip, unsigned short port)
{
  std::lock_guard<std::mutex> lock(workers_mutex_);
  bool validWorkerState = true;
  if (!broadcastData.empty()) {
    for (auto data : broadcastData) {
      try {
        asio::io_service io_service;
        tcp::socket socket(io_service);
        socket.connect(tcp::endpoint(ip::address::from_string(ip), port));
        asio::write(socket, asio::buffer(data));
        asio::streambuf receive_buffer;
        asio::read(socket, receive_buffer, asio::transfer_all());
      } catch (std::exception const& ex) {
        if (std::string(ex.what()).find("read: End of file")
            == std::string::npos) {
          // Since asio::transfer_all() used with a stream buffer it
          // always reach an eof file exception!
          validWorkerState = false;
          break;
        }
      }
    }
  }
  if (validWorkerState) {
    workers_.push(worker(ip::address::from_string(ip), port, 0));
  }
  return validWorkerState;
}
void LoadBalancer::updateWorker(const ip::address& ip, unsigned short port)
{
  std::lock_guard<std::mutex> lock(workers_mutex_);
  std::priority_queue<worker, std::vector<worker>, CompareWorker> newQueue;
  while (!workers_.empty()) {
    auto worker = workers_.top();
    workers_.pop();
    if (worker.ip == ip && worker.port == port) {
      worker.priority--;
    }
    newQueue.push(worker);
  }
  workers_.swap(newQueue);
}
void LoadBalancer::getNextWorker(ip::address& ip, unsigned short& port)
{
  std::lock_guard<std::mutex> lock(workers_mutex_);
  if (!workers_.empty()) {
    worker w = workers_.top();
    workers_.pop();
    ip = w.ip;
    port = w.port;
    if (w.priority != std::numeric_limits<unsigned short>::max()) {
      w.priority++;
    }
    workers_.push(w);
  }
}

void LoadBalancer::punishWorker(const ip::address& ip, unsigned short port)
{
  std::lock_guard<std::mutex> lock(workers_mutex_);
  std::priority_queue<worker, std::vector<worker>, CompareWorker> newQueue;
  while (!workers_.empty()) {
    auto worker = workers_.top();
    workers_.pop();
    if (worker.ip == ip && worker.port == port) {
      worker.priority = worker.priority == 0 ? 2 : worker.priority * 2;
    }
    newQueue.push(worker);
  }
  workers_.swap(newQueue);
}

void LoadBalancer::removeWorker(const ip::address& ip,
                                unsigned short port,
                                bool lock)
{
  if (lock) {
    workers_mutex_.lock();
  }
  std::priority_queue<worker, std::vector<worker>, CompareWorker> newQueue;
  while (!workers_.empty()) {
    auto worker = workers_.top();
    workers_.pop();
    if (worker.ip == ip && worker.port == port) {
      continue;
    }
    newQueue.push(worker);
  }
  workers_.swap(newQueue);
  if (lock) {
    workers_mutex_.unlock();
  }
}

void LoadBalancer::lookUpWorkers(const char* domain, unsigned short port)
{
  asio::io_service ios;
  std::vector<worker> workers_set;
  udp::resolver::query resolver_query(
      domain, std::to_string(port), udp::resolver::query::numeric_service);
  udp::resolver resolver(ios);
  while (alive) {
    std::vector<worker> new_workers;
    boost::system::error_code ec;
    auto it = resolver.resolve(resolver_query, ec);
    if (ec) {
      logger_->warn(utl::DST,
                    203,
                    "Workers domain resolution failed with error code = {}. "
                    "Message = {}.",
                    ec.value(),
                    ec.message());
    }
    int new_workers_count = 0;
    udp::resolver::iterator it_end;
    for (; it != it_end; ++it) {
      auto discovered_worker = worker(it->endpoint().address(), port, 0);
      if (std::find(workers_set.begin(), workers_set.end(), discovered_worker)
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
        boost::posix_time::milliseconds(workers_discovery_period * 1000));
  }
}

void LoadBalancer::handle_accept(const BalancerConnection::pointer& connection,
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
