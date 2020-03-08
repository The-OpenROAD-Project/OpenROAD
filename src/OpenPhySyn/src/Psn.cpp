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

// Temproary fix for OpenSTA import ordering
#define THROW_DCL throw()

#include <Config.hpp>
#include <OpenPhySyn/PsnLogger.hpp>
#include <OpenSTA/dcalc/ArcDelayCalc.hh>
#include <OpenSTA/network/ConcreteNetwork.hh>
#include <OpenSTA/search/Search.hh>
#include <OpenSTA/search/Sta.hh>
#include <Psn.hpp>
#include <flute.h>
#include <tcl.h>
#include "FileUtils.hpp"
#include "PsnException.hpp"
#include "StringUtils.hpp"
#include "TransformHandler.hpp"

#ifdef OPENPHYSYN_AUTO_LINK
#include "StandardTransforms/BufferFanoutTransform/src/BufferFanoutTransform.hpp"
#include "StandardTransforms/ConstantPropagationTransform/src/ConstantPropagationTransform.hpp"
#include "StandardTransforms/GateCloningTransform/src/GateCloningTransform.hpp"
#include "StandardTransforms/HelloTransform/src/HelloTransform.hpp"
#include "StandardTransforms/PinSwapTransform/src/PinSwapTransform.hpp"
#endif

extern "C"
{
    extern int Psn_Init(Tcl_Interp* interp);
    extern int Sta_Init(Tcl_Interp* interp);
}
namespace sta
{
extern void        evalTclInit(Tcl_Interp* interp, const char* inits[]);
extern const char* tcl_inits[];
} // namespace sta

