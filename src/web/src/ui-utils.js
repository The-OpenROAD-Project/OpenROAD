// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Shared UI utilities.

import { dbuRectToBounds } from './coordinates.js';
import { TILE_SIZE_CSS, TILE_SIZE_QUANTUM } from './tile-request.js';

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
        cancelBtn.className = 'or-btn';
        cancelBtn.textContent = 'Cancel';
        const confirmBtn = document.createElement('button');
        confirmBtn.className = 'or-btn' + (danger ? ' or-btn-danger' : '');
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

// Fraction of each dimension left empty on every side by zoom-to-fit.  Qt's
// LayoutViewer::zoomTo divides the fitted pixels-per-DBU by (1 + 2*margin) with
// defaultZoomMargin = 0.05, so the design fills 1/1.1 of the viewport.
export const kZoomMargin = 0.05;

// Grow a DBU rect by `fraction` of each of its own dimensions on every side.
// Per-dimension, not by the larger one: that is what dividing the fitted
// pixels-per-DBU by (1 + 2*fraction) amounts to, since Qt fits on
// min(W/dx, H/dy).
function bloatRect([[yMin, xMin], [yMax, xMax]], fraction) {
    const padX = (xMax - xMin) * fraction;
    const padY = (yMax - yMin) * fraction;
    return [[yMin - padY, xMin - padX], [yMax + padY, xMax + padX]];
}

// Coordinate transforms derived from a server bounds response
// ([[yMin, xMin], [yMax, xMax]], the tile-grid georeference).  Pure so it
// can be unit-tested; returns null when the design is empty.
//
// `fitRect`, in the same wire order, is what the map zooms to fit: the design
// without the pin-label margin the georeference rect carries.  Framing on the
// georeference rect instead leaves that margin as dead space around the design
// (issue #11338).  Omitted, the georeference rect is fitted, as before.
export function computeBoundsTransforms(designBounds, tileSize = 256,
                                        fitRect = null) {
    if (!isUsableRect(designBounds)) return null;
    const minY = designBounds[0][0];
    const minX = designBounds[0][1];
    const maxDXDY = Math.max(designBounds[1][1] - minX,
                             designBounds[1][0] - minY);
    const scale = tileSize / maxDXDY;
    const fit = isUsableRect(fitRect) ? fitRect : designBounds;
    const [[fitYMin, fitXMin], [fitYMax, fitXMax]] = bloatRect(fit, kZoomMargin);
    return {
        scale,
        maxDXDY,
        originX: minX,
        originY: minY,
        fitBounds: dbuRectToBounds(fitXMin, fitYMin, fitXMax, fitYMax,
                                   scale, maxDXDY, minX, minY),
    };
}

