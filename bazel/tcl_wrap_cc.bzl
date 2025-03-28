###############################################################################
##
## BSD 3-Clause License
##
## Copyright (c) 2025, Precision Innovations Inc.
## Copyright (c) 2025 Google LLC
## All rights reserved.
##
## Redistribution and use in source and binary forms, with or without
## modification, are permitted provided that the following conditions are met:
##
## * Redistributions of source code must retain the above copyright notice, this
##   list of conditions and the following disclaimer.
##
## * Redistributions in binary form must reproduce the above copyright notice,
##   this list of conditions and the following disclaimer in the documentation
##   and#or other materials provided with the distribution.
##
## * Neither the name of the copyright holder nor the names of its
##   contributors may be used to endorse or promote products derived from
##   this software without specific prior written permission.
##
## THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
## AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
## IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
## ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
## LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
## CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
## SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
## INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
## CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
## ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
## POSSIBILITY OF SUCH DAMAGE.
##
###############################################################################

"""A TCL SWIG wrapping rule for google3.

These rules generate a C++ src file that is expected to be used as srcs in
cc_library or cc_binary rules. See below for expected usage.

cc_library(srcs=[':tcl_foo"])
tcl_wrap_cc(name = "tcl_foo", srcs=["exception.i"],...)
"""
TclSwigInfo = provider("TclSwigInfo for taking dependencies on other swig info rules", fields = ["transitive_srcs", "includes", "swig_options"])

def _get_transative_srcs(srcs, deps):
    return depset(
        srcs,
        transitive = [dep[TclSwigInfo].transitive_srcs for dep in deps],
    )

def _get_transative_includes(local_includes, deps):
    return depset(
        local_includes,
        transitive = [dep[TclSwigInfo].includes for dep in deps],
    )

def _get_transative_options(options, deps):
    return depset(
        options,
        transitive = [dep[TclSwigInfo].swig_options for dep in deps],
    )

def _tcl_wrap_cc_impl(ctx):
    """Generates a single C++ file from the provided srcs in a DefaultInfo.
    """
    if len(ctx.files.srcs) > 1 and not ctx.attr.root_swig_src:
        fail("If multiple src files are provided root_swig_src must be specified.")

    root_file = ctx.file.root_swig_src
    root_file = root_file if root_file != None else ctx.files.srcs[0]

    root_label = ctx.attr.root_swig_src
    root_label = root_label if root_label != None else ctx.attr.srcs[0]
    root_label = root_label.label

    outfile_name = ctx.attr.out if ctx.attr.out else ctx.attr.name + ".cc"
    output_file = ctx.actions.declare_file(outfile_name)

    include_root_directory = ""
    if ctx.label.package:
        include_root_directory = ctx.label.package + "/"

    src_inputs = _get_transative_srcs(ctx.files.srcs + ctx.files.root_swig_src, ctx.attr.deps)
    includes_paths = _get_transative_includes(
        ["{}{}".format(include_root_directory, include) for include in ctx.attr.swig_includes],
        ctx.attr.deps,
    )
    swig_options = _get_transative_options(ctx.attr.swig_options, ctx.attr.deps)

    args = ctx.actions.args()
    args.add("-tcl8")
    args.add("-c++")
    if ctx.attr.module:
        args.add("-module")
        args.add(ctx.attr.module)
    if ctx.attr.namespace_prefix:
        args.add("-namespace")
        args.add("-prefix")
        args.add(ctx.attr.namespace_prefix)
    args.add_all(swig_options.to_list())
    args.add_all(includes_paths.to_list(), format_each = "-I%s")
    args.add("-o")
    args.add(output_file.path)
    args.add(root_file.path)

    ctx.actions.run(
        outputs = [output_file],
        inputs = src_inputs,
        arguments = [args],
        tools = ctx.files._swig,
        executable = ([file for file in ctx.files._swig if file.basename == "swig"][0]),
    )
    return [
        DefaultInfo(files = depset([output_file])),
        TclSwigInfo(
            transitive_srcs = src_inputs,
            includes = includes_paths,
            swig_options = swig_options,
        ),
    ]

tcl_wrap_cc = rule(
    implementation = _tcl_wrap_cc_impl,
    attrs = {
        "deps": attr.label_list(
            allow_empty = True,
            doc = "tcl_wrap_cc dependencies",
            providers = [TclSwigInfo],
        ),
        "module": attr.string(
            mandatory = False,
            default = "",
            doc = "swig module",
        ),
        "namespace_prefix": attr.string(
            mandatory = False,
            default = "",
            doc = "swig namespace prefix",
        ),
        "out": attr.string(
            doc = "The name of the C++ source file generated by these rules.",
        ),
        "root_swig_src": attr.label(
            allow_single_file = [".swig", ".i"],
            doc = """If more than one swig file is included in this rule.
            The root file must be explicitly provided. This is the which will be passed to
            swig for generation.""",
        ),
        "srcs": attr.label_list(
            allow_empty = False,
            allow_files = [".i", ".swig", ".h", ".hpp"],
            doc = "Swig files that generate C++ files",
        ),
        "swig_includes": attr.string_list(
            doc = "List of directories relative to the BUILD file to append as -I flags to SWIG",
        ),
        "swig_options": attr.string_list(
            doc = "args to pass directly to the swig binary",
        ),
        "_swig": attr.label(
            default = "@org_swig//:swig_stable",
            allow_files = True,
            cfg = "exec",
        ),
    },
)
