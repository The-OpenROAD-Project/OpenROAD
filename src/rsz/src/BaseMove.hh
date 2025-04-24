#include <algorithm>
#include <cmath>
#include <cstddef>
#include <memory>
#include <sstream>
#include <string>
#include <chrono>

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "dpl/Opendp.h"
#include "sta/FuncExpr.hh"
#include "sta/MinMax.hh"
#include "sta/PathExpanded.hh"
#include "sta/Path.hh"
#include "sta/StaState.hh"
#include "utl/Logger.h"


namespace rsz {

class Resizer; 

using dpl::Opendp;

using odb::dbMaster;
using odb::Point;

using utl::Logger;

using sta::Cell;
using sta::Corner;
using sta::dbDatabase;
using sta::dbNetwork;
using sta::dbSta;
using sta::DcalcAnalysisPt;
using sta::Instance;
using sta::LibertyCell;
using sta::LibertyPort;
using sta::MinMax;
using sta::Net;
using sta::Network;
using sta::PathExpanded;
using sta::Path;
using sta::Pin;
using sta::RiseFall;
using sta::Slack;
using sta::Slew;
using sta::StaState;
using sta::TimingArc;
using sta::Vertex;


class BaseMove : public sta::dbStaState {
    public:
        BaseMove(Resizer* resizer);
        virtual ~BaseMove() = default;

        virtual void init() = 0;

        virtual bool doMove(const Path* drvr_path,
                           const int drvr_index,
                           PathExpanded* expanded) { 
            return doMove(drvr_path, drvr_index, 0.0, expanded); 
        }
        virtual bool doMove(const Path* drvr_path,
                           const int drvr_index,
                           const Slack drvr_slack,
                           PathExpanded* expanded) {
            return doMove(drvr_path, drvr_index, expanded);
        }

        // Analysis functions
        virtual double deltaSlack() = 0;
        virtual double deltaPower() = 0;
        virtual double deltaArea() = 0;

        // Journaling functions
        virtual void undoMove(int num=0) = 0;
        // Each move may take different parameters so don't make it a virtual
        //virtual void journalMove() = 0;
        virtual void clear() = 0;
        int count() const { return count_; }

    protected:
       Resizer* resizer_;
       Logger* logger_;
       Network* network_;
       dbNetwork* db_network_;
       dbSta* sta_;
       dbDatabase* db_ = nullptr;
       int dbu_ = 0; 
       dpl::Opendp* opendp_ = nullptr;

       int count_ = 0;

       double area(Cell* cell);
       double area(dbMaster* master);
       double dbuToMeters(int dist) const;

};

}

