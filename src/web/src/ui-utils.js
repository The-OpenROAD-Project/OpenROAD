// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Shared UI utilities.

// True when the app was bootstrapped from a saved/static report
// (i.e. there is no live WebSocket backend).
export function isStaticMode(app) {
    return !!app?.websocketManager?.isStaticMode;
}

// Serialize the layer/selectability visibility flags the way the server
// parses them: each visibility key as a boolean, each selectability key with
// an `s_` prefix.  Callers add request-specific fields (visible_layers,
// selectable_layers, visible_chiplets) on top.  Shared by the tile requests,
// click-select, and the Save export so the columns can't drift.
export function buildVisibilityFlags(visibility, selectability) {
    const vf = {};
    for (const [k, v] of Object.entries(visibility || {})) {
        vf[k] = !!v;
    }
    for (const [k, v] of Object.entries(selectability || {})) {
        vf['s_' + k] = !!v;
    }
    return vf;
}

// Sync the client-side selection type flags from a server response so the
// context menu can enable/disable items by selection type.
export function applySelectionFlags(app, resp) {
    if (resp && typeof resp.sel_has_inst === 'boolean') {
        app.selHasInst = resp.sel_has_inst;
        app.selHasNet = !!resp.sel_has_net;
    }
}

// Transient notice near the bottom of the viewport (e.g. why a property
// edit was rejected).  Repeated calls replace the current message and
// restart the timer.
let toastTimer = null;
export function showToast(message, durationMs = 4000) {
    let toast = document.getElementById('or-toast');
    if (!toast) {
        toast = document.createElement('div');
        toast.id = 'or-toast';
        document.body.appendChild(toast);
    }
    toast.textContent = message;
    toast.classList.add('visible');
    if (toastTimer) clearTimeout(toastTimer);
    toastTimer = setTimeout(() => toast.classList.remove('visible'), durationMs);
    return toast;
}

// Themed confirmation dialog.  Resolves true when confirmed, false when
// cancelled (button, Escape, or clicking outside the box).  Focus starts
// on Cancel — the safe choice for destructive confirmations.
export function showConfirmModal({ title, message, confirmLabel = 'OK',
                                   danger = false }) {
    return new Promise((resolve) => {
        const overlay = document.createElement('div');
        overlay.className = 'or-modal-overlay';
        const box = document.createElement('div');
        box.className = 'or-modal';

        const titleEl = document.createElement('div');
        titleEl.className = 'or-modal-title';
        titleEl.textContent = title;
        const msgEl = document.createElement('div');
        msgEl.className = 'or-modal-message';
        msgEl.textContent = message;

        const buttons = document.createElement('div');
        buttons.className = 'or-modal-buttons';
        const cancelBtn = document.createElement('button');
        cancelBtn.className = 'or-modal-btn';
        cancelBtn.textContent = 'Cancel';
        const confirmBtn = document.createElement('button');
        confirmBtn.className = 'or-modal-btn'
            + (danger ? ' or-modal-btn-danger' : '');
        confirmBtn.textContent = confirmLabel;

        const close = (result) => {
            document.removeEventListener('keydown', onKey, true);
            overlay.remove();
            resolve(result);
        };
        const onKey = (e) => {
            if (e.key === 'Escape') { e.stopPropagation(); close(false); }
        };
        cancelBtn.addEventListener('click', () => close(false));
        confirmBtn.addEventListener('click', () => close(true));
        overlay.addEventListener('click', (e) => {
            if (e.target === overlay) close(false);
        });
        document.addEventListener('keydown', onKey, true);

        buttons.appendChild(cancelBtn);
        buttons.appendChild(confirmBtn);
        box.appendChild(titleEl);
        box.appendChild(msgEl);
        box.appendChild(buttons);
        overlay.appendChild(box);
        document.body.appendChild(overlay);
        cancelBtn.focus();
    });
}

