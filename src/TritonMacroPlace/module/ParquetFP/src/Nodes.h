/**************************************************************************
***    
*** Copyright (c) 2000-2006 Regents of the University of Michigan,
***               Saurabh N. Adya, Hayward Chan, Jarrod A. Roy
***               and Igor L. Markov
***
***  Contact author(s): sadya@umich.edu, imarkov@umich.edu
***  Original Affiliation:   University of Michigan, EECS Dept.
***                          Ann Arbor, MI 48109-2122 USA
***
***  Permission is hereby granted, free of charge, to any person obtaining 
***  a copy of this software and associated documentation files (the
***  "Software"), to deal in the Software without restriction, including
***  without limitation 
***  the rights to use, copy, modify, merge, publish, distribute, sublicense, 
***  and/or sell copies of the Software, and to permit persons to whom the 
***  Software is furnished to do so, subject to the following conditions:
***
***  The above copyright notice and this permission notice shall be included
***  in all copies or substantial portions of the Software.
***
*** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
*** EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
*** OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
*** IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
*** CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT
*** OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
*** THE USE OR OTHER DEALINGS IN THE SOFTWARE.
***
***
***************************************************************************/


#ifndef NODES_H
#define NODES_H

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include "Node.h"

namespace parquetfp
{
    class Nets;
    typedef std::vector<Node>::iterator itNode;

    class Nodes
    {
        private:
            std::vector<Node> _nodes;
            std::vector<Node> _terminals;

        public:
            Nodes(const std::string &baseName);
            Nodes(void){}
            Nodes(const Nodes& orig);
            Nodes& operator=(const Nodes& rhs);

            void parseNodes(const std::string & fnameBl);
            void parseTxt(const std::string & fnameTxt);
            void parsePl(const std::string &fnamePl);

            unsigned getNumNodes(void) const          { return _nodes.size(); }
            itNode nodesBegin(void)                   { return _nodes.begin(); }
            itNode nodesEnd(void)                     { return _nodes.end(); }
            void putNewNode(Node& node)               { _nodes.push_back(node); }
            Node& getNode(unsigned index)             { return _nodes[index]; }
            const Node& getNode(unsigned index) const { return _nodes[index]; }
            void clearNodes(void)                     { _nodes.clear(); }

            unsigned getNumTerminals(void) const      { return _terminals.size(); }
            itNode terminalsBegin(void)               { return _terminals.begin(); }
            itNode terminalsEnd(void)                 { return _terminals.end(); }
            void putNewTerm(Node& term)               { _terminals.push_back(term); }
            Node& getTerminal(unsigned index)         { return _terminals[index]; }
            const Node& getTerminal(unsigned index) const { return _terminals[index]; }
            void clearTerm(void)                      { _terminals.clear(); }

            void clean(void)
            {
                _nodes.clear();
                _terminals.clear();
            }

            std::vector<float> getNodeWidths();
            std::vector<float> getNodeHeights();
            std::vector<float> getXLocs();
            std::vector<float> getYLocs();
            float getNodeWidth(unsigned index);
            float getNodeHeight(unsigned index);
            float getNodesArea();
            float getMinHeight();
            float getMinWidth();

            void putNodeWidth(unsigned index, float width);
            void putNodeHeight(unsigned index, float height);

            void changeOrient(unsigned index, ORIENT newOrient, Nets& nets);
            void updatePinsInfo(Nets& nets);

            void updatePlacement(int index, bool type, float xloc, float yloc);
            void updateOrient(int index, bool type, ORIENT newOrient);
            void updateHW(int index, bool type, float width, float height);

            //initialize info for fast pin offset access.
            //if reset is true then all pin-offsets are assumed to be non-trivial
            //useful for fast HPWL calc.
            void initNodesFastPOAccess(Nets& nets, bool reset=false);

            void savePl(const char* baseFileName);
            void saveNodes(const char* baseFileName);

            //following functions save in Capo format
            void saveCapoNodes(const char* baseFileName);
            void saveCapoPl(const char* baseFileName);
            void saveCapoScl(const char* baseFileName, float reqdAR, float reqdWS, const BBox &);
    };
}
//using namespace parquetfp;

#endif
