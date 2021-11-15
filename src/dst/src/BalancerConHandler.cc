
#include "dst/BalancerConHandler.h"
#include "dst/LoadBalancer.h"

#include <boost/bind.hpp>
#include <iostream>
#include <thread>
#include "utl/Logger.h"

using namespace boost::asio;
using ip::tcp;
using std::cout;
using std::endl;
namespace dst {
BalancerConHandler::BalancerConHandler(boost::asio::io_service& io_service,
                       LoadBalancer* owner,
                       utl::Logger* logger)
    : sock(io_service), owner_(owner), logger_(logger)
{
}
// socket creation
tcp::socket& BalancerConHandler::socket()
{
  return sock;
}

void BalancerConHandler::start(ip::address workerAddress, unsigned short port)
{
  async_read_until(
      sock,
      in_packet_,
      '\n',
      [me = shared_from_this(), workerAddress, port](boost::system::error_code const& ec,
                                std::size_t bytes_xfer) {
        me->handle_read(ec, bytes_xfer,workerAddress, port);
      });
}

void BalancerConHandler::handle_read(boost::system::error_code const& err,
                             size_t bytes_transferred,
                             ip::address workerAddress, 
                             unsigned short port)
{
  if (!err) {
    boost::system::error_code error;
    if(workerAddress.is_unspecified())
      logger_->warn(utl::DST, 9, "No workers available");
    else {
      logger_->info(utl::DST, 10, "Sending to {}/{}", workerAddress.to_string(), port);
      boost::asio::io_service io_service;
      tcp::socket socket(io_service);
      socket.connect( tcp::endpoint( workerAddress, port ));
      boost::asio::write(socket, in_packet_, error);
      boost::asio::streambuf receive_buffer;
      boost::asio::read(socket, receive_buffer, boost::asio::transfer_all(), error);
      boost::asio::write(sock, receive_buffer, error);
    }
    
  } else {
    logger_->warn(utl::DST, 11, "Balancer conhandler failed with message: {}", err.message());
  }
  owner_->updateWorker(workerAddress, port);
  sock.close();
}
}  // namespace dst