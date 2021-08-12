// *****************************************************************************
// *****************************************************************************
// Copyright 2012 - 2017, Cadence Design Systems
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
//  $Author: dell $
//  $Revision: #1 $
//  $Date: 2017/06/06 $
//  $State:  $
// *****************************************************************************
// *****************************************************************************
#include "lefrReader.hpp"
#include "lex.h"
#include <stdlib.h>
#include <string.h>

#include "lefiDebug.hpp"
#include "lefrData.hpp"
#include "lefrSettings.hpp"
#include "lefrCallBacks.hpp"

#define NOCBK 100
#define NOLEFMSG 4701 // 4701 = 4700 + 1, message starts on 1

# define LEF_INIT lef_init(__FUNCTION__)

BEGIN_LEFDEF_PARSER_NAMESPACE

static const char *init_call_func = NULL;

void
lef_init(const char  *func)
{
    if (lefSettings == NULL) {
		lefrSettings::reset();
		init_call_func = func;
	}

    if (lefCallbacks == NULL) {
		lefrCallbacks::reset();
		init_call_func = func;
    }
}


void
lefiNerr(int i)
{
    sprintf(lefData->lefrErrMsg, "ERROR number %d\n", i);
    lefiError(1, 0, lefData->lefrErrMsg);
    exit(2);
}

void
lefiNwarn(int i)
{
    sprintf(lefData->lefrErrMsg, "WARNING number %d\n", i);
    lefiError(1, 0, lefData->lefrErrMsg);
    exit(2);
}

double
convert_name2num(const char *versionName)
{
    char    majorNm[80];
    char    minorNm[80];
    char    *subMinorNm = NULL;

    char    *versionNm = strdup(versionName);

    double  major = 0, minor = 0, subMinor = 0;
    double  version, versionNumber;
    char    finalVersion[80];

    sscanf(versionNm, "%[^.].%s", majorNm, minorNm);
    char    *p1 = strchr(minorNm, '.');
    if (p1) {
        subMinorNm = p1 + 1;
        *p1 = '\0';
    }
    major = atof(majorNm);
    minor = atof(minorNm);
    if (subMinorNm)
        subMinor = atof(subMinorNm);

    version = major;

    if (minor > 0)
        version = major + minor / 10;

    if (subMinor > 0)
        version = version + subMinor / 1000;

    lefFree(versionNm);

    sprintf(finalVersion, "%.4f", version);

    versionNumber = atof(finalVersion);

    return versionNumber;
}

bool
validateMaskNumber(int num)
{
    int digit = 0;
    int index = 0;

    if (num < 0) {
        return false;
    }

    while (num > 0) {
        digit = num % 10;

        if (digit > 3) {
            return false;
        }

        index++;
        num = num / 10;
    }

    if (index > 3) {
        return false;
    }

    return true;
}

// *****************************************************************************s
// Global variables
// *****************************************************************************

// 5.6 END LIBRARY is optional.
// Function to initialize global variables.
// This make sure the global variables are initialized

// User control warning to be printed by the parser
void
lefrDisableParserMsgs(int   nMsg,
                      int   *msgs)
{
    LEF_INIT;
    if (nMsg <= 0)
        return;

    for (int i = 0; i < nMsg; i++) {
        lefSettings->disableMsg(msgs[i]);
    }
}

void
lefrEnableParserMsgs(int    nMsg,
                     int    *msgs)
{
    LEF_INIT;
    for (int i = 0; i < nMsg; i++) {
        lefSettings->enableMsg(msgs[i]);
    }
}

void
lefrEnableAllMsgs()
{
    LEF_INIT;
    lefSettings->enableAllMsgs();
    lefSettings->dAllMsgs = 0;
}

void
lefrSetTotalMsgLimit(int totNumMsgs)
{
    LEF_INIT;
    lefSettings->TotalMsgLimit = totNumMsgs;
}

void
lefrSetLimitPerMsg(int  msgId,
                   int  numMsg)
{
    LEF_INIT;
    
    if ((msgId > 0) && (msgId < NOLEFMSG)) {
        lefSettings->MsgLimit[msgId] = numMsg;
    }
}

// *****************************************************************************
// Since the lef parser only keep one list of disable message ids, and does
// not have a list of enable message ids, if the API lefrDisableAllMsgs is
// called to disable all message ids, user has to call API lefrEnableAllMsgs
// to enable all message ids lefData->first, before calling lefrDisableParserMsgs &
// lefrEnableParserMsgs.
// Users cannot call lefrDisableAllMsgs and call lefrEnableParserMsgs to
// enable a small list of message ids since lefrDisableAllMsgs does not have
// a list of all message ids, hence there isn't a list for lefrEnableParserMsgs
// to work on to enable the message ids.
// *****************************************************************************
void
lefrDisableAllMsgs()
{
    LEF_INIT;
    lefSettings->enableAllMsgs();
    lefSettings->dAllMsgs = 1;
}

// Parser control by the user.
// Reader initialization
int
lefrInit()
{
	return lefrInitSession(0);
}

int
lefrInitSession(int startSession)
{
	if (startSession) { 
		if (init_call_func != NULL) {
			fprintf(stderr, "ERROR: Attempt to call configuration function '%s' in LEF parser before lefrInit() call in session-based mode.\n", init_call_func);
			return 1;
		}

		lefrCallbacks::reset();
		lefrSettings::reset();
	} else {
		if (lefCallbacks == NULL) {
			lefrCallbacks::reset();
		}
	
		if (lefSettings == NULL) {
			lefrSettings::reset();
		}
	}

    return 0;
}

int
lefrReset()
{
    // obsoleted.
    return 0;
}


int 
lefrClear()
{
    delete lefData;
    lefData = NULL;

    delete lefCallbacks;
    lefCallbacks = NULL;

    delete lefSettings;
    lefSettings = NULL;

    return 0;
}


const char *
lefrFName()
{
    return lefData->lefrFileName;
}

int
lefrReleaseNResetMemory()
{
    return 0;
}

int
lefrRead(FILE           *f,
         const char     *fName,
         lefiUserData   uData)
{
    LEF_INIT;
    int status;

    lefrData::reset();

    lefData->versionNum = (lefSettings->VersionNum == 0.0) ?
        CURRENT_VERSION :
        lefSettings->VersionNum;

    if (lefSettings->CaseSensitiveSet) {
        lefData->namesCaseSensitive = lefSettings->CaseSensitive;
    } else if (lefData->versionNum > 5.5) {
        lefData->namesCaseSensitive = true;
    }

    lefData->lefrFileName = (char*) fName;
    lefData->lefrFile = f;
    lefSettings->UserData = uData;

    status = lefyyparse();

    return status;
}

