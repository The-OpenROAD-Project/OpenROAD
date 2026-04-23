"""Rule to merge yosys share with additional plugins."""

def _merge_yosys_share_impl(ctx):
    out = ctx.actions.declare_directory(ctx.attr.name)
    inputs = ctx.files.share + ctx.files.plugins
    share_files = ctx.files.share

    commands = ["mkdir -p {out}/plugins".format(out = out.path)]
    for f in share_files:
        commands.append("rel={f}; rel=${{rel#*/share/}}; mkdir -p {out}/$(dirname $rel); cp {f} {out}/$rel".format(
            f = f.path,
            out = out.path,
        ))
    for p in ctx.files.plugins:
        commands.append("cp {p} {out}/plugins/{name}".format(
            p = p.path,
            out = out.path,
            name = p.basename,
        ))

    ctx.actions.run_shell(
        inputs = inputs,
        outputs = [out],
        command = " && ".join(commands),
    )
    return [DefaultInfo(files = depset([out]))]

merge_yosys_share = rule(
    implementation = _merge_yosys_share_impl,
    attrs = {
        "share": attr.label(mandatory = True),
        "plugins": attr.label_list(allow_files = True),
    },
)
