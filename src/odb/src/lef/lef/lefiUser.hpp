// *****************************************************************************
// *****************************************************************************
// Copyright 2012 - 2013, Cadence Design Systems
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
//  $Date: 2020/09/29 $
//  $State:  $
// *****************************************************************************
// *****************************************************************************

//  User header file for the LEF Interface.  This includes
//  all of the header files which are relevant to both the
//  reader and the writer.
//
//  lefrReader.h and lefwWriter.h include this file, so that
//  an application only needs to include either lefrReader.h(pp)
//  or lefwWriter.h(pp).
//

#ifndef LEFI_USER_H
#define LEFI_USER_H

#include "src/odb/src/lef/lef/lefiArray.hpp"
#include "src/odb/src/lef/lef/lefiCrossTalk.hpp"
#include "src/odb/src/lef/lef/lefiDebug.hpp"
#include "src/odb/src/lef/lef/lefiKRDefs.hpp"
#include "src/odb/src/lef/lef/lefiLayer.hpp"
#include "src/odb/src/lef/lef/lefiMacro.hpp"
#include "src/odb/src/lef/lef/lefiMisc.hpp"
#include "src/odb/src/lef/lef/lefiNonDefault.hpp"
#include "src/odb/src/lef/lef/lefiProp.hpp"
#include "src/odb/src/lef/lef/lefiPropType.hpp"
#include "src/odb/src/lef/lef/lefiUnits.hpp"
#include "src/odb/src/lef/lef/lefiVia.hpp"
#include "src/odb/src/lef/lef/lefiViaRule.hpp"

BEGIN_LEF_PARSER_NAMESPACE

// NEW CALLBACK add the reference here

END_LEF_PARSER_NAMESPACE

#endif
