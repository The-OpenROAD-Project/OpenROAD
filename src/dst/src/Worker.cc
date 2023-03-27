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

#include "Worker.h"

#include <boost/bind/bind.hpp>
#include <boost/thread/thread.hpp>

#include "utl/Logger.h"

namespace ip = asio::ip;

namespace dst {

void Worker::start_accept()
{
  auto connection
      = boost::make_shared<WorkerConnection>(service_, dist_, logger_, this);
  acceptor_.async_accept(
      connection->socket(),
      boost::bind(
          &Worker::handle_accept, this, connection, asio::placeholders::error));
}

Worker::Worker(Distributed* dist,
               utl::Logger* logger,
               const char* ip,
               unsigned short port)
    : acceptor_(service_, tcp::endpoint(ip::address::from_string(ip), port)),
      dist_(dist),
      logger_(logger)
{
  start_accept();
}

Worker::~Worker()
{
  service_.stop();
}
void Worker::run()
{
  service_.run();
}

void Worker::handle_accept(
    const boost::shared_ptr<WorkerConnection>& connection,
    const boost::system::error_code& err)
{
  if (!err) {
    connection->start();
  }
  start_accept();
}
}  // namespace dst
