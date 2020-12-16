/* ************************************************************************** */
/* ************************************************************************** */
/* ATTENTION: THIS IS AN AUTO-GENERATED FILE. DO NOT CHANGE IT!               */
/* ************************************************************************** */
/* ************************************************************************** */
/* Copyright 2012 - 2013, Cadence Design Systems                              */
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


#ifndef CLEFIUNITS_H
#define CLEFIUNITS_H

#include <stdio.h>
#include "lefiTypedefs.h"

EXTERN int lefiUnits_hasDatabase (const lefiUnits* obj);
EXTERN int lefiUnits_hasCapacitance (const lefiUnits* obj);
EXTERN int lefiUnits_hasResistance (const lefiUnits* obj);
EXTERN int lefiUnits_hasTime (const lefiUnits* obj);
EXTERN int lefiUnits_hasPower (const lefiUnits* obj);
EXTERN int lefiUnits_hasCurrent (const lefiUnits* obj);
EXTERN int lefiUnits_hasVoltage (const lefiUnits* obj);
EXTERN int lefiUnits_hasFrequency (const lefiUnits* obj);

EXTERN const char* lefiUnits_databaseName (const lefiUnits* obj);
EXTERN double lefiUnits_databaseNumber (const lefiUnits* obj);
EXTERN double lefiUnits_capacitance (const lefiUnits* obj);
EXTERN double lefiUnits_resistance (const lefiUnits* obj);
EXTERN double lefiUnits_time (const lefiUnits* obj);
EXTERN double lefiUnits_power (const lefiUnits* obj);
EXTERN double lefiUnits_current (const lefiUnits* obj);
EXTERN double lefiUnits_voltage (const lefiUnits* obj);
EXTERN double lefiUnits_frequency (const lefiUnits* obj);

  /* Debug print                                                              */
EXTERN void lefiUnits_print (const lefiUnits* obj, FILE*  f);

#endif
