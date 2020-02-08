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

#ifndef __PSN_DATABASE_SDC_NETWORK_H__
#define __PSN_DATABASE_SDC_NETWORK_H__

#ifndef OPENROAD_OPENPHYSYN_LIBRARY_BUILD

// Temproary fix for OpenSTA
#define THROW_DCL throw()

#include "SdcNetwork.hh"

namespace sta
{

class DatabaseSdcNetwork : public SdcNetwork
{
public:
    DatabaseSdcNetwork(Network* network);
    virtual Instance* findInstance(const char* path_name) const;
    virtual void      findInstancesMatching(const Instance*     contex,
                                            const PatternMatch* pattern,
                                            InstanceSeq*        insts) const;
    virtual void findNetsMatching(const Instance*, const PatternMatch* pattern,
                                  NetSeq* nets) const;
    virtual void findPinsMatching(const Instance*     instance,
                                  const PatternMatch* pattern,
                                  PinSeq*             pins) const;

protected:
    void findInstancesMatching1(const PatternMatch* pattern,
                                InstanceSeq*        insts) const;
    void findNetsMatching1(const PatternMatch* pattern, NetSeq* nets) const;
    void findMatchingPins(const Instance*     instance,
                          const PatternMatch* port_pattern, PinSeq* pins) const;
    Pin* findPin(const char* path_name) const;

    using SdcNetwork::findPin;
};

} // namespace sta
#else
#include "dbSdcNetwork.hh"

namespace sta
{
class dbSdcNetwork;
typedef dbSdcNetwork DatabaseSdcNetwork;
} // namespace sta
#endif
#endif