void
lefrSetUnusedCallbacks(lefrVoidCbkFnType func)
{
    // Set all of the callbacks that have not been set yet to
    // the given function.
    LEF_INIT;

    if (lefCallbacks->ArrayBeginCbk == 0)
        lefCallbacks->ArrayBeginCbk = (lefrStringCbkFnType) func;
    if (lefCallbacks->ArrayCbk == 0)
        lefCallbacks->ArrayCbk = (lefrArrayCbkFnType) func;
    if (lefCallbacks->ArrayEndCbk == 0)
        lefCallbacks->ArrayEndCbk = (lefrStringCbkFnType) func;
    if (lefCallbacks->DividerCharCbk == 0)
        lefCallbacks->DividerCharCbk = (lefrStringCbkFnType) func;
    if (lefCallbacks->BusBitCharsCbk == 0)
        lefCallbacks->BusBitCharsCbk = (lefrStringCbkFnType) func;
    if (lefCallbacks->CaseSensitiveCbk == 0)
        lefCallbacks->CaseSensitiveCbk = (lefrIntegerCbkFnType) func;
    if (lefCallbacks->NoWireExtensionCbk == 0)
        lefCallbacks->NoWireExtensionCbk = (lefrStringCbkFnType) func;
    if (lefCallbacks->CorrectionTableCbk == 0)
        lefCallbacks->CorrectionTableCbk = (lefrCorrectionTableCbkFnType) func;
    if (lefCallbacks->DielectricCbk == 0)
        lefCallbacks->DielectricCbk = (lefrDoubleCbkFnType) func;
    if (lefCallbacks->EdgeRateScaleFactorCbk == 0)
        lefCallbacks->EdgeRateScaleFactorCbk = (lefrDoubleCbkFnType) func;
    if (lefCallbacks->EdgeRateThreshold1Cbk == 0)
        lefCallbacks->EdgeRateThreshold1Cbk = (lefrDoubleCbkFnType) func;
    if (lefCallbacks->EdgeRateThreshold2Cbk == 0)
        lefCallbacks->EdgeRateThreshold2Cbk = (lefrDoubleCbkFnType) func;
    if (lefCallbacks->IRDropBeginCbk == 0)
        lefCallbacks->IRDropBeginCbk = (lefrVoidCbkFnType) func;
    if (lefCallbacks->IRDropCbk == 0)
        lefCallbacks->IRDropCbk = (lefrIRDropCbkFnType) func;
    if (lefCallbacks->IRDropEndCbk == 0)
        lefCallbacks->IRDropEndCbk = (lefrVoidCbkFnType) func;
    if (lefCallbacks->LayerCbk == 0)
        lefCallbacks->LayerCbk = (lefrLayerCbkFnType) func;
    if (lefCallbacks->LibraryEndCbk == 0)
        lefCallbacks->LibraryEndCbk = (lefrVoidCbkFnType) func;
    if (lefCallbacks->MacroBeginCbk == 0)
        lefCallbacks->MacroBeginCbk = (lefrStringCbkFnType) func;
    if (lefCallbacks->MacroCbk == 0)
        lefCallbacks->MacroCbk = (lefrMacroCbkFnType) func;
    if (lefCallbacks->MacroClassTypeCbk == 0)
        lefCallbacks->MacroClassTypeCbk = (lefrStringCbkFnType) func;
    if (lefCallbacks->MacroOriginCbk == 0)
        lefCallbacks->MacroOriginCbk = (lefrMacroNumCbkFnType) func;
    if (lefCallbacks->MacroSiteCbk == 0)
        lefCallbacks->MacroSiteCbk = (lefrMacroSiteCbkFnType) func;
    if (lefCallbacks->MacroForeignCbk == 0)
        lefCallbacks->MacroForeignCbk = (lefrMacroForeignCbkFnType) func;
    if (lefCallbacks->MacroSizeCbk == 0)
        lefCallbacks->MacroSizeCbk = (lefrMacroNumCbkFnType) func;
    if (lefCallbacks->MacroFixedMaskCbk == 0)
        lefCallbacks->MacroFixedMaskCbk = (lefrIntegerCbkFnType) func;
    if (lefCallbacks->TimingCbk == 0)
        lefCallbacks->TimingCbk = (lefrTimingCbkFnType) func;
    if (lefCallbacks->MinFeatureCbk == 0)
        lefCallbacks->MinFeatureCbk = (lefrMinFeatureCbkFnType) func;
    if (lefCallbacks->NoiseMarginCbk == 0)
        lefCallbacks->NoiseMarginCbk = (lefrNoiseMarginCbkFnType) func;
    if (lefCallbacks->NoiseTableCbk == 0)
        lefCallbacks->NoiseTableCbk = (lefrNoiseTableCbkFnType) func;
    if (lefCallbacks->NonDefaultCbk == 0)
        lefCallbacks->NonDefaultCbk = (lefrNonDefaultCbkFnType) func;
    if (lefCallbacks->ObstructionCbk == 0)
        lefCallbacks->ObstructionCbk = (lefrObstructionCbkFnType) func;
    if (lefCallbacks->PinCbk == 0)
        lefCallbacks->PinCbk = (lefrPinCbkFnType) func;
    if (lefCallbacks->PropBeginCbk == 0)
        lefCallbacks->PropBeginCbk = (lefrVoidCbkFnType) func;
    if (lefCallbacks->PropCbk == 0)
        lefCallbacks->PropCbk = (lefrPropCbkFnType) func;
    if (lefCallbacks->PropEndCbk == 0)
        lefCallbacks->PropEndCbk = (lefrVoidCbkFnType) func;
    if (lefCallbacks->SiteCbk == 0)
        lefCallbacks->SiteCbk = (lefrSiteCbkFnType) func;
    if (lefCallbacks->SpacingBeginCbk == 0)
        lefCallbacks->SpacingBeginCbk = (lefrVoidCbkFnType) func;
    if (lefCallbacks->SpacingCbk == 0)
        lefCallbacks->SpacingCbk = (lefrSpacingCbkFnType) func;
    if (lefCallbacks->SpacingEndCbk == 0)
        lefCallbacks->SpacingEndCbk = (lefrVoidCbkFnType) func;
    if (lefCallbacks->UnitsCbk == 0)
        lefCallbacks->UnitsCbk = (lefrUnitsCbkFnType) func;
    if ((lefCallbacks->VersionCbk == 0) && (lefCallbacks->VersionStrCbk == 0)) {
        // both version callbacks weren't set, if either one is set, it is ok
        lefCallbacks->VersionCbk = (lefrDoubleCbkFnType) func;
        lefCallbacks->VersionStrCbk = (lefrStringCbkFnType) func;
    }
    if (lefCallbacks->ViaCbk == 0)
        lefCallbacks->ViaCbk = (lefrViaCbkFnType) func;
    if (lefCallbacks->ViaRuleCbk == 0)
        lefCallbacks->ViaRuleCbk = (lefrViaRuleCbkFnType) func;
    if (lefCallbacks->InputAntennaCbk == 0)
        lefCallbacks->InputAntennaCbk = (lefrDoubleCbkFnType) func;
    if (lefCallbacks->OutputAntennaCbk == 0)
        lefCallbacks->OutputAntennaCbk = (lefrDoubleCbkFnType) func;
    if (lefCallbacks->InoutAntennaCbk == 0)
        lefCallbacks->InoutAntennaCbk = (lefrDoubleCbkFnType) func;

    // NEW CALLBACK - Add a line here for each new callback routine 
    if (lefCallbacks->AntennaInputCbk == 0)
        lefCallbacks->AntennaInputCbk = (lefrDoubleCbkFnType) func;
    if (lefCallbacks->AntennaInoutCbk == 0)
        lefCallbacks->AntennaInoutCbk = (lefrDoubleCbkFnType) func;
    if (lefCallbacks->AntennaOutputCbk == 0)
        lefCallbacks->AntennaOutputCbk = (lefrDoubleCbkFnType) func;
    if (lefCallbacks->ManufacturingCbk == 0)
        lefCallbacks->ManufacturingCbk = (lefrDoubleCbkFnType) func;
    if (lefCallbacks->UseMinSpacingCbk == 0)
        lefCallbacks->UseMinSpacingCbk = (lefrUseMinSpacingCbkFnType) func;
    if (lefCallbacks->ClearanceMeasureCbk == 0)
        lefCallbacks->ClearanceMeasureCbk = (lefrStringCbkFnType) func;
    if (lefCallbacks->MacroClassTypeCbk == 0)
        lefCallbacks->MacroClassTypeCbk = (lefrStringCbkFnType) func;
    if (lefCallbacks->MacroOriginCbk == 0)
        lefCallbacks->MacroOriginCbk = (lefrMacroNumCbkFnType) func;
    if (lefCallbacks->MacroSiteCbk == 0)
        lefCallbacks->MacroSiteCbk = (lefrMacroSiteCbkFnType) func;
    if (lefCallbacks->MacroForeignCbk == 0)
        lefCallbacks->MacroForeignCbk = (lefrMacroForeignCbkFnType) func;
    if (lefCallbacks->MacroSizeCbk == 0)
        lefCallbacks->MacroSizeCbk = (lefrMacroNumCbkFnType) func;
    if (lefCallbacks->MacroFixedMaskCbk == 0)
        lefCallbacks->MacroFixedMaskCbk = (lefrIntegerCbkFnType) func;
    if (lefCallbacks->MacroEndCbk == 0)
        lefCallbacks->MacroEndCbk = (lefrStringCbkFnType) func;
    if (lefCallbacks->MaxStackViaCbk == 0)
        lefCallbacks->MaxStackViaCbk = (lefrMaxStackViaCbkFnType) func;
    if (lefCallbacks->ExtensionCbk == 0)
        lefCallbacks->ExtensionCbk = (lefrStringCbkFnType) func;
    if (lefCallbacks->DensityCbk == 0)
        lefCallbacks->DensityCbk = (lefrDensityCbkFnType) func;
    if (lefCallbacks->FixedMaskCbk == 0)
        lefCallbacks->FixedMaskCbk = (lefrIntegerCbkFnType) func;
}

