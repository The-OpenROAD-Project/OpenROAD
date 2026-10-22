// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

import { LitElement, html, css, svg, nothing } from 'lit';
import { when } from 'lit/directives/when.js';
import { ifDefined } from 'lit/directives/if-defined.js';

// ─── Native CSS Grid IDE Layout Manager & Custom Elements ────────────────────

/**
 * Lightweight event emitter for layout manager and component containers.
 */
export class EventEmitter {
    constructor() {
        this.eventListeners = new Map();
    }

    on(event, cb) {
        if (!this.eventListeners.has(event)) {
            this.eventListeners.set(event, new Set());
        }
        this.eventListeners.get(event).add(cb);
    }

    off(event, cb) {
        this.eventListeners.get(event)?.delete(cb);
    }

    emit(event, ...args) {
        const listeners = this.eventListeners.get(event);
        if (listeners) {
            for (const cb of listeners) {
                try { cb(...args); } catch (e) { console.error(e); }
            }
        }
    }

    fire(event, ...args) {
        this.emit(event, ...args);
    }
}

/**
 * Dispatches a bubbling, composed custom event from a DOM node.
 */
export function emitEvent(host, type, detail = {}) {
    host.dispatchEvent(new CustomEvent(type, {
        bubbles: true,
        composed: true,
        detail,
    }));
}

/**
 * <ide-pane> - Encapsulated IDE window pane custom element using Lit.
 * Manages its header (tab bar and action controls) and content viewport.
 */
export class IdePaneElement extends LitElement {
    static properties = {
        name: { type: String, reflect: true },
        maximizable: { type: Boolean, reflect: true },
        collapsible: { type: Boolean, reflect: true },
        maximized: { type: Boolean, reflect: true },
        collapsed: { type: Boolean, reflect: true },
    };

    static styles = css`
        :host {
            display: flex;
            flex-direction: column;
            width: 100%;
            height: 100%;
            overflow: hidden;
            background: var(--bg-panel);
            position: relative;
        }

        :host([hidden]) {
            display: none !important;
        }

        :host([maximized]) .ide-btn-collapse {
            display: none !important;
        }

        .ide-header {
            display: flex;
            align-items: center;
            justify-content: space-between;
            height: 28px;
            background: var(--bg-header);
            border-bottom: 1px solid var(--line-1);
            padding: 0 4px;
            user-select: none;
            flex-shrink: 0;
        }

        .ide-tabs {
            display: flex;
            height: 100%;
            min-width: 0;
            overflow-x: auto;
            scrollbar-width: none;
        }

        .ide-tabs::-webkit-scrollbar {
            display: none;
        }

        .ide-controls {
            display: flex;
            align-items: center;
            gap: 2px;
            flex-shrink: 0;
        }

        .ide-btn {
            display: inline-flex;
            align-items: center;
            justify-content: center;
            width: 20px;
            height: 20px;
            border-radius: var(--radius-xs);
            background: transparent;
            border: none;
            color: var(--fg-muted);
            cursor: pointer;
            font-size: 11px;
            padding: 0;
            transition: background-color 120ms ease, color 120ms ease;
        }

        .ide-btn:hover {
            background: var(--bg-hover);
            color: var(--fg-primary);
        }

        .ide-content {
            flex: 1;
            position: relative;
            overflow: hidden;
        }
    `;

    constructor() {
        super();
        this._onMaximizeClick = this._onMaximizeClick.bind(this);
        this._onCollapseClick = this._onCollapseClick.bind(this);
    }

    connectedCallback() {
        super.connectedCallback();
        this.classList.add('ide-pane', `ide-pane-${this.paneName}`);
    }

    get paneName() {
        return this.name || this.id.replace(/^pane-/, '') || 'pane';
    }

    get _collapseIcon() {
        const name = this.paneName;
        if (name === 'bottom') return this.collapsed ? '▲' : '▼';
        if (name === 'left') return this.collapsed ? '▶' : '◀';
        if (name === 'right') return this.collapsed ? '◀' : '▶';
        return '▼';
    }

    get _collapseTitle() {
        const name = this.paneName;
        const capitalized = name.charAt(0).toUpperCase() + name.slice(1);
        if (name === 'bottom') return this.collapsed ? 'Expand Console' : 'Collapse Console';
        if (name === 'left') return this.collapsed ? 'Expand Left Panel' : 'Collapse Left Panel';
        if (name === 'right') return this.collapsed ? 'Expand Right Panel' : 'Collapse Right Panel';
        return `Collapse ${capitalized}`;
    }

    _onMaximizeClick(e) {
        e.stopPropagation();
        emitEvent(this, 'ide-pane-maximize', { pane: this.paneName });
    }

