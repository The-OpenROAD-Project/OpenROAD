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

#include "genHier.h"
#include "newcasecmp.h"

using std::ifstream;
using std::ofstream;
using std::endl;
using uofm::vector;

GenericHierarchy::GenericHierarchy(const char* hclFileName) : _root(UINT_MAX) {
        ifstream hclFile(hclFileName);
        abkfatal2(hclFile, "Could not open file ", hclFileName);
        char tmp[1024];

        hclFile >> needcaseword("<hcl>");
        hclFile >> needcaseword("<header>");

        hclFile >> needcaseword("<created>");

        while (hclFile) {
                hclFile >> tmp;
                if (!newstrcasecmp(tmp, "</created>")) break;
        }

        hclFile >> needcaseword("<author>");
        while (hclFile) {
                hclFile >> tmp;
                if (!newstrcasecmp(tmp, "</author>")) break;
        }

        hclFile >> needcaseword("</header>");
        hclFile >> needcaseword("<body>");

        unsigned numNodes = 0;
        vector<char*> parentNames;
        hclFile >> tmp;

        // collect all node names, setup mapping from name->id;
        while (newstrcasecmp(tmp, "</body>")) {
                abkfatal2(!newstrcasecmp(tmp, "<cluster"), "expected '<cluster', got ", tmp);
                hclFile >> tmp;
                numNodes++;
                // this assumes that there will be no spaces.
                // i.e.  'name="cellName"' and 'parent="p1"'
                // levels are ignored
                while (newstrcasecmp(tmp, "/>")) {
                        if (strstr(tmp, "name") || strstr(tmp, "NAME")) {
                                char* namePtr = strstr(tmp, "\"") + 1;
                                char* endPtr = strstr(namePtr, "\"");
                                if (endPtr) *endPtr = 0;  // the trailing \" doesn't
                                                          // always
                                // come with the string

                                _names.push_back(new char[strlen(namePtr) + 2]);
                                strcpy(_names.back(), namePtr);
                                _namesToId[_names.back()] = _names.size() - 1;
                        } else if (strstr(tmp, "parent") || strstr(tmp, "PARENT")) {
                                char* namePtr = strstr(tmp, "\"") + 1;
                                char* endPtr = strstr(namePtr, "\"");
                                abkfatal2(endPtr, "endPtr was null.  tmp is: ", tmp);
                                if (endPtr) *endPtr = 0;  // the trailing \" doesn't
                                                          // always
                                // come with the string

                                parentNames.push_back(new char[strlen(namePtr) + 2]);
                                strcpy(parentNames.back(), namePtr);
                        }
                        hclFile >> tmp;
                }
                hclFile >> skiptoeol;
                abkfatal2(_names.size() == numNodes, "missing(or extra) name for node ", numNodes);
                abkfatal2(parentNames.size() == numNodes, "missing(or extra) parent for node ", numNodes);
                hclFile >> tmp;
        }
        hclFile.close();

        // from the parentNames, generate the parent and then children Id's
        _parents = vector<unsigned>(_names.size(), GENH_DELETED_NODE);
        _children = vector<vector<unsigned> >(_names.size());
        for (unsigned i = 0; i < parentNames.size(); i++) {
                abkfatal3(_namesToId.count(parentNames[i]) > 0, _names[i], "'s parent was not defined: ", parentNames[i]);

                _parents[i] = _namesToId[parentNames[i]];
                if (_parents[i] == i)
                        _root = i;
                else
                        _children[_parents[i]].push_back(i);

                if (parentNames[i]) delete[] parentNames[i];
                parentNames[i] = NULL;
        }
}

void GenericHierarchy::saveHCL(const char* hclFileName) {
        ofstream hclOut(hclFileName);

        hclOut << "<hcl>" << endl;
        hclOut << "<header>" << endl;

        hclOut << "<created> \n" << TimeStamp() << "</created>" << endl;
        hclOut << "<author> GenericHierarchy output, \n         " << User() << "</author>" << endl;
        hclOut << "</header>" << endl;

        hclOut << "<body>" << endl;

        for (unsigned i = 0; i < _names.size(); i++) {
                if (_parents[i] == GENH_DELETED_NODE) continue;

                hclOut << "<cluster name=\"" << _names[i] << "\"   "
                       << "parent=\"" << _names[_parents[i]] << "\"   />" << endl;
        }

        hclOut << "</body>" << endl;
        hclOut << "</hcl>" << endl;

        hclOut.close();
}

GenericHierarchy::~GenericHierarchy() {
        for (unsigned i = 0; i < _names.size(); i++) {
                if (_names[i]) delete[] _names[i];
                _names[i] = NULL;
        }
}

void GenericHierarchy::deleteNode(unsigned nodeId) {
        abkfatal(_children[nodeId].size() == 0, "can't delete a non-leaf node");

        unsigned nodeToDelete = nodeId;
        while (_children[nodeToDelete].size() == 0) {
                abkfatal(_parents[nodeId] != nodeId, "error: deleteNode got to the root");

                unsigned nodesParent = _parents[nodeToDelete];
                _parents[nodeToDelete] = GENH_DELETED_NODE;
                vector<unsigned>& childRef = _children[nodesParent];

                for (unsigned i = 0; i < childRef.size(); ++i) {
                        if (childRef[i] == nodeToDelete) {
                                // porting q: not sure if ordedr needs to be
                                // preserved here or not.  If not, a swap and
                                // pop_back
                                // would be faster.
                                childRef.erase(childRef.begin() + i);
                                return;
                        }
                }
                nodeToDelete = nodesParent;
        }
}
