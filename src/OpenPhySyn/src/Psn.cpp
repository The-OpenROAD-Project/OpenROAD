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

#include "Psn.hpp"
#include <tcl.h>
#include "Config.hpp"
#include "FileUtils.hpp"
#include "OpenPhySyn/DatabaseHandler.hpp"
#include "OpenPhySyn/PsnGlobal.hpp"
#include "OpenPhySyn/PsnLogger.hpp"
#include "PsnException.hpp"
#include "StringUtils.hpp"
#include "TransformHandler.hpp"
#include "db_sta/dbSta.hh"

#include "opendp/Opendp.h"
#include "openroad/OpenRoad.hh"
#include "resizer/Resizer.hh"

#ifdef OPENPHYSYN_AUTO_LINK
#include "StandardTransforms/BufferFanoutTransform/src/BufferFanoutTransform.hpp"
#include "StandardTransforms/ConstantPropagationTransform/src/ConstantPropagationTransform.hpp"
#include "StandardTransforms/GateCloningTransform/src/GateCloningTransform.hpp"
#include "StandardTransforms/PinSwapTransform/src/PinSwapTransform.hpp"
#include "StandardTransforms/RepairTimingTransform/src/RepairTimingTransform.hpp"
#include "StandardTransforms/TimingBufferTransform/src/TimingBufferTransform.hpp"
#endif

extern "C"
{
    extern int Psn_Init(Tcl_Interp* interp);
}

