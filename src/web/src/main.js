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

// Data-model-driven checkbox tree (mirrors Qt's QStandardItemModel pattern).
// State lives in a plain JS tree; DOM is synced from model after every change.
// No event delegation, no bubbling — avoids the cascade bugs of the old approach.
class VisTree {
    constructor(visibility, onChange) {
        this.visibility = visibility;
        this.onChange = onChange;
        this.roots = [];
    }

    // Add a tree from a declarative spec.
    // Leaf:  { key, label }
    // Group: { label, children: [...], visKey?, disabled? }
    add(spec) {
        this.roots.push(this._build(spec, null));
        return this;
    }

    render(container) {
        for (const r of this.roots) container.appendChild(this._dom(r));
        for (const r of this.roots) this._sync(r);
    }

    // -- model --

    _build(spec, parent) {
        const node = {
            label: spec.label, key: spec.key || null,
            visKey: spec.visKey || null, disabled: !!spec.disabled,
            parent, children: [], checked: true, indeterminate: false, cb: null,
        };
        if (spec.children) {
            for (const c of spec.children)
                node.children.push(this._build(c, node));
            this._computeParent(node);
        } else if (node.key) {
            node.checked = !!this.visibility[node.key];
        }
        return node;
    }

    _computeParent(node) {
        const all  = node.children.every(c => c.checked && !c.indeterminate);
        const none = node.children.every(c => !c.checked && !c.indeterminate);
        node.checked = all;
        node.indeterminate = !all && !none;
    }

    _setSubtree(node, checked) {
        node.checked = checked;
        node.indeterminate = false;
        for (const c of node.children) this._setSubtree(c, checked);
    }

    _onCheck(node) {
        node.checked = node.cb.checked;
        node.indeterminate = false;
        if (!node.disabled) {
            for (const c of node.children) this._setSubtree(c, node.checked);
        }
        for (let n = node.parent; n; n = n.parent) this._computeParent(n);
        for (const r of this.roots) this._sync(r);
        this.onChange();
    }

    _sync(node) {
        if (node.cb) {
            node.cb.checked = node.checked;
            node.cb.indeterminate = node.indeterminate;
        }
        if (node.key) this.visibility[node.key] = node.checked;
        if (node.visKey) this.visibility[node.visKey] = node.checked || node.indeterminate;
        for (const c of node.children) this._sync(c);
    }

    // -- DOM --