    _onCollapseClick(e) {
        e.stopPropagation();
        emitEvent(this, 'ide-pane-collapse', { pane: this.paneName });
    }

    setMaximized(isMax) {
        this.maximized = !!isMax;
    }

    setCollapsed(isCollapsed) {
        this.collapsed = !!isCollapsed;
    }

    render() {
        const name = this.paneName;
        const prefix = this.id || `pane-${name}`;
        const hasMax = this.maximizable;
        const hasCol = this.collapsible;

        return html`
            <div class="ide-header" id="${prefix}-header">
                <div class="ide-tabs" id="${prefix}-tabs">
                    <slot name="tabs"></slot>
                </div>
                <div class="ide-controls" id="${prefix}-controls">
                    ${when(hasMax, () => html`
                        <button
                            class="ide-btn ide-btn-maximize"
                            id="btn-maximize-${name}"
                            title=${ifDefined(this.maximized ? 'Restore' : 'Maximize')}
                            @click=${this._onMaximizeClick}
                        >${this.maximized ? '⧉' : '⤢'}</button>
                    `)}
                    ${when(hasCol, () => html`
                        <button
                            class="ide-btn ide-btn-collapse"
                            id="btn-collapse-${name}"
                            title=${ifDefined(this._collapseTitle)}
                            @click=${this._onCollapseClick}
                        >${this._collapseIcon}</button>
                    `)}
                </div>
            </div>
            <div class="ide-content" id="${prefix}-content">
                <slot></slot>
            </div>
        `;
    }
}

/**
 * Reusable Lit Reactive Controller for pointer dragging and resizing.
 * Encapsulates pointer capture, coordinate delta tracking, and leak-proof window listeners.
 */
export class DragController {
    constructor(host, options = {}) {
        (this.host = host).addController(this);
        this.options = options;
        this._abort = null;
        this.isDragging = false;
        this.startPos = 0;
    }

    hostDisconnected() {
        this.stop();
    }

    start(e, isVertical = true) {
        e.preventDefault();
        try { this.host.setPointerCapture(e.pointerId); } catch (_) {}
        this.isDragging = true;
        this.host.classList.add('dragging');
        if (document.body) {
            document.body.classList.add('is-resizing', isVertical ? 'is-resizing-v' : 'is-resizing-h');
        }

        this.startPos = isVertical ? e.clientX : e.clientY;
        this.options.onStart?.({ startPos: this.startPos, event: e });

        this._abort = new AbortController();
        const { signal } = this._abort;

        window.addEventListener('pointermove', (ev) => {
            const currentPos = isVertical ? ev.clientX : ev.clientY;
            const delta = currentPos - this.startPos;
            this.options.onDrag?.({ delta, currentPos, startPos: this.startPos, event: ev });
        }, { signal });

        const endHandler = (ev) => {
            try { this.host.releasePointerCapture(e.pointerId); } catch (_) {}
            this.stop(isVertical);
            this.options.onEnd?.({ event: ev });
        };

        window.addEventListener('pointerup', endHandler, { signal });
        window.addEventListener('pointercancel', endHandler, { signal });
    }

    stop(isVertical = true) {
        if (!this.isDragging) return;
        this.isDragging = false;
        this.host.classList.remove('dragging');
        if (document.body) {
            document.body.classList.remove('is-resizing', 'is-resizing-v', 'is-resizing-h');
        }
        this._abort?.abort();
        this._abort = null;
    }
}

/**
 * <ide-splitter> - Drag-to-resize gutter splitter custom element using Lit.
 * Encapsulates pointer capture, resizing styles, and drag events via DragController.
 */
export class IdeSplitterElement extends LitElement {
    static properties = {
        for: { type: String },
        orientation: { type: String, reflect: true },
    };

    constructor() {
        super();
        this.orientation = 'vertical';
        this._connectedAbort = null;
        this.drag = new DragController(this, {
            onStart: ({ startPos }) => this._emit('start', { startPos }),
            onDrag: ({ delta, currentPos, startPos }) => this._emit('drag', { delta, currentPos, startPos }),
            onEnd: () => this._emit('end'),
        });
    }

    connectedCallback() {
        super.connectedCallback();
        this._connectedAbort = new AbortController();
        const { signal } = this._connectedAbort;

        this.setAttribute('role', 'separator');
        this.classList.add('ide-splitter');

        this.addEventListener('pointerdown', (e) => this.drag.start(e, this.isVertical), { signal });
        this.addEventListener('dblclick', () => this._emit('reset'), { signal });
    }

    disconnectedCallback() {
        super.disconnectedCallback();
        this._connectedAbort?.abort();
    }

