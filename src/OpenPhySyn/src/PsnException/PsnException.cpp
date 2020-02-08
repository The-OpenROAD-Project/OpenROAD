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

#include "PsnException.hpp"

namespace psn
{

PsnException::PsnException(const char* message, const char* name,
                           ErrorCode code)
    : msg_(message), name_(name), code_(code)
{
    setMessage(msg_);
}

PsnException::PsnException(const std::string& message, const char* name,
                           ErrorCode code)
    : PsnException(message.c_str(), name, code)
{
    setMessage(msg_);
}

std::string
PsnException::name() const
{
    return name_;
}
ErrorCode
PsnException::code() const
{
    return code_;
}

PsnException::~PsnException()
{
}
void
PsnException::setMessage(const char* message)
{
    std::string converted = std::string(message);
    setMessage(converted);
}

void
PsnException::setMessage(std::string& message)
{
    msg_     = message;
    fmt_msg_ = name_ + ": " + msg_;
}

std::string
PsnException::message() const
{
    return msg_;
}

const char*
PsnException::what() const throw()
{
    return fmt_msg_.c_str();
}

} // namespace psn
