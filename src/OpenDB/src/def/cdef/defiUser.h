/* ************************************************************************** */
/* ************************************************************************** */
/* ATTENTION: THIS IS AN AUTO-GENERATED FILE. DO NOT CHANGE IT!               */
/* ************************************************************************** */
/* ************************************************************************** */
/* Copyright 2013, Cadence Design Systems                                     */
/*                                                                            */
/* This  file  is  part  of  the  Cadence  LEF/DEF  Open   Source             */
/* Distribution,  Product Version 5.8.                                        */
/*                                                                            */
/* Licensed under the Apache License, Version 2.0 (the "License");            */
/*    you may not use this file except in compliance with the License.        */
/*    You may obtain a copy of the License at                                 */
/*                                                                            */
/*        http://www.apache.org/licenses/LICENSE-2.0                          */
/*                                                                            */
/*    Unless required by applicable law or agreed to in writing, software     */
/*    distributed under the License is distributed on an "AS IS" BASIS,       */
/*    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or         */
/*    implied. See the License for the specific language governing            */
/*    permissions and limitations under the License.                          */
/*                                                                            */
/* For updates, support, or to become part of the LEF/DEF Community,          */
/* check www.openeda.org for details.                                         */
/*                                                                            */
/*  $Author: dell $                                                       */
/*  $Revision: #1 $                                                           */
/*  $Date: 2017/06/06 $                                                       */
/*  $State:  $                                                                */
/* ************************************************************************** */
/* ************************************************************************** */

/*
 * User header file for the DEF Interface.  This includes
 * all of the header files which are relevant to both the
 * reader and the writer.
 *
 * defrReader.h and defwWriter.h include this file, so that
 * an application only needs to include either defwReader.h
 * or defwWriter.h.
 */


#ifndef CDEFIUSER_H
#define CDEFIUSER_H

#include "defiAlias.h"
#include "defiAssertion.h"
#include "defiBlockage.h"
#include "defiComponent.h"
#include "defiDebug.h"
#include "defiFill.h"
#include "defiFPC.h"
#include "defiGroup.h"
#include "defiIOTiming.h"
#include "defiMisc.h"
#include "defiNet.h"
#include "defiNonDefault.h"
#include "defiPartition.h"
#include "defiPath.h"
#include "defiPinCap.h"
#include "defiPinProp.h"
#include "defiProp.h"
#include "defiPropType.h"
#include "defiRegion.h"
#include "defiRowTrack.h"
#include "defiScanchain.h"
#include "defiSite.h"
#include "defiSlot.h"
#include "defiTimingDisable.h"
#include "defiVia.h"


/* General utilities. */
/* #include "defiMalloc.hpp" */
/* #include "defiUtils.hpp" */

/*
 * API objects
 */

/* NEW CALLBACK - If you are creating a new .cpp and .hpp file to
 * describe a new class of object in the parser, then add a reference
 * to the .hpp here.
 *
 *  You must also add an entry for the .h and the .hpp in the package_list
 * file of the ../../../release directory. */

#endif
