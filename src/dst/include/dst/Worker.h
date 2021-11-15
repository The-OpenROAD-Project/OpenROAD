
#pragma once
#include <boost/asio.hpp>

#include "WorkerConHandler.h"

namespace odb {
class dbDatabase;
}

namespace utl {
  class Logger;
}

using namespace boost::asio;
using namespace ip;
namespace dst {
class Worker
{
 private:
  tcp::acceptor acceptor_;
  io_service* service;
  odb::dbDatabase* db_;
  utl::Logger* logger_;
  void start_accept();
  void handle_accept(WorkerConHandler::pointer connection,
                     const boost::system::error_code& err);

 public:
  // constructor for accepting connection from client
  Worker(boost::asio::io_service& io_service,
         odb::dbDatabase* db,
         utl::Logger* logger,
         unsigned short port = 1234);
};
}  // namespace dst