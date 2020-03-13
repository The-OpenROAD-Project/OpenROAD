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

#ifndef __PSN_PSN_LOGGER__
#define __PSN_PSN_LOGGER__

#include <OpenPhySyn/LogLevel.hpp>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
namespace psn
{
class PsnLogger
{
public:
    template<typename... Args>
    void
    trace(Args&&... args) const
    {
        logger_->trace(args...);
    }
    template<typename... Args>
    void
    debug(Args&&... args) const
    {
        logger_->debug(args...);
    }
    template<typename... Args>
    void
    info(Args&&... args) const
    {
        logger_->info(args...);
    }
    template<typename... Args>
    void
    raw(Args&&... args)
    {
        std::string current_pattern = pattern_;
        setPattern("%v");
        logger_->info(args...);
        setPattern(current_pattern);
    }
    template<typename... Args>
    void
    warn(Args&&... args) const
    {
        logger_->warn(args...);
    }

    template<typename... Args>
    void
    critical(Args&&... args) const
    {
        logger_->critical(args...);
    }
    template<typename... Args>
    void
    error(Args&&... args) const
    {
        logger_->error(args...);
    }
    static PsnLogger& instance();

    void resetDefaultPattern();
    void setPattern(std::string pattern);
    void setLevel(LogLevel level);
    void setLogFile(std::string log_file);

private:
    PsnLogger();
    ~PsnLogger();
    LogLevel                                             level_;
    std::string                                          pattern_;
    std::shared_ptr<spdlog::sinks::stdout_color_sink_mt> console_sink_;
    std::shared_ptr<spdlog::sinks::basic_file_sink_mt>   file_sink_;
    std::shared_ptr<spdlog::logger>                      logger_;
};
} // namespace psn
#endif //__PSN_DEMO__