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

#ifndef _UMATRIX_H_
#define _UMATRIX_H_

#include "ABKCommon/abkcommon.h"
#include <iostream>

// Created by Igor Markov, Feb 26, 1998

class UDenseMatrix;

// ============================== Interfaces ==============================

class UDenseMatrix {
        unsigned rows, cols;
        unsigned* meat;

        void operator=(const UDenseMatrix&) { abkfatal(0, "Can't assign dense matrices"); }

       public:
        UDenseMatrix(int r, int c) : rows(r), cols(c) {
                abkassert(r > 0 && c > 0, "Invalid matrix dimensions");
                meat = new unsigned[r * c];
        }

        UDenseMatrix(int r, int c, unsigned val) : rows(r), cols(c) {
                abkassert(r > 0 && c > 0, "Invalid matrix dimensions");
                meat = new unsigned[r * c];
                for (int k = 0; k != r * c; k++) meat[k] = val;
        }

        UDenseMatrix(int r, int c, const unsigned* from, int) : rows(r), cols(c) {
                abkassert(r > 0 && c > 0, "Invalid matrix dimensions");
                meat = new unsigned[r * c];
                for (int k = 0; k != r * c; k++) meat[k] = from[k];
        }

        UDenseMatrix(const UDenseMatrix& from) : rows(from.rows), cols(from.cols) {
                meat = new unsigned[rows * cols];
                for (unsigned k = 0; k != rows * cols; k++) meat[k] = from.meat[k];
        }

        ~UDenseMatrix() { delete[] meat; }

        unsigned operator()(unsigned row, unsigned col) const {
                abkassert(row < rows && col < cols, " Out of bound indeci for dense matrix");
                return meat[row * cols + col];
        }

        unsigned& operator()(unsigned row, unsigned col) {
                abkassert(row < rows && col < cols, " Out of bound indeci for dense matrix");
                return meat[row * cols + col];
        }

        void setDiag(unsigned val) {
                unsigned min = (cols < rows ? cols : rows);
                for (unsigned i = 0; i < min; ++i) meat[i * cols + i] = val;
        }

        void setToZero() {
                unsigned* ptr = meat + rows * cols - 1;
                for (int c = rows * cols; c != 0; *(ptr--) = 0, c--)
                        ;
        }

        unsigned getNumRows() const { return rows; }
        unsigned getNumCols() const { return cols; }

        const unsigned* getMeat() const { return meat; }  // DANGEROUS:
                                                          // for C interfaces ONLY
};

std::ostream& operator<<(std::ostream& out, const UDenseMatrix& mat);

#ifndef AUTHMAR
#define AUTHMAR
static const char AuthorMarkov[50] = "022698, Igor Markov, VLSI CAD ABKGROUP UCLA";
#endif

#endif