// These count up the number of times an unset callback is called... 
static int lefrUnusedCount[NOCBK];

int
lefrCountFunc(lefrCallbackType_e    e,
              void                  *v,
              lefiUserData          d)
{
    LEF_INIT;
    int i = (int) e;
    if (lefiDebug(23))
        printf("count %d 0x%p 0x%p\n", (int) e, v, d);
    if (i >= 0 && i < NOCBK) {
        lefrUnusedCount[i] += 1;
        return 0;
    }
    return 1;
}

void
lefrSetRegisterUnusedCallbacks()
{
    LEF_INIT;
    int i;
    lefSettings->RegisterUnused = 1;
    lefrSetUnusedCallbacks(lefrCountFunc);
    for (i = 0; i < NOCBK; i++)
        lefrUnusedCount[i] = 0;
}

void
lefrPrintUnusedCallbacks(FILE *f)
{
    LEF_INIT;
    int i;
    int firstCB = 1;
    int trueCB = 1;

    if (lefSettings->RegisterUnused == 0) {
        fprintf(f,
                "ERROR (LEFPARS-101): lefrSetRegisterUnusedCallbacks was not called to setup this data.\n");
        return;
    }

    for (i = 0; i < NOCBK; i++) {
        if (lefrUnusedCount[i]) {
            // Do not need to print yet if i is:
            //  lefrMacroClassTypeCbkType
            //  lefrMacroOriginCbkType
            //  lefrMacroSizeCbkType
            //  lefrMacroEndCbkType
            // it will be taken care later
            if (firstCB &&
                (lefrCallbackType_e) i != lefrMacroClassTypeCbkType &&
                (lefrCallbackType_e) i != lefrMacroOriginCbkType &&
                (lefrCallbackType_e) i != lefrMacroSiteCbkType &&
                (lefrCallbackType_e) i != lefrMacroForeignCbkType &&
                (lefrCallbackType_e) i != lefrMacroSizeCbkType &&
                (lefrCallbackType_e) i != lefrMacroFixedMaskCbkType &&
                (lefrCallbackType_e) i != lefrMacroEndCbkType) {
                fprintf(f,
                        "WARNING (LEFPARS-201): LEF items that were present but ignored because of no callback:\n");
                firstCB = 0;
            }
            switch ((lefrCallbackType_e) i) {
            case lefrArrayBeginCbkType:
                fprintf(f, "ArrayBegin");
                break;
            case lefrArrayCbkType:
                fprintf(f, "Array");
                break;
            case lefrArrayEndCbkType:
                fprintf(f, "ArrayEnd");
                break;
            case lefrDividerCharCbkType:
                fprintf(f, "DividerChar");
                break;
            case lefrBusBitCharsCbkType:
                fprintf(f, "BusBitChars");
                break;
            case lefrNoWireExtensionCbkType:
                fprintf(f, "NoWireExtensionAtPins");
                break;
            case lefrCaseSensitiveCbkType:
                fprintf(f, "CaseSensitive");
                break;
            case lefrCorrectionTableCbkType:
                fprintf(f, "CorrectionTable");
                break;
            case lefrDielectricCbkType:
                fprintf(f, "Dielectric");
                break;
            case lefrEdgeRateScaleFactorCbkType:
                fprintf(f, "EdgeRateScaleFactor");
                break;
            case lefrEdgeRateThreshold1CbkType:
                fprintf(f, "EdgeRateThreshold1");
                break;
            case lefrEdgeRateThreshold2CbkType:
                fprintf(f, "EdgeRateThreshold2");
                break;
            case lefrIRDropBeginCbkType:
                fprintf(f, "IRDropBegin");
                break;
            case lefrIRDropCbkType:
                fprintf(f, "IRDrop");
                break;
            case lefrIRDropEndCbkType:
                fprintf(f, "IRDropEnd");
                break;
            case lefrLayerCbkType:
                fprintf(f, "Layer");
                break;
            case lefrLibraryEndCbkType:
                fprintf(f, "LibraryEnd");
                break;
            case lefrMacroBeginCbkType:
                fprintf(f, "MacroBegin");
                break;
            case lefrMacroCbkType:
                fprintf(f, "Macro");
                break;
            case lefrMinFeatureCbkType:
                fprintf(f, "MinFeature");
                break;
            case lefrNoiseMarginCbkType:
                fprintf(f, "NoiseMargin");
                break;
            case lefrNoiseTableCbkType:
                fprintf(f, "NoiseTable");
                break;
            case lefrNonDefaultCbkType:
                fprintf(f, "NonDefault");
                break;
            case lefrObstructionCbkType:
                fprintf(f, "Obstruction");
                break;
            case lefrPinCbkType:
                fprintf(f, "Pin");
                break;
            case lefrPropBeginCbkType:
                fprintf(f, "PropBegin");
                break;
            case lefrPropCbkType:
                fprintf(f, "Prop");
                break;
            case lefrPropEndCbkType:
                fprintf(f, "PropEnd");
                break;
            case lefrSiteCbkType:
                fprintf(f, "Site");
                break;
            case lefrSpacingBeginCbkType:
                fprintf(f, "SpacingBegin");
                break;
            case lefrSpacingCbkType:
                fprintf(f, "Spacing");
                break;
            case lefrSpacingEndCbkType:
                fprintf(f, "SpacingEnd");
                break;
            case lefrUnitsCbkType:
                fprintf(f, "Units");
                break;
            case lefrVersionCbkType:
                fprintf(f, "Version");
                break;
            case lefrVersionStrCbkType:
                fprintf(f, "Version");
                break;
            case lefrViaCbkType:
                fprintf(f, "Via");
                break;
            case lefrViaRuleCbkType:
                fprintf(f, "ViaRule");
                break;
            case lefrInputAntennaCbkType:
                fprintf(f, "InputAntenna");
                break;
            case lefrOutputAntennaCbkType:
                fprintf(f, "OutputAntenna");
                break;
            case lefrInoutAntennaCbkType:
                fprintf(f, "InoutAntenna");
                break;
            case lefrAntennaInputCbkType:
                fprintf(f, "AntennaInput");
                break;
            case lefrAntennaInoutCbkType:
                fprintf(f, "AntennaInout");
                break;
            case lefrAntennaOutputCbkType:
                fprintf(f, "AntennaOutput");
                break;
            case lefrManufacturingCbkType:
                fprintf(f, "Manufacturing");
                break;
            case lefrUseMinSpacingCbkType:
                fprintf(f, "UseMinSpacing");
                break;
            case lefrClearanceMeasureCbkType:
                fprintf(f, "ClearanceMeasure");
                break;
            case lefrTimingCbkType:
                fprintf(f, "Timing");
                break;
            case lefrMaxStackViaCbkType:
                fprintf(f, "MaxStackVia");
                break;
            case lefrExtensionCbkType:
                fprintf(f, "Extension");
                break;
                // 07/13/2001 - Wanda da Rosa
                // Don't need to print MacroClassType if it is not set,
                // since this is an extra CB for Ambit only.
                // Other users should not have to deal with it.
                // case lefrMacroClassTypeCbkType: fprintf(f, "MacroClassType"); break;
            case lefrMacroClassTypeCbkType:
            case lefrMacroOriginCbkType:
            case lefrMacroSiteCbkType:
            case lefrMacroForeignCbkType:
            case lefrMacroSizeCbkType:
            case lefrMacroFixedMaskCbkType:
            case lefrMacroEndCbkType:
                trueCB = 0;
                break;
                // NEW CALLBACK  add the print here 
            case lefrDensityCbkType:
                fprintf(f, "Density");
                break;
            case lefrFixedMaskCbkType:
                fprintf(f, "FixedMask");
                break;
            default:
                fprintf(f, "BOGUS ENTRY");
                break;
            }
            if (trueCB)
                fprintf(f, " %d\n", lefrUnusedCount[i]);
            else
                trueCB = 1;
        }
    }
}

