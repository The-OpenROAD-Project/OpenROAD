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

#ifndef __PSN_TRANSFORM__
#define __PSN_TRANSFORM__
#include <OpenPhySyn/Types.hpp>
#include <memory>
#include <string>

namespace psn
{
class Psn;
class PsnTransform
{
public:
#ifdef OPENPHYSYN_AUTO_LINK
    virtual std::shared_ptr<psn::PsnTransform> load()        = 0;
    virtual const char*                        name()        = 0;
    virtual const char*                        version()     = 0;
    virtual const char*                        help()        = 0;
    virtual const char*                        description() = 0;
#endif
    PsnTransform();
    virtual ~PsnTransform();
    virtual int run(Psn* psn_, std::vector<std::string> args) = 0;
};

#ifdef OPENPHYSYN_AUTO_LINK
#define DEFINE_TRANSFORM_VIRTUALS(classType, transformName, transformVersion,  \
                                  transformDescription, transformHelp)         \
    std::shared_ptr<psn::PsnTransform> classType::load()                       \
    {                                                                          \
        return std::shared_ptr<classType>(this);                               \
    };                                                                         \
                                                                               \
    const char* classType::name()                                              \
    {                                                                          \
        return transformName;                                                  \
    };                                                                         \
                                                                               \
    const char* classType::version()                                           \
    {                                                                          \
        return transformVersion;                                               \
    };                                                                         \
    const char* classType::help()                                              \
    {                                                                          \
        return transformHelp;                                                  \
    };                                                                         \
    const char* classType::description()                                       \
    {                                                                          \
        return transformDescription;                                           \
    };
#define DEFINE_TRANSFORM(classType, transformName, transformVersion,           \
                         transformDescription, transformHelp)
#else
#define DEFINE_TRANSFORM_VIRTUALS(classType, transformName, transformVersion,  \
                                  transformDescription, transformHelp)
#define DEFINE_TRANSFORM(classType, transformName, transformVersion,           \
                         transformDescription, transformHelp)                  \
    extern "C"                                                                 \
    {                                                                          \
        std::shared_ptr<psn::PsnTransform>                                     \
        load()                                                                 \
        {                                                                      \
            return std::make_shared<classType>();                              \
        }                                                                      \
                                                                               \
        const char*                                                            \
        name()                                                                 \
        {                                                                      \
            return transformName;                                              \
        }                                                                      \
                                                                               \
        const char*                                                            \
        version()                                                              \
        {                                                                      \
            return transformVersion;                                           \
        }                                                                      \
        const char*                                                            \
        help()                                                                 \
        {                                                                      \
            return transformHelp;                                              \
        }                                                                      \
        const char*                                                            \
        description()                                                          \
        {                                                                      \
            return transformDescription;                                       \
        }                                                                      \
    }
#endif
} // namespace psn
#endif /* ifndef __TRANSFORM__ */
