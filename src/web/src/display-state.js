// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Serialization of the display-controls panel state, for the Tcl
// save_display_controls / restore_display_controls round trip.
//
// Split out of main.js so it can be unit-tested: main.js keeps the parts that
// need the live app (pushing the snapshot over the WebSocket, saving the
// camera, reloading the page), and everything that only touches browser
// storage lives here.

import { getCookie, setCookie, deleteCookie, persistTheme,
         clearPersistedTheme } from './theme.js';

// Every key that together captures the full display-controls state, with the
// browser store it lives in.  The individual controls already persist to these
// keys, so serializing them lets save_display_controls /
// restore_display_controls round-trip the entire panel through the normal
// page-init path — no parallel apply logic to drift out of sync with the
// controls.
//
// The split between the two stores is deliberate, not incidental: per-layer
// visibility, selectability and fill patterns live in sessionStorage rather
// than cookies, because they must survive the reload that opening a database
// triggers but start fresh in a new session, as in Qt (review feedback on
// #10795).  Both paths below dispatch on `store`, so neither silently skips
// the sessionStorage-backed entries.
//
// `store: 'theme'` routes through theme.js, which owns that key's two-store
// persistence (cookie plus a localStorage mirror for standalone reports).
const DISPLAY_STATE_KEYS = [
    { key: 'or_visibility', store: 'cookie' },
    { key: 'or_selectability', store: 'cookie' },
    { key: 'or_hidden_layers', store: 'session' },
    { key: 'or_nonselectable_layers', store: 'session' },
    { key: 'or_layer_patterns', store: 'session' },
    { key: 'or_hidden_chiplets', store: 'cookie' },
    { key: 'or_bg_color', store: 'cookie' },
    { key: 'or_show_dbu', store: 'cookie' },
    { key: 'or_theme', store: 'theme' },
    { key: 'or_use_true_z', store: 'cookie' },
    // Options-menu preferences (2.15).  or_ruler_style predates them (2.12)
    // and was missing here, so a saved state silently dropped it.
    { key: 'or_ruler_style', store: 'cookie' },
    { key: 'or_wheel_zoom', store: 'cookie' },
    { key: 'or_arrow_step', store: 'cookie' },
    { key: 'or_font_family', store: 'cookie' },
    { key: 'or_font_scale', store: 'cookie' },
];

// Storage accessors, tolerant of a browser with Web Storage disabled (the
// cookie helpers in theme.js already are).  The store is resolved by NAME
// inside the try: with storage blocked, reading `window.sessionStorage` throws
// on the property access itself, so passing the object in would throw at the
// call site, outside this guard.
function readStorage(name, key) {
    try {
        return window[name].getItem(key);
    } catch (_) {
        return null;
    }
}

function writeStorage(name, key, value) {
    try {
        if (value === null) {
            window[name].removeItem(key);
        } else {
            window[name].setItem(key, value);
        }
    } catch (_) { /* storage disabled */ }
}

// Values are moved verbatim: the cookie helpers are raw pass-through (their
// callers URI-encode) and the Web Storage keys hold plain JSON, so reading and
// writing through the same store round-trips without re-encoding.
function readDisplayStateKey({ key, store }) {
    if (store === 'session') {
        return readStorage('sessionStorage', key);
    }
    // The theme's cookie is authoritative: its mirror only exists where there
    // is no server to push a snapshot to.
    return getCookie(key);
}

// `value === null` means "unset", so the reload falls back to the default.
function writeDisplayStateKey({ key, store }, value) {
    if (store === 'session') {
        writeStorage('sessionStorage', key, value);
    } else if (store === 'theme') {
        if (value === null) {
            clearPersistedTheme();
        } else {
            persistTheme(value);
        }
    } else if (value === null) {
        deleteCookie(key);
    } else {
        setCookie(key, value);
    }
}

// Snapshot the current display state as a plain object for the server to
// persist.  Only non-empty entries are included so restore can tell "unset"
// (use default) from "set".
export function serializeDisplayState() {
    const entries = {};
    for (const spec of DISPLAY_STATE_KEYS) {
        const value = readDisplayStateKey(spec);
        if (value) entries[spec.key] = value;
    }
    return { version: 1, entries };
}

// Write a saved state's entries back into browser storage.  A key missing from
// the state is cleared, so the page-init path that runs next falls back to the
// default.
//
// Reject anything that is not a cookie-safe string rather than coercing it: a
// number or boolean would silently change a key's meaning (a JSON `1` and the
// string '1' are not interchangeable for readers comparing with ===), and a
// ';' CR or LF would forge attributes on the cookie this is written into.
// Belt and braces — WEB-0054 already rejects such a file server-side, where the
// user gets an actionable error naming the offending entry.
function isUsableDisplayStateValue(value) {
    return typeof value === 'string' && value !== ''
        && !/[;\r\n]/.test(value);
}

export function applyDisplayStateEntries(entries) {
    for (const spec of DISPLAY_STATE_KEYS) {
        const value = entries[spec.key];
        writeDisplayStateKey(
            spec, isUsableDisplayStateValue(value) ? value : null);
    }
}
