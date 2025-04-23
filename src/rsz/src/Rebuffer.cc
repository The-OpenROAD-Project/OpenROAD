// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include <algorithm>
#include <cstddef>
#include <limits>
#include <string>
#include <unordered_map>
#include <utility>

#include "BufferedNet.hh"
#include "RepairSetup.hh"
#include "db_sta/dbNetwork.hh"
#include "rsz/Resizer.hh"
#include "sta/Corner.hh"
#include "sta/DcalcAnalysisPt.hh"
#include "sta/Fuzzy.hh"
#include "sta/Graph.hh"
#include "sta/Liberty.hh"
#include "sta/Parasitics.hh"
#include "sta/PortDirection.hh"
#include "sta/Units.hh"

namespace rsz {

using std::make_shared;

using utl::RSZ;

using sta::fuzzyGreater;
using sta::fuzzyGreaterEqual;
using sta::fuzzyLess;
using sta::fuzzyLessEqual;
using sta::INF;
using sta::NetConnectedPinIterator;
using sta::Path;
using sta::PinSeq;

// Template magic to make it easier to write algorithms descending
// over the buffer tree in the form of lambdas; it allows recursive
// lambda calling and it keeps track of the level number which is important
// for good debug prints
//
// https://stackoverflow.com/questions/2067988/how-to-make-a-recursive-lambda
template <class F>
struct visitor
{
  F f;
  int level = 0;
  explicit visitor(F&& f) : f(std::forward<F>(f)) {}
  template <class... Args>
  decltype(auto) operator()(Args&&... args)
  {
    level++;
    decltype(auto) ret = f(*this, level, std::forward<Args>(args)...);
    level--;
    return ret;
  }