void
lefrUnsetCallbacks()
{
    lefrCallbacks::reset();
}

// Unset callbacks functions
void
lefrUnsetAntennaInoutCbk()
{
    LEF_INIT;
    lefCallbacks->AntennaInoutCbk = 0;
}

void
lefrUnsetAntennaInputCbk()
{
    LEF_INIT;
    lefCallbacks->AntennaInputCbk = 0;
}

void
lefrUnsetAntennaOutputCbk()
{
    LEF_INIT;
    lefCallbacks->AntennaOutputCbk = 0;
}

void
lefrUnsetArrayBeginCbk()
{
    LEF_INIT;
    lefCallbacks->ArrayBeginCbk = 0;
}

void
lefrUnsetArrayCbk()
{
    LEF_INIT;
    lefCallbacks->ArrayCbk = 0;
}

void
lefrUnsetArrayEndCbk()
{
    LEF_INIT;
    lefCallbacks->ArrayEndCbk = 0;
}

void
lefrUnsetBusBitCharsCbk()
{
    LEF_INIT;
    lefCallbacks->BusBitCharsCbk = 0;
}

void
lefrUnsetCaseSensitiveCbk()
{
    LEF_INIT;
    lefCallbacks->CaseSensitiveCbk = 0;
}

void
lefrUnsetClearanceMeasureCbk()
{
    LEF_INIT;
    lefCallbacks->ClearanceMeasureCbk = 0;
}

