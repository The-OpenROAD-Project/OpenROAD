// *****************************************************************************
// *****************************************************************************
// Copyright 2013 - 2017, Cadence Design Systems
// 
// This  file  is  part  of  the  Cadence  LEF/DEF  Open   Source
// Distribution,  Product Version 5.8. 
// 
// Licensed under the Apache License, Version 2.0 (the "License");
//    you may not use this file except in compliance with the License.
//    You may obtain a copy of the License at
// 
//        http://www.apache.org/licenses/LICENSE-2.0
// 
//    Unless required by applicable law or agreed to in writing, software
//    distributed under the License is distributed on an "AS IS" BASIS,
//    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
//    implied. See the License for the specific language governing
//    permissions and limitations under the License.
// 
// For updates, support, or to become part of the LEF/DEF Community,
// check www.openeda.org for details.
// 
//  $Author: icftcm $
//  $Revision: #2 $
//  $Date: 2017/06/07 $
//  $State:  $
// *****************************************************************************
// *****************************************************************************

#include "defrReader.hpp"
#include "defiProp.hpp"
#include "defiPropType.hpp"
#include "defrCallBacks.hpp"
#include "defiDebug.hpp"
#include "defiMisc.hpp"
#include "defrData.hpp"
#include "defrSettings.hpp"

#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include "lex.h"
#include "defiUtil.hpp"
#include "defrCallBacks.hpp"

#define NODEFMSG 4013     // (9012 + 1) - 5000, def msg starts at 5000

# define DEF_INIT def_init(__FUNCTION__)


BEGIN_LEFDEF_PARSER_NAMESPACE

extern defrContext defContext;

void
def_init(const char  *func)
{
    // Need for debugging config re-owning;
    if (defContext.ownConfig) {
        return;
    }


    if (defContext.settings == NULL) {
        defContext.settings = new defrSettings();
        defContext.init_call_func = func;
    }

    if (defContext.callbacks == NULL) {
        defContext.callbacks = new defrCallbacks();
        defContext.init_call_func = func;
    }

    if (defContext.session == NULL) {
        defContext.session = new defrSession();
        defContext.init_call_func = func;
    }
}


int
defrCountUnused(defrCallbackType_e  e,
                void                *v,
                defiUserData        d)
{
    DEF_INIT;
    int i;
    if (defiDebug(23))
        printf("Count %d, 0x%p, 0x%p\n", (int) e, v, d);
    i = (int) e;
    if (i <= 0 || i >= CBMAX) {
        return 1;
    }

    defContext.settings->UnusedCallbacks[i] += 1;

    return 0;
}


const char *
typeToString(defrCallbackType_e num)
{
    switch ((int) num) {

    case defrUnspecifiedCbkType:
        return "Unspecified";
    case defrDesignStartCbkType:
        return "Design Start";
    case defrTechNameCbkType:
        return "Tech Name";
    case defrPropCbkType:
        return "Property";
    case defrPropDefEndCbkType:
        return "Property Definitions Section End";
    case defrPropDefStartCbkType:
        return "Property Definitions Section Start";
    case defrFloorPlanNameCbkType:
        return "FloorPlanName";
    case defrArrayNameCbkType:
        return "Array Name";
    case defrUnitsCbkType:
        return "Units";
    case defrDividerCbkType:
        return "Divider";
    case defrBusBitCbkType:
        return "BusBit Character";
    case defrSiteCbkType:
        return "Site";
    case defrComponentMaskShiftLayerCbkType:
        return "ComponentMaskShiftLayer";
    case defrComponentStartCbkType:
        return "Components Section Start";
    case defrComponentCbkType:
        return "Component";
    case defrComponentEndCbkType:
        return "Components Section End";
    case defrNetStartCbkType:
        return "Nets Section Start";
    case defrNetCbkType:
        return "Net";
    case defrNetNameCbkType:
        return "Net Name";
    case defrNetNonDefaultRuleCbkType:
        return "Net Nondefaultrule";
    case defrNetSubnetNameCbkType:
        return "Net Subnet Name";
    case defrNetEndCbkType:
        return "Nets Section End";
    case defrPathCbkType:
        return "Path";
    case defrVersionCbkType:
        return "Version";
    case defrVersionStrCbkType:
        return "Version";
    case defrComponentExtCbkType:
        return "Component User Extention";
    case defrPinExtCbkType:
        return "Pin User Extension";
    case defrViaExtCbkType:
        return "Via User Extension";
    case defrNetConnectionExtCbkType:
        return "NetConnection User Extention";
    case defrNetExtCbkType:
        return "Net User Extension";
    case defrGroupExtCbkType:
        return "Group User Extension";
    case defrScanChainExtCbkType:
        return "ScanChain User Extension";
    case defrIoTimingsExtCbkType:
        return "IoTimings User Extension";
    case defrPartitionsExtCbkType:
        return "Partitions User Extension";
    case defrHistoryCbkType:
        return "History";
    case defrDieAreaCbkType:
        return "DieArea";
    case defrCanplaceCbkType:
        return "Canplace";
    case defrCannotOccupyCbkType:
        return "CannotOccupy";
    case defrPinCapCbkType:
        return "PinCap";
    case defrDefaultCapCbkType:
        return "DefaultCap";
    case defrStartPinsCbkType:
        return "Start Pins Section";
    case defrPinCbkType:
        return "Pin";
    case defrPinEndCbkType:
        return "End Pins Section";
    case defrRowCbkType:
        return "Row";
    case defrTrackCbkType:
        return "Track";
    case defrGcellGridCbkType:
        return "GcellGrid";
    case defrViaStartCbkType:
        return "Start Vias Section";
    case defrViaCbkType:
        return "Via";
    case defrViaEndCbkType:
        return "End Vias Section";
    case defrRegionStartCbkType:
        return "Region Section Start";
    case defrRegionCbkType:
        return "Region";
    case defrRegionEndCbkType:
        return "Region Section End";
    case defrSNetStartCbkType:
        return "Special Net Section Start";
    case defrSNetCbkType:
        return "Special Net";
    case defrSNetEndCbkType:
        return "Special Net Section End";
    case defrGroupsStartCbkType:
        return "Groups Section Start";
    case defrGroupNameCbkType:
        return "Group Name";
    case defrGroupMemberCbkType:
        return "Group Member";
    case defrGroupCbkType:
        return "Group";
    case defrGroupsEndCbkType:
        return "Groups Section End";
    case defrAssertionsStartCbkType:
        return "Assertions Section Start";
    case defrAssertionCbkType:
        return "Assertion";
    case defrAssertionsEndCbkType:
        return "Assertions Section End";
    case defrConstraintsStartCbkType:
        return "Constraints Section Start";
    case defrConstraintCbkType:
        return "Constraint";
    case defrConstraintsEndCbkType:
        return "Constraints Section End";
    case defrScanchainsStartCbkType:
        return "Scanchains Section Start";
    case defrScanchainCbkType:
        return "Scanchain";
    case defrScanchainsEndCbkType:
        return "Scanchains Section End";
    case defrIOTimingsStartCbkType:
        return "IOTimings Section Start";
    case defrIOTimingCbkType:
        return "IOTiming";
    case defrIOTimingsEndCbkType:
        return "IOTimings Section End";
    case defrFPCStartCbkType:
        return "Floor Plan Constraints Section Start";
    case defrFPCCbkType:
        return "Floor Plan Constraint";
    case defrFPCEndCbkType:
        return "Floor Plan Constraints Section End";
    case defrTimingDisablesStartCbkType:
        return "TimingDisables Section Start";
    case defrTimingDisableCbkType:
        return "TimingDisable";
    case defrTimingDisablesEndCbkType:
        return "TimingDisables Section End";
    case defrPartitionsStartCbkType:
        return "Partitions Section Start";
    case defrPartitionCbkType:
        return "Partition";
    case defrPartitionsEndCbkType:
        return "Partitions Section End";
    case defrPinPropStartCbkType:
        return "PinProp Section Start";
    case defrPinPropCbkType:
        return "PinProp";
    case defrPinPropEndCbkType:
        return "PinProp Section End";
    case defrCaseSensitiveCbkType:
        return "CaseSensitive";
    case defrBlockageStartCbkType:
        return "Blockage Section Start";
    case defrBlockageCbkType:
        return "Blockage";
    case defrBlockageEndCbkType:
        return "Blockage Section End";
    case defrSlotStartCbkType:
        return "Slots Section Start";
    case defrSlotCbkType:
        return "Slots";
    case defrSlotEndCbkType:
        return "Slots Section End";
    case defrFillStartCbkType:
        return "Fills Section Start";
    case defrFillCbkType:
        return "Fills";
    case defrFillEndCbkType:
        return "Fills Section End";
    case defrNonDefaultStartCbkType:
        return "NonDefaultRule Section Start";
    case defrNonDefaultCbkType:
        return "NonDefault";
    case defrNonDefaultEndCbkType:
        return "NonDefaultRule Section End";
    case defrStylesStartCbkType:
        return "Styles Section Start";
    case defrStylesCbkType:
        return "Styles";
    case defrStylesEndCbkType:
        return "Styles Section End";
    case defrExtensionCbkType:
        return "Extension";

        // NEW CALLBACK - If you created a new callback then add the
        // type enums that you created here for debug printing.     

    case defrDesignEndCbkType:
        return "DesignEnd";
    default:
        break;
    }
    return "BOGUS";
}