  // delete the copy constructor
  visitor(const visitor&) = delete;
  visitor& operator=(const visitor&) = delete;
};

template <typename F, class... Args>
decltype(auto) visitTree(F&& f, Args&&... args)
{
  visitor<std::decay_t<F>> v{std::forward<F>(f)};
  return v(std::forward<Args>(args)...);
}

void pruneCapVsSlackOptions(StaState* sta, BufferedNetSeq& options)
{
  // Prune the options if there exists another option with
  // larger slack and smaller capacitance.
  // This is fanout*log(fanout) if options are
  // presorted to hit better options sooner.
  std::unordered_map<BufferedNet*, Slack> slacks;

  for (const BufferedNetPtr& p : options) {
    slacks[p.get()] = p->slack();
  }

  sort(options.begin(),
       options.end(),
       [&slacks](const BufferedNetPtr& option1, const BufferedNetPtr& option2) {
         const Slack slack1 = slacks[option1.get()];
         const Slack slack2 = slacks[option2.get()];
         return std::make_tuple(-slack1, option1->cap())
                < std::make_tuple(-slack2, option2->cap());
       });

  if (options.empty()) {
    return;
  }

  float lowest_cap_seen = options[0]->cap();
  size_t si = 1;
  // Remove options by shifting down with index si.
  // Because the options are sorted we don't have to look
  // beyond the first option. We also know that slack
  // is nonincreasing, so we can remove everything that has
  // higher capacitance than the lowest found so far.
  for (size_t pi = si; pi < options.size(); pi++) {
    const BufferedNetPtr& p = options[pi];
    const float cap = p->cap();
    // If cap is the same or worse than lowest_cap_seen, remove solution p.
    if (fuzzyLess(cap, lowest_cap_seen)) {
      // Otherwise copy the survivor down.
      options[si++] = p;
      lowest_cap_seen = cap;
    }
  }
  options.resize(si);
}

void pruneCapVsAreaOptions(StaState* sta, BufferedNetSeq& options)
{
  sort(options.begin(),
       options.end(),
       [](const BufferedNetPtr& option1, const BufferedNetPtr& option2) {
         return std::make_tuple(option1->area(), option1->cap())
                < std::make_tuple(option2->area(), option2->cap());
       });

  if (options.empty()) {
    return;
  }

  float lowest_cap_seen = options[0]->cap();
  size_t si = 1;
  for (size_t pi = si; pi < options.size(); pi++) {
    const BufferedNetPtr& p = options[pi];
    const float cap = p->cap();
    // If cap is the same or worse than lowest_cap_seen, remove solution p.
    if (fuzzyLess(cap, lowest_cap_seen)) {
      // Otherwise copy the survivor down.
      options[si++] = p;
      lowest_cap_seen = cap;
    }
  }
  options.resize(si);
}

// Find initial timing-optimized rebuffering choice
BufferedNetPtr RepairSetup::rebufferForTiming(const BufferedNetPtr& bnet)
{
  using BnetType = BufferedNetType;
  using BnetSeq = BufferedNetSeq;
  using BnetPtr = BufferedNetPtr;

  const BnetSeq Z = visitTree(
      [this](auto& recurse, const int level, const BnetPtr& bnet) -> BnetSeq {
        switch (bnet->type()) {
          case BnetType::wire: {
            BnetSeq Z = recurse(bnet->ref());
            for (BnetPtr& z : Z) {
              z = addWire(z, bnet->location(), bnet->layer(), level);
            }
            addBuffers(Z, level);
            return Z;
          }

          case BnetType::junction: {
            const BnetSeq& Z1 = recurse(bnet->ref());
            const BnetSeq& Z2 = recurse(bnet->ref2());
            BnetSeq Z;
            Z.reserve(Z1.size() * Z2.size());

            // Combine the options from both branches.
            for (const BnetPtr& p : Z1) {
              for (const BnetPtr& q : Z2) {
                const BufferedNetPtr& min_req
                    = fuzzyLess(p->slack(), q->slack()) ? p : q;
                BufferedNetPtr junc
                    = make_shared<BufferedNet>(BufferedNetType::junction,
                                               bnet->location(),
                                               p,
                                               q,
                                               resizer_);
                junc->setArrivalPath(min_req->arrivalPath());
                junc->setRequiredPath(min_req->requiredPath());
                junc->setRequiredDelay(min_req->requiredDelay());
                Z.push_back(std::move(junc));
              }
            }

            pruneCapVsSlackOptions(sta_, Z);
            return Z;
          }

          case BufferedNetType::load: {
            const Pin* load_pin = bnet->loadPin();
            Vertex* vertex = graph_->pinLoadVertex(load_pin);
            Path* req_path = sta_->vertexWorstSlackPath(vertex, max_);
            Path* arrival_path = nullptr;
            if (req_path) {
              arrival_path = req_path->prevPath();
            }
            bnet->setArrivalPath(arrival_path);
            bnet->setRequiredPath(req_path);
            debugPrint(logger_,
                       RSZ,
                       "rebuffer",
                       4,
                       "{:{}s}{}",
                       "",
                       level,
                       bnet->to_string(resizer_));
            BufferedNetSeq Z;
            Z.push_back(bnet);
            return Z;
          }
          default: {
            logger_->critical(RSZ, 71, "unhandled BufferedNet type");
          }
        }
      },
      bnet);

  debugPrint(logger_, RSZ, "rebuffer", 2, "timing-optimized options");

  Delay best_slack = -INF;
  BufferedNetPtr best_option = nullptr;
  int best_index = 0;
  int i = 1;
  for (const BufferedNetPtr& p : Z) {
    // Find slack for drvr_pin into option.
    const Path* req_path = p->requiredPath();
    if (req_path) {
      Slack slack = slackAtDriverPin(p, i);
      if (best_option == nullptr || fuzzyGreater(slack, best_slack)) {
        best_slack = slack;
        best_option = p;
        best_index = i;
      }
      i++;
    }
  }

  if (best_option) {
    debugPrint(logger_, RSZ, "rebuffer", 2, "best option {}", best_index);
  } else {
    debugPrint(logger_, RSZ, "rebuffer", 2, "no available option");
  }

  return best_option;
}

// Recover area on a rebuffering choice without regressing timing
BufferedNetPtr RepairSetup::recoverArea(const BufferedNetPtr& bnet,
                                        const Delay slack_target,
                                        const float alpha)
{
  using BnetType = BufferedNetType;
  using BnetSeq = BufferedNetSeq;
  using BnetPtr = BufferedNetPtr;

  const Delay drvr_delay = bnet->slack() - slackAtDriverPin(bnet);

  // spread down arrival delay
  visitTree(
      [](auto& recurse,
         const int level,
         const BnetPtr& bnet,
         const Delay arrival) -> int {
        bnet->setArrivalDelay(arrival);
        switch (bnet->type()) {
          case BnetType::wire:
          case BnetType::buffer:
            recurse(bnet->ref(), arrival + bnet->delay());
            break;
          case BnetType::junction:
            recurse(bnet->ref(), arrival);
            recurse(bnet->ref2(), arrival);
            break;
          case BnetType::load:
            break;
        }
        return 0;
      },
      bnet,
      drvr_delay);

  BnetSeq Z = visitTree(
      [&](auto& recurse,
          const int level,
          const BnetPtr& bnet) -> BufferedNetSeq {
        switch (bnet->type()) {
          case BnetType::buffer: {
            assert(bnet->ref()->type() == BnetType::wire);
            BnetSeq Z = recurse(bnet->ref()->ref());
            for (BnetPtr& z : Z) {
              z = addWire(z, bnet->location(), bnet->ref()->layer(), level);
            }
            addBuffers(Z,
                       level,
                       true,
                       (slack_target + bnet->arrivalDelay()) * alpha
                           + bnet->slack() * (1.0 - alpha));
            return Z;
          }
          case BnetType::wire: {
            BnetSeq Z = recurse(bnet->ref());
            for (BnetPtr& z : Z) {
              z = addWire(z, bnet->location(), bnet->layer(), level);
            }
            addBuffers(Z,
                       level,
                       true,
                       (slack_target + bnet->arrivalDelay()) * alpha
                           + bnet->slack() * (1.0 - alpha));
            return Z;
          }
          case BnetType::junction: {
            const BnetSeq& Z1 = recurse(bnet->ref());
            const BnetSeq& Z2 = recurse(bnet->ref2());
            BnetSeq Z;
            Z.reserve(Z1.size() * Z2.size());

            const Delay threshold
                = (slack_target + bnet->arrivalDelay()) * alpha
                  + bnet->slack() * (1.0 - alpha);

            Delay last_resort_slack = -INF;
            const BnetPtr *last_resort_p, *last_resort_q;

            // Combine the options from both branches.
            for (const BnetPtr& p : Z1) {
              for (const BnetPtr& q : Z2) {
                const BnetPtr& min_req
                    = fuzzyLess(p->slack(), q->slack()) ? p : q;

                if (Z.empty() && min_req->slack() > last_resort_slack) {
                  last_resort_p = &p;
                  last_resort_q = &q;
                }

                if (fuzzyLess(min_req->slack(), threshold)) {
                  // Filter out an option that doesn't meet local timing
                  // threshold
                  continue;
                }

                auto junc = make_shared<BufferedNet>(
                    BnetType::junction, bnet->location(), p, q, resizer_);
                junc->setArrivalPath(min_req->arrivalPath());
                junc->setRequiredPath(min_req->requiredPath());
                junc->setRequiredDelay(min_req->requiredDelay());

                debugPrint(logger_,
                           RSZ,
                           "rebuffer",
                           4,
                           "{:{}s}junc {}",
                           "",
                           level,
                           junc->to_string(resizer_));

                Z.push_back(std::move(junc));
              }
            }

            if (Z.empty() && last_resort_p && last_resort_q) {
              // No option met the timing threshold (probably due to rounding
              // errors), pick at least what came closest
              const BnetPtr& p = *last_resort_p;
              const BnetPtr& q = *last_resort_q;
              const BnetPtr& min_req
                  = fuzzyLess(p->slack(), q->slack()) ? p : q;
              auto junc = make_shared<BufferedNet>(
                  BnetType::junction, bnet->location(), p, q, resizer_);
              junc->setArrivalPath(min_req->arrivalPath());
              junc->setRequiredPath(min_req->requiredPath());
              junc->setRequiredDelay(min_req->requiredDelay());

              debugPrint(logger_,
                         RSZ,
                         "rebuffer",
                         4,
                         "{:{}s}last resort junc {}",
                         "",
                         level,
                         junc->to_string(resizer_));

              Z.push_back(std::move(junc));
            }

            pruneCapVsAreaOptions(sta_, Z);
            return Z;
          }
          case BnetType::load: {
            debugPrint(logger_,
                       RSZ,
                       "rebuffer",
                       3,
                       "{:{}s}{}",
                       "",
                       level,
                       bnet->to_string(resizer_));
            return {bnet};
          }
          default: {
            logger_->critical(RSZ, 150, "unhandled BufferedNet type");
          }
        }
      },
      bnet);

  Delay best_slack = -INF;
  float best_area = std::numeric_limits<float>::max();
  BufferedNetPtr best_slack_option = nullptr;
  BufferedNetPtr best_area_option = nullptr;
  int best_slack_index = 0;
  int best_area_index = 0;
  int i = 1;
  for (const BufferedNetPtr& p : Z) {
    // Find slack for drvr_pin into option.
    const Path* req_path = p->requiredPath();
    if (req_path) {
      const Slack slack = slackAtDriverPin(p, i);
      if (best_slack_option == nullptr || fuzzyGreater(slack, best_slack)) {
        best_slack = slack;
        best_slack_option = p;
        best_slack_index = i;
      }
      if (fuzzyGreaterEqual(slack, slack_target)
          && (best_area_option == nullptr || fuzzyLess(p->area(), best_area))) {
        best_area = p->area();
        best_area_option = p;
        best_area_index = i;
      }
      i++;
    }
  }

  if (best_area_option) {
    debugPrint(logger_,
               RSZ,
               "rebuffer",
               2,
               "best option {} (area optimized)",
               best_area_index);
    return best_area_option;
  }
  if (best_slack_option) {
    debugPrint(logger_,
               RSZ,
               "rebuffer",
               2,
               "best option {} (closest to meeting timing)",
               best_slack_index);
    return best_slack_option;
  }
  debugPrint(logger_, RSZ, "rebuffer", 2, "no available option");
  return nullptr;
}

// Return inserted buffer count.
int RepairSetup::rebuffer(const Pin* drvr_pin)
{
  int inserted_buffer_count = 0;
  Net* net;

  Instance* parent
      = db_network_->getOwningInstanceParent(const_cast<Pin*>(drvr_pin));
  odb::dbNet* db_net = nullptr;
  odb::dbModNet* db_modnet = nullptr;

  if (network_->isTopLevelPort(drvr_pin)) {
    net = network_->net(network_->term(drvr_pin));
    db_network_->staToDb(net, db_net, db_modnet);

    LibertyCell* buffer_cell = resizer_->buffer_lowest_drive_;
    // Should use sdc external driver here.
    LibertyPort* input;
    buffer_cell->bufferPorts(input, drvr_port_);
  } else {
    db_network_->net(drvr_pin, db_net, db_modnet);

    net = network_->net(drvr_pin);
    drvr_port_ = network_->libertyPort(drvr_pin);
  }

  if (drvr_port_
      && net
      // Verilog connects by net name, so there is no way to distinguish the
      // net from the port.
      && !hasTopLevelOutputPort(net)) {
    corner_ = sta_->cmdCorner();
    BufferedNetPtr bnet = resizer_->makeBufferedNet(drvr_pin, corner_);

    if (bnet) {
      const bool debug = (drvr_pin == resizer_->debug_pin_);
      if (debug) {
        logger_->setDebugLevel(RSZ, "rebuffer", 3);
      }
      debugPrint(logger_,
                 RSZ,
                 "rebuffer",
                 2,
                 "driver {}",
                 sdc_network_->pathName(drvr_pin));
      sta_->findRequireds();

      BufferedNetPtr best_option = rebufferForTiming(bnet);

      if (best_option) {
        Delay drvr_gate_delay;
        std::tie(std::ignore, drvr_gate_delay) = drvrPinTiming(best_option);

        const Delay target = slackAtDriverPin(best_option)
                             - (drvr_gate_delay + best_option->requiredDelay())
                                   * rebuffer_relaxation_factor_;

        for (int i = 0; i < 5 && best_option; i++) {
          best_option = recoverArea(best_option, target, ((float) (1 + i)) / 5);
        }

        if (!best_option) {
          logger_->warn(
              RSZ,
              145,
              "area recovery lost the ball in rebuffering for driver {}",
              network_->pathName(drvr_pin));
        }
      }

      if (best_option) {
        //
        // get the modnet driver
        //
        odb::dbITerm* drvr_op_iterm = nullptr;
        odb::dbBTerm* drvr_op_bterm = nullptr;
        odb::dbModITerm* drvr_op_moditerm = nullptr;
        odb::dbModBTerm* drvr_op_modbterm = nullptr;
        db_network_->staToDb(drvr_pin,
                             drvr_op_iterm,
                             drvr_op_bterm,
                             drvr_op_moditerm,
                             drvr_op_modbterm);

        if (db_net && db_modnet) {
          // as we move the modnet and dbnet around we will get a clash
          //(the dbNet name now exposed is the same as the modnet name)
          // so we uniquify the modnet name
          const std::string new_name = resizer_->makeUniqueNetName();
          db_modnet->rename(new_name.c_str());
        }

        inserted_buffer_count = rebufferTopDown(best_option,
                                                db_network_->dbToSta(db_net),
                                                1,
                                                parent,
                                                drvr_op_iterm,
                                                db_modnet);
        if (inserted_buffer_count > 0) {
          rebuffer_net_count_++;
          debugPrint(logger_,
                     RSZ,
                     "rebuffer",
                     2,
                     "rebuffer {} inserted {}",
                     network_->pathName(drvr_pin),
                     inserted_buffer_count);
        }
      }
      if (debug) {
        logger_->setDebugLevel(RSZ, "rebuffer", 0);
      }
    } else {
      logger_->warn(RSZ,
                    75,
                    "makeBufferedNet failed for driver {}",
                    network_->pathName(drvr_pin));
    }
  }
  return inserted_buffer_count;
}

Slack RepairSetup::slackAtDriverPin(const BufferedNetPtr& bnet)
{
  return slackAtDriverPin(bnet, -1);
}

std::tuple<const Path*, Delay> RepairSetup::drvrPinTiming(
    const BufferedNetPtr& bnet)
{
  const Path* req_path = bnet->requiredPath();
  if (req_path) {
    const Path* arrival_path = bnet->arrivalPath();
    TimingArc* driver_arc = arrival_path->prevArc(resizer_);
    Path* driver_path = arrival_path->prevPath();
    if (driver_path) {
      const DcalcAnalysisPt* dcalc_ap = arrival_path->dcalcAnalysisPt(sta_);
      sta::LoadPinIndexMap load_pin_index_map(network_);
      Slew slew = graph_->slew(driver_path->vertex(sta_),
                               driver_arc->fromEdge()->asRiseFall(),
                               dcalc_ap->index());
      auto dcalc_result
          = arc_delay_calc_->gateDelay(nullptr,
                                       driver_arc,
                                       slew,
                                       bnet->cap() + drvr_port_->capacitance(),
                                       nullptr,
                                       load_pin_index_map,
                                       dcalc_ap);
      return {driver_path, dcalc_result.gateDelay()};
    }

    return {arrival_path, 0};
  }
  return {nullptr, 0};
}

Slack RepairSetup::slackAtDriverPin(const BufferedNetPtr& bnet,
                                    // Only used for debug print.
                                    const int index)
{
  const Path* req_path = bnet->requiredPath();
  if (req_path) {
    const Path* drvr_path;
    Delay drvr_gate_delay;
    std::tie(drvr_path, drvr_gate_delay) = drvrPinTiming(bnet);

    const Delay slack = req_path->required() - bnet->requiredDelay()
                        - drvr_gate_delay - drvr_path->arrival();

    if (index >= 0) {
      debugPrint(logger_,
                 RSZ,
                 "rebuffer",
                 2,
                 "option {:3d}: {:2d} buffers slack {} cap {}",
                 index,
                 bnet->bufferCount(),
                 delayAsString(slack, this, 3),
                 units_->capacitanceUnit()->asString(bnet->cap()));
    }
    return slack;
  }
  return INF;
}

// For testing.
void RepairSetup::rebufferNet(const Pin* drvr_pin)
{
  init();
  resizer_->incrementalParasiticsBegin();
  inserted_buffer_count_ = rebuffer(drvr_pin);
  // Leave the parasitics up to date.
  resizer_->updateParasitics();
  resizer_->incrementalParasiticsEnd();
  logger_->report("Inserted {} buffers.", inserted_buffer_count_);
}

bool RepairSetup::hasTopLevelOutputPort(Net* net)
{
  NetConnectedPinIterator* pin_iter = network_->connectedPinIterator(net);
  while (pin_iter->hasNext()) {
    const Pin* pin = pin_iter->next();
    if (network_->isTopLevelPort(pin) && network_->direction(pin)->isOutput()) {
      delete pin_iter;
      return true;
    }
  }
  delete pin_iter;
  return false;
}

BufferedNetPtr RepairSetup::addWire(const BufferedNetPtr& p,
                                    const Point& wire_end,
                                    const int wire_layer,
                                    const int level)
{
  const Path* req_path = p->requiredPath();
  const Corner* corner = !req_path ? sta_->cmdCorner()
                                   : req_path->dcalcAnalysisPt(sta_)->corner();

  BufferedNetPtr z = make_shared<BufferedNet>(
      BufferedNetType::wire, wire_end, wire_layer, p, corner, resizer_);

  double layer_res, layer_cap;
  z->wireRC(corner, resizer_, layer_res, layer_cap);
  const double wire_length = resizer_->dbuToMeters(z->length());
  const double wire_res = wire_length * layer_res;
  const double wire_cap = wire_length * layer_cap;
  const double wire_delay = wire_res * (wire_cap / 2 + p->cap());

  z->setArrivalPath(p->arrivalPath());
  z->setRequiredPath(req_path);
  // account for wire delay
  z->setDelay(wire_delay);
  z->setRequiredDelay(p->requiredDelay() + wire_delay);

  debugPrint(logger_,
             RSZ,
             "rebuffer",
             4,
             "{:{}s}wire wl {} {}",
             "",
             level,
             z->length(),
             z->to_string(resizer_));

  return z;
}

void RepairSetup::addBuffers(
    BufferedNetSeq& Z1,
    const int level,
    const bool area_oriented /*=false*/,
    const Delay slack_threshold /*=0; used for area mode only*/)
{
  if (!Z1.empty()) {
    BufferedNetSeq buffered_options;
    for (LibertyCell* buffer_cell : resizer_->buffer_fast_sizes_) {
      LibertyPort *in, *out;
      buffer_cell->bufferPorts(in, out);
      Delay best_slack = -INF;
      float best_area = std::numeric_limits<float>::max();
      BufferedNetPtr best_option = nullptr;
      BufferedNetPtr best_area_option = nullptr;

      for (const BufferedNetPtr& z : Z1) {
        const Path* req_path = z->requiredPath();
        // Do not buffer unconstrained paths.
        if (req_path) {
          const DcalcAnalysisPt* dcalc_ap = req_path->dcalcAnalysisPt(sta_);
          const Delay buffer_delay
              = resizer_->bufferDelay(buffer_cell,
                                      req_path->transition(sta_),
                                      z->cap() + out->capacitance(),
                                      dcalc_ap);
          const Delay slack = z->slack() - buffer_delay;
          if (fuzzyGreater(slack, best_slack)) {
            best_slack = slack;
            best_option = z;
          }

          if (area_oriented) {
            const float buffered_area = z->area() + 1;
            if (fuzzyGreaterEqual(slack, slack_threshold)
                && buffered_area < best_area) {
              best_area = buffered_area;
              best_area_option = z;
            }
          }
        }
      }

      if (area_oriented) {
        // In area-oriented mode, anything which meets the slack threshold
        // is OK timing-wise, we can select good area instead
        best_option = best_area_option;
      }

      if (best_option) {
        Required slack = INF;
        const Path* req_path = best_option->requiredPath();
        float buffer_cap = 0.0;
        Delay buffer_delay = 0.0;
        if (req_path) {
          const DcalcAnalysisPt* dcalc_ap = req_path->dcalcAnalysisPt(sta_);
          buffer_cap = bufferInputCapacitance(buffer_cell, dcalc_ap);
          buffer_delay
              = resizer_->bufferDelay(buffer_cell,
                                      req_path->transition(sta_),
                                      best_option->cap() + out->capacitance(),
                                      dcalc_ap);
          slack = req_path->slack(sta_) - buffer_delay;
        }
        bool prune = false;
        if (!area_oriented) {
          // Don't add this buffer option if it has worse input cap and req than
          // another existing buffer option.
          for (const BufferedNetPtr& buffer_option : buffered_options) {
            if (fuzzyLessEqual(buffer_option->cap(), buffer_cap)
                && fuzzyGreaterEqual(buffer_option->slack(), slack)) {
              prune = true;
              break;
            }
          }
        } else {
          // Ditto for cap vs area
          float buffered_area = best_option->area() + 1;
          for (const BufferedNetPtr& buffer_option : buffered_options) {
            if (fuzzyLessEqual(buffer_option->cap(), buffer_cap)
                && fuzzyLessEqual(buffer_option->area(), buffered_area)) {
              prune = true;
              break;
            }
          }
        }
        if (!prune) {
          BufferedNetPtr z = make_shared<BufferedNet>(
              BufferedNetType::buffer,
              // Locate buffer at opposite end of wire.
              best_option->location(),
              buffer_cell,
              best_option,
              corner_,
              resizer_);
          z->setArrivalPath(best_option->arrivalPath());
          z->setRequiredPath(best_option->requiredPath());
          z->setDelay(buffer_delay);
          z->setRequiredDelay(best_option->requiredDelay() + buffer_delay);
          debugPrint(logger_,
                     RSZ,
                     "rebuffer",
                     3,
                     "{:{}s}buffer cap {} req {} -> {}",
                     "",
                     level,
                     units_->capacitanceUnit()->asString(best_option->cap()),
                     delayAsString(best_slack, this),
                     z->to_string(resizer_));
          buffered_options.push_back(z);
        }
      }
    }
    for (const BufferedNetPtr& z : buffered_options) {
      Z1.push_back(z);
    }
  }
}

float RepairSetup::bufferInputCapacitance(LibertyCell* buffer_cell,
                                          const DcalcAnalysisPt* dcalc_ap)
{
  LibertyPort *input, *output;
  buffer_cell->bufferPorts(input, output);
  const int lib_ap = dcalc_ap->libertyIndex();
  const LibertyPort* corner_input = input->cornerPort(lib_ap);
  return corner_input->capacitance();
}

int RepairSetup::rebufferTopDown(const BufferedNetPtr& choice,
                                 Net* net,  // output of buffer.
                                 const int level,
                                 Instance* parent_in,
                                 odb::dbITerm* mod_net_drvr,
                                 odb::dbModNet* mod_net_in)
{
  // HFix, pass in the parent
  Instance* parent = parent_in;
  switch (choice->type()) {
    case BufferedNetType::buffer: {
      const std::string buffer_name = resizer_->makeUniqueInstName("rebuffer");

      // HFix: make net in hierarchy
      const std::string net_name = resizer_->makeUniqueNetName();
      Net* net2 = db_network_->makeNet(net_name.c_str(), parent);

      LibertyCell* buffer_cell = choice->bufferCell();
      Instance* buffer = resizer_->makeBuffer(
          buffer_cell, buffer_name.c_str(), parent, choice->location());

      resizer_->level_drvr_vertices_valid_ = false;
      LibertyPort *input, *output;
      buffer_cell->bufferPorts(input, output);
      debugPrint(logger_,
                 RSZ,
                 "rebuffer",
                 3,
                 "{:{}s}insert {} -> {} ({}) -> {}",
                 "",
                 level,
                 sdc_network_->pathName(net),
                 buffer_name.c_str(),
                 buffer_cell->name(),
                 sdc_network_->pathName(net2));

      odb::dbNet* db_ip_net = nullptr;
      odb::dbModNet* db_ip_modnet = nullptr;
      db_network_->staToDb(net, db_ip_net, db_ip_modnet);

      //      sta_->connectPin(buffer, input, net);  //rebuffer
      sta_->connectPin(
          buffer, input, db_network_->dbToSta(db_ip_net));  // rebuffer
      sta_->connectPin(buffer, output, net2);

      Pin* buffer_ip_pin = nullptr;
      Pin* buffer_op_pin = nullptr;

      resizer_->getBufferPins(buffer, buffer_ip_pin, buffer_op_pin);
      odb::dbITerm* buffer_op_iterm = nullptr;
      odb::dbBTerm* buffer_op_bterm = nullptr;
      odb::dbModITerm* buffer_op_moditerm = nullptr;
      odb::dbModBTerm* buffer_op_modbterm = nullptr;
      db_network_->staToDb(buffer_op_pin,
                           buffer_op_iterm,
                           buffer_op_bterm,
                           buffer_op_moditerm,
                           buffer_op_modbterm);

      // disconnect modnet from original driver
      // connect the output to the modnet.
      // inch the modnet to the end of the buffer chain created in this scope

      // Hierarchy handling
      if (mod_net_drvr && mod_net_in) {
        // save original dbnet
        dbNet* orig_db_net = mod_net_drvr->getNet();
        // disconnect everything
        mod_net_drvr->disconnect();
        // restore dbnet
        mod_net_drvr->connect(orig_db_net);
        // add the modnet to the new output
        buffer_op_iterm->connect(mod_net_in);
      }

      const int buffer_count = rebufferTopDown(
          choice->ref(), net2, level + 1, parent, buffer_op_iterm, mod_net_in);

      // ip_net
      odb::dbNet* db_net = nullptr;
      odb::dbModNet* db_modnet = nullptr;
      db_network_->staToDb(net, db_net, db_modnet);
      resizer_->parasiticsInvalid(db_network_->dbToSta(db_net));

      db_network_->staToDb(net2, db_net, db_modnet);
      resizer_->parasiticsInvalid(db_network_->dbToSta(db_net));
      return buffer_count + 1;
    }

    case BufferedNetType::wire:
      debugPrint(logger_, RSZ, "rebuffer", 3, "{:{}s}wire", "", level);
      return rebufferTopDown(
          choice->ref(), net, level + 1, parent, mod_net_drvr, mod_net_in);

    case BufferedNetType::junction: {
      debugPrint(logger_, RSZ, "rebuffer", 3, "{:{}s}junction", "", level);
      return rebufferTopDown(choice->ref(),
                             net,
                             level + 1,
                             parent,
                             mod_net_drvr,
                             mod_net_in)
             + rebufferTopDown(choice->ref2(),
                               net,
                               level + 1,
                               parent,
                               mod_net_drvr,
                               mod_net_in);
    }

    case BufferedNetType::load: {
      const Pin* load_pin = choice->loadPin();

      if (resizer_->dontTouch(load_pin)) {
        debugPrint(logger_,
                   RSZ,
                   "rebuffer",
                   3,
                   "{:{}s}connect load: skipped on {} due to dont touch",
                   "",
                   level,
                   sdc_network_->pathName(load_pin));
        return 0;
      }

      odb::dbNet* db_net = nullptr;
      odb::dbModNet* db_modnet = nullptr;
      db_network_->staToDb(net, db_net, db_modnet);

      // only access at dbnet level
      Net* load_net = network_->net(load_pin);

      dbNet* db_load_net;
      odb::dbModNet* db_mod_load_net;
      db_network_->staToDb(load_net, db_load_net, db_mod_load_net);
      (void) db_load_net;

      if (load_net != net) {
        odb::dbITerm* load_iterm = nullptr;
        odb::dbBTerm* load_bterm = nullptr;
        odb::dbModITerm* load_moditerm = nullptr;
        odb::dbModBTerm* load_modbterm = nullptr;
        db_network_->staToDb(
            load_pin, load_iterm, load_bterm, load_moditerm, load_modbterm);

        debugPrint(logger_,
                   RSZ,
                   "rebuffer",
                   3,
                   "{:{}s}connect load {} to {}",
                   "",
                   level,
                   sdc_network_->pathName(load_pin),
                   sdc_network_->pathName(load_net));

        // disconnect removes everything.
        sta_->disconnectPin(const_cast<Pin*>(load_pin));
        // prepare for hierarchy
        load_iterm->connect(db_net);
        // preserve the mod net.
        if (db_mod_load_net) {
          load_iterm->connect(db_mod_load_net);
        }

        // sta_->connectPin(load_inst, load_port, net);
        resizer_->parasiticsInvalid(db_network_->dbToSta(db_net));
        // resizer_->parasiticsInvalid(load_net);
      }
      return 0;
    }
  }
  return 0;
}

}  // namespace rsz
