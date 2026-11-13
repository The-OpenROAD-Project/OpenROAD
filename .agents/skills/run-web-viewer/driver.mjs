// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Drive a running OpenROAD web viewer in headless Chrome.  Each argument
// is one command, run in order in one browser session:
//
//   node .agents/skills/run-web-viewer/driver.mjs <out_dir> 'tab Timing' \
//       'click .timing-widget button:text-is("Update")' 'wait 2000' \
//       'widths .timing-path-table-container table' 'shot timing'
//
// Commands:
//   tab <Title>            click a panel tab (Timing, Hierarchy, Inspector,
//                          Select Highlight, DRC, Charts, Clock Tree, ...)
//   click <selector>       click an element (Playwright selector)
//   clickxy <x> <y>        click page coordinates (the layout selects there)
//   shiftclickxy <x> <y>   add to the selection
//   drag <selector> <dx>   press on the element, move dx px, release
//   hover <selector>       move the mouse over an element
//   fill <selector> | <text>
//                          type into an input
//   key <key>              press a key (Enter, Escape, ...)
//   tcl <command>          run a Tcl command in the console, print its output
//   wait <ms>              sleep
//   widths <table-sel>     print each header cell's rendered width
//   eval <js>              evaluate a JS expression in the page, print JSON
//   shot <name>            screenshot to <out_dir>/<name>.png
//
// Exit status 1 if a command fails (the rest are skipped), the token is
// rejected, or the page reports an error, a failed request, or no layers.
// The token comes from <out_dir>/token, which launch.sh writes.
// Env: WEB_PORT (8765), PW_DIR (~/.cache/openroad-web-driver, where
// playwright-core is installed), CHROME (/usr/bin/google-chrome),
// VIEWPORT (1600x1000).

import { createRequire } from 'module';
import fs from 'fs';
import os from 'os';
import path from 'path';

const pwDir = process.env.PW_DIR
    || path.join(os.homedir(), '.cache', 'openroad-web-driver');
const require = createRequire(path.join(pwDir, 'package.json'));
const { chromium } = require('playwright-core');

const [out, ...commands] = process.argv.slice(2);
if (!out) {
  console.error('usage: driver.mjs <out_dir> <command>...');
  process.exit(2);
}
const port = process.env.WEB_PORT || 8765;
const token = fs.readFileSync(path.join(out, 'token'), 'utf8').trim();
const [vw, vh] = (process.env.VIEWPORT || '1600x1000').split('x').map(Number);

const browser = await chromium.launch({
  executablePath: process.env.CHROME || '/usr/bin/google-chrome',
  args: ['--no-sandbox'],
});
const page = await browser.newPage({ viewport: { width: vw, height: vh } });
// Every page problem is printed and fails the run (exit 1) once all
// commands have run, so a broken page cannot pass as a clean run.
let pageErrors = 0;
const pageError = (...msg) => {
  ++pageErrors;
  console.log(...msg);
};
page.on('pageerror', (e) => pageError('PAGEERROR', e.message));
// A failed request shows in the console without its URL; report failed
// responses by URL instead, so the favicon's 401 (it carries no token) can
// be told apart from a rejected token.
page.on('console', (m) => {
  if (m.type() === 'error' && !m.text().startsWith('Failed to load resource')) {
    pageError('CONSOLE', m.text());
  }
});
page.on('response', (r) => {
  if (r.status() >= 400 && !new URL(r.url()).pathname.endsWith('favicon.ico')) {
    pageError('HTTP', r.status(), r.url().replace(/token=[0-9a-f]+/, 'token=…'));
  }
});
const response = await page.goto(`http://localhost:${port}/?token=${token}`);
if (response.status() === 401) {
  console.log(`token rejected: is ${path.join(out, 'token')} from this server?`);
  await browser.close();
  process.exit(1);
}
// Ready once the display controls list the server's layers (rows exist even
// when the first is hidden).  A design with no tech layers never gets
// there: save a screenshot of the broken page and stop, since every
// command would only time out.
// Poll from here: the page's CSP forbids eval, which waitForFunction's
// in-page polling needs.
const layersListed = async () => {
  for (const deadline = Date.now() + 30000; Date.now() < deadline;) {
    if (await page.evaluate(
        () => document.querySelectorAll('.vis-leaf').length > 0)) {
      return true;
    }
    await page.waitForTimeout(250);
  }
  return false;
};
if (!await layersListed()) {
  const file = path.join(out, 'load-failed.png');
  await page.screenshot({ path: file });
  console.log('ERROR: no layers listed after 30 s; the page did not load;',
              'see', file);
  await browser.close();
  process.exit(1);
}
await page.waitForTimeout(500);