void
lefrUnsetCorrectionTableCbk()
{
    LEF_INIT;
    lefCallbacks->CorrectionTableCbk = 0;
}

void
lefrUnsetDensityCbk()
{
    LEF_INIT;
    lefCallbacks->DensityCbk = 0;
}

void
lefrUnsetDielectricCbk()
{
    LEF_INIT;
    lefCallbacks->DielectricCbk = 0;
}

void
lefrUnsetDividerCharCbk()
{
    LEF_INIT;
    lefCallbacks->DividerCharCbk = 0;
}

void
lefrUnsetEdgeRateScaleFactorCbk()
{
    LEF_INIT;
    lefCallbacks->EdgeRateScaleFactorCbk = 0;
}

void
lefrUnsetEdgeRateThreshold1Cbk()
{
    LEF_INIT;
    lefCallbacks->EdgeRateThreshold1Cbk = 0;
}

void
lefrUnsetEdgeRateThreshold2Cbk()
{
    LEF_INIT;
    lefCallbacks->EdgeRateThreshold2Cbk = 0;
}

void
lefrUnsetExtensionCbk()
{
    LEF_INIT;
    lefCallbacks->ExtensionCbk = 0;
}

void
lefrUnsetFixedMaskCbk()
{
    LEF_INIT;
    lefCallbacks->FixedMaskCbk = 0;
}
void
lefrUnsetIRDropBeginCbk()
{
    LEF_INIT;
    lefCallbacks->IRDropBeginCbk = 0;
}

void
lefrUnsetIRDropCbk()
{
    LEF_INIT;
    lefCallbacks->IRDropCbk = 0;
}

void
lefrUnsetIRDropEndCbk()
{
    LEF_INIT;
    lefCallbacks->IRDropEndCbk = 0;
}

void
lefrUnsetInoutAntennaCbk()
{
    LEF_INIT;
    lefCallbacks->InoutAntennaCbk = 0;
}

void
lefrUnsetInputAntennaCbk()
{
    LEF_INIT;
    lefCallbacks->InputAntennaCbk = 0;
}

void
lefrUnsetLayerCbk()
{
    LEF_INIT;
    lefCallbacks->LayerCbk = 0;
}

void
lefrUnsetLibraryEndCbk()
{
    LEF_INIT;
    lefCallbacks->LibraryEndCbk = 0;
}

void
lefrUnsetMacroBeginCbk()
{
    LEF_INIT;
    lefCallbacks->MacroBeginCbk = 0;
}

void
lefrUnsetMacroCbk()
{
    LEF_INIT;
    lefCallbacks->MacroCbk = 0;
}

void
lefrUnsetMacroClassTypeCbk()
{
    LEF_INIT;
    lefCallbacks->MacroClassTypeCbk = 0;
}

void
lefrUnsetMacroEndCbk()
{
    LEF_INIT;
    lefCallbacks->MacroEndCbk = 0;
}

void
lefrUnsetMacroFixedMaskCbk()
{
    LEF_INIT;
    lefCallbacks->MacroFixedMaskCbk = 0;
}

void
lefrUnsetMacroOriginCbk()
{
    LEF_INIT;
    lefCallbacks->MacroOriginCbk = 0;
}

void
lefrUnsetMacroSiteCbk()
{
    LEF_INIT;
    lefCallbacks->MacroSiteCbk = 0;
}

void
lefrUnsetMacroForeignCbk()
{
    LEF_INIT;
    lefCallbacks->MacroForeignCbk = 0;
}

void
lefrUnsetMacroSizeCbk()
{
    LEF_INIT;
    lefCallbacks->MacroSizeCbk = 0;
}

void
lefrUnsetManufacturingCbk()
{
    LEF_INIT;
    lefCallbacks->ManufacturingCbk = 0;
}

void
lefrUnsetMaxStackViaCbk()
{
    LEF_INIT;
    lefCallbacks->MaxStackViaCbk = 0;
}

void
lefrUnsetMinFeatureCbk()
{
    LEF_INIT;
    lefCallbacks->MinFeatureCbk = 0;
}

void
lefrUnsetNoWireExtensionCbk()
{
    LEF_INIT;
    lefCallbacks->NoWireExtensionCbk = 0;
}

void
lefrUnsetNoiseMarginCbk()
{
    LEF_INIT;
    lefCallbacks->NoiseMarginCbk = 0;
}

void
lefrUnsetNoiseTableCbk()
{
    LEF_INIT;
    lefCallbacks->NoiseTableCbk = 0;
}

void
lefrUnsetNonDefaultCbk()
{
    LEF_INIT;
    lefCallbacks->NonDefaultCbk = 0;
}

void
lefrUnsetObstructionCbk()
{
    LEF_INIT;
    lefCallbacks->ObstructionCbk = 0;
}

void
lefrUnsetOutputAntennaCbk()
{
    LEF_INIT;
    lefCallbacks->OutputAntennaCbk = 0;
}

void
lefrUnsetPinCbk()
{
    LEF_INIT;
    lefCallbacks->PinCbk = 0;
}

void
lefrUnsetPropBeginCbk()
{
    LEF_INIT;
    lefCallbacks->PropBeginCbk = 0;
}

void
lefrUnsetPropCbk()
{
    LEF_INIT;
    lefCallbacks->PropCbk = 0;
}

void
lefrUnsetPropEndCbk()
{
    LEF_INIT;
    lefCallbacks->PropEndCbk = 0;
}

void
lefrUnsetSiteCbk()
{
    LEF_INIT;
    lefCallbacks->SiteCbk = 0;
}

void
lefrUnsetSpacingBeginCbk()
{
    LEF_INIT;
    lefCallbacks->SpacingBeginCbk = 0;
}

