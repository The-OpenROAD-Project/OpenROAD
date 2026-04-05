// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Theme initialization and helpers.

if (typeof localStorage !== 'undefined') {
    const savedTheme = localStorage.getItem('theme')
        || (matchMedia('(prefers-color-scheme: light)').matches ? 'light' : 'dark');
    document.documentElement.dataset.theme = savedTheme;
}

// Dark-theme defaults for non-browser environments (e.g. Node.js tests).
// Values must match the dark theme in style.css.
const DARK_THEME_DEFAULTS = {
    canvasBg: '#1a1a1a', canvasText: '#666', canvasAxis: '#555',
    canvasLabel: '#aaa', canvasGrid: '#333', canvasTitle: '#888',
    fgPrimary: '#ccc', fgMuted: '#888', bgPanel: '#252525',
    bgMap: '#111',
};

// Read current CSS custom property values for canvas-based widgets.
export function getThemeColors() {
    if (typeof document === 'undefined') {
        return { ...DARK_THEME_DEFAULTS };
    }
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
