# OpenROAD's CCS Calculator

Currently serves as documentation for the 
[CCS Calculator](https://www.paripath.com/blog/characterization-blog/comparing-nldm-and-ccs-delay-models) development project

CCS is the thing that calculates delay for static timing analysis. 
Currently OpenROAD only supports NLDM which is innacurate below 130nm. This project
will implement CCS timing support in OpenROAD to enable better STA at those nodes.

We'll eventually implement this in C++, but currently have a python notebook that we're
using to sketch the process.

## Interacting With Notebook

Click the [Google CoLab](https://colab.sandbox.google.com/github/The-OpenROAD-Project/OpenROAD/blob/master/src/ccs/ccs_calculator.ipynb) link. Modify the doc
and upload changes as you see fit.

While it's possible to interact with the notebook on Github's interface the experience isn't always the best.