    updated(changedProperties) {
        super.updated(changedProperties);
        const isVertical = this.isVertical;
        this.setAttribute('aria-orientation', isVertical ? 'vertical' : 'horizontal');
        this.classList.toggle('ide-splitter-v', isVertical);
        this.classList.toggle('ide-splitter-h', !isVertical);
    }

    get targetPane() {
        return this.for || '';
    }

    get isVertical() {
        return this.orientation === 'vertical';
    }

    _emit(type, detail = {}) {
        emitEvent(this, `ide-splitter-${type}`, {
            for: this.targetPane,
            orientation: this.isVertical ? 'vertical' : 'horizontal',
            ...detail,
        });
    }
}

const makeIcon = (paths) => svg`<svg viewBox="0 0 24 24" fill="none" stroke="currentColor"
    stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round"
    aria-hidden="true">${paths}</svg>`;

/**
 * Tab icon SVG glyphs for IDE panels.
 * Standard 24x24 viewBox stroke glyphs in Feather / Lucide style.
 */
export const TabIcon = Object.freeze({
    LAYOUT:           makeIcon(svg`<rect x="2.5" y="2.5" width="19" height="19" rx="2"/><path d="M2.5 9h19M9 2.5v19"/>`),
    SCHEMATIC:        makeIcon(svg`<circle cx="6" cy="18" r="2.5"/><circle cx="18" cy="6" r="2.5"/><path d="M6 15.5V8a2 2 0 012-2h7.5"/>`),
    VIEWER_3D:        makeIcon(svg`<path d="M12 2.5l9 5v9l-9 5-9-5v-9z"/><path d="M12 12l9-4.5M12 12v9.5M12 12L3 7.5"/>`),
    DISPLAY_CONTROLS: makeIcon(svg`<path d="M3 6h18M3 12h18M3 18h18"/><circle cx="8" cy="6" r="2"/><circle cx="15" cy="12" r="2"/><circle cx="10" cy="18" r="2"/>`),
    INSPECTOR:        makeIcon(svg`<path d="M4 6h16M4 12h16M4 18h10"/>`),
    HIERARCHY:        makeIcon(svg`<path d="M5 4v13a2 2 0 002 2h4M5 10h6"/><rect x="13" y="2" width="7" height="5" rx="1"/><rect x="13" y="15" width="7" height="5" rx="1"/>`),
    TIMING:           makeIcon(svg`<path d="M2 14h5l3-8 4 14 3-6h5"/>`),
    DRC:              makeIcon(svg`<path d="M12 3l9 16H3z"/><path d="M12 10v4M12 17h.01"/>`),
    SELECT_HIGHLIGHT: makeIcon(svg`<path d="M4 3l14 9-6 1 4 7-3 2-4-7-5 4z"/>`),
    CLOCK_TREE:       makeIcon(svg`<circle cx="12" cy="12" r="9"/><path d="M12 7v5l3.5 2"/>`),
    CHARTS:           makeIcon(svg`<path d="M4 20V10M10 20V4M16 20v-7M22 20V7"/>`),
    TCL_CONSOLE:      makeIcon(svg`<path d="M5 8l4 4-4 4M12 16h7"/>`),
    HELP:             makeIcon(svg`<circle cx="12" cy="12" r="9"/><path d="M9.5 9.5a2.5 2.5 0 115 .5c0 1.5-2.5 2-2.5 3.5M12 17h.01"/>`),
});

/**
 * <ide-tab> - Encapsulated IDE tab custom element using Lit and Shadow DOM.
 * Manages its title label, icon, active state, accessibility attributes, and optional close action.
 */
export class IdeTabElement extends LitElement {
    static properties = {
        title: { type: String, reflect: true },
        closable: { type: Boolean, reflect: true },
        for: { type: String },
        active: { type: Boolean, reflect: true },
        icon: { attribute: false },
    };