void
lefrUnsetSpacingCbk()
{
    LEF_INIT;
    lefCallbacks->SpacingCbk = 0;
}

void
lefrUnsetSpacingEndCbk()
{
    LEF_INIT;
    lefCallbacks->SpacingEndCbk = 0;
}

void
lefrUnsetTimingCbk()
{
    LEF_INIT;
    lefCallbacks->TimingCbk = 0;
}

void
lefrUnsetUnitsCbk()
{
    LEF_INIT;
    lefCallbacks->UnitsCbk = 0;
}

void
lefrUnsetUseMinSpacingCbk()
{
    LEF_INIT;
    lefCallbacks->UseMinSpacingCbk = 0;
}

void
lefrUnsetVersionCbk()
{
    LEF_INIT;
    lefCallbacks->VersionCbk = 0;
}

void
lefrUnsetVersionStrCbk()
{
    LEF_INIT;
    lefCallbacks->VersionStrCbk = 0;
}

void
lefrUnsetViaCbk()
{
    LEF_INIT;
    lefCallbacks->ViaCbk = 0;
}

void
lefrUnsetViaRuleCbk()
{
    LEF_INIT;
    lefCallbacks->ViaRuleCbk = 0;
}

// Setting of user data.
void
lefrSetUserData(lefiUserData d)
{
    LEF_INIT;
    lefSettings->UserData = d;
}

lefiUserData
lefrGetUserData()
{
    LEF_INIT;
    return lefSettings->UserData;
}

// Callbacks set functions.

void
lefrSetAntennaInoutCbk(lefrDoubleCbkFnType f)
{
    LEF_INIT;
    lefCallbacks->AntennaInoutCbk = f;
}

void
lefrSetAntennaInputCbk(lefrDoubleCbkFnType f)
{
    LEF_INIT;
    lefCallbacks->AntennaInputCbk = f;
}

void
lefrSetAntennaOutputCbk(lefrDoubleCbkFnType f)
{
    LEF_INIT;
    lefCallbacks->AntennaOutputCbk = f;
}

void
lefrSetArrayBeginCbk(lefrStringCbkFnType f)
{
    LEF_INIT;
    lefCallbacks->ArrayBeginCbk = f;
}

void
lefrSetArrayCbk(lefrArrayCbkFnType f)
{
    LEF_INIT;
    lefCallbacks->ArrayCbk = f;
}

void
lefrSetArrayEndCbk(lefrStringCbkFnType f)
{
    LEF_INIT;
    lefCallbacks->ArrayEndCbk = f;
}

void
lefrSetBusBitCharsCbk(lefrStringCbkFnType f)
{
    LEF_INIT;
    lefCallbacks->BusBitCharsCbk = f;
}

void
lefrSetCaseSensitiveCbk(lefrIntegerCbkFnType f)
{
    LEF_INIT;
    lefCallbacks->CaseSensitiveCbk = f;
}

void
lefrSetClearanceMeasureCbk(lefrStringCbkFnType f)
{
    LEF_INIT;
    lefCallbacks->ClearanceMeasureCbk = f;
}

void
lefrSetCorrectionTableCbk(lefrCorrectionTableCbkFnType f)
{
    LEF_INIT;
    lefCallbacks->CorrectionTableCbk = f;
}

void
lefrSetDensityCbk(lefrDensityCbkFnType f)
{
    LEF_INIT;
    lefCallbacks->DensityCbk = f;
}

void
lefrSetDielectricCbk(lefrDoubleCbkFnType f)
{
    LEF_INIT;
    lefCallbacks->DielectricCbk = f;
}

void
lefrSetDividerCharCbk(lefrStringCbkFnType f)
{
    LEF_INIT;
    lefCallbacks->DividerCharCbk = f;
}

void
lefrSetEdgeRateScaleFactorCbk(lefrDoubleCbkFnType f)
{
    LEF_INIT;
    lefCallbacks->EdgeRateScaleFactorCbk = f;
}

void
lefrSetEdgeRateThreshold1Cbk(lefrDoubleCbkFnType f)
{
    LEF_INIT;
    lefCallbacks->EdgeRateThreshold1Cbk = f;
}

void
lefrSetEdgeRateThreshold2Cbk(lefrDoubleCbkFnType f)
{
    LEF_INIT;
    lefCallbacks->EdgeRateThreshold2Cbk = f;
}

void
lefrSetExtensionCbk(lefrStringCbkFnType f)
{
    LEF_INIT;
    lefCallbacks->ExtensionCbk = f;
}

void
lefrSetFixedMaskCbk(lefrIntegerCbkFnType f)
{
    LEF_INIT;
    lefCallbacks->FixedMaskCbk = f;
}

void
lefrSetIRDropBeginCbk(lefrVoidCbkFnType f)
{
    LEF_INIT;
    lefCallbacks->IRDropBeginCbk = f;
}

void
lefrSetIRDropCbk(lefrIRDropCbkFnType f)
{
    LEF_INIT;
    lefCallbacks->IRDropCbk = f;
}

void
lefrSetIRDropEndCbk(lefrVoidCbkFnType f)
{
    LEF_INIT;
    lefCallbacks->IRDropEndCbk = f;
}

void
lefrSetInoutAntennaCbk(lefrDoubleCbkFnType f)
{
    LEF_INIT;
    lefCallbacks->InoutAntennaCbk = f;
}

void
lefrSetInputAntennaCbk(lefrDoubleCbkFnType f)
{
    LEF_INIT;
    lefCallbacks->InputAntennaCbk = f;
}

void
lefrSetLayerCbk(lefrLayerCbkFnType f)
{
    LEF_INIT;
    lefCallbacks->LayerCbk = f;
}

void
lefrSetLibraryEndCbk(lefrVoidCbkFnType f)
{
    LEF_INIT;
    lefCallbacks->LibraryEndCbk = f;
}

void
lefrSetMacroBeginCbk(lefrStringCbkFnType f)
{
    LEF_INIT;
    lefCallbacks->MacroBeginCbk = f;
}

void
lefrSetMacroCbk(lefrMacroCbkFnType f)
{
    LEF_INIT;
    lefCallbacks->MacroCbk = f;
}

void
lefrSetMacroClassTypeCbk(lefrStringCbkFnType f)
{
    LEF_INIT;
    lefCallbacks->MacroClassTypeCbk = f;
}

