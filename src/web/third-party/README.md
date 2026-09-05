# Vendored browser libraries

The web viewer used to pull these libraries from CDNs, one of them over plain
http (issue #11065). They are now checked in here, embedded in the OpenROAD
binary and served by `web_server` itself, so the browser only ever talks to the
OpenROAD process and the viewer works on a machine with no network.

| Directory | Package | Version | Used for |
| --- | --- | --- | --- |
| `leaflet/` | leaflet | 1.9.4 | the tiled layout view |
| `golden-layout/` | golden-layout | 2.6.0 | the dockable panel layout |
| `three/` | three | 0.160.0 | the 3D viewer |
| `elkjs/` | elkjs | 0.9.3 | schematic placement and routing |
| `netlistsvg/` | netlistsvg | 1.0.2 | schematic rendering |

Each directory keeps the layout its package ships, because the stylesheets
reach their icons through relative urls (`leaflet.css` asks for
`images/*.png`, the golden-layout themes for `../../img/*.png`). The served
paths mirror this directory, so `leaflet/leaflet.css` is served as
`/third-party/leaflet/leaflet.css`.

Two ordering constraints are easy to break:

- `netlistsvg.bundle.js` does not bundle ELK; it reads `window.ELK`, so
  `elkjs/elk.bundled.js` must load first in `index.html`.
- `three` and `golden-layout` are ES modules, resolved from bare specifiers
  through the import map in `index.html`. `golden-layout` publishes no browser
  build, so `golden-layout.esm.js` is bundled from its `dist/esm` tree — a
  single file both because it saves requests and because the saved report
  inlines it as a `data:` URI, where relative imports would not resolve.

## Upgrading

Every file here is generated. Do not edit them by hand: `update_vendor.py
--check` compares them against `vendor.lock.json` and CI runs it.

```sh
# edit the version in PACKAGES, then
./update_vendor.py --rewrite-lock    # downloads, rebuilds, records new digests
./update_vendor.py --check           # what CI runs
```

Without `--rewrite-lock` the script refuses a tarball whose digest differs from
the lock: the npm registry is immutable, so a mismatch means either a stale lock
or a tampered download. Adding or removing a file also means updating the asset
lists in `src/web/BUILD` and `src/web/CMakeLists.txt`, which is what gets them
embedded and served.

The licenses of all five packages are vendored alongside the code (MIT for
leaflet, golden-layout, three and netlistsvg; EPL-2.0 for elkjs).
