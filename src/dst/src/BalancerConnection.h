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
#include <boost/enable_shared_from_this.hpp>
#include <boost/make_shared.hpp>

namespace asio = boost::asio;
namespace ip = asio::ip;
using asio::ip::tcp;

namespace utl {
class Logger;
}
namespace dst {
class LoadBalancer;

class BalancerConnection
    : public boost::enable_shared_from_this<BalancerConnection>
{
 public:
  using pointer = boost::shared_ptr<BalancerConnection>;
  BalancerConnection(asio::io_service& io_service,
                     LoadBalancer* owner,
                     utl::Logger* logger);
  static pointer create(asio::io_service& io_service,
                        LoadBalancer* owner,
                        utl::Logger* logger)
  {
    return boost::make_shared<BalancerConnection>(io_service, owner, logger);
  }
  tcp::socket& socket();
  void start();
  void handle_read(boost::system::error_code const& err,
                   size_t bytes_transferred);
  LoadBalancer* getOwner() const { return owner_; }

 private:
  tcp::socket sock_;
  asio::streambuf in_packet_;
  utl::Logger* logger_;
  LoadBalancer* owner_;
  const int MAX_FAILED_WORKERS_TRIALS = 3;
  const int MAX_BROADCAST_FAILED_NODES = 2;
};
}  // namespace dst
