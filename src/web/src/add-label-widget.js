// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

export class LabelManager {
    constructor(app) {
        this._app = app;
        this._labels = [];
    }

    addLabelAt(text, color, size, latlng) {
        if (!this._app.map) return;

        // Escape user-supplied text — it is interpolated into innerHTML below.
        const safeText = String(text)
            .replace(/&/g, '&amp;')
            .replace(/</g, '&lt;')
            .replace(/>/g, '&gt;');

        const icon = L.divIcon({
            className: 'custom-leaflet-label',
            html: `<div style="color: ${color}; font-size: ${size}px; font-weight: bold; white-space: nowrap; text-shadow: -1px -1px 0 #000, 1px -1px 0 #000, -1px 1px 0 #000, 1px 1px 0 #000;">${safeText}</div>`,
            iconSize: null,
        });

        const marker = L.marker(latlng, {
            icon,
            draggable: true,
        }).addTo(this._app.map);

        marker.on('mousedown', (e) => {
            if (e.originalEvent.button === 2) {
                e.originalEvent.stopPropagation();
            }
        });

        marker.on('contextmenu', (e) => {
            // Prevent the layout viewer's right-click menu from opening.
            e.originalEvent.preventDefault();
            e.originalEvent.stopPropagation();
            setTimeout(() => {
                marker.remove();
                this._labels = this._labels.filter(m => m !== marker);
            }, 0);
        });

        this._labels.push(marker);
    }

    clearAll() {
        this._labels.forEach(m => m.remove());
        this._labels = [];
    }
}
