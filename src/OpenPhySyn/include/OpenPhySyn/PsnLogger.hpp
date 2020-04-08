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

#pragma once

#include <memory>

namespace psn
{
enum class LogLevel
{
    trace    = 0,
    debug    = 1,
    info     = 2,
    warn     = 3,
    error    = 4,
    critical = 5,
    off      = 6
};
class PsnLogger
{
public:
    template<typename... Args>
    void
    trace(const char* format, Args&&... args) const
    {
        if (level_ <= LogLevel::trace)
        {
            std::printf(format, args...);
        }
    }
    template<typename... Args>
    void
    debug(const char* format, Args&&... args) const
    {
        if (level_ <= LogLevel::debug)
        {
            std::printf(format, args...);
        }
    }
    template<typename... Args>
    void
    info(const char* format, Args&&... args) const
    {
        if (level_ <= LogLevel::info)
        {
            std::printf(format, args...);
        }
    }
    template<typename... Args>
    void
    raw(const char* format, Args&&... args)
    {
        std::string current_pattern = pattern_;
        setPattern("");
        if (level_ <= LogLevel::info)
        {
            std::printf(format, args...);
        }
        setPattern(current_pattern);
    }
    template<typename... Args>
    void
    warn(const char* format, Args&&... args) const
    {
        if (level_ <= LogLevel::warn)
        {
            std::printf(format, args...);
        }
    }

    template<typename... Args>
    void
    critical(const char* format, Args&&... args) const
    {
        if (level_ <= LogLevel::critical)
        {
            std::printf(format, args...);
        }
    }
    template<typename... Args>
    void
    error(const char* format, Args&&... args) const
    {
        if (level_ <= LogLevel::error)
        {
            std::printf(format, args...);
        }
    }
    static PsnLogger& instance();

    void resetDefaultPattern();
    void setPattern(std::string pattern);
    void setLevel(LogLevel level);

private:
    PsnLogger();
    ~PsnLogger();
    LogLevel    level_;
    std::string pattern_;
};
} // namespace psn
