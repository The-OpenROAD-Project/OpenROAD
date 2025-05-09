
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

using namespace abc;
namespace rmp {


Ntk2Cut::Ntk2Cut(Abc_Ntk_t* abc_nwk, Cut* cut, sta::Network* sta_nwk)
    : mapped_netlist_(abc_nwk), cut_(cut), target_nwk_(sta_nwk)
{
}

/*
  Make the sta network for the elements of the cut
*/

void Ntk2Cut::BuildNwkElements()
{
  std::map<Abc_Obj_t*, sta::Net*> abc2net;
  std::map<sta::Net*, Abc_Obj_t_*> net2abc;
  std::map<Abc_Obj_t*, sta::Instance*> abc2inst;
  std::map<sta::Instance*, Abc_Obj_t*> inst2abc;
  std::map<const sta::Pin*, sta::Net*> cut_ip_nets;

  Abc_Obj_t* pObj = nullptr;
  int i;
  static int net_id = 0;
  static int inst_id = 0;

  
  sta::Instance* parent = target_nwk_->topInstance();

  //Step 1: table of inputs <-> abc object
  // Create a net for each primary input
  for (i = 0; i < Abc_NtkPiNum(mapped_netlist_); i++) {
    pObj = Abc_NtkPi(mapped_netlist_, i);
    sta::Pin* cut_ip_pin = const_cast<sta::Pin*>(cut_->leaves_[i]);
    sta::Net* ip_net = target_nwk_->net(cut_ip_pin);
    if (!ip_net) {
      ip_net = target_nwk_->findNet(target_nwk_->pathName(cut_ip_pin));
      if (!ip_net)
        printf("Error cannot find ip net for leaf %s\n",
               target_nwk_->pathName(cut_ip_pin));
    }
    cut_ip_nets[cut_ip_pin] = ip_net;
    abc2net[pObj] = ip_net;
    net2abc[ip_net] = pObj;
    net_id++;
  }

  //Step 2:Create the instances and make a net for each instance output (single output
  // objects only)
  i = 0;
  Abc_NtkForEachNode(mapped_netlist_, pObj, i)
  {
    inst_id++;
    if (pObj->Type == ABC_OBJ_PI || pObj->Type == ABC_OBJ_PO) {
      continue;
    } else {
      Mio_Gate_t* mapped_gate = (Mio_Gate_t*) (pObj->pData);
      std::string op_port_name = mapped_gate->pOutName;
      char* lib_gate_name = Mio_GateReadName(mapped_gate);
      std::string inst_name
          = std::string(lib_gate_name) + "_" + std::to_string(inst_id);
      while (target_nwk_->findInstance(inst_name.c_str())) {
        inst_id++;
        inst_name = std::string(lib_gate_name) + "_" + std::to_string(inst_id);
      }
      sta::LibertyCell* liberty_cell
          = target_nwk_->findLibertyCell(lib_gate_name);
      sta::Instance* cur_inst
          = ((sta::ConcreteNetwork*) target_nwk_)
                ->makeInstance(liberty_cell, inst_name.c_str(), parent);
      ((sta::ConcreteNetwork*) target_nwk_)->makePins(cur_inst);
      sta::LibertyPort* liberty_op_port
          = liberty_cell->findLibertyPort(op_port_name.c_str());

      inst2abc[cur_inst] = pObj;
      abc2inst[pObj] = cur_inst;
      std::string op_net_name = inst_name + "_" + std::to_string(net_id);
      net_id++;
      sta::Net* op_net = ((sta::ConcreteNetwork*) target_nwk_)
                             ->makeNet(op_net_name.c_str(), parent);
      abc2net[pObj] = op_net;
      net2abc[op_net] = pObj;
      ((sta::ConcreteNetwork*) target_nwk_)
          ->connect(cur_inst, liberty_op_port, op_net);
    }
  }

  //Step 3: Wiring up the inputs for each gate
  i = 0;
  Abc_NtkForEachNode(mapped_netlist_, pObj, i)
  {
    if (pObj->Type == ABC_OBJ_PI || pObj->Type == ABC_OBJ_PO) {
      continue;
    } else {
      // Wire up the inputs of the instances
      Mio_Gate_t* mapped_gate = (Mio_Gate_t*) (pObj->pData);
      char* lib_gate_name = Mio_GateReadName(mapped_gate);
      sta::LibertyCell* liberty_cell = ((sta::ConcreteNetwork*) target_nwk_)
                                           ->findLibertyCell(lib_gate_name);
      sta::Instance* cur_inst = abc2inst[pObj];
      // get the inputs
      int fanin_count = Abc_ObjFaninNum(pObj);
      for (int ip_ix = 0; ip_ix < fanin_count; ip_ix++) {
        Abc_Obj_t* driver = Abc_ObjFanin(pObj, ip_ix);
        sta::Net* driving_net = abc2net[driver];
        int count = 0;
        while (!driving_net && count < 10) {
          driver = Abc_ObjFanin(driver, 0);
          if (!driver) {
            printf("Cannot find fanin driver for abc object %d\n", pObj);
          }
          driving_net = abc2net[driver];
          if (!driving_net) {
            printf("Cannot find net for abc obj %d\n", driver->Id);
          }
          count++;
        }
        if (!driving_net) {
          printf(
              "Cannot find input %d driver for abc obj %d of type %d which "
              "isdriven by abc obj %d\n",
              ip_ix,
              pObj->Id,
              pObj->Type,
              driver->Id);
          continue;
        }
        assert(driving_net);
        char* dest_name = Mio_GateReadPinName(mapped_gate, ip_ix);
        sta::Pin* dest_pin = ((sta::ConcreteNetwork*) target_nwk_)
                                 ->findPin(cur_inst, dest_name);
        // connect driving pin to this pin
        if (!((sta::ConcreteNetwork*) target_nwk_)
                 ->isConnected(driving_net, dest_pin))
          ((sta::ConcreteNetwork*) target_nwk_)
              ->connectPin(dest_pin, driving_net);
      }
    }
  }
  //Step 4: wire up the primary outputs
  // Wire up the outputs
  for (i = 0; i < Abc_NtkPoNum(mapped_netlist_); i++) {
    pObj = Abc_NtkPo(mapped_netlist_, i);
    // get the fanin driver
    // demote this assert to a test
    assert(Abc_ObjFaninNum(pObj) > 0);
    Abc_Obj_t_* fanin = Abc_ObjFanin(pObj, 0);
    sta::Net* driving_net = abc2net[fanin];
    sta::Pin* driving_pin = nullptr;
    if (!driving_net) {
      fanin = Abc_ObjFanin(fanin, 0);
      driving_net = abc2net[fanin];
      sta::PinSet* drivers = target_nwk_->drivers(driving_net);
      if (drivers) {
        sta::PinSet::Iterator drvr_iter(drivers);
        driving_pin = const_cast<sta::Pin*>(drvr_iter.next());
#ifdef DEBUG_RESTRUCT
        printf(
            "Setting cut root %d to %s\n", i, target_nwk_->name(driving_pin));
#endif
        // hook up cut root to drive the other stuff in original
        sta::Net* orig_sink_net = target_nwk_->net(cut_->roots_[i]);
        sta::Port* driver_port = target_nwk_->port(driving_pin);
        sta::Instance* driving_instance = target_nwk_->instance(driving_pin);
        ((sta::NetworkEdit*) target_nwk_)->disconnectPin(cut_->roots_[i]);
        cut_->roots_[i] = driving_pin;
        ((sta::NetworkEdit*) target_nwk_)
            ->connectPin(cut_->roots_[i], orig_sink_net);
      }
    } else {
      // we are intentionally modifying this pin, so unconst it
      sta::Pin* cut_op_pin = const_cast<sta::Pin*>(cut_->roots_[i]);
#ifdef DEBUG_RESTRUCT
      printf("Cut op pin being connected up %s\n",
             target_nwk_->name(cut_op_pin));
      if (driving_pin)
        printf("But real root pin is %s\n", target_nwk_->name(driving_pin));
#endif

      if (!(target_nwk_->isConnected(driving_net, cut_op_pin))) {
        ((sta::ConcreteNetwork*) target_nwk_)
            ->connectPin(cut_op_pin, driving_net);
#ifdef DEBUG_RESTRUCT
        printf("Connecting cut op pin to driving_net\n");
#endif
      } else {
#ifdef DEBUG_RESTRUCT
        printf("Net is already connected\n");
#endif
      }
    }
  }
  // STA will clean up the connections as we delete the instances
  for (auto cur_inst : cut_->volume_) {
    ((sta::ConcreteNetwork*) target_nwk_)
        ->deleteInstance(const_cast<sta::Instance*>(cur_inst));
  }
}
}