int
defrCatchAll(defrCallbackType_e typ, void*, defiUserData)
{
    DEF_INIT;

    if ((int) typ >= 0 && (int) typ < CBMAX) {
        defContext.settings->UnusedCallbacks[(int) typ] += 1;
    } else {
        defContext.settings->UnusedCallbacks[0] += 1;
        return 1;
    }

    return 0;
}

// *****************************************************************
// Wrapper functions.
//
// These functions provide access to the class member functions
// for compatibility with previous parser kits. Returns non-zero
// status if the initialization is failed. 
// *****************************************************************
int 
defrInit()
{
    return defrInitSession(0);
}

int
defrInitSession(int startSession)
{
    if (startSession) { 
        if (defContext.init_call_func != NULL) {
            fprintf(stderr, "ERROR: Attempt to call configuration function '%s' in DEF parser before defrInit() call in session-based mode.\n", defContext.init_call_func);
            return 1;
        }

        delete defContext.settings;
        defContext.settings = new defrSettings();

        delete defContext.callbacks;
        defContext.callbacks = new defrCallbacks();

        delete defContext.session;
        defContext.session = new defrSession();
    } else {
        if (defContext.callbacks == NULL) {
            defContext.callbacks = new defrCallbacks();
        }
    
        if (defContext.settings == NULL) {
            defContext.settings = new defrSettings();
        }

        if (defContext.session == NULL) {
            defContext.session = new defrSession();
        } else {
            memset(defContext.settings->UnusedCallbacks, 0, CBMAX * sizeof(int));
        }
    }

    defContext.ownConfig = 0;
    defContext.init_call_func = 0;

    return 0;
}

// obsoleted now 
int
defrReset()
{
    return 0;
}

int 
defrClear()
{
    delete defContext.callbacks;
    defContext.callbacks = NULL;

    delete defContext.settings;
    defContext.settings = NULL;

    delete defContext.session;
    defContext.session = NULL;

    delete defContext.data;
    defContext.data = NULL;

    defContext.init_call_func = NULL;
    defContext.ownConfig = 0;

    return 0;
}


void
defrSetRegisterUnusedCallbacks()
{
    DEF_INIT;
    defrSetUnusedCallbacks(defrCountUnused);
}

void
defrSetUnusedCallbacks(defrVoidCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->SetUnusedCallbacks(f);
}

void
defrUnsetCallbacks()
{
    DEF_INIT;
    delete defContext.callbacks;
    defContext.callbacks = new defrCallbacks();
}

void
defrPrintUnusedCallbacks(FILE *log)
{
    int i;
    int first = 1;

    for (i = 0; i < CBMAX; i++) {
        if (defContext.settings->UnusedCallbacks[i]) {
            if (first) {
                fprintf(log,
                        "WARNING (DEFPARS-5001): DEF statement found in the def file with no callback set.\n");
                first = 0;
            }
            fprintf(log, "%5d %s\n", defContext.settings->UnusedCallbacks[i],
                    typeToString((defrCallbackType_e) i));
        }
    }
}

// obsoleted
int
defrReleaseNResetMemory()
{
    return 0;
}

void
defrUnsetArrayNameCbk()
{
    DEF_INIT;
    defContext.callbacks->ArrayNameCbk = NULL;
}

