// *****************************************************************************
// *****************************************************************************
// Copyright 2012 - 2013, Cadence Design Systems
// 
// This  file  is  part  of  the  Cadence  LEF/DEF  Open   Source
// Distribution,  Product Version 5.8. 
// 
// Licensed under the Apache License, Version 2.0 (the "License");
//    you may not use this file except in compliance with the License.
//    You may obtain a copy of the License at
// 
//        http://www.apache.org/licenses/LICENSE-2.0
// 
//    Unless required by applicable law or agreed to in writing, software
//    distributed under the License is distributed on an "AS IS" BASIS,
//    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
//    implied. See the License for the specific language governing
//    permissions and limitations under the License.
// 
// For updates, support, or to become part of the LEF/DEF Community,
// check www.openeda.org for details.
// 
//  $Author: dell $
//  $Revision: #1 $
//  $Date: 2017/06/06 $
//  $State:  $
// *****************************************************************************
// *****************************************************************************

#ifndef lefiKRDEFS_h
#define lefiKRDEFS_h

#define BEGIN_LEFDEF_PARSER_NAMESPACE namespace LefDefParser {
#define END_LEFDEF_PARSER_NAMESPACE }
#define USE_LEFDEF_PARSER_NAMESPACE using namespace LefDefParser;

// Added by mgwoo
#define LEF_COPY_CONSTRUCTOR_H(cname) cname(const cname & prev)
#define LEF_COPY_CONSTRUCTOR_C(cname) cname::cname(const cname & prev)

#define LEF_ASSIGN_OPERATOR_H(cname) cname& operator=(const cname & prev)
#define LEF_ASSIGN_OPERATOR_C(cname) cname& cname::operator=(const cname & prev)
#define CHECK_SELF_ASSIGN                                               \
{                                                                       \
    if (this == &prev) {                                                \
        return *this;                                                   \
    }                                                                   \
}

#define LEF_COPY_FUNC(varname) {(varname) = prev.varname;}
#define LEF_MALLOC_FUNC(varname, vartype, length)                           \
{                                                                       \
    if( prev.varname ) {                                                \
        varname = (vartype*) lefMalloc(length);                         \
        memcpy(varname, prev.varname, length);                          \
    }                                                                   \
}

#define LEF_MALLOC_FUNC_WITH_OPERATOR(varname, vartype, length)             \
{                                                                       \
    if( prev.varname ) {                                                \
        varname = (vartype*) lefMalloc(length);                         \
        *(varname) = *(prev.varname);                                   \
    }                                                                   \
}


#define LEF_MALLOC_FUNC_FOR_2D(varname, vartype, length1, length2 )     \
{                                                                       \
    if(prev.varname) {                                                  \
        varname = (vartype**) lefMalloc( sizeof(vartype*) * length1 );  \
                                                                        \
        for(int i=0; i<length1; i++) {                                  \
            if( prev.varname[i] ) {                                     \
                varname[i] = (vartype*) lefMalloc( sizeof(vartype) * length2 );\
                *(varname[i]) = *(prev.varname[i]);                     \
            }                                                           \
            else {                                                      \
                varname[i] = 0;                                         \
            }                                                           \
        }                                                               \
    }                                                                   \
    else {                                                              \
        varname = 0;                                                    \
    }                                                                   \
}

#define LEF_MALLOC_FUNC_FOR_2D_ALWAYS_MALLOC(varname, vartype, length1, length2 ) \
{                                                                       \
    if(prev.varname) {                                                  \
        varname = (vartype**) lefMalloc( sizeof(vartype*) * length1 );  \
                                                                        \
        for(int i=0; i<length1; i++) {                                  \
            varname[i] = (vartype*) lefMalloc( sizeof(vartype) * length2 ); \
            if( prev.varname[i] ) {                                     \
                *(varname[i]) = *(prev.varname[i]);                     \
            }                                                           \
        }                                                               \
    }                                                                   \
    else {                                                              \
        varname = 0;                                                    \
    }                                                                   \
}

#define LEF_MALLOC_FUNC_FOR_2D_STR(varname, length )                        \
{                                                                       \
    if(prev.varname) {                                                  \
        varname = (char**) lefMalloc( sizeof(char*) * length );         \
                                                                        \
        for(int i=0; i<length; i++) {                                   \
            if( prev.varname[i] ) {                                     \
                int len = strlen(prev.varname[i])+1;                      \
                varname[i] = (char*) lefMalloc( len );                  \
                strcpy( varname[i], prev.varname[i]);                   \
            }                                                           \
            else {                                                      \
                varname[i] = 0;                                         \
            }                                                           \
        }                                                               \
    }                                                                   \
    else {                                                              \
        varname = 0;                                                    \
    }                                                                   \
}

#endif