    static styles = css`
        :host {
            display: inline-flex;
            align-items: center;
            gap: var(--space-sm, 6px);
            height: 26px;
            margin-top: 1px;
            padding: 0 10px;
            background: transparent;
            color: var(--fg-muted);
            font-family: var(--font-sans);
            font-size: var(--fs-12);
            border-bottom: 2px solid transparent;
            border-radius: var(--radius-sm) var(--radius-sm) 0 0;
            cursor: pointer;
            white-space: nowrap;
            transition: background-color 120ms ease, color 120ms ease;
            box-sizing: border-box;
            user-select: none;
        }

        :host([hidden]) {
            display: none !important;
        }

        :host(:hover) {
            background: var(--bg-hover);
            color: var(--fg-secondary);
        }

        :host([active]) {
            background: var(--bg-panel);
            color: var(--fg-bright);
            border-bottom: 2px solid var(--accent-tab);
        }

        .ide-tab-icon,
        ::slotted([slot="icon"]),
        .or-tab-icon {
            display: inline-flex;
            align-items: center;
            justify-content: center;
            flex: none;
            width: 13px;
            height: 13px;
            opacity: 0.7;
        }

        :host([active]) .ide-tab-icon,
        :host([active]) ::slotted([slot="icon"]),
        :host([active]) .or-tab-icon {
            color: var(--accent-tab);
            opacity: 1;
        }

        .ide-tab-icon svg,
        ::slotted([slot="icon"]) svg,
        .or-tab-icon svg {
            width: 100%;
            height: 100%;
            display: block;
        }

        .ide-tab-title {
            user-select: none;
            padding-top: 0;
        }

        .ide-tab-close {
            display: inline-flex;
            align-items: center;
            justify-content: center;
            width: 14px;
            height: 14px;
            border-radius: var(--radius-xs);
            margin-left: 4px;
            font-size: 10px;
            line-height: 1;
            opacity: 0.5;
            cursor: pointer;
            transition: opacity 120ms ease, background-color 120ms ease, color 120ms ease;
        }

        .ide-tab-close:hover {
            opacity: 1;
            background: var(--bg-hover);
            color: var(--sem-error);
        }
    `;

    constructor() {
        super();
        this._connectedAbort = null;
        this.icon = null;
        this._onClick = this._onClick.bind(this);
        this._onCloseClick = this._onCloseClick.bind(this);
    }

    connectedCallback() {
        super.connectedCallback();
        this._connectedAbort = new AbortController();
        const { signal } = this._connectedAbort;

        this.classList.add('ide-tab');
        this.setAttribute('role', 'tab');
        if (!this.hasAttribute('aria-selected')) {
            this.setAttribute('aria-selected', 'false');
        }
        this.addEventListener('click', this._onClick, { signal });
    }

    disconnectedCallback() {
        super.disconnectedCallback();
        this._connectedAbort?.abort();
    }

    _onClick(e) {
        const path = e.composedPath ? e.composedPath() : [e.target];
        if (path.some(el => el.classList?.contains('ide-tab-close'))) return;
        emitEvent(this, 'tab-select', { componentType: this.targetComponent });
    }

    _onCloseClick(e) {
        e.stopPropagation();
        emitEvent(this, 'tab-close', { componentType: this.targetComponent });
    }

    get targetComponent() {
        return this.for || this.getAttribute('data-component') || this.id.replace(/^tab-/, '') || '';
    }

    setActive(isActive) {
        this.active = !!isActive;
    }

    updated(changedProperties) {
        super.updated(changedProperties);
        if (changedProperties.has('active')) {
            this.setAttribute('aria-selected', this.active ? 'true' : 'false');
        }
    }

    render() {
        return html`
            ${this.icon ? html`<span class="ide-tab-icon or-tab-icon">${this.icon}</span>` : nothing}
            <span class="ide-tab-title">${this.title || ''}</span>
            ${this.closable ? html`<span class="ide-tab-close" title="Close" @click=${this._onCloseClick}>✕</span>` : nothing}
        `;
    }
}

// Register custom elements
if (!customElements.get('ide-pane')) customElements.define('ide-pane', IdePaneElement);
if (!customElements.get('ide-splitter')) customElements.define('ide-splitter', IdeSplitterElement);
if (!customElements.get('ide-tab')) customElements.define('ide-tab', IdeTabElement);

export const COMPONENT_CONFIG = {
    DisplayControls: { pane: 'left',   title: 'Display Controls', icon: TabIcon.DISPLAY_CONTROLS, closable: false },
    LayoutViewer:    { pane: 'main',   title: 'Layout',           icon: TabIcon.LAYOUT,           closable: false },
    SchematicWidget: { pane: 'main',   title: 'Schematic',        icon: TabIcon.SCHEMATIC },
    '3DViewer':      { pane: 'main',   title: '3D Viewer',        icon: TabIcon.VIEWER_3D },
    TclConsole:      { pane: 'bottom', title: 'Tcl Console',      icon: TabIcon.TCL_CONSOLE,      closable: false },
    Inspector:       { pane: 'right',  title: 'Inspector',        icon: TabIcon.INSPECTOR },
    Browser:         { pane: 'right',  title: 'Hierarchy',        icon: TabIcon.HIERARCHY },
    TimingWidget:    { pane: 'right',  title: 'Timing',           icon: TabIcon.TIMING },
    DRCWidget:       { pane: 'right',  title: 'DRC',              icon: TabIcon.DRC },
    SelectHighlight: { pane: 'right',  title: 'Select Highlight', icon: TabIcon.SELECT_HIGHLIGHT },
    ClockWidget:     { pane: 'right',  title: 'Clock Tree',       icon: TabIcon.CLOCK_TREE },
    ChartsWidget:    { pane: 'right',  title: 'Charts',           icon: TabIcon.CHARTS },
    HelpWidget:      { pane: 'right',  title: 'Help',             icon: TabIcon.HELP },
};

