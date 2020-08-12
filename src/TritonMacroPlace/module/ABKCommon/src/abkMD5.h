/**************************************************************************
***    
*** Copyright (c) 1995-2000 Regents of the University of California,
***               Andrew E. Caldwell, Andrew B. Kahng and Igor L. Markov
*** Copyright (c) 2000-2010 Regents of the University of Michigan,
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


/*Copyright notice:
 The following software is derived from the RSA Data
 Security, Inc. MD5 Message-Digest Algorithm.  Accordingly,
 the following copyright notice is reproduced, and applies to
 such parts of this file which are copied from RSA's source code.
 All other parts of the file are copyright 1998 ABKGroup and
 the Regents of the University of California.
*/

/* Copyright (C) 1991-2, RSA Data Security, Inc. Created 1991. All
rights reserved.

License to copy and use this software is granted provided that it
is identified as the "RSA Data Security, Inc. MD5 Message-Digest
Algorithm" in all material mentioning or referencing this software
or this function.

License is also granted to make and use derivative works provided
that such works are identified as "derived from the RSA Data
Security, Inc. MD5 Message-Digest Algorithm" in all material
mentioning or referencing the derived work.

RSA Data Security, Inc. makes no representations concerning either
the merchantability of this software or the suitability of this
software for any particular purpose. It is provided "as is"
without express or implied warranty of any kind.

These notices must be retained in any copies of any part of this
documentation and/or software.
*/


#ifndef _MD5_H
#define _MD5_H

#include "abkcommon.h"

//: Message-Digest Algorithm
class MD5
    {
    friend std::ostream& operator<<(std::ostream& os, const MD5& md5);
    public:
        class Wrap128
            {
            public:
                unsigned meat[4];
                unsigned getWord(unsigned idx) const {return meat[idx];}
                unsigned operator[](unsigned idx) const {return meat[idx];}
            };
    public:
        MD5();
        MD5(const char *input, unsigned length, bool lastUpdate=true);
        
        MD5(const char *input, bool lastUpdate=true); 
        //here input should be null-terminated
        MD5(const MD5&){abkfatal(0,"can't copy-ctruct MD5");}


        ~MD5();

        void update(const char *input,unsigned inputLen, bool lastUpdate);
        void update(const char *input, bool lastUpdate); 
        //here input should be null-terminated
        operator unsigned() const; 
        // returns word 0 of state
        /*operator DWORD();*/
        operator Wrap128() const;

        MD5 &operator=(const MD5&){abkfatal(0,"can't assign MD5");
                                       return *this;}
        unsigned getWord(unsigned idx) const; //idx goes from 0 to 3
        
        void finalize();

    public:
        static void encode (unsigned char *output,
                            const unsigned *input, unsigned len);
        // helper functions that there's no need to protect
        // (not that they're likely to be of much use)

        static void decode (unsigned *output,
                            const unsigned char *input, unsigned len);

    protected:
        void _update(const unsigned char *input,unsigned inputLen);
        //helper functions that change things
        void     _initState();
        void     _finalize();
        void     _transform(const unsigned char *block);
    protected:
        unsigned _state[4];
        //internal data
        unsigned _count[2];  
        // number of bits, modulo 2\^64 (lsb first)
        unsigned char _buffer[64];
        bool     _isFinalized;

    };

std::ostream& operator<<(std::ostream& os, const MD5& md5);

#endif
