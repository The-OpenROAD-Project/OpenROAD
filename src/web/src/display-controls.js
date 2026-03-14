// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Display controls — layer checkboxes and visibility tree.

import { CheckboxTreeModel } from './checkbox-tree-model.js';
import { VisTree } from './vis-tree.js';

// Compute a Set of layer indices around `center` within [0, count).
// `lower` layers below and `upper` layers above are included.
export function layerRangeSet(center, lower, upper, count) {
    const indices = new Set();
    const lo = Math.max(0, center - lower);
    const hi = Math.min(count - 1, center + upper);
    for (let i = lo; i <= hi; i++) indices.add(i);
    return indices;
}

// Layer color palette (must match server-side palette in web.cpp)
const layerPalette = [
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
export function populateDisplayControls(app, visibility, WebSocketTileLayer,
                                         techData, redrawAllLayers) {
    if (!app.displayControlsEl) return;
    app.displayControlsEl.innerHTML = '';
    app.allLayers = [];

    // Instance borders layer (always below routing layers)
    const instancesLayer = new WebSocketTileLayer(app.websocketManager, '_instances', {
        zIndex: 0,
    });
    instancesLayer.addTo(app.map);
    app.allLayers.push(instancesLayer);

    // Module coloring overlay layer (between instances and routing layers)
    const modulesLayer = new WebSocketTileLayer(app.websocketManager, '_modules', {
        zIndex: 1,
    });
    // Don't add to map until "Module view" is enabled
    app.modulesLayer = modulesLayer;
    app.allLayers.push(modulesLayer);

    // --- Layers group (using CheckboxTreeModel) ---

    // Create Leaflet layers and build a model spec.
    const leafletLayers = [];  // index → WebSocketTileLayer
    const layerIds = [];       // index → model node id

    const layerSpec = {
        id: 'layers_parent',
        children: techData.layers.map((name, index) => {
            const layer = new WebSocketTileLayer(app.websocketManager, name, {
                opacity: 0.7,
                zIndex: index + 2,
            });
            layer.addTo(app.map);
            app.allLayers.push(layer);
            leafletLayers.push(layer);

            const id = `layer_${index}`;
            layerIds.push(id);
            return { id, data: { name, layer, colorIndex: index }, checked: true };
        }),
    };

    const layerModel = new CheckboxTreeModel(() => {
        // Sync DOM and Leaflet layer visibility from model.
        layerModel.forEach(node => {
            if (node.cb) {
                node.cb.checked = node.checked;
                node.cb.indeterminate = node.indeterminate;
            }
            if (node.data && node.data.layer) {
                if (node.checked) {
                    node.data.layer.addTo(app.map);
                } else {
                    app.map.removeLayer(node.data.layer);
                }
            }
        });
    });
    layerModel.addFromSpec(layerSpec);

    // Build layer DOM.
    const layerGroup = document.createElement('div');
    layerGroup.className = 'vis-group';

    const layerHeader = document.createElement('label');
    layerHeader.className = 'vis-group-header';
    const layerArrow = document.createElement('span');
    layerArrow.className = 'vis-arrow';
    layerArrow.textContent = '▼';
    layerHeader.appendChild(layerArrow);

    const parentNode = layerModel.get('layers_parent');
    const parentCb = document.createElement('input');
    parentCb.type = 'checkbox';
    parentCb.checked = true;
    parentNode.cb = parentCb;
    parentCb.addEventListener('change', () => {
        layerModel.check('layers_parent', parentCb.checked);
    });
    layerHeader.appendChild(parentCb);
    layerHeader.appendChild(document.createTextNode('Layers'));
    layerGroup.appendChild(layerHeader);

    const layerChildren = document.createElement('div');
    layerChildren.className = 'vis-group-children';

    techData.layers.forEach((name, index) => {
        const id = layerIds[index];
        const modelNode = layerModel.get(id);

        const label = document.createElement('label');

        const checkbox = document.createElement('input');
        checkbox.type = 'checkbox';
        checkbox.checked = true;
        modelNode.cb = checkbox;
        checkbox.addEventListener('change', () => {
            layerModel.check(id, checkbox.checked);
        });
        label.appendChild(checkbox);

        const colorSwatch = document.createElement('span');
        colorSwatch.className = 'layer-color';
        const c = layerPalette[index % layerPalette.length];
        colorSwatch.style.backgroundColor = `rgb(${c[0]},${c[1]},${c[2]})`;
        label.appendChild(colorSwatch);

        label.appendChild(document.createTextNode(name));
        layerChildren.appendChild(label);
    });
    layerGroup.appendChild(layerChildren);

    // --- Layer context menu (right-click) ---
    const contextMenu = document.createElement('div');
    contextMenu.className = 'context-menu';
    contextMenu.style.display = 'none';
    document.body.appendChild(contextMenu);

    function showOnlyLayers(indices) {
        const updates = {};
        layerIds.forEach((id, i) => {
            updates[id] = indices.has(i);
        });
        layerModel.checkSet(updates);
    }

    function hideContextMenu() {
        contextMenu.style.display = 'none';
    }

    const n = techData.layers.length;
    const menuItems = [
        { label: 'Show only this layer',  fn: (i) => layerRangeSet(i, 0, 0, n) },
        { label: 'Show layer range \u2195',   fn: (i) => layerRangeSet(i, 1, 1, n) },
        { label: 'Show layer range \u2195\u2195', fn: (i) => layerRangeSet(i, 2, 2, n) },
        { label: 'Show layer range \u2193',   fn: (i) => layerRangeSet(i, 1, 0, n) },
        { label: 'Show layer range \u2191',   fn: (i) => layerRangeSet(i, 0, 1, n) },
    ];

    layerChildren.querySelectorAll('label').forEach((label, index) => {
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
    });

    document.addEventListener('click', (e) => {
        if (!contextMenu.contains(e.target)) hideContextMenu();
    });
    document.addEventListener('keydown', (e) => {
        if (e.key === 'Escape') hideContextMenu();
    });

    // Toggle collapse
    layerArrow.addEventListener('click', (e) => {
        e.preventDefault();
        e.stopPropagation();
        const collapsed = layerChildren.classList.toggle('collapsed');
        layerArrow.textContent = collapsed ? '▶' : '▼';
    });

    app.displayControlsEl.appendChild(layerGroup);

    // --- Visibility tree (Instances, Nets, Shapes, Debug) ---
    const visTree = new VisTree(visibility, redrawAllLayers);
    visTree.add({ label: 'Instances', children: [
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
    visTree.add({ label: 'Nets', children: [
        { key: 'net_signal', label: 'Signal' },
        { key: 'net_power', label: 'Power' },
        { key: 'net_ground', label: 'Ground' },
        { key: 'net_clock', label: 'Clock' },
        { key: 'net_reset', label: 'Reset' },
        { key: 'net_tieoff', label: 'Tie off' },
        { key: 'net_scan', label: 'Scan' },
        { key: 'net_analog', label: 'Analog' },
    ]});
    visTree.add({ label: 'Shapes', children: [
        { key: 'routing', label: 'Routing' },
        { key: 'special_nets', label: 'Special Nets' },
        { key: 'pins', label: 'Pins' },
        { key: 'blockages', label: 'Blockages' },
    ]});
    visTree.add({ label: 'Blockages', children: [
        { key: 'placement_blockages', label: 'Placement' },
        { key: 'routing_obstructions', label: 'Routing' },
    ]});
    if (techData.sites && techData.sites.length > 0) {
        visTree.add({ label: 'Rows', visKey: 'rows', children:
            techData.sites.map(name => ({
                key: 'site_' + name, label: name,
            })),
        });
    }
    visTree.add({ label: 'Tracks', children: [
        { key: 'tracks_pref', label: 'Pref' },
        { key: 'tracks_non_pref', label: 'Non Pref' },
    ]});
    visTree.add({ key: 'module_view', label: 'Module view' });
    visTree.add({ key: 'debug', label: 'Debug tiles' });
    visTree.render(app.displayControlsEl);
}