// Coordinate transforms derived from a server bounds response
// ([[yMin, xMin], [yMax, xMax]], the tile-grid georeference).  Pure so it
// can be unit-tested; returns null when the design is empty.
export function computeBoundsTransforms(designBounds, tileSize = 256) {
    if (!designBounds) return null;
    const minY = designBounds[0][0];
    const minX = designBounds[0][1];
    const maxY = designBounds[1][0];
    const maxX = designBounds[1][1];
    const width = maxX - minX;
    const height = maxY - minY;
    if (!(width > 0) || !(height > 0)) return null;
    const maxDXDY = Math.max(width, height);
    const scale = tileSize / maxDXDY;
    return {
        scale,
        maxDXDY,
        originX: minX,
        originY: minY,
        fitBounds: [
            [-maxDXDY * scale, 0],
            [(height - maxDXDY) * scale, width * scale],
        ],
    };
}

// True when two bounds responses describe the same rectangle.
export function boundsEqual(a, b) {
    return !!a && !!b
        && a[0][0] === b[0][0] && a[0][1] === b[0][1]
        && a[1][0] === b[1][0] && a[1][1] === b[1][1];
}

// --- Selection ownership ---
//
// Several panels can replace the selection: the canvas, the Inspector's links
// and prev/next, the Display Control's layer rows, the fanout chart, the
// schematic and the rulers.  The server runs its handlers on a thread pool, so
// two selections issued in quick succession can complete out of order and the
// slower — older — one would otherwise land last, leaving the Inspector
// describing an object the server no longer has selected.
//
// beginSelection() hands ownership to the caller and returns a token; the
// caller drops its own response once isCurrentSelection() goes false.  Panels
// that decorate their own selection register a reset callback so the new owner
// clears the previous one's decoration — the callback also runs for the panel
// that called beginSelection(), so paint after the call, never before.

export function beginSelection(app) {
    app.selectionToken = (app.selectionToken || 0) + 1;
    for (const reset of app.selectionResetters || []) {
        reset();
    }
    return app.selectionToken;
}

export function isCurrentSelection(app, token) {
    return app.selectionToken === token;
}

export function onSelectionReset(app, fn) {
    if (!app.selectionResetters) {
        app.selectionResetters = [];
    }
    app.selectionResetters.push(fn);
}

// Leaflet map options for the 2D layout viewer.
//
// zoomSnap:1 + zoomDelta:1 force the map to rest only at INTEGER zoom levels,
// so server tiles are displayed 1:1 at rest with no fractional CSS rescaling
// of the tile pane.  Downscaling a band-limited dense raster (bump arrays)
// re-introduces the moiré beat the server worked to remove; resting on integer
// zoom (combined with the floor _clampZoom override in the tile layers, which
// guarantees the pane is only ever upscaled) prevents that.
//
// `crs` is parameterized so this is unit-testable without the Leaflet global.
// `prefs` carries the two Options-menu preferences the map itself owns
// (see kArrowStepDefault and installWheelPanning).
export function buildMapOptions(
    crs = (typeof L !== 'undefined' ? L.CRS.Simple : undefined),
    prefs = {}) {
    return {
        crs,
        zoom: 1,
        zoomSnap: 1,
        zoomDelta: 1,
        fadeAnimation: false,
        attributionControl: false,
        // Qt Options > "Mouse wheel mapped to zoom by default".  The live
        // viewer never reads this: installWheelPanning turns Leaflet's own
        // wheel handler off and implements both directions itself, because
        // Leaflet cannot pan on the wheel.  Kept so the option carries the
        // preference for a consumer that does not install that handler, and
        // absent preference keeps the wheel zooming, which is what shipped.
        scrollWheelZoom: prefs.wheelZoom !== false,
        keyboardPanDelta: clampArrowStep(prefs.arrowStep),
    };
}

// Qt Options > "Arrow keys scroll step": how far one arrow press pans, in CSS
// px.  Qt's dialog accepts 10..1000 with a default of 20; the default here
// stays at Leaflet's 80 because that is what the web viewer has always done
// and dropping to 20 would quietly make every arrow press a quarter as far.
export const kArrowStepDefault = 80;
export const kArrowStepMin = 10;
export const kArrowStepMax = 1000;

