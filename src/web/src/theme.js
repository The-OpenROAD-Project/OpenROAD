// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Theme initialization and helpers.
// Cookies are used instead of localStorage because the port changes on
// every restart (port 0 = OS-assigned) and localStorage is origin-scoped.

import { isValidHexColor } from './ui-utils.js';

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
// passed so the 3D viewer (which caches --bg-map) can re-render.
export function setBackgroundColor(color, app) {
    if (!isValidHexColor(color)) {
        return;
    }
    document.documentElement.style.setProperty('--bg-map', color);
    setCookie('or_bg_color', color);
    refreshBackgroundConsumers(app);
}

// Drop the override and fall back to the theme's --bg-map value.
export function resetBackgroundColor(app) {
    document.documentElement.style.removeProperty('--bg-map');
    deleteCookie('or_bg_color');
    refreshBackgroundConsumers(app);
}

// The layout container reads --bg-map live via CSS, but the 3D viewer
// caches the resolved color in its Three.js scene, so re-render it on
// change.  (Charts/clock use --canvas-bg, not --bg-map, so they don't
// need refreshing here.)
function refreshBackgroundConsumers(app) {
    if (app && app.threeDViewerWidget && app.threeDViewerWidget.render) {
        app.threeDViewerWidget.render();
    }
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
    };
}
