// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

import { LitElement, html } from 'lit';
import { ref, createRef } from 'lit/directives/ref.js';
import { when } from 'lit/directives/when.js';
import { ifDefined } from 'lit/directives/if-defined.js';

// ─── Native CSS Grid IDE Layout Manager & Custom Elements ────────────────────

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

    constructor() {
        super();
        this.tabsRef = createRef();
        this.contentRef = createRef();
        this.headerRef = createRef();
        this.controlsRef = createRef();
        this.maximizeBtnRef = createRef();
        this.collapseBtnRef = createRef();
        this._onMaximizeClick = this._onMaximizeClick.bind(this);
        this._onCollapseClick = this._onCollapseClick.bind(this);
        this._onResetLayoutClick = this._onResetLayoutClick.bind(this);
    }

    createRenderRoot() {
        return this;
    }

    connectedCallback() {
        super.connectedCallback();
        this.classList.add('ide-pane', `ide-pane-${this.paneName}`);
        this.performUpdate();
    }

    get paneName() {
        return this.name || this.getAttribute('name') || this.id.replace(/^pane-/, '') || 'pane';
    }

    get tabsEl() {
        if (!this.hasUpdated) this.performUpdate();
        return this.tabsRef.value;
    }

    get contentEl() {
        if (!this.hasUpdated) this.performUpdate();
        return this.contentRef.value;
    }

    get headerEl() {
        if (!this.hasUpdated) this.performUpdate();
        return this.headerRef.value;
    }

    get controlsEl() {
        if (!this.hasUpdated) this.performUpdate();
        return this.controlsRef.value;
    }

    get maximizeBtn() {
        if (!this.hasUpdated) this.performUpdate();
        return this.maximizeBtnRef.value;
    }

    get collapseBtn() {
        if (!this.hasUpdated) this.performUpdate();
        return this.collapseBtnRef.value;
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

    _emit(type, detail = {}) {
        this.dispatchEvent(new CustomEvent(type, {
            bubbles: true,
            composed: true,
            detail,
        }));
    }

    _onMaximizeClick(e) {
        e.stopPropagation();
        this._emit('ide-pane-maximize', { pane: this.paneName });
    }

    _onCollapseClick(e) {
        e.stopPropagation();
        this._emit('ide-pane-collapse', { pane: this.paneName });
    }

    _onResetLayoutClick(e) {
        e.stopPropagation();
        this._emit('ide-layout-reset');
    }

    setMaximized(isMax) {
        this.maximized = !!isMax;
        this.classList.toggle('maximized', this.maximized);
        if (this.maximizeBtn) {
            this.maximizeBtn.textContent = this.maximized ? '⧉' : '⤢';
            this.maximizeBtn.title = this.maximized ? 'Restore' : 'Maximize';
        }
    }

    setCollapsed(isCollapsed) {
        this.collapsed = !!isCollapsed;
        this.classList.toggle('collapsed', this.collapsed);
        if (this.collapseBtn) {
            this.collapseBtn.textContent = this._collapseIcon;
            this.collapseBtn.title = this._collapseTitle;
        }
    }

    build() {
        if (!this.hasUpdated) this.performUpdate();
    }

    updated(changedProperties) {
        super.updated(changedProperties);
        if (changedProperties.has('maximized')) {
            this.classList.toggle('maximized', !!this.maximized);
        }
        if (changedProperties.has('collapsed')) {
            this.classList.toggle('collapsed', !!this.collapsed);
        }
    }

    render() {
        const name = this.paneName;
        const prefix = this.id || `pane-${name}`;
        const hasMax = this.maximizable || this.hasAttribute('maximizable');
        const hasCol = this.collapsible || this.hasAttribute('collapsible');

        return html`
            <div class="ide-header" id="${prefix}-header" ${ref(this.headerRef)}>
                <div class="ide-tabs" id="${prefix}-tabs" ${ref(this.tabsRef)}></div>
                <div class="ide-controls" id="${prefix}-controls" ${ref(this.controlsRef)}>
                    ${when(hasMax, () => html`
                        <button
                            class="ide-btn ide-btn-maximize"
                            id="btn-maximize-${name}"
                            title=${ifDefined(this.maximized ? 'Restore' : 'Maximize')}
                            ${ref(this.maximizeBtnRef)}
                            @click=${this._onMaximizeClick}
                        >${this.maximized ? '⧉' : '⤢'}</button>
                    `)}
                    ${when(hasCol, () => html`
                        <button
                            class="ide-btn ide-btn-collapse"
                            id="btn-collapse-${name}"
                            title=${ifDefined(this._collapseTitle)}
                            ${ref(this.collapseBtnRef)}
                            @click=${this._onCollapseClick}
                        >${this._collapseIcon}</button>
                    `)}
                </div>
            </div>
            <div class="ide-content" id="${prefix}-content" ${ref(this.contentRef)}>
                ${when(name === 'main', () => html`
                    <div id="ide-empty-state" class="ide-empty-state" style="display: none;">
                        <div class="ide-empty-icon">⧉</div>
                        <div class="ide-empty-title">All views are closed</div>
                        <div class="ide-empty-desc">Reopen a view from the Windows menu or restore the default layout.</div>
                        <button
                            id="btn-empty-reset-layout"
                            class="or-btn or-btn-primary"
                            @click=${this._onResetLayoutClick}
                        >Reset to Default Layout</button>
                    </div>
                `)}
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
        this.performUpdate();

        this.addEventListener('pointerdown', (e) => this.drag.start(e, this.isVertical), { signal });
        this.addEventListener('dblclick', () => this._emit('reset'), { signal });
    }

    disconnectedCallback() {
        super.disconnectedCallback();
        this._connectedAbort?.abort();
    }

    build() {
        // Idempotent backward-compatibility stub
    }

    updated(changedProperties) {
        super.updated(changedProperties);
        const isVertical = this.isVertical;
        this.setAttribute('aria-orientation', isVertical ? 'vertical' : 'horizontal');
        this.classList.toggle('ide-splitter-v', isVertical);
        this.classList.toggle('ide-splitter-h', !isVertical);
    }

    get targetPane() {
        return this.for || this.getAttribute('for') || '';
    }

    get isVertical() {
        return (this.orientation || this.getAttribute('orientation') || 'vertical') === 'vertical';
    }

    _emit(type, detail = {}) {
        this.dispatchEvent(new CustomEvent(`ide-splitter-${type}`, {
            bubbles: true,
            composed: true,
            detail: {
                for: this.targetPane,
                orientation: this.isVertical ? 'vertical' : 'horizontal',
                ...detail,
            },
        }));
    }
}

/**
 * <ide-tab> - Encapsulated IDE tab custom element using Lit (Light DOM).
 * Manages its title label, active state, accessibility attributes, and optional close action.
 */
export class IdeTabElement extends LitElement {
    static properties = {
        title: { type: String, reflect: true },
        closable: { type: Boolean, reflect: true },
        for: { type: String },
        active: { type: Boolean, reflect: true },
    };

    constructor() {
        super();
        this._connectedAbort = null;
        this.titleRef = createRef();
        this.closeBtnRef = createRef();
        this._onClick = this._onClick.bind(this);
        this._onCloseClick = this._onCloseClick.bind(this);
    }

    // Light DOM: Do not use Shadow DOM
    createRenderRoot() {
        return this;
    }

    connectedCallback() {
        super.connectedCallback();
        this._connectedAbort = new AbortController();
        const { signal } = this._connectedAbort;

        this.classList.add('ide-tab', 'lm_tab');
        this.setAttribute('role', 'tab');
        if (!this.hasAttribute('aria-selected')) {
            this.setAttribute('aria-selected', 'false');
        }
        this.addEventListener('click', this._onClick, { signal });
        this.performUpdate();
    }

    disconnectedCallback() {
        super.disconnectedCallback();
        this._connectedAbort?.abort();
    }

    _emit(type, detail = {}) {
        this.dispatchEvent(new CustomEvent(type, {
            bubbles: true,
            composed: true,
            detail,
        }));
    }

    _onClick(e) {
        if (e && e.target && e.target.closest('.ide-tab-close')) return;
        this._emit('tab-select', { componentType: this.targetComponent });
    }

    _onCloseClick(e) {
        e.stopPropagation();
        this._emit('tab-close', { componentType: this.targetComponent });
    }

    get targetComponent() {
        return this.for || this.getAttribute('for') || this.getAttribute('data-component') || this.id.replace(/^tab-/, '') || '';
    }

    get titleText() {
        return this.title || '';
    }

    set titleText(value) {
        this.title = value;
    }

    get titleEl() {
        if (!this.hasUpdated) this.performUpdate();
        return this.titleRef.value;
    }

    get closeBtn() {
        if (!this.hasUpdated) this.performUpdate();
        return this.closeBtnRef.value;
    }

    setActive(isActive) {
        this.active = !!isActive;
        this.performUpdate();
    }

    updated(changedProperties) {
        super.updated(changedProperties);
        if (changedProperties.has('active')) {
            this.classList.toggle('active', this.active);
            this.classList.toggle('lm_active', this.active);
            this.setAttribute('aria-selected', this.active ? 'true' : 'false');
        }
    }

    render() {
        const title = this.title || '';
        const isClosable = Boolean(this.closable);

        return html`
            <span class="ide-tab-title lm_title" ${ref(this.titleRef)}>${title}</span>
            ${when(isClosable, () => html`
                <span
                    class="ide-tab-close lm_close_tab"
                    title="Close"
                    ${ref(this.closeBtnRef)}
                    @click=${this._onCloseClick}
                >✕</span>
            `)}
        `;
    }
}

// Register custom elements
if (!customElements.get('ide-pane')) customElements.define('ide-pane', IdePaneElement);
if (!customElements.get('ide-splitter')) customElements.define('ide-splitter', IdeSplitterElement);
if (!customElements.get('ide-tab')) customElements.define('ide-tab', IdeTabElement);

export const COMPONENT_CONFIG = {
    DisplayControls: { pane: 'left',   title: 'Display Controls', closable: false },
    LayoutViewer:    { pane: 'main',   title: 'Layout',           closable: false },
    SchematicWidget: { pane: 'main',   title: 'Schematic',        closable: true },
    '3DViewer':      { pane: 'main',   title: '3D Viewer',        closable: true },
    TclConsole:      { pane: 'bottom', title: 'Tcl Console',      closable: false },
    Inspector:       { pane: 'right',  title: 'Inspector',        closable: true },
    Browser:         { pane: 'right',  title: 'Hierarchy',        closable: true },
    TimingWidget:    { pane: 'right',  title: 'Timing',           closable: true },
    DRCWidget:       { pane: 'right',  title: 'DRC',              closable: true },
    SelectHighlight: { pane: 'right',  title: 'Select Highlight', closable: true },
    ClockWidget:     { pane: 'right',  title: 'Clock Tree',       closable: true },
    ChartsWidget:    { pane: 'right',  title: 'Charts',           closable: true },
    HelpWidget:      { pane: 'right',  title: 'Help',             closable: true },
};

/**
 * Sizing bounds, default dimensions, and drag characteristics for IDE splitters.
 */
export const SPLITTER_CONFIG = {
    left: {
        cssVar: '--pane-left-width',
        defaultSize: '240px',
        minSize: 140,
        maxFraction: 0.45,
        dimension: 'width',
        isVertical: true,
        invertDelta: false,
    },
    right: {
        cssVar: '--pane-right-width',
        defaultSize: '340px',
        minSize: 160,
        maxFraction: 0.5,
        dimension: 'width',
        isVertical: true,
        invertDelta: true,
    },
    bottom: {
        cssVar: '--pane-bottom-height',
        defaultSize: '220px',
        minSize: 70,
        maxFraction: 0.75,
        dimension: 'height',
        isVertical: false,
        invertDelta: true,
    },
};

export class GridLayoutManager {
    constructor(containerEl) {
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
        this.eventListeners = new Map();
        this._resizeScheduled = false;

        this._ensureDOM();
        this._initSplitters();
        this._initControls();
        this._initResizeObserver();
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
        // 1. Gutter expand affordances for collapsed panels
        this.containerEl.appendChild(this._createGutterButton('left', '▶ Display Controls', 'Restore Display Controls'));
        this.containerEl.appendChild(this._createGutterButton('right', '◀ Tools & Inspector', 'Restore Right Panel'));

        // 2. Left Pane & Splitter
        this.containerEl.appendChild(this._createPane('pane-left', 'left'));
        this.containerEl.appendChild(this._createSplitter('splitter-left', 'left', 'vertical', 'Drag to resize panel'));

        // 3. Center Area (Main Viewer + Console)
        const center = document.createElement('main');
        center.id = 'pane-center';
        center.className = 'ide-center';

        center.appendChild(this._createPane('pane-main', 'main', { maximizable: true }));
        center.appendChild(this._createSplitter('splitter-bottom', 'bottom', 'horizontal', 'Drag to resize console'));
        center.appendChild(this._createGutterButton('bottom', '▲ Tcl Console', 'Restore Tcl Console'));
        center.appendChild(this._createPane('pane-bottom', 'bottom', { maximizable: true, collapsible: true }));

        this.containerEl.appendChild(center);

        // 4. Right Splitter & Pane
        this.containerEl.appendChild(this._createSplitter('splitter-right', 'right', 'vertical', 'Drag to resize panel'));
        this.containerEl.appendChild(this._createPane('pane-right', 'right', { maximizable: true, collapsible: true }));
    }

    _createGutterButton(side, text, title) {
        const btn = document.createElement('button');
        btn.id = `gutter-expand-${side}`;
        btn.className = `ide-gutter-expand ide-gutter-${side}`;
        btn.title = title;
        const span = document.createElement('span');
        span.textContent = text;
        btn.appendChild(span);
        return btn;
    }

    _createPane(id, name, options = {}) {
        const pane = document.createElement('ide-pane');
        pane.id = id;
        pane.setAttribute('name', name);
        if (options.maximizable) pane.setAttribute('maximizable', '');
        if (options.collapsible) pane.setAttribute('collapsible', '');
        return pane;
    }

    _createSplitter(id, forPane, orientation, title) {
        const splitter = document.createElement('ide-splitter');
        splitter.id = id;
        splitter.setAttribute('for', forPane);
        splitter.setAttribute('orientation', orientation);
        splitter.title = title;
        return splitter;
    }

    _resolvePaneReferences() {
        const paneNames = ['left', 'main', 'bottom', 'right'];
        this.panes = {};

        for (const name of paneNames) {
            const el = this.containerEl.querySelector(`#pane-${name}`);
            if (el?.build) el.build();

            this.panes[name] = {
                el,
                paneComponent: el,
                tabsEl: el?.tabsEl || this.containerEl.querySelector(`#pane-${name}-tabs`),
                contentEl: el?.contentEl || this.containerEl.querySelector(`#pane-${name}-content`),
            };
        }

        this.centerEl = this.containerEl.querySelector('#pane-center');
    }

    _initSplitters() {
        let dragInfo = null;

        this.containerEl.addEventListener('ide-splitter-start', (e) => {
            const paneName = e.detail?.for;
            const cfg = SPLITTER_CONFIG[paneName];
            if (!cfg) return;

            const referenceEl = paneName === 'bottom'
                ? (this.centerEl || this.containerEl)
                : this.containerEl;
            const containerRect = referenceEl.getBoundingClientRect();
            const paneRect = this.panes[paneName]?.el?.getBoundingClientRect() || {};

            const containerSpan = cfg.dimension === 'width' ? containerRect.width : containerRect.height;
            const currentSpan = cfg.dimension === 'width' ? (paneRect.width || 240) : (paneRect.height || 220);

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

        this._setupFallbackSplitters();
    }

    _resetSplitterPane(paneName) {
        const cfg = SPLITTER_CONFIG[paneName];
        if (!cfg) return;

        this.containerEl.style.setProperty(cfg.cssVar, cfg.defaultSize);
        if (this.isPaneCollapsed(paneName)) {
            this.restorePane(paneName);
        } else {
            this.saveLayout();
            this.notifyResize();
        }
    }

    _setupFallbackSplitters() {
        for (const [paneName, cfg] of Object.entries(SPLITTER_CONFIG)) {
            const splitterEl = this.containerEl.querySelector(`#splitter-${paneName}`);
            if (!splitterEl || splitterEl instanceof IdeSplitterElement) continue;

            const isLeftOrTop = !cfg.invertDelta;
            const isVertical = cfg.isVertical;

            splitterEl.addEventListener('pointerdown', (e) => {
                e.preventDefault();
                try { splitterEl.setPointerCapture(e.pointerId); } catch (_) {}
                splitterEl.classList.add('dragging');
                if (document.body) {
                    document.body.classList.add('is-resizing', isVertical ? 'is-resizing-v' : 'is-resizing-h');
                }

                const startPos = isVertical ? e.clientX : e.clientY;
                const referenceEl = paneName === 'bottom' ? (this.centerEl || this.containerEl) : this.containerEl;
                const cRect = referenceEl.getBoundingClientRect();
                const pRect = this.panes[paneName]?.el?.getBoundingClientRect() || {};
                const currentSize = cfg.dimension === 'width' ? (pRect.width || 240) : (pRect.height || 220);
                const containerSpan = cfg.dimension === 'width' ? cRect.width : cRect.height;
                const maxLimit = Math.round(containerSpan * cfg.maxFraction) || 500;

                const dragAbort = new AbortController();
                const { signal } = dragAbort;

                const onMove = (ev) => {
                    const currentPos = isVertical ? ev.clientX : ev.clientY;
                    const delta = isLeftOrTop ? (currentPos - startPos) : (startPos - currentPos);
                    const newSize = Math.round(currentSize + delta);
                    const clamped = Math.max(cfg.minSize, Math.min(maxLimit, newSize));
                    this.containerEl.style.setProperty(cfg.cssVar, `${clamped}px`);
                    this.notifyResize();
                };

                const onUp = (ev) => {
                    try { splitterEl.releasePointerCapture(ev.pointerId); } catch (_) {}
                    splitterEl.classList.remove('dragging');
                    if (document.body) {
                        document.body.classList.remove('is-resizing', 'is-resizing-v', 'is-resizing-h');
                    }
                    dragAbort.abort();
                    this.saveLayout();
                    this.notifyResize();
                };

                window.addEventListener('pointermove', onMove, { signal });
                window.addEventListener('pointerup', onUp, { signal });
                window.addEventListener('pointercancel', onUp, { signal });
            });

            splitterEl.addEventListener('dblclick', () => {
                this._resetSplitterPane(paneName);
            });
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

        // Fallback listeners for controls not inside an <ide-pane> custom element
        const fallbackControls = [
            { sel: '#btn-collapse-left', handler: () => this.toggleCollapse('left') },
            { sel: '#btn-collapse-right', handler: () => this.toggleCollapse('right') },
            { sel: '#btn-collapse-bottom', handler: () => this.toggleCollapse('bottom') },
            { sel: '#btn-maximize-main', handler: () => this.toggleMaximize('main') },
            { sel: '#btn-maximize-bottom', handler: () => this.toggleMaximize('bottom') },
            { sel: '#btn-maximize-right', handler: () => this.toggleMaximize('right') },
            { sel: '#btn-empty-reset-layout', handler: () => this.resetLayout() },
        ];

        for (const { sel, handler } of fallbackControls) {
            const el = this.containerEl.querySelector(sel);
            if (el && !el.closest('ide-pane')) {
                el.addEventListener('click', handler);
            }
        }
    }

    /**
     * Checks if a given pane (or any pane if paneName is omitted) is currently maximized.
     */
    isPaneMaximized(paneName) {
        if (!paneName) {
            return this.containerEl.classList.contains('maximized-main')
                || this.containerEl.classList.contains('maximized-bottom')
                || this.containerEl.classList.contains('maximized-right');
        }
        return this.containerEl.classList.contains(`maximized-${paneName}`);
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
            this.panes[name]?.paneComponent?.setMaximized(active);

            const btn = this.containerEl.querySelector(`#btn-maximize-${name}`);
            if (btn) {
                btn.textContent = active ? '⧉' : '⤢';
                btn.title = active ? 'Restore' : 'Maximize';
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
        return targetEl ? targetEl.classList.contains(`hide-${paneName}`) : false;
    }

    /**
     * Sets the collapsed state of a dockable pane, updating its container class,
     * header button label, custom element state, and triggering layout persistence.
     */
    _setPaneCollapsed(paneName, shouldCollapse) {
        const targetEl = (paneName === 'bottom') ? this.centerEl : this.containerEl;
        if (!targetEl) return;

        targetEl.classList.toggle(`hide-${paneName}`, shouldCollapse);
        this.panes[paneName]?.paneComponent?.setCollapsed?.(shouldCollapse);

        // Update header collapse button icon if present outside or inside the pane
        const btn = this.containerEl.querySelector(`#btn-collapse-${paneName}`);
        if (btn) {
            const icons = {
                bottom: { collapsed: '▲', expanded: '▼' },
                left:   { collapsed: '▶', expanded: '◀' },
                right:  { collapsed: '◀', expanded: '▶' },
            };
            const pair = icons[paneName] || { collapsed: '▶', expanded: '◀' };
            btn.textContent = shouldCollapse ? pair.collapsed : pair.expanded;
            btn.title = shouldCollapse
                ? `Expand ${paneName.charAt(0).toUpperCase() + paneName.slice(1)}`
                : `Collapse ${paneName.charAt(0).toUpperCase() + paneName.slice(1)}`;
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

    _updateEmptyState() {
        const emptyStateEl = this.containerEl.querySelector('#ide-empty-state');
        if (!emptyStateEl) return;
        const hasActiveMain = !!this.activeTabs.main;
        emptyStateEl.style.display = hasActiveMain ? 'none' : 'flex';
    }

    resetLayout() {
        try {
            localStorage.removeItem('or_ide_layout');
        } catch (_) {}

        for (const [paneName, cfg] of Object.entries(SPLITTER_CONFIG)) {
            this.containerEl.style.setProperty(cfg.cssVar, cfg.defaultSize);
        }

        for (const name of ['left', 'right', 'bottom']) {
            this.restorePane(name);
        }

        this.restoreMaximized();

        this.closedComponents.clear();
        for (const comp of this.components.values()) {
            comp.tabEl.style.display = '';
            comp.contentDiv.style.display = '';
        }

        if (this.components.has('DisplayControls')) this.activateTab('DisplayControls');
        if (this.components.has('LayoutViewer')) this.activateTab('LayoutViewer');
        if (this.components.has('TclConsole')) this.activateTab('TclConsole');
        if (this.components.has('Inspector')) this.activateTab('Inspector');

        this._updateEmptyState();
        this.saveLayout();
        this.notifyResize();
        this.emit('stateChanged');
        if (typeof app !== 'undefined' && app.map) {
            setTimeout(() => app.map.invalidateSize({ animate: false }), 0);
        }
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
        for (const [componentType, cfg] of Object.entries(COMPONENT_CONFIG)) {
            const factory = this.factories.get(componentType);
            if (!factory) continue;
            this._mountComponent(componentType, cfg.pane, cfg.title, cfg.closable, factory);
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

            this.activateTab(savedState.activeTabs?.left || 'DisplayControls');
            this.activateTab(savedState.activeTabs?.main || 'LayoutViewer');
            this.activateTab(savedState.activeTabs?.bottom || 'TclConsole');
            this.activateTab(savedState.activeTabs?.right || 'Inspector');

            if (savedState.hideLeft) this.collapsePane('left');
            if (savedState.hideRight) this.collapsePane('right');
            if (savedState.hideBottom) this.collapsePane('bottom');
        } else {
            this.activateTab('DisplayControls');
            this.activateTab('LayoutViewer');
            this.activateTab('TclConsole');
            this.activateTab('Inspector');
        }

        this._updateEmptyState();
        this.notifyResize();
        this.emit('stateChanged');
    }

    _mountComponent(componentType, paneName, title, closable, factory) {
        if (this.components.has(componentType)) return;

        const pane = this.panes[paneName];
        if (!pane) return;

        const contentDiv = document.createElement('div');
        contentDiv.className = 'ide-component-container';
        contentDiv.id = `cmp-${componentType}`;
        pane.contentEl.appendChild(contentDiv);

        const tabEl = document.createElement('ide-tab');
        tabEl.id = `tab-${componentType}`;
        tabEl.setAttribute('for', componentType);
        tabEl.setAttribute('data-component', componentType);
        tabEl.setAttribute('title', title);
        if (closable) tabEl.setAttribute('closable', '');

        tabEl.addEventListener('tab-select', () => {
            this.activateTab(componentType);
        });
        tabEl.addEventListener('tab-close', () => {
            this.closeComponent(componentType);
        });

        pane.tabsEl.appendChild(tabEl);

        const listeners = new Map();
        const container = {
            element: contentDiv,
            get width() { return contentDiv.clientWidth; },
            get height() { return contentDiv.clientHeight; },
            title: title,
            on: (event, cb) => {
                if (!listeners.has(event)) listeners.set(event, new Set());
                listeners.get(event).add(cb);
            },
            off: (event, cb) => {
                listeners.get(event)?.delete(cb);
            },
            fire: (event, ...args) => {
                const set = listeners.get(event);
                if (set) {
                    for (const cb of set) {
                        try { cb(...args); } catch (e) { console.error(e); }
                    }
                }
            },
            focus: () => this.focusComponent(componentType),
            close: () => this.closeComponent(componentType),
        };

        const comp = {
            componentType,
            paneName,
            title,
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
        return !!comp.contentDiv?.classList.contains('active');
    }

    /**
     * Sets a component's tab and content view active or inactive, updating
     * accessibility attributes and class names.
     */
    _setComponentActive(comp, isActive) {
        if (!comp) return;
        const active = !!isActive;
        comp.tabEl?.setActive(active);
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
        comp.tabEl.style.display = '';
        comp.contentDiv.style.display = '';
        this._setComponentActive(comp, true);
        this.closedComponents.delete(componentType);

        comp.container.fire('show');
        comp.container.fire('open');
        comp.container.fire('resize');

        if (typeof app !== 'undefined') {
            if (componentType === 'LayoutViewer' && app.map) {
                setTimeout(() => app.map.invalidateSize({ animate: false }), 0);
            }
            if (componentType === '3DViewer' && app.threeDViewerWidget) {
                setTimeout(() => app.threeDViewerWidget.onOpen(), 0);
            }
        }

        try {
            comp.tabEl.scrollIntoView({ behavior: 'smooth', inline: 'nearest', block: 'nearest' });
        } catch (_) {}

        this._updateEmptyState();
        this.notifyResize();
        this.emit('stateChanged');
    }

    closeComponent(componentType, autoSelectNext = true) {
        const comp = this.components.get(componentType);
        if (!comp || !comp.closable) return;

        comp.tabEl.style.display = 'none';
        comp.contentDiv.style.display = 'none';
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

        this._updateEmptyState();
        this.notifyResize();
        this.emit('stateChanged');
    }

    focusComponent(componentType) {
        const comp = this.components.get(componentType);
        if (!comp) {
            const factory = this.factories.get(componentType);
            if (factory) {
                const cfg = COMPONENT_CONFIG[componentType] || { pane: 'right', title: componentType, closable: true };
                this._mountComponent(componentType, cfg.pane, cfg.title, cfg.closable, factory);
            } else {
                return;
            }
        }
        this.activateTab(componentType);
    }

    saveLayout() {
        const state = {
            leftWidth: parseInt(this.containerEl.style.getPropertyValue('--pane-left-width') || '240', 10),
            rightWidth: parseInt(this.containerEl.style.getPropertyValue('--pane-right-width') || '340', 10),
            bottomHeight: parseInt(this.containerEl.style.getPropertyValue('--pane-bottom-height') || '220', 10),
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
