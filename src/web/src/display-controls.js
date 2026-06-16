// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Display controls — layer checkboxes and visibility tree.

import { CheckboxTreeModel } from './checkbox-tree-model.js';
import { VisTree } from './vis-tree.js';
import { getCookie, setCookie } from './theme.js';

// Compute a Set of layer indices around `center` within [0, count).
// `lower` layers below and `upper` layers above are included.
export function layerRangeSet(center, lower, upper, count) {
    const indices = new Set();
    const lo = Math.max(0, center - lower);
    const hi = Math.min(count - 1, center + upper);
    for (let i = lo; i <= hi; i++) indices.add(i);
    return indices;
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

// Populate display controls with layer checkboxes and visibility tree.
export function populateDisplayControls(app, visibility, selectability,
                                         WebSocketTileLayer,
                                         techData, redrawAllLayers,
                                         HeatMapTileLayer) {
    if (!app.displayControlsEl) return;
    app.displayControlsEl.innerHTML = '';
    app.allLayers = [];

    // Instance borders layer (always below routing layers)
    const instancesLayer = new WebSocketTileLayer(app.websocketManager, '_instances', {
        zIndex: 0,
    });
    instancesLayer.addTo(app.map);
    app.allLayers.push(instancesLayer);

    // IO pin markers layer (between instances and routing layers)
    const pinsLayer = new WebSocketTileLayer(app.websocketManager, '_pins', {
        zIndex: 1,
    });
    pinsLayer.addTo(app.map);
    app.pinsLayer = pinsLayer;
    app.allLayers.push(pinsLayer);

    // Module coloring overlay layer (between instances and routing layers)
    const modulesLayer = new WebSocketTileLayer(app.websocketManager, '_modules', {
        zIndex: 2,
    });
    // Don't add to map until "Module view" is enabled
    app.modulesLayer = modulesLayer;
    app.allLayers.push(modulesLayer);

    // --- Layers group (using CheckboxTreeModel) ---

    // Create Leaflet layers and build a model spec.
    const leafletLayers = [];  // index → WebSocketTileLayer
    const layerIds = [];       // index → model node id

    // Forward declaration so the layerModel callback can mirror chiplet
    // toggles into the Chiplets panel (created later when chipletData
    // has more than one entry).
    let chipletModel = null;

    // Restore saved hidden-layers and non-selectable-layers sets.
    let savedHiddenLayers = new Set();
    let savedNonSelectableLayers = new Set();
    try {
        const raw = getCookie('or_hidden_layers');
        if (raw) savedHiddenLayers = new Set(JSON.parse(decodeURIComponent(raw)));
    } catch (_) { /* ignore */ }
    try {
        const raw = getCookie('or_nonselectable_layers');
        if (raw) {
            savedNonSelectableLayers
                = new Set(JSON.parse(decodeURIComponent(raw)));
        }
    } catch (_) { /* ignore */ }

    // Global counter so each layer (across the whole hierarchy) gets a unique
    // z-index and palette slot regardless of which chiplet it belongs to.
    let nextLayerSlot = 0;

    function buildLayerSpec(hierarchyNode, parentId = 'layers') {
        const children = [];

        if (hierarchyNode.instances && hierarchyNode.instances.length > 0) {
            hierarchyNode.instances.forEach((inst, idx) => {
                const instId = parentId + "/" + (inst.name || idx);
                children.push(buildLayerSpec(inst, instId));
            });
        }

        if (hierarchyNode.layers && hierarchyNode.layers.length > 0) {
            hierarchyNode.layers.forEach((layerObj) => {
                const name = layerObj.name || layerObj;
                const slot = nextLayerSlot++;
                const zIndex = slot + 3;
                const layer = new WebSocketTileLayer(app.websocketManager, name, {
                    opacity: 0.7,
                    zIndex: zIndex,
                });

                const id = `${parentId}/${name}`;
                layer._nodeId = id;

                const visible = !savedHiddenLayers.has(id);
                if (visible) {
                    layer.addTo(app.map);
                    app.visibleLayers.add(id);
                    app.visibleLayerNames.add(name);
                }
                app.allLayers.push(layer);
                leafletLayers.push(layer);

                layerIds.push(id);
                children.push({
                    id,
                    data: { name, layer, color: layerObj.color, colorIndex: slot, nodeId: id },
                    checked: visible,
                });
            });
        }

        const nodeData = { name: hierarchyNode.name, isInstance: true };
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
                const layer = new WebSocketTileLayer(app.websocketManager, name, {
                    opacity: 0.7,
                    zIndex: index + 3,
                });

                const id = `layers_parent/${name}`;
                layer._nodeId = id;
                
                const visible = !savedHiddenLayers.has(id);
                if (visible) {
                    layer.addTo(app.map);
                    app.visibleLayers.add(id);
                    app.visibleLayerNames.add(name);
                }
                app.allLayers.push(layer);
                leafletLayers.push(layer);

                layerIds.push(id);
                return { id, data: { name, layer, colorIndex: index, nodeId: id }, checked: visible };
            }),
        };
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
                    node.data.layer.addTo(app.map);
                    app.visibleLayers.add(id);
                    app.visibleLayerNames.add(node.data.name);
                } else {
                    app.map.removeLayer(node.data.layer);
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
        // Visibility off ⇒ selectability disabled — refresh selectability DOM.
        syncLayerSelDom();
        // Refresh pins layer so it filters by the updated visible_layers.
        if (app.pinsLayer && app.map.hasLayer(app.pinsLayer)) {
            app.pinsLayer.refreshTiles();
        }

        const hiddenNodes = allLayerIds.filter(n => !app.visibleLayers.has(n));
        setCookie('or_hidden_layers',
                  encodeURIComponent(JSON.stringify(hiddenNodes)));

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
        setCookie('or_nonselectable_layers',
                  encodeURIComponent(JSON.stringify(nonSel)));
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

    function buildLayerDOM(node, isRoot = false) {
        const selNode = layerSelModel.get(node.id);
        if (!node.children || node.children.length === 0) {
            // Leaf node (layer)
            const label = document.createElement('label');

            const checkbox = document.createElement('input');
            checkbox.type = 'checkbox';
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
            }

            const index = node.data.colorIndex;
            const name = node.data.name;

            const colorSwatch = document.createElement('span');
            colorSwatch.className = 'layer-color';
            const c = node.data.color || (techData.layer_colors && techData.layer_colors[index]) || fallbackLayerPalette[index % fallbackLayerPalette.length];
            colorSwatch.style.backgroundColor = `rgb(${c[0]},${c[1]},${c[2]})`;
            label.appendChild(colorSwatch);

            label.appendChild(document.createTextNode(name));

            // Setup context menu for layer
            label.addEventListener('contextmenu', (e) => {
                e.preventDefault();
                e.stopPropagation();
                contextMenu.innerHTML = '';
                for (const item of menuItems) {
                    const div = document.createElement('div');
                    div.className = 'context-menu-item';
                    div.textContent = item.label;
                    div.addEventListener('click', () => {
                        showOnlyLayers(item.fn(index));
                        hideContextMenu();
                    });
                    contextMenu.appendChild(div);
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

            const header = document.createElement('label');
            header.className = 'vis-group-header';
            
            const arrow = document.createElement('span');
            arrow.className = 'vis-arrow';
            arrow.textContent = '▼';
            header.appendChild(arrow);

            const cb = document.createElement('input');
            cb.type = 'checkbox';
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
            }

            const name = isRoot ? 'Layers' : (node.data.name || 'Group');
            header.appendChild(document.createTextNode(name));
            group.appendChild(header);

            const kids = document.createElement('div');
            kids.className = 'vis-group-children';
            
            // Build children recursively
            for (const child of node.children) {
                kids.appendChild(buildLayerDOM(child, false));
            }
            group.appendChild(kids);

            // Toggle collapse
            arrow.addEventListener('click', (e) => {
                e.preventDefault();
                e.stopPropagation();
                const collapsed = kids.classList.toggle('collapsed');
                arrow.textContent = collapsed ? '▶' : '▼';
            });

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

        const chipletHeader = document.createElement('label');
        chipletHeader.className = 'vis-group-header';
        const chipletArrow = document.createElement('span');
        chipletArrow.className = 'vis-arrow';
        chipletArrow.textContent = '▼';
        chipletHeader.appendChild(chipletArrow);

        // Group-level checkbox: toggles every chiplet at once and
        // shows tri-state when the children disagree, matching the
        // Layers group's UX.
        const rootNode = rootId != null ? chipletModel.get(rootId) : null;
        if (rootNode) {
            const parentCb = document.createElement('input');
            parentCb.type = 'checkbox';
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
        chipletHeader.appendChild(document.createTextNode('Chiplets'));
        chipletGroup.appendChild(chipletHeader);

        const chipletChildren = document.createElement('div');
        chipletChildren.className = 'vis-group-children';

        function renderChipletNode(node) {
            const c = node.data;
            // The root is rendered by the header above; skip it here.
            if (node !== rootNode) {
                const label = document.createElement('label');
                label.style.paddingLeft = (8 * (c.depth - 1)) + 'px';
                label.title = c.path
                    + (c.master ? ` (${c.master})` : '');
                const checkbox = document.createElement('input');
                checkbox.type = 'checkbox';
                checkbox.checked = node.checked;
                checkbox.indeterminate = node.indeterminate;
                node.cb = checkbox;
                checkbox.addEventListener('change', () => {
                    chipletModel.check(node.id, checkbox.checked);
                    mirrorChipletToLayers(c.path, checkbox.checked);
                });
                label.appendChild(checkbox);
                label.appendChild(document.createTextNode(c.name));
                chipletChildren.appendChild(label);
            }
            if (node.children) {
                node.children.forEach(renderChipletNode);
            }
        }
        chipletModel.roots.forEach(renderChipletNode);
        chipletGroup.appendChild(chipletChildren);

        chipletArrow.addEventListener('click', (e) => {
            e.preventDefault();
            e.stopPropagation();
            const collapsed = chipletChildren.classList.toggle('collapsed');
            chipletArrow.textContent = collapsed ? '▶' : '▼';
        });

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
    ]});
    visTree.add({ label: 'Misc', children: [
        { label: 'Instances', children: [
            { key: 'inst_names', label: 'Names' },
            { key: 'inst_pins', label: 'Pins', selectable: true },
            { key: 'inst_pin_names', label: 'Pin Names', disabledBy: 'inst_pins' },
            { key: 'blockages', label: 'Blockages' },
        ]},
        { key: 'rulers', label: 'Rulers' },
        { key: 'scale_bar', label: 'Scale bar' },
    ]});
    visTree.add({ key: 'module_view', label: 'Module view' });
    visTree.add({ key: 'debug', label: 'Debug tiles' });
    // Debug graphics overlay: calls drawObjects() on registered renderers
    // (e.g. gpl::GraphicsImpl when global_placement_debug is enabled).
    visTree.add({ label: 'Debug Graphics', visKey: 'debug_renderers',
        children: [
            { key: 'debug_live', label: 'Live (don\'t require pause)' },
        ],
    });
    visTree.render(app.displayControlsEl);

    if (!app.heatMapLayer) {
        app.heatMapLayer = new HeatMapTileLayer(app.websocketManager, app, {
            zIndex: leafletLayers.length + 10,
            opacity: 1,
        });
    }

    const heatMapGroup = document.createElement('div');
    heatMapGroup.className = 'vis-group heatmap-controls';
    app.displayControlsEl.appendChild(heatMapGroup);

    const heatMapHeader = document.createElement('label');
    heatMapHeader.className = 'vis-group-header heatmap-header';
    const heatMapArrow = document.createElement('span');
    heatMapArrow.className = 'vis-arrow';
    heatMapArrow.textContent = '▼';
    heatMapHeader.appendChild(heatMapArrow);
    heatMapHeader.appendChild(document.createTextNode('Heat Maps'));
    heatMapGroup.appendChild(heatMapHeader);

    const heatMapContainer = document.createElement('div');
    heatMapContainer.className = 'vis-group-children heatmap-group-children collapsed';
    heatMapGroup.appendChild(heatMapContainer);

    let heatMapCollapsed = true;
    heatMapArrow.addEventListener('click', (e) => {
        e.preventDefault();
        e.stopPropagation();
        heatMapCollapsed = !heatMapCollapsed;
        heatMapContainer.classList.toggle('collapsed', heatMapCollapsed);
        heatMapArrow.textContent = heatMapCollapsed ? '▶' : '▼';
    });

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
