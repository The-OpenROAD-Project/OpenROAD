/**************************************************************************
***
*** Copyright (c) 1995-2000 Regents of the University of California,
***               Andrew E. Caldwell, Andrew B. Kahng and Igor L. Markov
*** Copyright (c) 2000-2007 Regents of the University of Michigan,
***               Saurabh N. Adya, Jarrod A. Roy, David A. Papa and
***               Igor L. Markov
***
***  Contact author(s): abk@cs.ucsd.edu, imarkov@umich.edu
***  Original Affiliation:   UCLA, Computer Science Department,
***                          Los Angeles, CA 90095-1596 USA
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

#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif

#include <ClusteredHGraph/clustHGNet.h>
#include <ClusteredHGraph/clustHGCluster.h>
#include <HGraph/hgFixed.h>

using std::ostream;
using std::cout;
using std::endl;
using std::vector;

ClHG_ClNet::ClHG_ClNet(const HGFEdge& netSrc) : _index(netSrc.getIndex()), _maxDegree(netSrc.getDegree()), _netWt(netSrc.getWeight()) {
        abkfatal(netSrc.getWeight() > 0, "edge with 0 weight");

        _clusters.reserve(_maxDegree);
}

void ClHG_ClNet::removeCluster(ClHG_Cluster& oldCl) {
        for (unsigned c = 0; c < _clusters.size(); c++) {
                if (_clusters[c] == &oldCl) {
                        _clusters[c] = _clusters.back();
                        _clusters.pop_back();
                        return;
                }
        }
        cout << "NetId " << _index << endl;
        cout << " MaxDegree: " << _maxDegree << endl;
        cout << " Degree:    " << _clusters.size() << endl;
        cout << " Failed trying to remove Cluster " << oldCl.getIndex() << " which has degree " << oldCl.getDegree() << endl;

        abkfatal(0, "removing a cluster not on the net");
}

ostream& operator<<(ostream& os, const ClHG_ClNet& net) {
        os << "Net :" << net.getIndex() << " - " << net.getWeight() << endl;
        os << "{";
        vector<ClHG_Cluster*>::const_iterator c;
        for (c = net._clusters.begin(); c != net._clusters.end(); c++) os << (*c)->getIndex() << " ";
        os << "}" << endl;

        return os;
}
