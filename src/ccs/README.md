# OpenROAD's CCS Calculator

Currently serves as documentation for the 
[CCS Calculator](https://www.paripath.com/blog/characterization-blog/comparing-nldm-and-ccs-delay-models) development project

CCS is the thing that calculates delay for static timing analysis. 
Currently OpenROAD only supports NLDM which is innacurate below 130nm. This project
will implement CCS timing support in OpenROAD to enable better STA at those nodes.

We'll eventually implement this in C++, but currently have a python notebook that we're
using to sketch the process out which can be found in [ccs_calculator.ipynb](ccs_calculator.ipynb)