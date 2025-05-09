/*
  Cut methods
 */
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
#include "rmp/Restructure.h"
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

//#define DEBUG_RESTRUCT
//#define REGRESS_RESTRUCT

using utl::RMP;
using namespace sta;


namespace rmp {

bool Cut::LeafPin(const Pin* cur_pin)
{
  for (auto i : leaves_)
    if (i == cur_pin)
      return true;
  return false;
}

void Cut::Print(sta::Network* nwk)
{
  printf("Cut %d\n", id_);
  printf("---\n");
  printf("Roots:\n");
  for (auto r : roots_)
    printf("Root %s\n", nwk->pathName(r));
  printf("Leaves:\n");
  for (auto l : leaves_)
    printf("Leaf %s\n", nwk->pathName(l));
  printf("Volume:\n");
  for (auto v : volume_)
    printf("V: %s\n", nwk->pathName(v));

  printf("---\n");
}


//
// Check axioms for cut
//
// 1. Same pin cannot be a leaf and a root.
// 2. Leaf and Root instances cannot be in volume.
//
bool Cut::Check(sta::Network* nwk)
{
  bool ret = true;
  std::set<sta::Pin*> root_leaves;
  std::set<sta::Pin*> leaves;
  std::set<sta::Pin*> roots;
  std::set<sta::Instance*> volume;
  for (auto i : volume_) {
    volume.insert(i);
  }
  for (auto r : roots_) {
    roots.insert(r);
    root_leaves.insert(r);
  }
  for (auto l : leaves_) {
    leaves.insert(l);
    if (root_leaves.find(l) != root_leaves.end()) {
      printf("Error same pin (%s)  cannot be a leaf and root\n",
             nwk->pathName(l));
      ret = false;
    } else
      root_leaves.insert(l);
  }
  for (auto i : volume_) {
    sta::InstancePinIterator* pin_it = nwk->pinIterator(i);
    while (pin_it->hasNext()) {
      sta::Pin* cur_pin = pin_it->next();
      if (roots.find(cur_pin) != roots.end()
          && !(nwk->direction(cur_pin)->isOutput())) {
        printf(
            "Error: Internal volume instance pin %s cannot be in  root set\n",
            nwk->pathName(cur_pin));
        ret = false;
      } else if (leaves.find(cur_pin) != leaves.end()) {
        printf("Error: Internal volume instance pin %s cannot be in leaf set\n",
               nwk->pathName(cur_pin));
        ret = false;
      }
      // Check that every driver is either driven by something in the volume
      // or a leaf
      if (nwk->direction(cur_pin)->isInput()) {
        sta::PinSet* drivers = nwk->drivers(cur_pin);
        sta::PinSet::Iterator drvr_iter(drivers);
        if (drivers) {
          sta::Pin* driving_pin = const_cast<sta::Pin*>(drvr_iter.next());
          sta::Instance* driving_instance = nwk->instance(driving_pin);
          // leaf or volume...
          if (!(leaves.find(driving_pin) != leaves.end()
                || volume.find(driving_instance) != volume.end())) {
            printf(
                "Error: every driving pin (eg %s) should be a leaf or volume "
                "instance pin (Escape node bug)\n",
                nwk->pathName(driving_pin));
            ret = false;
          }
        } else {
          printf("No driver found for pin %s\n", nwk->pathName(cur_pin));
          ret = false;
        }
      }
    }
  }
  assert(ret);
  printf("Success: Cut passes axiom checks\n");
  return ret;
}

}  // namespace rmp
