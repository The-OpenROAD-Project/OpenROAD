// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Hierarchy panel — one tab holding the design's tree views, picked from a
// dropdown in the active view's toolbar.  Views are built once and switched by
// hiding one: each holds loaded data and a color map the server already has.

import { ClustersWidget } from './clusters-widget.js';
import { HierarchyBrowser } from './hierarchy-browser.js';

// Add a view here and it shows up in the dropdown.
const VIEWS = [
    { name: 'instances', label: 'Instances', build: (c, app, redraw) =>
        new HierarchyBrowser(c, app, redraw) },
    { name: 'clusters', label: 'Clusters', build: (c, app, redraw) =>
        new ClustersWidget(c, app, redraw) },
];

export class HierarchyPanel {
    constructor(container, app, redrawAllLayers) {
        this._app = app;
        this._widgets = new Map();  // view name → widget
        this._stale = new Set();    // views to re-render when next shown

        this._select = document.createElement('select');
        this._select.className = 'hierarchy-view-select';
        this._select.setAttribute('aria-label', 'Tree view');
        for (const view of VIEWS) {
            const option = document.createElement('option');
            option.value = view.name;
            option.textContent = view.label;
            this._select.appendChild(option);
        }
        this._select.addEventListener('change',
                                      () => this.selectView(this._select.value));

        // All views share this container so their roots are siblings: hiding a
        // root then takes it out of the flow.  Wrapping each view in its own
        // element instead leaves the wrapper occupying the panel's height.
        for (const view of VIEWS) {
            this._widgets.set(view.name,
                              view.build(container, app, redrawAllLayers));
        }

        app.hierarchyPanel = this;
        this.selectView(VIEWS[0].name);
    }

    // Show one view and hand it the selector.  Entry point for the View menu.
    selectView(name) {
        if (!this._widgets.has(name)) return;
        for (const [view_name, widget] of this._widgets) {
            const active = view_name === name;
            widget.element.style.display = active ? '' : 'none';
            if (active) {
                // Moves the node: an element has one parent, so the selector
                // cannot be left behind in the hidden view.
                widget.toolbar.insertBefore(this._select,
                                            widget.toolbar.firstChild);
                if (this._stale.delete(view_name)) widget._render();
            }
        }
        this._select.value = name;
        this._activeView = name;
    }

    // Re-render after something that changes how the rows are formatted (the
    // DBU toggle).  A hidden table cannot be measured, so its columns would
    // come out collapsed; the inactive views are only marked and re-rendered
    // when they are next shown.
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