namespace psn
{

Psn* Psn::psn_instance_;
bool Psn::is_initialized_ = false;

Psn::Psn(DatabaseSta* sta) : sta_(sta), db_(nullptr), interp_(nullptr)
{
    exec_path_  = FileUtils::executablePath();
    db_         = sta_->db();
    db_handler_ = new DatabaseHandler(this, sta_);
}

void
Psn::initialize(DatabaseSta* sta, bool load_transforms, Tcl_Interp* interp,
                bool import_psn_namespace, bool print_psn_version,
                bool setup_sta_tcl)
{
    psn_instance_ = new Psn(sta);
    if (load_transforms)
    {
        psn_instance_->loadTransforms();
    }
    if (interp != nullptr)
    {
        psn_instance_->setupInterpreter(interp, import_psn_namespace,
                                        print_psn_version, setup_sta_tcl);
    }

    is_initialized_ = true;
    setupCallbacks();
}

Psn::~Psn()
{
    delete db_handler_;
}

Database*
Psn::database() const
{
    return db_;
}
DatabaseSta*
Psn::sta() const
{
    return sta_;
}

LibraryTechnology*
Psn::tech() const
{
    return db_->getTech();
}

bool
Psn::hasLiberty() const
{
    return handler()->hasLiberty();
}

bool
Psn::hasDesign() const
{
    return (database() && database()->getChip() != nullptr);
}

DatabaseHandler*
Psn::handler() const
{
    return db_handler_;
}

Psn&
Psn::instance()
{
    if (!is_initialized_)
    {
        PSN_LOG_CRITICAL("OpenPhySyn is not initialized!");
    }
    return *psn_instance_;
}
Psn*
Psn::instancePtr()
{
    return psn_instance_;
}

int
Psn::loadTransforms()
{
    int load_count = 0;

    OPENPHYSYN_LOAD_TRANSFORMS(transforms_, transforms_info_, load_count);

#ifdef OPENPHYSYN_ENABLE_DYNAMIC_TRANSFORM_LIBRARY
    std::string transforms_paths(
        FileUtils::joinPath(FileUtils::homePath(), ".OpenPhySyn/transforms") +
        ":" + FileUtils::joinPath(exec_path_, "transforms") + ":" +
        std::string(PSN_TRANSFORM_INSTALL_FULL_PATH));
    const char* env_path = std::getenv("PSN_TRANSFORM_PATH");

    if (env_path)
    {
        transforms_paths =
            transforms_paths + std::string(":") + std::string(env_path);
    }

    std::vector<std::string> transforms_dirs =
        StringUtils::split(transforms_paths, ":");

    for (auto& transform_parent_path : transforms_dirs)
    {
        if (!transform_parent_path.length())
        {
            continue;
        }
        if (!FileUtils::isDirectory(transform_parent_path))
        {
            continue;
        }
        PSN_LOG_DEBUG("Searching for transforms at {}", transform_parent_path);
        std::vector<std::string> transforms_paths =
            FileUtils::readDirectory(transform_parent_path, true);
        for (auto& path : transforms_paths)
        {
            PSN_LOG_DEBUG("Loading transform {}", path);
            handlers_.push_back(psn::TransformHandler(path));
        }

        PSN_LOG_DEBUG("Found {} transforms under {}.", transforms_paths.size(),
                      transform_parent_path);
    }

    for (auto tr : handlers_)
    {
        std::string tr_name(tr.name());
        if (!transforms_.count(tr_name))
        {
            auto transform            = tr.load();
            transforms_[tr_name]      = transform;
            transforms_info_[tr_name] = TransformInfo(
                tr_name, tr.help(), tr.version(), tr.description());
            load_count++;
        }
        else
        {
            PSN_LOG_DEBUG(
                "Transform {} was already loaded, discarding subsequent loads",
                tr_name);
        }
    }
#endif
    PSN_LOG_DEBUG("Loaded {} transforms.", load_count);
    return load_count;
}

bool
Psn::hasTransform(std::string transform_name)
{
    return transforms_.count(transform_name);
}

int
Psn::runTransform(std::string transform_name, std::vector<std::string> args)
{
    if (!database() || database()->getChip() == nullptr)
    {
        PSN_LOG_ERROR("Could not find any loaded design.");
        return -1;
    }
    try
    {
        if (!transforms_.count(transform_name))
        {
            throw TransformNotFoundException();
        }
        if (args.size() && args[0] == "version")
        {
            PSN_LOG_INFO(transforms_info_[transform_name].version());
            return 0;
        }
        else if (args.size() && args[0] == "help")
        {
            PSN_LOG_INFO(transforms_info_[transform_name].help());
            return 0;
        }
        else
        {

            PSN_LOG_INFO("Invoking", transform_name, "transform");
            int rc = transforms_[transform_name]->run(this, args);
            sta_->ensureLevelized();
            handler()->resetDelays();
            PSN_LOG_INFO("Finished", transform_name, "transform (", rc, ")");
            return rc;
        }
    }
    catch (PsnException& e)
    {
        PSN_LOG_ERROR(e.what());
        return -1;
    }
}

Tcl_Interp*
Psn::interpreter() const
{
    return interp_;
}
int
Psn::setupInterpreter(Tcl_Interp* interp, bool import_psn_namespace,
                      bool print_psn_version, bool setup_sta)
{
    if (interp_)
    {
        PSN_LOG_WARN("Multiple interpreter initialization!");
    }
    interp_ = interp;
    if (Psn_Init(interp) == TCL_ERROR)
    {
        return TCL_ERROR;
    }
    if (setup_sta)
    {
        const char* tcl_psn_setup =
#include "Tcl/SetupPsnSta.tcl"
            ;
        if (evaluateTclCommands(tcl_psn_setup) != TCL_OK)
        {
            return TCL_ERROR;
        }
    }
    else
    {

        const char* tcl_psn_setup =
#include "Tcl/SetupPsn.tcl"
            ;
        if (evaluateTclCommands(tcl_psn_setup) != TCL_OK)
        {
            return TCL_ERROR;
        }
    }

    const char* tcl_define_cmds =
#include "Tcl/DefinePSNCommands.tcl"
        ;
    if (evaluateTclCommands(tcl_define_cmds) != TCL_OK)
    {
        return TCL_ERROR;
    }

    if (import_psn_namespace)
    {
        const char* tcl_psn_import =
#include "Tcl/ImportNS.tcl"
            ;
        if (evaluateTclCommands(tcl_psn_import) != TCL_OK)
        {
            return TCL_ERROR;
        }
    }
    if (print_psn_version)
    {
        const char* tcl_print_version =
#include "Tcl/PrintVersion.tcl"
            ;
        if (evaluateTclCommands(tcl_print_version) != TCL_OK)
        {
            return TCL_ERROR;
        }
    }

    return TCL_OK;
}
int
Psn::evaluateTclCommands(const char* commands) const
{
    if (!interp_)
    {
        PSN_LOG_ERROR("Tcl Interpreter is not initialized");
        return TCL_ERROR;
    }
    return Tcl_Eval(interp_, commands);
}
void
Psn::printVersion(bool raw_str)
{
    if (raw_str)
    {

        PSN_LOG_RAW("OpenPhySyn:", PSN_VERSION_STRING);
    }
    else
    {

        PSN_LOG_INFO("OpenPhySyn:", PSN_VERSION_STRING);
    }
}
void
Psn::printUsage(bool raw_str, bool print_transforms, bool print_commands)
{
    PSN_LOG_RAW("");
    if (print_commands)
    {
        printCommands(true);
    }
    PSN_LOG_RAW("");
    if (print_transforms)
    {
        printTransforms(true);
    }
}
void
Psn::printLicense(bool raw_str)
{
    std::string license = R"===<><>===(BSD 3-Clause License
Copyright (c) 2019, SCALE Lab, Brown University
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

* Neither the name of the copyright holder nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE."
)===<><>===";
    license = std::string("OpenPhySyn ") + PSN_VERSION_STRING + "\n" + license;
    PSN_LOG_RAW("");
    if (raw_str)
    {
        PSN_LOG_RAW(license);
    }
    else
    {
        PSN_LOG_INFO(license);
    }
}
void
Psn::printCommands(bool raw_str)
{
    if (raw_str)
    {
        PSN_LOG_RAW("Available command: ");
    }
    else
    {

        PSN_LOG_INFO("Available commands: ");
    }
    PSN_LOG_RAW("");
    std::string commands_str;
    "design_area			Report design total cell area\n"
    "has_transform			Check if the specified transform is "
    "loaded\n"
    "optimize_fanout			Perform maximum-fanout based "
    "buffering\n"
    "optimize_logic			Perform logic optimization\n"
    "optimize_power			Perform power optimization\n"
    "pin_swap			Perform timing optimization by "
    "commutative pin swapping\n"
    "print_transforms		Print loaded transforms\n"
    "print_usage			Print usage instructions\n"
    "print_version			Print tool version\n"
    "propagate_constants		Perform logic optimization by constant "
    "propgation\n"
    "timing_buffer			Repair violations through buffer tree "
    "insertion\n"
    "repair_timing			Repair design timing and electrical "
    "violations "
    "through resizing, buffer insertion, and pin-swapping\n"
    "set_log				Alias for "
    "set_log_level\n"
    "set_log_level			Set log level [trace, debug, info, "
    "warn, error, critical, off]\n"
    "transform			Run loaded transform\n"
    "version				Alias for "
    "print_version\n";
    PSN_LOG_RAW(commands_str);
}
void
Psn::printTransforms(bool raw_str)
{
    if (raw_str)
    {
        PSN_LOG_RAW("Loaded transforms: ");
    }
    else
    {

        PSN_LOG_INFO("Loaded transforms: ");
    }
    PSN_LOG_RAW("");
    std::string transform_str;
    for (auto it = transforms_.begin(); it != transforms_.end(); ++it)
    {
        transform_str = it->first;
        transform_str += " (";
        transform_str += transforms_info_[it->first].version();
        transform_str += "): ";
        transform_str += transforms_info_[it->first].description();
        if (raw_str)
        {
            PSN_LOG_RAW(transform_str);
        }
        else
        {
            PSN_LOG_INFO(transform_str);
        }
    }
    PSN_LOG_RAW("");

} // namespace psn

