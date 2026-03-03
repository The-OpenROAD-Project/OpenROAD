import { GoldenLayout } from 'https://esm.sh/golden-layout@2.6.0';

// ─── WebSocket Manager ──────────────────────────────────────────────────────

class WebSocketManager {
    constructor(url) {
        this.url = url;
        this.ws = null;
        this.nextId = 1;
        this.pending = new Map(); // id -> {resolve, reject}
        this.reconnectDelay = 1000;
        this.readyPromise = null;
        this.readyResolve = null;
        this.connect();
    }

    connect() {
        this.readyPromise = new Promise(resolve => {
            this.readyResolve = resolve;
        });

        this.ws = new WebSocket(this.url);
        this.ws.binaryType = 'arraybuffer';

        this.ws.onopen = () => {
            console.log('WebSocket connected');
            this.reconnectDelay = 1000;
            this.readyResolve();
        };

        this.ws.onmessage = (event) => {
            this.handleMessage(event.data);
        };

        this.ws.onclose = () => {
            console.log('WebSocket closed, reconnecting...');
            for (const [id, handler] of this.pending) {
                handler.reject(new Error('WebSocket closed'));
            }
            this.pending.clear();
            setTimeout(() => this.connect(), this.reconnectDelay);
            this.reconnectDelay = Math.min(this.reconnectDelay * 2, 30000);
        };

        this.ws.onerror = (err) => {
            console.error('WebSocket error:', err);
        };
    }

    handleMessage(data) {
        // Binary frame: [4B id][1B type][3B reserved][payload...]
        const view = new DataView(data);
        const id = view.getUint32(0);   // big-endian
        const type = view.getUint8(4);  // 0=JSON, 1=PNG, 2=error

        const handler = this.pending.get(id);
        if (!handler) {
            return; // stale response (e.g. tile scrolled away)
        }
        this.pending.delete(id);
        updateStatus();

        const payload = data.slice(8);

        if (type === 2) {
            handler.reject(new Error(new TextDecoder().decode(payload)));
        } else if (type === 0) {
            handler.resolve(JSON.parse(new TextDecoder().decode(payload)));
        } else if (type === 1) {
            handler.resolve(new Blob([payload], { type: 'image/png' }));
        }
    }

    request(msg) {
        const id = this.nextId++;
        msg.id = id;
        return new Promise((resolve, reject) => {
            if (!this.ws || this.ws.readyState !== WebSocket.OPEN) {
                reject(new Error('WebSocket not connected'));
                return;
            }
            this.pending.set(id, { resolve, reject });
            this.ws.send(JSON.stringify(msg));
            updateStatus();
        });
    }

    cancel(id) {
        this.pending.delete(id);
        updateStatus();
    }
}

// ─── WS Tile Layer ──────────────────────────────────────────────────────────

const WSTileLayer = L.GridLayer.extend({
    initialize: function(wsManager, layerName, options) {
        this._wsManager = wsManager;
        this._layerName = layerName;
        L.GridLayer.prototype.initialize.call(this, options);
    },

    createTile: function(coords, done) {
        const tile = document.createElement('img');
        tile.alt = '';
        tile.setAttribute('role', 'presentation');

        const requestId = this._wsManager.nextId;
        tile._wsRequestId = requestId;

        const vf = {};
        for (const [k, v] of Object.entries(visibility)) {
            vf[k] = v ? 1 : 0;
        }
        this._wsManager.request({
            type: 'tile',
            layer: this._layerName,
            z: coords.z,
            x: coords.x,
            y: coords.y,
            ...vf,
        }).then(blob => {
            tile.onload = () => {
                URL.revokeObjectURL(tile.src);
                done(null, tile);
            };
            tile.onerror = () => {
                done(new Error('tile image load error'), tile);
            };
            tile.src = URL.createObjectURL(blob);
        }).catch(err => {
            done(err, tile);
        });

        return tile;
    },

    // Re-request all existing tiles in place (no removal/flash).
    // Use this instead of redraw() for visibility changes.
    refreshTiles: function() {
        if (!this._map) return;

        const vf = {};
        for (const [k, v] of Object.entries(visibility)) {
            vf[k] = v ? 1 : 0;
        }

        for (const key in this._tiles) {
            const tileInfo = this._tiles[key];
            if (!tileInfo || !tileInfo.el) continue;

            const tile = tileInfo.el;
            const coords = tileInfo.coords;

            // Cancel any pending request for this tile
            if (tile._wsRequestId !== undefined) {
                this._wsManager.cancel(tile._wsRequestId);
            }

            const requestId = this._wsManager.nextId;
            tile._wsRequestId = requestId;

            this._wsManager.request({
                type: 'tile',
                layer: this._layerName,
                z: coords.z,
                x: coords.x,
                y: coords.y,
                ...vf,
            }).then(blob => {
                if (tile.src && tile.src.startsWith('blob:')) {
                    URL.revokeObjectURL(tile.src);
                }
                tile.src = URL.createObjectURL(blob);
            }).catch(() => {
                // Tile refresh failed; keep existing image
            });
        }
    },

    _removeTile: function(key) {
        const tile = this._tiles[key];
        if (tile && tile.el) {
            if (tile.el._wsRequestId !== undefined) {
                this._wsManager.cancel(tile.el._wsRequestId);
            }
            if (tile.el.src && tile.el.src.startsWith('blob:')) {
                URL.revokeObjectURL(tile.el.src);
            }
        }
        L.GridLayer.prototype._removeTile.call(this, key);
    }
});

