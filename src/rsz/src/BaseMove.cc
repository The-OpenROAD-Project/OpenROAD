#include "BaseMove.hh"

#include "rsz/Resizer.hh"

namespace rsz {

BaseMove::BaseMove(Resizer* resizer)
{
   resizer_ = resizer;
   logger_ = resizer_->logger_;
   network_ = resizer_->network_;
   db_ = resizer_->db_;
   db_network_ = resizer_->db_network_;
   dbStaState::init(resizer_->sta_);
   sta_ = resizer_->sta_;
   dbu_ = resizer_->dbu_;
   opendp_ = resizer_->opendp_;


}

double 
BaseMove::area(Cell* cell) 
{ 
    return area(db_network_->staToDb(cell)); 
}

double 
BaseMove::area(dbMaster* master)
{
  if (!master->isCoreAutoPlaceable()) {
    return 0;
  }
  return dbuToMeters(master->getWidth()) * dbuToMeters(master->getHeight());
}

double 
BaseMove::dbuToMeters(int dist) const
{
  return dist / (dbu_ * 1e+6);
}

// namespace rsz
}

