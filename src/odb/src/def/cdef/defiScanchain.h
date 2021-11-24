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
/*  $Author: dell $                                                                  */
/*  $Revision: #1 $                                                                */
/*  $Date: 2017/06/06 $                                                                    */
/*  $State:  $                                                                */
/* ************************************************************************** */
/* ************************************************************************** */


#ifndef CDEFISCANCHAIN_H
#define CDEFISCANCHAIN_H

#include <stdio.h>
#include "defiTypedefs.h"

EXTERN int defiOrdered_num (const defiOrdered* obj);
EXTERN char** defiOrdered_inst (const defiOrdered* obj);
EXTERN char** defiOrdered_in (const defiOrdered* obj);
EXTERN char** defiOrdered_out (const defiOrdered* obj);
EXTERN int* defiOrdered_bits (const defiOrdered* obj);

/* Struct holds the data for one Scan chain.                                  */
/*                                                                            */

EXTERN const char* defiScanchain_name (const defiScanchain* obj);
EXTERN int defiScanchain_hasStart (const defiScanchain* obj);
EXTERN int defiScanchain_hasStop (const defiScanchain* obj);
EXTERN int defiScanchain_hasFloating (const defiScanchain* obj);
EXTERN int defiScanchain_hasOrdered (const defiScanchain* obj);
EXTERN int defiScanchain_hasCommonInPin (const defiScanchain* obj);
EXTERN int defiScanchain_hasCommonOutPin (const defiScanchain* obj);
EXTERN int defiScanchain_hasPartition (const defiScanchain* obj);
EXTERN int defiScanchain_hasPartitionMaxBits (const defiScanchain* obj);

  /* If the pin part of these routines were not supplied in the DEF           */
  /* then a NULL pointer will be returned.                                    */
EXTERN void defiScanchain_start (const defiScanchain* obj, char**  inst, char**  pin);
EXTERN void defiScanchain_stop (const defiScanchain* obj, char**  inst, char**  pin);

  /* There could be many ORDERED constructs in the DEF.  The data in          */
  /* each ORDERED construct is stored in its own array.  The numOrderedLists( */
  /* routine tells how many lists there are.                                  */
EXTERN int defiScanchain_numOrderedLists (const defiScanchain* obj);

  /* This routine will return an array of instances and                       */
  /* an array of in and out pins.                                             */
  /* The number if things in the arrays is returned in size.                  */
  /* The inPin and outPin entry is optional for each instance.                */
  /* If an entry is not given, then that char* is NULL.                       */
  /* For example if the second instance has                                   */
  /* instnam= "FOO" and IN="A", but no OUT given, then inst[1] points         */
  /* to "FOO"  inPin[1] points to "A" and outPin[1] is a NULL pointer.        */
EXTERN void defiScanchain_ordered (const defiScanchain* obj, int  index, int*  size, char***  inst, char***  inPin, char***  outPin, int**  bits);

  /* All of the floating constructs in the scan chain are                     */
  /* stored in this one array.                                                */
  /* If the IN or OUT of an entry is not supplied then the array will have    */
  /* a NULL pointer in that place.                                            */
EXTERN void defiScanchain_floating (const defiScanchain* obj, int*  size, char***  inst, char***  inPin, char***  outPin, int**  bits);

EXTERN const char* defiScanchain_commonInPin (const defiScanchain* obj);
EXTERN const char* defiScanchain_commonOutPin (const defiScanchain* obj);

EXTERN const char* defiScanchain_partitionName (const defiScanchain* obj);
EXTERN int defiScanchain_partitionMaxBits (const defiScanchain* obj);

EXTERN void defiScanchain_print (const defiScanchain* obj, FILE*  f);

#endif
