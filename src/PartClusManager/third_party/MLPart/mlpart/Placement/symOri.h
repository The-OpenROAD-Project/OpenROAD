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

// Symmetry and Orientation classes.
// Igor Markov, last rev: July 16, 1997  VLSI CAD ABKGROUP, UCLA/CS
// 980217  dv added orientation-SET for cells.
// 990425 moved to Placement from DB by AEC
// 000502 ilm workarounds for CC5.0 bugs (in handling operators)

#ifndef _PLACEMENT_SYMORI_H_
#define _PLACEMENT_SYMORI_H_

#ifdef _MSC_VER
#pragma warning(disable : 4800)
#endif

#include <ABKCommon/abkcommon.h>

class Orient;
class OrientationSet;
typedef Orient Orientation;

class Symmetry {
        static char text[10];

       public:
        unsigned char rot90 : 1;
        unsigned char y : 1;
        unsigned char x : 1;  // Note 1: none of int types can be used here
        // Note 2: char implicitely converts to int types,
        // but if you cout<< it, you need to put int() conversions
        Symmetry(const char*);      // Allows for, e.g., Symmetry("X R90");
        Symmetry(unsigned val = 0)  // integer conversions allow | and & on Symmetry
        {
                x = val % 2;
                val = val / 2;
                y = val % 2;
                val = val / 2;
                rot90 = val % 2;
        }
        Symmetry(bool rot, bool yy, bool xx) : rot90(rot), y(yy), x(xx) {}
        operator unsigned() const { return x + 2 * y + 4 * rot90; }
        operator const char*() const;
        // the whole string should be copied at once !!!
        friend std::ostream& operator<<(std::ostream&, const Symmetry&);
};

class Orient {
        static char text[5];

       public:
        unsigned char _angle : 2;
        unsigned char _faceUp : 1;

        // Model : the default orientation is "vertically and face up" (think of
        // a dial)
        // First rotate by _angle*90 (clockwise),
        // then flip >>about the _angle*90 deg axis<< if ! _faceUp
        // Alternatively: flip horizontally before rotation

        Orient(const char*);  // allows, e.g., Orient("FS");
        Orient(unsigned angle, bool faceUp)
            :  // angle 0,90,180,270;
              _angle(angle / 90),
              _faceUp(faceUp) {
                abkassert(angle == 0 || angle == 90 || angle == 180 || angle == 270, "Can only handle angles 0,90,180 and 270 for orientation");
        }
        Orient() : _angle(0), _faceUp(1) {};

        bool isFaceUp() const { return _faceUp; }
        unsigned getAngle() const { return _angle * 90; }

        bool flipHoriz() {
                _angle += 2 * (_angle % 2);
                return (_faceUp = !_faceUp);
        }
        bool flipVert() {
                _angle += 2 * (!(_angle % 2));
                return (_faceUp = !_faceUp);
        }
        unsigned rotateCwise() {
                return (++_angle) * 90;
        };
        unsigned rotateCCwise() {
                return (--_angle) * 90;
        };

        operator unsigned() const { return _angle * 8 + _faceUp; }
        bool operator==(Orient another) const { return _angle == another._angle && _faceUp == another._faceUp; }
        bool operator!=(Orient another) const { return !(*this == another); }
        operator const char*() const;
        // the whole string should be copied at once !!!
        friend std::ostream& operator<<(std::ostream&, const Orient&);
};

/*
   For now, the orientation set stuff is thrown in here as well.
   Maybe it should get moved somewhere else.
*/

class OrientationSet {
       protected:
        unsigned char N : 1;
        unsigned char FN : 1;
        unsigned char E : 1;
        unsigned char FE : 1;
        unsigned char S : 1;
        unsigned char FS : 1;
        unsigned char W : 1;
        unsigned char FW : 1;

       public:
        OrientationSet(const Symmetry& sym, const Orient& orient);
        OrientationSet(const Symmetry& sym) { OrientationSet(sym, Orient("N")); }
        OrientationSet(unsigned x);
        Orient getNth(unsigned n);
        Orient getOrient();

        operator unsigned() const { return N + 2 * FN + 4 * E + 8 * FE + 16 * S + 32 * FS + 64 * W + 128 * FW; }

        OrientationSet operator&(const OrientationSet& os) { return OrientationSet(unsigned(*this) & unsigned(os)); }

        bool isNotEmpty() { return unsigned(*this); }

        unsigned size() { return N + FN + E + FE + S + FS + W + FW; }

        friend std::ostream& operator<<(std::ostream& out, const OrientationSet& os);
};

#ifdef _MSC_VER
#pragma warning(default : 4800)
#endif
#endif
