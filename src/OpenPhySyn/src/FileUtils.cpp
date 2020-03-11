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

#ifdef __has_include
#if __cplusplus > 201402L
#include <filesystem>
namespace fs = std::filesystem;
#else
#include <FilesystemLegacyHelpers.hpp>
namespace fs = psn::filesystem;
#endif
#else
#include <FilesystemLegacyHelpers.hpp>
namespace fs = psn::filesystem;
#endif

#include <fstream>
#include <libgen.h>
#include <sys/stat.h>
#include "FileUtils.hpp"
#include "PsnException.hpp"

#ifdef _WIN32
#include <windows.h>
#else
#include <limits.h>
#include <unistd.h>
#endif

namespace psn
{
bool
FileUtils::pathExists(const std::string& path)
{
    struct stat buf;
    return (stat(path.c_str(), &buf) == 0);
}
bool
FileUtils::isDirectory(const std::string& path)
{
    struct stat buf;
    if (stat(path.c_str(), &buf) == 0)
    {
        if (buf.st_mode & S_IFDIR)
        {
            return true;
        }
    }
    return false;
}
bool
FileUtils::createDirectory(const std::string& path)
{
    return fs::create_directory(path);
}
bool
FileUtils::createDirectoryIfNotExists(const std::string& path)
{
    if (pathExists(path))
    {
        if (isDirectory(path))
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        return createDirectory(path);
    }
}
std::vector<std::string>
FileUtils::readDirectory(const std::string& path)
{
    std::vector<std::string> paths;
    for (const auto& entry : fs::directory_iterator(path))
    {
        paths.push_back(entry.path());
    }
    return paths;
}
std::string
FileUtils::readFile(const std::string& path)
{
    std::ifstream infile{path};
    if (!infile.is_open())
    {
        throw FileException();
    }
    std::string file_contents{std::istreambuf_iterator<char>(infile),
                              std::istreambuf_iterator<char>()};
    return file_contents;
}
std::string
FileUtils::homePath()
{
    std::string home_path = std::getenv("HOME");
    if (!home_path.length())
    {
        home_path = std::getenv("USERPROFILE");
    }
    if (!home_path.length())
    {
        home_path = std::getenv("HOMEDRIVE");
    }
    if (!home_path.length())
    {
        home_path = "/home/OpenPhySyn";
    }
    return home_path;
}
std::string
FileUtils::joinPath(const std::string& first_path,
                    const std::string& second_path)
{
    fs::path base(first_path);
    fs::path appendee(second_path);
    fs::path full_path = base / appendee;
    return full_path.generic_string();
}

std::string
FileUtils::baseName(const std::string& path)
{
    return std::string(basename(const_cast<char*>(path.c_str())));
}
std::string
FileUtils::executablePath()
{
#ifdef _WIN32
    const char *drive, exec_path, fname, ext;
    char        exec_file_path[MAX_PATH];

    HMODULE hModule = GetModuleHandle(NULL);
    if (hModule != NULL)
    {
        GetModuleFileName(hModule, exec_file_path, (sizeof(exec_file_path)));
        _splitpath(exec_file_path, drive, fname, ext);
        return exec_path;
    }
    else
    {
        return ".";
    }
#else
    const char* exec_path;
    char        exec_file_path[PATH_MAX] = {0};

    readlink("/proc/self/exe", exec_file_path, PATH_MAX);
    exec_path = dirname(exec_file_path);
    return exec_path;
#endif
}
} // namespace psn