int
Psn::setLogLevel(const char* level)
{
    std::string level_str(level);
    std::transform(level_str.begin(), level_str.end(), level_str.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    if (level_str == "trace")
    {
        return setLogLevel(LogLevel::trace);
    }
    else if (level_str == "debug")
    {
        return setLogLevel(LogLevel::debug);
    }
    else if (level_str == "info")
    {
        return setLogLevel(LogLevel::info);
    }
    else if (level_str == "warn")
    {
        return setLogLevel(LogLevel::warn);
    }
    else if (level_str == "error")
    {
        return setLogLevel(LogLevel::error);
    }
    else if (level_str == "critical")
    {
        return setLogLevel(LogLevel::critical);
    }
    else if (level_str == "off")
    {
        return setLogLevel(LogLevel::off);
    }
    else
    {
        PSN_LOG_ERROR("Invalid log level", level);
        return false;
    }
    return true;
} // namespace psn
int
Psn::setLogPattern(const char* pattern)
{
    PsnLogger::instance().setPattern(pattern);

    return true;
}
int
Psn::setLogLevel(LogLevel level)
{
    PsnLogger::instance().setLevel(level);
    return true;
}
int
Psn::setupInterpreterReadline()
{
    const char* rl_setup =
#include "Tcl/Readline.tcl"
        ;
    return evaluateTclCommands(rl_setup);
}

int
Psn::sourceTclScript(const char* script_path)
{
    if (!FileUtils::pathExists(script_path))
    {
        PSN_LOG_ERROR("Failed to open", script_path);
        return -1;
    }
    if (interp_ == nullptr)
    {
        PSN_LOG_ERROR("Tcl Interpreter is not initialized");
        return -1;
    }
    std::string script_content;
    try
    {
        script_content = FileUtils::readFile(script_path);
    }
    catch (FileException& e)
    {
        PSN_LOG_ERROR("Failed to open", script_path);
        PSN_LOG_ERROR(e.what());
        return -1;
    }
    if (evaluateTclCommands(script_content.c_str()) == TCL_ERROR)
    {
        return -1;
    }
    return 1;
}
void
Psn::setupCallbacks()
{
    // Legalizer
    psn_instance_->handler()->setLegalizer([=](int max_displacment) -> bool {
        auto openroad = ord::OpenRoad::openRoad();
        auto opendp   = openroad->getOpendp();
        opendp->detailedPlacement(max_displacment);
        return true;
    });

    // Dont use cells
    psn_instance_->handler()->setDontUseCallback(
        [=](LibraryCell* cell) -> bool {
            auto openroad = ord::OpenRoad::openRoad();
            auto resizer  = openroad->getResizer();
            return resizer->dontUse(cell);
        });

    // Wire parasitics
    psn_instance_->handler()->setWireRC(
        [=]() -> float {
            auto openroad = ord::OpenRoad::openRoad();
            auto resizer  = openroad->getResizer();
            return resizer->wireResistance();
        },
        [=]() -> float {
            auto openroad = ord::OpenRoad::openRoad();
            auto resizer  = openroad->getResizer();
            return resizer->wireCapacitance();
        });
    psn_instance_->handler()->setComputeParasiticsCallback([=](Net* net) {
        auto openroad = ord::OpenRoad::openRoad();
        auto resizer  = openroad->getResizer();
        resizer->estimateWireParasitic(net);
    });
    // Max area and design arae
    psn_instance_->handler()->setMaximumArea([=]() -> float {
        auto openroad = ord::OpenRoad::openRoad();
        auto resizer  = openroad->getResizer();
        return resizer->maxArea();
    });
    psn_instance_->handler()->setUpdateDesignArea([=](float new_area) {
        auto  openroad             = ord::OpenRoad::openRoad();
        auto  resizer              = openroad->getResizer();
        float current_resizer_area = resizer->designArea();
        float delta                = new_area - current_resizer_area;
        return resizer->designAreaIncr(delta);
    });
}

// Private methods:

void
Psn::clearDatabase()
{
    handler()->clear();
}

} // namespace psn