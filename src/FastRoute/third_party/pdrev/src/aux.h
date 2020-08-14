#include "graph.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include <vector>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <string>

unsigned num_nets = 1000;
unsigned num_terminals = 64;
unsigned verbose = 0;
float alpha1 = 1;
float alpha2 = 0.45;
float alpha3 = 0;
float alpha4 = 0;
float margin = 1.1;
unsigned seed = 0;
unsigned root_idx = 0;
unsigned dist = 2;
float beta = 1.4;
bool runOneNet = false;
unsigned net_num = 0;
vector<Graph*> my_graphs;
