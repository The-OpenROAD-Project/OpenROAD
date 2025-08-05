CUGR 2.0
======================================
CUGR 2.0, the successor of [CUGR](https://github.com/cuhk-eda/cu-gr), is a VLSI global routing tool developed by the research team supervised by Prof. Evangeline F. Y. Young at The Chinese University of Hong Kong (CUHK).
CUGR 2.0 is a detailed routability-driven global router and its solution quality is solely determined by the final detailed routing results.

Please read our paper for more technical details:

* Jinwei Liu, Evangeline F. Y. Young,
EDGE: Efficient DAG-Based Global Routing Engine,
ACM/IEEE Design Automation Conference (DAC), San Francisco, CA, USA, July 9-13, 2023.

(CUGR 2.0 supports ICCAD'19 benchmarks ([v2](http://iccad-contest.org/2019/Problem_C/iccad19_benchmarks_v2.tar.gz), [hidden](http://iccad-contest.org/2019/Problem_C/iccad19_hidden_benchmarks.tar.gz)).
This version of code is consistent with the DAC paper.)

## 1. How to Build

**Step 1:** Download the source code. For example,
```bash
$ git clone https://github.com/cuhk-eda/cu-gr-2
```

**Step 2:** Go to the project root and build by
```bash
$ cd cu-gr-2
$ scripts/build.py -o release
```

Note that this will generate two folders under the root, `build` and `run` (`build` contains intermediate files for build/compilation, while `run` contains binaries and auxiliary files).
More details are in [`scripts/build.py`](scripts/build.py).

### 1.1. Dependencies

* [GCC](https://gcc.gnu.org/) (version >= 5.5.0) or other working c++ compliers
* [CMake](https://cmake.org/) (version >= 2.8)
* [Boost](https://www.boost.org/) (version >= 1.58)
* [Python](https://www.python.org/) (version 3, optional, for utility scripts)
* [Innovus®](https://www.cadence.com/content/cadence-www/global/en_US/home/tools/digital-design-and-signoff/soc-implementation-and-floorplanning/innovus-implementation-system.html) (version 18.1, optional, for design rule checking and evaluation)
* [Rsyn](https://github.com/RsynTeam/rsyn-x) (a trimmed version is used, already added under folder `rsyn`)
* [Dr. CU](https://github.com/cuhk-eda/dr-cu) (v4.1.1, optional, official detailed router for ICCAD'19 Contest, [binary](http://iccad-contest.org/2019/Problem_C/drcu_june19.zip) is already included under the root)

## 2. License

READ THIS LICENSE AGREEMENT CAREFULLY BEFORE USING THIS PRODUCT. BY USING THIS PRODUCT YOU INDICATE YOUR ACCEPTANCE OF THE TERMS OF THE FOLLOWING AGREEMENT. THESE TERMS APPLY TO YOU AND ANY SUBSEQUENT LICENSEE OF THIS PRODUCT.



License Agreement for CUGR 2.0



Copyright (c) 2023 by The Chinese University of Hong Kong



All rights reserved



CU-SD LICENSE (adapted from the original BSD license) Redistribution of the any code, with or without modification, are permitted provided that the conditions below are met.



Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.



Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.



Neither the name nor trademark of the copyright holder or the author may be used to endorse or promote products derived from this software without specific prior written permission.



Users are entirely responsible, to the exclusion of the author, for compliance with (a) regulations set by owners or administrators of employed equipment, (b) licensing terms of any other software, and (c) local, national, and international regulations regarding use, including those regarding import, export, and use of encryption software.
