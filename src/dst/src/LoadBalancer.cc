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

#include "utl/Logger.h"
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
                           unsigned short port)
    : dist_(dist),
      acceptor_(io_service, tcp::endpoint(ip::address::from_string(ip), port)),
      logger_(logger),
      jobs_(0)
{
  // pool_ = std::make_unique<asio::thread_pool>();
  service = &io_service;
  start_accept();
}

void LoadBalancer::addWorker(std::string ip, unsigned short port)
{
  workers_.push(worker(ip::address::from_string(ip), port, 0));
}
void LoadBalancer::updateWorker(ip::address ip, unsigned short port)
{
  std::lock_guard<std::mutex> lock(workers_mutex_);
  std::priority_queue<worker, std::vector<worker>, CompareWorker> newQueue;
  while (!workers_.empty()) {
    auto worker = workers_.top();
    workers_.pop();
    if (worker.ip == ip && worker.port == port)
      worker.priority--;
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
    if (w.priority != std::numeric_limits<unsigned short>::max())
      w.priority++;
    workers_.push(w);
  }
}

void LoadBalancer::handle_accept(BalancerConnection::pointer connection,
                                 const boost::system::error_code& err)
{
  if (!err)
    connection->start();
  start_accept();
}
}  // namespace dst