// Coerce a stored or user-typed step to a usable number.  Anything
// unparseable falls back to the default rather than to NaN, which Leaflet
// would turn into a map that ignores the arrow keys entirely.
export function clampArrowStep(value) {
    // Number('') and Number(null) are both 0, which would clamp to the
    // minimum; an unset cookie has to read as absent, not as "10 px".
    const step = (value === '' || value === null)
        ? NaN : Math.round(Number(value));
    if (!Number.isFinite(step)) {
        return kArrowStepDefault;
    }
    return Math.min(kArrowStepMax, Math.max(kArrowStepMin, step));
}

// Build a display-controls group header row: an expand/collapse triangle and
// a stretching name cell.  Callers fill in `name.textContent` and append their
// checkbox columns after it, which pins those columns to the row's right edge
// (see the .vis-name / .vis-cb rules in style.css).
//
// The header is a plain <div>, not a <label>: a <label> wrapping the
// visibility checkbox activates that checkbox for a click anywhere in the
// row, so any click that misses a checkbox toggles the whole category's
// visibility.  Here only the checkboxes toggle state; the triangle, the
// group name and the empty space expand/collapse, matching the Qt GUI's
// tree where clicking an item's name never changes visibility.
export function makeGroupHeader(className = 'vis-group-header') {
    const header = document.createElement('div');
    header.className = className;
    const arrow = document.createElement('span');
    arrow.className = 'vis-arrow';
    header.appendChild(arrow);
    const name = document.createElement('span');
    name.className = 'vis-name';
    header.appendChild(name);
    return { header, arrow, name };
}

// Wire `header` so clicking its triangle, its name cell, or its own bare area
// (the empty space, which targets `header` itself) expands or collapses
// `children`, keeping `arrow`'s glyph in sync.  `collapsed` is the initial
// state and is applied immediately.
//
// Only those three targets collapse.  Every other element in the row keeps its
// own click, so a control added later cannot both act and collapse the group:
// the worst a new child can do is not respond, which is visible immediately,
// rather than firing two actions at once.
//
// The name cell is listed explicitly because it is an element: the group name
// used to be a bare text node whose clicks reported `header` as the target,
// and moving it into a stretching .vis-name span (for column alignment) would
// otherwise have silently dropped the largest click target in the row.
export function attachGroupCollapse(header, arrow, children, collapsed) {
    const apply = (c) => {
        children.classList.toggle('collapsed', c);
        arrow.textContent = c ? '▶' : '▼';
    };
    apply(!!collapsed);
    header.addEventListener('click', (e) => {
        const isName = e.target && e.target.parentNode === header
            && e.target.classList
            && e.target.classList.contains('vis-name');
        if (e.target !== header && e.target !== arrow && !isName) return;
        apply(!children.classList.contains('collapsed'));
    });
}

// Deepest zoom worth offering, given `designScale` (pixels per DBU at zoom 0).
//
// Leaflet's default maxZoom is Infinity unless a layer supplies one, and
// L.GridLayer -- unlike L.TileLayer -- supplies none.  Left unbounded the zoom
// keeps climbing past any useful magnification until the arithmetic gives out:
// the server's tile span (maxDXDY / 2^z) underflows to zero, so tiles come back
// empty and the design appears to vanish, and the map's own 2^z factors reach
// Infinity, which JSON.stringify writes as `null` in the tile request.
//
// The cap is where one DBU already covers `maxPxPerDbu` screen pixels: a DBU is
// the smallest distance the database can express, so magnifying it further
// shows nothing that was not already visible.  Returned as an integer because
// the map rests on integer zoom levels (see buildMapOptions).
// Must match kMaxTileZoom in request_handler.h: the server refuses a deeper
// tile, so asking for one would only trade blank tiles for error responses.
export const MAX_TILE_ZOOM = 30;

