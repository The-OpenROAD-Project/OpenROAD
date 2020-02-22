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

#ifndef __PSN_FILESYSTEM_LEGACY_HELPERS__
#define __PSN_FILESYSTEM_LEGACY_HELPERS__

#include <string>
#include <vector>

namespace psn
{
namespace filesystem
{

bool create_directory(const std::string& raw_path);

class path
{
    std::string path_str_;

public:
    path(const std::string& raw_path);
    path        operator/(path const& other_path) const;
    std::string generic_string() const;

    operator std::string() const;

    static inline char
    separator()
    {
#ifdef _WIN32
        return '\\';
#else
        return '/';
#endif
    }
};

class directory_entry
{
    typedef class path path_type;
    path_type          path_;

public:
    directory_entry(const std::string& p) : path_(p)
    {
    }
    directory_entry(path_type p) : path_(p)
    {
    }
    path_type
    path() const
    {
        return path_type(path_);
    }
};

std::vector<directory_entry> directory_iterator(const std::string& target_path);
std::vector<directory_entry> directory_iterator(const path target_path);

} // namespace filesystem

} // namespace psn
#endif