void
lefrSetMacroEndCbk(lefrStringCbkFnType f)
{
    LEF_INIT;
    lefCallbacks->MacroEndCbk = f;
}

void
lefrSetMacroFixedMaskCbk(lefrIntegerCbkFnType f)
{
    LEF_INIT;
    lefCallbacks->MacroFixedMaskCbk = f;
}

void
lefrSetMacroOriginCbk(lefrMacroNumCbkFnType f)
{
    LEF_INIT;
    lefCallbacks->MacroOriginCbk = f;
}

void
lefrSetMacroSiteCbk(lefrMacroSiteCbkFnType f)
{
    LEF_INIT;
    lefCallbacks->MacroSiteCbk = f;
}

void
lefrSetMacroForeignCbk(lefrMacroForeignCbkFnType f)
{
    LEF_INIT;
    lefCallbacks->MacroForeignCbk = f;
}

void
lefrSetMacroSizeCbk(lefrMacroNumCbkFnType f)
{
    LEF_INIT;
    lefCallbacks->MacroSizeCbk = f;
}

void
lefrSetManufacturingCbk(lefrDoubleCbkFnType f)
{
    LEF_INIT;
    lefCallbacks->ManufacturingCbk = f;
}

void
lefrSetMaxStackViaCbk(lefrMaxStackViaCbkFnType f)
{
    LEF_INIT;
    lefCallbacks->MaxStackViaCbk = f;
}

void
lefrSetMinFeatureCbk(lefrMinFeatureCbkFnType f)
{
    LEF_INIT;
    lefCallbacks->MinFeatureCbk = f;
}

void
lefrSetNoWireExtensionCbk(lefrStringCbkFnType f)
{
    LEF_INIT;
    lefCallbacks->NoWireExtensionCbk = f;
}

void
lefrSetNoiseMarginCbk(lefrNoiseMarginCbkFnType f)
{
    LEF_INIT;
    lefCallbacks->NoiseMarginCbk = f;
}

void
lefrSetNoiseTableCbk(lefrNoiseTableCbkFnType f)
{
    LEF_INIT;
    lefCallbacks->NoiseTableCbk = f;
}

void
lefrSetNonDefaultCbk(lefrNonDefaultCbkFnType f)
{
    LEF_INIT;
    lefCallbacks->NonDefaultCbk = f;
}

void
lefrSetObstructionCbk(lefrObstructionCbkFnType f)
{
    LEF_INIT;
    lefCallbacks->ObstructionCbk = f;
}

void
lefrSetOutputAntennaCbk(lefrDoubleCbkFnType f)
{
    LEF_INIT;
    lefCallbacks->OutputAntennaCbk = f;
}

void
lefrSetPinCbk(lefrPinCbkFnType f)
{
    LEF_INIT;
    lefCallbacks->PinCbk = f;
}

void
lefrSetPropBeginCbk(lefrVoidCbkFnType f)
{
    LEF_INIT;
    lefCallbacks->PropBeginCbk = f;
}

void
lefrSetPropCbk(lefrPropCbkFnType f)
{
    LEF_INIT;
    lefCallbacks->PropCbk = f;
}

void
lefrSetPropEndCbk(lefrVoidCbkFnType f)
{
    LEF_INIT;
    lefCallbacks->PropEndCbk = f;
}

void
lefrSetSiteCbk(lefrSiteCbkFnType f)
{
    LEF_INIT;
    lefCallbacks->SiteCbk = f;
}

void
lefrSetSpacingBeginCbk(lefrVoidCbkFnType f)
{
    LEF_INIT;
    lefCallbacks->SpacingBeginCbk = f;
}

void
lefrSetSpacingCbk(lefrSpacingCbkFnType f)
{
    LEF_INIT;
    lefCallbacks->SpacingCbk = f;
}

void
lefrSetSpacingEndCbk(lefrVoidCbkFnType f)
{
    LEF_INIT;
    lefCallbacks->SpacingEndCbk = f;
}

void
lefrSetTimingCbk(lefrTimingCbkFnType f)
{
    LEF_INIT;
    lefCallbacks->TimingCbk = f;
}

void
lefrSetUnitsCbk(lefrUnitsCbkFnType f)
{
    LEF_INIT;
    lefCallbacks->UnitsCbk = f;
}

void
lefrSetUseMinSpacingCbk(lefrUseMinSpacingCbkFnType f)
{
    LEF_INIT;
    lefCallbacks->UseMinSpacingCbk = f;
}

void
lefrSetVersionCbk(lefrDoubleCbkFnType f)
{
    LEF_INIT;
    lefCallbacks->VersionCbk = f;
}

void
lefrSetVersionStrCbk(lefrStringCbkFnType f)
{
    LEF_INIT;
    lefCallbacks->VersionStrCbk = f;
}

void
lefrSetViaCbk(lefrViaCbkFnType f)
{
    LEF_INIT;
    lefCallbacks->ViaCbk = f;
}

void
lefrSetViaRuleCbk(lefrViaRuleCbkFnType f)
{
    LEF_INIT;
    lefCallbacks->ViaRuleCbk = f;
}

int
lefrLineNumber()
{
    // Compatibility feature: in old versions the translators,  
    // the function can be called before lefData initialization. 
    return lefData ? lefData->lef_nlines : 0; 
}

void
lefrSetLogFunction(LEFI_LOG_FUNCTION f)
{
    LEF_INIT;
    lefSettings->ErrorLogFunction = f;
}

void
lefrSetWarningLogFunction(LEFI_WARNING_LOG_FUNCTION f)
{
    LEF_INIT;
    lefSettings->WarningLogFunction = f;
}

void
lefrSetMallocFunction(LEFI_MALLOC_FUNCTION f)
{
    LEF_INIT;
    lefSettings->MallocFunction = f;
}

void
lefrSetReallocFunction(LEFI_REALLOC_FUNCTION f)
{
    LEF_INIT;
    lefSettings->ReallocFunction = f;
}

void
lefrSetFreeFunction(LEFI_FREE_FUNCTION f)
{
    LEF_INIT;
    lefSettings->FreeFunction = f;
}

void
lefrSetLineNumberFunction(LEFI_LINE_NUMBER_FUNCTION f)
{
    LEF_INIT;
    lefSettings->LineNumberFunction = f;
}

void
lefrSetDeltaNumberLines(int numLines)
{
    LEF_INIT;
    lefSettings->DeltaNumberLines = numLines;
}

