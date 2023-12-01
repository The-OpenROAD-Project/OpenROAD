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

#include <tcl.h>

#include <boost/asio.hpp>
#include <memory>
#include <string>
#include <vector>

namespace utl {
class Logger;
}

namespace asio = boost::asio;
using asio::ip::tcp;

namespace dst {
using socket = asio::basic_stream_socket<tcp>;
class JobMessage;
class JobCallBack;
class Worker;

class Distributed
{
 public:
  Distributed(utl::Logger* logger = nullptr);
  ~Distributed();
  void init(Tcl_Interp* tcl_interp, utl::Logger* logger);
  void runWorker(const char* ip, unsigned short port, bool interactive);
  void runLoadBalancer(const char* ip,
                       unsigned short port,
                       const char* workers_domain);
  void addWorkerAddress(const char* address, unsigned short port);
  bool sendJob(JobMessage& msg,
               const char* ip,
               unsigned short port,
               JobMessage& result);
  bool sendJobMultiResult(JobMessage& msg,
                          const char* ip,
                          unsigned short port,
                          JobMessage& result);
  bool sendResult(JobMessage& msg, socket& sock);
  void addCallBack(JobCallBack* cb);
  const std::vector<JobCallBack*>& getCallBacks() const { return callbacks_; }

 private:
  struct EndPoint
  {
    std::string ip;
    unsigned short port;
    EndPoint(std::string ip_in, unsigned short port_in)
        : ip(ip_in), port(port_in)
    {
    }
  };
  utl::Logger* logger_;
  std::vector<EndPoint> end_points_;
  std::vector<JobCallBack*> callbacks_;
  std::vector<std::unique_ptr<Worker>> workers_;
};
}  // namespace dst
