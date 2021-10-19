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


#ifndef CDEFIPROP_H
#define CDEFIPROP_H

#include <stdio.h>
#include "defiTypedefs.h"

/* Struct holds the data for one property.                                    */

EXTERN const char* defiProp_string (const defiProp* obj);
EXTERN const char* defiProp_propType (const defiProp* obj);
EXTERN const char* defiProp_propName (const defiProp* obj);
EXTERN char defiProp_dataType (const defiProp* obj);
       /* either I:integer R:real S:string Q:quotedstring N:nameMapString     */
EXTERN int defiProp_hasNumber (const defiProp* obj);
EXTERN int defiProp_hasRange (const defiProp* obj);
EXTERN int defiProp_hasString (const defiProp* obj);
EXTERN int defiProp_hasNameMapString (const defiProp* obj);
EXTERN double defiProp_number (const defiProp* obj);
EXTERN double defiProp_left (const defiProp* obj);
EXTERN double defiProp_right (const defiProp* obj);

EXTERN void defiProp_print (const defiProp* obj, FILE*  f);

                        /*   N:nameMapString                                  */

#endif