const split = (s) => {
  const i = s.indexOf(' ');
  return i < 0 ? [s, ''] : [s.slice(0, i), s.slice(i + 1)];
};

let status = 0;
for (const command of commands) {
  const [op, arg] = split(command.trim());
  try {
    switch (op) {
      case 'tab':
        // the tab strip is an <ide-tab> shadow root; its title is a span
        await page.locator(`ide-tab[title="${arg}"] .ide-tab-title`)
            .click({ force: true });
        break;
      case 'click':
        await page.locator(arg).first().click();
        break;
      case 'clickxy':
      case 'shiftclickxy': {
        const [x, y] = arg.split(/\s+/).map(Number);
        if (!Number.isFinite(x) || !Number.isFinite(y)) {
          throw new Error(`expected <x> <y>, got: ${arg}`);
        }
        if (op === 'shiftclickxy') await page.keyboard.down('Shift');
        await page.mouse.click(x, y);
        if (op === 'shiftclickxy') await page.keyboard.up('Shift');
        break;
      }
      case 'drag': {
        const at = arg.lastIndexOf(' ');
        const sel = arg.slice(0, at);
        const dx = Number(arg.slice(at + 1));
        if (at < 0 || !Number.isFinite(dx)) {
          throw new Error(`expected <selector> <dx>, got: ${arg}`);
        }
        const el = page.locator(sel).first();
        await el.scrollIntoViewIfNeeded();
        const box = await el.boundingBox();
        if (!box) throw new Error(`${sel} has no bounding box (not rendered)`);
        const x = box.x + box.width / 2;
        const y = box.y + box.height / 2;
        await page.mouse.move(x, y);
        await page.mouse.down();
        await page.mouse.move(x + dx, y, { steps: 5 });
        await page.mouse.up();
        break;
      }
      case 'hover':
        await page.locator(arg).first().hover();
        break;
      case 'fill': {
        // ' | ' separates them: both the selector and the text may hold spaces
        const at = arg.indexOf(' | ');
        if (at < 0) throw new Error(`expected <selector> | <text>, got: ${arg}`);
        await page.locator(arg.slice(0, at)).first().fill(arg.slice(at + 3));
        break;
      }
      case 'key':
        await page.keyboard.press(arg);
        break;
      case 'tcl': {
        // print the console text the command added
        const output = page.locator('.tcl-output');
        const before = (await output.innerText()).length;
        const input = page.locator('input.tcl-input');
        await input.fill(arg);
        await input.press('Enter');
        await page.waitForTimeout(1000);
        console.log((await output.innerText()).slice(before).trim());
        break;
      }
      case 'wait':
        await page.waitForTimeout(Number(arg));
        break;
      case 'widths':
        console.log('widths', arg, '=>', await page.evaluate((sel) => {
          const t = document.querySelector(sel);
          if (!t) return 'no such table';
          return Array.from(t.querySelectorAll('thead th'), (th) =>
            `${th.textContent.trim() || '(blank)'}=`
            + Math.round(th.getBoundingClientRect().width)).join(' ');
        }, arg));
        break;
      case 'eval':
        console.log('eval =>', JSON.stringify(
            await page.evaluate(arg)));
        break;
      case 'shot': {
        const file = path.join(out, `${arg}.png`);
        await page.screenshot({ path: file });
        console.log('shot', file);
        break;
      }
      default:
        throw new Error(`unknown command: ${op}`);
    }
  } catch (e) {
    console.log(`FAILED ${command}: ${e.message.split('\n')[0]}`);
    status = 1;
    break;
  }
}
await browser.close();
if (pageErrors) {
  console.log(`${pageErrors} page error(s): failing the run`);
  status = 1;
}
process.exit(status);
