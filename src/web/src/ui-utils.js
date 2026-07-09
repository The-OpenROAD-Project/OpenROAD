// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Shared UI utilities.

// True when the app was bootstrapped from a saved/static report
// (i.e. there is no live WebSocket backend).
export function isStaticMode(app) {
    return !!app?.websocketManager?.isStaticMode;
}

// True for a "#rrggbb" hex color string (the form an <input type="color">
// emits and the form persisted for the background color).
export function isValidHexColor(s) {
    return typeof s === 'string' && /^#[0-9a-fA-F]{6}$/.test(s);
}

// Wrap fn in a single-flight requestAnimationFrame scheduler: calls made
// while a frame is already pending coalesce into one invocation of fn.
export function rafCoalesce(fn) {
    let pending = null;
    return () => {
        if (pending !== null) {
            return;
        }
        pending = requestAnimationFrame(() => {
            pending = null;
            fn();
        });
    };
}

// Round a positive value to the nearest 1/2/5/10 × 10^n, returning both the
// rounded value and the chosen leading digit (used to pick tick subdivisions).
export function niceRoundParts(value) {
    const mag = Math.pow(10, Math.floor(Math.log10(value)));
    const residual = value / mag;
    let digit;
    if (residual < 1.5) {
        digit = 1;
    } else if (residual < 3.5) {
        digit = 2;
    } else if (residual < 7.5) {
        digit = 5;
    } else {
        digit = 10;
    }
    return { value: digit * mag, digit };
}

// Compute the scale-bar geometry and label from the current zoom.
//   targetPx      desired bar length in screen pixels (~15% of viewport)
//   pxPerDbu      pixels per DBU at the current zoom
//   dbuPerMicron  DBU per micron for the design (metric mode)
//   showDbu       true → label in DBU (no unit suffix), false → metric
// Returns { barPx, label, segments } or null when not drawable.
// Pure (no DOM) so it can be unit-tested.
export function computeScaleBar({ targetPx, pxPerDbu, dbuPerMicron, showDbu }) {
    if (!Number.isFinite(pxPerDbu) || pxPerDbu <= 0
        || !Number.isFinite(targetPx) || targetPx <= 0) {
        return null;
    }

    let barPx;
    let label;
    let digit;
    if (showDbu) {
        const nice = niceRoundParts(Math.max(1, targetPx / pxPerDbu));
        digit = nice.digit;
        barPx = Math.round(nice.value * pxPerDbu);
        label = String(Math.round(nice.value));
    } else {
        const dbuPerUm = dbuPerMicron || 1000;
        const pxPerUm = pxPerDbu * dbuPerUm;
        const nice = niceRoundParts(targetPx / pxPerUm);
        digit = nice.digit;
        const niceUm = nice.value;
        barPx = Math.round(niceUm * pxPerUm);
        // Pick the unit whose value comes out as a clean integer.
        let scale;
        let unit;
        if (niceUm >= 1000) {
            scale = 1 / 1000;
            unit = 'mm';
        } else if (niceUm >= 1) {
            scale = 1;
            unit = 'µm';
        } else if (niceUm >= 0.001) {
            scale = 1000;
            unit = 'nm';
        } else {
            scale = 1e6;
            unit = 'pm';
        }
        label = Math.round(niceUm * scale) + ' ' + unit;
    }

    // Interior subdivisions: 1 → 5 ticks, otherwise the leading digit
    // (2 → 2, 5 → 5, 10 → 10), mirroring the Qt scale bar's peg count.
    const segments = digit === 1 ? 5 : digit;
    return { barPx, label, segments };
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
