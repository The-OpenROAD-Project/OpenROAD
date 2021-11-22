#include "Worker.h"

#include <boost/bind.hpp>

using namespace boost::asio;
using ip::tcp;
namespace dst {

void Worker::start_accept()
{
  WorkerConHandler::pointer connection = WorkerConHandler::create(*service, db_, logger_);
  acceptor_.async_accept(connection->socket(),
                         boost::bind(&Worker::handle_accept,
                                     this,
                                     connection,
                                     boost::asio::placeholders::error));
}

Worker::Worker(boost::asio::io_service& io_service,
               odb::dbDatabase* db,
               utl::Logger* logger,
               unsigned short port)
    : acceptor_(io_service, tcp::endpoint(tcp::v4(), port)), db_(db), logger_(logger)
{
  service = &io_service;
  start_accept();
}

void Worker::handle_accept(WorkerConHandler::pointer connection,
                           const boost::system::error_code& err)
{
  if (!err) {
    connection->start();
  }
  start_accept();
}
}  // namespace dst
