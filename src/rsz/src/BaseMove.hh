// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#pragma once

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
#include "sta/Liberty.hh"
#include "sta/PathExpanded.hh"
#include "sta/Path.hh"
#include "sta/PortDirection.hh"
#include "sta/StaMain.hh"
#include "sta/StaState.hh"
#include "sta/TimingRole.hh"
#include "utl/Logger.h"


namespace rsz {

class Resizer; 

using std::max;

using dpl::Opendp;

using odb::dbMaster;
using odb::Point;

using utl::RSZ;
using utl::Logger;

using sta::ArcDcalcResult;
using sta::ArcDelayCalc;
using sta::ArcDelay;
using sta::Cell;
using sta::Corner;
using sta::dbDatabase;
using sta::dbNetwork;
using sta::dbSta;
using sta::DcalcAnalysisPt;
using sta::Instance;
using sta::InstanceSet;
using sta::INF;
using sta::LoadPinIndexMap;
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
using sta::TimingArcSet;
using sta::TimingRole;
using sta::Vertex;

using InputSlews = std::array<Slew, RiseFall::index_count>;
using TgtSlews = std::array<Slew, RiseFall::index_count>;

class BaseMove : public sta::dbStaState {
    public:
        BaseMove(Resizer* resizer);
        virtual ~BaseMove() = default;

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

        void init();

        // Analysis functions
        //virtual double deltaSlack() = 0;
        //virtual double deltaPower() = 0;
        //virtual douele deltaArea() = 0;

        // Accept the pending optimizations
        void commitMoves();
        // Abandon the pending optimizations
        void restoreMoves();
        // Total pending optimizations (since last checkpoint)
        int pendingMoves() const;
        // Total optimizations 
        int committedMoves() const;
        // Whether this optimization is pending
        int countMoves(Instance* inst) const;
        // Total accepted and pending optimizations 
        int countMoves() const;
        // Add a new pending optimization
        void addMove(Instance* inst);

    protected:
       Resizer* resizer_;
       Logger* logger_;
       Network* network_;
       dbNetwork* db_network_;
       dbSta* sta_;
       dbDatabase* db_ = nullptr;
       int dbu_ = 0; 
       dpl::Opendp* opendp_ = nullptr;

       // Need to track these so we don't optimize the optimzations.
       // This can result in long run-time.
       // These are all of the optimized insts of this type .
       // Some may not have been accepted, but this replicates the prior behavior.
       InstanceSet all_inst_set_;
       int count_ = 0;
       int all_count_ = 0;

       // Use actual input slews for accurate delay/slew estimation
       sta::UnorderedMap<LibertyPort*, InputSlews> input_slew_map_;
       TgtSlews tgt_slews_;

       double area(Cell* cell);
       double area(dbMaster* master);
       double dbuToMeters(int dist) const;


      void gateDelays(const LibertyPort* drvr_port,
                      float load_cap,
                      const DcalcAnalysisPt* dcalc_ap,
                      // Return values.
                      ArcDelay delays[RiseFall::index_count],
                      Slew slews[RiseFall::index_count]);
      void gateDelays(const LibertyPort* drvr_port,
                      float load_cap,
                      const Slew in_slews[RiseFall::index_count],
                      const DcalcAnalysisPt* dcalc_ap,
                      // Return values.
                      ArcDelay delays[RiseFall::index_count],
                      Slew out_slews[RiseFall::index_count]);
      ArcDelay gateDelay(const LibertyPort* drvr_port,
                         float load_cap,
                         const DcalcAnalysisPt* dcalc_ap);
      ArcDelay gateDelay(const LibertyPort* drvr_port,
                         const RiseFall* rf,
                         float load_cap,
                         const DcalcAnalysisPt* dcalc_ap);

      bool isPortEqiv(sta::FuncExpr* expr,
                      const LibertyCell* cell,
                      const LibertyPort* port_a,
                      const LibertyPort* port_b);

      bool simulateExpr(
          sta::FuncExpr* expr,
          sta::UnorderedMap<const LibertyPort*, std::vector<bool>>& port_stimulus,
          size_t table_index);
      std::vector<bool> simulateExpr(
          sta::FuncExpr* expr,
          sta::UnorderedMap<const LibertyPort*, std::vector<bool>>& port_stimulus);

};

}

