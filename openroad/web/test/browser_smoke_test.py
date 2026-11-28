# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors
#
# The layout view drawn by a real browser: the live viewer and a saved report
# opened from disk with the network off each show a tile.  The unit tests run
# the view against a stand-in for deck.gl, so only this catches a bundle that
# loads but draws nothing.
#
# Needs Chrome with WebGL 2 (SwiftShader is enough), named by CHROME or found
# on PATH.  Tagged manual: CI runs it from
# .github/workflows/github-actions-web-browser-smoke.yml.
#
# usage: browser_smoke_test.py OPENROAD LIBERTY LEF DEF

import fcntl
import json
import os
import pathlib
import select
import shutil
import socket
import subprocess
import sys
import tempfile
import time
import unittest

_OPENROAD, _LIBERTY, _LEF, _DEF = (None,) * 4
# Headless: nothing of the desktop session reaches Chrome.
_DESKTOP_VARS = ("DISPLAY", "WAYLAND_DISPLAY", "DBUS_SESSION_BUS_ADDRESS")

# Pixels of the layout that differ from the view's background, once its tiles
# have settled, and the tiles each slot holds.
_DRAWN = """(async () => {
    const v = window.__orView;
    if (!v) {
        return null;
    }
    await v.whenTilesSettled();
    const shot = await v.captureCanvas();
    if (!shot) {
        return null;
    }
    const c = document.createElement('canvas');
    c.width = shot.image.width;
    c.height = shot.image.height;
    const g = c.getContext('2d');
    g.drawImage(shot.image, 0, 0);
    const d = g.getImageData(0, 0, c.width, c.height).data;
    const bg = getComputedStyle(v.getContainer()).backgroundColor
        .match(/[\\d.]+/g).map(Number);
    let drawn = 0;
    for (let i = 0; i < d.length; i += 4) {
        const diff = Math.abs(d[i] - bg[0]) + Math.abs(d[i + 1] - bg[1])
            + Math.abs(d[i + 2] - bg[2]);
        if (d[i + 3] > 0 && diff > 24) {
            drawn++;
        }
    }
    return { drawn, tiles: v.debugSnapshot().slots.map((s) => s.drawable) };
})()"""


def _chrome():
    path = os.environ.get("CHROME") or next(
        filter(None, map(shutil.which, ["google-chrome", "chromium", "chrome"])),
        None,
    )
    if not path:
        raise RuntimeError("no Chrome: set CHROME")
    return path


class _Browser:
    """Chrome driven over the DevTools protocol on a pipe (fds 3 and 4)."""

    def __init__(self, profile):
        to_chrome_r, self._to_chrome = os.pipe()
        self._from_chrome, from_chrome_w = os.pipe()

        def pipes():
            # Above 4 first: dup2 onto its own number keeps O_CLOEXEC, and one
            # end may already sit on 3 or 4.
            ends = [
                fcntl.fcntl(fd, fcntl.F_DUPFD, 5) for fd in (to_chrome_r, from_chrome_w)
            ]
            for target, fd in zip((3, 4), ends):
                os.dup2(fd, target)
                os.close(fd)

        flags = [
            "--headless=new",
            "--remote-debugging-pipe",
            f"--user-data-dir={profile}",
            "--no-first-run",
            "--no-default-browser-check",
            "--disable-background-networking",
            "--disable-component-update",
            "--disable-extensions",
            # WebGL 2 without a GPU.
            "--use-angle=swiftshader",
            "--enable-unsafe-swiftshader",
            "--ignore-gpu-blocklist",
            "--window-size=1280,800",
        ]
        if os.geteuid() == 0:
            flags.append("--no-sandbox")
        env = {k: v for k, v in os.environ.items() if k not in _DESKTOP_VARS}
        env["HOME"] = profile
        self._proc = subprocess.Popen(
            [_chrome(), *flags, "about:blank"],
            preexec_fn=pipes,
            close_fds=False,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
            env=env,
        )
        os.close(to_chrome_r)
        os.close(from_chrome_w)
        self._buf = b""
        self._id = 0
        self.events = []
        target = self.send("Target.createTarget", url="about:blank")["targetId"]
        self.session = self.send(
            "Target.attachToTarget", targetId=target, flatten=True
        )["sessionId"]

    def close(self):
        # A clean exit takes the child processes and Chrome's own temp files
        # with it; a killed Chrome leaves both behind.
        try:
            self.send("Browser.close", timeout=10)
        except (OSError, RuntimeError, TimeoutError):
            pass
        try:
            self._proc.wait(timeout=10)
        except subprocess.TimeoutExpired:
            self._proc.kill()
            self._proc.wait()
        os.close(self._to_chrome)
        os.close(self._from_chrome)

    def _read(self, timeout):
        while b"\0" not in self._buf:
            ready, _, _ = select.select([self._from_chrome], [], [], max(0, timeout))
            if not ready:
                raise TimeoutError("no reply from Chrome")
            chunk = os.read(self._from_chrome, 1 << 16)
            if not chunk:
                raise RuntimeError("Chrome exited")
            self._buf += chunk
        msg, self._buf = self._buf.split(b"\0", 1)
        return json.loads(msg)

    def send(self, method, session=None, timeout=60, **params):
        self._id += 1
        msg = {"id": self._id, "method": method, "params": params}
        if session:
            msg["sessionId"] = session
        os.write(self._to_chrome, json.dumps(msg).encode() + b"\0")
        deadline = time.monotonic() + timeout
        while True:
            reply = self._read(deadline - time.monotonic())
            if reply.get("id") == self._id:
                if "error" in reply:
                    raise RuntimeError(f"{method}: {reply['error']}")
                return reply.get("result", {})
            self.events.append(reply)

    def page(self, method, **params):
        return self.send(method, session=self.session, **params)

    def evaluate(self, expression, timeout=60):
        result = self.send(
            "Runtime.evaluate",
            session=self.session,
            timeout=timeout,
            expression=expression,
            awaitPromise=True,
            returnByValue=True,
        )
        if "exceptionDetails" in result:
            raise RuntimeError(result["exceptionDetails"])
        return result["result"].get("value")

    def event_params(self, method):
        return [e["params"] for e in self.events if e.get("method") == method]


