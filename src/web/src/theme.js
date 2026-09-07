// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Theme initialization and helpers.
// Cookies are used instead of localStorage because the port changes on
// every restart (port 0 = OS-assigned) and localStorage is origin-scoped.

import { cssColorToHex, isValidHexColor } from './ui-utils.js';

export function getCookie(name) {
    const m = document.cookie.match('(?:^|; )' + name + '=([^;]*)');
    return m ? m[1] : null;
}

export function setCookie(name, value) {
    document.cookie = name + '=' + value + '; path=/; max-age=31536000; SameSite=Lax';
}

// Expire a cookie immediately.  The path/SameSite attributes must match
// setCookie's for the browser to target the same cookie.
export function deleteCookie(name) {
    document.cookie = name + '=; path=/; max-age=0; SameSite=Lax';
}

// The theme is persisted in TWO stores: a cookie (shared across ports for the
// live server) and a localStorage mirror (which is all a standalone file://
// report has).  Initialization below reads `cookie || localStorage || system`,
// so the two must always move together — clearing only the cookie would fall
// through to a stale mirror.  Both writers go through this pair so that rule
// lives in exactly one place.
//
// Persistence only: applying the theme to the DOM is the caller's business,
// because the display-state restore path writes storage and then reloads.
export function persistTheme(theme) {
    setCookie('or_theme', theme);
    try {
        window.localStorage.setItem('or_theme', theme);
    } catch (_) { /* storage disabled */ }
}

export function clearPersistedTheme() {
    deleteCookie('or_theme');
    try {
        window.localStorage.removeItem('or_theme');
    } catch (_) { /* storage disabled */ }
}

// Enable the Golden Layout theme stylesheet matching the active theme.
export function applyGLTheme(theme) {
    const dark  = document.getElementById('gl-theme-dark');
    const light = document.getElementById('gl-theme-light');
    if (!dark || !light) return;
    dark.disabled  = (theme !== 'dark');
    light.disabled = (theme !== 'light');
}

// Optional per-user override of the layout background color (Qt GUI
// "Background" parity).  Stored as "#rrggbb"; applied as an inline
// --bg-map override so it survives dark/light theme toggles.  app is
// passed so the 3D viewer (which caches --bg-map) can re-render, and
// so the saved display state (which includes or_bg_color) is pushed to
// the server here — callers don't need to remember to sync.
export function setBackgroundColor(color, app) {
    if (!isValidHexColor(color)) {
        return;
    }
    document.documentElement.style.setProperty('--bg-map', color);
    setCookie('or_bg_color', color);
    refreshBackgroundConsumers(app);
    app.syncDisplayState();
}

// Drop the override and fall back to the theme's --bg-map value.
export function resetBackgroundColor(app) {
    document.documentElement.style.removeProperty('--bg-map');
    deleteCookie('or_bg_color');
    refreshBackgroundConsumers(app);
    app.syncDisplayState();
}

// The active theme's own --bg-map value in the "#rrggbb" form an
// <input type="color"> requires (#000 in both themes, matching the Qt GUI's
// default background so the same layer alpha composites to the same color).
// Must be read with no inline override active — resetBackgroundColor
// removes it — otherwise the override value is returned instead.
export function getThemeDefaultBgColor() {
    return cssColorToHex(
        getComputedStyle(document.documentElement)
            .getPropertyValue('--bg-map')) ?? '#000000';
}

// The layout container reads --bg-map live via CSS, but the 3D viewer
// caches the resolved color in its Three.js scene, so re-render it on
// change.  (Charts/clock use --canvas-bg, not --bg-map, so they don't
// need refreshing here.)
function refreshBackgroundConsumers(app) {
    app.threeDViewerWidget?.render?.();
}

if (typeof document !== 'undefined') {
    // Try cookie first (shared across ports for the live server),
    // then localStorage (works for standalone file:// reports).
    const savedTheme = getCookie('or_theme')
        || (typeof localStorage !== 'undefined' && localStorage.getItem('or_theme'))
        || (matchMedia('(prefers-color-scheme: light)').matches ? 'light' : 'dark');
    document.documentElement.dataset.theme = savedTheme;
    applyGLTheme(savedTheme);
    // Restore a saved background-color override, if any and valid.
    const savedBg = getCookie('or_bg_color');
    if (isValidHexColor(savedBg)) {
        document.documentElement.style.setProperty('--bg-map', savedBg);
    }
}

// Read current CSS custom property values for canvas-based widgets.
export function getThemeColors() {
    const s = getComputedStyle(document.documentElement);
    const v = (name) => s.getPropertyValue(name).trim();
    return {
        canvasBg:    v('--canvas-bg'),
        canvasText:  v('--canvas-text'),
        canvasAxis:  v('--canvas-axis'),
        canvasLabel: v('--canvas-label'),
        canvasGrid:  v('--canvas-grid'),
        canvasTitle: v('--canvas-title'),
        fgPrimary:   v('--fg-primary'),
        fgMuted:     v('--fg-muted'),
        bgPanel:     v('--bg-panel'),
        bgMap:       v('--bg-map'),
        // Font stacks for ctx.font, so the canvas widgets draw in the same
        // faces as the DOM ones: mono for values, sans for titles and labels.
        fontMono:    v('--font-mono'),
        fontSans:    v('--font-sans'),
    };
}
