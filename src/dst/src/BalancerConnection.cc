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

#include <boost/asio/post.hpp>
#include <boost/bind/bind.hpp>
#include <thread>

#include "LoadBalancer.h"
#include "utl/Logger.h"

namespace dst {
BalancerConnection::BalancerConnection(asio::io_service& io_service,
                                       LoadBalancer* owner,
                                       utl::Logger* logger)
    : sock_(io_service), logger_(logger), owner_(owner)
{
}
// socket creation
tcp::socket& BalancerConnection::socket()
{
  return sock_;
}

void BalancerConnection::start()
{
  async_read_until(
      sock_,
      in_packet_,
      JobMessage::EOP,
      [me = shared_from_this()](boost::system::error_code const& ec,
                                std::size_t bytes_xfer) {
        std::unique_lock<std::mutex> lock(me->getOwner()->pool_mutex_);
        asio::post(
            *me->getOwner()->pool_.get(),
            boost::bind(&BalancerConnection::handle_read, me, ec, bytes_xfer));
      });
}

void BalancerConnection::handle_read(boost::system::error_code const& err,
                                     size_t bytes_transferred)
{
  if (!err) {
    boost::system::error_code error;
    std::string data{buffers_begin(in_packet_.data()),
                     buffers_begin(in_packet_.data()) + bytes_transferred};
    JobMessage msg(JobMessage::NONE);
    if (!JobMessage::serializeMsg(JobMessage::READ, msg, data)) {
      logger_->warn(utl::DST,
                    42,
                    "Received malformed msg {} from port {}",
                    data,
                    sock_.remote_endpoint().port());
      asio::write(sock_, asio::buffer("0"), error);
      sock_.close();
      return;
    }
    switch (msg.getMessageType()) {
      case JobMessage::UNICAST: {
        ip::address workerAddress;
        unsigned short port;
        owner_->getNextWorker(workerAddress, port);
        if (workerAddress.is_unspecified()) {
          logger_->warn(utl::DST, 6, "No workers available");
          sock_.close();
        } else {
          asio::io_service io_service;
          tcp::socket socket(io_service);
          socket.connect(tcp::endpoint(workerAddress, port));
          asio::write(socket, in_packet_, error);
          asio::streambuf receive_buffer;
          asio::read(socket, receive_buffer, asio::transfer_all(), error);
          asio::write(sock_, receive_buffer, error);
          owner_->updateWorker(workerAddress, port);
          sock_.close();
        }
        break;
      }
      case JobMessage::BROADCAST: {
        std::lock_guard<std::mutex> lock(owner_->workers_mutex_);
        asio::thread_pool pool(owner_->workers_.size());
        auto workers_copy = owner_->workers_;
        while (!workers_copy.empty()) {
          auto worker = workers_copy.top();
          workers_copy.pop();
          asio::post(pool, [worker, data]() {
            boost::system::error_code error;
            asio::io_service io_service;
            tcp::socket socket(io_service);
            socket.connect(tcp::endpoint(worker.ip, worker.port));
            asio::write(socket, asio::buffer(data), error);
            asio::streambuf receive_buffer;
            asio::read(socket, receive_buffer, asio::transfer_all(), error);
          });
        }
        pool.join();
        JobMessage result(JobMessage::NONE);
        std::string msgStr;
        JobMessage::serializeMsg(JobMessage::WRITE, result, msgStr);
        asio::write(sock_, asio::buffer(msgStr), error);
        sock_.close();
        break;
      }
    }

  } else {
    logger_->warn(utl::DST,
                  8,
                  "Balancer conhandler failed with message: {}",
                  err.message());
    sock_.close();
  }
}
}  // namespace dst