    _dom(node) {
        const cb = document.createElement('input');
        cb.type = 'checkbox';
        node.cb = cb;
        cb.addEventListener('change', () => this._onCheck(node));

        if (!node.children.length) {
            const label = document.createElement('label');
            label.appendChild(cb);
            label.appendChild(document.createTextNode(node.label));
            return label;
        }

        const div = document.createElement('div');
        div.className = 'vis-group';

        const header = document.createElement('label');
        header.className = 'vis-group-header';
        const arrow = document.createElement('span');
        arrow.className = 'vis-arrow';
        arrow.textContent = '\u25B6';
        header.appendChild(arrow);
        header.appendChild(cb);
        header.appendChild(document.createTextNode(node.label));
        div.appendChild(header);

        const kids = document.createElement('div');
        kids.className = 'vis-group-children collapsed';
        if (node.disabled) kids.classList.add('disabled');
        for (const c of node.children) kids.appendChild(this._dom(c));
        div.appendChild(kids);

        arrow.addEventListener('click', (e) => {
            e.preventDefault();
            e.stopPropagation();
            kids.classList.toggle('collapsed');
            arrow.textContent = kids.classList.contains('collapsed')
                ? '\u25B6' : '\u25BC';
        });

        return div;
    }
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

// ─── Inspector Panel ────────────────────────────────────────────────────────

let inspectorEl = null;

let hoverRect = null;
function clearHoverHighlight() {
    if (hoverRect) {
        map.removeLayer(hoverRect);
        hoverRect = null;
    }
}

function makeClickable(el, selectId) {
    el.classList.add('inspector-link');
    el.addEventListener('click', (e) => {
        e.stopPropagation();
        navigateInspector(selectId);
    });
    el.addEventListener('mouseenter', () => {
        wsManager.request({ type: 'hover', select_id: selectId })
            .then(data => {
                if (data.bbox && map && designScale) {
                    clearHoverHighlight();
                    const [x1, y1, x2, y2] = data.bbox;
                    // Enforce a minimum visible size (in pixels) centered on
                    // the object, so tiny items like ITerms are still visible.
                    const minPx = 20;
                    const zoom = map.getZoom();
                    const scale = Math.pow(2, zoom);
                    const minDbu = minPx / (designScale * scale);
                    const cx = (x1 + x2) / 2;
                    const cy = (y1 + y2) / 2;
                    const hw = Math.max((x2 - x1) / 2, minDbu / 2);
                    const hh = Math.max((y2 - y1) / 2, minDbu / 2);
                    const bounds = [
                        [((cy - hh) - designMaxDXDY) * designScale, (cx - hw) * designScale],
                        [((cy + hh) - designMaxDXDY) * designScale, (cx + hw) * designScale],
                    ];
                    hoverRect = L.rectangle(bounds, {
                        className: 'hover-highlight',
                        color: '#fff', weight: 2, fill: true,
                        fillColor: '#fff', fillOpacity: 0.15,
                    }).addTo(map);
                }
            })
            .catch(() => {});
    });
    el.addEventListener('mouseleave', () => {
        clearHoverHighlight();
    });
}

function navigateInspector(selectId) {
    wsManager.request({ type: 'inspect', select_id: selectId })
        .then(data => {
            if (data.error) {
                console.error('Inspect error:', data.error);
                return;
            }
            updateInspector(data);

            // Show popup and highlight on the map
            map.closePopup();
            if (highlightRect) {
                map.removeLayer(highlightRect);
                highlightRect = null;
            }
            if (data.bbox && map && designScale) {
                const [x1, y1, x2, y2] = data.bbox;
                // For non-instance objects, show dashed bbox outline
                // (instances get the yellow tile-based highlight instead)
                if (data.type !== 'Inst') {
                    highlightBBox(x1, y1, x2, y2);
                }
                // Show a popup at the center of the bbox
                const cx = ((x1 + x2) / 2) * designScale;
                const cy = (((y1 + y2) / 2) - designMaxDXDY) * designScale;
                L.popup()
                    .setLatLng([cy, cx])
                    .setContent(
                        `<strong>${data.name}</strong><br>` +
                        `<small style="color:#888">${data.type}</small>`)
                    .openOn(map);
            }
            // Redraw tiles to update instance highlight
            redrawAllLayers();
        })
        .catch(err => console.error('Inspect failed:', err));
}

let highlightRect = null;
function highlightBBox(x1, y1, x2, y2) {
    if (highlightRect) {
        map.removeLayer(highlightRect);
    }
    // Tile rendering applies two Y-flips (tile index + pixel), so
    // the correct mapping is: lat = (dbu_y - maxDXDY) * scale
    const bounds = [
        [(y1 - designMaxDXDY) * designScale, x1 * designScale],
        [(y2 - designMaxDXDY) * designScale, x2 * designScale],
    ];
    highlightRect = L.rectangle(bounds, {
        color: '#ff0', weight: 2, fill: false, dashArray: '6,4',
    }).addTo(map);
}

function renderProperty(prop) {
    // Group with children (PropertyList or SelectionSet)
    if (prop.children) {
        const group = document.createElement('div');
        group.className = 'inspector-group';

        const header = document.createElement('div');
        header.className = 'inspector-group-header';
        const arrow = document.createElement('span');
        arrow.className = 'vis-arrow';
        arrow.textContent = '\u25B6';
        const nameSpan = document.createElement('span');
        nameSpan.className = 'inspector-prop-name';
        nameSpan.textContent = prop.name;
        const countSpan = document.createElement('span');
        countSpan.className = 'inspector-count';
        countSpan.textContent = `(${prop.children.length})`;
        header.appendChild(arrow);
        header.appendChild(nameSpan);
        header.appendChild(countSpan);
        group.appendChild(header);

        const kids = document.createElement('div');
        kids.className = 'inspector-group-children collapsed';
        for (const child of prop.children) {
            kids.appendChild(renderProperty(child));
        }
        group.appendChild(kids);

        arrow.addEventListener('click', () => {
            kids.classList.toggle('collapsed');
            arrow.textContent = kids.classList.contains('collapsed')
                ? '\u25B6' : '\u25BC';
        });
        header.addEventListener('click', () => {
            kids.classList.toggle('collapsed');
            arrow.textContent = kids.classList.contains('collapsed')
                ? '\u25B6' : '\u25BC';
        });

        return group;
    }

    // Leaf property: name + value
    const row = document.createElement('div');
    row.className = 'inspector-prop';
    const nameEl = document.createElement('span');
    nameEl.className = 'inspector-prop-name';
    nameEl.textContent = prop.name || '';
    const valEl = document.createElement('span');
    valEl.className = 'inspector-prop-value';
    valEl.textContent = prop.value || '';
    row.appendChild(nameEl);
    row.appendChild(valEl);

    // Make name/value clickable if they have a select_id
    if (prop.name_select_id !== undefined) {
        makeClickable(nameEl, prop.name_select_id);
    }
    if (prop.value_select_id !== undefined) {
        makeClickable(valEl, prop.value_select_id);
    }

    return row;
}

function zoomToBBox(bbox) {
    if (!bbox || !map || !designScale) return;
    const [x1, y1, x2, y2] = bbox;
    const bounds = [
        [(y1 - designMaxDXDY) * designScale, x1 * designScale],
        [(y2 - designMaxDXDY) * designScale, x2 * designScale],
    ];
    map.fitBounds(bounds, { padding: [20, 20] });
}

function updateInspector(data) {
    if (!inspectorEl) return;
    inspectorEl.innerHTML = '';

    if (!data || !data.properties || data.properties.length === 0) {
        const placeholder = document.createElement('div');
        placeholder.className = 'stub-panel';
        placeholder.innerHTML =
            '<div class="stub-title">Inspector</div>' +
            '<div class="stub-desc">Select an object in the layout to inspect its properties.</div>';
        inspectorEl.appendChild(placeholder);
        return;
    }

    // Toolbar with action buttons
    if (data.bbox) {
        const toolbar = document.createElement('div');
        toolbar.className = 'inspector-toolbar';
        const zoomBtn = document.createElement('button');
        zoomBtn.className = 'inspector-btn';
        zoomBtn.title = 'Zoom to';
        // Magnifying glass SVG (matches Google Material "zoom_in" icon)
        zoomBtn.innerHTML =
            '<svg width="16" height="16" viewBox="0 0 24 24" fill="currentColor">' +
            '<path d="M15.5 14h-.79l-.28-.27A6.471 6.471 0 0 0 16 9.5 6.5 6.5 0 1 0 9.5 16c1.61 0 3.09-.59 4.23-1.57l.27.28v.79l5 4.99L20.49 19l-4.99-5zm-6 0C7.01 14 5 11.99 5 9.5S7.01 5 9.5 5 14 7.01 14 9.5 11.99 14 9.5 14z"/>' +
            '<path d="M12 10h-2v2H9v-2H7V9h2V7h1v2h2v1z"/>' +
            '</svg>';
        zoomBtn.addEventListener('click', () => zoomToBBox(data.bbox));
        toolbar.appendChild(zoomBtn);
        inspectorEl.appendChild(toolbar);
    }

    for (const prop of data.properties) {
        inspectorEl.appendChild(renderProperty(prop));
    }
}

function createInspector(container) {
    const el = document.createElement('div');
    el.className = 'inspector';
    inspectorEl = el;
    container.element.appendChild(el);

    // Show placeholder initially
    updateInspector(null);
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

            // --- Visibility tree (Instances, Nets, Shapes, Debug) ---
            const visTree = new VisTree(visibility, redrawAllLayers);
            visTree.add({ label: 'Instances', children: [
                { label: 'Std Cells', visKey: 'stdcells', disabled: !hasLiberty, children: [
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
            visTree.add({ key: 'debug', label: 'Debug tiles' });
            visTree.render(displayControlsEl);
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
                        updateInspector(data);
                        // Highlight selected instance bbox
                        if (inst.bbox) {
                            highlightBBox(inst.bbox[0], inst.bbox[1],
                                          inst.bbox[2], inst.bbox[3]);
                        }
                    } else {
                        L.popup()
                            .setLatLng(e.latlng)
                            .setContent(
                                `<em>No instance at (${dbu_x}, ${dbu_y})</em>`)
                            .openOn(map);
                        updateInspector(null);
                        if (highlightRect) {
                            map.removeLayer(highlightRect);
                            highlightRect = null;
                        }
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