void
defrUnsetAssertionCbk()
{
    DEF_INIT;
    defContext.callbacks->AssertionCbk = NULL;
}

void
defrUnsetAssertionsStartCbk()
{
    DEF_INIT;
    defContext.callbacks->AssertionsStartCbk = NULL;
}

void
defrUnsetAssertionsEndCbk()
{
    DEF_INIT;
    defContext.callbacks->AssertionsEndCbk = NULL;
}

void
defrUnsetBlockageCbk()
{
    DEF_INIT;
    defContext.callbacks->BlockageCbk = NULL;
}

void
defrUnsetBlockageStartCbk()
{
    DEF_INIT;
    defContext.callbacks->BlockageStartCbk = NULL;
}

void
defrUnsetBlockageEndCbk()
{
    DEF_INIT;
    defContext.callbacks->BlockageEndCbk = NULL;
}

void
defrUnsetBusBitCbk()
{
    DEF_INIT;
    defContext.callbacks->BusBitCbk = NULL;
}

void
defrUnsetCannotOccupyCbk()
{
    DEF_INIT;
    defContext.callbacks->CannotOccupyCbk = NULL;
}

void
defrUnsetCanplaceCbk()
{
    DEF_INIT;
    defContext.callbacks->CanplaceCbk = NULL;
}

void
defrUnsetCaseSensitiveCbk()
{
    DEF_INIT;
    defContext.callbacks->CaseSensitiveCbk = NULL;
}

void
defrUnsetComponentCbk()
{
    DEF_INIT;
    defContext.callbacks->ComponentCbk = NULL;
}

void
defrUnsetComponentExtCbk()
{
    DEF_INIT;
    defContext.callbacks->ComponentExtCbk = NULL;
}

void
defrUnsetComponentStartCbk()
{
    DEF_INIT;
    defContext.callbacks->ComponentStartCbk = NULL;
}

void
defrUnsetComponentEndCbk()
{
    DEF_INIT;
    defContext.callbacks->ComponentEndCbk = NULL;
}

void
defrUnsetConstraintCbk()
{
    DEF_INIT;
    defContext.callbacks->ConstraintCbk = NULL;
}

void
defrUnsetConstraintsStartCbk()
{
    DEF_INIT;
    defContext.callbacks->ConstraintsStartCbk = NULL;
}

void
defrUnsetConstraintsEndCbk()
{
    DEF_INIT;
    defContext.callbacks->ConstraintsEndCbk = NULL;
}

void
defrUnsetDefaultCapCbk()
{
    DEF_INIT;
    defContext.callbacks->DefaultCapCbk = NULL;
}

void
defrUnsetDesignCbk()
{
    DEF_INIT;
    defContext.callbacks->DesignCbk = NULL;
}

void
defrUnsetDesignEndCbk()
{
    DEF_INIT;
    defContext.callbacks->DesignEndCbk = NULL;
}

void
defrUnsetDieAreaCbk()
{
    DEF_INIT;
    defContext.callbacks->DieAreaCbk = NULL;
}

void
defrUnsetDividerCbk()
{
    DEF_INIT;
    defContext.callbacks->DividerCbk = NULL;
}

void
defrUnsetExtensionCbk()
{
    DEF_INIT;
    defContext.callbacks->ExtensionCbk = NULL;
}

void
defrUnsetFillCbk()
{
    DEF_INIT;
    defContext.callbacks->FillCbk = NULL;
}

void
defrUnsetFillStartCbk()
{
    DEF_INIT;
    defContext.callbacks->FillStartCbk = NULL;
}

void
defrUnsetFillEndCbk()
{
    DEF_INIT;
    defContext.callbacks->FillEndCbk = NULL;
}

void
defrUnsetFPCCbk()
{
    DEF_INIT;
    defContext.callbacks->FPCCbk = NULL;
}

void
defrUnsetFPCStartCbk()
{
    DEF_INIT;
    defContext.callbacks->FPCStartCbk = NULL;
}

void
defrUnsetFPCEndCbk()
{
    DEF_INIT;
    defContext.callbacks->FPCEndCbk = NULL;
}

void
defrUnsetFloorPlanNameCbk()
{
    DEF_INIT;
    defContext.callbacks->FloorPlanNameCbk = NULL;
}

void
defrUnsetGcellGridCbk()
{
    DEF_INIT;
    defContext.callbacks->GcellGridCbk = NULL;
}

void
defrUnsetGroupCbk()
{
    DEF_INIT;
    defContext.callbacks->GroupCbk = NULL;
}

void
defrUnsetGroupExtCbk()
{
    DEF_INIT;
    defContext.callbacks->GroupExtCbk = NULL;
}

void
defrUnsetGroupMemberCbk()
{
    DEF_INIT;
    defContext.callbacks->GroupMemberCbk = NULL;
}

void
defrUnsetComponentMaskShiftLayerCbk()
{
    DEF_INIT;
    defContext.callbacks->ComponentMaskShiftLayerCbk = NULL;
}

void
defrUnsetGroupNameCbk()
{
    DEF_INIT;
    defContext.callbacks->GroupNameCbk = NULL;
}

void
defrUnsetGroupsStartCbk()
{
    DEF_INIT;
    defContext.callbacks->GroupsStartCbk = NULL;
}

void
defrUnsetGroupsEndCbk()
{
    DEF_INIT;
    defContext.callbacks->GroupsEndCbk = NULL;
}

void
defrUnsetHistoryCbk()
{
    DEF_INIT;
    defContext.callbacks->HistoryCbk = NULL;
}

void
defrUnsetIOTimingCbk()
{
    DEF_INIT;
    defContext.callbacks->IOTimingCbk = NULL;
}

void
defrUnsetIOTimingsStartCbk()
{
    DEF_INIT;
    defContext.callbacks->IOTimingsStartCbk = NULL;
}

void
defrUnsetIOTimingsEndCbk()
{
    DEF_INIT;
    defContext.callbacks->IOTimingsEndCbk = NULL;
}

void
defrUnsetIOTimingsExtCbk()
{
    DEF_INIT;
    defContext.callbacks->IoTimingsExtCbk = NULL;
}

void
defrUnsetNetCbk()
{
    DEF_INIT;
    defContext.callbacks->NetCbk = NULL;
}

void
defrUnsetNetNameCbk()
{
    DEF_INIT;
    defContext.callbacks->NetNameCbk = NULL;
}

