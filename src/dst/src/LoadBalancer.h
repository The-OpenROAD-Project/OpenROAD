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

#pragma once
#include <boost/asio.hpp>
#include <queue>
#include <vector>
#include <mutex>

#include "BalancerConnection.h"

namespace utl {
class Logger;
}

namespace dst {
class LoadBalancer
{
 public:
  // constructor for accepting connection from client
  LoadBalancer(asio::io_service& io_service,
               utl::Logger* logger,
               const char* ip,
               unsigned short port = 1234);
  void addWorker(std::string ip, unsigned short port, unsigned short avail);
  void updateWorker(ip::address ip, unsigned short port);

 private:
  struct worker
  {
    ip::address ip;
    unsigned short port;
    unsigned short priority;
    worker(ip::address ipIn, unsigned short portIn, unsigned short priorityIn)
        : ip(ipIn), port(portIn), priority(priorityIn)
    {
    }
  };
  struct CompareWorker
  {
    bool operator()(worker const& w1, worker const& w2)
    {
      return w1.priority < w2.priority;
    }
  };

  tcp::acceptor acceptor_;
  asio::io_service* service;
  utl::Logger* logger_;
  std::priority_queue<worker, std::vector<worker>, CompareWorker> workers_;
  std::mutex workers_mutex_;

  void start_accept();
  void handle_accept(BalancerConnection::pointer connection,
                     const boost::system::error_code& err);
};
}  // namespace dst