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


#ifndef CDEFIASSERTION_H
#define CDEFIASSERTION_H

#include <stdio.h>
#include "defiTypedefs.h"

/* Struct holds the data for one assertion/constraint.                        */
/* An assertion or constraint is either a net/path rule or a                  */
/* wired logic rule.                                                          */
/*                                                                            */
/*  A net/path rule is an item or list of items plus specifications.          */
/*    The specifications are: rise/fall min/max.                              */
/*    The items are a list of (one or more) net names or paths or a           */
/*    combination of both.                                                    */
/*                                                                            */
/*  A wired logic rule is a netname and a distance.                           */
/*                                                                            */
/*  We will NOT allow the mixing of wired logic rules and net/path delays     */
/*  in the same assertion/constraint.                                         */
/*                                                                            */
/*  We will allow the rule to be a sum of sums (which will be interpreted     */
/*  as just one list).                                                        */
/*                                                                            */

EXTERN int defiAssertion_isAssertion (const defiAssertion* obj);
EXTERN int defiAssertion_isConstraint (const defiAssertion* obj);
EXTERN int defiAssertion_isWiredlogic (const defiAssertion* obj);
EXTERN int defiAssertion_isDelay (const defiAssertion* obj);
EXTERN int defiAssertion_isSum (const defiAssertion* obj);
EXTERN int defiAssertion_isDiff (const defiAssertion* obj);
EXTERN int defiAssertion_hasRiseMin (const defiAssertion* obj);
EXTERN int defiAssertion_hasRiseMax (const defiAssertion* obj);
EXTERN int defiAssertion_hasFallMin (const defiAssertion* obj);
EXTERN int defiAssertion_hasFallMax (const defiAssertion* obj);
EXTERN double defiAssertion_riseMin (const defiAssertion* obj);
EXTERN double defiAssertion_riseMax (const defiAssertion* obj);
EXTERN double defiAssertion_fallMin (const defiAssertion* obj);
EXTERN double defiAssertion_fallMax (const defiAssertion* obj);
EXTERN const char* defiAssertion_netName (const defiAssertion* obj);
EXTERN double defiAssertion_distance (const defiAssertion* obj);
EXTERN int defiAssertion_numItems (const defiAssertion* obj);
EXTERN int defiAssertion_isPath (const defiAssertion* obj, int  index);
EXTERN int defiAssertion_isNet (const defiAssertion* obj, int  index);
EXTERN void defiAssertion_path (const defiAssertion* obj, int  index, char**  fromInst, char**  fromPin, char**  toInst, char**  toPin);
EXTERN void defiAssertion_net (const defiAssertion* obj, int  index, char**  netName);

EXTERN void defiAssertion_print (const defiAssertion* obj, FILE*  f);

#endif