void
defrUnsetNetNonDefaultRuleCbk()
{
    DEF_INIT;
    defContext.callbacks->NetNonDefaultRuleCbk = NULL;
}

void
defrUnsetNetConnectionExtCbk()
{
    DEF_INIT;
    defContext.callbacks->NetConnectionExtCbk = NULL;
}

void
defrUnsetNetExtCbk()
{
    DEF_INIT;
    defContext.callbacks->NetExtCbk = NULL;
}

void
defrUnsetNetPartialPathCbk()
{
    DEF_INIT;
    defContext.callbacks->NetPartialPathCbk = NULL;
}

void
defrUnsetNetSubnetNameCbk()
{
    DEF_INIT;
    defContext.callbacks->NetSubnetNameCbk = NULL;
}

void
defrUnsetNetStartCbk()
{
    DEF_INIT;
    defContext.callbacks->NetStartCbk = NULL;
}

void
defrUnsetNetEndCbk()
{
    DEF_INIT;
    defContext.callbacks->NetEndCbk = NULL;
}

void
defrUnsetNonDefaultCbk()
{
    DEF_INIT;
    defContext.callbacks->NonDefaultCbk = NULL;
}

void
defrUnsetNonDefaultStartCbk()
{
    DEF_INIT;
    defContext.callbacks->NonDefaultStartCbk = NULL;
}

void
defrUnsetNonDefaultEndCbk()
{
    DEF_INIT;
    defContext.callbacks->NonDefaultEndCbk = NULL;
}

void
defrUnsetPartitionCbk()
{
    DEF_INIT;
    defContext.callbacks->PartitionCbk = NULL;
}

void
defrUnsetPartitionsExtCbk()
{
    DEF_INIT;
    defContext.callbacks->PartitionsExtCbk = NULL;
}

void
defrUnsetPartitionsStartCbk()
{
    DEF_INIT;
    defContext.callbacks->PartitionsStartCbk = NULL;
}

void
defrUnsetPartitionsEndCbk()
{
    DEF_INIT;
    defContext.callbacks->PartitionsEndCbk = NULL;
}

void
defrUnsetPathCbk()
{
    DEF_INIT;
    defContext.callbacks->PathCbk = NULL;
}

void
defrUnsetPinCapCbk()
{
    DEF_INIT;
    defContext.callbacks->PinCapCbk = NULL;
}

void
defrUnsetPinCbk()
{
    DEF_INIT;
    defContext.callbacks->PinCbk = NULL;
}

void
defrUnsetPinEndCbk()
{
    DEF_INIT;
    defContext.callbacks->PinEndCbk = NULL;
}

void
defrUnsetPinExtCbk()
{
    DEF_INIT;
    defContext.callbacks->PinExtCbk = NULL;
}

void
defrUnsetPinPropCbk()
{
    DEF_INIT;
    defContext.callbacks->PinPropCbk = NULL;
}

void
defrUnsetPinPropStartCbk()
{
    DEF_INIT;
    defContext.callbacks->PinPropStartCbk = NULL;
}

void
defrUnsetPinPropEndCbk()
{
    DEF_INIT;
    defContext.callbacks->PinPropEndCbk = NULL;
}

void
defrUnsetPropCbk()
{
    DEF_INIT;
    defContext.callbacks->PropCbk = NULL;
}

void
defrUnsetPropDefEndCbk()
{
    DEF_INIT;
    defContext.callbacks->PropDefEndCbk = NULL;
}

void
defrUnsetPropDefStartCbk()
{
    DEF_INIT;
    defContext.callbacks->PropDefStartCbk = NULL;
}

void
defrUnsetRegionCbk()
{
    DEF_INIT;
    defContext.callbacks->RegionCbk = NULL;
}

void
defrUnsetRegionStartCbk()
{
    DEF_INIT;
    defContext.callbacks->RegionStartCbk = NULL;
}

void
defrUnsetRegionEndCbk()
{
    DEF_INIT;
    defContext.callbacks->RegionEndCbk = NULL;
}

void
defrUnsetRowCbk()
{
    DEF_INIT;
    defContext.callbacks->RowCbk = NULL;
}

void
defrUnsetScanChainExtCbk()
{
    DEF_INIT;
    defContext.callbacks->ScanChainExtCbk = NULL;
}

void
defrUnsetScanchainCbk()
{
    DEF_INIT;
    defContext.callbacks->ScanchainCbk = NULL;
}

void
defrUnsetScanchainsStartCbk()
{
    DEF_INIT;
    defContext.callbacks->ScanchainsStartCbk = NULL;
}

void
defrUnsetScanchainsEndCbk()
{
    DEF_INIT;
    defContext.callbacks->ScanchainsEndCbk = NULL;
}

void
defrUnsetSiteCbk()
{
    DEF_INIT;
    defContext.callbacks->SiteCbk = NULL;
}

void
defrUnsetSlotCbk()
{
    DEF_INIT;
    defContext.callbacks->SlotCbk = NULL;
}

void
defrUnsetSlotStartCbk()
{
    DEF_INIT;
    defContext.callbacks->SlotStartCbk = NULL;
}

void
defrUnsetSlotEndCbk()
{
    DEF_INIT;
    defContext.callbacks->SlotEndCbk = NULL;
}

void
defrUnsetSNetWireCbk()
{
    DEF_INIT;
    defContext.callbacks->SNetWireCbk = NULL;
}

void
defrUnsetSNetCbk()
{
    DEF_INIT;
    defContext.callbacks->SNetCbk = NULL;
}

void
defrUnsetSNetStartCbk()
{
    DEF_INIT;
    defContext.callbacks->SNetStartCbk = NULL;
}

void
defrUnsetSNetEndCbk()
{
    DEF_INIT;
    defContext.callbacks->SNetEndCbk = NULL;
}

void
defrUnsetSNetPartialPathCbk()
{
    DEF_INIT;
    defContext.callbacks->SNetPartialPathCbk = NULL;
}

void
defrUnsetStartPinsCbk()
{
    DEF_INIT;
    defContext.callbacks->StartPinsCbk = NULL;
}

void
defrUnsetStylesCbk()
{
    DEF_INIT;
    defContext.callbacks->StylesCbk = NULL;
}

void
defrUnsetStylesStartCbk()
{
    DEF_INIT;
    defContext.callbacks->StylesStartCbk = NULL;
}