export const DEFAULT_ACTIVE_TABS = {
    left: 'DisplayControls',
    main: 'LayoutViewer',
    bottom: 'TclConsole',
    right: 'Inspector',
};

/**
 * Sizing bounds, default dimensions, and drag characteristics for IDE splitters.
 */
export const SPLITTER_CONFIG = {
    left:   { cssVar: '--pane-left-width',     defaultSize: 240, minSize: 140, maxFraction: 0.45 },
    right:  { cssVar: '--pane-right-width',    defaultSize: 340, minSize: 160, maxFraction: 0.50, invertDelta: true },
    bottom: { cssVar: '--pane-bottom-height',  defaultSize: 220, minSize: 70,  maxFraction: 0.75, invertDelta: true },
};

export class GridLayoutManager extends EventEmitter {
    constructor(containerEl) {
        super();
        this.containerEl = containerEl;
        this.factories = new Map();
        this.components = new Map();
        this.activeTabs = {
            left: null,
            main: null,
            bottom: null,
            right: null,
        };
        this.closedComponents = new Set();
        this._resizeScheduled = false;

        this._ensureDOM();
        this._initSplitters();
        this._initControls();
        this._initResizeObserver();
    }

    _ensureDOM() {
        this.containerEl.classList.add('ide-layout');

        // Look for existing pane elements (e.g. pre-rendered in index.html) or construct initial DOM
        if (!this.containerEl.querySelector('#pane-left')) {
            this._buildInitialLayoutTree();
        }

        this._resolvePaneReferences();
    }

    /**
     * Programmatically builds the initial IDE layout tree using Custom Elements
     * when initializing with an empty container (e.g. dynamic instantiation or unit tests).
     */
    _buildInitialLayoutTree() {
        this.containerEl.innerHTML = `
            <button id="gutter-expand-left" class="ide-gutter-expand ide-gutter-left" title="Restore Display Controls">
                <span>▶ Display Controls</span>
            </button>
            <button id="gutter-expand-right" class="ide-gutter-expand ide-gutter-right" title="Restore Right Panel">
                <span>◀ Tools &amp; Inspector</span>
            </button>
            <ide-pane id="pane-left" name="left"></ide-pane>
            <ide-splitter id="splitter-left" for="left" orientation="vertical" title="Drag to resize panel"></ide-splitter>
            <main id="pane-center" class="ide-center">
                <ide-pane id="pane-main" name="main" maximizable></ide-pane>
                <ide-splitter id="splitter-bottom" for="bottom" orientation="horizontal" title="Drag to resize console"></ide-splitter>
                <button id="gutter-expand-bottom" class="ide-gutter-expand ide-gutter-bottom" title="Restore Tcl Console">
                    <span>▲ Tcl Console</span>
                </button>
                <ide-pane id="pane-bottom" name="bottom" maximizable collapsible></ide-pane>
            </main>
            <ide-splitter id="splitter-right" for="right" orientation="vertical" title="Drag to resize panel"></ide-splitter>
            <ide-pane id="pane-right" name="right" maximizable collapsible></ide-pane>
        `;
    }

    _resolvePaneReferences() {
        const paneNames = ['left', 'main', 'bottom', 'right'];
        this.panes = {};

        for (const name of paneNames) {
            const el = this.containerEl.querySelector(`#pane-${name}`);
            if (el) {
                el.el = el;
                el.paneComponent = el;
            }
            this.panes[name] = el;
        }

        this.centerEl = this.containerEl.querySelector('#pane-center');
    }

    _initSplitters() {
        let dragInfo = null;

        this.containerEl.addEventListener('ide-splitter-start', (e) => {
            const paneName = e.detail?.for;
            const cfg = SPLITTER_CONFIG[paneName];
            if (!cfg) return;

            const isVertical = (e.detail?.orientation || 'vertical') === 'vertical';
            const referenceEl = !isVertical
                ? (this.centerEl || this.containerEl)
                : this.containerEl;
            const containerRect = referenceEl.getBoundingClientRect();
            const paneRect = this.panes[paneName]?.getBoundingClientRect() || {};

            const containerSpan = isVertical ? containerRect.width : containerRect.height;
            const currentSpan = isVertical
                ? (paneRect.width || cfg.defaultSize)
                : (paneRect.height || cfg.defaultSize);

            dragInfo = {
                paneName,
                cfg,
                startSize: currentSpan,
                maxLimit: Math.round(containerSpan * cfg.maxFraction) || 500,
            };
        });

        this.containerEl.addEventListener('ide-splitter-drag', (e) => {
            if (!dragInfo || dragInfo.paneName !== e.detail?.for) return;
            const { cfg, startSize, maxLimit } = dragInfo;

            const delta = cfg.invertDelta ? -e.detail.delta : e.detail.delta;
            const newSize = Math.round(startSize + delta);
            const clamped = Math.max(cfg.minSize, Math.min(maxLimit, newSize));

            this.containerEl.style.setProperty(cfg.cssVar, `${clamped}px`);
            this.notifyResize();
        });

        this.containerEl.addEventListener('ide-splitter-end', () => {
            dragInfo = null;
            this.saveLayout();
            this.notifyResize();
        });

        this.containerEl.addEventListener('ide-splitter-reset', (e) => {
            const paneName = e.detail?.for;
            this._resetSplitterPane(paneName);
        });
    }