def _load_design(tcl, then):
    tcl.write_text(
        f"read_liberty {_LIBERTY}\nread_lef {_LEF}\nread_def {_DEF}\n{then}\n"
    )


def _free_port():
    with socket.socket() as s:
        s.bind(("127.0.0.1", 0))
        return s.getsockname()[1]


class LayoutViewDraws(unittest.TestCase):
    def setUp(self):
        self.tmp = pathlib.Path(tempfile.mkdtemp(dir=os.environ.get("TEST_TMPDIR")))
        self.addCleanup(shutil.rmtree, self.tmp, ignore_errors=True)
        self.browser = _Browser(str(self.tmp / "profile"))
        self.addCleanup(self.browser.close)
        self.browser.page("Runtime.enable")
        self.browser.page("Network.enable")
        has_webgl2 = self.browser.evaluate(
            "!!document.createElement('canvas').getContext('webgl2')"
        )
        self.assertTrue(has_webgl2, "this Chrome has no WebGL 2")

    def assertDrawsATile(self, url):
        self.browser.page("Page.navigate", url=url)
        drawn = None
        deadline = time.monotonic() + 90
        while time.monotonic() < deadline:
            drawn = self.browser.evaluate(_DRAWN)
            if drawn and drawn["drawn"] > 0 and any(drawn["tiles"]):
                break
            time.sleep(1)
        self.assertTrue(drawn and drawn["drawn"] > 0, f"nothing drawn: {drawn}")
        self.assertTrue(any(drawn["tiles"]), f"no tile: {drawn}")
        errors = self.browser.event_params("Runtime.exceptionThrown")
        self.assertEqual(errors, [])

    def test_live_viewer(self):
        port = _free_port()
        token = os.urandom(16).hex()
        tcl = self.tmp / "serve.tcl"
        _load_design(tcl, f"web_server -port {port} -token {token} -no_browser")
        log = self.tmp / "server.log"
        with open(log, "w") as out:
            server = subprocess.Popen(
                [_OPENROAD, "-no_init", str(tcl)],
                stdin=subprocess.PIPE,
                stdout=out,
                stderr=subprocess.STDOUT,
            )
        self.addCleanup(server.stdin.close)
        self.addCleanup(server.wait)
        self.addCleanup(server.kill)
        deadline = time.monotonic() + 120
        while "WEB-0001" not in log.read_text():
            self.assertIsNone(server.poll(), log.read_text())
            self.assertLess(time.monotonic(), deadline, log.read_text())
            time.sleep(0.2)
        self.assertDrawsATile(f"http://127.0.0.1:{port}/?token={token}")

    def test_saved_report_offline(self):
        report = self.tmp / "report.html"
        tcl = self.tmp / "report.tcl"
        _load_design(tcl, f"web_save_report {report}")
        subprocess.run(
            [_OPENROAD, "-no_init", "-exit", str(tcl)], check=True, timeout=300
        )
        self.browser.page(
            "Network.emulateNetworkConditions",
            offline=True,
            latency=0,
            downloadThroughput=-1,
            uploadThroughput=-1,
        )
        self.assertDrawsATile(report.as_uri())
        remote = [
            r["request"]["url"]
            for r in self.browser.event_params("Network.requestWillBeSent")
            if not r["request"]["url"].startswith(("file:", "data:", "blob:"))
        ]
        self.assertEqual(remote, [])


if __name__ == "__main__":
    _OPENROAD, _LIBERTY, _LEF, _DEF = (os.path.abspath(a) for a in sys.argv[1:5])
    unittest.main(argv=sys.argv[:1])
