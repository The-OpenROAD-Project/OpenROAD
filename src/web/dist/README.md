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
| `report.min.js` | inlined into a saved timing report; leaves out elk and netlistsvg |
| `vendor.min.css` | leaflet and golden-layout's base sheet |
| `gl-dark.min.css`, `gl-light.min.css` | the two `<style>` elements `theme.js` switches between |
| `app.min.css` | the app's own `style.css`, last in the cascade |

The four stylesheets are inlined into `index.min.html` for the served page, and
embedded separately for the saved report. Their order is load-bearing — see
`../src/vendor.css`.

The two served blobs are gzipped at build time and stored compressed; nothing
here is.

## Why they are checked in

The bundling runs under Bazel (`//src/web:app_bundle` and friends), but the
CMake build embeds the same assets and `cmake -B build` has to keep working on
its own — the macOS CI job and ORFS both build that way. So the output is
checked in and **both builds read it from here**, the way both builds read
`src/web/web.tcl`. It is the same arrangement as the generated ODB files and
their `Are-Odb-Files-Generated` check.

## Regenerating

After changing anything under `src/web/src/`, or bumping a dependency in
`src/web/package.json`:

```sh
bazel run //src/web/dist:dist
```

Then commit what it changed. `bazel test //src/web/dist:dist_tests` tells you
locally whether they are in sync. CI enforces it in the
`Are-Web-Bundles-Generated` workflow, which regenerates and fails on any
difference — no other job would notice, because the CMake build trusts whatever
is committed here.

[issue]: https://github.com/The-OpenROAD-Project/OpenROAD/issues/11065
