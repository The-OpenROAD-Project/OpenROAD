// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Display controls — layer checkboxes and visibility tree.

import { CheckboxTreeModel } from './checkbox-tree-model.js';
import { VisTree, makeColumnHeader, makeNameSpan, makeSelSpacer }
    from './vis-tree.js';
import { getCookie, setCookie, setBackgroundColor, resetBackgroundColor,
         getThemeDefaultBgColor }
    from './theme.js';
import { isStaticMode, makeGroupHeader, attachGroupCollapse, beginSelection,
         isCurrentSelection, onSelectionReset, isValidHexColor }
    from './ui-utils.js';

// Compute a Set of layer indices around `center` within [0, count).
// `lower` layers below and `upper` layers above are included.
export function layerRangeSet(center, lower, upper, count) {
    const indices = new Set();
    const lo = Math.max(0, center - lower);
    const hi = Math.min(count - 1, center + upper);
    for (let i = lo; i <= hi; i++) indices.add(i);
    return indices;
}

// Reduce a layer-name → FillPattern-int map to only the non-solid entries so
// the cookie stays small and Solid (the default) never needs persisting.
// Values mirror the server's FillPattern enum (1 = solid).
export function nonSolidPatterns(patterns) {
    const out = {};
    for (const [name, value] of Object.entries(patterns || {})) {
        if (value !== 1) {
            out[name] = value;
        }
    }
    return out;
}

// Build the CheckboxTreeModel input for the Chiplets group.  Each
// `chipletData` entry comes from the backend serializeTechResponse and
// has shape { path, name, parent, master, depth }.  `savedHidden` is the
// set of chiplet paths the user has hidden (loaded from the cookie).
//
// Returns a flat array of nodes keyed by the chiplet path string.  The
// root chiplet has parentId === null; hasCheckbox is false for the root
// (its DOM toggle is rendered by the group header).
export function buildChipletFlatNodes(chipletData, savedHidden) {
    return chipletData.map((c) => {
        const visible = !savedHidden.has(c.path);
        return {
            id: c.path,
            parentId: c.parent != null ? c.parent : null,
            hasCheckbox: c.parent != null,
            checked: visible,
            data: { path: c.path, name: c.name, master: c.master,
                    depth: c.depth },
        };
    });
}

// Fallback color used when the server didn't supply a layer color.
const fallbackLayerPalette = [
    [70, 130, 210],  // moderate blue
    [200, 50, 50],   // red
    [50, 180, 80],   // green
    [200, 160, 40],  // amber
    [160, 60, 200],  // purple
    [40, 190, 190],  // teal
    [220, 120, 50],  // orange
    [180, 70, 150],  // magenta
];

import {
    computeGroupCount, estimateTilesPerPane, measureViewport,
    partitionIntoGroups, reserveForUnmergedPanes, setItemVisible, tileBytes,
    DEFAULT_BUDGET_BYTES, UNMERGED_PANE_COUNT,
} from './tile-merge.js';
import { buildMergedPanes } from './merged-tile-layer.js';