    _resetSplitterPane(paneName) {
        const cfg = SPLITTER_CONFIG[paneName];
        if (!cfg) return;

        this.containerEl.style.setProperty(cfg.cssVar, `${cfg.defaultSize}px`);
        if (this.isPaneCollapsed(paneName)) {
            this.restorePane(paneName);
        } else {
            this.saveLayout();
            this.notifyResize();
        }
    }

    _initControls() {
        this.containerEl.addEventListener('ide-pane-maximize', (e) => {
            this.toggleMaximize(e.detail?.pane);
        });

        this.containerEl.addEventListener('ide-pane-collapse', (e) => {
            this.toggleCollapse(e.detail?.pane);
        });

        this.containerEl.addEventListener('ide-layout-reset', () => {
            this.resetLayout();
        });

        // Wire gutter expand affordance buttons
        for (const side of ['left', 'right', 'bottom']) {
            const gutter = this.containerEl.querySelector(`#gutter-expand-${side}`);
            if (gutter) {
                gutter.addEventListener('click', () => this.restorePane(side));
            }
        }
    }

    /**
     * Checks if a given pane (or any pane if paneName is omitted) is currently maximized.
     */
    isPaneMaximized(paneName) {
        if (!paneName) {
            return ['main', 'bottom', 'right'].some(name => !!this.panes[name]?.maximized);
        }
        return !!this.panes[paneName]?.maximized;
    }

    /**
     * Sets the maximized state of a pane across the IDE workspace.
     * When shouldMaximize is true, the given pane expands to fill the entire workspace while
     * other tracks collapse to 0px. When false, normal workspace layout is restored.
     */
    setMaximizedPane(paneName, shouldMaximize = true) {
        const MAXIMIZABLE_PANES = ['main', 'bottom', 'right'];
        if (paneName && !MAXIMIZABLE_PANES.includes(paneName)) return false;

        const targetPane = shouldMaximize ? paneName : null;

        for (const name of MAXIMIZABLE_PANES) {
            const active = (name === targetPane);
            this.containerEl.classList.toggle(`maximized-${name}`, active);
            if (this.panes[name]) {
                this.panes[name].maximized = active;
            }
        }

        this.notifyResize();
        return !!targetPane;
    }

    /**
     * Maximizes a pane across the IDE workspace.
     */
    maximizePane(paneName) {
        return this.setMaximizedPane(paneName, true);
    }

    /**
     * Restores all maximized panes back to normal layout.
     */
    restoreMaximized() {
        return this.setMaximizedPane(null, false);
    }

    /**
     * Toggles the maximized state of a pane across the IDE workspace.
     */
    toggleMaximize(paneName) {
        return this.setMaximizedPane(paneName, !this.isPaneMaximized(paneName));
    }

    /**
     * Checks if a given pane is currently collapsed.
     */
    isPaneCollapsed(paneName) {
        const targetEl = (paneName === 'bottom') ? this.centerEl : this.containerEl;
        return !!this.panes[paneName]?.collapsed || (targetEl ? targetEl.classList.contains(`hide-${paneName}`) : false);
    }

    /**
     * Sets the collapsed state of a dockable pane, updating its container class,
     * header button label, custom element state, and triggering layout persistence.
     */
    _setPaneCollapsed(paneName, shouldCollapse) {
        const targetEl = (paneName === 'bottom') ? this.centerEl : this.containerEl;
        if (!targetEl) return;

        targetEl.classList.toggle(`hide-${paneName}`, shouldCollapse);
        if (this.panes[paneName]) {
            this.panes[paneName].collapsed = shouldCollapse;
        }

        this.saveLayout();
        this.notifyResize();
    }

    toggleCollapse(paneName) {
        this._setPaneCollapsed(paneName, !this.isPaneCollapsed(paneName));
    }

    collapsePane(paneName) {
        this._setPaneCollapsed(paneName, true);
    }

