// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Theme initialization and helpers.
// Cookies are used instead of localStorage because the port changes on
// every restart (port 0 = OS-assigned) and localStorage is origin-scoped.

function getCookie(name) {
    const m = document.cookie.match('(?:^|; )' + name + '=([^;]*)');
    return m ? m[1] : null;
}

export function setCookie(name, value) {
    document.cookie = name + '=' + value + '; path=/; max-age=31536000; SameSite=Lax';
}

// Enable the Golden Layout theme stylesheet matching the active theme.
export function applyGLTheme(theme) {
    const dark  = document.getElementById('gl-theme-dark');
    const light = document.getElementById('gl-theme-light');
    if (!dark || !light) return;
    dark.disabled  = (theme !== 'dark');
    light.disabled = (theme !== 'light');
}

if (typeof document !== 'undefined') {
    // Try cookie first (shared across ports for the live server),
    // then localStorage (works for standalone file:// reports).
    const savedTheme = getCookie('or_theme')
        || (typeof localStorage !== 'undefined' && localStorage.getItem('or_theme'))
        || (matchMedia('(prefers-color-scheme: light)').matches ? 'light' : 'dark');
    document.documentElement.dataset.theme = savedTheme;
    applyGLTheme(savedTheme);
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
