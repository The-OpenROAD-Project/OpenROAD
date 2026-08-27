// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Renders the custom toolbar populated by Tcl `create_toolbar_button`.
//
// The server (WebViewerHook) is the source of truth: it holds the button
// registry and pushes `{type:'custom_ui', toolbar:[...]}` whenever it
// changes.  main.js fetches the initial registry on connect and calls
// rebuild() on every push, so a button created in one browser's Tcl console
// appears live in every connected browser — something the single-window Qt
// GUI cannot do.
//
// Each button descriptor (from customUiJson()):
//   { key, text, script, icon, tooltip, toggle, script_off, echo }
//
// `icon`/`tooltip` and `toggle` are web-only extensions over Qt's text-only,
// stateless create_toolbar_button.
import { runTclScript } from './ui-utils.js';

export function createToolbar(app) {
    const bar = document.getElementById('toolbar');
    if (!bar) return;

    // Persist toggle state across rebuilds (keyed by button key) so a live
    // registry push doesn't reset a button the user turned on.
    if (!app._toolbarToggleState) app._toolbarToggleState = {};

    function render() {
        bar.innerHTML = '';
        const buttons = app.customToolbar || [];
        // Drop stale toggle state for buttons that no longer exist.
        const liveKeys = new Set(buttons.map(b => b.key));
        for (const k of Object.keys(app._toolbarToggleState)) {
            if (!liveKeys.has(k)) delete app._toolbarToggleState[k];
        }

        for (const btn of buttons) {
            const el = document.createElement('button');
            el.className = 'toolbar-button';
            el.dataset.key = btn.key;
            if (btn.tooltip) el.title = btn.tooltip;

            if (btn.icon) {
                const iconEl = document.createElement('span');
                iconEl.className = 'toolbar-icon';
                // Support both emoji/text icons and image data URIs / URLs.
                if (/^(data:|https?:|\.?\/)/.test(btn.icon)) {
                    const img = document.createElement('img');
                    img.src = btn.icon;
                    img.alt = '';
                    iconEl.appendChild(img);
                } else {
                    iconEl.textContent = btn.icon;
                }
                el.appendChild(iconEl);
            }

            if (btn.text) {
                const label = document.createElement('span');
                label.className = 'toolbar-label';
                label.textContent = btn.text;
                el.appendChild(label);
            }

            if (btn.toggle && app._toolbarToggleState[btn.key]) {
                el.classList.add('active');
            }

            el.addEventListener('click', () => {
                if (btn.toggle) {
                    const on = !app._toolbarToggleState[btn.key];
                    app._toolbarToggleState[btn.key] = on;
                    el.classList.toggle('active', on);
                    // script when turning on, script_off when turning off.
                    runTclScript(app, on ? btn.script : btn.script_off, btn.echo);
                } else {
                    runTclScript(app, btn.script, btn.echo);
                }
            });

            bar.appendChild(el);
        }
    }

    render();

    // Exposed so main.js can re-render when a custom_ui push arrives.
    app.rebuildToolbar = render;
    return { rebuild: render };
}
