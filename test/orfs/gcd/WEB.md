# Web GUI and Static HTML Reports

## Web GUI

Open the live web-based viewer for a completed stage:

    bazelisk run //test/orfs/gcd:gcd_route_web

This loads the routed design into OpenROAD and starts the web viewer on
port 8088. A browser window opens automatically. The live viewer supports
interactive inspection, timing analysis, and hierarchy browsing.

## Static HTML Report

Generate a self-contained static HTML viewer — same layout and widgets as
the live web viewer, but with all data pre-embedded (zero click-and-wait):

    bazelisk run //test/orfs/gcd:gcd_route_html

Opens in the browser automatically, just like `gcd_route_web`.

## A/B: Live Web Viewer vs. Static HTML

Run both side by side to compare the live interactive viewer against the
static snapshot:

    bazelisk run //test/orfs/gcd:gcd_route_web &
    bazelisk run //test/orfs/gcd:gcd_route_html

## Demo: Histogram-only prototype

A simpler prototype that only renders the endpoint slack histogram and
timing path table (used during development for A/B comparison):

    bazelisk run //test/orfs/gcd:gcd_route_demo_html

## Compare across stages

Extract JSON payloads for multiple stages, then combine them into a
single viewer with tabs. Flip between stages to watch the endpoint slack
histogram animate as timing evolves through the flow:

    bazelisk build //test/orfs/gcd:gcd_floorplan_json
    bazelisk build //test/orfs/gcd:gcd_place_json
    bazelisk build //test/orfs/gcd:gcd_cts_json
    bazelisk build //test/orfs/gcd:gcd_route_json

    node src/web/src/render-static.js \
      --label floorplan bazel-bin/test/orfs/gcd/gcd_floorplan.json \
      --label place     bazel-bin/test/orfs/gcd/gcd_place.json \
      --label cts       bazel-bin/test/orfs/gcd/gcd_cts.json \
      --label route     bazel-bin/test/orfs/gcd/gcd_route.json \
      -o gcd_stages.html

## Compare across variants

Compare different parameter sweeps (e.g. placement density) at the same
stage:

    node src/web/src/render-static.js \
      --label "base"  gcd_base_route.json \
      --label "dense" gcd_dense_route.json \
      -o density_comparison.html

## Compare across time

Rebuild after an OpenROAD change and compare before/after:

    node src/web/src/render-static.js \
      --label "before" gcd_route_before.json \
      --label "after"  gcd_route_after.json \
      -o regression_check.html

## JSON payload extraction

Extract raw timing data as JSON for custom analysis or external tools:

    bazelisk build //test/orfs/gcd:gcd_route_json

The payload contains the endpoint slack histogram and top timing paths
in the same format as the live web viewer's WebSocket protocol.

## Debugging the static HTML viewer

To debug the static HTML without a human in the loop, use puppeteer-core
with the system chromium in headless mode:

```bash
# One-time setup
cd /tmp && npm init -y && npm install puppeteer-core

# Generate the HTML
node src/web/src/render-static-page.js payload.json -o ~/test.html

# Run headless Chrome and capture errors + screenshot
node --input-type=module << 'SCRIPT'
import puppeteer from '/tmp/node_modules/puppeteer-core/lib/esm/puppeteer/puppeteer-core.js';
const browser = await puppeteer.launch({
    executablePath: '/snap/bin/chromium',
    headless: true,
    args: ['--no-sandbox', '--allow-file-access-from-files', '--disable-web-security']
});
const page = await browser.newPage();
await page.evaluateOnNewDocument(() => { localStorage.clear(); });
const errors = [];
page.on('console', msg => { if (msg.type() === 'error') errors.push(msg.text()); });
page.on('pageerror', err => errors.push(err.message));
await page.goto(`file://${process.env.HOME}/test.html`, {
    waitUntil: 'networkidle0', timeout: 20000
});
await new Promise(r => setTimeout(r, 3000));
console.log('Errors:', errors.length === 0 ? 'NONE' : errors.join('\n'));
const tabs = await page.evaluate(() =>
    [...document.querySelectorAll('.lm_tab .lm_title')].map(t => t.textContent));
console.log('Tabs:', tabs.join(', ') || '(none)');
await page.screenshot({ path: `${process.env.HOME}/screenshot.png` });
await browser.close();
SCRIPT
```

Use `page.evaluate()` to click tabs, inspect DOM, and check widget state.
Read the screenshot PNG with the Read tool to visually verify rendering.

## Known issues (pre-existing, not introduced by this PR)

- **tclreadline warning on startup**: `Runfiles::Create failed: cannot find
  runfiles` followed by `tclreadlineInit.tcl not found`. The web viewer
  works fine without tclreadline — it only affects tab-completion in the
  OpenROAD CLI, which isn't used by the web viewer.

- **tclreadline error on exit**: Ctrl-C or Ctrl-D prints
  `can't read "::auto_index(::tclreadline::ScriptCompleter)"`. Harmless
  noise from tclreadline teardown when it was never fully initialized.
