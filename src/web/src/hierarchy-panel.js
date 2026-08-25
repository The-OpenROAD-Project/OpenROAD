// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Hierarchy panel — one tab holding the design's tree views, picked from a
// dropdown in the active view's toolbar.  Views are built once and switched by
// hiding one: each holds loaded data and a color map the server already has.

import { ClustersWidget } from './clusters-widget.js';
import { HierarchyBrowser } from './hierarchy-browser.js';
import { getCookie, setCookie } from './theme.js';

// Add a view here and it shows up in the dropdown, with its overlay wired up.
// The labels name where the tree comes from, not what it is called internally:
// one is the Verilog module tree, the other the design's dbGroups.  `gate` is
// the visibility flag its overlay draws under, `layer` the pseudo-layer that
// paints it.
const VIEWS = [
    { name: 'instances', label: 'Verilog Modules',
        gate: 'module_view', layer: 'modulesLayer', noun: 'modules',
        build: (c, app, redraw, gate) =>
            new HierarchyBrowser(c, app, redraw, gate) },
    { name: 'clusters', label: 'Instance Groups',
        gate: 'cluster_view', layer: 'clustersLayer', noun: 'groups',
        build: (c, app, redraw, gate) =>
            new ClustersWidget(c, app, redraw, gate) },
];

// The source outlives the page: it decides which overlay `ui_hierarchy_view`
// turns on, so a session that came back on the other one would paint the
// wrong thing.  Part of the saved display state (see display-state.js).
const SOURCE_COOKIE = 'or_hierarchy_source';

// Validated, not trusted: a stale or hand-edited cookie naming a view that no
// longer exists would leave the panel with no active view at all, since
// selectView() ignores names it does not know.
function savedSource() {
    const name = getCookie(SOURCE_COOKIE);
    return VIEWS.some(v => v.name === name) ? name : VIEWS[0].name;
}

// Whose source is showing: the panel's, or the last one recorded if a saved
// layout came back without the tab.
export function activeHierarchySource(app) {
    const panel = app ? app.hierarchyPanel : null;
    return panel ? panel.activeView() : savedSource();
}

// The two color overlays are one control: `ui_hierarchy_view` says whether the
// hierarchy paints at all, `source` says which of the two does it.  Their own
// flags stay the gate each pseudo-layer and the server read, so the Tcl
// `-display_option {cluster_view true}` path is untouched by this.
//
// A pure derivation over the map it is handed — the source comes in rather
// than being fetched, so nothing here depends on which panel is registered.
// Callers without one of their own get it from activeHierarchySource().
// Returns whether anything moved, so callers can skip a repaint nobody needs.
export function syncHierarchyOverlay(visibility, source) {
    let changed = false;
    for (const view of VIEWS) {
        const on = !!visibility.ui_hierarchy_view && view.name === source;
        changed = changed || visibility[view.gate] !== on;
        visibility[view.gate] = on;
    }
    return changed;
}

export class HierarchyPanel {
    constructor(container, app, redrawAllLayers) {
        this._app = app;
        this._widgets = new Map();  // view name → widget
        this._stale = new Set();    // views to re-render when next shown

        this._select = document.createElement('select');
        this._select.className = 'hierarchy-view-select';
        this._select.setAttribute('aria-label', 'Source');
        for (const view of VIEWS) {
            const option = document.createElement('option');
            option.value = view.name;
            option.textContent = view.label;
            this._select.appendChild(option);
        }
        this._select.addEventListener('change',
                                      () => this.selectView(this._select.value));

        // Label and select as one node: selectView moves the picker between
        // toolbars and the label has to travel with it.
        const label = document.createElement('span');
        label.className = 'hierarchy-view-label';
        label.textContent = 'Source:';
        this._picker = document.createElement('span');
        this._picker.className = 'hierarchy-view-picker';
        this._picker.appendChild(label);
        this._picker.appendChild(this._select);

        // All views share this container so their roots are siblings: hiding a
        // root then takes it out of the flow.  Wrapping each view in its own
        // element instead leaves the wrapper occupying the panel's height.
        for (const view of VIEWS) {
            this._widgets.set(
                view.name,
                view.build(container, app, redrawAllLayers, view.gate));
        }

        app.hierarchyPanel = this;
        this.selectView(savedSource());

        container.on('destroy', () => this._onDestroy());
    }

    // Closing the tab takes the coloring with it: the overlays are fed by
    // these views, and with the tab gone there is no source dropdown and no
    // per-row checkbox left to control what they paint.  The flags stay as
    // they are -- the Display Controls checkbox is the user's, not ours -- and
    // an empty color map is what stops the drawing.
    _onDestroy() {
        // A reopened tab registers before the old one is destroyed; only the
        // panel still on the app may clear it.
        if (this._app.hierarchyPanel === this) this._app.hierarchyPanel = null;
        // Both, not just the visible one: the hidden view has a map on the
        // server too.
        for (const widget of this._widgets.values()) widget.clearOverlay();
    }

    // Show one view and hand it the selector.  What the Source dropdown calls.
    selectView(name) {
        if (!this._widgets.has(name)) return;
        for (const [view_name, widget] of this._widgets) {
            const active = view_name === name;
            widget.element.style.display = active ? '' : 'none';
            if (active) {
                // Moves the node: an element has one parent, so the picker
                // cannot be left behind in the hidden view.
                widget.toolbar.insertBefore(this._picker,
                                            widget.toolbar.firstChild);
                if (this._stale.delete(view_name)) widget._render();
            }
        }
        this._select.value = name;
        this._activeView = name;
        setCookie(SOURCE_COOKIE, name);
        // Set last: the derivation reads activeView() back off the panel.
        this.syncOverlay();
    }

    // Move the overlay to the view now on screen.  Repaints only the two
    // pseudo-layers, and only when a flag actually moved: nothing else reads
    // these flags, so a full redraw would re-request every metal layer's grid
    // for a change they ignore.  Display Controls does its own redraw, so it
    // calls refreshActiveStatus() instead of this.
    syncOverlay() {
        const app = this._app;
        if (!app || !app.visibility) return;
        if (syncHierarchyOverlay(app.visibility, this._activeView)) {
            // Both, not just the one now on: the other has to stop painting.
            for (const view of VIEWS) {
                const layer = app[view.layer];
                if (layer) layer.refreshTiles();
            }
        }
        this.refreshActiveStatus();
    }

    // Let the visible view redo its status line, which reports whether its
    // overlay is switched on.  Nothing else writes that line, so a warning
    // would otherwise outlive what it warns about.
    refreshActiveStatus() {
        const widget = this.activeWidget();
        if (widget) widget.refreshStatus();
    }

    // Re-render after something that changes how the rows are formatted (the
    // DBU toggle).  Only the view on screen: a hidden one is a full table
    // rebuild nobody is looking at, so it is marked and rendered when shown.
    refresh() {
        for (const [name, widget] of this._widgets) {
            if (name === this._activeView) {
                widget._render();
            } else {
                this._stale.add(name);
            }
        }
    }

    activeView() {
        return this._activeView;
    }

    activeWidget() {
        return this._widgets.get(this._activeView);
    }
}
