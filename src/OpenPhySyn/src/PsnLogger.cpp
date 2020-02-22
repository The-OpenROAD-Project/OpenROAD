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

#include <OpenPhySyn/PsnLogger.hpp>

namespace psn
{

PsnLogger::PsnLogger() : pattern_("%+")
{
    console_sink_ = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    file_sink_    = nullptr;
    logger_       = std::make_shared<spdlog::logger>(
        "", spdlog::sinks_init_list({console_sink_}));
    spdlog::set_default_logger(logger_);
    resetDefaultPattern();
    setLevel(LogLevel::info);
}
PsnLogger::~PsnLogger()
{
    logger_->flush();
}
void
PsnLogger::setPattern(std::string pattern)
{
    pattern_ = pattern;
    console_sink_->set_pattern(pattern);
    logger_->set_pattern(pattern);
    if (file_sink_ != nullptr)
    {
        file_sink_->set_pattern(pattern);
    }
}
void
PsnLogger::resetDefaultPattern()
{
    setPattern("%+");
}

PsnLogger&
PsnLogger::instance()
{
    static PsnLogger loggerSingelton;
    return loggerSingelton;
}

void
PsnLogger::setLogFile(std::string log_file)
{
    file_sink_ =
        std::make_shared<spdlog::sinks::basic_file_sink_mt>(log_file, true);
    logger_ = std::make_shared<spdlog::logger>(
        "", spdlog::sinks_init_list({file_sink_, console_sink_}));
    logger_->flush_on(spdlog::level::info);
    spdlog::set_default_logger(logger_);
    setPattern(pattern_);
    setLevel(level_);
}

void
PsnLogger::setLevel(LogLevel level)
{
    level_ = level;
    switch (level_)
    {
    case LogLevel::trace:
        logger_->set_level(spdlog::level::trace);
        console_sink_->set_level(spdlog::level::trace);
        if (file_sink_ != nullptr)
        {
            file_sink_->set_level(spdlog::level::trace);
        }
        break;
    case LogLevel::debug:
        logger_->set_level(spdlog::level::debug);
        console_sink_->set_level(spdlog::level::debug);
        if (file_sink_ != nullptr)
        {
            file_sink_->set_level(spdlog::level::debug);
        }
        break;
    case LogLevel::info:
        logger_->set_level(spdlog::level::info);
        console_sink_->set_level(spdlog::level::info);
        if (file_sink_ != nullptr)
        {
            file_sink_->set_level(spdlog::level::info);
        }
        break;
    case LogLevel::warn:
        logger_->set_level(spdlog::level::warn);
        console_sink_->set_level(spdlog::level::warn);
        if (file_sink_ != nullptr)
        {
            file_sink_->set_level(spdlog::level::warn);
        }
        break;
    case LogLevel::error:
        logger_->set_level(spdlog::level::err);
        console_sink_->set_level(spdlog::level::err);
        if (file_sink_ != nullptr)
        {
            file_sink_->set_level(spdlog::level::err);
        }
        break;
    case LogLevel::critical:
        logger_->set_level(spdlog::level::critical);
        console_sink_->set_level(spdlog::level::critical);
        if (file_sink_ != nullptr)
        {
            file_sink_->set_level(spdlog::level::critical);
        }
        break;
    case LogLevel::off:
        logger_->set_level(spdlog::level::off);
        console_sink_->set_level(spdlog::level::off);
        if (file_sink_ != nullptr)
        {
            file_sink_->set_level(spdlog::level::off);
        }
        break;
    }
}
} // namespace psn