export function maxUsefulZoom(designScale, maxPxPerDbu = 8) {
    if (!Number.isFinite(designScale) || designScale <= 0) {
        // Callers hold off until the design bounds arrive, so this is a
        // belt-and-braces value: still finite, so the zoom stays bounded.
        return MAX_TILE_ZOOM;
    }
    const z = Math.ceil(Math.log2(maxPxPerDbu / designScale));
    // Never above the server's ceiling, and never below 1: a design so small
    // that one DBU already fills the budget at zoom 0 would otherwise pin the
    // user at the fit zoom with no way to zoom in at all.
    return Math.max(1, Math.min(MAX_TILE_ZOOM, z));
}

// --- Display units ---
//
// Every length the UI shows or accepts is stored in DBU and displayed in
// whichever unit the "Show DBU" setting (Qt's MainWindow::useDBU) selects.
// These are pure so the app object, the rulers, the Go-to dialog and their
// tests all share one implementation instead of each keeping a copy; `opts`
// is `{ showDbu, dbuPerMicron }`.

// DBU → display string.  Mirrors Qt's MainWindow::convertDBUToString.
export function formatDbu(value, { showDbu, dbuPerMicron }, addUnits = false) {
    if (showDbu) return String(Math.round(value));
    const dbuPerUm = dbuPerMicron > 0 ? dbuPerMicron : 1000;
    // Enough decimals that two adjacent DBU cannot print the same, which is
    // ceil() and not round(): at 2000 DBU/µm round() would give 3, and 1 and
    // 2 DBU would both come out as "0.001".
    const precision = Math.ceil(Math.log10(dbuPerUm));
    const um = (value / dbuPerUm).toFixed(precision);
    return addUnits ? um + ' µm' : um;
}

// Display string → DBU, the inverse of formatDbu, or null when the text is
// not a number.  Mirrors Qt's MainWindow::convertStringToDBU.
export function parseDbu(str, { showDbu, dbuPerMicron }) {
    const num = parseFloat(str);
    if (!Number.isFinite(num)) return null;
    if (showDbu) return Math.round(num);
    const dbuPerUm = dbuPerMicron > 0 ? dbuPerMicron : 1000;
    return Math.round(num * dbuPerUm);
}

// A distance (always positive) with an auto-scaled unit, for the ruler
// labels.  Unlike formatDbu this always names its unit, because the value
// appears on the canvas with no column header to carry it.
export function formatDistance(dbuLength, { showDbu, dbuPerMicron }) {
    if (showDbu) return String(Math.round(dbuLength));
    const dbuPerUm = dbuPerMicron > 0 ? dbuPerMicron : 1000;
    const um = dbuLength / dbuPerUm;
    if (um >= 1000) return (um / 1000).toFixed(3) + ' mm';
    if (um >= 1) return um.toFixed(3) + ' um';
    return (um * 1000).toFixed(1) + ' nm';
}

// Unit suffix for a field label, e.g. "X (µm)" / "X (DBU)".
export function unitLabel({ showDbu }) {
    return showDbu ? 'DBU' : 'µm';
}

// True for a "#rrggbb" hex color string (the form an <input type="color">
// emits and the form persisted for the background color).
export function isValidHexColor(s) {
    return typeof s === 'string' && /^#[0-9a-fA-F]{6}$/.test(s);
}