namespace psn
{
using sta::evalTclInit;
using sta::tcl_inits;

Psn* Psn::psn_instance_;
bool Psn::is_initialized_ = false;

Psn::Psn(DatabaseSta* sta) : sta_(sta), db_(nullptr), interp_(nullptr)
{
    if (sta == nullptr)
    {
        initializeDatabase();
        initializeSta();
    }
    exec_path_  = FileUtils::executablePath();
    db_         = sta_->db();
    settings_   = new DesignSettings();
    db_handler_ = new DatabaseHandler(sta_);
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
}

Psn::~Psn()
{
    delete settings_;
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

DatabaseHandler*
Psn::handler() const
{
    return db_handler_;
}

DesignSettings*
Psn::settings() const
{
    return settings_;
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
#ifndef OPENROAD_BUILD
    if (!is_initialized_)
    {
        PSN_LOG_CRITICAL("OpenPhySyn is not initialized!");
    }
#endif
    return psn_instance_;
}

int
Psn::loadTransforms()
{
    std::vector<psn::TransformHandler> handlers;
    int         load_count = 0;

#ifdef OPENPHYSYN_AUTO_LINK
#ifdef OPENPHYSYN_TRANSFORM_HELLO_TRANSFORM_ENABLED
    handlers.push_back(TransformHandler("hello_transform", std::make_shared<HelloTransform>()));
#endif
#ifdef OPENPHYSYN_TRANSFORM_BUFFER_FANOUT_ENABLED
    handlers.push_back(TransformHandler("buffer_fanout", std::make_shared<BufferFanoutTransform>()));
#endif
#ifdef OPENPHYSYN_TRANSFORM_GATE_CLONE_ENABLED
    handlers.push_back(TransformHandler("gate_clone", std::make_shared<GateCloningTransform>()));
#endif
#ifdef OPENPHYSYN_TRANSFORM_PIN_SWAP_ENABLED
    handlers.push_back(TransformHandler("pin_swap", std::make_shared<PinSwapTransform>()));
#endif
#ifdef OPENPHYSYN_TRANSFORM_CONSTANT_PROPAGATION_ENABLED
    handlers.push_back(TransformHandler("constant_propagation", std::make_shared<ConstantPropagationTransform>()));
#endif

#else
    std::string                        transforms_paths(
        FileUtils::joinPath(FileUtils::homePath(), ".OpenPhySyn/transforms") +
        ":" + FileUtils::joinPath(exec_path_, "./transforms") +
        ":" + FileUtils::joinPath(exec_path_, "../transforms"));
    const char* env_path   = std::getenv("PSN_TRANSFORM_PATH");

    if (env_path)
    {
        transforms_paths = std::string(env_path);
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
        std::vector<std::string> transforms_paths =
            FileUtils::readDirectory(transform_parent_path);
        for (auto& path : transforms_paths)
        {
            PSN_LOG_DEBUG("Loading transform {}", path);
            handlers.push_back(TransformHandler(path));
        }

        PSN_LOG_DEBUG("Found {} transforms under {}.", transforms_paths.size(),
                      transform_parent_path);
    }
#endif
    for (auto tr : handlers)
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
            PSN_LOG_WARN(
                "Transform {} was already loaded, discarding subsequent loads",
                tr_name);
        }
    }
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
            PSN_LOG_INFO("{}", transforms_info_[transform_name].version());
            return 0;
        }
        else if (args.size() && args[0] == "help")
        {
            PSN_LOG_INFO("{}", transforms_info_[transform_name].help());
            return 0;
        }
        else
        {

            PSN_LOG_INFO("Invoking {} transform", transform_name);
            int rc = transforms_[transform_name]->run(this, args);
            sta_->ensureLevelized();
            handler()->resetDelays();
            PSN_LOG_INFO("Finished {} transform ({})", transform_name, rc);
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

        PSN_LOG_RAW("OpenPhySyn: {}", PSN_VERSION_STRING);
    }
    else
    {

        PSN_LOG_INFO("OpenPhySyn: {}", PSN_VERSION_STRING);
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
    commands_str +=
        "print_version                         Print version\n"
        "version                               Print version\n"
        "help                                  Print help\n"
        "print_usage                           Print help\n"
        "print_license                         Print license information\n"
        "print_transforms                      List loaded transforms\n"
        "import_lef <file path>                Load LEF file\n"
        "import_def <file path>                Load DEF file\n"
        "import_lib <file path>                Load a liberty file\n"
        "import_liberty <file path>            Load a liberty file\n"
        "export_def <output file>              Write DEF file\n"
        "set_wire_rc <res> <cap>               Set resistance & capacitance "
        "per micron\n"
        "set_max_area <area>                   Set maximum design area\n"
        "optimize_design [<options>]           Perform timing optimization on "
        "the design\n"
        "optimize_fanout <options>             Buffer high-fanout nets\n"
        "optimize_power [<options>]            Perform power optimization on "
        "the design\n"
        "transform <transform name> <args>     Run transform on the loaded "
        "design\n"
        "has_transform <transform name>        Checks if a specific transform "
        "is loaded\n"
        "design_area                           Returns total design cell area\n"
        "link <design name>                    Link design top module\n"
        "link_design <design name>             Link design top module\n"
        "sta <OpenSTA commands>                Run OpenSTA commands\n"
        "make_steiner_tree <net>               Construct steiner tree for the "
        "provided net\n"
        "set_log <log level>                   Set log level [trace, debug, "
        "info, "
        "warn, error, critical, off]\n"
        "set_log_level <log level>             Set log level [trace, debug, "
        "info, warn, error, critical, off]\n"
        "set_log_pattern <pattern>             Set log printing pattern, refer "
        "to spdlog logger for pattern formats";
    PSN_LOG_RAW("{}", commands_str);
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
        PSN_LOG_RAW("");
    }

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
        PSN_LOG_ERROR("Invalid log level {}", level);
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
        PSN_LOG_ERROR("Failed to open {}", script_path);
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
        PSN_LOG_ERROR("Failed to open {}", script_path);
        PSN_LOG_ERROR("{}", e.what());
        return -1;
    }
    if (evaluateTclCommands(script_content.c_str()) == TCL_ERROR)
    {
        return -1;
    }
    return 1;
}
void
Psn::setWireRC(float res_per_micon, float cap_per_micron)
{
    handler()->resetDelays();
    settings()
        ->setResistancePerMicron(res_per_micon)
        ->setCapacitancePerMicron(cap_per_micron);
}

int
Psn::setWireRC(const char* layer_name)
{
    auto tech = db_->getTech();

    if (!tech)
    {
        PSN_LOG_ERROR("Could not find any loaded technology file.");
        return -1;
    }

    auto layer = tech->findLayer(layer_name);
    if (!layer)
    {
        PSN_LOG_ERROR("Could not find layer with the name {}.", layer_name);
        return -1;
    }
    auto  width         = handler()->dbuToMicrons(layer->getWidth());
    float res_per_micon = (layer->getResistance() / width) * 1E6;
    float cap_per_micron =
        (handler()->dbuToMicrons(1) * width * layer->getCapacitance() +
         layer->getEdgeCapacitance() * 2.0) *
        1E-12 * 1E6;
    setWireRC(res_per_micon, cap_per_micron);
    return 1;
}

// Private methods:
int
Psn::initializeDatabase()
{
    if (db_ == nullptr)
    {
        db_ = odb::dbDatabase::create();
    }
    return 0;
}
int
Psn::initializeSta(Tcl_Interp* interp)
{
    if (interp == nullptr)
    {
        // This is a very bad solution! but temporarily until
        // dbSta can take a database without interp..
        interp = Tcl_CreateInterp();
        Tcl_Init(interp);
    }
    sta_ = new DatabaseSta;
    sta_->init(interp, db_);
    return 0;
}

void
Psn::clearDatabase()
{
    handler()->clear();
}

} // namespace psn