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

// Created by Stefanus Mantik , September 1998, VLSICAD ABKGroup UCLA/CS
#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif

#include <Placement/layoutBBoxes.h>
#include <Placement/xyDraw.h>
#include "newcasecmp.h"

using std::ifstream;
using std::ofstream;
using std::istream;
using std::ostream;
using std::endl;

void LayoutBBoxes::readBlocks(istream& in) {
        int lineNo = 1;
        char word1[100], word2[100], word3[100];
        unsigned nBoxes;
        in >> eathash(lineNo) >> word1 >> word2 >> word3;
        abkfatal(newstrcasecmp(word1, "Total") == 0, "Blocks files should start with \'Total blocks :\'\n");
        abkfatal(newstrcasecmp(word2, "blocks") == 0, "Blocks files should start with \'Total blocks :\'\n");
        abkfatal(newstrcasecmp(word3, ":") == 0, "Blocks files should start with \'Total blocks :\'\n");
        in >> my_isnumber(lineNo) >> nBoxes >> skiptoeol;

        abkfatal(nBoxes > 0, "Empty box list");
        for (unsigned i = 0; i < nBoxes; i++) {
                BBox box;
                in >> eathash(lineNo) >> box >> skiptoeol;
                push_back(box);
        }
}

void LayoutBBoxes::saveBlocks(ostream& out) const {
        out << "Total blocks : " << size() << endl;
        for (const_iterator b = begin(); b != end(); b++) out << (*b);
}

LayoutBBoxes::LayoutBBoxes(istream& in) { readBlocks(in); }

LayoutBBoxes::LayoutBBoxes(const char* blockFileName) {
        abkfatal(blockFileName != NULL, "file name is empty");
        ifstream blockFile(blockFileName);
        abkfatal(blockFile, "error opening block file");
        readBlocks(blockFile);
        blockFile.close();
}

void LayoutBBoxes::save(ostream& out) { saveBlocks(out); }

void LayoutBBoxes::save(const char* blockFileName) {
        abkfatal(blockFileName != NULL, "file name is empty");
        ofstream blockFile(blockFileName);
        abkfatal(blockFile, "error opening block file");
        saveBlocks(blockFile);
        blockFile.close();
}

void LayoutBBoxes::xyDraw(ostream& xyStream) {
        for (const_iterator b = begin(); b != end(); b++) xyDrawRectangle(xyStream, *b);
}

bool LayoutBBoxes::contains(const Point& x) const {
        bool found = false;
        for (const_iterator b = begin(); b != end() && !found; b++)
                if ((*b).contains(x)) found = true;
        return found;
}

unsigned LayoutBBoxes::locate(const Point& x) const {
        unsigned index = 0;
        bool found = false;
        for (const_iterator b = begin(); b != end() && !found; b++)
                if ((*b).contains(x))
                        found = true;
                else
                        index++;
        return index;
}
