/* Authors: Lutong Wang and Bangqi Xu */
/*
 * Copyright (c) 2019, The Regents of the University of California
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the University nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE REGENTS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <iostream>
#include <chrono>
#include "FlexRoute.h"

using namespace std;
using namespace fr;

int main(int argc, char** argv) {
  using namespace std::chrono;
  high_resolution_clock::time_point t1 = high_resolution_clock::now();
  if (argc == 1) {
    cout <<"Error: usage ./TritonRoute -lef <LEF_FILE> -def <DEF_FILE> -guide <GUIDE_FILE> -output <OUTPUT_DEF>" <<endl;
    return 2;
  }

  if (argc == 2) {
    bool readSuccess = FlexRoute::readParams(argv[1]);
    if (!readSuccess) {
      cout <<"Error reading param file!!!" <<endl;
      return 2;
    }
  } else {
    argv++;
    argc--;
    while (argc--) {
      if (strcmp(*argv, "-lef") == 0) {
        argv++;
        argc--;
        LEF_FILE = *argv;
      } else if (strcmp(*argv, "-def") == 0) {
        argv++;
        argc--;
        DEF_FILE = *argv;
        REF_OUT_FILE = DEF_FILE; 
      } else if (strcmp(*argv, "-guide") == 0) {
        argv++;
        argc--;
        GUIDE_FILE = *argv;
      } else if (strcmp(*argv, "-threads") == 0) {
        argv++;
        argc--;
        sscanf(*argv, "%d", &MAX_THREADS);
      } else if (strcmp(*argv, "-output") == 0) {
        argv++;
        argc--;
        OUT_FILE = *argv;
      } else if (strcmp(*argv, "-verbose") == 0) {
        argv++;
        argc--;
        VERBOSE = atoi(*argv);
      } else {
        cout <<"ERROR: Illegal command line option: " <<*argv <<endl;
        return 2;
      }
      argv++;
    }
  }
  
  FlexRoute router;
  router.main();
  
  high_resolution_clock::time_point t2 = high_resolution_clock::now();
  duration<double> time_span = duration_cast<duration<double>>(t2 - t1);
  if (VERBOSE > 0) {
    cout <<endl <<"Runtime taken (hrt): " << time_span.count()    <<endl;
  }
  return 0;
}
