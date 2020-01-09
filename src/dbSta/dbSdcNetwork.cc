// Resizer, LEF/DEF gate resizer
// Copyright (c) 2019, Parallax Software, Inc.
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include "Machine.hh"
#include "PatternMatch.hh"
#include "ParseBus.hh"
#include "dbSdcNetwork.hh"

namespace sta {

static const char *
escapeDividers(const char *token,
	       const Network *network);

dbSdcNetwork::dbSdcNetwork(Network *network) :
  SdcNetwork(network)
{
}

// Override SdcNetwork to NetworkNameAdapter.
Instance *
dbSdcNetwork::findInstance(const char *path_name) const
{
  Instance *inst = network_->findInstance(path_name);
  if (inst == nullptr)
    inst = network_->findInstance(escapeDividers(path_name, this));
  return inst;
}

void
dbSdcNetwork::findInstancesMatching(const Instance *,
				    const PatternMatch *pattern,
				    InstanceSeq *insts) const
{
  if (pattern->hasWildcards())
    findInstancesMatching1(pattern, insts);
  else {
    Instance *inst = findInstance(pattern->pattern());
    if (inst)
      insts->push_back(inst);
    else {
      // Look for a match with path dividers escaped.
      const char *escaped = escapeChars(pattern->pattern(), divider_, '\0',
					escape_);
      inst = findInstance(escaped);
      if (inst)
	insts->push_back(inst);
      else
	// Malo
	findInstancesMatching1(pattern, insts);
    }
  }
}

void
dbSdcNetwork::findInstancesMatching1(const PatternMatch *pattern,
				     InstanceSeq *insts) const
{
  InstanceChildIterator *child_iter = childIterator(topInstance());
  while (child_iter->hasNext()) {
    Instance *child = child_iter->next();
    if (pattern->match(staToSdc(name(child))))
      insts->push_back(child);
  }
  delete child_iter;
}

void
dbSdcNetwork::findNetsMatching(const Instance *,
			       const PatternMatch *pattern,
			       NetSeq *nets) const
{
  if (pattern->hasWildcards())
    findNetsMatching1(pattern, nets);
  else {
    Net *net = findNet(pattern->pattern());
    if (net)
      nets->push_back(net);
    else {
      // Look for a match with path dividers escaped.
      const char *escaped = escapeChars(pattern->pattern(), divider_, '\0',
					escape_);
      net = findNet(escaped);
      if (net)
	nets->push_back(net);
      else
	findNetsMatching1(pattern, nets);
    }
  }
}

void
dbSdcNetwork::findNetsMatching1(const PatternMatch *pattern,
				NetSeq *nets) const
{
  NetIterator *net_iter = netIterator(topInstance());
  while (net_iter->hasNext()) {
    Net *net = net_iter->next();
    if (pattern->match(staToSdc(name(net))))
      nets->push_back(net);
  }
  delete net_iter;
}

void
dbSdcNetwork::findPinsMatching(const Instance *instance,
			       const PatternMatch *pattern,
			       PinSeq *pins) const
{
  if (stringEq(pattern->pattern(), "*")) {
    // Pattern of '*' matches all child instance pins.
    InstanceChildIterator *child_iter = childIterator(instance);
    while (child_iter->hasNext()) {
      Instance *child = child_iter->next();
      InstancePinIterator *pin_iter = pinIterator(child);
      while (pin_iter->hasNext()) {
	Pin *pin = pin_iter->next();
	pins->push_back(pin);
      }
      delete pin_iter;
    }
    delete child_iter;
  }
  else {
    char *inst_path, *port_name;
    pathNameLast(pattern->pattern(), inst_path, port_name);
    if (port_name) {
      PatternMatch inst_pattern(inst_path, pattern);
      InstanceSeq insts;
      findInstancesMatching(nullptr, &inst_pattern, &insts);
      PatternMatch port_pattern(port_name, pattern);
      for (auto inst : insts)
	findMatchingPins(inst, &port_pattern, pins);
    }
  }
}

void
dbSdcNetwork::findMatchingPins(const Instance *instance,
			       const PatternMatch *port_pattern,
			       PinSeq *pins) const
{
  if (instance != network_->topInstance()) {
    Cell *cell = network_->cell(instance);
    CellPortIterator *port_iter = network_->portIterator(cell);
    while (port_iter->hasNext()) {
      Port *port = port_iter->next();
      const char *port_name = network_->name(port);
      if (network_->hasMembers(port)) {
	bool bus_matches = port_pattern->match(port_name)
	  || port_pattern->match(escapeDividers(port_name, network_));
	PortMemberIterator *member_iter = network_->memberIterator(port);
	while (member_iter->hasNext()) {
	  Port *member_port = member_iter->next();
	  Pin *pin = network_->findPin(instance, member_port);
	  if (pin) {
	    if (bus_matches)
	      pins->push_back(pin);
	    else {
	      const char *member_name = network_->name(member_port);
	      if (port_pattern->match(member_name)
		  || port_pattern->match(escapeDividers(member_name, network_)))
		pins->push_back(pin);
	    }
	  }
	}
	delete member_iter;
      }
      else if (port_pattern->match(port_name)
	       || port_pattern->match(escapeDividers(port_name, network_))) {
	Pin *pin = network_->findPin(instance, port);
	if (pin)
	  pins->push_back(pin);
      }
    }
    delete port_iter;
  }
}

Pin *
dbSdcNetwork::findPin(const char *path_name) const
{
  char *inst_path, *port_name;
  pathNameLast(path_name, inst_path, port_name);
  Pin *pin = nullptr;
  if (inst_path) {
    Instance *inst = findInstance(inst_path);
    if (inst)
      pin = findPin(inst, port_name);
    else
      pin = nullptr;
  }
  else
    pin = findPin(topInstance(), path_name);
  stringDelete(inst_path);
  stringDelete(port_name);
  return pin;
}

static const char *
escapeDividers(const char *token,
	       const Network *network)
{
  return escapeChars(token, network->pathDivider(), '\0',
		     network->pathEscape());
}

} // namespace
