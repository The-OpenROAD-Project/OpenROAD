# Generated browser bundle

Everything in this directory except this file and `BUILD` is **generated**. Do
not edit it by hand.

The web viewer's libraries come from npm and are bundled and minified by
esbuild ([#11065][issue]); see `../README.md`. The files below are that output,
which the OpenROAD binary embeds and serves itself.

| File | Where it goes |
| --- | --- |
| `index.html` | served as `/index.html`; carries all four stylesheets inline |
| `app.min.js` | served as `/app.min.js`; the app plus every library it uses |
| `THIRD_PARTY_LICENSES.txt` | served as `/THIRD_PARTY_LICENSES.txt` |
| `report.min.js` | inlined into a saved timing report; leaves out netlistsvg and three |

## Why they are checked in

The bundling runs under Bazel (`//src/web:app_bundle` and friends), and the
Bazel build embeds its output directly. The CMake build cannot run it, yet
`cmake -B build` has to keep working on its own: the macOS CI job and ORFS
both build that way. So a copy of the output is checked in here, and **only the
CMake build reads it**. It is the same arrangement as the generated ODB files
and their `Are-Odb-Files-Generated` check. When the CMake build goes, so does
this directory.

## Regenerating

After changing the JavaScript, CSS, HTML or skin under `src/web/src/`,
`esbuild.config.mjs`, or a dependency in `src/web/package.json`:

```sh
bazelisk run //src/web/dist:dist
```

Then commit what it changed. `bazelisk test //src/web/dist:all` tells you
locally whether they are in sync, and the `Are-Web-Bundles-Generated` workflow
fails a PR whose copy is stale.

[issue]: https://github.com/The-OpenROAD-Project/OpenROAD/issues/11065
