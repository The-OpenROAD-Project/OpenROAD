# Browser libraries

The web viewer used to pull these libraries from CDNs, one of them over plain
http (issue #11065). The build now fetches them from the npm registry, embeds
them in the OpenROAD binary and `web_server` serves them itself, so the browser
only ever talks to the OpenROAD process and the viewer works on a machine with
no network.

Nothing here is checked in. `packages.json` is the manifest — version and
tarball sha256 per package — and it is the only place those live:

- Bazel reads it in [`//bazel:web_third_party.bzl`](../../../bazel/web_third_party.bzl),
  which turns each entry into an `http_archive`.
- CMake reads it through `fetch_packages.py`, at configure time.

| Package | Used for |
| --- | --- |
| leaflet | the tiled layout view |
| golden-layout | the dockable panel layout |
| three | the 3D viewer |
| elkjs | schematic placement and routing |
| netlistsvg | schematic rendering |

Each package keeps the layout it ships, because the stylesheets reach their
icons through relative urls (`leaflet.css` asks for `images/*.png`, the
golden-layout themes for `../../img/*.png`). The served paths mirror that, so
leaflet's stylesheet is served as `/third-party/leaflet/leaflet.css`. Every
package's license is served alongside its code, at
`/third-party/<package>/LICENSE` (MIT for leaflet, golden-layout, three and
netlistsvg; EPL-2.0 for elkjs).

Two ordering constraints are easy to break:

- `netlistsvg.bundle.js` does not bundle ELK; it reads `window.ELK`, so
  `elk.bundled.js` must load first in `index.html`.
- `three` and `golden-layout` are ES modules, resolved from bare specifiers
  through the import map in `index.html`. `golden-layout` publishes no browser
  build, so the build bundles its `dist/esm` tree with esbuild — a single file
  both because it saves requests and because the saved report inlines it as a
  `data:` URI, where relative imports would not resolve. esbuild is a static
  binary, pinned per platform in the manifest like everything else.

## Upgrading

Change `version` and `sha256` together — the registry is immutable, so a wrong
digest means a wrong download, never a stale one:

```sh
./fetch_packages.py --print-sha256 leaflet@1.9.5   # the new digest
```

Adding or removing a served file means editing the `files` map, and nothing
else: both build systems derive their asset lists from it.

## Building without network access

Both build systems verify every byte against the manifest, so priming a cache
is enough:

```sh
./fetch_packages.py --download-only --tarball-dir ~/openroad-web-tarballs
cmake -B build -DWEB_THIRD_PARTY_TARBALL_DIR=~/openroad-web-tarballs
```

For Bazel the equivalent is its own `--distdir`, which needs the tarballs named
as their urls end.
