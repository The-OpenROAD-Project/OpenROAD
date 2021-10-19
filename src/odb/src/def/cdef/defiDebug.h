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
/*  $Author: dell $                                                           */
/*  $Revision: #1 $                                                           */
/*  $Date: 2017/06/06 $                                                       */
/*  $State:  $                                                                */
/* ************************************************************************** */
/* ************************************************************************** */


#ifndef CDEFIDEBUG_H
#define CDEFIDEBUG_H

#include <stdio.h>
#include "defiTypedefs.h"

/* Set flag */
EXTERN void defiSetDebug (int  num, int  value);

/* Read flag */
EXTERN int defiDebug (int  num);

/* Error loggin function */
EXTERN void defiError (int  check, int  msgNum, const char*  message);

/* for auto upshifting names in case insensitive files */
EXTERN const char* upperCase (const char*  c);
EXTERN const char* DEFCASE (const char*  ch);

#endif
