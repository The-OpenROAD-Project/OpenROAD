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

#include "WorkerConnection.h"

#include <thread>

#include "dst/Distributed.h"
#include "dst/JobCallBack.h"
#include "dst/JobMessage.h"
#include "utl/Logger.h"

namespace dst {

WorkerConnection::WorkerConnection(asio::io_service& io_service,
                                   Distributed* dist,
                                   utl::Logger* logger)
    : sock(io_service), dist_(dist), logger_(logger)
{
}
// socket creation
tcp::socket& WorkerConnection::socket()
{
  return sock;
}

void WorkerConnection::start()
{
  async_read_until(
      sock,
      in_packet_,
      JobMessage::EOP,
      [me = shared_from_this()](boost::system::error_code const& ec,
                                std::size_t bytes_xfer) {
        std::thread t1(&WorkerConnection::handle_read, me, ec, bytes_xfer);
        t1.detach();
      });
}

void WorkerConnection::handle_read(boost::system::error_code const& err,
                                   size_t bytes_transferred)
{
  if (!err) {
    std::string data{buffers_begin(in_packet_.data()),
                     buffers_begin(in_packet_.data()) + bytes_transferred};
    boost::system::error_code error;
    JobMessage msg(JobMessage::NONE);
    if (!JobMessage::serializeMsg(JobMessage::READ, msg, data)) {
      logger_->warn(utl::DST,
                    41,
                    "Received malformed msg {} from port {}",
                    data,
                    sock.remote_endpoint().port());
      asio::write(sock, asio::buffer("0"), error);
      sock.close();
      return;
    }
    switch (msg.getType()) {
      case JobMessage::ROUTING:
        /* code */
        for (auto& cb : dist_->getCallBacks()) {
          cb->onRoutingJobReceived(msg, sock);
        }
        break;
      default:
        logger_->warn(utl::DST,
                      5,
                      "Unsupported job type {} from port {}",
                      msg.getType(),
                      sock.remote_endpoint().port());
        asio::write(sock, asio::buffer("0"), error);
        sock.close();
        break;
    }
  } else {
    logger_->warn(utl::DST,
                  4,
                  "Worker conhandler failed with message: \"{}\"",
                  err.message());
  }
  sock.close();
}
}  // namespace dst