void
defrUnsetStylesEndCbk()
{
    DEF_INIT;
    defContext.callbacks->StylesEndCbk = NULL;
}

void
defrUnsetTechnologyCbk()
{
    DEF_INIT;
    defContext.callbacks->TechnologyCbk = NULL;
}

void
defrUnsetTimingDisableCbk()
{
    DEF_INIT;
    defContext.callbacks->TimingDisableCbk = NULL;
}

void
defrUnsetTimingDisablesStartCbk()
{
    DEF_INIT;
    defContext.callbacks->TimingDisablesStartCbk = NULL;
}

void
defrUnsetTimingDisablesEndCbk()
{
    DEF_INIT;
    defContext.callbacks->TimingDisablesEndCbk = NULL;
}

void
defrUnsetTrackCbk()
{
    DEF_INIT;
    defContext.callbacks->TrackCbk = NULL;
}

void
defrUnsetUnitsCbk()
{
    DEF_INIT;
    defContext.callbacks->UnitsCbk = NULL;
}

void
defrUnsetVersionCbk()
{
    DEF_INIT;
    defContext.callbacks->VersionCbk = NULL;
}

void
defrUnsetVersionStrCbk()
{
    DEF_INIT;
    defContext.callbacks->VersionStrCbk = NULL;
}

void
defrUnsetViaCbk()
{
    DEF_INIT;
    defContext.callbacks->ViaCbk = NULL;
}

void
defrUnsetViaExtCbk()
{
    DEF_INIT;
    defContext.callbacks->ViaExtCbk = NULL;
}

void
defrUnsetViaStartCbk()
{
    DEF_INIT;
    defContext.callbacks->ViaStartCbk = NULL;
}


void
defrUnsetViaEndCbk()
{
    DEF_INIT;
    defContext.callbacks->ViaEndCbk = NULL;
}

int *
defUnusedCallbackCount()
{
    DEF_INIT;
    return defContext.settings->UnusedCallbacks;
}


const char *
defrFName()
{
    DEF_INIT;
    return NULL;
}

void 
defrClearSession()
{
    if (defContext.session) {
        delete defContext.session;
        defContext.session = new defrSession();
    }
}

int
defrRead(FILE           *f,
         const char     *fName,
         defiUserData   uData,
         int            case_sensitive)
{

    int status;

    delete defContext.data;

    defrData *defData = new defrData(defContext.callbacks, 
                                     defContext.settings, 
                                     defContext.session);

    defContext.data = defData;

    // lex_init
    struct stat statbuf;

    /* 4/11/2003 - Remove file lefrRWarning.log from directory if it exist */
    /* pcr 569729 */
    if (stat("defRWarning.log", &statbuf) != -1) {
        /* file exist, remove it */
        if (!defContext.settings->LogFileAppend) {
            remove("defRWarning.log");
        }
    }

    // Propagate Settings parameter to Data.
    if (defData->settings->reader_case_sensitive_set) {
        defData->names_case_sensitive = defData->session->reader_case_sensitive;
    } else if (defData->VersionNum > 5.5) {
        defData->names_case_sensitive = true;
    }

    defData->session->FileName = (char*) fName;
    defData->File = f;
    defData->session->UserData = uData;
    defData->session->reader_case_sensitive = case_sensitive;

    // Create a path pointer that is all ready to go just in case
    // we need it later.

    defData->NeedPathData = (
        ((defData->callbacks->NetCbk || defData->callbacks->SNetCbk) && defData->settings->AddPathToNet) || defData->callbacks->PathCbk) ? 1 : 0;
    if (defData->NeedPathData) {
        defData->PathObj.Init();
    }

    status = defyyparse(defData);

    return status;
}

void
defrSetUserData(defiUserData ud)
{
    DEF_INIT;
    defContext.session->UserData = ud;
}


defiUserData
defrGetUserData()
{
    return defContext.session->UserData;
}


void
defrSetDesignCbk(defrStringCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->DesignCbk = f;
}


void
defrSetTechnologyCbk(defrStringCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->TechnologyCbk = f;
}


void
defrSetDesignEndCbk(defrVoidCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->DesignEndCbk = f;
}


void
defrSetPropCbk(defrPropCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->PropCbk = f;
}


void
defrSetPropDefEndCbk(defrVoidCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->PropDefEndCbk = f;
}


void
defrSetPropDefStartCbk(defrVoidCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->PropDefStartCbk = f;
}


void
defrSetArrayNameCbk(defrStringCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->ArrayNameCbk = f;
}


void
defrSetFloorPlanNameCbk(defrStringCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->FloorPlanNameCbk = f;
}


void
defrSetUnitsCbk(defrDoubleCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->UnitsCbk = f;
}


void
defrSetVersionCbk(defrDoubleCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->VersionCbk = f;
}


void
defrSetVersionStrCbk(defrStringCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->VersionStrCbk = f;
}


void
defrSetDividerCbk(defrStringCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->DividerCbk = f;
}


void
defrSetBusBitCbk(defrStringCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->BusBitCbk = f;
}


void
defrSetSiteCbk(defrSiteCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->SiteCbk = f;
}


void
defrSetCanplaceCbk(defrSiteCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->CanplaceCbk = f;
}


void
defrSetCannotOccupyCbk(defrSiteCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->CannotOccupyCbk = f;
}


void
defrSetComponentStartCbk(defrIntegerCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->ComponentStartCbk = f;
}


void
defrSetComponentEndCbk(defrVoidCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->ComponentEndCbk = f;
}

void
defrSetComponentCbk(defrComponentCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->ComponentCbk = f;
}

void
defrSetNetStartCbk(defrIntegerCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->NetStartCbk = f;
}

void
defrSetNetEndCbk(defrVoidCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->NetEndCbk = f;
}

void
defrSetNetCbk(defrNetCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->NetCbk = f;
}

void
defrSetNetNameCbk(defrStringCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->NetNameCbk = f;
}

void
defrSetNetSubnetNameCbk(defrStringCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->NetSubnetNameCbk = f;
}

void
defrSetNetNonDefaultRuleCbk(defrStringCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->NetNonDefaultRuleCbk = f;
}

void
defrSetNetPartialPathCbk(defrNetCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->NetPartialPathCbk = f;
}

void
defrSetSNetStartCbk(defrIntegerCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->SNetStartCbk = f;
}

