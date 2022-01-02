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

#include "BalancerConnection.h"

#include <dst/JobMessage.h>

#include <thread>

#include "LoadBalancer.h"
#include "utl/Logger.h"

namespace dst {
BalancerConnection::BalancerConnection(asio::io_service& io_service,
                                       LoadBalancer* owner,
                                       utl::Logger* logger)
  : sock(io_service), logger_(logger), owner_(owner)
{
}
// socket creation
tcp::socket& BalancerConnection::socket()
{
  return sock;
}

void BalancerConnection::start(ip::address workerAddress, unsigned short port)
{
  async_read_until(
      sock,
      in_packet_,
      JobMessage::EOP,
      [me = shared_from_this(), workerAddress, port](
          boost::system::error_code const& ec, std::size_t bytes_xfer) {
        me->handle_read(ec, bytes_xfer, workerAddress, port);
      });
}

void BalancerConnection::handle_read(boost::system::error_code const& err,
                                     size_t bytes_transferred,
                                     ip::address workerAddress,
                                     unsigned short port)
{
  if (!err) {
    boost::system::error_code error;
    if (workerAddress.is_unspecified())
      logger_->warn(utl::DST, 6, "No workers available");
    else {
      logger_->info(
          utl::DST, 7, "Sending to {}/{}", workerAddress.to_string(), port);
      asio::io_service io_service;
      tcp::socket socket(io_service);
      socket.connect(tcp::endpoint(workerAddress, port));
      asio::write(socket, in_packet_, error);
      asio::streambuf receive_buffer;
      asio::read(socket, receive_buffer, asio::transfer_all(), error);
      asio::write(sock, receive_buffer, error);
    }

  } else {
    logger_->warn(utl::DST,
                  8,
                  "Balancer conhandler failed with message: {}",
                  err.message());
  }
  owner_->updateWorker(workerAddress, port);
  sock.close();
}
}  // namespace dst
