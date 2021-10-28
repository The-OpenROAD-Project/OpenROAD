////////////////////////////////////////////////////////////////////////////////
// File: plotgnu.h
////////////////////////////////////////////////////////////////////////////////


#pragma once

////////////////////////////////////////////////////////////////////////////////
// Includes.
////////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <vector>
#include <map>
#include <algorithm>
#include <string.h>
#include "network.h"
#include "architecture.h"
#include "router.h"
#include "rectangle.h"
using namespace std;



namespace aak
{

////////////////////////////////////////////////////////////////////////////////
// Defines.
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Classes.
////////////////////////////////////////////////////////////////////////////////
class PlotGnu
{
public:
	PlotGnu();
    virtual ~PlotGnu() {}

    void Draw(Network* network, Architecture* arch, char* msg = 0);
    void DrawLargeCells(Network* network, Architecture* arch, char* msg = 0);

    void draw_placement_density(Network* nw, Architecture* arch, RoutingParams* rt, char* buf);

    void draw_rectangles( std::vector<Rectangle>& rects, double xmin,  double xmax, double ymin, double ymax, char* buf );


    void Draw( Network* network, Architecture* arch, std::vector<std::pair<double,double> >& otherPl, char* msg = 0 );

protected:
    void drawArchitecture(char* buf);
    void drawEdges(char* buf);
    void drawNodes(char* buf);


protected:
#define	BUFFER_SIZE	2047
    Network* m_network;
    Architecture* m_arch;
    RoutingParams* m_rt;
    unsigned m_counter;
    bool m_drawArchitecture;
    bool m_drawNodes;
    bool m_drawEdges;
    char m_filename[BUFFER_SIZE + 1];
    bool m_skipStandardCells;
    bool m_drawDisp;

    std::vector<std::pair<double,double> > m_otherPl;
};

} // namespace aak

