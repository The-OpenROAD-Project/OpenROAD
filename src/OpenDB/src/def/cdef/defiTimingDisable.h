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


#ifndef CDEFITIMINGDISABLE_H
#define CDEFITIMINGDISABLE_H

#include <stdio.h>
#include "defiTypedefs.h"

/* A Timing disable can be a from-to  or a thru or a macro.                   */
/*   A macro is either a fromto macro or a thru macro.                        */

EXTERN int defiTimingDisable_hasMacroThru (const defiTimingDisable* obj);
EXTERN int defiTimingDisable_hasMacroFromTo (const defiTimingDisable* obj);
EXTERN int defiTimingDisable_hasThru (const defiTimingDisable* obj);
EXTERN int defiTimingDisable_hasFromTo (const defiTimingDisable* obj);
EXTERN int defiTimingDisable_hasReentrantPathsFlag (const defiTimingDisable* obj);

EXTERN const char* defiTimingDisable_fromPin (const defiTimingDisable* obj);
EXTERN const char* defiTimingDisable_toPin (const defiTimingDisable* obj);
EXTERN const char* defiTimingDisable_fromInst (const defiTimingDisable* obj);
EXTERN const char* defiTimingDisable_toInst (const defiTimingDisable* obj);
EXTERN const char* defiTimingDisable_macroName (const defiTimingDisable* obj);
EXTERN const char* defiTimingDisable_thruPin (const defiTimingDisable* obj);
EXTERN const char* defiTimingDisable_thruInst (const defiTimingDisable* obj);

  /* debug print                                                              */
EXTERN void defiTimingDisable_print (const defiTimingDisable* obj, FILE*  f);

#endif
