///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2018, The Regents of the University of California
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
///////////////////////////////////////////////////////////////////////////////
/**************************************************************************
 * Copyright(c) 2018 Regents of the University of California
 *              Kwangsoo Han, Andrew B. Kahng and Sriram Venkatesh
 * Contact      kwhan@ucsd.edu, abk@cs.ucsd.edu, srvenkat@ucsd.edu
 * Affiliation: Computer Science and Engineering Department, UC San Diego,
 *              La Jolla, CA 92093-0404, USA
 *
 *************************************************************************/

/**************************************************************************
 * UCSD Prim-Dijkstra Revisited
 * argument.cpp
 *************************************************************************/

#include "argument.h"
#include "mystring.h"
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <cstring>

using   namespace   std;

/*** Default Parameters **************************************************/
CArgument::CArgument(){
    num_nets            = 10000;
    num_terminals       = 64;
    verbose             = 0;
    alpha1               = 0.3;
    alpha2               = 0.45;
    alpha3               = 0;
    alpha4               = 0;
    margin               = 1.1;
    root_idx            = 0;
    seed 		= 0;
    input_file 		= "";
    hvw_yes 		= 0;
    pdbu_yes 		= 0;
    beta	        = 1.4;
    distance  		= 2;
}

/*** Command Line Analyzer ***********************************************/
bool    CArgument::argument(int argc, char* argv[]){

    runOneNet = false;
    for (int i=1; i<argc; i++){
        string arg  = argv[i];

        if ((arg == "--help")){
            help();
            return  false;
        } 

        else if ((arg == "-h")){
            help();
            return  false;
        }
        
        else if(arg == "-o") {
            if (i+1 < argc){
                i++;

                if (argv[i][0] != '-') {
                    net_num = atoi(argv[i]);
                    runOneNet = true;
                } else {
                    cout << "*** Error :: number of nets is needed.." << endl;
                    return  false;
                }
            } else {
                cout << "*** Error :: number of nets is needed.." << endl;
                return  false;
            }
        }
        else if(arg == "-n") {
            if (i+1 < argc){
                i++;

                if (argv[i][0] != '-') {
                    num_nets = atoi(argv[i]);
                } else {
                    cout << "*** Error :: number of nets is needed.." << endl;
                    return  false;
                }
            } else {
                cout << "*** Error :: number of nets is needed.." << endl;
                return  false;
            }
        }
        else if(arg == "-t") {
            if (i+1 < argc){
                i++;

                if (argv[i][0] != '-') {
                    num_terminals = atoi(argv[i]);
                } else {
                    cout << "*** Error :: number of terminals for each net is needed.." << endl;
                    return  false;
                }
            } else {
                cout << "*** Error :: number of terminals for each net is needed.." << endl;
                return  false;
            }
        }
        else if(arg == "-root") {
            if (i+1 < argc){
                i++;

                if (argv[i][0] != '-') {
                    root_idx = atoi(argv[i]);
                } else {
                    cout << "*** Error :: index of root is needed.." << endl;
                    return  false;
                }
            } else {
                cout << "*** Error :: index of root is needed.." << endl;
                return  false;
            }
        }
        else if(arg == "-v") {
            if (i+1 < argc){
                i++;

                if (argv[i][0] != '-') {
                    verbose = atoi(argv[i]);
                } else {
                    cout << "*** Error :: verbose mode is needed.." << endl;
                    return  false;
                }
            } else {
                cout << "*** Error :: verbose mode is needed.." << endl;
                return  false;
            }
        }
        else if(arg == "-m1") {
            if (i+1 < argc){
                i++;

                if (argv[i][0] != '-') {
                    margin = atof(argv[i]);
                } else {
                    cout << "*** Error :: m1 (m1 >= 1) is needed.." << endl;
                    return  false;
                }
            } else {
                cout << "*** Error :: m1 (m1 >= 1) is needed.." << endl;
                return  false;
            }
        }
        else if(arg == "-a1") {
            if (i+1 < argc){
                i++;

                if (argv[i][0] != '-') {
                    alpha1 = atof(argv[i]);
                } else {
                    cout << "*** Error :: alpha for PD (0 <= alpha <= 1) is needed.." << endl;
                    return  false;
                }
            } else {
                cout << "*** Error :: alpha for PD (0 <= alpha <= 1) is needed.." << endl;
                return  false;
            }
        }
        else if(arg == "-a2") {
            if (i+1 < argc){
                i++;
                if (argv[i][0] != '-') {
                    alpha2 = atof(argv[i]);
                } else {
                    cout << "*** Error :: alpha for PD-II (0 <= alpha <= 1) is needed.." << endl;
                    return  false;
                }
            } else {
                cout << "*** Error :: alpha for PD-II (0 <= alpha <= 1) is needed.." << endl;
                return  false;
            }
        }
        else if(arg == "-s") {
            if (i+1 < argc){
                i++;

                if (argv[i][0] != '-') {
                    seed = atoi(argv[i]);
                } else {
                    cout << "*** Error :: seed (0 <= seed <= 2000) is needed.." << endl;
                    return  false;
                }
            } else {
                cout << "*** Error :: seed (0 <= seed <= 2000) is needed.." << endl;
                return  false;
            }
        }
        else if(arg == "-a3") {
            if (i+1 < argc){
                i++;

//                if (argv[i][0] != '-') {
                      alpha3 = atof(argv[i]);
//                } else {
//                    cout << "*** Error :: alpha2 (0 <= alpha2 <= 1) is needed.." << endl;
//                    return  false;
//                }
            } else {
                cout << "*** Error :: alpha3 (0 <= alpha3 <= 1) is needed.." << endl;
                return  false;
            }
        }
        else if(arg == "-a4") {
            if (i+1 < argc){
                i++;

//                if (argv[i][0] != '-') {
                      alpha4 = atof(argv[i]);
//                } else {
//                    cout << "*** Error :: alpha2 (0 <= alpha2 <= 1) is needed.." << endl;
//                    return  false;
//                }
            } else {
                cout << "*** Error :: alpha4 (0 <= alpha4 <= 1) is needed.." << endl;
                return  false;
            }
        }
        else if(arg == "-b") {
            if (i+1 < argc){
                i++;

//                if (argv[i][0] != '-') {
                    beta = atof(argv[i]);
//               } else {
//                    cout << "*** Error :: beta (0 <= beta <= 1) is needed.." << endl;
//                    return  false;
//                }
            } else {
                cout << "*** Error :: beta (0 <= beta <= 1) is needed.." << endl;
                return  false;
            }
        }
        else if(arg == "-d") {
            if (i+1 < argc){
                i++;

                if (argv[i][0] != '-') {
                    distance = atoi(argv[i]);
                } else {
                    cout << "*** Error :: distance (0 <= distance <= 2000) is needed.." << endl;
                    return  false;
                }
            } else {
                cout << "*** Error :: distance (0 <= distance <= 2000) is needed.." << endl;
                return  false;
            }
        }
        else if(arg == "-f") {
            if (i+1 < argc){
                i++;

                if (argv[i][0] != '-') {
                    input_file = argv[i];
                } else {
                    cout << "*** Error :: Filename not specified" << endl;
                    return  false;
                }
            } else {
                cout << "*** Error :: Filename not specified" << endl;
                return  false;
            }
        }
        else if(arg == "-hv") {
            if (i+1 < argc){
                i++;

                if (argv[i][0] != '-') {
                    hvw_yes = atoi(argv[i]);
                } else {
                    cout << "*** Error :: hvw option not specified" << endl;
                    return  false;
                }
            } else {
                cout << "*** Error :: Filename not specified" << endl;
                return  false;
            }
        }
        else if(arg == "-bu") {
            if (i+1 < argc){
                i++;

                if (argv[i][0] != '-') {
                    pdbu_yes = atoi(argv[i]);
                } else {
                    cout << "*** Error :: bu option not specified" << endl;
                    return  false;
                }
            } else {
                cout << "*** Error :: Filename not specified" << endl;
                return  false;
            }
        }
        else {
            cout << "*** Error :: Option(s) is(are) NOT applicable.." << endl;
            return  false;
        }
    }
    return  true;
}

void    CArgument::help(){
    cout << "-v \t verbose mode" << endl;
    cout << "-a1 \t alpha for PD (default: 0.3)" << endl;
    cout << "-a2 \t alpha for PD-II (default: 0.45)" << endl;
    cout << "-bu \t enable PD-II (default: off)" << endl;
    cout << "-m1 \t Upperbound of max pathlength degradation in DAS wirelength minimization (default: 1.1 --> allow 10% max PL degradation for WL optimization)" << endl;
    cout << "-f \t input file with pointset" << endl;
    cout << "-hv \t enable HoVW Steinerization and Detour-Aware Steinerization (default off)" << endl;
    cout << endl;
}
