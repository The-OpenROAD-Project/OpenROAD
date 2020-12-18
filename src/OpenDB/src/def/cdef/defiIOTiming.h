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


#ifndef CDEFIIOTIMING_H
#define CDEFIIOTIMING_H

#include <stdio.h>
#include "defiTypedefs.h"

EXTERN int defiIOTiming_hasVariableRise (const defiIOTiming* obj);
EXTERN int defiIOTiming_hasVariableFall (const defiIOTiming* obj);
EXTERN int defiIOTiming_hasSlewRise (const defiIOTiming* obj);
EXTERN int defiIOTiming_hasSlewFall (const defiIOTiming* obj);
EXTERN int defiIOTiming_hasCapacitance (const defiIOTiming* obj);
EXTERN int defiIOTiming_hasDriveCell (const defiIOTiming* obj);
EXTERN int defiIOTiming_hasFrom (const defiIOTiming* obj);
EXTERN int defiIOTiming_hasTo (const defiIOTiming* obj);
EXTERN int defiIOTiming_hasParallel (const defiIOTiming* obj);

EXTERN const char* defiIOTiming_inst (const defiIOTiming* obj);
EXTERN const char* defiIOTiming_pin (const defiIOTiming* obj);
EXTERN double defiIOTiming_variableFallMin (const defiIOTiming* obj);
EXTERN double defiIOTiming_variableRiseMin (const defiIOTiming* obj);
EXTERN double defiIOTiming_variableFallMax (const defiIOTiming* obj);
EXTERN double defiIOTiming_variableRiseMax (const defiIOTiming* obj);
EXTERN double defiIOTiming_slewFallMin (const defiIOTiming* obj);
EXTERN double defiIOTiming_slewRiseMin (const defiIOTiming* obj);
EXTERN double defiIOTiming_slewFallMax (const defiIOTiming* obj);
EXTERN double defiIOTiming_slewRiseMax (const defiIOTiming* obj);
EXTERN double defiIOTiming_capacitance (const defiIOTiming* obj);
EXTERN const char* defiIOTiming_driveCell (const defiIOTiming* obj);
EXTERN const char* defiIOTiming_from (const defiIOTiming* obj);
EXTERN const char* defiIOTiming_to (const defiIOTiming* obj);
EXTERN double defiIOTiming_parallel (const defiIOTiming* obj);

  /* debug print                                                              */
EXTERN void defiIOTiming_print (const defiIOTiming* obj, FILE*  f);

#endif