void
defrSetSNetEndCbk(defrVoidCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->SNetEndCbk = f;
}

void
defrSetSNetCbk(defrNetCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->SNetCbk = f;
}

void
defrSetSNetPartialPathCbk(defrNetCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->SNetPartialPathCbk = f;
}

void
defrSetSNetWireCbk(defrNetCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->SNetWireCbk = f;
}

void
defrSetPathCbk(defrPathCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->PathCbk = f;
}

void
defrSetAddPathToNet()
{
    DEF_INIT;
    defContext.settings->AddPathToNet = 1;
}

void
defrSetAllowComponentNets()
{
    DEF_INIT;
    defContext.settings->AllowComponentNets = 1;
}

int
defrGetAllowComponentNets()
{
    DEF_INIT;
    return defContext.settings->AllowComponentNets;
}


void
defrSetComponentExtCbk(defrStringCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->ComponentExtCbk = f;
}

void
defrSetPinExtCbk(defrStringCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->PinExtCbk = f;
}

void
defrSetViaExtCbk(defrStringCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->ViaExtCbk = f;
}

void
defrSetNetConnectionExtCbk(defrStringCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->NetConnectionExtCbk = f;
}

void
defrSetNetExtCbk(defrStringCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->NetExtCbk = f;
}

void
defrSetGroupExtCbk(defrStringCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->GroupExtCbk = f;
}

void
defrSetScanChainExtCbk(defrStringCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->ScanChainExtCbk = f;
}

void
defrSetIoTimingsExtCbk(defrStringCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->IoTimingsExtCbk = f;
}

void
defrSetPartitionsExtCbk(defrStringCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->PartitionsExtCbk = f;
}

void
defrSetHistoryCbk(defrStringCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->HistoryCbk = f;
}

void
defrSetDieAreaCbk(defrBoxCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->DieAreaCbk = f;
}

void
defrSetPinCapCbk(defrPinCapCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->PinCapCbk = f;
}

void
defrSetPinEndCbk(defrVoidCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->PinEndCbk = f;
}

void
defrSetStartPinsCbk(defrIntegerCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->StartPinsCbk = f;
}

void
defrSetDefaultCapCbk(defrIntegerCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->DefaultCapCbk = f;
}

void
defrSetPinCbk(defrPinCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->PinCbk = f;
}

void
defrSetRowCbk(defrRowCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->RowCbk = f;
}

void
defrSetTrackCbk(defrTrackCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->TrackCbk = f;
}

void
defrSetGcellGridCbk(defrGcellGridCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->GcellGridCbk = f;
}

void
defrSetViaStartCbk(defrIntegerCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->ViaStartCbk = f;
}

void
defrSetViaEndCbk(defrVoidCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->ViaEndCbk = f;
}

void
defrSetViaCbk(defrViaCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->ViaCbk = f;
}

void
defrSetRegionStartCbk(defrIntegerCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->RegionStartCbk = f;
}

void
defrSetRegionEndCbk(defrVoidCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->RegionEndCbk = f;
}

void
defrSetRegionCbk(defrRegionCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->RegionCbk = f;
}

void
defrSetGroupsStartCbk(defrIntegerCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->GroupsStartCbk = f;
}

void
defrSetGroupsEndCbk(defrVoidCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->GroupsEndCbk = f;
}

void
defrSetGroupNameCbk(defrStringCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->GroupNameCbk = f;
}

void
defrSetGroupMemberCbk(defrStringCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->GroupMemberCbk = f;
}

void
defrSetComponentMaskShiftLayerCbk(defrComponentMaskShiftLayerCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->ComponentMaskShiftLayerCbk = f;
}

void
defrSetGroupCbk(defrGroupCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->GroupCbk = f;
}

void
defrSetAssertionsStartCbk(defrIntegerCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->AssertionsStartCbk = f;
}

void
defrSetAssertionsEndCbk(defrVoidCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->AssertionsEndCbk = f;
}

void
defrSetAssertionCbk(defrAssertionCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->AssertionCbk = f;
}

void
defrSetConstraintsStartCbk(defrIntegerCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->ConstraintsStartCbk = f;
}

void
defrSetConstraintsEndCbk(defrVoidCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->ConstraintsEndCbk = f;
}

void
defrSetConstraintCbk(defrAssertionCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->ConstraintCbk = f;
}

void
defrSetScanchainsStartCbk(defrIntegerCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->ScanchainsStartCbk = f;
}

void
defrSetScanchainsEndCbk(defrVoidCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->ScanchainsEndCbk = f;
}

void
defrSetScanchainCbk(defrScanchainCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->ScanchainCbk = f;
}

void
defrSetIOTimingsStartCbk(defrIntegerCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->IOTimingsStartCbk = f;
}

void
defrSetIOTimingsEndCbk(defrVoidCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->IOTimingsEndCbk = f;
}

void
defrSetIOTimingCbk(defrIOTimingCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->IOTimingCbk = f;
}

void
defrSetFPCStartCbk(defrIntegerCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->FPCStartCbk = f;
}

void
defrSetFPCEndCbk(defrVoidCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->FPCEndCbk = f;
}

void
defrSetFPCCbk(defrFPCCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->FPCCbk = f;
}

void
defrSetTimingDisablesStartCbk(defrIntegerCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->TimingDisablesStartCbk = f;
}

void
defrSetTimingDisablesEndCbk(defrVoidCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->TimingDisablesEndCbk = f;
}

void
defrSetTimingDisableCbk(defrTimingDisableCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->TimingDisableCbk = f;
}

void
defrSetPartitionsStartCbk(defrIntegerCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->PartitionsStartCbk = f;
}

void
defrSetPartitionsEndCbk(defrVoidCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->PartitionsEndCbk = f;
}

void
defrSetPartitionCbk(defrPartitionCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->PartitionCbk = f;
}

void
defrSetPinPropStartCbk(defrIntegerCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->PinPropStartCbk = f;
}

void
defrSetPinPropEndCbk(defrVoidCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->PinPropEndCbk = f;
}

void
defrSetPinPropCbk(defrPinPropCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->PinPropCbk = f;
}

void
defrSetCaseSensitiveCbk(defrIntegerCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->CaseSensitiveCbk = f;
}

void
defrSetBlockageStartCbk(defrIntegerCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->BlockageStartCbk = f;
}

void
defrSetBlockageEndCbk(defrVoidCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->BlockageEndCbk = f;
}

