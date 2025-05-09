
//
// Routines for building abc structures from a cut
// Class Cut2Ntk
//

#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <sta/Corner.hh>
#include <sta/PathRef.hh>
#include <sta/VertexVisitor.hh>

#include "base/abc/abc.h"
#include "base/main/abcapis.h"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "map/mio/mio.h"
#include "map/mio/mioInt.h"
#include "odb/db.h"
#include "ord/OpenRoad.hh"
#include "rmp/FuncExpr2Kit.h"
#include "rmp/blif.h"
#include "sta/ConcreteNetwork.hh"
#include "sta/Graph.hh"
#include "sta/Liberty.hh"
#include "sta/Network.hh"
#include "sta/PathEnd.hh"
#include "sta/PathExpanded.hh"
#include "sta/PathRef.hh"
#include "sta/PatternMatch.hh"
#include "sta/PortDirection.hh"
#include "sta/Sdc.hh"
#include "sta/Search.hh"
#include "sta/Sta.hh"
#include "utl/Logger.h"

#include "rmp/Restructure.h"
#include <base/io/ioAbc.h>
#include <base/main/main.h>
#include <base/main/mainInt.h>
#include <map/amap/amapInt.h>

using utl::RMP;
using namespace abc;

namespace rmp {

Abc_Ntk_t* Cut2Ntk::BuildSopLogicNetlist()
{
  // First check our cut is sane !
  cut_->Check(sta_nwk_);

  // Now build the abc view of the cut
  abc_nwk_ = (Abc_NtkAlloc(ABC_NTK_NETLIST, ABC_FUNC_SOP, 1));
  Abc_NtkSetName(abc_nwk_, Extra_UtilStrsav(cut_name_.c_str()));

  //Step 1: Build the primary outputs of abc netlist
  for (auto po : cut_->roots_) {
    Abc_Obj_t* abc_po = Abc_NtkCreatePo(abc_nwk_);
    po_table_[po] = abc_po;
    Abc_ObjAssignName(abc_po, (char*) (sta_nwk_->pathName(po)), NULL);
    Abc_Obj_t* abc_op_net = Abc_NtkCreateNet(abc_nwk_);
    Abc_ObjAssignName(abc_op_net, (char*) (sta_nwk_->pathName(po)), NULL);
    Abc_ObjAddFanin(abc_po, abc_op_net);
    net_table_sta2abc_[po] = abc_op_net;
    net_table_abc2sta_[abc_op_net] = po;
  }

  //Step 2:Build the primary inputs to abc netlist
  for (auto pi : cut_->leaves_) {
    Abc_Obj_t* abc_pi = Abc_NtkCreatePi(abc_nwk_);
    Abc_ObjAssignName(abc_pi, (char*) (sta_nwk_->pathName(pi)), NULL);
    pi_table_[pi] = abc_pi;
    Abc_Obj_t* abc_ip_net = Abc_NtkCreateNet(abc_nwk_);

    Abc_ObjAssignName(abc_ip_net, (char*) (sta_nwk_->pathName(pi)), NULL);
    Abc_ObjAddFanin(abc_ip_net, abc_pi);

    net_table_sta2abc_[pi] = abc_ip_net;
    net_table_abc2sta_[abc_ip_net] = pi;
  }
  //Step 3: create instances
  // create the instances for each gate in the cut
  // as a side effect make the gate input nets and output
  // nets. This means for every pin we have a net (which simplifies
  // the logic) but does lead to net -> net connections.

  for (auto cur_inst : cut_->volume_) {
    Abc_Obj_t* cur_gate = CreateGate(cur_inst);
  }

  //Step 4: wiring up gate inputs
  // Wire Up all the inputs for each gate
  for (auto cur_inst : cut_->volume_) {
    sta::InstancePinIterator* pin_iter = sta_nwk_->pinIterator(cur_inst);
    while (pin_iter->hasNext()) {
      const sta::Pin* ip_pin = pin_iter->next();
      if (sta_nwk_->direction(ip_pin)->isInput()) {
        // note we created the input net during CreateGate.
        Abc_Obj_t* ip_net = net_table_sta2abc_[ip_pin];
        sta::PinSet* drvrs = sta_nwk_->drivers(ip_pin);
        if (drvrs) {
          sta::PinSet::Iterator drvr_iter(drvrs);
          const sta::Pin* driving_pin = drvr_iter.next();
          Abc_Obj_t* driver = net_table_sta2abc_[driving_pin];
          // in case when ip is driven by PI we have already
          // stashed that pi driver with the input pin
          // so the ip_net and driver will be the same
          // so skip that case.
          if (!driver) {
            logger_->warn(RMP,
                          100,
                          "Cannot find net for driving pin {} for pin {} on "
                          "instance {}. Candidate escape node\n",
                          sta_nwk_->pathName(driving_pin),
                          sta_nwk_->pathName(ip_pin),
                          sta_nwk_->pathName(cur_inst));
          }
          if (ip_net != driver) {
            Abc_ObjAddFanin(ip_net, driver);
          }
        }
      }
    }
  }
  //Step 5: wire up the primary outputs
  // Wire Up all the primary outputs (roots);
  // We may have wired up the root when creating the gates
  // in which case the abc object for the root will have
  // a fanin count > 0.
  for (auto root_pin : cut_->roots_) {
    Abc_Obj_t* abc_sink_net = net_table_sta2abc_[root_pin];
    // make sure not already wired up
    if (Abc_ObjFaninNum(abc_sink_net) == 0) {
      sta::PinSet* drvrs = sta_nwk_->drivers(root_pin);
      if (drvrs) {
        sta::PinSet::Iterator drvr_iter(drvrs);
        const sta::Pin* driving_pin = drvr_iter.next();
        Abc_Obj_t* abc_driver = net_table_sta2abc_[driving_pin];
        assert(abc_driver);
        Abc_ObjAddFanin(abc_sink_net /* thing driven */,
                        abc_driver /* fanin */);
      }
    }
  }
  //Step 6: Net clean up
  //
  // remove the net -> net connections
  // These are created from our over zealous net creation scheme
  //
  CleanUpNetNets();
  Abc_NtkFinalizeRead(abc_nwk_);
  Abc_Frame_t* gf = Abc_FrameGetGlobalFrame();
  gf->pNtkCur = abc_nwk_;
  // Check the network using the abc network checker
  assert(Abc_NtkCheck(abc_nwk_));
  // code to inspect the netlist
  Abc_NtkSetName(abc_nwk_, Extra_UtilStrsav(cut_name_.c_str()));
  // SopNwkCheck(abc_nwk_);

  Abc_Ntk_t* ret = Abc_NtkDup(abc_nwk_);
  Abc_NtkTrasferNames(abc_nwk_, ret);
  return ret;
}

Abc_Obj_t* Cut2Ntk::CreateGate(const sta::Instance* cur_inst)
{
  Abc_Obj_t* ret = nullptr;
  static int debug;

  debug++;
  // make the input nets.
  sta::InstancePinIterator* pin_iter = sta_nwk_->pinIterator(cur_inst);
  std::vector<Abc_Obj_t*>
      ip_nets;  // the vector of driving nets indexed by pin id.
  while (pin_iter->hasNext()) {
    const sta::Pin* cur_pin = pin_iter->next();
    if (sta_nwk_->direction(cur_pin)->isInput()) {
      // Check, have we already created a net for this pin driver?
      // If so use it, else create a new one
      sta::PinSet* drvrs = sta_nwk_->drivers(cur_pin);
      if (drvrs) {
        sta::PinSet::Iterator drvr_iter(drvrs);
        const sta::Pin* driving_pin = drvr_iter.next();
        if (net_table_sta2abc_.find(driving_pin) != net_table_sta2abc_.end()) {
          Abc_Obj_t* driver = net_table_sta2abc_[driving_pin];
          ip_nets.push_back(driver);
          net_table_sta2abc_[cur_pin] = driver;
        } else {
          Abc_Obj_t* ip_net = Abc_NtkCreateNet(abc_nwk_);
          Abc_ObjAssignName(
              ip_net, (char*) (sta_nwk_->pathName(cur_pin)), NULL);
          ip_nets.push_back(ip_net);
          net_table_sta2abc_[cur_pin] = ip_net;
          net_table_abc2sta_[ip_net] = cur_pin;
        }
      }
    }
  }

  pin_iter = sta_nwk_->pinIterator(cur_inst);
  while (pin_iter->hasNext()) {
    const sta::Pin* op_pin = pin_iter->next();

    if (sta_nwk_->direction(op_pin)->isOutput()) {
      std::vector<Abc_Obj_t*> product_terms;
      sta::LibertyPort* lp = sta_nwk_->libertyPort(op_pin);
      if (lp && lp->function()) {
        // Get the kit representation
        int var_count = 0;
        /*word32*/ unsigned* kit_tables;
        sta::FuncExpr2Kit(lp->function(), var_count, kit_tables);
        unsigned num_bits_to_set = (0x01 << var_count);
        unsigned mask = 0x00;
        for (unsigned j = 0; j < num_bits_to_set; j++)
          mask = mask | (0x1 << j);
        unsigned kit_result = *kit_tables & mask;
        // Make an SOP gate in abc
        char* fn = Abc_SopCreateFromTruth(
            static_cast<Mem_Flex_t_*>(abc_nwk_->pManFunc),
            var_count,
            kit_tables);
        ret = Abc_NtkCreateNode(abc_nwk_);
        Abc_ObjSetData(ret, fn);
        // wire up the inputs
        for (int i = 0; i < var_count; i++)
          Abc_ObjAddFanin(ret, ip_nets[i]);

        // wire up the output
        if (net_table_sta2abc_.find(op_pin) != net_table_sta2abc_.end()) {
          debug++;
          Abc_Obj_t* op_wire = net_table_sta2abc_[op_pin];
          Abc_ObjAddFanin(op_wire, ret);
        } else {
          // make net and wire it in
          Abc_Obj_t* sop_op_net = Abc_NtkCreateNet(abc_nwk_);
          Abc_ObjAssignName(
              sop_op_net, (char*) (sta_nwk_->pathName(op_pin)), NULL);
          Abc_ObjAddFanin(sop_op_net, ret);
          net_table_sta2abc_[op_pin] = sop_op_net;
          debug++;
          net_table_abc2sta_[sop_op_net] = op_pin;
        }
      }
    }
  }
  return ret;
}

void Cut2Ntk::CleanUpNetNets()
{
  bool done_something = true;
  while (done_something) {
    done_something = false;
    std::vector<Abc_Obj_t*> orphans;
    for (auto pin_net : net_table_abc2sta_) {
      Abc_Obj_t* cur_net = pin_net.first;
      if (!cur_net)
        continue;
      if (Abc_ObjFaninVec(cur_net)->nSize == 0)
        continue;
      Abc_Obj_t* fanin_obj = Abc_ObjFanin0(cur_net);
      if (net_table_abc2sta_.find(fanin_obj) != net_table_abc2sta_.end()) {
        Abc_Obj_t* fanin_fanin_obj = Abc_ObjFanin0(fanin_obj);
        // delete the cur net
        // fanin_fanin_obj -> fanin_obj -> cur_net -> {FO}
        //-->
        // fanin_fanin_obj -> fanin_obj -> {FO}
        //
        Abc_ObjDeleteFanin(cur_net, fanin_obj);
        Abc_ObjTransferFanout(cur_net, fanin_obj);
        // at this point cur_net is totally disconnected
        orphans.push_back(cur_net);
        done_something = true;
      }
    }
    // kill the orphans
    for (auto n : orphans) {
      const sta::Pin* cur_pin = net_table_abc2sta_[n];
      net_table_abc2sta_.erase(n);
      net_table_sta2abc_.erase(cur_pin);
      Abc_NtkDeleteObj(n);
    }
  }
}
}