    restorePane(paneName) {
        this._setPaneCollapsed(paneName, false);
    }

    resetLayout() {
        try {
            localStorage.removeItem('or_ide_layout');
        } catch (_) {}

        for (const [paneName, cfg] of Object.entries(SPLITTER_CONFIG)) {
            this.containerEl.style.setProperty(cfg.cssVar, `${cfg.defaultSize}px`);
        }

        for (const name of ['left', 'right', 'bottom']) {
            this.restorePane(name);
        }

        this.restoreMaximized();

        this.closedComponents.clear();
        for (const comp of this.components.values()) {
            comp.tabEl.hidden = false;
            comp.contentDiv.hidden = false;
        }

        for (const compType of Object.values(DEFAULT_ACTIVE_TABS)) {
            if (this.components.has(compType)) this.activateTab(compType);
        }

        this.saveLayout();
        this.notifyResize();
        this.emit('stateChanged');
    }

    _initResizeObserver() {
        if (typeof ResizeObserver !== 'undefined') {
            const ro = new ResizeObserver(() => {
                this.notifyResize();
            });
            ro.observe(this.containerEl);
        }
    }

    registerComponentFactoryFunction(componentType, factoryFn) {
        this.factories.set(componentType, factoryFn);
    }

    loadLayout(config) {
        for (const componentType of Object.keys(COMPONENT_CONFIG)) {
            const factory = this.factories.get(componentType);
            if (!factory) continue;
            this._mountComponent(componentType, factory);
        }

        let savedState = null;
        try {
            const raw = localStorage.getItem('or_ide_layout');
            if (raw) savedState = JSON.parse(raw);
        } catch (_) {}

        if (savedState) {
            if (savedState.leftWidth) {
                this.containerEl.style.setProperty('--pane-left-width', `${savedState.leftWidth}px`);
            }
            if (savedState.rightWidth) {
                this.containerEl.style.setProperty('--pane-right-width', `${savedState.rightWidth}px`);
            }
            if (savedState.bottomHeight) {
                this.containerEl.style.setProperty('--pane-bottom-height', `${savedState.bottomHeight}px`);
            }
            if (savedState.closed) {
                for (const c of savedState.closed) {
                    this.closeComponent(c, false);
                }
            }

            for (const side of ['left', 'right', 'bottom']) {
                const key = `hide${side[0].toUpperCase()}${side.slice(1)}`;
                if (savedState[key]) this.collapsePane(side);
            }
        }

        for (const [pane, defaultType] of Object.entries(DEFAULT_ACTIVE_TABS)) {
            this.activateTab(savedState?.activeTabs?.[pane] || defaultType);
        }

        this.notifyResize();
        this.emit('stateChanged');
    }

    _mountComponent(componentType, factory) {
        if (this.components.has(componentType)) return;

        const cfg = COMPONENT_CONFIG[componentType] || {
            pane: 'right',
            title: componentType,
            closable: true,
        };
        const pane = this.panes[cfg.pane];
        if (!pane) return;

        const contentDiv = document.createElement('div');
        contentDiv.className = 'ide-component-container';
        contentDiv.id = `cmp-${componentType}`;
        pane.appendChild(contentDiv);

        const closable = cfg.closable !== false;
        const tabEl = document.createElement('ide-tab');
        tabEl.id = `tab-${componentType}`;
        tabEl.slot = 'tabs';
        tabEl.for = componentType;
        tabEl.setAttribute('data-component', componentType);
        tabEl.title = cfg.title;
        if (closable) tabEl.setAttribute('closable', '');
        if (cfg.icon) tabEl.icon = cfg.icon;

        tabEl.addEventListener('tab-select', () => this.activateTab(componentType));
        tabEl.addEventListener('tab-close', () => this.closeComponent(componentType));
        pane.appendChild(tabEl);

        const container = Object.assign(new EventEmitter(), {
            element: contentDiv,
            get width() { return contentDiv.clientWidth; },
            get height() { return contentDiv.clientHeight; },
            title: cfg.title,
            focus: () => this.focusComponent(componentType),
            close: () => this.closeComponent(componentType),
        });

        const comp = {
            componentType,
            paneName: cfg.pane,
            title: cfg.title,
            closable,
            tabEl,
            contentDiv,
            container,
        };
        this.components.set(componentType, comp);

        try {
            factory(container);
        } catch (err) {
            console.error(`Failed to initialize component ${componentType}:`, err);
        }
    }

    /**
     * Checks if a component's content view is currently active.
     */
    isComponentActive(comp) {
        if (!comp) return false;
        return this.activeTabs[comp.paneName] === comp.componentType;
    }

