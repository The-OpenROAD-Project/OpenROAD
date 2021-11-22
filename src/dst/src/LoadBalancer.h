
#pragma once
#include <boost/asio.hpp>
#include <queue>
#include <vector>
#include "BalancerConHandler.h"

namespace triton_route {
class TritonRoute;
}

namespace utl {
  class Logger;
}

using namespace boost::asio;
using namespace ip;
namespace dst {
class LoadBalancer
{
 private:
  struct worker
  {
    address ip_;
    unsigned short port_;
    unsigned short priority_;
    worker(address ip, unsigned short port, unsigned short priority) : ip_(ip), port_(port), priority_(priority) {}
  };
  struct CompareWorker
  {
    bool operator()(worker const& w1, worker const& w2)
    {
      return w1.priority_ < w2.priority_;
    }
  };
  
  tcp::acceptor acceptor_;
  io_service* service;
  utl::Logger* logger_;
  std::priority_queue<worker, std::vector<worker>, CompareWorker> workers_;
  boost::asio::detail::mutex workers_mutex_;

  void start_accept();
  void handle_accept(BalancerConHandler::pointer connection,
                     const boost::system::error_code& err);

 public:
  // constructor for accepting connection from client
  LoadBalancer(boost::asio::io_service& io_service,
               utl::Logger* logger,
               unsigned short port = 1234);
  void addWorker(std::string ip, unsigned short port, unsigned short avail);
  void updateWorker(address ip, unsigned short port);
};
}  // namespace dst