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

#ifndef __PSN_PSN_EXCEPTION__
#define __PSN_PSN_EXCEPTION__
#include <exception>
#include <string>

namespace psn
{
typedef long ErrorCode;

namespace Error
{

enum File
{
    ERR_FILE_RW
};

enum Parse
{
    ERR_NO_TECH,
    ERR_INVALID_LIBERTY
};
enum Transform
{
    ERR_NOT_FOUND,
    ERR_LOAD
};

enum Common
{
    ERR_COMMON_UNRECOGNIZED,
    ERR_PROGRAM_OPTIONS,
    ERR_FLUTE_NO_LUT
};

} // namespace Error

class PsnException : public std::exception
{
public:
    explicit PsnException(
        const char* message, const char* name = "PsnException",
        ErrorCode code = Error::Common::ERR_COMMON_UNRECOGNIZED);

    explicit PsnException(
        const std::string& message, const char* name = "PsnException",
        ErrorCode code = Error::Common::ERR_COMMON_UNRECOGNIZED);

    void        setMessage(std::string& message);
    void        setMessage(const char* message);
    std::string message() const;

    ErrorCode code() const;

    std::string name() const;

    virtual ~PsnException() throw();

    virtual const char* what() const throw();

protected:
    std::string msg_;
    std::string name_;
    std::string fmt_msg_;
    ErrorCode   code_;
};

class FileException : public PsnException
{
public:
    explicit FileException();
    explicit FileException(const char* message)        = delete;
    explicit FileException(const std::string& message) = delete;

private:
    int errno_;
};

class FluteInitException : public PsnException
{
public:
    explicit FluteInitException();
    explicit FluteInitException(const char* message)        = delete;
    explicit FluteInitException(const std::string& message) = delete;

private:
};

class LoadTransformException : public PsnException
{
public:
    explicit LoadTransformException();
    explicit LoadTransformException(const char* message)        = delete;
    explicit LoadTransformException(const std::string& message) = delete;

private:
};

class NoTechException : public PsnException
{
public:
    explicit NoTechException();
    explicit NoTechException(const char* message)        = delete;
    explicit NoTechException(const std::string& message) = delete;

private:
};

class ParseLibertyException : public PsnException
{
public:
    explicit ParseLibertyException(const char* message);
    explicit ParseLibertyException(const std::string& message);

private:
};

class ProgramOptionsException : public PsnException
{
public:
    explicit ProgramOptionsException(const char* message);
    explicit ProgramOptionsException(const std::string& message);

private:
};

class SteinerException : public PsnException
{
public:
    explicit SteinerException();
    explicit SteinerException(const char* message)        = delete;
    explicit SteinerException(const std::string& message) = delete;

private:
};

class TransformNotFoundException : public PsnException
{
public:
    explicit TransformNotFoundException();
    explicit TransformNotFoundException(const char* message)        = delete;
    explicit TransformNotFoundException(const std::string& message) = delete;

private:
};

class UnimplementedMethodException : public PsnException
{
public:
    explicit UnimplementedMethodException();
    explicit UnimplementedMethodException(const char* message)        = delete;
    explicit UnimplementedMethodException(const std::string& message) = delete;

private:
};

} // namespace psn
#endif
