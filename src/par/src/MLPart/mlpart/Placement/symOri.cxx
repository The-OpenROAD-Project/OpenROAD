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

// 980217  dv added orientation-SET for cells.
#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif

#include <Placement/symOri.h>
#include <iostream>
#include <iomanip>

using std::ostream;
using std::cout;
using std::endl;

// definitions of static class members

char Symmetry::text[10] = "";
char Orient::text[5] = "";

Symmetry::Symmetry(const char* txt) {
        if (strstr(txt, "R90") || strstr(txt, "r90"))
                rot90 = true;
        else
                rot90 = false;
        if (strchr(txt, 'Y') || strchr(txt, 'y'))
                y = true;
        else
                y = false;
        if (strchr(txt, 'X') || strchr(txt, 'x'))
                x = true;
        else
                x = false;
}

Symmetry::operator const char*() const {
        strcpy(text, "");
        if (rot90) strcat(text, " R90");
        if (y) strcat(text, " Y");
        if (x) strcat(text, " X");
        strcat(text, " ");
        return text;
}

Orient::Orient(const char* txt) {
        _angle = 0;  // initialize to N orientation
        if (strchr(txt, 'F') || strchr(txt, 'f'))
                _faceUp = false;
        else
                _faceUp = true;
        if (strchr(txt, 'N') || strchr(txt, 'n')) {
                _angle = 0;
                return;
        }
        if (strchr(txt, 'E') || strchr(txt, 'e')) {
                _angle = 1;
                return;
        }
        if (strchr(txt, 'S') || strchr(txt, 's')) {
                _angle = 2;
                return;
        }
        if (strchr(txt, 'W') || strchr(txt, 'w')) {
                _angle = 3;
                return;
        }
}

Orient::operator const char*() const {
        strcpy(text, (_faceUp ? " " : "F"));
        switch (_angle) {
                case 0:
                        strcat(text, "N ");
                        break;
                case 1:
                        strcat(text, "E ");
                        break;
                case 2:
                        strcat(text, "S ");
                        break;
                case 3:
                        strcat(text, "W ");
                        break;
        }
        return text;
}

OrientationSet::OrientationSet(const Symmetry& sym, const Orient& orient)
    /*
       Convert the symmetry, orientation pair into an orientation set.

       It would be nice to do something slick here like:
           Start with the single orientation, then generate the
           whole set by applying symmetry generation operations...

       But I don't think there's an easy way to reverse the bits
       in an unsigned, and Symmetry already exposes its
       internal representation, so we'll just look at the bits
       and do a big switch.  Sigh.
    */
{
        unsigned r;
        unsigned ori = orient.getAngle() + (orient.isFaceUp() ? 1 : 0);

        //  cout << "sym: " << sym << " orient " << orient << endl;
        //  cout << "sym, or: " << unsigned(sym) << " " << or << endl;

        switch (unsigned(sym)) {
                case 0:  // No symmetry
                        switch (ori) {
                                case 1:
                                        r = 1;  // N
                                        break;
                                case 0:
                                        r = 2;  // FN
                                        break;
                                case 91:
                                        r = 4;  // E
                                        break;
                                case 90:
                                        r = 8;  // FE
                                        break;
                                case 181:
                                        r = 16;  // S
                                        break;
                                case 180:
                                        r = 32;  // FS
                                        break;
                                case 271:
                                        r = 64;  // W
                                        break;
                                case 270:
                                        r = 128;  // FW
                                        break;
                                default:
                                        cout << "bad value case 0: " << ori << endl;
                        }
                case 1:  // X only
                        switch (ori) {
                                case 1:
                                case 180:
                                        r = 33;  // N+FS
                                        break;
                                case 0:
                                case 181:
                                        r = 18;  // FN+S
                                        break;
                                case 90:
                                case 91:
                                        r = 12;  // E+FE
                                        break;
                                case 270:
                                case 271:
                                        r = 192;  // W+FW
                                        break;
                                default:
                                        cout << "bad value case 1: " << ori << endl;
                        }
                        break;
                case 2:  // Y only
                        switch (ori) {
                                case 0:
                                case 1:
                                        r = 3;  // N+FN
                                        break;
                                case 91:
                                case 270:
                                        r = 132;  // E+FW
                                        break;
                                case 90:
                                case 271:
                                        r = 72;  // FE+W
                                        break;
                                case 180:
                                case 181:
                                        r = 48;  // S+FS
                                        break;
                                default:
                                        cout << "bad value case 2: " << ori << endl;
                        }
                        break;
                case 3:  // X and Y
                        switch (ori) {
                                case 0:
                                case 1:
                                case 180:
                                case 181:
                                        r = 51;  // N+FN+S+FS
                                        break;
                                default:
                                        r = 214;  // E+FE+W+FW
                        }
                        break;
                case 4:  // Rot90 only
                        switch (ori) {
                                case 1:
                                case 91:
                                case 181:
                                case 271:
                                        r = 85;  // N+E+S+W
                                        break;
                                default:
                                        r = 170;  // FN+FE+FS+FW
                        }
                        break;

                default:  // all other cases are the same
                        r = 255;
        }

        // Now just set all the bits

        N = r % 2;
        r /= 2;
        FN = r % 2;
        r /= 2;
        E = r % 2;
        r /= 2;
        FE = r % 2;
        r /= 2;
        S = r % 2;
        r /= 2;
        FS = r % 2;
        r /= 2;
        W = r % 2;
        r /= 2;
        FW = r % 2;
}

