/////////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2019, The Regents of the University of California
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
///////////////////////////////////////////////////////////////////////////////

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

/*
Cut Generator
*/
#include "rmp/Restructure.h"

//#define CUTGEN_DEBUG 1

using utl::RMP;


namespace rmp {

void CutGen::GenerateInstanceWaveFrontCutSet(sta::Instance* cur_inst,
                                             std::vector<Cut*>& inst_cut_set)
{
  // build candidate wave front for cut enumeration
  std::vector<const sta::Pin*> candidate_wavefront;
  sta::InstancePinIterator* cpin_iter = network_->pinIterator(cur_inst);
  const sta::Pin* root_pin = nullptr;
  while (cpin_iter->hasNext()) {
    const sta::Pin* c_pin = cpin_iter->next();
    if (Boundary(c_pin))
      continue;
    if (network_->direction(c_pin)->isInput())
      candidate_wavefront.push_back(c_pin);
    else {
      root_pin = c_pin;
#ifdef CUTGEN_DEBUG
      printf("Registering root_pin %s\n", network_->pathName(root_pin));
#endif
    }
  }

  // Expand the wave front: Keep set, Expand Set
  expansionSet expansion_set;
  EnumerateExpansions(candidate_wavefront, expansion_set);
  //
  // Build cut for each expansion
  //
  for (auto exp_el : expansion_set) {
    std::vector<const sta::Pin*> keep = exp_el.first;
    std::vector<const sta::Pin*> expand = exp_el.second;
    Cut* cur_cut
        = BuildCutFromExpansionElement(cur_inst, root_pin, keep, expand);
#ifdef CUTGEN_DEBUG
    printf("Cut generated:\n");
    cur_cut->Print(network_);
#endif
    inst_cut_set.push_back(cur_cut);
  }
}

Cut* CutGen::BuildCutFromExpansionElement(
    sta::Instance* cur_inst,
    const sta::Pin* root_pin,
    std::vector<const sta::Pin*>& keep,  // set of pins to keep
    std::vector<const sta::Pin*>& expand)
{
  std::set<const sta::Pin*> unique_driver_pins;
  std::set<sta::Instance*> unique_insts;

  static int id;
  id++;

  Cut* ret = new Cut();
  ret->id_ = id;
#ifdef CUTGEN_DEBUG
  printf("Building cut %d from root %s\n", id, network_->pathName(root_pin));
  printf("Keep set:\n");
  for (auto k : keep) {
    printf("%s \n", network_->pathName(k));
  }
  printf("Expand set:\n");
  for (auto e : expand) {
    printf("%s \n", network_->pathName(e));
  }
#endif

  ret->roots_.push_back(const_cast<sta::Pin*>(root_pin));
  for (auto k : keep) {
    // get the drivers..
    sta::PinSet* drvrs = network_->drivers(k);
    if (drvrs) {
      sta::PinSet::Iterator drvr_iter(drvrs);
      if (drvr_iter.hasNext()) {
        const sta::Pin* driving_pin = drvr_iter.next();
        if (unique_driver_pins.find(driving_pin) == unique_driver_pins.end()) {
#ifdef CUTGEN_DEBUG
          printf("Registering driver pin %s\n",
                 network_->pathName(driving_pin));
#endif
          ret->leaves_.push_back(const_cast<sta::Pin*>(driving_pin));
          unique_driver_pins.insert(driving_pin);
        }
      }
    }
  }

  // if root is an output of a regular gate then put in the
  // volume
  if (network_->direction(root_pin)->isOutput())
    ret->volume_.push_back(cur_inst);

  unique_insts.insert(cur_inst);

  for (auto e : expand) {
    sta::PinSet* drvrs = network_->drivers(e);
    if (drvrs) {
      sta::PinSet::Iterator drvr_iter(drvrs);
      if (drvr_iter.hasNext()) {
        const sta::Pin* driving_pin = drvr_iter.next();

        // filter: a boundary (eg memory, or primary i/o)
        if (Boundary(driving_pin)) {
          if (unique_driver_pins.find(driving_pin)
              == unique_driver_pins.end()) {
            unique_driver_pins.insert(driving_pin);
            ret->leaves_.push_back(const_cast<sta::Pin*>(driving_pin));
          }
          continue;
        }

        if (unique_driver_pins.find(driving_pin) == unique_driver_pins.end()) {
          unique_driver_pins.insert(driving_pin);
#ifdef CUTGEN_DEBUG
          printf("Expanding driving pin %s\n", network_->pathName(driving_pin));
#endif

          // expand through buffers and inverters
          driving_pin = WalkThroughBuffersAndInverters(
              ret, driving_pin, unique_driver_pins);

          if (network_->direction(driving_pin)->isOutput()) {
            sta::LibertyPort* lp = network_->libertyPort(driving_pin);
            ret->internal_pins_[driving_pin] = lp;
          }

          // get the driving instance
          sta::Instance* driving_inst = network_->instance(driving_pin);
#ifdef CUTGEN_DEBUG
          printf("Expanding driving instance %s\n",
                 network_->pathName(driving_inst));
#endif

          // add to volume, uniquely
          if (unique_insts.find(driving_inst) == unique_insts.end()) {
            unique_insts.insert(driving_inst);
            ret->volume_.push_back(driving_inst);
            sta::InstancePinIterator* pin_iter
                = network_->pinIterator(driving_inst);
            while (pin_iter->hasNext()) {
              const sta::Pin* dpin = pin_iter->next();
              if (network_->direction(dpin)->isInput()) {
#ifdef CUTGEN_DEBUG
                printf("Expanding through driving instance ip %s\n",
                       network_->pathName(dpin));
#endif
                // get the driver add to cut leaves
                sta::PinSet* drvrs = network_->drivers(dpin);
                if (drvrs) {
                  sta::PinSet::Iterator drvr_iter(drvrs);
                  if (drvr_iter.hasNext()) {
                    const sta::Pin* cur_pin = drvr_iter.next();

                    // could be a primary input pin or an output pin
                    if (network_->direction(cur_pin)->isOutput()
                        || network_->term(cur_pin)) {
#ifdef CUTGEN_DEBUG
                      printf("Looking at driving pin %s\n",
                             network_->pathName(cur_pin));
#endif
                      if (unique_driver_pins.find(cur_pin)
                          == unique_driver_pins.end()) {
                        ret->leaves_.push_back(const_cast<sta::Pin*>(cur_pin));
                        unique_driver_pins.insert(cur_pin);
#ifdef CUTGEN_DEBUG
                        printf("Registering unique ip pin %s\n",
                               network_->pathName(cur_pin));
#endif
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  return ret;
}

//
// walk through a driving pin
//
const sta::Pin* CutGen::WalkThroughBuffersAndInverters(
    Cut* cut,
    const sta::Pin* op_pin,
    std::set<const sta::Pin*>& unique_set)
{
  sta::Instance* cur_inst = network_->instance(op_pin);
  sta::InstancePinIterator* cpin_iter = network_->pinIterator(cur_inst);
  int var_count = 0;
  const sta::Pin* ip_pin;
  while (cpin_iter->hasNext()) {
    const sta::Pin* c_pin = cpin_iter->next();
    if (network_->direction(c_pin)->isInput()) {
      var_count++;
      ip_pin = c_pin;
    }
  }
  if (var_count == 1) {
    sta::LibertyPort* lp = network_->libertyPort(op_pin);
    if (lp && lp->function()) {
      unsigned* kit_table = nullptr;
      // TODO
      //	FuncExpr2Kit FuncExpr2Kit(lp -> function(), var_count,
      //kit_table);
      unsigned num_bits_to_set = (0x01 << var_count);
      unsigned mask = 0x00;
      for (unsigned j = 0; j < num_bits_to_set; j++)
        mask = mask | (0x1 << j);
      unsigned result = *kit_table & mask;

      if (0) /*(Kit::Kit_IsBuffer(kit_table,var_count) ||
               Kit::Kit_IsInverter(kit_table,var_count))*/
      {
        if (ip_pin) {
#ifdef CUTGEN_DEBUG
          printf("Walking through buffer/inverter\n");
#endif
          sta::PinSet* drvrs = network_->drivers(ip_pin);
          sta::PinSet::Iterator drvr_iter(drvrs);
          if (drvr_iter.hasNext()) {
            const sta::Pin* driving_pin = drvr_iter.next();
            if (unique_set.find(driving_pin) == unique_set.end()) {
              unique_set.insert(driving_pin);
              cut->volume_.push_back(cur_inst);
              cut->internal_pins_[driving_pin] = lp;
              return WalkThroughBuffersAndInverters(
                  cut, driving_pin, unique_set);
            } else
              return driving_pin;
          }
        }
      }
    }
  }
  // default do nothing
  return op_pin;
}

bool CutGen::EnumerateExpansions(
    std::vector<const sta::Pin*>& candidate_wavefront,
    expansionSet& expansion_set)
{
  // equivalent to the cartesian product, but organized for physical level.
  // remove trivial cases
  if (candidate_wavefront.size() == 2) {
    const sta::Pin* p1 = candidate_wavefront[0];
    const sta::Pin* p2 = candidate_wavefront[1];
    std::vector<const sta::Pin*> keep;
    std::vector<const sta::Pin*> expand;

    // case analysis
    /*
    keep.push_back(p1);
    keep.push_back(p2);
    expansion_set.push_back(std::pair<std::vector<const
    sta::Pin*>,std::vector<const sta::Pin*> >(keep,expand));

    keep.clear();
    expand.clear();
    keep.push_back(p1);
    expand.push_back(p2);
    expansion_set.push_back(std::pair<std::vector<const
    sta::Pin*>,std::vector<const sta::Pin*> >(keep,expand));

    keep.clear();
    expand.clear();
    */

    keep.push_back(p2);
    expand.push_back(p1);
    expansion_set.push_back(
        std::pair<std::vector<const sta::Pin*>, std::vector<const sta::Pin*>>(
            keep, expand));

    keep.clear();
    expand.clear();

    expand.push_back(p1);
    expand.push_back(p2);
    expansion_set.push_back(
        std::pair<std::vector<const sta::Pin*>, std::vector<const sta::Pin*>>(
            keep, expand));

    return true;
  }

  //
  // N == 3
  //
  // K:{p1,p2,p3} E:{}
  // K:{p1} E:{p2,p3}
  // K:{p1,p2}  E: {p3}
  // K: {p1,p3} E: {p2}

  // E:{p1,p2,p3} K:{}
  // E:{p1} K:{p2,p3}
  // E:{p1,p2}  K: {p3}
  // E: {p1,p3} K: {p2}

  else if (candidate_wavefront.size() == 3) {
    const sta::Pin* p1 = candidate_wavefront[0];
    const sta::Pin* p2 = candidate_wavefront[1];
    const sta::Pin* p3 = candidate_wavefront[2];
    std::vector<const sta::Pin*> keep;
    std::vector<const sta::Pin*> expand;

    // case analysis
    /*
    keep.push_back(p1);
    keep.push_back(p2);
    keep.push_back(p3);
    expansion_set.push_back(std::pair<std::vector<const
    sta::Pin*>,std::vector<const sta::Pin*> >(keep,expand));

    keep.clear();
    expand.clear();
    */

    keep.push_back(p1);
    expand.push_back(p2);
    expand.push_back(p3);
    expansion_set.push_back(
        std::pair<std::vector<const sta::Pin*>, std::vector<const sta::Pin*>>(
            keep, expand));

    keep.clear();
    expand.clear();
    keep.push_back(p1);
    keep.push_back(p2);
    expand.push_back(p3);
    expansion_set.push_back(
        std::pair<std::vector<const sta::Pin*>, std::vector<const sta::Pin*>>(
            keep, expand));

    keep.clear();
    expand.clear();

    keep.push_back(p1);
    keep.push_back(p3);
    expand.push_back(p2);
    expansion_set.push_back(
        std::pair<std::vector<const sta::Pin*>, std::vector<const sta::Pin*>>(
            keep, expand));

    keep.clear();
    expand.clear();
    expand.push_back(p1);
    expand.push_back(p2);
    expand.push_back(p3);
    expansion_set.push_back(
        std::pair<std::vector<const sta::Pin*>, std::vector<const sta::Pin*>>(
            keep, expand));

    keep.clear();
    expand.clear();
    expand.push_back(p1);
    keep.push_back(p2);
    keep.push_back(p3);
    expansion_set.push_back(
        std::pair<std::vector<const sta::Pin*>, std::vector<const sta::Pin*>>(
            keep, expand));

    keep.clear();
    expand.clear();
    expand.push_back(p1);
    expand.push_back(p2);
    keep.push_back(p3);
    expansion_set.push_back(
        std::pair<std::vector<const sta::Pin*>, std::vector<const sta::Pin*>>(
            keep, expand));

    keep.clear();
    expand.clear();
    expand.push_back(p1);
    expand.push_back(p3);
    keep.push_back(p2);
    expansion_set.push_back(
        std::pair<std::vector<const sta::Pin*>, std::vector<const sta::Pin*>>(
            keep, expand));
    return true;
  } else if (candidate_wavefront.size() == 4) {
    //
    // N == 4
    //
    // K:{p1,p2,p3,p4} E:{}

    // K:{p1} E:{p2,p3,p4}
    // K:{p1,p2}  E: {p3,p4}
    // K:{p1,p2,p3} E: {p4}

    // K:{p2} E: {p1,p3,p4}
    // K:{p2,p3} E: {p1,p4}
    // K:{p2,p3,p4} E: {p1}

    // K:{p3} E: {p1,p2,p4}
    // K:{p3,p1} E: {p2,p4}
    // K:{p3,p1,p2} E: {p4}

    // K:{} E{p1,p2,p3,p4}

    // E:{p1,p2,p3} K:{}
    // E:{p1} K:{p2,p3}
    // E:{p1,p2}  K: {p3}
    // E: {p1,p3} K: {p2}
  }
  return false;
}

bool CutGen::Boundary(const sta::Pin* cur_pin)
{
  // a terminal
  if (network_->term(cur_pin))
    return true;
  // a memory
  sta::Instance* cur_inst = network_->instance(cur_pin);
  if (cur_inst) {
    sta::LibertyCell* cell = network_->libertyCell(cur_inst);
    if (cell->isMemory() || cell->hasSequentials())
      return true;
  }
  return false;
}

}  // namespace rmp
