# TritonRoute
**TritonRoute** is an open source detailed router for modern industrial
designs. The router consists of several main building blocks, including
pin access analysis, track assignment, initial detailed routing,
search and repair, and a DRC engine. The initial development of the
[router](https://vlsicad.ucsd.edu/Publications/Conferences/363/c363.pdf)
is inspired by the
[ISPD-2018 initial detailed routing contest](http://www.ispd.cc/contests/18/).
However, the current framework differs and is built from scratch, aiming
for an industrial-oriented scalable and flexible flow.

TritonRoute was developed by graduate students Lutong Wang and Bangqi Xu from
UC San Diego, and serves as the detailed router in the
[OpenROAD](https://theopenroadproject.org/) project.

TritonRoute provides industry standard LEF/DEF interface with
support of [ISPD-2018](http://www.ispd.cc/contests/18/) and
[ISPD-2019](http://www.ispd.cc/contests/19/) contest-compatible route guide
format.

## Installation ##
TritonRoute is tested in 64-bit CentOS 6/7 environments with the following
prerequisites:
* A compatible C++ compiler supporting C++17 (GCC 7 and above)
* Boost >= 1.68.0
* OpenMP >= 4.5
* Bison >= 3.0.4
* zlib >= 1.2.7
* CMake >= 3.1

## Supported Technologies ##
* ISPD-2018 and ISPD-2019 Initial Detailed Routing Contests

## References ##
Please cite the following paper(s) for publication:
* A. B. Kahng, L. Wang and B. Xu, "TritonRoute: The Open Source Detailed Router", IEEE Transactions on Computer-Aided Design of Integrated Circuits and Systems (2020), doi:10.1109/TCAD.2020.3003234.
* A. B. Kahng, L. Wang and B. Xu, "The Tao of PAO: Anatomy of a Pin Access Oracle for Detailed Routing", Proc. ACM/IEEE Design Automation Conf., 2020, pp. 1-6.

## License ##
* [BSD 3-clause License](LICENSE)