// ─── Status Indicator ───────────────────────────────────────────────────────

const statusDiv = document.getElementById('ws-status');

function updateStatus() {
    const n = wsManager ? wsManager.pending.size : 0;
    if (n === 0) {
        statusDiv.textContent = '';
        statusDiv.style.display = 'none';
    } else {
        statusDiv.textContent = `pending: ${n}`;
        statusDiv.style.display = '';
        statusDiv.style.color = n > 20 ? '#f88' : '#ff0';
    }
}

// ─── Component Factories ────────────────────────────────────────────────────

let map = null;
let fitBounds = null;
let displayControlsEl = null;
let allLayers = [];
let designScale = null;   // pixels-per-DBU for coordinate conversion
let designMaxDXDY = null; // max(width, height) in DBU for Y-axis mapping

const visibility = {
    stdcells: true,
    macros: true,
    // Pad sub-types
    pad_input: true,
    pad_output: true,
    pad_inout: true,
    pad_power: true,
    pad_spacer: true,
    pad_areaio: true,
    pad_other: true,
    // Physical sub-types
    phys_fill: false,
    phys_endcap: true,
    phys_welltap: true,
    phys_tie: true,
    phys_antenna: true,
    phys_cover: true,
    phys_bump: true,
    phys_other: true,
    // Std cell sub-types
    std_bufinv: true,
    std_bufinv_timing: true,
    std_clock_bufinv: true,
    std_clock_gate: true,
    std_level_shift: true,
    std_sequential: true,
    std_combinational: true,
    // Net sub-types
    net_signal: true,
    net_power: true,
    net_ground: true,
    net_clock: true,
    net_reset: true,
    net_tieoff: true,
    net_scan: true,
    net_analog: true,
    // Shapes
    routing: true,
    special_nets: true,
    pins: true,
    blockages: true,
    // Debug
    debug: false,
};

function redrawAllLayers() {
    for (const layer of allLayers) {
        layer.refreshTiles();
    }
}

function makeVisToggle(key, labelText, checked) {
    if (checked === undefined) {
        checked = visibility[key];
    } else {
        visibility[key] = checked;
    }
    const label = document.createElement('label');
    const checkbox = document.createElement('input');
    checkbox.type = 'checkbox';
    checkbox.checked = checked;
    checkbox.addEventListener('change', () => {
        visibility[key] = checkbox.checked;
        redrawAllLayers();
    });
    label.appendChild(checkbox);
    label.appendChild(document.createTextNode(labelText));
    return label;
}

