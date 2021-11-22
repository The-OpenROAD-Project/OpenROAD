#include "LoadBalancer.h"

#include <boost/bind.hpp>

using namespace boost::asio;
using ip::tcp;
namespace dst {

void LoadBalancer::start_accept()
{
  BalancerConHandler::pointer connection = BalancerConHandler::create(*service, this, logger_);
  acceptor_.async_accept(connection->socket(),
                         boost::bind(&LoadBalancer::handle_accept,
                                     this,
                                     connection,
                                     boost::asio::placeholders::error));
}

LoadBalancer::LoadBalancer(boost::asio::io_service& io_service,
               utl::Logger* logger,
               unsigned short port)
    : acceptor_(io_service, tcp::endpoint(tcp::v4(), port)), logger_(logger)
{
  service = &io_service;
  start_accept();
}

void LoadBalancer::addWorker(std::string ip, unsigned short port, unsigned short avail)
{
  workers_.push(worker(address::from_string(ip), port, avail));
}
void LoadBalancer::updateWorker(address ip, unsigned short port)
{
  // std::unique_lock lock(workers_mutex_);
  workers_mutex_.lock();
  std::priority_queue<worker, std::vector<worker>, CompareWorker> newQueue;
  while (!workers_.empty())
  {
    auto worker = workers_.top();
    workers_.pop();
    if(worker.ip_ == ip && worker.port_ == port)
      worker.priority_++;
    newQueue.push(worker);
  }
  workers_.swap(newQueue);
  workers_mutex_.unlock();
}
void LoadBalancer::handle_accept(BalancerConHandler::pointer connection,
                                 const boost::system::error_code& err)
{
  if (!err) {
    // std::unique_lock lock(workers_mutex_);
    workers_mutex_.lock();
    address workerAddress;
    unsigned short port;
    if(!workers_.empty())
    {
      worker w = workers_.top();
      workers_.pop();
      workerAddress = w.ip_;
      port = w.port_;
      w.priority_--;
      workers_.push(w);
    }
    workers_mutex_.unlock();
    connection->start(workerAddress, port);
  }
  start_accept();
}
}  // namespace dst
