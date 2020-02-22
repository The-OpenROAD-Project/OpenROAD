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

#ifndef __PSN_GLOBAL__
#define __PSN_GLOBAL__

#define PSN_UNUSED(arg) (void)arg;
#define PSN_LOG_ERROR psn::PsnLogger::instance().error
#define PSN_LOG_INFO psn::PsnLogger::instance().info
#define PSN_LOG_CRITICAL psn::PsnLogger::instance().critical
#define PSN_LOG_DEBUG psn::PsnLogger::instance().debug
#define PSN_LOG_WARN psn::PsnLogger::instance().warn
#define PSN_LOG_TRACE psn::PsnLogger::instance().trace
#define PSN_LOG_RAW psn::PsnLogger::instance().raw

#define PSN_HANDLER_UNSUPPORTED_METHOD(HANDLER_NAME, METHOD_NAME)              \
    PSN_LOG_ERROR("The method " #METHOD_NAME                                   \
                  " is not supported by " #HANDLER_NAME);

#endif