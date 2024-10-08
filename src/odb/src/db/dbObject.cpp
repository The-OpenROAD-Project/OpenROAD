///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2019, Nefelus Inc
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

#include <cstring>

#include "dbCore.h"
#include "dbDatabase.h"
#include "dbProperty.h"
#include "dbTable.h"
#include "odb/db.h"
#include "utl/Logger.h"

namespace odb {

uint dbObject::getId() const
{
  return getImpl()->getOID();
}

inline uint getOid(const char*& name)
{
  char* end;
  uint id = strtoul(name, &end, 10);
  ZASSERT(name != end);
  name = end;
  return id;
}

dbObjectType dbObject::getObjectType() const
{
  return getImpl()->getType();
}

//
// NOTE: The name declaration order MUST match the enumeration order.
//
static const char* name_tbl[] = {"dbDatabase",

                                 // Design Objects
                                 "dbChip",
                                 "dbGDSLib",
                                 "dbBlock",
                                 "dbInstHdr",
                                 "dbInst",
                                 "dbNet",
                                 "dbBTerm",
                                 "dbITerm",
                                 "dbBox",
                                 "dbVia",
                                 "dbTrackGrid",
                                 "dbObstruction",
                                 "dbBlockage",
                                 "dbWire",
                                 "dbSWire",
                                 "dbSBox",
                                 "dbCapNode",
                                 "dbRSeg",
                                 "dbCCSeg",
                                 "dbRow",
                                 "dbFill",
                                 "dbRegion",
                                 "dbHier",
                                 "dbBPin",
                                 // Generator Code Begin ObjectNames
                                 "dbAccessPoint",
                                 "dbBusPort",
                                 "dbDft",
                                 "dbGCellGrid",
                                 "dbGDSBoundary",
                                 "dbGDSBox",
                                 "dbGDSNode",
                                 "dbGDSPath",
                                 "dbGDSSRef",
                                 "dbGDSStructure",
                                 "dbGDSText",
                                 "dbGlobalConnect",
                                 "dbGroup",
                                 "dbGuide",
                                 "dbIsolation",
                                 "dbLevelShifter",
                                 "dbLogicPort",
                                 "dbMarker",
                                 "dbMarkerCategory",
                                 "dbMetalWidthViaMap",
                                 "dbModBTerm",
                                 "dbModInst",
                                 "dbModITerm",
                                 "dbModNet",
                                 "dbModule",
                                 "dbNetTrack",
                                 "dbPolygon",
                                 "dbPowerDomain",
                                 "dbPowerSwitch",
                                 "dbScanChain",
                                 "dbScanInst",
                                 "dbScanList",
                                 "dbScanPartition",
                                 "dbScanPin",
                                 "dbTechLayer",
                                 "dbTechLayerAreaRule",
                                 "dbTechLayerArraySpacingRule",
                                 "dbTechLayerCornerSpacingRule",
                                 "dbTechLayerCutClassRule",
                                 "dbTechLayerCutEnclosureRule",
                                 "dbTechLayerCutSpacingRule",
                                 "dbTechLayerCutSpacingTableDefRule",
                                 "dbTechLayerCutSpacingTableOrthRule",
                                 "dbTechLayerEolExtensionRule",
                                 "dbTechLayerEolKeepOutRule",
                                 "dbTechLayerForbiddenSpacingRule",
                                 "dbTechLayerKeepOutZoneRule",
                                 "dbTechLayerMaxSpacingRule",
                                 "dbTechLayerMinCutRule",
                                 "dbTechLayerMinStepRule",
                                 "dbTechLayerSpacingEolRule",
                                 "dbTechLayerSpacingTablePrlRule",
                                 "dbTechLayerTwoWiresForbiddenSpcRule",
                                 "dbTechLayerWidthTableRule",
                                 "dbTechLayerWrongDirSpacingRule",
                                 // Generator Code End ObjectNames

                                 // Lib Objects
                                 "dbLib",
                                 "dbGDSLib",
                                 "dbSite",
                                 "dbMaster",
                                 "dbMPin",
                                 "dbMTerm",
                                 "dbTarget",
                                 "dbTechAntennaPinModel",

                                 // Tech Objects
                                 "dbTech",
                                 "dbTechVia",
                                 "dbTechNonDefaultRule",
                                 "dbTechLayerRule",
                                 "dbTechSameNetRule",
                                 "dbTechLayerSpacingRule",
                                 "dbTechMinCutRule",
                                 "dbTechMinEncRule",
                                 "dbTechV55InfluenceEntry",
                                 "dbTechLayerAntennaRule",
                                 "dbTechViaRule",
                                 "dbTechViaGenerateRule",
                                 "dbTechViaLayerRule",

                                 "dbProperty",
                                 "dbName"};

const char* dbObject::getTypeName() const
{
  return name_tbl[getImpl()->getType()];
}

const char* dbObject::getTypeName(dbObjectType type)
{
  return name_tbl[type];
}

dbObjectType dbObject::getType(const char* name, utl::Logger* logger)
{
  std::size_t i = 0;
  for (const char* type_name : name_tbl) {
    if (strcmp(type_name, name) == 0) {
      return (dbObjectType) i;
    }

    i++;
  }

  logger->error(utl::ODB, 267, "Unable to find {} object type", name);

  // should not get here
  return (dbObjectType) 0;
}

// We have to compare the id not only of this object but all its
// owning objects to properly compare.  For example dbMTerm is owned
// by dbMaster so two mterms could have the same id within the scope
// of different masters.
bool compare_by_id(const dbObject* lhs, const dbObject* rhs)
{
  if (lhs == nullptr || rhs == nullptr) {
    return lhs < rhs;
  }
  const auto lhs_id = lhs->getId();
  const auto rhs_id = rhs->getId();
  if (lhs_id != rhs_id) {
    return lhs_id < rhs_id;
  }
  const auto lhs_owner = lhs->getImpl()->getOwner();
  const auto rhs_owner = rhs->getImpl()->getOwner();
  return compare_by_id(lhs_owner, rhs_owner);
}

}  // namespace odb