// A bounds-response rect with a positive extent in both axes.
function isUsableRect(r) {
    return !!r && r[1][0] > r[0][0] && r[1][1] > r[0][1];
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

// Push a new arrow-key step into an already-open map.
//
// Leaflet reads options.keyboardPanDelta once, in Keyboard.initialize, so
// assigning that option on a live map does nothing -- the distance lives in
// the handler's own key map, which _setPanDelta rebuilds from this argument
// (Leaflet 1.9.4, Map.Keyboard.js).  Hence the push rather than an option
// write.  _setPanDelta is private, so every hop is optional-chained: a static
// report has no keyboard handler, and setArrowStep can run before the map
// exists.  buildMapOptions carries the value for the next map either way.
export function applyArrowStep(map, step) {
    map?.keyboard?._setPanDelta?.(step);
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

// The CSS tile size that makes zoom-to-fit land on an integer zoom level.
//
// The map rests only on integer zoom (buildMapOptions), and Leaflet's
// fitBounds FLOORS the zoom it computes, so the design is displayed anywhere
// between half and all of what would fit — 52% of the viewport in the case
// reported as issue #11338, against 91% in the Qt GUI.
//
// The lever is the tile size, because it is the one free parameter left. At
// zoom z the design spans `Dx * (T/M) * 2^z` CSS px, so the zoom the fit wants
// is
//
//     z*(T) = log2(min(W, H) / the framed box at tile size T)
//
// and T scales z* without touching anything else: the tile grid is indexed by
// zoom alone, so tile (z,x,y) still covers the same DBU (see
// useFittedTileSize). Taking T' = TILE_SIZE_CSS * 2^frac(z*) — which lands in
// [TILE_SIZE_CSS, 2*TILE_SIZE_CSS) — makes z* whole and the floor stops
// costing a level.
//
// T' is rounded DOWN to a multiple of TILE_SIZE_QUANTUM: down because a T'
// above the ideal drops z* just below the integer and the floor takes a whole
// level back, and to the quantum because a tile size has to stay a whole
// number of device pixels at every ratio a display can report, not just at
// today's one. That leaves two sizes to pick from, so the design fills at
// least 2/3 of the framed box rather than the 1/2 the bare floor allows.
//
// Only the zoom-to-fit rect gets this: every other fitBounds caller (zoom to
// selection, the DRC widget, rubber-band zoom) still loses up to a level to
// the same floor, and a size picked for this rect cannot help theirs.
//
// The choice depends on the viewport, so it is made once, at boot: a later
// window resize detunes it until the next reload, which costs at worst the
// level the fit loses today.
export function fittedTileSizeCss({ designBounds, fitRect, viewW, viewH }) {
    // The framed box, margin and fit-rect fallback included, in the map units
    // TILE_SIZE_CSS defines -- so this cannot drift from what fitBounds sees.
    const t = computeBoundsTransforms(designBounds, TILE_SIZE_CSS, fitRect);
    const finitePositive = (v) => Number.isFinite(v) && v > 0;
    if (!t || !finitePositive(viewW) || !finitePositive(viewH)) {
        return TILE_SIZE_CSS;
    }
    const framedW = t.fitBounds[1][1] - t.fitBounds[0][1];
    const framedH = t.fitBounds[1][0] - t.fitBounds[0][0];
    // Pixels the framed box may grow by before it leaves the viewport, which
    // is 2^(the zoom fitBounds wants) at tile size TILE_SIZE_CSS.
    const room = Math.min(viewW / framedW, viewH / framedH);
    const zStar = Math.log2(room);
    // Below zoom 0 the map is already pinned at its minZoom and the fit has
    // nothing left to floor, so a bigger tile would only overflow the viewport.
    if (!Number.isFinite(zStar) || zStar < 0) {
        return TILE_SIZE_CSS;
    }
    const target = Math.floor(zStar);
    const ideal = TILE_SIZE_CSS * 2 ** (zStar - target);
    // ideal < 2 * TILE_SIZE_CSS and the quantum is half of it, so there is at
    // most one size above the base to consider.  Re-derive the zoom rather
    // than trust it: at exactly the ideal size the fit zoom IS the integer,
    // and one ULP below it the floor takes the whole level straight back.
    const bigger = Math.floor(ideal / TILE_SIZE_QUANTUM) * TILE_SIZE_QUANTUM;
    if (bigger > TILE_SIZE_CSS
        && Math.log2(TILE_SIZE_CSS * room / bigger) >= target) {
        return bigger;
    }
    return TILE_SIZE_CSS;
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

// ─── Panel tab icons ────────────────────────────────────────────────────────

// A 16-viewBox stroke glyph per panel, keyed by the tab title in
// defaultLayoutConfig (and in componentTitles, which must agree).  A stack can
// hold six panels whose titles all read alike at tab width; the icon is what
// makes one findable at a glance.
const kPanelIconPaths = {
    'Layout':           '<rect x="2.5" y="2.5" width="19" height="19" rx="2"/>'
                        + '<path d="M2.5 9h19M9 2.5v19"/>',
    'Schematic':        '<circle cx="6" cy="18" r="2.5"/><circle cx="18" cy="6" r="2.5"/>'
                        + '<path d="M6 15.5V8a2 2 0 012-2h7.5"/>',
    '3D Viewer':        '<path d="M12 2.5l9 5v9l-9 5-9-5v-9z"/><path d="M12 12l9-4.5M12 12v9.5M12 12L3 7.5"/>',
    'Display Controls': '<path d="M3 6h18M3 12h18M3 18h18"/><circle cx="8" cy="6" r="2"/>'
                        + '<circle cx="15" cy="12" r="2"/><circle cx="10" cy="18" r="2"/>',
    'Inspector':        '<path d="M4 6h16M4 12h16M4 18h10"/>',
    'Hierarchy':        '<path d="M5 4v13a2 2 0 002 2h4M5 10h6"/>'
                        + '<rect x="13" y="2" width="7" height="5" rx="1"/>'
                        + '<rect x="13" y="15" width="7" height="5" rx="1"/>',
    'Timing':           '<path d="M2 14h5l3-8 4 14 3-6h5"/>',
    'DRC':              '<path d="M12 3l9 16H3z"/><path d="M12 10v4M12 17h.01"/>',
    'Select Highlight': '<path d="M4 3l14 9-6 1 4 7-3 2-4-7-5 4z"/>',
    'Clock Tree':       '<circle cx="12" cy="12" r="9"/><path d="M12 7v5l3.5 2"/>',
    'Charts':           '<path d="M4 20V10M10 20V4M16 20v-7M22 20V7"/>',
    'Tcl Console':      '<path d="M5 8l4 4-4 4M12 16h7"/>',
    'Help':             '<circle cx="12" cy="12" r="9"/>'
                        + '<path d="M9.5 9.5a2.5 2.5 0 115 .5c0 1.5-2.5 2-2.5 3.5M12 17h.01"/>',
};

// Build the icon element for a tab title, or null when the title has none --
// a Tcl-added panel, say, which should get a plain tab rather than a wrong
// icon.
export function panelTabIcon(title) {
    const path = kPanelIconPaths[title];
    if (!path) return null;
    const span = document.createElement('span');
    span.className = 'or-tab-icon';
    span.innerHTML = '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor"'
        + ' stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round"'
        + ' aria-hidden="true">' + path + '</svg>';
    return span;
}

// Prepend the icon to every tab that does not have one yet, and return how
// many were added.  Idempotent, so it can be called again after any layout
// change -- Golden Layout rebuilds tab elements when a stack is dragged, split
// or reordered -- without accumulating icons.  Titles are read from .lm_title
// rather than from the component, so the overflow dropdown's tabs are
// decorated by the same pass.
//
// The return value matters: an icon widens its tab, and Golden Layout decides
// which tabs fit in a header before these exist, so the caller has to make it
// measure again.  Reporting zero when there was nothing to do is what keeps
// that re-measure from looping through the layout event that triggered it.
export function decorateTabIcons(root = document) {
    let added = 0;
    for (const tab of root.querySelectorAll('.lm_tab:not([data-or-icon])')) {
        const titleEl = tab.querySelector('.lm_title');
        if (!titleEl) continue;
        const title = titleEl.textContent.trim();
        // An empty title means the tab is not built yet.  Marking it now would
        // spend its one chance at an icon on a title we cannot match, so leave
        // it for the next layout change.
        if (!title) continue;
        // Mark before the lookup, so a title with no icon of its own is not
        // re-examined on every subsequent layout change.
        tab.setAttribute('data-or-icon', '');
        const icon = panelTabIcon(title);
        if (icon) {
            tab.insertBefore(icon, titleEl);
            added++;
        }
    }
    return added;
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
                <button class="or-btn cancel">Cancel</button>
                <button class="or-btn or-btn-primary ok">${okLabel}</button>
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