OrientationSet::OrientationSet(unsigned x) {
        N = x % 2;
        x /= 2;
        FN = x % 2;
        x /= 2;
        E = x % 2;
        x /= 2;
        FE = x % 2;
        x /= 2;
        S = x % 2;
        x /= 2;
        FS = x % 2;
        x /= 2;
        W = x % 2;
        x /= 2;
        FW = x % 2;
}

Orient OrientationSet::getNth(unsigned n) {
        if (N) {
                if (n == 0)
                        return Orient("N");
                else
                        --n;
        }
        if (FN) {
                if (n == 0)
                        return Orient("FN");
                else
                        --n;
        }
        if (E) {
                if (n == 0)
                        return Orient("E");
                else
                        --n;
        }
        if (FE) {
                if (n == 0)
                        return Orient("FE");
                else
                        --n;
        }
        if (S) {
                if (n == 0)
                        return Orient("S");
                else
                        --n;
        }
        if (FS) {
                if (n == 0)
                        return Orient("FS");
                else
                        --n;
        }
        if (W) {
                if (n == 0)
                        return Orient("W");
                else
                        --n;
        }
        if (FW) {
                if (n == 0)
                        return Orient("FW");
                else
                        --n;
        }
        abkfatal(false, "Requested non-existent member of orientation set.");

        return Orient(NULL);  // satisfy compiler
}

Orient OrientationSet::getOrient() {
        abkassert(isNotEmpty(), "Tried to extract orientation from empty set");
        if (N)
                return (Orient("N"));
        else if (FN)
                return (Orient("FN"));
        else if (E)
                return (Orient("E"));
        else if (FE)
                return (Orient("FE"));
        else if (S)
                return (Orient("S"));
        else if (FS)
                return (Orient("FS"));
        else if (W)
                return (Orient("W"));
        else if (FW)
                return (Orient("FW"));
        else
                abkassert(false, "Logic failure in OrientationSet::getOrient");

        return Orient(NULL);  // satisfy compiler
}

ostream& operator<<(ostream& out, const OrientationSet& os) {
        out << "OrientationSet{";
        if (os.N) out << "N ";
        if (os.FN) out << "FN ";
        if (os.E) out << "E ";
        if (os.FE) out << "FE ";
        if (os.S) out << "S ";
        if (os.FS) out << "FS ";
        if (os.W) out << "W ";
        if (os.FW) out << "FW ";
        out << "}";
        return out;
}

ostream& operator<<(ostream& out, const Symmetry& sym) {
        out << (const char*)sym;
        return out;
}

ostream& operator<<(ostream& out, const Orient& ori) {
        out << (const char*)ori;
        return out;
}
