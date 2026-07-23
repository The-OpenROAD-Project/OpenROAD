// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Shared UI utilities.

// True when the app was bootstrapped from a saved/static report
// (i.e. there is no live WebSocket backend).
export function isStaticMode(app) {
    return !!app?.websocketManager?.isStaticMode;
}

// Run a Tcl script from a custom menu item / toolbar button: prefer the
// console-aware app.runTclScript (echo + result in the console), falling back
// to a bare tcl_eval request when it isn't wired (e.g. in unit tests).
export function runTclScript(app, script, echo) {
    if (!script) return Promise.resolve();
    if (typeof app.runTclScript === 'function') {
        return app.runTclScript(script, echo);
    }
    if (echo && typeof app.echoTcl === 'function') app.echoTcl(script);
    return app.websocketManager.request({ type: 'tcl_eval', cmd: script })
        .catch(() => {});
}

// Make table column headers resizable by dragging.
// widths is an optional array of CSS width strings (e.g. saved from a
// previous render); when given, it is applied directly instead of
// measuring natural widths, avoiding a forced reflow.
export function makeResizableHeaders(table, widths) {
    const headers = table.querySelectorAll('thead th');
    if (!widths || !widths[0]) {
        // Reset to auto layout so browser computes natural column widths;
        // reading offsetWidth forces a reflow.
        table.style.tableLayout = 'auto';
        headers.forEach((th) => th.style.width = '');
        widths = Array.from(headers, (th) => th.offsetWidth + 'px');
    }
    // Lock in widths and switch to fixed layout
    headers.forEach((th, i) => th.style.width = widths[i] || '');
    table.style.tableLayout = 'fixed';

    headers.forEach((th, idx) => {
        if (idx === headers.length - 1) return; // skip last column
        const grip = document.createElement('div');
        grip.className = 'col-resize-grip';
        th.style.position = 'relative';
        th.appendChild(grip);

        let startX, startW;
        const onMouseMove = (e) => {
            th.style.width = Math.max(30, startW + e.clientX - startX) + 'px';
        };
        const onMouseUp = () => {
            document.removeEventListener('mousemove', onMouseMove);
            document.removeEventListener('mouseup', onMouseUp);
            // Releasing the drag fires a click on the common ancestor of the
            // press/release targets (often the th); swallow it so header
            // click handlers (e.g. sorting) don't trigger. The timeout
            // clears the suppressor when no click fires (e.g. release
            // outside the window).
            const swallowClick = (e) => e.stopPropagation();
            document.addEventListener('click', swallowClick,
                                      { capture: true, once: true });
            setTimeout(() => {
                document.removeEventListener('click', swallowClick,
                                             { capture: true });
            }, 0);
        };
        grip.addEventListener('mousedown', (e) => {
            e.preventDefault();
            startX = e.clientX;
            startW = th.offsetWidth;
            document.addEventListener('mousemove', onMouseMove);
            document.addEventListener('mouseup', onMouseUp);
        });
    });
}
