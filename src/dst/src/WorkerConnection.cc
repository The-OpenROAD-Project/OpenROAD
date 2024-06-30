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

#include <boost/asio/post.hpp>
#include <boost/bind/bind.hpp>

#include "Worker.h"
#include "dst/Distributed.h"
#include "dst/JobCallBack.h"
#include "dst/JobMessage.h"
#include "utl/Logger.h"
namespace dst {

WorkerConnection::WorkerConnection(asio::io_service& io_service,
                                   Distributed* dist,
                                   utl::Logger* logger,
                                   Worker* worker)
    : sock_(io_service),
      dist_(dist),
      logger_(logger),
      msg_(JobMessage::NONE),
      worker_(worker)
{
}
// socket creation
tcp::socket& WorkerConnection::socket()
{
  return sock_;
}

void WorkerConnection::start()
{
  async_read_until(
      sock_,
      in_packet_,
      JobMessage::EOP,
      [me = shared_from_this()](boost::system::error_code const& ec,
                                std::size_t bytes_xfer) {
        me->handle_read(ec, bytes_xfer);
      });
}

void WorkerConnection::handle_read(boost::system::error_code const& err,
                                   size_t bytes_transferred)
{
  if (!err) {
    std::string data{buffers_begin(in_packet_.data()),
                     buffers_begin(in_packet_.data()) + bytes_transferred};
    boost::system::error_code error;
    if (!JobMessage::serializeMsg(JobMessage::READ, msg_, data)) {
      logger_->warn(utl::DST,
                    41,
                    "Received malformed msg {} from port {}",
                    data,
                    sock_.remote_endpoint().port());
      asio::write(sock_, asio::buffer("0"), error);
      sock_.close();
      return;
    }
    switch (msg_.getJobType()) {
      case JobMessage::ROUTING:
        for (auto& cb : dist_->getCallBacks()) {
          cb->onRoutingJobReceived(msg_, sock_);
        }
        break;
      case JobMessage::UPDATE_DESIGN: {
        for (auto& cb : dist_->getCallBacks()) {
          cb->onFrDesignUpdated(msg_, sock_);
        }
        break;
      }
      case JobMessage::PIN_ACCESS: {
        for (auto& cb : dist_->getCallBacks()) {
          cb->onPinAccessJobReceived(msg_, sock_);
        }
        break;
      }
      case JobMessage::GRDR_INIT: {
        for (auto& cb : dist_->getCallBacks()) {
          cb->onGRDRInitJobReceived(msg_, sock_);
        }
        break;
      }
      default:
        logger_->warn(utl::DST,
                      5,
                      "Unsupported job type {} from port {}",
                      (int) msg_.getJobType(),
                      sock_.remote_endpoint().port());
        asio::write(sock_, asio::buffer("0"), error);
        sock_.close();
        return;
    }
  } else {
    logger_->warn(utl::DST,
                  4,
                  "Worker conhandler failed with message: \"{}\"",
                  err.message());
    sock_.close();
  }
}
}  // namespace dst
