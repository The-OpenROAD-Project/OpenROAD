// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#include "dst/Distributed.h"

#include <cstddef>
#include <cstring>
#include <exception>
#include <memory>
#include <string>
#include <utility>

#include "LoadBalancer.h"
#include "Worker.h"
#include "boost/asio.hpp"
#include "boost/asio/ip/tcp.hpp"
#include "boost/bind/bind.hpp"
#include "boost/system/system_error.hpp"
#include "boost/thread/thread.hpp"
#include "dst/JobCallBack.h"
#include "dst/JobMessage.h"
#include "utl/Logger.h"

namespace dst {

constexpr int kMaxTries = 5;

Distributed::Distributed(utl::Logger* logger) : logger_(logger)
{
}

Distributed::~Distributed()
{
  for (auto cb : callbacks_) {
    delete cb;
  }
  callbacks_.clear();
}

void Distributed::runWorker(const char* ip,
                            unsigned short port,
                            bool interactive)
{
  try {
    auto u_worker = std::make_unique<Worker>(this, logger_, ip, port);
    auto worker = u_worker.get();
    workers_.push_back(std::move(u_worker));
    if (interactive) {
      boost::thread t(boost::bind(&Worker::run, worker));
      t.detach();
    } else {
      worker->run();
    }
  } catch (std::exception& e) {
    logger_->error(utl::DST, 1, "Worker server error: {}", e.what());
  }
}

void Distributed::runLoadBalancer(const char* ip,
                                  unsigned short port,
                                  const char* workers_domain)
{
  try {
    asio::io_context service;
    LoadBalancer balancer(this, service, logger_, ip, workers_domain, port);
    if (std::strcmp(workers_domain, "") == 0) {
      for (const auto& worker : end_points_) {
        balancer.addWorker(worker.ip, worker.port);
      }
    }
    service.run();
  } catch (std::exception& e) {
    logger_->error(utl::DST, 9, "LoadBalancer error: {}", e.what());
  }
}

void Distributed::addWorkerAddress(const char* address, unsigned short port)
{
  end_points_.emplace_back(address, port);
}
// TODO: exponential backoff
bool sendMsg(dst::Socket& sock, const std::string& msg, std::string& error_msg)
{
  int tries = 0;
  while (tries++ < kMaxTries) {
    boost::system::error_code error;
    sock.wait(asio::ip::tcp::socket::wait_write);
    write(sock, asio::buffer(msg), error);
    if (!error) {
      error_msg.clear();
      return true;
    }
    error_msg = error.message();
  }
  return false;
}

bool readMsg(dst::Socket& sock, std::string& data_str)
{
  boost::system::error_code error;
  asio::streambuf receive_buffer;
  sock.wait(asio::ip::tcp::socket::wait_read);
  asio::read(sock, receive_buffer, asio::transfer_all(), error);
  if (error && error != asio::error::eof) {
    data_str = error.message();
    return false;
  }

  auto bufs = receive_buffer.data();
  auto offset = asio::buffers_begin(bufs) + receive_buffer.size();
  std::string result;
  if (offset <= asio::buffers_end(bufs)) {
    result = std::string(asio::buffers_begin(bufs), offset);
  }

  data_str = result;
  return !data_str.empty();
}

bool Distributed::sendJob(JobMessage& msg,
                          const char* ip,
                          unsigned short port,
                          JobMessage& result)
{
  int tries = 0;
  std::string msg_str;
  if (!JobMessage::serializeMsg(JobMessage::kWrite, msg, msg_str)) {
    logger_->warn(utl::DST, 112, "Serializing JobMessage failed");
    return false;
  }
  std::string result_str;
  while (tries++ < kMaxTries) {
    asio::io_context service;
    dst::Socket sock(service);
    try {
      sock.connect(tcp::endpoint(ip::make_address(ip), port));
    } catch (const boost::system::system_error& ex) {
      logger_->warn(utl::DST,
                    113,
                    "Trial {}, socket connection failed with message \"{}\"",
                    tries,
                    ex.what());
      continue;
    }
    bool ok = sendMsg(sock, msg_str, result_str);
    if (!ok) {
      continue;
    }
    ok = readMsg(sock, result_str);
    if (!ok) {
      continue;
    }
    if (!JobMessage::serializeMsg(JobMessage::kRead, result, result_str)) {
      continue;
    }
    if (sock.is_open()) {
      sock.close();
    }
    return true;
  }
  if (result_str.empty()) {
    result_str = "MAX_TRIES reached";
  }
  logger_->warn(
      utl::DST, 114, "Sending job failed with message \"{}\"", result_str);
  return false;
}

inline bool getNextMsg(std::string& haystack,
                       const std::string& needle,
                       std::string& result)
{
  std::size_t found = haystack.find(needle);
  if (found != std::string::npos) {
    result = haystack.substr(0, found + needle.size());
    haystack.erase(0, found + needle.size());
    return true;
  }
  return false;
}
bool Distributed::sendJobMultiResult(JobMessage& msg,
                                     const char* ip,
                                     unsigned short port,
                                     JobMessage& result)
{
  int tries = 0;
  std::string msg_str;
  if (!JobMessage::serializeMsg(JobMessage::kWrite, msg, msg_str)) {
    logger_->warn(utl::DST, 12, "Serializing JobMessage failed");
    return false;
  }
  std::string result_str;
  while (tries++ < kMaxTries) {
    asio::io_context service;
    dst::Socket sock(service);
    try {
      sock.connect(tcp::endpoint(ip::make_address(ip), port));
    } catch (const boost::system::system_error& ex) {
      logger_->warn(utl::DST,
                    13,
                    "Socket connection failed with message \"{}\"",
                    ex.what());
      return false;
    }
    boost::asio::ip::tcp::no_delay option(true);
    sock.set_option(option);
    bool ok = sendMsg(sock, msg_str, result_str);
    if (!ok) {
      continue;
    }
    ok = readMsg(sock, result_str);
    if (!ok) {
      continue;
    }
    std::string split;
    while (getNextMsg(result_str, JobMessage::kEop, split)) {
      JobMessage tmp;
      if (!JobMessage::serializeMsg(JobMessage::kRead, tmp, split)) {
        logger_->error(utl::DST, 9999, "Problem in deserialize {}", split);
      } else {
        result.addJobDescription(std::move(tmp.getJobDescriptionRef()));
      }
    }
    result.setJobType(JobMessage::kSuccess);
    if (sock.is_open()) {
      sock.close();
    }
    return true;
  }
  if (result_str.empty()) {
    result_str = "MAX_TRIES reached";
  }
  logger_->warn(
      utl::DST, 14, "Sending job failed with message \"{}\"", result_str);
  return false;
}

bool Distributed::sendResult(JobMessage& msg, dst::Socket& sock)
{
  std::string msg_str;
  if (!JobMessage::serializeMsg(JobMessage::kWrite, msg, msg_str)) {
    logger_->warn(utl::DST, 20, "Serializing result JobMessage failed");
    return false;
  }
  int tries = 0;
  std::string error;
  while (tries++ < kMaxTries) {
    if (sendMsg(sock, msg_str, error)) {
      return true;
    }
  }
  logger_->warn(
      utl::DST, 22, "Sending result failed with message \"{}\"", error);
  return false;
}

void Distributed::addCallBack(JobCallBack* cb)
{
  callbacks_.push_back(cb);
}

}  // namespace dst
