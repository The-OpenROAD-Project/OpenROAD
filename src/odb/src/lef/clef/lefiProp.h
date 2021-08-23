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


#ifndef CLEFIPROP_H
#define CLEFIPROP_H

#include <stdio.h>
#include "lefiTypedefs.h"

/* Struct holds the data for one property.                                    */

EXTERN const char* lefiProp_string (const lefiProp* obj);
EXTERN const char* lefiProp_propType (const lefiProp* obj);
EXTERN const char* lefiProp_propName (const lefiProp* obj);
EXTERN char lefiProp_dataType (const lefiProp* obj);
      /* either I:integer R:real S:string Q:quotedstring                      */
      /* N:property name is not defined in the property definition section    */
EXTERN int lefiProp_hasNumber (const lefiProp* obj);
EXTERN int lefiProp_hasRange (const lefiProp* obj);
EXTERN int lefiProp_hasString (const lefiProp* obj);
EXTERN int lefiProp_hasNameMapString (const lefiProp* obj);
EXTERN double lefiProp_number (const lefiProp* obj);
EXTERN double lefiProp_left (const lefiProp* obj);
EXTERN double lefiProp_right (const lefiProp* obj);

EXTERN void lefiProp_print (const lefiProp* obj, FILE*  f);

            /* N:property name is not defined.                                */

#endif
