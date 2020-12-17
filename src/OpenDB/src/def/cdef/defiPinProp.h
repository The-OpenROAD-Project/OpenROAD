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


#ifndef CDEFIPINPROP_H
#define CDEFIPINPROP_H

#include <stdio.h>
#include "defiTypedefs.h"

EXTERN int defiPinProp_isPin (const defiPinProp* obj);
EXTERN const char* defiPinProp_instName (const defiPinProp* obj);
EXTERN const char* defiPinProp_pinName (const defiPinProp* obj);

EXTERN int defiPinProp_numProps (const defiPinProp* obj);
EXTERN const char* defiPinProp_propName (const defiPinProp* obj, int  index);
EXTERN const char* defiPinProp_propValue (const defiPinProp* obj, int  index);
EXTERN double defiPinProp_propNumber (const defiPinProp* obj, int  index);
EXTERN char defiPinProp_propType (const defiPinProp* obj, int  index);
EXTERN int defiPinProp_propIsNumber (const defiPinProp* obj, int  index);
EXTERN int defiPinProp_propIsString (const defiPinProp* obj, int  index);

EXTERN void defiPinProp_print (const defiPinProp* obj, FILE*  f);

#endif
