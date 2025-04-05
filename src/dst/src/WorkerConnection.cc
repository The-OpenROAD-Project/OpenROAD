// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#include "WorkerConnection.h"

#include <boost/asio/post.hpp>
#include <boost/bind/bind.hpp>
#include <cstddef>
#include <string>

#include "Worker.h"
#include "dst/Distributed.h"
#include "dst/JobCallBack.h"
#include "dst/JobMessage.h"
#include "utl/Logger.h"
namespace dst {

WorkerConnection::WorkerConnection(asio::io_context& service,
                                   Distributed* dist,
                                   utl::Logger* logger,
                                   Worker* worker)
    : sock_(service),
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
