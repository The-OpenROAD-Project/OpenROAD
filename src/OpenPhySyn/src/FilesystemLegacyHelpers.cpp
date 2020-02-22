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

#include <FilesystemLegacyHelpers.hpp>
#include <cstring>
#include <sys/stat.h>
#include <sys/types.h>

#ifdef _WIN32
#include <direct.h>
#else
#include <dirent.h>
#endif

#include "PsnException.hpp"

namespace psn
{
namespace filesystem
{
bool
create_directory(const std::string& raw_path)
{
    int rc;
#ifdef _WIN32
    rc = ::_mkdir(raw_path.c_str());
#else
    rc        = ::mkdir(raw_path.c_str(), 0755);
#endif
    if (rc == 0)
    {
        return true;
    }
    return false;
}

std::vector<directory_entry>
directory_iterator(const std::string& target_path)
{
    return directory_iterator(path(target_path));
}
std::vector<directory_entry>
directory_iterator(const path target_path)
{
    std::vector<directory_entry> entries;
#ifdef _WIN32
    std::string     pattern(target_path.generic_string());
    WIN32_FIND_DATA data;
    HANDLE          hFind;
    if ((hFind = FindFirstFile(pattern.c_str(), &data)) != INVALID_HANDLE_VALUE)
    {
        do
        {
            entries.push_back(data.cFileName);
        } while (FindNextFile(hFind, &data) != 0);
        FindClose(hFind);
    }
#else
    DIR* dirp = opendir(target_path.generic_string().c_str());
    if (dirp)
    {
        struct dirent* dp;
        while ((dp = readdir(dirp)) != NULL)
        {
            if (strcmp(dp->d_name, ".") && strcmp(dp->d_name, ".."))
            {
                entries.push_back(target_path / path(dp->d_name));
            }
        }
    }
    closedir(dirp);
#endif
    return entries;
}

path::path(const std::string& raw_path) : path_str_(raw_path)
{
    if (path_str_.size())
    {
        int last_c = path_str_.size() - 1;
        while (path_str_.size() > 1 && path_str_[last_c] == separator())
        {
            path_str_ = path_str_.substr(0, path_str_.size() - 1);
            last_c    = path_str_.size() - 1;
        }
    }
}

path
path::operator/(path const& other_path) const
{
    std::string join_path_str = path_str_;
    int         last_c        = join_path_str.size() - 1;
    while (join_path_str.size() > 1 && join_path_str[last_c] == separator())
    {
        join_path_str = join_path_str.substr(0, join_path_str.size() - 1);
        last_c        = join_path_str[join_path_str.size() - 1];
    }
    join_path_str =
        join_path_str.size()
            ? (join_path_str + path::separator() + other_path.path_str_)
            : other_path.path_str_;
    return path(join_path_str);
}

std::string
path::generic_string() const
{
    return path_str_;
}

path::operator std::string() const
{
    return std::string(generic_string());
}

} // namespace filesystem
} // namespace psn