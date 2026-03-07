// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Inspector panel — property tree, hover highlights, bbox display.

import { dbuToLatLng, dbuRectToBounds } from './coordinates.js';

export function createInspectorPanel(app, redrawAllLayers) {
    function clearHoverHighlight() {
        for (const r of app.hoverRects) {
            app.map.removeLayer(r);
        }
        app.hoverRects = [];
    }

    function makeClickable(el, selectId) {
        el.classList.add('inspector-link');
        el.addEventListener('click', (e) => {
            e.stopPropagation();
            navigateInspector(selectId);
        });
        el.addEventListener('mouseenter', () => {
            app.websocketManager.request({ type: 'hover', select_id: selectId })
                .then(data => {
                    if (data.rects && app.map && app.designScale) {
                        clearHoverHighlight();
                        const minPx = 20;
                        const zoom = app.map.getZoom();
                        const scale = Math.pow(2, zoom);
                        const minDbu = minPx / (app.designScale * scale);
                        for (const [x1, y1, x2, y2] of data.rects) {
                            const cx = (x1 + x2) / 2;
                            const cy = (y1 + y2) / 2;
                            const hw = Math.max((x2 - x1) / 2, minDbu / 2);
                            const hh = Math.max((y2 - y1) / 2, minDbu / 2);
                            const bounds = dbuRectToBounds(
                                cx - hw, cy - hh, cx + hw, cy + hh,
                                app.designScale, app.designMaxDXDY);
                            app.hoverRects.push(L.rectangle(bounds, {
                                className: 'hover-highlight',
                                color: '#ff0', weight: 3, fill: true,
                                fillColor: '#ff0', fillOpacity: 0.2,
                            }).addTo(app.map));
                        }
                    }
                })
                .catch(() => {});
        });
        el.addEventListener('mouseleave', () => {
            clearHoverHighlight();
        });
    }

    function navigateInspector(selectId) {
        app.websocketManager.request({ type: 'inspect', select_id: selectId })
            .then(data => {
                if (data.error) {
                    console.error('Inspect error:', data.error);
                    return;
                }
                updateInspector(data);

                // Clear hover highlight from the link we just clicked
                clearHoverHighlight();

                // Show popup and highlight on the map
                app.map.closePopup();
                if (app.highlightRect) {
                    app.map.removeLayer(app.highlightRect);
                    app.highlightRect = null;
                }
                if (data.bbox && app.map && app.designScale) {
                    const [x1, y1, x2, y2] = data.bbox;
                    // For non-instance objects, show dashed bbox outline
                    // (instances get the yellow tile-based highlight instead)
                    if (data.type !== 'Inst') {
                        highlightBBox(x1, y1, x2, y2);
                    }
                    // Show a popup at the center of the bbox
                    const center = dbuToLatLng((x1 + x2) / 2, (y1 + y2) / 2,
                                               app.designScale, app.designMaxDXDY);
                    L.popup()
                        .setLatLng(center)
                        .setContent(
                            `<strong>${data.name}</strong><br>` +
                            `<small style="color:#888">${data.type}</small>`)
                        .openOn(app.map);
                }
                // Redraw tiles to update instance highlight
                redrawAllLayers();
            })
            .catch(err => console.error('Inspect failed:', err));
    }

    function highlightBBox(x1, y1, x2, y2) {
        if (app.highlightRect) {
            app.map.removeLayer(app.highlightRect);
        }
        const bounds = dbuRectToBounds(x1, y1, x2, y2, app.designScale, app.designMaxDXDY);
        app.highlightRect = L.rectangle(bounds, {
            color: '#ff0', weight: 2, fill: false, dashArray: '6,4',
        }).addTo(app.map);
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
            const autoExpand = prop.children.length < 10;
            kids.className = 'inspector-group-children' + (autoExpand ? '' : ' collapsed');
            arrow.textContent = autoExpand ? '\u25BC' : '\u25B6';
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
        if (!bbox || !app.map || !app.designScale) return;
        const [x1, y1, x2, y2] = bbox;
        app.map.fitBounds(dbuRectToBounds(x1, y1, x2, y2, app.designScale, app.designMaxDXDY),
                          { padding: [20, 20] });
    }

    function updateInspector(data) {
        if (!app.inspectorEl) return;
        app.inspectorEl.innerHTML = '';

        if (!data || !data.properties || data.properties.length === 0) {
            const placeholder = document.createElement('div');
            placeholder.className = 'stub-panel';
            placeholder.innerHTML =
                '<div class="stub-title">Inspector</div>' +
                '<div class="stub-desc">Select an object in the layout to inspect its properties.</div>';
            app.inspectorEl.appendChild(placeholder);
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
            app.inspectorEl.appendChild(toolbar);
        }

        for (const prop of data.properties) {
            app.inspectorEl.appendChild(renderProperty(prop));
        }
    }

    function createInspector(container) {
        const el = document.createElement('div');
        el.className = 'inspector';
        app.inspectorEl = el;
        container.element.appendChild(el);

        // Show placeholder initially
        updateInspector(null);
    }

    return { createInspector, updateInspector, highlightBBox };
}