// Normalize a CSS color to the "#rrggbb" form an <input type="color">
// requires, or null when unrecognized.  Handles the forms a CSS custom
// property can hold: "#rgb", "#rrggbb", and "rgb(r, g, b)".
export function cssColorToHex(s) {
    if (typeof s !== 'string') return null;
    const str = s.trim();
    if (isValidHexColor(str)) return str.toLowerCase();
    const short = str.match(/^#([0-9a-fA-F]{3})$/);
    if (short) {
        return '#' + short[1].split('').map(c => c + c).join('').toLowerCase();
    }
    const rgb = str.match(/^rgb\(\s*(\d{1,3})\s*,\s*(\d{1,3})\s*,\s*(\d{1,3})\s*\)$/);
    if (rgb) {
        const parts = rgb.slice(1).map(Number);
        if (parts.some(v => v > 255)) return null;
        return '#' + parts.map(v => v.toString(16).padStart(2, '0')).join('');
    }
    return null;
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
        // A corrupt (non-finite/non-positive) DBU-per-micron would send
        // niceRoundParts a negative value and yield NaN geometry; fall
        // back to the same default used for a missing value.
        const dbuPerUm = (Number.isFinite(dbuPerMicron) && dbuPerMicron > 0)
            ? dbuPerMicron : 1000;
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

// Qt's LayoutScroll::wheelEvent, ported.  The preference and the Ctrl modifier
// pick between panning and zooming, so Ctrl always does whichever one the
// preference did not: with the preference off (Qt's default) the wheel pans and
// Ctrl+wheel zooms, and with it on they swap.
//
// Both branches live here and Leaflet's own scrollWheelZoom is left disabled:
// Leaflet has no pan-on-wheel option, and leaving its handler on would zoom in
// addition to whatever this one did.  Zooming goes through setZoomAround so the
// point under the cursor stays put, matching Qt's zoomIn(mouse_pos, true) and
// the viewer's own Z / Shift+Z keys.
export function installWheelPanning(map, wheelZoomEnabled) {
    map.scrollWheelZoom.disable();
    map.getContainer().addEventListener('wheel', (e) => {
        e.preventDefault();
        if (wheelZoomEnabled() === e.ctrlKey) {
            const [dx, dy] = wheelDeltaPx(e, map.getContainer());
            map.panBy([dx, dy], { animate: false });
            return;
        }
        // A wheel notch is one zoom level, like the Z / Shift+Z keys.
        const step = e.deltaY < 0 ? 1 : -1;
        map.setZoomAround(map.mouseEventToLatLng(e), map.getZoom() + step);
    }, { passive: false });
}

// A wheel event's scroll amount in CSS pixels.  deltaMode is not always pixels:
// Firefox reports lines, and a page-mode event (rare, but produced by some
// remote-desktop stacks) means a viewport at a time.  Treating either as pixels
// would make one notch pan by a couple of pixels.
function wheelDeltaPx(e, container) {
    const kLineHeightPx = 16;
    let scale = 1;
    if (e.deltaMode === 1) {
        scale = kLineHeightPx;
    } else if (e.deltaMode === 2) {
        scale = container.clientHeight || kLineHeightPx;
    }
    return [e.deltaX * scale, e.deltaY * scale];
}

// A modal dialog shell: title, caller-supplied body, an error line and
// Cancel/OK.  Closes on the X, Cancel, Escape and a click on the backdrop.
// `dialogClass` selects the per-dialog CSS (width, row layout).
export function buildModal(title, bodyHtml, okLabel, dialogClass) {
    const overlay = document.createElement('div');
    overlay.className = 'modal-overlay';
    overlay.innerHTML = `
        <div class="modal-dialog ${dialogClass}">
            <button class="modal-close" title="Close" aria-label="Close">&times;</button>
            <h3>${title}</h3>
            ${bodyHtml}
            <div class="modal-error" style="display:none"></div>
            <div class="modal-buttons">
                <button class="cancel">Cancel</button>
                <button class="primary ok">${okLabel}</button>
            </div>
        </div>`;
    document.body.appendChild(overlay);

    const close = () => overlay.remove();
    overlay.addEventListener('click', (e) => { if (e.target === overlay) close(); });
    overlay.addEventListener('keydown', (e) => { if (e.key === 'Escape') close(); });
    overlay.querySelector('.cancel').addEventListener('click', close);
    overlay.querySelector('.modal-close').addEventListener('click', close);

    const errorDiv = overlay.querySelector('.modal-error');
    const showError = (msg, isInfo) => {
        errorDiv.textContent = msg;
        errorDiv.style.display = msg ? 'block' : 'none';
        errorDiv.classList.toggle('info', !!isInfo);
    };
    return { overlay, close, okBtn: overlay.querySelector('.ok'), showError };
}
