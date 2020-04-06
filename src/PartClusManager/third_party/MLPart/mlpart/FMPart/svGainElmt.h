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

// created by Andrew Caldwell and Igor Markov on 05/24/98

#ifndef __SV_GAIN_ELEMENT_H__
#define __SV_GAIN_ELEMENT_H__

class SVGainElement {
       public:
        SVGainElement* _next;
        SVGainElement* _prev;
        int _gain;
        int _gainOffset;  // used for clip

        SVGainElement() : _next(NULL), _prev(NULL), _gain(0), _gainOffset(0) {}

        ~SVGainElement() {}
        void disconnect();

        int getGain() const { return _gain; }
        int getRealGain() const { return _gain + _gainOffset; }

        friend std::ostream& operator<<(std::ostream&, const SVGainElement&);
};

inline std::ostream& operator<<(std::ostream& os, const SVGainElement& elmt) {
        os << "[" << elmt._gain << "+" << elmt._gainOffset << "]";
        return os;
}

inline void SVGainElement::disconnect()
    // removes itself form the list it may be in
{
        if (_next != NULL) _next->_prev = _prev;

        if (_prev != NULL) _prev->_next = _next;

        _next = NULL;
        _prev = NULL;
}

#endif