    /**
     * Sets a component's tab and content view active or inactive, updating
     * accessibility attributes and class names.
     */
    _setComponentActive(comp, isActive) {
        if (!comp) return;
        const active = !!isActive;
        if (comp.tabEl) comp.tabEl.active = active;
        comp.contentDiv?.classList.toggle('active', active);
    }

    activateTab(componentType) {
        const comp = this.components.get(componentType);
        if (!comp) return;

        const pane = this.panes[comp.paneName];
        if (!pane) return;

        if (['left', 'right', 'bottom'].includes(comp.paneName) && this.isPaneCollapsed(comp.paneName)) {
            this.restorePane(comp.paneName);
        }

        const prevType = this.activeTabs[comp.paneName];
        if (prevType && prevType !== componentType) {
            const prevComp = this.components.get(prevType);
            if (prevComp) {
                this._setComponentActive(prevComp, false);
                prevComp.container.fire('hide');
            }
        }

        this.activeTabs[comp.paneName] = componentType;
        comp.tabEl.hidden = false;
        comp.contentDiv.hidden = false;
        this._setComponentActive(comp, true);
        this.closedComponents.delete(componentType);

        comp.container.fire('show');
        comp.container.fire('open');
        comp.container.fire('resize');

        try {
            comp.tabEl.scrollIntoView({ behavior: 'smooth', inline: 'nearest', block: 'nearest' });
        } catch (_) {}

        this.notifyResize();
        this.emit('stateChanged');
    }

    closeComponent(componentType, autoSelectNext = true) {
        const comp = this.components.get(componentType);
        if (!comp || !comp.closable) return;

        comp.tabEl.hidden = true;
        comp.contentDiv.hidden = true;
        this._setComponentActive(comp, false);
        comp.container.fire('hide');
        this.closedComponents.add(componentType);

        if (autoSelectNext && this.activeTabs[comp.paneName] === componentType) {
            let nextComp = null;
            for (const other of this.components.values()) {
                if (other.paneName === comp.paneName && !this.closedComponents.has(other.componentType)) {
                    nextComp = other;
                    break;
                }
            }
            if (nextComp) {
                this.activateTab(nextComp.componentType);
            } else {
                this.activeTabs[comp.paneName] = null;
                if (['left', 'right', 'bottom'].includes(comp.paneName)) {
                    this.collapsePane(comp.paneName);
                }
            }
        }

        this.notifyResize();
        this.emit('stateChanged');
    }

    focusComponent(componentType) {
        const comp = this.components.get(componentType);
        if (!comp) {
            const factory = this.factories.get(componentType);
            if (factory) {
                this._mountComponent(componentType, factory);
            } else {
                return;
            }
        }
        this.activateTab(componentType);
    }

    saveLayout() {
        const state = {
            leftWidth: parseInt(this.containerEl.style.getPropertyValue(SPLITTER_CONFIG.left.cssVar) || SPLITTER_CONFIG.left.defaultSize, 10),
            rightWidth: parseInt(this.containerEl.style.getPropertyValue(SPLITTER_CONFIG.right.cssVar) || SPLITTER_CONFIG.right.defaultSize, 10),
            bottomHeight: parseInt(this.containerEl.style.getPropertyValue(SPLITTER_CONFIG.bottom.cssVar) || SPLITTER_CONFIG.bottom.defaultSize, 10),
            hideLeft: this.isPaneCollapsed('left'),
            hideRight: this.isPaneCollapsed('right'),
            hideBottom: this.isPaneCollapsed('bottom'),
            activeTabs: { ...this.activeTabs },
            closed: Array.from(this.closedComponents),
        };
        try {
            localStorage.setItem('or_ide_layout', JSON.stringify(state));
        } catch (_) {}
        return state;
    }

    notifyResize() {
        if (this._resizeScheduled) return;
        this._resizeScheduled = true;
        const schedule = (typeof requestAnimationFrame === 'function')
            ? requestAnimationFrame
            : (cb) => { cb(); };
        schedule(() => {
            this._resizeScheduled = false;
            for (const comp of this.components.values()) {
                if (this.isComponentActive(comp)) {
                    comp.container.fire('resize');
                }
            }
            if (typeof app !== 'undefined' && app && app.map) {
                app.map.invalidateSize({ animate: false });
            }
            window.dispatchEvent(new Event('resize'));
        });
    }

    updateSize() {
        this.notifyResize();
    }

    setSize(w, h) {
        this.notifyResize();
    }

    addComponent(componentType, state, title) {
        this.focusComponent(componentType);
    }

    get rootItem() {
        return {
            contentItems: Array.from(this.components.keys()).map((componentType) => ({
                isComponent: true,
                componentType,
                focus: () => this.focusComponent(componentType),
            })),
        };
    }
}
