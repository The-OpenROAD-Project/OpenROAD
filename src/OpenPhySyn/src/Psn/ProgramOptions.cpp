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

#include <OpenPhySyn/Psn/ProgramOptions.hpp>

#ifndef OPENPHYSYN_CXX_OPTS_DISABLED
#include "cxxopts.hpp"
#endif

#include <sstream>
#include "PsnException/ProgramOptionsException.hpp"

namespace psn
{
ProgramOptions::ProgramOptions(int argc, char** argv)
    : argc_(argc),
      argv_(argv),
      file_(),
      has_file_(false),
      log_file_(),
      has_log_file_(false),
      log_level_(),
      has_log_level_(false),
      help_(false),
      version_(false),
      verbose_(false),
      quiet_(false)
{
#ifndef OPENPHYSYN_CXX_OPTS_DISABLED
    try
    {
        std::vector<std::string> file_positional;
        cxxopts::Options         options(
            "OpenPhySyn", "OpenPhySyn - OpenROAD physical synthesis toolkit.");
        options.add_options()("h,help", "Display this help message and exit")(
            "file", "Load TCL script",
            cxxopts::value<std::vector<std::string>>(file_positional))(
            "log-file", "Write output to log file",
            cxxopts::value<std::string>())(
            "log-level", "Default log level [info, warn, error, critical]",
            cxxopts::value<std::string>())("v,version",
                                           "Display the version number")(
            "verbose", "Verbose output")("quiet", "Disable output");
        usage_ = options.help();
        if (argc_)
        {

            options.parse_positional({"file"});
            auto result = options.parse(argc_, argv_);

            if (file_positional.size())
            {
                if (file_positional.size() > 1)
                {
                    throw ProgramOptionsException(
                        "Only one script file read is supported per "
                        "exectution.");
                }
                has_file_ = true;
                file_     = file_positional[0];
            }

            if (result.count("help"))
            {
                help_ = true;
            }
            if (result.count("version"))
            {
                version_ = true;
            }
            if (result.count("verbose"))
            {
                verbose_ = true;
            }
            if (result.count("quiet"))
            {
                quiet_ = true;
            }

            if (result.count("log-file"))
            {
                has_log_file_ = true;
                log_file_     = result["log-file"].as<std::string>();
            }
            if (result.count("log-level"))
            {
                has_log_level_ = true;
                log_level_     = result["log-level"].as<std::string>();
            }
        }
    }
    catch (cxxopts::OptionSpecException& e)
    {
        throw ProgramOptionsException(e.what());
    }
    catch (cxxopts::OptionParseException& e)
    {
        throw ProgramOptionsException(e.what());
    }
#endif
}
std::string
ProgramOptions::usage() const
{
    return usage_;
}

bool
ProgramOptions::help() const
{
    return help_;
}
bool
ProgramOptions::version() const
{
    return version_;
}
bool
ProgramOptions::verbose() const
{
    return version_;
}
bool
ProgramOptions::quiet() const
{
    return quiet_;
}
bool
ProgramOptions::hasFile() const
{
    return has_file_;
}
std::string
ProgramOptions::file() const
{
    return file_;
}
bool
ProgramOptions::hasLogFile() const
{
    return has_log_file_;
}
std::string
ProgramOptions::logFile() const
{
    return log_file_;
}
bool
ProgramOptions::hasLogLevel() const
{
    return has_log_level_;
}
std::string
ProgramOptions::logLevel() const
{
    return log_level_;
}

} // namespace psn