void
defrSetBlockageCbk(defrBlockageCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->BlockageCbk = f;
}

void
defrSetSlotStartCbk(defrIntegerCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->SlotStartCbk = f;
}

void
defrSetSlotEndCbk(defrVoidCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->SlotEndCbk = f;
}

void
defrSetSlotCbk(defrSlotCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->SlotCbk = f;
}

void
defrSetFillStartCbk(defrIntegerCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->FillStartCbk = f;
}

void
defrSetFillEndCbk(defrVoidCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->FillEndCbk = f;
}

void
defrSetFillCbk(defrFillCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->FillCbk = f;
}

void
defrSetNonDefaultStartCbk(defrIntegerCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->NonDefaultStartCbk = f;
}

void
defrSetNonDefaultEndCbk(defrVoidCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->NonDefaultEndCbk = f;
}

void
defrSetNonDefaultCbk(defrNonDefaultCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->NonDefaultCbk = f;
}

void
defrSetStylesStartCbk(defrIntegerCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->StylesStartCbk = f;
}

void
defrSetStylesEndCbk(defrVoidCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->StylesEndCbk = f;
}

void
defrSetStylesCbk(defrStylesCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->StylesCbk = f;
}

void
defrSetExtensionCbk(defrStringCbkFnType f)
{
    DEF_INIT;
    defContext.callbacks->ExtensionCbk = f;
}

// NEW CALLBACK - Put the set functions for the new callbacks here. 

void
defrSetAssertionWarnings(int warn)
{
    DEF_INIT;
    defContext.settings->AssertionWarnings = warn;
}


void
defrSetBlockageWarnings(int warn)
{
    DEF_INIT;
    defContext.settings->BlockageWarnings = warn;
}


void
defrSetCaseSensitiveWarnings(int warn)
{
    DEF_INIT;
    defContext.settings->CaseSensitiveWarnings = warn;
}


void
defrSetComponentWarnings(int warn)
{
    DEF_INIT;
    defContext.settings->ComponentWarnings = warn;
}


void
defrSetConstraintWarnings(int warn)
{
    DEF_INIT;
    defContext.settings->ConstraintWarnings = warn;
}


void
defrSetDefaultCapWarnings(int warn)
{
    DEF_INIT;
    defContext.settings->DefaultCapWarnings = warn;
}


void
defrSetGcellGridWarnings(int warn)
{
    DEF_INIT;
    defContext.settings->GcellGridWarnings = warn;
}


void
defrSetIOTimingWarnings(int warn)
{
    DEF_INIT;
    defContext.settings->IOTimingWarnings = warn;
}


void
defrSetNetWarnings(int warn)
{
    DEF_INIT;
    defContext.settings->NetWarnings = warn;
}


void
defrSetNonDefaultWarnings(int warn)
{
    DEF_INIT;
    defContext.settings->NonDefaultWarnings = warn;
}


void
defrSetPinExtWarnings(int warn)
{
    DEF_INIT;
    defContext.settings->PinExtWarnings = warn;
}


void
defrSetPinWarnings(int warn)
{
    DEF_INIT;
    defContext.settings->PinWarnings = warn;
}


void
defrSetRegionWarnings(int warn)
{
    DEF_INIT;
    defContext.settings->RegionWarnings = warn;
}


void
defrSetRowWarnings(int warn)
{
    DEF_INIT;
    defContext.settings->RowWarnings = warn;
}


void
defrSetScanchainWarnings(int warn)
{
    DEF_INIT;
    defContext.settings->ScanchainWarnings = warn;
}


void
defrSetSNetWarnings(int warn)
{
    DEF_INIT;
    defContext.settings->SNetWarnings = warn;
}


void
defrSetStylesWarnings(int warn)
{
    DEF_INIT;
    defContext.settings->StylesWarnings = warn;
}


void
defrSetTrackWarnings(int warn)
{
    DEF_INIT;
    defContext.settings->TrackWarnings = warn;
}


void
defrSetUnitsWarnings(int warn)
{
    DEF_INIT;
    defContext.settings->UnitsWarnings = warn;
}


void
defrSetVersionWarnings(int warn)
{
    DEF_INIT;
    defContext.settings->VersionWarnings = warn;
}


void
defrSetViaWarnings(int warn)
{
    DEF_INIT;
    defContext.settings->ViaWarnings = warn;
}


void
defrDisableParserMsgs(int   nMsg,
                      int   *msgs)
{
    DEF_INIT;
    int i, j;
    int *tmp;

    if (defContext.settings->nDDMsgs == 0) {
        defContext.settings->nDDMsgs = nMsg;
        defContext.settings->disableDMsgs = (int*) malloc(sizeof(int) * nMsg);
        for (i = 0; i < nMsg; i++)
            defContext.settings->disableDMsgs[i] = msgs[i];
    } else {  // add the list to the existing list 
        // 1st check if the msgId is already on the list before adding it on 
        tmp = (int*) malloc(sizeof(int) * (nMsg + defContext.settings->nDDMsgs));
        for (i = 0; i < defContext.settings->nDDMsgs; i++)  // copy the existing to the new list 
            tmp[i] = defContext.settings->disableDMsgs[i];
        free((int*) (defContext.settings->disableDMsgs));
        defContext.settings->disableDMsgs = tmp;           // set disableDMsgs to the new list 
        for (i = 0; i < nMsg; i++) { // merge the new list with the existing 
            for (j = 0; j < defContext.settings->nDDMsgs; j++) {
                if (defContext.settings->disableDMsgs[j] == msgs[i])
                    break;             // msgId already on the list 
            }
            if (j == defContext.settings->nDDMsgs)           // msgId not on the list, add it on 
                defContext.settings->disableDMsgs[defContext.settings->nDDMsgs++] = msgs[i];
        }
    }
    return;
}


