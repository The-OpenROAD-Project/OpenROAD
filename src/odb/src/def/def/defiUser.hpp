// *****************************************************************************
// *****************************************************************************
// Copyright 2013, Cadence Design Systems
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

/*
 * User header file for the DEF Interface.  This includes
 * all of the header files which are relevant to both the
 * reader and the writer.
 *
 * defrReader.h and defwWriter.h include this file, so that
 * an application only needs to include either defwReader.h
 * or defwWriter.h.
 */

#ifndef DEFI_USER_H
#define DEFI_USER_H

/* General utilities. */
/* #include "defiMalloc.hpp" */
/* #include "defiUtils.hpp" */

/*
 * API objects
 */
#include "src/odb/src/def/def/defiAssertion.hpp"
#include "src/odb/src/def/def/defiBlockage.hpp"
#include "src/odb/src/def/def/defiComponent.hpp"
#include "src/odb/src/def/def/defiDebug.hpp"
#include "src/odb/src/def/def/defiFPC.hpp"
#include "src/odb/src/def/def/defiFill.hpp"
#include "src/odb/src/def/def/defiGroup.hpp"
#include "src/odb/src/def/def/defiIOTiming.hpp"
#include "src/odb/src/def/def/defiKRDefs.hpp"
#include "src/odb/src/def/def/defiNet.hpp"
#include "src/odb/src/def/def/defiNonDefault.hpp"
#include "src/odb/src/def/def/defiPartition.hpp"
#include "src/odb/src/def/def/defiPath.hpp"
#include "src/odb/src/def/def/defiPinCap.hpp"
#include "src/odb/src/def/def/defiPinProp.hpp"
#include "src/odb/src/def/def/defiProp.hpp"
#include "src/odb/src/def/def/defiPropType.hpp"
#include "src/odb/src/def/def/defiRegion.hpp"
#include "src/odb/src/def/def/defiRowTrack.hpp"
#include "src/odb/src/def/def/defiScanchain.hpp"
#include "src/odb/src/def/def/defiSite.hpp"
#include "src/odb/src/def/def/defiSlot.hpp"
#include "src/odb/src/def/def/defiTimingDisable.hpp"
#include "src/odb/src/def/def/defiVia.hpp"

BEGIN_DEF_PARSER_NAMESPACE

/* NEW CALLBACK - If you are creating a new .cpp and .hpp file to
 * describe a new class of object in the parser, then add a reference
 * to the .hpp here.
 *
 *  You must also add an entry for the .h and the .hpp in the package_list
 * file of the ../../../release directory. */

END_DEF_PARSER_NAMESPACE

#endif
