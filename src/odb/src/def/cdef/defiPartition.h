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


#ifndef CDEFIPARTITION_H
#define CDEFIPARTITION_H

#include <stdio.h>
#include "defiTypedefs.h"

EXTERN const char* defiPartition_name (const defiPartition* obj);
EXTERN char defiPartition_direction (const defiPartition* obj);
EXTERN const char* defiPartition_itemType (const defiPartition* obj);
EXTERN const char* defiPartition_pinName (const defiPartition* obj);
EXTERN const char* defiPartition_instName (const defiPartition* obj);

EXTERN int defiPartition_numPins (const defiPartition* obj);
EXTERN const char* defiPartition_pin (const defiPartition* obj, int  index);

EXTERN int defiPartition_isSetupRise (const defiPartition* obj);
EXTERN int defiPartition_isSetupFall (const defiPartition* obj);
EXTERN int defiPartition_isHoldRise (const defiPartition* obj);
EXTERN int defiPartition_isHoldFall (const defiPartition* obj);
EXTERN int defiPartition_hasMin (const defiPartition* obj);
EXTERN int defiPartition_hasMax (const defiPartition* obj);
EXTERN int defiPartition_hasRiseMin (const defiPartition* obj);
EXTERN int defiPartition_hasFallMin (const defiPartition* obj);
EXTERN int defiPartition_hasRiseMax (const defiPartition* obj);
EXTERN int defiPartition_hasFallMax (const defiPartition* obj);
EXTERN int defiPartition_hasRiseMinRange (const defiPartition* obj);
EXTERN int defiPartition_hasFallMinRange (const defiPartition* obj);
EXTERN int defiPartition_hasRiseMaxRange (const defiPartition* obj);
EXTERN int defiPartition_hasFallMaxRange (const defiPartition* obj);

EXTERN double defiPartition_partitionMin (const defiPartition* obj);
EXTERN double defiPartition_partitionMax (const defiPartition* obj);

EXTERN double defiPartition_riseMin (const defiPartition* obj);
EXTERN double defiPartition_fallMin (const defiPartition* obj);
EXTERN double defiPartition_riseMax (const defiPartition* obj);
EXTERN double defiPartition_fallMax (const defiPartition* obj);

EXTERN double defiPartition_riseMinLeft (const defiPartition* obj);
EXTERN double defiPartition_fallMinLeft (const defiPartition* obj);
EXTERN double defiPartition_riseMaxLeft (const defiPartition* obj);
EXTERN double defiPartition_fallMaxLeft (const defiPartition* obj);
EXTERN double defiPartition_riseMinRight (const defiPartition* obj);
EXTERN double defiPartition_fallMinRight (const defiPartition* obj);
EXTERN double defiPartition_riseMaxRight (const defiPartition* obj);
EXTERN double defiPartition_fallMaxRight (const defiPartition* obj);

  /* debug print                                                              */
EXTERN void defiPartition_print (const defiPartition* obj, FILE*  f);

#endif
