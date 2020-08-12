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


#include "abkMD5.h"
#include "abkCRC32.h"

using std::cout;
using std::endl;

static void md5test(const char*str)
{
    cout << "MD5 (\"" <<str<<"\") = "<<MD5(str) << endl;
}

static void crc32test(const char*str)
{
    CRC32 crc(str);
    cout << "CRC32 (\"" <<str<<"\") = "<<crc        << endl;
}

int main(void)
    {
    cout << "MD5 test suite:" << endl;
    md5test("");
    md5test("a");
    md5test("abc");
    md5test("message digest"); 
    md5test("abcdefghijklmnopqrstuvwxyz"); 
    md5test ("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789");
    md5test("1234567890123456789012345678901234567890"
        "1234567890123456789012345678901234567890");

    cout << "CRC32 test suite:" << endl;
    crc32test("");
    crc32test("a");
    crc32test("abc");
    crc32test("message digest"); 
    crc32test("abcdefghijklmnopqrstuvwxyz"); 
    crc32test("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789");
    crc32test("1234567890123456789012345678901234567890"
        "1234567890123456789012345678901234567890");
    
    return 0;
    }