// Populate display controls with layer checkboxes and visibility tree.
export function populateDisplayControls(app, visibility, selectability,
                                         WebSocketTileLayer,
                                         techData, redrawAllLayers,
                                         HeatMapTileLayer) {
    if (!app.displayControlsEl) return;
    app.displayControlsEl.innerHTML = '';
    app.allLayers = [];

    // Column header (visibility / selectability icons), mirroring the Qt GUI's
    // QHeaderView.  Added first so it is the panel's sticky top row.
    app.displayControlsEl.appendChild(makeColumnHeader());

    // ─── Tile grouping ───────────────────────────────────────────────────
    //
    // Two modes.  Legacy: one Leaflet pane per tech layer per chiplet, which on
    // a multi-die design is ~97 panes holding ~582 MB of decoded tile images —
    // past the browser's ~458 MB ceiling, where it starts discarding decodes and
    // the discarded regions paint white.  Merged:
    // the routing layers are composited into N canvas panes, N chosen from a
    // memory budget, so the total is bounded no matter how many layers or
    // chiplets the design has.
    //
    // Only the routing layers are merged.  The pseudo layers (_instances,
    // _pins, _modules and the Misc overlays below) stay as their own panes:
    // there is a fixed handful of them, they carry no meaningful memory, and
    // they have their own add/remove rules driven by the Shapes, Module-view
    // and Misc toggles which are not worth folding into the draw list.
    const mergedLayerClass = app.mergeTiles ? app.MergedTileLayer : null;
    // Panes whose draw list changed during one tree update, refreshed once at
    // the end rather than per layer — a group toggle walks every node, and
    // refreshing per node would re-request the same pane dozens of times.
    const dirtyPanes = new Set();

    function flushDirtyPanes() {
        for (const pane of dirtyPanes) {
            pane.refreshTiles();
        }
        dirtyPanes.clear();
    }

    // A routing layer, as the tree sees it.  In legacy mode this IS the Leaflet
    // layer; in merged mode it is an adapter over one entry of a pane's draw
    // list, exposing the same three things the tree uses.
    //
    // Routing layers are drawn at full pane opacity.  The layer transparency is
    // already baked into the tile: the server paints layer shapes with the
    // palette alpha of 180/255 (tile_generator.cpp buildLayerColorMap), the same
    // alpha displayControls.cpp gives the Qt GUI's brush.  Dimming the pane on
    // top of that would apply the transparency a second time and render the
    // layout darker than the Qt GUI.
    function makeRoutingLayer(name, zIndex) {
        if (!mergedLayerClass) {
            const layer = new WebSocketTileLayer(app.websocketManager, name, {
                opacity: 1,
                zIndex,
            });
            layer._orShow = () => layer.addTo(app.map);
            layer._orHide = () => app.map.removeLayer(layer);
            return layer;
        }
        const item = { layer: name, opacity: 1, visible: false };
        return {
            _orItem: item,
            _orPane: null,
            refreshTiles() {
                if (this._orPane) {
                    dirtyPanes.add(this._orPane);
                    flushDirtyPanes();
                }
            },
            // setItemVisible reports whether the state actually changed, and
            // only a real change dirties the pane.  The tree's onChange walks
            // EVERY node and asserts the desired state on each, so dirtying
            // unconditionally made one checkbox re-request every tile in every
            // pane.
            _orShow() {
                if (setItemVisible(item, true) && this._orPane) {
                    dirtyPanes.add(this._orPane);
                }
            },
            _orHide() {
                if (setItemVisible(item, false) && this._orPane) {
                    dirtyPanes.add(this._orPane);
                }
            },
        };
    }

    // Create a pseudo-layer tile layer and register it on the app.
    // `appProp` names the app.<prop> slot (null = anonymous); `addToMap`
    // attaches it immediately (layers whose toggle is default-ON).
    function addPseudoLayer(name, appProp, zIndex, addToMap) {
        const layer = new WebSocketTileLayer(app.websocketManager, name, {
            zIndex,
        });
        if (addToMap) layer.addTo(app.map);
        if (appProp) app[appProp] = layer;
        app.allLayers.push(layer);
        return layer;
    }

    // The initial attach state must follow `visibility`, which was already
    // restored from the or_visibility cookie: a hardcoded attach leaves the
    // checkbox (rendered from `visibility`) out of sync with the map until
    // the first toggle runs redrawAllLayers.

    // Instance borders layer (always below routing layers; no toggle)
    addPseudoLayer('_instances', null, 0, true);
    // IO pin markers layer (between instances and routing layers)
    addPseudoLayer('_pins', 'pinsLayer', 1, visibility.pins);
    // Module coloring overlay (Module view)
    addPseudoLayer('_modules', 'modulesLayer', 2, visibility.module_view);
    // Access-point markers overlay (Misc > Access Points)
    addPseudoLayer(
        '_access_points', 'accessPointsLayer', 1000, visibility.access_points);
    // Manufacturing-grid dots overlay (Misc > Manufacturing grid)
    addPseudoLayer('_mfg_grid', 'mfgGridLayer', 2, visibility.mfg_grid);
    // GCell-grid lines overlay (topmost, GUI paint order)
    addPseudoLayer('_gcell_grid', 'gcellGridLayer', 1002, visibility.gcell_grid);

    // Region boundaries overlay (above access points, GUI paint order).
    // Only created when the design has dbRegions — the layer is default-ON
    // (Qt parity) and would otherwise issue per-viewport tile requests that
    // always come back transparent.  (Regions created via Tcl mid-session
    // need a page reload to appear.)
    app.regionsLayer = null;
    if (techData && techData.has_regions) {
        addPseudoLayer('_regions', 'regionsLayer', 1001, visibility.regions);
    }

    // --- Layers group (using CheckboxTreeModel) ---

    // Create Leaflet layers and build a model spec.
    const leafletLayers = [];  // index → WebSocketTileLayer
    const layerIds = [];       // index → model node id

    // Forward declaration so the layerModel callback can mirror chiplet
    // toggles into the Chiplets panel (created later when chipletData
    // has more than one entry).
    let chipletModel = null;

    // Restore saved hidden-layers and non-selectable-layers sets.
    // Per-layer visibility/selectability intentionally live in
    // sessionStorage, not cookies: they must survive the page reload that
    // opening a database triggers, but start fresh in a new session — the
    // Qt GUI does not persist layer options between sessions either
    // (review feedback on #10795).
    let savedHiddenLayers = new Set();
    let savedNonSelectableLayers = new Set();
    try {
        const raw = window.sessionStorage.getItem('or_hidden_layers');
        if (raw) savedHiddenLayers = new Set(JSON.parse(raw));
    } catch (_) { /* ignore */ }
    try {
        const raw = window.sessionStorage.getItem('or_nonselectable_layers');
        if (raw) {
            savedNonSelectableLayers = new Set(JSON.parse(raw));
        }
    } catch (_) { /* ignore */ }

    // Restore saved per-layer fill patterns (raw layer name → FillPattern int).
    // Kept in sessionStorage alongside the visibility/selectability sets so a
    // pattern survives the open-database reload but starts fresh in a new
    // session, matching the Qt GUI (review feedback on #10795).
    try {
        const raw = window.sessionStorage.getItem('or_layer_patterns');
        if (raw) Object.assign(app.layerPatterns, JSON.parse(raw));
    } catch (_) { /* ignore */ }

    // Persist only non-solid patterns; setting a layer back to Solid drops it.
    function persistLayerPatterns() {
        try {
            window.sessionStorage.setItem(
                'or_layer_patterns',
                JSON.stringify(nonSolidPatterns(app.layerPatterns)));
        } catch (_) { /* ignore */ }
    }

    // Apply a fill pattern to one layer and re-render just that layer's tiles.
    function setLayerPattern(name, layer, value) {
        if (value === 1) {
            delete app.layerPatterns[name];
        } else {
            app.layerPatterns[name] = value;
        }
        persistLayerPatterns();
        if (layer && typeof layer.refreshTiles === 'function') {
            layer.refreshTiles();
        }
    }

    // Fill-pattern choices for the layer context menu (values match the
    // server's FillPattern enum: kNone=0, kSolid=1, kDiagonal=2, …).
    const PATTERN_OPTIONS = [
        { label: 'Solid', value: 1 },
        { label: 'Diagonal', value: 2 },
        { label: 'Cross', value: 3 },
        { label: 'Dots', value: 4 },
        { label: 'None (no fill)', value: 0 },
    ];

    // Global counter so each layer (across the whole hierarchy) gets a unique
    // z-index and palette slot regardless of which chiplet it belongs to.
    let nextLayerSlot = 0;

    // `ownerPath` is the chiplet whose dbTech owns the layers rendered at this
    // level.  Category nodes (Backside/Implant/Other) are pure UI folders with
    // no path of their own, so they inherit their parent chiplet's — the
    // select_layer request needs it to pick the right tech in a multi-die
    // design, where two chips can both have an "M1".
    function buildLayerSpec(hierarchyNode, parentId = 'layers',
                            ownerPath = undefined) {
        const children = [];
        const chipletPath = hierarchyNode.type === 'category'
            ? ownerPath : hierarchyNode.path;

        if (hierarchyNode.instances && hierarchyNode.instances.length > 0) {
            hierarchyNode.instances.forEach((inst, idx) => {
                const instId = parentId + "/" + (inst.name || idx);
                children.push(buildLayerSpec(inst, instId, chipletPath));
            });
        }

        if (hierarchyNode.layers && hierarchyNode.layers.length > 0) {
            hierarchyNode.layers.forEach((layerObj) => {
                const name = layerObj.name || layerObj;
                const slot = nextLayerSlot++;
                const zIndex = slot + 3;
                const layer = makeRoutingLayer(name, zIndex);

                const id = `${parentId}/${name}`;
                layer._nodeId = id;

                const visible = !savedHiddenLayers.has(id);
                if (visible) {
                    layer._orShow();
                    app.visibleLayers.add(id);
                    app.visibleLayerNames.add(name);
                }
                // In merged mode the panes go into allLayers instead, once the
                // grouping is known — a draw-list entry is not something
                // redrawAllLayers() can refresh.
                if (!mergedLayerClass) {
                    app.allLayers.push(layer);
                }
                leafletLayers.push(layer);

                layerIds.push(id);
                children.push({
                    id,
                    // ownerChipletPath, not chipletPath: the latter means
                    // "this node IS that chiplet" and drives the Chiplets
                    // panel sync, which must only ever see group nodes.
                    data: { name, layer, color: layerObj.color,
                            colorIndex: slot, nodeId: id,
                            ownerChipletPath: chipletPath },
                    checked: visible,
                });
            });
        }

        const nodeData = { name: hierarchyNode.name, isInstance: true };
        // Review feedback on #10795: the Implant and Other categories
        // start collapsed (they are rarely-used layer groups).
        if (hierarchyNode.type === 'category'
            && (hierarchyNode.name === 'Implant'
                || hierarchyNode.name === 'Other')) {
            nodeData.startCollapsed = true;
        }
        // chipletPath is the canonical "top.wrapper_1.MEM_2" string the
        // backend emits in layer_hierarchy; it matches ChipletNode::path
        // exactly so toggling this node can drive app.visibleChiplets.
        // Category nodes (e.g. "Backside") are pure UI folders — they have
        // no chiplet path and must not participate in chiplet sync.
        if (hierarchyNode.type !== 'category') {
            nodeData.chipletPath = hierarchyNode.path;
        }
        return {
            id: parentId,
            data: nodeData,
            children: children,
        };
    }

    let layerSpec;
    if (techData.layer_hierarchy) {
        layerSpec = buildLayerSpec(techData.layer_hierarchy, 'layers_parent');
    } else {
        // Fallback for old backends
        layerSpec = {
            id: 'layers_parent',
            children: techData.layers.map((name, index) => {
                const layer = makeRoutingLayer(name, index + 3);

                const id = `layers_parent/${name}`;
                layer._nodeId = id;

                const visible = !savedHiddenLayers.has(id);
                if (visible) {
                    layer._orShow();
                    app.visibleLayers.add(id);
                    app.visibleLayerNames.add(name);
                }
                if (!mergedLayerClass) {
                    app.allLayers.push(layer);
                }
                leafletLayers.push(layer);

                layerIds.push(id);
                return { id, data: { name, layer, colorIndex: index, nodeId: id }, checked: visible };
            }),
        };
    }

    // ─── Build the merged panes ──────────────────────────────────────────
    //
    // leafletLayers is in slot order, which IS the z-order, so it can be
    // partitioned directly.  Runs must be contiguous: each group becomes one
    // image, and interleaved groups have no stacking that reproduces the
    // original (see partitionIntoGroups).
    if (mergedLayerClass && leafletLayers.length > 0) {
        const items = leafletLayers.map(l => l._orItem);
        const budget = app.tileBudgetBytes || DEFAULT_BUDGET_BYTES;

        // How many groups fit right now.  measureViewport falls back to the
        // window when the container has not been laid out yet: a 0x0 container
        // yields 1 tile per pane, which makes the budget look big enough for
        // one pane per layer and silently disables the merging entirely.
        function wantedGroupCount() {
            const { width, height } = measureViewport(app.map.getContainer());
            const tilesPerPane = estimateTilesPerPane(width, height);
            const perTile = tileBytes(256, app.tileDpr ? app.tileDpr() : 1);
            // The panes that are NOT merged still hold full tile grids, and
            // they are not free: at dpr 3 each costs ~54 MB, so the three of
            // them would put the real total over the ceiling while the budget
            // reported it as fitting.  Charge them first.
            const count = app.mergeGroupCount || computeGroupCount({
                budgetBytes: reserveForUnmergedPanes(budget, tilesPerPane,
                                                     perTile),
                tilesPerPane,
                bytesPerTile: perTile,
                paneCount: items.length,
            });
            return { count, tilesPerPane, perTile };
        }

        function describe(paneCount, tilesPerPane, perTile) {
            // estimatedMB counts the unmerged panes too, so the reported figure
            // is the whole viewer's tile memory rather than just the part this
            // grouping controls.
            const total = (paneCount + UNMERGED_PANE_COUNT) * tilesPerPane
                          * perTile;
            return {
                layers: items.length,
                groups: paneCount,
                tilesPerPane,
                unmergedPanes: UNMERGED_PANE_COUNT,
                budgetMB: Math.round(budget / (1024 * 1024)),
                estimatedMB: Math.round(total / (1024 * 1024)),
            };
        }

        // Hand each group to a pane and point every tree entry at the pane that
        // owns its draw item.
        function assign(panes, groups) {
            let at = 0;
            groups.forEach((group, gi) => {
                for (let i = 0; i < group.length; i++) {
                    leafletLayers[at++]._orPane = panes[gi];
                }
            });
        }

        const initial = wantedGroupCount();
        const groups = partitionIntoGroups(items, initial.count);
        // zIndex 3 upwards, matching the slots the per-layer panes used, so the
        // routing stack still sits above _instances/_pins/_modules.
        const panes = buildMergedPanes(mergedLayerClass, app.websocketManager,
                                       groups, app.map, 3);
        assign(panes, groups);
        // These are what redrawAllLayers() refreshes — the design-changed push,
        // pattern changes and visibility changes all arrive through it.
        app.allLayers.push(...panes);
        app.mergedPanes = panes;
        app.mergeStats
            = describe(panes.length, initial.tilesPerPane, initial.perTile);
        console.log('[tiles] merged %d layers into %d panes '
                    + '(~%d MB of %d MB budget, %d tiles/pane)',
                    app.mergeStats.layers, app.mergeStats.groups,
                    app.mergeStats.estimatedMB, app.mergeStats.budgetMB,
                    initial.tilesPerPane);

        // A pane holds a full grid of tiles, so enlarging the window raises the
        // per-pane cost and a grouping that fitted the budget at startup no
        // longer does — the same failure, arriving later.  Recompute on resize.
        //
        // Shrink only.  Growing N back when the window shrinks would churn
        // panes on every drag of a splitter for no safety benefit, and leaves
        // the merge coarser than strictly necessary, which is the harmless
        // direction.  A layer toggle then re-merges a slightly larger group.
        app.map.on('resize', () => {
            const want = wantedGroupCount();
            if (want.count >= app.mergedPanes.length) {
                return;
            }
            const regrouped = partitionIntoGroups(items, want.count);
            // Reuse the surviving panes and drop the surplus, so z-order and
            // pane identity stay stable for the ones that remain.
            const keep = app.mergedPanes.slice(0, regrouped.length);
            for (const pane of app.mergedPanes.slice(regrouped.length)) {
                app.map.removeLayer(pane);
                const at = app.allLayers.indexOf(pane);
                if (at >= 0) {
                    app.allLayers.splice(at, 1);
                }
            }
            app.mergedPanes = keep;
            assign(keep, regrouped);
            keep.forEach((pane, i) => pane.setItems(regrouped[i]));
            app.mergeStats
                = describe(keep.length, want.tilesPerPane, want.perTile);
            console.log('[tiles] window grew — regrouped %d layers into %d '
                        + 'panes (~%d MB of %d MB budget, %d tiles/pane)',
                        app.mergeStats.layers, app.mergeStats.groups,
                        app.mergeStats.estimatedMB, app.mergeStats.budgetMB,
                        want.tilesPerPane);
        });
    }

    // Parallel selectability model with the same node ids as layerSpec so
    // syncLayerSelDom() and buildLayerDOM() can pair each visibility node
    // with its selectability peer.
    function mirrorForSelectability(node) {
        if (!node.children || node.children.length === 0) {
            const name = node.data && node.data.name;
            const selectable = name ? !savedNonSelectableLayers.has(name) : true;
            if (selectable && name) {
                app.selectableLayers.add(name);
            }
            return { id: node.id, data: { name }, checked: selectable };
        }
        return {
            id: node.id,
            data: { name: node.data && node.data.name },
            children: node.children.map(mirrorForSelectability),
        };
    }
    const layerSelSpec = mirrorForSelectability(layerSpec);

    const layerModel = new CheckboxTreeModel(() => {
        // Single pass over the tree: rebuild visibleLayerNames in place
        // (the WebSocketTileLayer closure captured this Set by reference
        // at startup, so we mutate rather than reassign), sync DOM and
        // Leaflet, collect the cookie payload and accumulate any chiplet
        // toggles that need to mirror into the Chiplets panel.  The
        // rebuild-from-scratch pattern avoids incrementally dropping a
        // layer name still owned by a checked sibling in multi-tech
        // designs.
        app.visibleLayerNames.clear();
        const allLayerIds = [];
        // Two-tier chiplet propagation: stateUpdates carries every
        // tri-state change for the UI checkboxes; visibilityChangedAny
        // tracks whether any toggle also moved the chiplet in or out
        // of visibleChiplets.  Splitting them lets us skip the
        // chipletModel.onChange path (which redraws every Leaflet tile)
        // when the visible set is unchanged.
        const stateUpdates = {};
        let stateChangedAny = false;
        let visibilityChangedAny = false;
        const trackChiplets
            = chipletModel && app.visibleChiplets instanceof Set;

        layerModel.forEach(node => {
            if (node.cb) {
                node.cb.checked = node.checked;
                node.cb.indeterminate = node.indeterminate;
            }
            if (node.data && node.data.layer) {
                const id = node.data.nodeId || node.data.name;
                allLayerIds.push(id);
                if (node.checked) {
                    node.data.layer._orShow();
                    app.visibleLayers.add(id);
                    app.visibleLayerNames.add(node.data.name);
                } else {
                    node.data.layer._orHide();
                    app.visibleLayers.delete(id);
                }
            }
            if (trackChiplets && node.data && node.data.chipletPath) {
                const path = node.data.chipletPath;
                const cn = chipletModel.get(path);
                if (cn) {
                    // Indeterminate parents (some descendant layers
                    // still visible) must count as visible — otherwise
                    // the backend filter would drop ALL their layers.
                    const want = node.checked || node.indeterminate;
                    const update = {
                        checked: node.checked,
                        indeterminate: node.indeterminate,
                    };
                    if (cn.checked !== node.checked
                        || cn.indeterminate !== node.indeterminate) {
                        stateUpdates[path] = update;
                        stateChangedAny = true;
                    }
                    if (app.visibleChiplets.has(path) !== want) {
                        visibilityChangedAny = true;
                    }
                }
            }
        });
        // One refresh per affected merged pane, after the whole walk: a group
        // toggle touches many nodes that share a pane.
        flushDirtyPanes();
        // Visibility off ⇒ selectability disabled — refresh selectability DOM.
        syncLayerSelDom();
        // Refresh pins layer so it filters by the updated visible_layers.
        if (app.pinsLayer && app.map.hasLayer(app.pinsLayer)) {
            app.pinsLayer.refreshTiles();
        }

        const hiddenNodes = allLayerIds.filter(n => !app.visibleLayers.has(n));
        try {
            window.sessionStorage.setItem(
                'or_hidden_layers', JSON.stringify(hiddenNodes));
        } catch (_) { /* ignore */ }
        // or_hidden_layers is part of the saved display state; the chiplet
        // mirror below only reaches the sync (via redrawAllLayers) when the
        // visible-chiplet set changes, so push explicitly (rAF-coalesced).
        app.syncDisplayState();

        // Mirror chiplet toggles into the Chiplets panel.  Toggling
        // wrapper_1 in the Layers tree must also remove its path from
        // app.visibleChiplets, otherwise other chiplets' tile requests
        // still pull wrapper_1's shapes from the backend (which iterates
        // every chiplet whose path is in visible_chiplets).
        // mirrorStates preserves indeterminate (unlike checkSet) so a
        // partially-selected chiplet keeps its tri-state checkbox AND
        // stays in visibleChiplets.  When only the tri-state changed
        // (without entering/leaving the visible set) we take the quiet
        // path so chipletModel.onChange — which redraws every Leaflet
        // tile — is skipped; we sync the DOM checkboxes by hand instead.
        if (visibilityChangedAny) {
            // Every visibility flip is also a tri-state change, so
            // stateUpdates already covers both — passing it through
            // mirrorStates fires the chiplet callback exactly once and
            // both the checkbox tri-state and visibleChiplets are kept
            // consistent.
            chipletModel.mirrorStates(stateUpdates);
        } else if (stateChangedAny) {
            // Tri-state changed without flipping the visible set — go
            // through the quiet path to skip redrawAllLayers, and sync
            // the DOM checkboxes ourselves since onChange is suppressed.
            chipletModel.mirrorStatesQuiet(stateUpdates);
            chipletModel.forEach(node => {
                if (node.cb) {
                    node.cb.checked = node.checked;
                    node.cb.indeterminate = node.indeterminate;
                }
            });
        }
    });
    
    app.layerModel = layerModel; // expose it so other rendering mechanism can use it
    layerModel.addFromSpec(layerSpec);

    // Parallel selectability model — picks gate on this set on the server.
    const layerSelModel = new CheckboxTreeModel(() => {
        // Rebuild from scratch so multi-chiplet/multi-tech subtrees that
        // share a layer name don't fall into last-writer-wins (an unchecked
        // M1 leaf in one subtree would otherwise delete the name even when
        // another M1 leaf is still checked).  Mutate in place — the
        // WebSocketTileLayer closure captured this Set by reference.
        app.selectableLayers.clear();
        layerSelModel.forEach(node => {
            if (node.checked && node.data && node.data.name) {
                app.selectableLayers.add(node.data.name);
            }
        });
        syncLayerSelDom();
        const nonSel
            = techData.layers.filter(n => !app.selectableLayers.has(n));
        try {
            window.sessionStorage.setItem(
                'or_nonselectable_layers', JSON.stringify(nonSel));
        } catch (_) { /* ignore */ }
        // Selectability changes the rendering of nothing, so no redraw runs;
        // push the saved display state explicitly.
        app.syncDisplayState();
    });
    layerSelModel.addFromSpec(layerSelSpec);

    // Sync layer selectability DOM: visibility off disables the sel checkbox.
    function syncLayerSelDom() {
        layerSelModel.forEach(node => {
            if (!node.selCb) return;
            node.selCb.checked = node.checked;
            node.selCb.indeterminate = node.indeterminate;
            const visNode = layerModel.get(node.id);
            const visOff
                = visNode && !visNode.checked && !visNode.indeterminate;
            node.selCb.disabled = visOff;
        });
    }

    // --- Layer context menu (right-click) ---
    const contextMenu = document.createElement('div');
    contextMenu.className = 'context-menu';
    contextMenu.style.display = 'none';
    document.body.appendChild(contextMenu);

    function showOnlyLayers(indices) {
        const updates = {};
        layerIds.forEach((id, i) => {
            if (id) updates[id] = indices.has(i);
        });
        layerModel.checkSet(updates);
    }

    function hideContextMenu() {
        contextMenu.style.display = 'none';
    }

    // Append a clickable row to the layer context menu.  onClick runs, then the
    // menu closes.  Shared by the visibility-range items and the fill-pattern
    // items so both stay in sync.
    function addContextMenuItem(label, onClick) {
        const div = document.createElement('div');
        div.className = 'context-menu-item';
        div.textContent = label;
        div.addEventListener('click', () => {
            onClick();
            hideContextMenu();
        });
        contextMenu.appendChild(div);
    }

    const n = leafletLayers.length;
    const menuItems = [
        { label: 'Show only this layer',  fn: (i) => layerRangeSet(i, 0, 0, n) },
        { label: 'Show layer range \u2195',   fn: (i) => layerRangeSet(i, 1, 1, n) },
        { label: 'Show layer range \u2195\u2195', fn: (i) => layerRangeSet(i, 2, 2, n) },
        { label: 'Show layer range \u2193',   fn: (i) => layerRangeSet(i, 1, 0, n) },
        { label: 'Show layer range \u2191',   fn: (i) => layerRangeSet(i, 0, 1, n) },
    ];

    document.addEventListener('click', (e) => {
        if (!contextMenu.contains(e.target)) hideContextMenu();
    });
    document.addEventListener('keydown', (e) => {
        if (e.key === 'Escape') hideContextMenu();
    });

    // --- Layer row selection ---
    //
    // Clicking a layer's name selects it and shows its properties in the
    // Inspector, as the Qt GUI does (DisplayControls::displayItemSelected
    // emits `selected(makeSelected(tech_layer))` when a row is clicked).
    //
    // Only the checkboxes toggle state — leaf rows are <div>s rather than
    // <label>s so that a click on the name, the indent spacer or the row's
    // padding does not activate the visibility checkbox, matching the Qt GUI
    // where the name column selects and the checkbox column toggles.
    //
    // Saved reports have no backend to answer select_layer, so their rows are
    // not selectable: the name is inert there and only the checkboxes work.
    const layerRowsSelectable = !isStaticMode(app);
    let selectedLayerRow = null;

    function clearSelectedLayerRow() {
        if (selectedLayerRow) {
            selectedLayerRow.classList.remove('vis-row-selected');
            selectedLayerRow = null;
        }
    }

    // Another panel taking the selection (a canvas click, Inspector
    // navigation, the fanout chart, ...) leaves this row's highlight claiming
    // a selection the server no longer holds, so drop it.
    if (layerRowsSelectable) {
        onSelectionReset(app, clearSelectedLayerRow);
    }

    function selectLayerRow(row, name, chipletPath) {
        // Take the selection before painting: beginSelection() runs the
        // resetters, which clears whichever row was highlighted before.
        const token = beginSelection(app);
        selectedLayerRow = row;
        row.classList.add('vis-row-selected');

        const msg = { type: 'select_layer', layer: name,
                      use_dbu: app.showDbu };
        if (chipletPath) msg.chiplet = chipletPath;
        app.websocketManager.request(msg).then(data => {
            // Clicking through several layers — or clicking a layer and then
            // selecting elsewhere — leaves one request in flight per click,
            // and they can land out of order.  A response that is no longer
            // the newest selection must not drive the Inspector.
            if (!isCurrentSelection(app, token)) {
                return;
            }
            if (app.updateInspector) {
                app.updateInspector(data);
            }
            if (app.focusComponent) {
                app.focusComponent('Inspector');
            }
            // The server replaced the selection set, so whatever highlight the
            // previous selection painted is stale.  A tech layer contributes
            // no shapes of its own, so only the overlay needs repainting —
            // not every tile.
            if (app.refreshOverlay) {
                app.refreshOverlay();
            }
        }).catch(err => {
            console.error('select_layer failed:', err);
            // The server did not take the selection, so drop the highlight
            // rather than leave the panel claiming a selection that is not
            // there.  Only if this click is still the newest one — a later
            // selection has already moved the highlight.
            if (isCurrentSelection(app, token)) {
                clearSelectedLayerRow();
            }
        });
    }

    function buildLayerDOM(node, isRoot = false) {
        const selNode = layerSelModel.get(node.id);
        if (!node.children || node.children.length === 0) {
            // Leaf node (layer).  Column order matches the Qt GUI: the name
            // stretches on the left, the visibility and selectability
            // checkboxes are pinned to the right under the header icons.
            //
            // A <div>, not a <label>: a <label> wrapping the visibility
            // checkbox activates it for a click anywhere in the row, so
            // clicking the name, the indent spacer or the row's padding
            // flipped the layer.  Only the checkboxes toggle state now.
            const label = document.createElement('div');
            label.className
                = layerRowsSelectable ? 'vis-leaf vis-leaf-selectable'
                                      : 'vis-leaf';

            const spacer = document.createElement('span');
            spacer.className = 'vis-arrow';
            spacer.style.visibility = 'hidden';
            spacer.textContent = '▶';
            label.appendChild(spacer);

            const index = node.data.colorIndex;
            const name = node.data.name;

            const nameSpan = makeNameSpan();
            const colorSwatch = document.createElement('span');
            colorSwatch.className = 'layer-color';
            const c = node.data.color || (techData.layer_colors && techData.layer_colors[index]) || fallbackLayerPalette[index % fallbackLayerPalette.length];
            colorSwatch.style.backgroundColor = `rgb(${c[0]},${c[1]},${c[2]})`;
            nameSpan.appendChild(colorSwatch);
            nameSpan.appendChild(document.createTextNode(name));
            label.appendChild(nameSpan);
            if (layerRowsSelectable) {
                nameSpan.addEventListener('click', () => {
                    selectLayerRow(label, name, node.data.ownerChipletPath);
                });
            }

            const checkbox = document.createElement('input');
            checkbox.type = 'checkbox';
            checkbox.className = 'vis-cb';
            checkbox.title = 'Visible';
            checkbox.checked = node.checked;
            node.cb = checkbox;
            checkbox.addEventListener('change', () => {
                layerModel.check(node.id, checkbox.checked);
            });
            label.appendChild(checkbox);

            if (selNode) {
                const selCheckbox = document.createElement('input');
                selCheckbox.type = 'checkbox';
                selCheckbox.className = 'vis-sel-cb';
                selCheckbox.title = 'Selectable';
                selCheckbox.checked = selNode.checked;
                selNode.selCb = selCheckbox;
                selCheckbox.addEventListener('change', () => {
                    layerSelModel.check(node.id, selCheckbox.checked);
                });
                label.appendChild(selCheckbox);
            } else {
                label.appendChild(makeSelSpacer());
            }

            // Setup context menu for layer
            label.addEventListener('contextmenu', (e) => {
                e.preventDefault();
                e.stopPropagation();
                contextMenu.innerHTML = '';
                for (const item of menuItems) {
                    addContextMenuItem(item.label,
                        () => showOnlyLayers(item.fn(index)));
                }

                // Fill-pattern submenu: choosing one re-renders only this
                // layer's tiles and persists the choice.
                const sep = document.createElement('div');
                sep.className = 'context-menu-separator';
                sep.style.borderTop = '1px solid #555';
                sep.style.margin = '4px 0';
                contextMenu.appendChild(sep);
                const current = app.layerPatterns[name] ?? 1;
                for (const opt of PATTERN_OPTIONS) {
                    const marker = current === opt.value ? '● ' : '   ';
                    addContextMenuItem(
                        marker + 'Fill: ' + opt.label,
                        () => setLayerPattern(name, node.data.layer, opt.value));
                }

                contextMenu.style.left = e.clientX + 'px';
                contextMenu.style.top = e.clientY + 'px';
                contextMenu.style.display = 'block';
            });

            return label;
        } else {
            // Group node (top or sub-instance)
            const group = document.createElement('div');
            group.className = 'vis-group';

            const { header, arrow, name } = makeGroupHeader();
            name.textContent
                = isRoot ? 'Layers' : (node.data.name || 'Group');

            const cb = document.createElement('input');
            cb.type = 'checkbox';
            cb.className = 'vis-cb';
            cb.title = 'Visible';
            cb.checked = node.checked;
            cb.indeterminate = node.indeterminate;
            node.cb = cb;
            cb.addEventListener('change', () => {
                layerModel.check(node.id, cb.checked);
            });
            header.appendChild(cb);

            if (selNode) {
                const selCb = document.createElement('input');
                selCb.type = 'checkbox';
                selCb.className = 'vis-sel-cb';
                selCb.title = 'Selectable';
                selCb.checked = selNode.checked;
                selCb.indeterminate = selNode.indeterminate;
                selNode.selCb = selCb;
                selCb.addEventListener('change', () => {
                    layerSelModel.check(node.id, selCb.checked);
                });
                header.appendChild(selCb);
            } else {
                header.appendChild(makeSelSpacer());
            }

            group.appendChild(header);

            const kids = document.createElement('div');
            kids.className = 'vis-group-children';
            
            // Build children recursively
            for (const child of node.children) {
                kids.appendChild(buildLayerDOM(child, false));
            }
            group.appendChild(kids);

            // Categories flagged startCollapsed (Implant/Other) open folded.
            attachGroupCollapse(header, arrow, kids,
                                !!(node.data && node.data.startCollapsed));

            return group;
        }
    }

    // Build layer DOM.
    const parentNode = layerModel.roots[0] || layerModel.get('layers_parent');
    const layerGroup = buildLayerDOM(parentNode, true);

    app.displayControlsEl.appendChild(layerGroup);

    // Initial selectability DOM sync (esp. disabled state for layers whose
    // visibility was restored as false).
    syncLayerSelDom();

    // --- Chiplets group (multi-die / 3D-IC visibility) ---
    //
    // Web-only feature: the Qt GUI has no equivalent panel today —
    // `gui::DisplayControls::setCurrentChip` only switches the active
    // chip, it does not toggle per-chiplet visibility.  Backend sends
    // one entry per dbChip / dbChipInst node with a unique `path`
    // ("top", "top.soc_inst", "top.soc_inst.sub_ip", …).  Toggling a
    // node refreshes every Leaflet tile so the server's chiplet
    // filter (`visible_chiplets`) takes effect on the next render.
    const chipletData = (techData && Array.isArray(techData.chiplets))
        ? techData.chiplets : [];
    if (chipletData.length > 1) {
        // Cookie schema: { "<block_name>": ["hidden.path1", "hidden.path2"] }.
        // Keying by top-block name keeps hidden state isolated per design —
        // opening design B no longer inherits design A's hides just because
        // both happen to expose a chiplet path like "top.soc_inst".
        // When block_name is empty (anonymous design) we skip persistence
        // entirely rather than collapse every nameless design into the
        // shared "" bucket.
        const blockKey = (techData && techData.block_name) || '';
        const persistHides = blockKey !== '';
        let cookieMap = {};
        if (persistHides) {
            try {
                const raw = getCookie('or_hidden_chiplets');
                if (raw) {
                    const parsed = JSON.parse(decodeURIComponent(raw));
                    if (parsed && typeof parsed === 'object'
                        && !Array.isArray(parsed)) {
                        cookieMap = parsed;
                    }
                }
            } catch (_) { /* ignore */ }
        }
        const savedHiddenChiplets = new Set(
            Array.isArray(cookieMap[blockKey]) ? cookieMap[blockKey] : []);

        // Build a flat node list keyed by path; CheckboxTreeModel will
        // wire parent/child relationships from `parent` strings.  Force
        // hasCheckbox=true on the root so its tri-state drives the
        // group-header checkbox below.  buildChipletFlatNodes itself
        // returns hasCheckbox=false for the root (its DOM is rendered
        // by the header, not by renderChipletNode).
        const flatNodes = buildChipletFlatNodes(chipletData,
                                                savedHiddenChiplets);
        const rootIdx = flatNodes.findIndex(n => n.parentId === null);
        const rootId = rootIdx >= 0 ? flatNodes[rootIdx].id : null;
        if (rootIdx >= 0) {
            flatNodes[rootIdx].hasCheckbox = true;
        }

        // Initialize visible set from the saved cookie state.
        app.visibleChiplets = new Set(
            chipletData
                .filter(c => !savedHiddenChiplets.has(c.path))
                .map(c => c.path));

        chipletModel = new CheckboxTreeModel(() => {
            // Sync DOM checkboxes and recompute the visibility set in
            // a single pass (renaming `node.cb` mirrors the layer
            // group's pattern).
            const newVisible = new Set();
            chipletModel.forEach(node => {
                if (node.cb) {
                    node.cb.checked = node.checked;
                    node.cb.indeterminate = node.indeterminate;
                }
                // Tri-state nodes (partial selection) still have at
                // least one descendant layer visible, so their path
                // must remain in visibleChiplets — coercing only
                // node.checked would let the backend filter hide the
                // entire chiplet subtree.
                if (node.data && node.data.path
                    && (node.checked || node.indeterminate)) {
                    newVisible.add(node.data.path);
                }
            });
            app.visibleChiplets = newVisible;

            // Persist hidden paths per design.  We re-read the cookie
            // here (rather than mutating a captured copy) so any state
            // saved by other tabs / designs in the meantime survives.
            // Anonymous designs (empty block_name) opt out of persistence
            // to avoid sharing a single "" bucket across distinct designs.
            if (persistHides) {
                // or_hidden_chiplets persists only FULLY-hidden chiplets.
                // Indeterminate ones stay in newVisible (their path keeps
                // the backend tile filter active) and are reconstructed
                // on reload from or_hidden_layers via _computeParent.
                const hidden = chipletData
                    .filter(c => !newVisible.has(c.path))
                    .map(c => c.path);
                let writeMap = {};
                try {
                    const raw = getCookie('or_hidden_chiplets');
                    if (raw) {
                        const parsed = JSON.parse(decodeURIComponent(raw));
                        if (parsed && typeof parsed === 'object'
                            && !Array.isArray(parsed)) {
                            writeMap = parsed;
                        }
                    }
                } catch (_) { /* ignore */ }
                if (hidden.length === 0) {
                    delete writeMap[blockKey];
                } else {
                    writeMap[blockKey] = hidden;
                }
                setCookie('or_hidden_chiplets',
                          encodeURIComponent(JSON.stringify(writeMap)));
            }

            // Refresh every Leaflet tile so the server applies the
            // updated `visible_chiplets` filter on the next request.
            redrawAllLayers();
        });
        chipletModel.buildFromNodes(flatNodes);

        // Sanity check (debug aid): the two trees are expected to
        // describe the same set of chiplet paths because they share
        // origin in tile_generator.cpp's serializeTechResponse.  If
        // that invariant ever breaks, mirrorStates / mirrorChipletToLayers
        // would silently drop the orphan paths — surfacing a warning
        // here makes the drift visible without breaking anything.
        if (typeof console !== 'undefined' && console.warn) {
            const layerPaths = new Set();
            layerModel.forEach(n => {
                if (n.data && n.data.chipletPath) {
                    layerPaths.add(n.data.chipletPath);
                }
            });
            const chipletPaths = new Set();
            chipletModel.forEach(n => {
                if (n.data && n.data.path) {
                    chipletPaths.add(n.data.path);
                }
            });
            for (const p of chipletPaths) {
                if (!layerPaths.has(p)) {
                    console.warn(
                        `[display-controls] chiplet "${p}" missing from `
                        + 'layer_hierarchy — Layers/Chiplets sync may drift');
                }
            }
            for (const p of layerPaths) {
                if (!chipletPaths.has(p)) {
                    console.warn(
                        `[display-controls] layer hierarchy node "${p}" `
                        + 'has no matching chiplet — Layers/Chiplets sync '
                        + 'may drift');
                }
            }
        }

        // Pre-cache chipletPath → layerModel node for O(1) lookups in
        // the chiplet-panel event handlers below.  layerModel is built
        // once via addFromSpec above and is not mutated structurally
        // afterwards (only checkbox states change), so the mapping
        // stays valid for the lifetime of these controls.
        const layerNodeByChipletPath = new Map();
        layerModel.forEach(n => {
            if (n.data && n.data.chipletPath) {
                layerNodeByChipletPath.set(n.data.chipletPath, n);
            }
        });

        // Cascade a chiplet toggle down into the layerModel so the
        // Layers panel checkboxes (and the Leaflet tiles, and the
        // or_hidden_layers cookie) follow.  cascadeQuiet skips the
        // layerModel's onChange; we fire it ourselves so the standard
        // sync path runs exactly once.  No loop with the chipletModel
        // mirror logic — the layerModel callback observes that the
        // chipletModel state already matches and emits nothing back.
        function mirrorChipletToLayers(chipletPath, checked) {
            const ln = layerNodeByChipletPath.get(chipletPath);
            if (!ln) return;
            layerModel.cascadeQuiet(ln.id, checked);
            layerModel.onChange();
        }

        // Initial sync: derive chiplet tri-state from the layerModel
        // (which already reflects or_hidden_layers via _computeParent).
        // Without this, reloading a session with partially-hidden layers
        // shows the Chiplets panel fully checked even though some of
        // the chiplet's layers are hidden, AND visibleChiplets may
        // contain paths whose every layer is hidden — feeding stale
        // entries to the backend filter.  Use the quiet variant so we
        // don't fire redrawAllLayers during boot; the checkboxes are
        // rendered just below and pick up the mutated state then.
        //
        // Chiplets the user explicitly hid via the Chiplets panel
        // (savedHiddenChiplets) are skipped: that cookie is the
        // authority at the chiplet level, regardless of whether the
        // layerModel still considers their layers visible.
        {
            const initial = {};
            layerModel.forEach(node => {
                if (!node.data || !node.data.chipletPath) return;
                const path = node.data.chipletPath;
                if (savedHiddenChiplets.has(path)) return;
                if (!chipletModel.get(path)) return;
                initial[path] = {
                    checked: node.checked,
                    indeterminate: node.indeterminate,
                };
            });
            if (Object.keys(initial).length > 0) {
                chipletModel.mirrorStatesQuiet(initial);
                // Refresh visibleChiplets from the freshly mirrored
                // tri-state so chiplets whose layers are all hidden
                // drop out of the filter set.
                const refreshed = new Set();
                chipletModel.forEach(node => {
                    if (node.data && node.data.path
                        && (node.checked || node.indeterminate)) {
                        refreshed.add(node.data.path);
                    }
                });
                app.visibleChiplets = refreshed;
            }
        }

        const chipletGroup = document.createElement('div');
        chipletGroup.className = 'vis-group';

        const { header: chipletHeader, arrow: chipletArrow,
                name: chipletName } = makeGroupHeader();
        chipletName.textContent = 'Chiplets';

        // Group-level checkbox: toggles every chiplet at once and
        // shows tri-state when the children disagree, matching the
        // Layers group's UX.
        const rootNode = rootId != null ? chipletModel.get(rootId) : null;
        if (rootNode) {
            const parentCb = document.createElement('input');
            parentCb.type = 'checkbox';
            parentCb.className = 'vis-cb';
            parentCb.checked = rootNode.checked;
            parentCb.indeterminate = rootNode.indeterminate;
            rootNode.cb = parentCb;
            parentCb.addEventListener('change', (e) => {
                e.stopPropagation();
                chipletModel.check(rootId, parentCb.checked);
                if (rootNode.data && rootNode.data.path) {
                    mirrorChipletToLayers(rootNode.data.path,
                                          parentCb.checked);
                }
            });
            chipletHeader.appendChild(parentCb);
        }
        // Chiplets have no selectability column; keep the layout aligned.
        chipletHeader.appendChild(makeSelSpacer());
        chipletGroup.appendChild(chipletHeader);

        const chipletChildren = document.createElement('div');
        chipletChildren.className = 'vis-group-children';

        function renderChipletNode(node) {
            const c = node.data;
            // The root is rendered by the header above; skip it here.
            if (node !== rootNode) {
                // A <div>, like the layer rows: only the checkbox toggles.
                const label = document.createElement('div');
                label.className = 'vis-leaf';
                label.style.paddingLeft = (8 * (c.depth - 1)) + 'px';
                label.title = c.path
                    + (c.master ? ` (${c.master})` : '');
                label.appendChild(makeNameSpan(c.name));
                const checkbox = document.createElement('input');
                checkbox.type = 'checkbox';
                checkbox.className = 'vis-cb';
                checkbox.checked = node.checked;
                checkbox.indeterminate = node.indeterminate;
                node.cb = checkbox;
                checkbox.addEventListener('change', () => {
                    chipletModel.check(node.id, checkbox.checked);
                    mirrorChipletToLayers(c.path, checkbox.checked);
                });
                label.appendChild(checkbox);
                label.appendChild(makeSelSpacer());
                chipletChildren.appendChild(label);
            }
            if (node.children) {
                node.children.forEach(renderChipletNode);
            }
        }
        chipletModel.roots.forEach(renderChipletNode);
        chipletGroup.appendChild(chipletChildren);

        attachGroupCollapse(chipletHeader, chipletArrow, chipletChildren,
                            false);

        app.displayControlsEl.appendChild(chipletGroup);
    } else {
        // Single-chip designs render every chiplet (i.e. only the top).
        // Clearing keeps WebSocketTileLayer's serializer from sending an
        // empty array and accidentally enabling the filter.
        app.visibleChiplets = null;
    }

    // --- Visibility tree (ordered to match Qt GUI display controls) ---
    // Subtrees that opt into a second "selectable" checkbox column mirror
    // the Qt GUI's selectability column (see displayControls.cpp).
    const visTree = new VisTree(visibility, selectability, redrawAllLayers);
    visTree.add({ label: 'Nets', addSelectable: true, children: [
        { key: 'net_signal', label: 'Signal' },
        { key: 'net_power', label: 'Power' },
        { key: 'net_ground', label: 'Ground' },
        { key: 'net_clock', label: 'Clock' },
        { key: 'net_reset', label: 'Reset' },
        { key: 'net_tieoff', label: 'Tie off' },
        { key: 'net_scan', label: 'Scan' },
        { key: 'net_analog', label: 'Analog' },
    ]});
    visTree.add({ label: 'Instances', addSelectable: true, children: [
        { label: 'Std Cells', visKey: 'stdcells', disabled: !app.hasLiberty, children: [
            { label: 'Bufs/Invs', children: [
                { key: 'std_bufinv_timing', label: 'Timing opt.' },
                { key: 'std_bufinv', label: 'Netlist' },
            ]},
            { key: 'std_combinational', label: 'Combinational' },
            { key: 'std_sequential', label: 'Sequential' },
            { label: 'Clock tree', children: [
                { key: 'std_clock_bufinv', label: 'Buffer/Inverter' },
                { key: 'std_clock_gate', label: 'Clock gate' },
            ]},
            { key: 'std_level_shift', label: 'Level shifter' },
        ]},
        { key: 'macros', label: 'Macros' },
        { label: 'Pads', children: [
            { key: 'pad_input', label: 'Input' },
            { key: 'pad_output', label: 'Output' },
            { key: 'pad_inout', label: 'Inout' },
            { key: 'pad_power', label: 'Power' },
            { key: 'pad_spacer', label: 'Spacer' },
            { key: 'pad_areaio', label: 'Area IO' },
            { key: 'pad_other', label: 'Other' },
        ]},
        { label: 'Physical', children: [
            { key: 'phys_fill', label: 'Fill' },
            { key: 'phys_endcap', label: 'Endcap' },
            { key: 'phys_welltap', label: 'Welltap' },
            { key: 'phys_tie', label: 'Tie Hi/Lo' },
            { key: 'phys_antenna', label: 'Antenna' },
            { key: 'phys_cover', label: 'Cover' },
            { key: 'phys_bump', label: 'Bump' },
            { key: 'phys_other', label: 'Other' },
        ]},
    ]});
    visTree.add({ label: 'Blockages', addSelectable: true, children: [
        { key: 'placement_blockages', label: 'Placement' },
        { key: 'routing_obstructions', label: 'Routing' },
    ]});
    if (techData.sites && techData.sites.length > 0) {
        visTree.add({ label: 'Rows', visKey: 'rows', addSelectable: true,
            children: techData.sites.map(name => ({
                key: 'site_' + name, label: name,
            })),
        });
    }
    visTree.add({ label: 'Tracks', children: [
        { key: 'tracks_pref', label: 'Pref' },
        { key: 'tracks_non_pref', label: 'Non Pref' },
    ]});
    visTree.add({ label: 'Shapes', children: [
        { label: 'Routing', visKey: 'routing', children: [
            { key: 'routing_segments', label: 'Segments' },
            { key: 'routing_vias', label: 'Vias' },
        ]},
        { label: 'Special Routing', visKey: 'special_nets', children: [
            { key: 'srouting_segments', label: 'Segments' },
            { key: 'srouting_vias', label: 'Vias' },
        ]},
        { key: 'pins', label: 'Pins', selectable: true },
        { key: 'pin_names', label: 'Pin Names', disabledBy: 'pins' },
        { key: 'fills', label: 'Fills' },
    ]});
    visTree.add({ label: 'Misc', children: [
        { label: 'Instances', children: [
            { key: 'inst_names', label: 'Names' },
            { key: 'inst_pins', label: 'Pins', selectable: true },
            { key: 'inst_pin_names', label: 'Pin Names', disabledBy: 'inst_pins' },
            { key: 'blockages', label: 'Blockages' },
        ]},
        { key: 'detailed', label: 'Detailed view' },
        { key: 'rulers', label: 'Rulers' },
        { key: 'labels', label: 'Labels' },
        { key: 'scale_bar', label: 'Scale bar' },
        { key: 'access_points', label: 'Access Points' },
        { key: 'regions', label: 'Regions' },
        { key: 'mfg_grid', label: 'Manufacturing grid' },
        { key: 'gcell_grid', label: 'GCell grid' },
        { key: 'flywires_only', label: 'Flywires only' },
        { key: 'focused_nets_guides', label: 'Focused nets guides' },
        { key: 'highlight_selected', label: 'Highlight selected' },
    ]});
    visTree.add({ key: 'module_view', label: 'Module view' });
    // Developer overlays.  All three are plain leaves under a visKey-less
    // group: giving the group `visKey: 'debug_renderers'` would tie the
    // renderer overlay to the group's tri-state, so ticking the unrelated
    // "Tiles" row would switch the renderers on via the indeterminate state.
    visTree.add({ label: 'Debug Graphics', children: [
        // Renderer overlay: calls drawObjects() on registered renderers
        // (e.g. gpl::GraphicsImpl when global_placement_debug is enabled).
        { key: 'debug_renderers', label: 'Renderers' },
        { key: 'debug_live', label: 'Live (don\'t require pause)',
          disabledBy: 'debug_renderers' },
        { key: 'debug', label: 'Tiles' },
    ]});
    visTree.render(app.displayControlsEl);

    // Background color control (Qt GUI "Background" parity): a swatch that
    // opens the native color picker + a reset-to-theme link.  The layout
    // background is the CSS var --bg-map on the Leaflet container, so this
    // is purely client-side (tiles are transparent).
    const bgRow = document.createElement('div');
    bgRow.className = 'bg-color-row';
    const bgLabel = document.createElement('span');
    bgLabel.textContent = 'Background';
    const bgInput = document.createElement('input');
    bgInput.type = 'color';
    bgInput.className = 'bg-color-input';
    bgInput.title = 'Layout background color';
    const savedBg = getCookie('or_bg_color');
    bgInput.value = isValidHexColor(savedBg) ? savedBg : getThemeDefaultBgColor();
    // 'input' fires on every tick while dragging inside the picker: keep it
    // to the cheap CSS-var preview.  Persistence (cookie), the 3D-viewer
    // re-render and the server sync run once, on 'change' (picker closed).
    bgInput.addEventListener('input', () => {
        document.documentElement.style.setProperty('--bg-map', bgInput.value);
    });
    bgInput.addEventListener('change', () => {
        setBackgroundColor(bgInput.value, app);
    });
    const bgReset = document.createElement('button');
    bgReset.className = 'bg-color-reset';
    bgReset.textContent = 'Reset';
    bgReset.title = 'Reset background to the theme default';
    bgReset.addEventListener('click', () => {
        // resetBackgroundColor removes the inline --bg-map override, so
        // the default read afterwards is the theme's own value.
        resetBackgroundColor(app);
        bgInput.value = getThemeDefaultBgColor();
    });
    bgRow.appendChild(bgLabel);
    bgRow.appendChild(bgInput);
    bgRow.appendChild(bgReset);
    app.displayControlsEl.appendChild(bgRow);

    if (!app.heatMapLayer) {
        app.heatMapLayer = new HeatMapTileLayer(app.websocketManager, app, {
            zIndex: leafletLayers.length + 10,
            opacity: 1,
        });
    }

    const heatMapGroup = document.createElement('div');
    heatMapGroup.className = 'vis-group heatmap-controls';
    app.displayControlsEl.appendChild(heatMapGroup);

    const { header: heatMapHeader, arrow: heatMapArrow,
            name: heatMapName }
        = makeGroupHeader('vis-group-header heatmap-header');
    heatMapName.textContent = 'Heat Maps';
    heatMapGroup.appendChild(heatMapHeader);

    const heatMapContainer = document.createElement('div');
    heatMapContainer.className = 'vis-group-children heatmap-group-children';
    heatMapGroup.appendChild(heatMapContainer);

    attachGroupCollapse(heatMapHeader, heatMapArrow, heatMapContainer, true);

    function addCheckbox(parent, label, checked, onChange) {
        const row = document.createElement('label');
        row.className = 'heatmap-setting';
        const input = document.createElement('input');
        input.type = 'checkbox';
        input.checked = checked;
        input.addEventListener('change', () => onChange(input.checked));
        row.appendChild(input);
        row.appendChild(document.createTextNode(label));
        parent.appendChild(row);
    }

    function addNumber(parent, label, value, step, onChange) {
        const row = document.createElement('label');
        row.className = 'heatmap-setting';
        const text = document.createElement('span');
        text.textContent = label;
        const input = document.createElement('input');
        input.type = 'number';
        input.value = String(value);
        input.step = String(step || 1);
        input.addEventListener('change', () => onChange(parseFloat(input.value)));
        row.appendChild(text);
        row.appendChild(input);
        parent.appendChild(row);
    }

    function addSelect(parent, label, value, choices, onChange) {
        const row = document.createElement('label');
        row.className = 'heatmap-setting';
        const text = document.createElement('span');
        text.textContent = label;
        const select = document.createElement('select');
        for (const choice of choices) {
            const option = document.createElement('option');
            option.value = choice;
            option.textContent = choice;
            if (choice === value) option.selected = true;
            select.appendChild(option);
        }
        select.addEventListener('change', () => onChange(select.value));
        row.appendChild(text);
        row.appendChild(select);
        parent.appendChild(row);
    }

    function sendHeatMapUpdate(message) {
        app.websocketManager.request(message).then(data => {
            if (app.updateHeatMaps) {
                app.updateHeatMaps(data);
            }
        }).catch(err => console.error('Heat map update failed', err));
    }

    function renderMapLegend(active) {
        if (!app.heatMapLegendEl) {
            return;
        }

        const legend = app.heatMapLegendEl;
        legend.innerHTML = '';

        if (!active || !active.show_legend || !active.legend || active.legend.length === 0) {
            legend.classList.add('hidden');
            return;
        }

        const title = document.createElement('div');
        title.className = 'heatmap-map-legend-title';
        title.textContent = active.title;
        legend.appendChild(title);

        const units = document.createElement('div');
        units.className = 'heatmap-map-legend-units';
        units.textContent = active.units || '';
        legend.appendChild(units);

        const list = document.createElement('div');
        list.className = 'heatmap-map-legend-list';
        for (const entry of active.legend) {
            const row = document.createElement('div');
            row.className = 'heatmap-legend-row';

            const swatch = document.createElement('span');
            swatch.className = 'heatmap-legend-swatch';
            swatch.style.backgroundColor
                = `rgba(${entry.color[0]}, ${entry.color[1]}, ${entry.color[2]}, ${entry.color[3] / 255})`;

            const text = document.createElement('span');
            text.textContent = entry.value;

            row.appendChild(swatch);
            row.appendChild(text);
            list.appendChild(row);
        }

        legend.appendChild(list);
        legend.classList.remove('hidden');
    }

    app.renderHeatMapControls = (data) => {
        heatMapContainer.innerHTML = '';

        const list = document.createElement('div');
        list.className = 'heatmap-list';
        heatMapContainer.appendChild(list);

        const noneLabel = document.createElement('label');
        noneLabel.className = 'heatmap-setting';
        const noneInput = document.createElement('input');
        noneInput.type = 'radio';
        noneInput.name = 'active-heatmap';
        noneInput.checked = !data.active;
        noneInput.addEventListener('change', () => {
            sendHeatMapUpdate({ type: 'set_active_heatmap', name: '' });
        });
        noneLabel.appendChild(noneInput);
        noneLabel.appendChild(document.createTextNode('Off'));
        list.appendChild(noneLabel);

        for (const heatMap of data.heatmaps || []) {
            const label = document.createElement('label');
            label.className = 'heatmap-setting';
            const input = document.createElement('input');
            input.type = 'radio';
            input.name = 'active-heatmap';
            input.checked = heatMap.name === data.active;
            input.addEventListener('change', () => {
                sendHeatMapUpdate({
                    type: 'set_active_heatmap',
                    name: heatMap.name,
                });
            });
            label.appendChild(input);
            label.appendChild(document.createTextNode(heatMap.title));
            list.appendChild(label);
        }

        const active = (data.heatmaps || []).find(h => h.name === data.active);
        renderMapLegend(active);
        if (!active) {
            return;
        }

        const settings = document.createElement('div');
        settings.className = 'heatmap-settings';
        heatMapContainer.appendChild(settings);

        addNumber(settings, 'Display min', active.display_min,
                  active.display_range_increment, value => {
                      sendHeatMapUpdate({
                          type: 'set_heatmap',
                          name: active.name,
                          option: 'DisplayMin',
                          value,
                      });
                  });
        addNumber(settings, 'Display max', active.display_max,
                  active.display_range_increment, value => {
                      sendHeatMapUpdate({
                          type: 'set_heatmap',
                          name: active.name,
                          option: 'DisplayMax',
                          value,
                      });
                  });
        addCheckbox(settings, 'Show below min', active.draw_below_min, value => {
            sendHeatMapUpdate({
                type: 'set_heatmap',
                name: active.name,
                option: 'ShowMin',
                value,
            });
        });
        addCheckbox(settings, 'Show above max', active.draw_above_max, value => {
            sendHeatMapUpdate({
                type: 'set_heatmap',
                name: active.name,
                option: 'ShowMax',
                value,
            });
        });
        addCheckbox(settings, 'Log scale', active.log_scale, value => {
            sendHeatMapUpdate({
                type: 'set_heatmap',
                name: active.name,
                option: 'LogScale',
                value,
            });
        });
        addCheckbox(settings, 'Reverse log', active.reverse_log, value => {
            sendHeatMapUpdate({
                type: 'set_heatmap',
                name: active.name,
                option: 'ReverseLog',
                value,
            });
        });
        if (active.can_adjust_grid) {
            addNumber(settings, 'Grid X', active.grid_x, 0.1, value => {
                sendHeatMapUpdate({
                    type: 'set_heatmap',
                    name: active.name,
                    option: 'GridX',
                    value,
                });
            });
            addNumber(settings, 'Grid Y', active.grid_y, 0.1, value => {
                sendHeatMapUpdate({
                    type: 'set_heatmap',
                    name: active.name,
                    option: 'GridY',
                    value,
                });
            });
        }
        addNumber(settings, 'Alpha', active.alpha, 1, value => {
            sendHeatMapUpdate({
                type: 'set_heatmap',
                name: active.name,
                option: 'Alpha',
                value,
            });
        });
        addCheckbox(settings, 'Legend', active.show_legend, value => {
            sendHeatMapUpdate({
                type: 'set_heatmap',
                name: active.name,
                option: 'ShowLegend',
                value,
            });
        });
        if (active.supports_numbers) {
            addCheckbox(settings, 'Show numbers', active.show_numbers, value => {
                sendHeatMapUpdate({
                    type: 'set_heatmap',
                    name: active.name,
                    option: 'ShowNumbers',
                    value,
                });
            });
        }

        for (const option of active.options || []) {
            if (option.type === 'bool') {
                addCheckbox(settings, option.label, option.value, value => {
                    sendHeatMapUpdate({
                        type: 'set_heatmap',
                        name: active.name,
                        option: option.name,
                        value,
                    });
                });
            } else if (option.type === 'choice') {
                addSelect(settings, option.label, option.value,
                          option.choices || [], value => {
                              sendHeatMapUpdate({
                                  type: 'set_heatmap',
                                  name: active.name,
                                  option: option.name,
                                  value,
                              });
                          });
            }
        }

        const rebuild = document.createElement('button');
        rebuild.className = 'heatmap-rebuild';
        rebuild.textContent = 'Rebuild data';
        rebuild.addEventListener('click', () => {
            sendHeatMapUpdate({
                type: 'set_heatmap',
                name: active.name,
                option: 'rebuild',
                value: 1,
            });
        });
        settings.appendChild(rebuild);
    };
}
