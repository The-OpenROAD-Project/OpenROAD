// BSD 3-Clause License

// Copyright (c) 2019, SCALE Lab, Brown University
// All rights reserved.

// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:

// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.

// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.

// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.

// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#ifndef __PSN_STRING_UTILS__
#define __PSN_STRING_UTILS__
#include <algorithm>
#include <fstream>
#include <libgen.h>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <vector>

namespace psn
{
class StringUtils
{
public:
    static std::vector<std::string>
    split(std::string str, const std::string& delimiter)
    {
        std::vector<std::string> tokens;
        size_t                   pos = 0;
        std::string              token;
        while ((pos = str.find(delimiter)) != std::string::npos)
        {
            token = str.substr(0, pos);
            tokens.push_back(token);
            str.erase(0, pos + delimiter.length());
        }
        tokens.push_back(str);
        return tokens;
    }
    static bool
    isNumber(const char* s)
    {
        std::string        str(s);
        std::istringstream iss(str);
        float              f;
        iss >> std::noskipws >> f;
        return iss.eof() && !iss.fail();
    }
    static bool
    isTruthy(const char* s)
    {
        std::string str(s);
        std::transform(str.begin(), str.end(), str.begin(), ::tolower);
        return (str == "true" || str == "1" || str == "yes" || str == "y");
    }
    static bool
    isFalsy(const char* s)
    {
        std::string str(s);
        std::transform(str.begin(), str.end(), str.begin(), ::tolower);
        return (str == "false" || str == "0" || str == "no" || str == "n");
    }
};
} // namespace psn
#endif