// children: array of [key, label] for leaf toggles, or nested makeVisGroup elements
// options: { visKey, disabled, expanded, checked }
//   visKey: if set, parent checkbox also controls this visibility key directly
//   disabled: if true, children are grayed out and non-interactive
//   expanded: if true, start with children visible
//   checked: if false, start unchecked with all children off (default true)
function makeVisGroup(groupLabel, children, options = {}) {
    const startChecked = options.checked !== false;
    const container = document.createElement('div');
    container.className = 'vis-group';

    // Parent row with checkbox + disclosure triangle
    const header = document.createElement('label');
    header.className = 'vis-group-header';

    const arrow = document.createElement('span');
    arrow.className = 'vis-arrow';
    arrow.textContent = options.expanded ? '\u25BC' : '\u25B6';
    header.appendChild(arrow);

    const parentCb = document.createElement('input');
    parentCb.type = 'checkbox';
    parentCb.checked = startChecked;
    if (options.visKey) {
        visibility[options.visKey] = startChecked;
    }
    header.appendChild(parentCb);
    header.appendChild(document.createTextNode(groupLabel));
    container.appendChild(header);

    // Child items
    const childDiv = document.createElement('div');
    childDiv.className = options.expanded
        ? 'vis-group-children'
        : 'vis-group-children collapsed';
    if (options.disabled) {
        childDiv.classList.add('disabled');
    }

    const childCbs = [];
    const nestedGroups = [];  // nested makeVisGroup containers
    for (const child of children) {
        if (child instanceof HTMLElement) {
            // Nested group element — if parent starts unchecked, uncheck children
            childDiv.appendChild(child);
            nestedGroups.push(child);
            const cbs = child.querySelectorAll('input[type="checkbox"]');
            cbs.forEach(cb => {
                childCbs.push(cb);
                if (!startChecked) {
                    cb.checked = false;
                    cb.dispatchEvent(new Event('change'));
                }
            });
        } else {
            const [key, label] = child;
            const toggle = makeVisToggle(key, label, startChecked ? undefined : false);
            childDiv.appendChild(toggle);
            childCbs.push(toggle.querySelector('input'));
        }
    }
    // Sync nested group parents after init overrides
    for (const ng of nestedGroups) {
        if (ng._syncParent) ng._syncParent();
    }
    container.appendChild(childDiv);

    // Toggle collapse on arrow click
    arrow.addEventListener('click', (e) => {
        e.preventDefault();
        e.stopPropagation();
        const collapsed = childDiv.classList.toggle('collapsed');
        arrow.textContent = collapsed ? '\u25B6' : '\u25BC';
    });

    // Parent checkbox toggles all children + optional direct visKey
    parentCb.addEventListener('change', () => {
        if (options.visKey) {
            visibility[options.visKey] = parentCb.checked;
        }
        if (!options.disabled) {
            for (const cb of childCbs) {
                cb.checked = parentCb.checked;
                cb.dispatchEvent(new Event('change'));
            }
            // Sync nested group parents (indeterminate won't clear
            // without this since non-bubbling events don't reach
            // the nested childDiv listener).
            for (const ng of nestedGroups) {
                if (ng._syncParent) ng._syncParent();
            }
        }
        redrawAllLayers();
    });

    // Update parent state when children change
    function syncParent() {
        const allChecked = childCbs.every(cb => cb.checked);
        const someChecked = childCbs.some(cb => cb.checked);
        parentCb.checked = allChecked;
        parentCb.indeterminate = someChecked && !allChecked;
    }
    childDiv.addEventListener('change', syncParent);

    // Expose syncParent so ancestor groups can call it
    container._syncParent = syncParent;

    // Compute initial parent state from actual child states
    syncParent();

    return container;
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

function createLayoutViewer(container) {
    const mapDiv = document.createElement('div');
    mapDiv.style.width = '100%';
    mapDiv.style.height = '100%';
    mapDiv.style.backgroundColor = '#111';
    container.element.appendChild(mapDiv);

    map = L.map(mapDiv, {
        crs: L.CRS.Simple,
        zoom: 1,
        zoomSnap: 0,
        fadeAnimation: false,
        attributionControl: false,
    });

    new ResizeObserver(() => {
        map.invalidateSize({ animate: false });
    }).observe(mapDiv);
}

function createDisplayControls(container) {
    const el = document.createElement('div');
    el.className = 'display-controls';
    el.innerHTML = '<div class="loading">Loading layers...</div>';
    container.element.appendChild(el);
    displayControlsEl = el;
}

let tclOutputEl = null;

function tclAppend(text) {
    if (tclOutputEl) {
        tclOutputEl.value += text;
        tclOutputEl.scrollTop = tclOutputEl.scrollHeight;
    }
}

function createTclConsole(container) {
    const el = document.createElement('div');
    el.className = 'tcl-console';
    el.innerHTML =
        '<textarea class="tcl-output" readonly>OpenROAD Tcl Console\n% </textarea>' +
        '<div class="tcl-input-row">' +
        '  <span class="tcl-prompt">%</span>' +
        '  <input class="tcl-input" type="text" placeholder="Enter Tcl command..." />' +
        '</div>';
    container.element.appendChild(el);

    tclOutputEl = el.querySelector('.tcl-output');
    const input = el.querySelector('.tcl-input');
    input.addEventListener('keydown', (e) => {
        if (e.key === 'Enter') {
            const cmd = input.value.trim();
            if (cmd) {
                tclAppend(cmd + '\n');
                tclAppend('(Not connected to Tcl interpreter)\n% ');
                input.value = '';
            }
        }
    });
}

function createInspector(container) {
    createStubPanel(container, 'Inspector',
        'Select an object in the layout to inspect its properties.');
}

function createBrowser(container) {
    createStubPanel(container, 'Hierarchy',
        'Design hierarchy browser.');
}

function createTimingWidget(container) {
    createStubPanel(container, 'Timing',
        'Timing path analysis.');
}

function createDRCWidget(container) {
    createStubPanel(container, 'DRC',
        'Design rule check violations viewer.');
}

function createClockWidget(container) {
    createStubPanel(container, 'Clock Tree',
        'Clock tree visualization.');
}

function createChartsWidget(container) {
    createStubPanel(container, 'Charts',
        'Timing histograms and charts.');
}

function createHelpWidget(container) {
    const el = document.createElement('div');
    el.className = 'help-panel';
    el.innerHTML =
        '<h3>Keyboard Shortcuts</h3>' +
        '<table>' +
        '<tr><td><kbd>f</kbd></td><td>Fit design to viewport</td></tr>' +
        '<tr><td><kbd>scroll</kbd></td><td>Zoom in/out</td></tr>' +
        '<tr><td><kbd>drag</kbd></td><td>Pan the view</td></tr>' +
        '</table>';
    container.element.appendChild(el);
}

function createSelectHighlight(container) {
    createStubPanel(container, 'Selection',
        'Selection and highlight browser.');
}

function createStubPanel(container, title, description) {
    const el = document.createElement('div');
    el.className = 'stub-panel';
    el.innerHTML =
        `<div class="stub-title">${title}</div>` +
        `<div class="stub-desc">${description}</div>`;
    container.element.appendChild(el);
}

// ─── Layout Configuration ───────────────────────────────────────────────────

const defaultLayoutConfig = {
    root: {
        type: 'row',
        content: [
            {
                type: 'component',
                componentType: 'DisplayControls',
                title: 'Display Controls',
                width: 15,
            },
            {
                type: 'column',
                width: 55,
                content: [
                    {
                        type: 'component',
                        componentType: 'LayoutViewer',
                        title: 'Layout',
                        height: 70,
                        isClosable: false,
                    },
                    {
                        type: 'component',
                        componentType: 'TclConsole',
                        title: 'Tcl Console',
                        height: 30,
                    },
                ],
            },
            {
                type: 'stack',
                width: 30,
                content: [
                    {
                        type: 'component',
                        componentType: 'Inspector',
                        title: 'Inspector',
                    },
                    {
                        type: 'component',
                        componentType: 'Browser',
                        title: 'Hierarchy',
                    },
                    {
                        type: 'component',
                        componentType: 'TimingWidget',
                        title: 'Timing',
                    },
                    {
                        type: 'component',
                        componentType: 'DRCWidget',
                        title: 'DRC',
                    },
                    {
                        type: 'component',
                        componentType: 'ClockWidget',
                        title: 'Clock Tree',
                    },
                    {
                        type: 'component',
                        componentType: 'ChartsWidget',
                        title: 'Charts',
                    },
                    {
                        type: 'component',
                        componentType: 'HelpWidget',
                        title: 'Help',
                    },
                ],
            },
        ],
    },
};

// ─── Golden Layout Init ─────────────────────────────────────────────────────

const goldenLayout = new GoldenLayout(document.getElementById('gl-container'));

goldenLayout.registerComponentFactoryFunction('LayoutViewer', createLayoutViewer);
goldenLayout.registerComponentFactoryFunction('DisplayControls', createDisplayControls);
goldenLayout.registerComponentFactoryFunction('TclConsole', createTclConsole);
goldenLayout.registerComponentFactoryFunction('Inspector', createInspector);
goldenLayout.registerComponentFactoryFunction('Browser', createBrowser);
goldenLayout.registerComponentFactoryFunction('TimingWidget', createTimingWidget);
goldenLayout.registerComponentFactoryFunction('DRCWidget', createDRCWidget);
goldenLayout.registerComponentFactoryFunction('ClockWidget', createClockWidget);
goldenLayout.registerComponentFactoryFunction('ChartsWidget', createChartsWidget);
goldenLayout.registerComponentFactoryFunction('HelpWidget', createHelpWidget);
goldenLayout.registerComponentFactoryFunction('SelectHighlight', createSelectHighlight);

goldenLayout.loadLayout(defaultLayoutConfig);

// Handle window resize
window.addEventListener('resize', () => {
    goldenLayout.setSize(window.innerWidth, window.innerHeight);
});

// ─── WebSocket Init ─────────────────────────────────────────────────────────

const wsUrl = `ws://${window.location.hostname || 'localhost'}:8080/ws`;
const wsManager = new WebSocketManager(wsUrl);

let hasLiberty = false;

wsManager.readyPromise.then(async () => {
    try {
        const [layersData, boundsData, infoData] = await Promise.all([
            wsManager.request({ type: 'layers' }),
            wsManager.request({ type: 'bounds' }),
            wsManager.request({ type: 'info' }),
        ]);
        hasLiberty = infoData.has_liberty;

        // --- Populate Display Controls ---
        if (displayControlsEl) {
            displayControlsEl.innerHTML = '';
            allLayers = [];

            // Instance borders layer (always below routing layers)
            const instancesLayer = new WSTileLayer(wsManager, '_instances', {
                zIndex: 0,
            });
            instancesLayer.addTo(map);
            allLayers.push(instancesLayer);

            // --- Layers group ---
            const layerGroup = document.createElement('div');
            layerGroup.className = 'vis-group';

            const layerHeader = document.createElement('label');
            layerHeader.className = 'vis-group-header';
            const layerArrow = document.createElement('span');
            layerArrow.className = 'vis-arrow';
            layerArrow.textContent = '\u25BC';
            layerHeader.appendChild(layerArrow);
            const layerParentCb = document.createElement('input');
            layerParentCb.type = 'checkbox';
            layerParentCb.checked = true;
            layerHeader.appendChild(layerParentCb);
            layerHeader.appendChild(document.createTextNode('Layers'));
            layerGroup.appendChild(layerHeader);

            const layerChildren = document.createElement('div');
            layerChildren.className = 'vis-group-children';
            const layerCbs = [];

            layersData.layers.forEach((name, index) => {
                const layer = new WSTileLayer(wsManager, name, {
                    opacity: 0.7,
                    zIndex: index + 1
                });
                layer.addTo(map);
                allLayers.push(layer);

                const label = document.createElement('label');

                const checkbox = document.createElement('input');
                checkbox.type = 'checkbox';
                checkbox.checked = true;
                checkbox.addEventListener('change', () => {
                    if (checkbox.checked) {
                        layer.addTo(map);
                    } else {
                        map.removeLayer(layer);
                    }
                });
                label.appendChild(checkbox);
                layerCbs.push(checkbox);

                const colorSwatch = document.createElement('span');
                colorSwatch.className = 'layer-color';
                const c = layerPalette[index % layerPalette.length];
                colorSwatch.style.backgroundColor = `rgb(${c[0]},${c[1]},${c[2]})`;
                label.appendChild(colorSwatch);

                label.appendChild(document.createTextNode(name));
                layerChildren.appendChild(label);
            });
            layerGroup.appendChild(layerChildren);

            // Parent checkbox toggles all layers
            layerParentCb.addEventListener('change', () => {
                for (const cb of layerCbs) {
                    cb.checked = layerParentCb.checked;
                    cb.dispatchEvent(new Event('change', { bubbles: true }));
                }
            });
            // Update parent state when children change
            layerChildren.addEventListener('change', () => {
                const allChecked = layerCbs.every(cb => cb.checked);
                const someChecked = layerCbs.some(cb => cb.checked);
                layerParentCb.checked = allChecked;
                layerParentCb.indeterminate = someChecked && !allChecked;
            });
            // Toggle collapse
            layerArrow.addEventListener('click', (e) => {
                e.preventDefault();
                e.stopPropagation();
                const collapsed = layerChildren.classList.toggle('collapsed');
                layerArrow.textContent = collapsed ? '\u25B6' : '\u25BC';
            });

            displayControlsEl.appendChild(layerGroup);

            // --- Instances section (parent checkbox toggles all) ---
            const instanceGroup = makeVisGroup('Instances', [
                makeVisGroup('Std Cells', [
                    makeVisGroup('Bufs/Invs', [
                        ['std_bufinv_timing', 'Timing opt.'],
                        ['std_bufinv', 'Netlist'],
                    ]),
                    ['std_combinational', 'Combinational'],
                    ['std_sequential', 'Sequential'],
                    makeVisGroup('Clock tree', [
                        ['std_clock_bufinv', 'Buffer/Inverter'],
                        ['std_clock_gate', 'Clock gate'],
                    ]),
                    ['std_level_shift', 'Level shifter'],
                ], { visKey: 'stdcells', disabled: !hasLiberty }),
                makeVisToggle('macros', 'Macros'),
                makeVisGroup('Pads', [
                    ['pad_input', 'Input'],
                    ['pad_output', 'Output'],
                    ['pad_inout', 'Inout'],
                    ['pad_power', 'Power'],
                    ['pad_spacer', 'Spacer'],
                    ['pad_areaio', 'Area IO'],
                    ['pad_other', 'Other'],
                ]),
                makeVisGroup('Physical', [
                    ['phys_fill', 'Fill'],
                    ['phys_endcap', 'Endcap'],
                    ['phys_welltap', 'Welltap'],
                    ['phys_tie', 'Tie Hi/Lo'],
                    ['phys_antenna', 'Antenna'],
                    ['phys_cover', 'Cover'],
                    ['phys_bump', 'Bump'],
                    ['phys_other', 'Other'],
                ]),
            ]);
            displayControlsEl.appendChild(instanceGroup);

            // --- Nets group ---
            const netGroup = makeVisGroup('Nets', [
                ['net_signal', 'Signal'],
                ['net_power', 'Power'],
                ['net_ground', 'Ground'],
                ['net_clock', 'Clock'],
                ['net_reset', 'Reset'],
                ['net_tieoff', 'Tie off'],
                ['net_scan', 'Scan'],
                ['net_analog', 'Analog'],
            ]);
            displayControlsEl.appendChild(netGroup);

            // --- Shapes group ---
            const shapeGroup = makeVisGroup('Shapes', [
                ['routing', 'Routing'],
                ['special_nets', 'Special Nets'],
                ['pins', 'Pins'],
                ['blockages', 'Blockages'],
            ]);
            displayControlsEl.appendChild(shapeGroup);

            // --- Debug toggle ---
            displayControlsEl.appendChild(makeVisToggle('debug', 'Debug tiles'));
        }

        // --- Set Bounds ---
        const designBounds = boundsData.bounds;

        const minY = designBounds[0][0];
        const minX = designBounds[0][1];
        const maxY = designBounds[1][0];
        const maxX = designBounds[1][1];

        const designWidth = maxX - minX;
        const designHeight = maxY - minY;
        const tileSize = 256;

        const scale = tileSize / Math.max(designWidth, designHeight);
        designScale = scale;
        designMaxDXDY = Math.max(designWidth, designHeight);

        fitBounds = [
            [-minY * scale, minX * scale],
            [-maxY * scale, maxX * scale]
        ];
        map.fitBounds(fitBounds);

        // Click-to-select: convert click position to DBU and query server
        map.on('click', (e) => {
            const dbu_x = Math.round(e.latlng.lng / designScale);
            // Tile rendering applies two Y-flips (tile index + pixel), so
            // the visual dbu_y maps as: dbu_y = maxDXDY + lat / scale
            const dbu_y = Math.round(designMaxDXDY + e.latlng.lat / designScale);

            const vf = {};
            for (const [k, v] of Object.entries(visibility)) {
                vf[k] = v ? 1 : 0;
            }
            wsManager.request({ type: 'select', dbu_x, dbu_y, zoom: map.getZoom(), ...vf })
                .then(data => {
                    console.log('Select response:', data, 'at dbu', dbu_x, dbu_y);
                    map.closePopup();
                    if (data.selected && data.selected.length > 0) {
                        const inst = data.selected[0];
                        L.popup()
                            .setLatLng(e.latlng)
                            .setContent(
                                `<strong>${inst.name}</strong><br>${inst.master}<br><small style="color:#888">(${dbu_x}, ${dbu_y})</small>`)
                            .openOn(map);
                    } else {
                        L.popup()
                            .setLatLng(e.latlng)
                            .setContent(
                                `<em>No instance at (${dbu_x}, ${dbu_y})</em>`)
                            .openOn(map);
                    }
                    redrawAllLayers();
                })
                .catch(err => {
                    console.error('Select failed:', err);
                });
        });
    } catch (err) {
        console.error('Failed to load initial data from server:', err);
    }
});

// ─── Keyboard Shortcuts ─────────────────────────────────────────────────────

document.addEventListener('keydown', (e) => {
    if (e.key === 'f' && fitBounds) {
        map.fitBounds(fitBounds);
    }
});
