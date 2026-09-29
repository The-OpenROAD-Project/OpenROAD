# Generated browser bundles

Everything in this directory except this file and `BUILD` is **generated**. Do
not edit it by hand.

The web viewer used to load leaflet, golden-layout, three, elkjs and netlistsvg
from CDNs, one of them over plain http ([#11065][issue]). They now come from
npm and are bundled and minified by esbuild into the blobs below, which the
OpenROAD binary embeds and serves itself — so the browser only ever talks to
the OpenROAD process, and the viewer works on a machine with no network.

| File | Where it goes |
| --- | --- |
| `index.min.html` | served as `/index.html`; carries all four stylesheets inline |
| `app.min.js` | served as `/app.min.js`; the app plus every library it uses |
| `report.min.js` | inlined into a saved timing report; leaves out elk, netlistsvg and three |
| `THIRD_PARTY_LICENSES.txt` | served as `/THIRD_PARTY_LICENSES.txt`, and copied into every saved report |

The four stylesheets live inline in `index.min.html`, and a saved report copies
them from there. Their order is load-bearing — see `../src/vendor.css`.

What the binary embeds from here is gzipped at build time and stored
compressed; nothing here is.

## Why they are checked in

The bundling runs under Bazel (`//src/web:app_bundle` and friends), and the
Bazel build embeds its output directly. The CMake build cannot run it, yet
`cmake -B build` has to keep working on its own — the macOS CI job and ORFS
both build that way. So a copy of the output is checked in here, and **only the
CMake build reads it**. It is the same arrangement as the generated ODB files
and their `Are-Odb-Files-Generated` check.

## Regenerating

After changing anything under `src/web/src/`, or bumping a dependency in
`src/web/package.json`:

```sh
bazel run //src/web/dist:dist
```

Then commit what it changed. `bazel test //src/web/dist:dist_tests` tells you
locally whether they are in sync. CI enforces it in the
`Are-Web-Bundles-Generated` workflow, which rebuilds the bundles and fails on
any difference without waiting for a full build; Jenkins' `bazel test` runs the
same check, but only after building everything.

[issue]: https://github.com/The-OpenROAD-Project/OpenROAD/issues/11065