// from the lexer

void
lefrSetShiftCase()
{
    LEF_INIT;
    lefSettings->ShiftCase = 1;
}

void
lefrSetCommentChar(char c)
{
    LEF_INIT;
    lefSettings->CommentChar = c;
}

void
lefrSetCaseSensitivity(int caseSense)
{
    LEF_INIT;
    lefSettings->CaseSensitive = caseSense;
    lefSettings->CaseSensitiveSet = TRUE;
    if (lefData) {
        lefData->namesCaseSensitive = caseSense;
    }
}

void
lefrSetRelaxMode()
{
    LEF_INIT;
    lefSettings->RelaxMode = TRUE;
}

void
lefrUnsetRelaxMode()
{
    LEF_INIT;
    lefSettings->RelaxMode = FALSE;
}

void
lefrSetVersionValue(const char *version)
{
    LEF_INIT;
    lefSettings->VersionNum = convert_name2num(version);
}

void
lefrSetOpenLogFileAppend()
{
    LEF_INIT;
    lefSettings->LogFileAppend = TRUE;
}

void
lefrUnsetOpenLogFileAppend()
{
    LEF_INIT;
    lefSettings->LogFileAppend = FALSE;
}

void
lefrSetReadFunction(LEFI_READ_FUNCTION f)
{
    LEF_INIT;
    lefSettings->ReadFunction = f;
}

void
lefrUnsetReadFunction()
{
    LEF_INIT;
    lefSettings->ReadFunction = 0;
}

// Set the maximum number of warnings
//
// *****************************************************************************

void
lefrSetAntennaInoutWarnings(int warn)
{
    LEF_INIT;
    lefSettings->AntennaInoutWarnings = warn;
}

void
lefrSetAntennaInputWarnings(int warn)
{
    LEF_INIT;
    lefSettings->AntennaInputWarnings = warn;
}

void
lefrSetAntennaOutputWarnings(int warn)
{
    LEF_INIT;
    lefSettings->AntennaOutputWarnings = warn;
}

void
lefrSetArrayWarnings(int warn)
{
    LEF_INIT;
    lefSettings->ArrayWarnings = warn;
}

void
lefrSetCaseSensitiveWarnings(int warn)
{
    LEF_INIT;
    lefSettings->CaseSensitiveWarnings = warn;
}

void
lefrSetCorrectionTableWarnings(int warn)
{
    LEF_INIT;
    lefSettings->CorrectionTableWarnings = warn;
}

void
lefrSetDielectricWarnings(int warn)
{
    LEF_INIT;
    lefSettings->DielectricWarnings = warn;
}

void
lefrSetEdgeRateThreshold1Warnings(int warn)
{
    LEF_INIT;
    lefSettings->EdgeRateThreshold1Warnings = warn;
}

void
lefrSetEdgeRateThreshold2Warnings(int warn)
{
    LEF_INIT;
    lefSettings->EdgeRateThreshold2Warnings = warn;
}

void
lefrSetEdgeRateScaleFactorWarnings(int warn)
{
    LEF_INIT;
    lefSettings->EdgeRateScaleFactorWarnings = warn;
}

void
lefrSetInoutAntennaWarnings(int warn)
{
    LEF_INIT;
    lefSettings->InoutAntennaWarnings = warn;
}

void
lefrSetInputAntennaWarnings(int warn)
{
    LEF_INIT;
    lefSettings->InputAntennaWarnings = warn;
}

void
lefrSetIRDropWarnings(int warn)
{
    LEF_INIT;
    lefSettings->IRDropWarnings = warn;
}

void
lefrSetLayerWarnings(int warn)
{
    LEF_INIT;
    lefSettings->LayerWarnings = warn;
}

void
lefrSetMacroWarnings(int warn)
{
    LEF_INIT;
    lefSettings->MacroWarnings = warn;
}

void
lefrSetMaxStackViaWarnings(int warn)
{
    LEF_INIT;
    lefSettings->MaxStackViaWarnings = warn;
}

void
lefrSetMinFeatureWarnings(int warn)
{
    LEF_INIT;
    lefSettings->MinFeatureWarnings = warn;
}

void
lefrSetNoiseMarginWarnings(int warn)
{
    LEF_INIT;
    lefSettings->NoiseMarginWarnings = warn;
}

void
lefrSetNoiseTableWarnings(int warn)
{
    LEF_INIT;
    lefSettings->NoiseTableWarnings = warn;
}

void
lefrSetNonDefaultWarnings(int warn)
{
    LEF_INIT;
    lefSettings->NonDefaultWarnings = warn;
}

void
lefrSetNoWireExtensionWarnings(int warn)
{
    LEF_INIT;
    lefSettings->NoWireExtensionWarnings = warn;
}

void
lefrSetOutputAntennaWarnings(int warn)
{
    LEF_INIT;
    lefSettings->OutputAntennaWarnings = warn;
}

void
lefrSetPinWarnings(int warn)
{
    LEF_INIT;
    lefSettings->PinWarnings = warn;
}

void
lefrSetSiteWarnings(int warn)
{
    LEF_INIT;
    lefSettings->SiteWarnings = warn;
}

void
lefrSetSpacingWarnings(int warn)
{
    LEF_INIT;
    lefSettings->SpacingWarnings = warn;
}

void
lefrSetTimingWarnings(int warn)
{
    LEF_INIT;
    lefSettings->TimingWarnings = warn;
}

void
lefrSetUnitsWarnings(int warn)
{
    LEF_INIT;
    lefSettings->UnitsWarnings = warn;
}

void
lefrSetUseMinSpacingWarnings(int warn)
{
    LEF_INIT;
    lefSettings->UseMinSpacingWarnings = warn;
}

void
lefrSetViaRuleWarnings(int warn)
{
    LEF_INIT;
    lefSettings->ViaRuleWarnings = warn;
}

void
lefrSetViaWarnings(int warn)
{
    LEF_INIT;
    lefSettings->ViaWarnings = warn;
}

void
lefrDisablePropStrProcess()
{
    LEF_INIT;
    lefSettings->DisPropStrProcess = 1;
}

void
lefrRegisterLef58Type(const char *lef58Type,
                      const char *layerType)
{
    LEF_INIT;
    const char *typeLayers[] = {layerType, ""};

    lefSettings->addLef58Type(lef58Type, typeLayers);
}

END_LEFDEF_PARSER_NAMESPACE

