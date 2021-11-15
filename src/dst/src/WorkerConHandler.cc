
#include "dst/WorkerConHandler.h"

#include <boost/bind.hpp>
#include <iostream>
#include <thread>
#include "utl/Logger.h"

#include "triton_route/TritonRoute.h"
using namespace boost::asio;
using ip::tcp;
using std::cout;
using std::endl;
namespace dst {
WorkerConHandler::WorkerConHandler(boost::asio::io_service& io_service,
                       odb::dbDatabase* db,
                       utl::Logger* logger)
    : sock(io_service), db_(db), logger_(logger)
{
}
// socket creation
tcp::socket& WorkerConHandler::socket()
{
  return sock;
}

void WorkerConHandler::start()
{
  async_read_until(
      sock,
      in_packet_,
      '\n',
      [me = shared_from_this()](boost::system::error_code const& ec,
                                std::size_t bytes_xfer) {
        std::thread t1(&WorkerConHandler::handle_read, me, ec, bytes_xfer);
        t1.detach();
        // me->handle_read(ec, bytes_xfer);
      });
}

void WorkerConHandler::handle_read(boost::system::error_code const& err,
                             size_t bytes_transferred)
{
  if (!err) {
    std::istream stream(&in_packet_);
    std::string data;
    stream >> data;
    triton_route::TritonRoute* router = new triton_route::TritonRoute();
    router->init(db_, logger_);
    logger_->info(utl::DST, 2, "running worker {} from port {}", data, sock.remote_endpoint().port());
    data = router->runDRWorker(data.c_str());
    logger_->info(utl::DST, 3, "worker {} is done", data);
    boost::system::error_code error;
    boost::asio::write(sock, boost::asio::buffer(data), error);
  } else {
    logger_->warn(utl::DST, 4, "Routing conhandler failed with message: {}", err.message());
  }
  sock.close();
}
}  // namespace dst