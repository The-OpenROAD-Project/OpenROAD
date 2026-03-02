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
        return new Promise((resolve, reject) => {
            if (!this.ws || this.ws.readyState !== WebSocket.OPEN) {
                reject(new Error('WebSocket not connected'));
                return;
            }
            const id = this.nextId++;
            msg.id = id;
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

        this._wsManager.request({
            type: 'tile',
            layer: this._layerName,
            z: coords.z,
            x: coords.x,
            y: coords.y
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

wsManager.readyPromise.then(async () => {
    try {
        const [layersData, boundsData] = await Promise.all([
            wsManager.request({ type: 'layers' }),
            wsManager.request({ type: 'bounds' })
        ]);

        // --- Populate Display Controls with layer checkboxes ---
        if (displayControlsEl) {
            displayControlsEl.innerHTML = '';
            layersData.layers.forEach((name, index) => {
                const layer = new WSTileLayer(wsManager, name, {
                    opacity: 0.7,
                    zIndex: index + 1
                });
                layer.addTo(map);

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

                const colorSwatch = document.createElement('span');
                colorSwatch.className = 'layer-color';
                const c = layerPalette[index % layerPalette.length];
                colorSwatch.style.backgroundColor = `rgb(${c[0]},${c[1]},${c[2]})`;
                label.appendChild(colorSwatch);

                label.appendChild(document.createTextNode(name));
                displayControlsEl.appendChild(label);
            });
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

        fitBounds = [
            [-minY * scale, minX * scale],
            [-maxY * scale, maxX * scale]
        ];
        map.fitBounds(fitBounds);
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
