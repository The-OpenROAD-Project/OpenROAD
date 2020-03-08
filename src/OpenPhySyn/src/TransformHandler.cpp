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
#include "TransformHandler.hpp"
#include "PsnException.hpp"
#include <OpenPhySyn/PsnLogger.hpp>
#include <OpenPhySyn/PsnGlobal.hpp>
namespace psn
{
TransformHandler::TransformHandler(std::string name)
{
    handle_ = dlopen(name.c_str(), RTLD_LAZY);
    if (!handle_)
    {
        throw LoadTransformException();
    }
    load_        = (std::shared_ptr<PsnTransform>(*)())dlsym(handle_, "load");
    get_name_    = (char* (*)())dlsym(handle_, "name");
    get_version_ = (char* (*)())dlsym(handle_, "version");
    get_help_    = (char* (*)())dlsym(handle_, "help");
    get_description_ = (char* (*)())dlsym(handle_, "description");

    if (!load_ || !get_name_ || !get_version_ || !get_help_ ||
        !get_description_)
    {
        throw LoadTransformException();
    }
}
#ifdef OPENPHYSYN_AUTO_LINK
TransformHandler::TransformHandler(std::string trName, std::shared_ptr<PsnTransform> transform)
{
    instance = transform;
    get_name_        = [=]()->const char*{return transform.get()->name();};
    get_version_        = [=]()->const char*{return transform.get()->version();};
    get_help_        = [=]()->const char*{return transform.get()->help();};
    get_description_        = [=]()->const char*{return transform.get()->description();};
}
#endif

std::string
TransformHandler::name()
{
    return std::string(get_name_());
}

std::string
TransformHandler::version()
{
    return std::string(get_version_());
}

std::string
TransformHandler::help()
{
    return std::string(get_help_());
}

std::shared_ptr<PsnTransform>
TransformHandler::load()
{
    if (!instance)
        instance = load_();
    return instance;
}
std::string
TransformHandler::description()
{
    return std::string(get_description_());
}
} // namespace psn