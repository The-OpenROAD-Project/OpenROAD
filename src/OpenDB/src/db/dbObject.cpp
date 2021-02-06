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

#include <string.h>

#include "db.h"
#include "dbCore.h"
#include "dbDatabase.h"
#include "dbProperty.h"
#include "dbTable.h"

namespace odb {

uint dbObject::getId() const
{
  return getImpl()->getOID();
}

void dbObject::getDbName(char name[max_name_length]) const
{
  const dbObject*  parent;
  const dbObject*  child;
  const dbObject*  stack[16];
  const dbObject** end  = &stack[16];
  const dbObject** sptr = end;

  for (child = this; child; child = parent) {
    parent    = child->getImpl()->getOwner();
    *(--sptr) = child;
  }

  char* cptr = name;
  *cptr++    = '/';

  for (; sptr < end; ++sptr) {
    const dbObject* obj = *sptr;
    uint            id;

    const _dbObject* impl = getImpl();

    switch (impl->getType()) {
      case dbDatabaseObj: {
        *cptr++         = 'D';
        _dbDatabase* db = (_dbDatabase*) obj;
        id              = db->_unique_id;
        break;
      }

      // Design Objects
      case dbChipObj:
        *cptr++ = 'C';
        id      = impl->getOID();
        break;

      case dbBlockObj:
        *cptr++ = 'B';
        id      = impl->getOID();
        break;

      case dbInstHdrObj:
        // This object is not accessable from the API
        break;

      case dbInstObj:
        *cptr++ = 'I';
        id      = impl->getOID();
        break;

      case dbNetObj:
        *cptr++ = 'N';
        id      = impl->getOID();
        break;

      case dbBTermObj:
        *cptr++ = 'T';
        id      = impl->getOID();
        break;

      case dbITermObj:
        *cptr++ = 'Y';
        id      = impl->getOID();
        break;

      case dbBoxObj:
        *cptr++ = 'X';
        id      = impl->getOID();
        break;

      case dbViaObj:
        *cptr++ = 'V';
        id      = impl->getOID();
        break;

      case dbGCellGridObj:
        *cptr++ = 'G';
        id      = impl->getOID();
        break;

      case dbTrackGridObj:
        *cptr++ = 'K';
        id      = impl->getOID();
        break;

      case dbObstructionObj:
        *cptr++ = 'O';
        id      = impl->getOID();
        break;

      case dbBlockageObj:
        *cptr++ = 'E';
        id      = impl->getOID();
        break;

      case dbWireObj:
        *cptr++ = 'W';
        id      = impl->getOID();
        break;

      case dbSWireObj:
        *cptr++ = 'S';
        id      = impl->getOID();
        break;

      case dbSBoxObj:
        *cptr++ = 'Q';
        id      = impl->getOID();
        break;

      case dbCapNodeObj:  // DKF
        *cptr++ = 'n';
        id      = impl->getOID();
        break;

      case dbRSegObj:  // DKF
        *cptr++ = 's';
        id      = impl->getOID();
        break;

      case dbCCSegObj:
        *cptr++ = 'c';
        id      = impl->getOID();
        break;

      case dbRowObj:
        *cptr++ = 'x';
        id      = impl->getOID();
        break;

      case dbFillObj:
        *cptr++ = 'F';
        id      = impl->getOID();
        break;

      case dbRegionObj:
        *cptr++ = 'a';
        id      = impl->getOID();
        break;

      case dbHierObj:
        assert(0);  // hidden object....
        break;

      case dbBPinObj:
        *cptr++ = 'p';
        id      = impl->getOID();
        break;

      // Lib Objects
      case dbLibObj:
        *cptr++ = 'L';
        id      = impl->getOID();
        break;

      case dbSiteObj:
        *cptr++ = 'U';
        id      = impl->getOID();
        break;

      case dbMasterObj:
        *cptr++ = 'M';
        id      = impl->getOID();
        break;

      case dbMPinObj:
        *cptr++ = 'P';
        id      = impl->getOID();
        break;

      case dbMTermObj:
        *cptr++ = 'T';
        id      = impl->getOID();
        break;

      case dbTargetObj:
        *cptr++ = 't';
        id      = impl->getOID();
        break;

      case dbTechAntennaPinModelObj:
        *cptr++ = 'Z';
        id      = impl->getOID();
        break;

      // Tech Objects
      case dbTechObj:
        *cptr++ = 'H';
        id      = impl->getOID();
        break;

      case dbTechLayerObj:
        *cptr++ = 'A';
        id      = impl->getOID();
        break;

      case dbTechViaObj:
        *cptr++ = 'V';
        id      = impl->getOID();
        break;

      case dbTechNonDefaultRuleObj:
        *cptr++ = 'd';
        id      = impl->getOID();
        break;

      case dbTechLayerRuleObj:
        *cptr++ = 'l';
        id      = impl->getOID();
        break;

      case dbTechSameNetRuleObj:
        *cptr++ = 'q';
        id      = impl->getOID();
        break;

      case dbTechLayerSpacingRuleObj:
        *cptr++ = 'r';
        id      = impl->getOID();
        break;

      case dbTechLayerSpacingEolRuleObj:
        *cptr++ = 'k';
        id      = impl->getOID();
        break;

      case dbTechMinCutRuleObj:
        *cptr++ = 'e';
        id      = impl->getOID();
        break;

      case dbTechMinEncRuleObj:
        *cptr++ = 'f';
        id      = impl->getOID();
        break;

      case dbTechV55InfluenceEntryObj:
        *cptr++ = 'g';
        id      = impl->getOID();
        break;

      case dbTechLayerAntennaRuleObj:
        *cptr++ = 'R';
        id      = impl->getOID();
        break;

      case dbTechViaRuleObj:
        *cptr++ = 'y';
        id      = impl->getOID();
        break;

      case dbTechViaGenerateRuleObj:
        *cptr++ = 'z';
        id      = impl->getOID();
        break;

      case dbTechViaLayerRuleObj:
        *cptr++ = 'u';
        id      = impl->getOID();
        break;

      case dbPropertyObj:
        *cptr++ = 'o';
        id      = impl->getOID();
        break;
      case dbModuleObj:
        *cptr++ = 'm';
        id      = impl->getOID();
        break;
      case dbModInstObj:
        *cptr++ = 'i';
        id      = impl->getOID();
        break;
      case dbGroupObj:
        *cptr++ = 'b';
        id      = impl->getOID();
        break;
      case dbTechLayerMinStepRuleObj:
      case dbTechLayerCornerSpacingRuleObj:
      case dbTechLayerSpacingTablePrlRuleObj:
      case dbTechLayerCutClassRuleObj:
      case dbTechLayerCutSpacingRuleObj:
      case dbTechLayerCutSpacingTableDefRuleObj:
      case dbTechLayerCutSpacingTableOrthRuleObj:
        *cptr++ = 'J';
        id      = impl->getOID();
        break;

      case dbNameObj:
        assert(0);
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
  uint  id = strtoul(name, &end, 10);
  ZASSERT(name != end);
  name = end;
  return id;
}

dbObject* dbObject::resolveDbName(dbDatabase* db_, const char* name)
{
  ZASSERT(name[0] == '/');
  ++name;
  int       c;
  uint      oid;
  dbObject* obj = NULL;

  while ((c = *name++) != '\0') {
    switch (c) {
      case 'A':  // layer
        oid = getOid(name);
        obj = dbTechLayer::getTechLayer((dbTech*) obj, oid);
        break;

      case 'B':  // block
        oid = getOid(name);

        if (obj->getImpl()->getType() == dbBlockObj)
          obj = dbBlock::getBlock((dbBlock*) obj, oid);
        else
          obj = dbBlock::getBlock((dbChip*) obj, oid);

        break;

      case 'C':  // chip
        oid = getOid(name);
        obj = db_->getChip();
        break;

      case 'D':  // Database
        oid = getOid(name);
        ZASSERT(oid == (uint)((_dbDatabase*) db_)->_unique_id);
        obj = db_;
        break;

      case 'E':  // blockage
        oid = getOid(name);
        obj = dbBlockage::getBlockage((dbBlock*) obj, oid);
        break;

      case 'G':  // gcell
        oid = getOid(name);
        obj = dbGCellGrid::getGCellGrid((dbBlock*) obj, oid);
        break;

      case 'H':  // tech
        oid = getOid(name);
        obj = dbTech::getTech(db_, oid);
        break;

      case 'I':  // inst
        oid = getOid(name);
        obj = dbInst::getInst((dbBlock*) obj, oid);
        break;

      case 'K':  // track
        oid = getOid(name);
        obj = dbTrackGrid::getTrackGrid((dbBlock*) obj, oid);
        break;

      case 'L':  // lib
        oid = getOid(name);
        obj = dbLib::getLib(db_, oid);
        break;

      case 'M':  // master
        oid = getOid(name);
        obj = dbMaster::getMaster((dbLib*) obj, oid);
        break;

      case 'N':  // net or tnet
        oid = getOid(name);
        obj = dbNet::getNet((dbBlock*) obj, oid);
        break;

      case 'O':  // obstruction
        oid = getOid(name);
        obj = dbObstruction::getObstruction((dbBlock*) obj, oid);
        break;

      case 'P':  // mpin
        oid = getOid(name);
        obj = dbMPin::getMPin((dbMaster*) obj, oid);
        break;

      case 'Q':  // sbox
        oid = getOid(name);
        obj = dbSBox::getSBox((dbBlock*) obj, oid);
        break;

      case 'R':  // antenna rule
        oid = getOid(name);
        obj = dbTechLayerAntennaRule::getAntennaRule((dbTech*) obj, oid);
        break;

      case 'S':  // swire
        oid = getOid(name);
        obj = dbSWire::getSWire((dbBlock*) obj, oid);
        break;

      case 'T':  // bterm, mterm
        oid = getOid(name);

        if (obj->getImpl()->getType() == dbBlockObj)
          obj = dbBTerm::getBTerm((dbBlock*) obj, oid);
        else
          obj = dbMTerm::getMTerm((dbMaster*) obj, oid);

        break;

      case 'U':  // site
        oid = getOid(name);
        obj = dbSite::getSite((dbLib*) obj, oid);
        break;

      case 'V':  // block-via, or tech-via
        oid = getOid(name);

        if (obj->getImpl()->getType() == dbBlockObj)
          obj = dbVia::getVia((dbBlock*) obj, oid);
        else
          obj = dbTechVia::getTechVia((dbTech*) obj, oid);
        break;

      case 'W':  // wire
        oid = getOid(name);
        obj = dbWire::getWire((dbBlock*) obj, oid);
        break;

      case 'X':  // box
      {
        oid               = getOid(name);
        dbObjectType type = obj->getImpl()->getType();

        if (type == dbBlockObj)
          obj = dbBox::getBox((dbBlock*) obj, oid);
        else if (type == dbTechObj)
          obj = dbBox::getBox((dbTech*) obj, oid);
        else if (type == dbMasterObj)
          obj = dbBox::getBox((dbMaster*) obj, oid);

        break;
      }

      case 'Y':  // iterm
        oid = getOid(name);
        obj = dbITerm::getITerm((dbBlock*) obj, oid);
        break;

      case 'Z':  // antenna pin model
        oid = getOid(name);
        obj = dbTechAntennaPinModel::getAntennaPinModel((dbMaster*) obj, oid);
        break;

      case 'a':  // region
        oid = getOid(name);
        obj = dbRegion::getRegion((dbBlock*) obj, oid);
        break;

      case 'c':  // ccseg
        oid = getOid(name);
        obj = dbCCSeg::getCCSeg((dbBlock*) obj, oid);
        break;

      case 'd':  // tech-non-default-rule
        oid = getOid(name);
        obj = dbTechNonDefaultRule::getTechNonDefaultRule((dbTech*) obj, oid);
        break;

      case 'e':  // tech-min-cut-rule
        oid = getOid(name);
        obj = dbTechMinCutRule::getMinCutRule((dbTechLayer*) obj, oid);
        break;

      case 'f':  // tech-min-enc-rule
        oid = getOid(name);
        obj = dbTechMinEncRule::getMinEncRule((dbTechLayer*) obj, oid);
        break;

      case 'g':  // tech-min-influence-entry
        oid = getOid(name);
        obj = dbTechV55InfluenceEntry::getV55InfluenceEntry((dbTechLayer*) obj,
                                                            oid);
        break;

      case 'l':  // tech-layer-rule
        oid = getOid(name);
        obj = dbTechLayerRule::getTechLayerRule((dbTech*) obj, oid);
        break;

      case 'n':  // rcseg
        oid = getOid(name);
        obj = dbCapNode::getCapNode((dbBlock*) obj, oid);
        break;

      case 'p':  // bpin
        oid = getOid(name);
        obj = dbBPin::getBPin((dbBlock*) obj, oid);
        break;

      case 'o':  // property
      {
        oid                             = getOid(name);
        dbTable<_dbProperty>* propTable = _dbProperty::getPropTable(obj);
        obj                             = propTable->getPtr(oid);
        break;
      }

      case 'q':  // samenet
        oid = getOid(name);
        obj = dbTechSameNetRule::getTechSameNetRule((dbTech*) obj, oid);
        break;

      case 'r':  // spacing-rule
        oid = getOid(name);
        obj = dbTechLayerSpacingRule::getTechLayerSpacingRule(
            (dbTechLayer*) obj, oid);
        break;

      case 'k':  // spacing eol rule
        oid = getOid(name);
        obj = dbTechLayerSpacingEolRule::getTechLayerSpacingEolRule(
            (dbTechLayer*) obj, oid);
        break;

      case 's':  // rseg
        oid = getOid(name);
        obj = dbRSeg::getRSeg((dbBlock*) obj, oid);
        break;

      case 't':  // target
        oid = getOid(name);
        obj = dbTarget::getTarget((dbMaster*) obj, oid);
        break;

      case 'u':  // via-rule
        oid = getOid(name);
        obj = dbTechViaLayerRule::getTechViaLayerRule((dbTech*) obj, oid);
        break;

      case 'x':  // row
        oid = getOid(name);
        obj = dbRow::getRow((dbBlock*) obj, oid);
        break;

      case 'F':  // fill
        oid = getOid(name);
        obj = dbFill::getFill((dbBlock*) obj, oid);
        break;

      case 'y':  // via-rule
        oid = getOid(name);
        obj = dbTechViaRule::getTechViaRule((dbTech*) obj, oid);
        break;

      case 'z':  // via-rule-generate
        oid = getOid(name);
        obj = dbTechViaGenerateRule::getTechViaGenerateRule((dbTech*) obj, oid);
        break;

      case 'm':  // Module
        oid = getOid(name);
        obj = dbModule::getModule((dbBlock*) obj, oid);
        break;

      case 'i':  // ModInst
        oid = getOid(name);
        obj = dbModInst::getModInst((dbBlock*) obj, oid);
        break;

      case 'b':  // Group
        oid = getOid(name);
        obj = dbGroup::getGroup((dbBlock*) obj, oid);
        break;
      case 'J':
        // SKIP
        break;

      case '/':
        break;
    }
  }

  ZASSERT(obj != NULL);
  return obj;
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
                                 "dbGCellGrid",
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
                                 // Generator Code Begin 1
                                 "dbTechLayer",
                                 "dbTechLayerSpacingEolRule",
                                 "dbTechLayerMinStepRule",
                                 "dbTechLayerCornerSpacingRule",
                                 "dbTechLayerSpacingTablePrlRule",
                                 "dbTechLayerCutClassRule",
                                 "dbTechLayerCutSpacingRule",
                                 "dbTechLayerCutSpacingTableOrthRule",
                                 "dbTechLayerCutSpacingTableDefRule",
                                 "dbModule",
                                 "dbModInst",
                                 "dbGroup",
                                 // Generator Code End 1

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
