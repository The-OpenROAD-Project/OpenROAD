#pragma once
#include <boost/asio.hpp>
#include <boost/enable_shared_from_this.hpp>

using namespace boost::asio;
using ip::tcp;
namespace utl {
  class Logger;
}
namespace triton_route {
class TritonRoute;
}
namespace dst {
class LoadBalancer;

class BalancerConHandler : public boost::enable_shared_from_this<BalancerConHandler>
{
 private:
  tcp::socket sock;
  streambuf in_packet_;
  utl::Logger* logger_;
  LoadBalancer* owner_;
 public:
  typedef boost::shared_ptr<BalancerConHandler> pointer;
  BalancerConHandler(boost::asio::io_service& io_service,
             LoadBalancer* owner,
             utl::Logger* logger);
  static pointer create(boost::asio::io_service& io_service,
                        LoadBalancer* owner,
                        utl::Logger* logger)
  {
    return pointer(new BalancerConHandler(io_service, owner, logger));
  }
  tcp::socket& socket();
  void start(ip::address workerAddress, unsigned short port);
  void handle_read(boost::system::error_code const& err,
                   size_t bytes_transferred,
                   ip::address workerAddress, 
                   unsigned short port);
};
}  // namespace dst