void
defrEnableParserMsgs(int    nMsg,
                     int    *msgs)
{
    DEF_INIT;
    int i, j;

    if (defContext.settings->nDDMsgs == 0)
        return;                       // list is empty, nothing to remove 

    for (i = 0; i < nMsg; i++) {     // loop through the given list 
        for (j = 0; j < defContext.settings->nDDMsgs; j++) {
            if (defContext.settings->disableDMsgs[j] == msgs[i]) {
                defContext.settings->disableDMsgs[j] = -1;    // temp assign a -1 on that slot 
                break;
            }
        }
    }
    // fill up the empty slot with the next non -1 msgId 
    for (i = 0; i < defContext.settings->nDDMsgs; i++) {
        if (defContext.settings->disableDMsgs[i] == -1) {
            j = i + 1;
            while (j < defContext.settings->nDDMsgs) {
                if (defContext.settings->disableDMsgs[j] != -1)
                    defContext.settings->disableDMsgs[i++] = defContext.settings->disableDMsgs[j++];
            }
            break;     // break out the for loop, the list should all moved 
        }
    }
    // Count how many messageId left and change all -1 to 0 
    for (j = i; j < defContext.settings->nDDMsgs; j++) {
        defContext.settings->disableDMsgs[j] = 0;     // set to 0 
    }
    defContext.settings->nDDMsgs = i;
    return;
}


void
defrEnableAllMsgs()
{
    DEF_INIT;
    defContext.settings->nDDMsgs = 0;
    free((int*) (defContext.settings->disableDMsgs));
}


void
defrSetTotalMsgLimit(int totNumMsgs)
{
    DEF_INIT;
    defContext.settings->totalDefMsgLimit = totNumMsgs;
}


void
defrSetLimitPerMsg(int  msgId,
                   int  numMsg)
{
    DEF_INIT;
    char msgStr[10];

    if ((msgId <= 0) || ((msgId - 5000) >= NODEFMSG)) {   // Def starts at 5000
        sprintf(msgStr, "%d", msgId);
        return;
    }
    defContext.settings->MsgLimit[msgId - 5000] = numMsg;
    return;
}


// *****************************************************************
// Utility functions
//
// These are utility functions. Note: this part still contains some
// global variables. Ideally they would be part of the main class.
// *****************************************************************

void
defrSetMagicCommentFoundFunction(DEFI_MAGIC_COMMENT_FOUND_FUNCTION f)
{
    DEF_INIT;
    defContext.settings->MagicCommentFoundFunction = f;
}


void
defrSetMagicCommentString(char *s)
{
    DEF_INIT;

    free(defContext.data->magic);
    defContext.data->magic = strdup(s);
}

void
defrSetLogFunction(DEFI_LOG_FUNCTION f)
{
    DEF_INIT;
    defContext.settings->ErrorLogFunction = f;
}

void
defrSetWarningLogFunction(DEFI_WARNING_LOG_FUNCTION f)
{
    DEF_INIT;
    defContext.settings->WarningLogFunction = f;
}

void
defrSetContextLogFunction(DEFI_CONTEXT_LOG_FUNCTION f)
{
    DEF_INIT;
    defContext.settings->ContextErrorLogFunction = f;
}

void
defrSetContextWarningLogFunction(DEFI_CONTEXT_WARNING_LOG_FUNCTION f)
{
    DEF_INIT;
    defContext.settings->ContextWarningLogFunction = f;
}


void
defrSetMallocFunction(DEFI_MALLOC_FUNCTION f)
{
    DEF_INIT;
    defContext.settings->MallocFunction = f;
}

void
defrSetReallocFunction(DEFI_REALLOC_FUNCTION f)
{
    DEF_INIT;
    defContext.settings->ReallocFunction = f;
}

void
defrSetFreeFunction(DEFI_FREE_FUNCTION f)
{
    DEF_INIT;
    defContext.settings->FreeFunction = f;
}

void
defrSetLineNumberFunction(DEFI_LINE_NUMBER_FUNCTION f)
{
    DEF_INIT;
    defContext.settings->LineNumberFunction = f;
}

void
defrSetLongLineNumberFunction(DEFI_LONG_LINE_NUMBER_FUNCTION f)
{
    DEF_INIT;
    defContext.settings->LongLineNumberFunction = f;
}


void
defrSetDeltaNumberLines(int numLines)
{
    DEF_INIT;
    defContext.settings->defiDeltaNumberLines = numLines;
}


void
defrSetCommentChar(char c)
{
    DEF_INIT;
    defContext.settings->CommentChar = c;
}

void
defrSetCaseSensitivity(int caseSense)
{
    DEF_INIT;

    defContext.settings->reader_case_sensitive_set = 1;
    defContext.session->reader_case_sensitive = caseSense;
    if (defContext.data) {
        defContext.data->names_case_sensitive = caseSense;
    }
}


void
defrAddAlias(const char     *key,
             const char     *value,
             int            marked)
{
    // Since the alias data is stored in the hash table, the hash table
    // only takes the key and the data, the marked data will be stored
    // at the end of the value data

    defrData *defData = defContext.data ;

    char    *k1;
    char    *v1;
    int     len = strlen(key) + 1;
    k1 = (char*) malloc(len);
    strcpy(k1, key);
    len = strlen(value) + 1 + 1;   // 1 for the marked
    v1 = (char*) malloc(len);
    //strcpy(v1, value);
    if (marked != 0)
        marked = 1;                 // make sure only 1 digit
    sprintf(v1, "%d%s", marked, value);

    defData->def_alias_set[k1] = v1;
    free(k1);
    free(v1);
}

void
defrSetOpenLogFileAppend()
{
    DEF_INIT;
    defContext.settings->LogFileAppend = TRUE;
}

void
defrUnsetOpenLogFileAppend()
{
    DEF_INIT;
    defContext.settings->LogFileAppend = FALSE;
}


void
defrSetReadFunction(DEFI_READ_FUNCTION f)
{
    DEF_INIT;
    defContext.settings->ReadFunction = f;
}

void
defrUnsetReadFunction()
{
    DEF_INIT;
    defContext.settings->ReadFunction = 0;
}

void
defrDisablePropStrProcess()
{
    DEF_INIT;
    defContext.settings->DisPropStrProcess = 1;
}

void
defrSetNLines(long long n)
{
    defrData *defData = defContext.data;

    defData->nlines = n;
}

int defrLineNumber() 
{    
    // Compatibility feature: in old versions the translators,  
    // the function can be called before defData initialization. 
    if (defContext.data) {
        return (int)defContext.data->nlines; 
    }

    return 0;
}

long long defrLongLineNumber() {
    // Compatibility feature: in old versions the translators,  
    // the function can be called before defData initialization. 
    
    if (defContext.data) {
        return defContext.data->nlines; 
    }

    return (long  long) 0; 
}

END_LEFDEF_PARSER_NAMESPACE

