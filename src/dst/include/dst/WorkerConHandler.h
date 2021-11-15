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
namespace odb {
class dbDatabase;
}
namespace dst {
class WorkerConHandler : public boost::enable_shared_from_this<WorkerConHandler>
{
 private:
  tcp::socket sock;
  odb::dbDatabase* db_;
  streambuf in_packet_;
  utl::Logger* logger_;
 public:
  typedef boost::shared_ptr<WorkerConHandler> pointer;
  WorkerConHandler(boost::asio::io_service& io_service,
             odb::dbDatabase* db,
             utl::Logger* logger);
  static pointer create(boost::asio::io_service& io_service,
                        odb::dbDatabase* db,
                        utl::Logger* logger)
  {
    return pointer(new WorkerConHandler(io_service, db, logger));
  }
  tcp::socket& socket();
  void start();
  void handle_read(boost::system::error_code const& err,
                   size_t bytes_transferred);
};
}  // namespace dst
