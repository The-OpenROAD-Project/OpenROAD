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

#include "db.h"
#include "dbCore.h"
#include "dbDatabase.h"
#include "dbProperty.h"
#include "dbTable.h"
#include "utl/Logger.h"

namespace odb {

uint dbObject::getId() const
{
  return getImpl()->getOID();
}

void dbObject::getDbName(char name[max_name_length]) const
{
  const dbObject* parent;
  const dbObject* child;
  const dbObject* stack[16];
  const dbObject** end = &stack[16];
  const dbObject** sptr = end;

  for (child = this; child; child = parent) {
    parent = child->getImpl()->getOwner();
    *(--sptr) = child;
  }

  char* cptr = name;
  *cptr++ = '/';

  for (; sptr < end; ++sptr) {
    const dbObject* obj = *sptr;
    uint id;

    const _dbObject* impl = getImpl();

    switch (impl->getType()) {
      case dbDatabaseObj: {
        *cptr++ = 'D';
        _dbDatabase* db = (_dbDatabase*) obj;
        id = db->_unique_id;
        break;
      }

      // Design Objects
      case dbChipObj:
        *cptr++ = 'C';
        id = impl->getOID();
        break;

      case dbBlockObj:
        *cptr++ = 'B';
        id = impl->getOID();
        break;

      case dbInstHdrObj:
        getImpl()->getLogger()->critical(
            utl::ODB, 294, "dbInstHdrObj not expected in getDbName");
        break;

      case dbInstObj:
        *cptr++ = 'I';
        id = impl->getOID();
        break;

      case dbNetObj:
        *cptr++ = 'N';
        id = impl->getOID();
        break;

      case dbBTermObj:
        *cptr++ = 'T';
        id = impl->getOID();
        break;

      case dbITermObj:
        *cptr++ = 'Y';
        id = impl->getOID();
        break;

      case dbBoxObj:
        *cptr++ = 'X';
        id = impl->getOID();
        break;

      case dbViaObj:
        *cptr++ = 'V';
        id = impl->getOID();
        break;

      case dbGCellGridObj:
        *cptr++ = 'G';
        id = impl->getOID();
        break;

      case dbTrackGridObj:
        *cptr++ = 'K';
        id = impl->getOID();
        break;

      case dbObstructionObj:
        *cptr++ = 'O';
        id = impl->getOID();
        break;

      case dbBlockageObj:
        *cptr++ = 'E';
        id = impl->getOID();
        break;

      case dbWireObj:
        *cptr++ = 'W';
        id = impl->getOID();
        break;

      case dbSWireObj:
        *cptr++ = 'S';
        id = impl->getOID();
        break;

      case dbSBoxObj:
        *cptr++ = 'Q';
        id = impl->getOID();
        break;

      case dbCapNodeObj:  // DKF
        *cptr++ = 'n';
        id = impl->getOID();
        break;

      case dbRSegObj:  // DKF
        *cptr++ = 's';
        id = impl->getOID();
        break;

      case dbCCSegObj:
        *cptr++ = 'c';
        id = impl->getOID();
        break;

      case dbRowObj:
        *cptr++ = 'x';
        id = impl->getOID();
        break;

      case dbFillObj:
        *cptr++ = 'F';
        id = impl->getOID();
        break;

      case dbRegionObj:
        *cptr++ = 'a';
        id = impl->getOID();
        break;

      case dbHierObj:
        getImpl()->getLogger()->critical(
            utl::ODB, 295, "dbHierObj not expected in getDbName");
        break;

      case dbBPinObj:
        *cptr++ = 'p';
        id = impl->getOID();
        break;

      case dbGlobalConnectObj:
        *cptr++ = 'j';
        id = impl->getOID();
        break;

      // Lib Objects
      case dbLibObj:
        *cptr++ = 'L';
        id = impl->getOID();
        break;

      case dbSiteObj:
        *cptr++ = 'U';
        id = impl->getOID();
        break;

      case dbMasterObj:
        *cptr++ = 'M';
        id = impl->getOID();
        break;

      case dbMPinObj:
        *cptr++ = 'P';
        id = impl->getOID();
        break;

      case dbMTermObj:
        *cptr++ = 'T';
        id = impl->getOID();
        break;

      case dbTargetObj:
        *cptr++ = 't';
        id = impl->getOID();
        break;

      case dbTechAntennaPinModelObj:
        *cptr++ = 'Z';
        id = impl->getOID();
        break;

      // Tech Objects
      case dbTechObj:
        *cptr++ = 'H';
        id = impl->getOID();
        break;

      case dbTechLayerObj:
        *cptr++ = 'A';
        id = impl->getOID();
        break;

      case dbTechViaObj:
        *cptr++ = 'V';
        id = impl->getOID();
        break;

      case dbTechNonDefaultRuleObj:
        *cptr++ = 'd';
        id = impl->getOID();
        break;

      case dbTechLayerRuleObj:
        *cptr++ = 'l';
        id = impl->getOID();
        break;

      case dbTechSameNetRuleObj:
        *cptr++ = 'q';
        id = impl->getOID();
        break;

      case dbTechLayerSpacingRuleObj:
        *cptr++ = 'r';
        id = impl->getOID();
        break;

      case dbTechLayerSpacingEolRuleObj:
        *cptr++ = 'k';
        id = impl->getOID();
        break;

      case dbTechMinCutRuleObj:
        *cptr++ = 'e';
        id = impl->getOID();
        break;

      case dbTechMinEncRuleObj:
        *cptr++ = 'f';
        id = impl->getOID();
        break;

      case dbTechV55InfluenceEntryObj:
        *cptr++ = 'g';
        id = impl->getOID();
        break;

      case dbTechLayerAntennaRuleObj:
        *cptr++ = 'R';
        id = impl->getOID();
        break;

      case dbTechViaRuleObj:
        *cptr++ = 'y';
        id = impl->getOID();
        break;

      case dbTechViaGenerateRuleObj:
        *cptr++ = 'z';
        id = impl->getOID();
        break;

      case dbTechViaLayerRuleObj:
        *cptr++ = 'u';
        id = impl->getOID();
        break;

      case dbPropertyObj:
        *cptr++ = 'o';
        id = impl->getOID();
        break;
      case dbModuleObj:
        *cptr++ = 'm';
        id = impl->getOID();
        break;
      case dbModInstObj:
        *cptr++ = 'i';
        id = impl->getOID();
        break;
      case dbGroupObj:
        *cptr++ = 'b';
        id = impl->getOID();
        break;
      case dbAccessPointObj:
        *cptr++ = 'h';
        id = impl->getOID();
        break;
      case dbGuideObj:
        *cptr++ = ';';
        id = impl->getOID();
        break;
      case dbNetTrackObj:
        *cptr++ = '~';
        id = impl->getOID();
        break;
      case dbMetalWidthViaMapObj:
        *cptr++ = '^';
        id = impl->getOID();
        break;
      case dbTechLayerMinStepRuleObj:
      case dbTechLayerCornerSpacingRuleObj:
      case dbTechLayerSpacingTablePrlRuleObj:
      case dbTechLayerCutClassRuleObj:
      case dbTechLayerCutSpacingRuleObj:
      case dbTechLayerCutSpacingTableDefRuleObj:
      case dbTechLayerCutSpacingTableOrthRuleObj:
      case dbTechLayerCutEnclosureRuleObj:
      case dbTechLayerEolKeepOutRuleObj:
      case dbTechLayerEolExtensionRuleObj:
      case dbTechLayerArraySpacingRuleObj:
      case dbTechLayerWidthTableRuleObj:
      case dbTechLayerAreaRuleObj:
      case dbTechLayerKeepOutZoneRuleObj:
      case dbTechLayerMinCutRuleObj:
        *cptr++ = 'J';
        id = impl->getOID();
        break;

      case dbLogicPortObj:
      case dbPowerDomainObj:
      case dbPowerSwitchObj:
      case dbIsolationObj:
        *cptr++ = 'w';
        id = impl->getOID();
        break;

      case dbNameObj:
        getImpl()->getLogger()->critical(
            utl::ODB, 296, "dbNameObj not expected in getDbName");
        break;
    }

    snprintf(cptr, 10, "%u", id);
    cptr += strlen(cptr);
    *cptr++ = '/';
  }

  *--cptr = '\0';
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
                                 "dbGCellGrid",
                                 "dbGlobalConnect",
                                 "dbGroup",
                                 "dbGuide",
                                 "dbIsolation",
                                 "dbLogicPort",
                                 "dbMetalWidthViaMap",
                                 "dbModInst",
                                 "dbModule",
                                 "dbNetTrack",
                                 "dbPowerDomain",
                                 "dbPowerSwitch",
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
                                 "dbTechLayerKeepOutZoneRule",
                                 "dbTechLayerMinCutRule",
                                 "dbTechLayerMinStepRule",
                                 "dbTechLayerSpacingEolRule",
                                 "dbTechLayerSpacingTablePrlRule",
                                 "dbTechLayerWidthTableRule",
                                 // Generator Code End ObjectNames

                                 // Lib Objects
                                 "dbLib",
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

const char* dbObject::getObjName() const
{
  return name_tbl[getImpl()->getType()];
}

const char* dbObject::getObjName(dbObjectType type)
{
  return name_tbl[type];
}

}  // namespace odb
