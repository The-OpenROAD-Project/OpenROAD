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

#ifndef __GENERIC_HIERARCHY_H__
#define __GENERIC_HIERARCHY_H__

#include <ABKCommon/abkcommon.h>
#include <ABKCommon/uofm_alloc.h>
#include <ABKCommon/sgi_hash_map.h>
#include <map>

struct GenHier_Eqstr {
        bool operator()(const char* s1, const char* s2) const { return strcmp(s1, s2) == 0; }
};

struct GenHier_Ltstr {
        bool operator()(const char* s1, const char* s2) const { return strcmp(s1, s2) < 0; }
};

// nodes which have been removed with have DELETED_NODE
// as their parent

#define GENH_DELETED_NODE UINT_MAX

class GenericHierarchy {
       protected:
        uofm::vector<char*> _names;
        typedef hash_map<const char*, unsigned, hash<const char*>, GenHier_Eqstr> CharIntHashMap;

        // typedef map<const char*, unsigned, GenHier_Ltstr> CharIntHashMap;

        CharIntHashMap _namesToId;

        uofm::vector<unsigned> _parents;
        uofm::vector<uofm::vector<unsigned> > _children;

        unsigned _root;

        // does not _really_ delete the node..just removes it from
        // the parent-child relationships.  Will remove any ancestors
        // that become leaves as a result of this deletion.  Can
        // only be performed on a leaf
        void deleteNode(unsigned nodeId);

       public:
        // the hierarchy is extracted from the nodeNames.
        // the delimiter set is assumed to be '\/|_-:;^#$'
        GenericHierarchy(const uofm::vector<const char*>& nodeNames, const char* delimit_set = "\\/|!_-:;^#$");

        GenericHierarchy(const char* hclFileName);

        virtual ~GenericHierarchy();

        unsigned getNodeId(const char* nodeName) const { return (*_namesToId.find(nodeName)).second; }
        const char* getNodeName(unsigned nodeId) const {
                abkassert(nodeId < _names.size(), "requrested name for nodeId that is too large");
                return _names[nodeId];
        }

        unsigned getParent(unsigned nodeId) const { return _parents[nodeId]; }
        const uofm::vector<unsigned>& getChildren(unsigned nodeId) const { return _children[nodeId]; }

        void saveHCL(const char* hclFileName);

        unsigned getRootId() const { return _root; }
        unsigned getNumNodes() const { return _names.size(); }
};

void gen_hier_common_prefix(const char* a, const char* b, const char* delim_set, char* commonPrefix);

#endif
